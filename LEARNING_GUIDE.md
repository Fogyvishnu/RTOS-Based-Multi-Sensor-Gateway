# Comprehensive Embedded Systems & FreeRTOS Learning Guide

Welcome to the **RTOS-Based Multi-Sensor Gateway** learning guide. This document is designed as a deep-dive educational companion covering the core embedded engineering concepts, real-time operating system (RTOS) design patterns, peripheral communication protocols, mathematical sensor fusion, and fault-tolerant architecture implemented in this project.

---

## Table of Contents
1. [Target Architecture: ARM Cortex-M4](#1-target-architecture-arm-cortex-m4)
2. [FreeRTOS Fundamentals & Design Patterns](#2-freertos-fundamentals--design-patterns)
3. [Peripheral Communication Protocols](#3-peripheral-communication-protocols)
   - [I2C & The MPU6050 IMU](#i2c--the-mpu6050-imu)
   - [I2C Bus Lockup & 9-Clock Recovery](#i2c-bus-lockup--9-clock-recovery)
   - [1-Wire Protocol & The DHT11 Sensor](#1-wire-protocol--the-dht11-sensor)
   - [ADC & DMA Circular Buffering](#adc--dma-circular-buffering)
   - [UART & Ring Buffers with DMA](#uart--ring-buffers-with-dma)
4. [Sensor Fusion & Math Algorithms](#4-sensor-fusion--math-algorithms)
   - [Why Sensor Fusion?](#why-sensor-fusion)
   - [The Complementary Filter for Attitude Estimation](#the-complementary-filter-for-attitude-estimation)
   - [Dew Point & Heat Index Formulations](#dew-point--heat-index-formulations)
   - [Exponential Moving Average (EWMA) Filter](#exponential-moving-average-ewma-filter)
5. [Watchdogs & System Reliability](#5-watchdogs--system-reliability)
   - [Why Kicking Watchdog in Idle Task is an Anti-Pattern](#why-kicking-watchdog-in-idle-task-is-an-anti-pattern)
   - [Multi-Task Check-in Matrix](#multi-task-check-in-matrix)
6. [Interactive Terminal & CLI Architecture](#6-interactive-terminal--cli-architecture)
7. [Embedded Systems Interview Q&A (Project-Based)](#7-embedded-systems-interview-qa-project-based)

---

## 1. Target Architecture: ARM Cortex-M4

The **STM32L433RC-P** is powered by an **ARM Cortex-M4** core clocked up to 80 MHz.

### Key Hardware Features Used:
- **Harvard Architecture**: Separate instruction and data buses (I-Code, D-Code, System Bus) allow simultaneous instruction fetching and data manipulation.
- **Hardware FPU (Floating Point Unit)**: Single-precision IEEE 754 floating-point hardware (`FPv4-SP-D16`). This accelerates trigonometric calculations (`atan2f`, `sqrtf`, floating-point filters) in single machine cycles rather than slow software emulation.
- **DWT (Data Watchpoint and Trace) Cycle Counter**: A 32-bit hardware register (`DWT->CYCCNT`) running at CPU frequency ($80\text{ MHz} \implies 12.5\text{ ns}$ per tick). It enables nanosecond/microsecond delays without wasting hardware timers, which is critical for 1-wire timing (DHT11).
- **NVIC (Nested Vectored Interrupt Controller)**: Low-latency, deterministic interrupt handling with configurable priority levels (16 levels via 4 priority bits).
- **Low Power & Voltage Scaling**: Ultra-low-power modes with independent peripheral clock domains.

---

## 2. FreeRTOS Fundamentals & Design Patterns

### Task Scheduling & Preemption
FreeRTOS utilizes a **preemptive priority-based scheduler** driven by the SysTick timer (configured to 1000 Hz / 1 ms tick):
- Higher priority tasks always preempt lower priority tasks when unblocked.
- Tasks of identical priority share CPU time via round-robin time-slicing.
- The `IDLE` task (priority 0) runs whenever no application task is ready.

### Inter-Task Communication (IPC)
1. **Queues (`xQueueSend` / `xQueueReceive`)**:
   - Queues copy data **by value**, ensuring thread safety and preventing race conditions without needing manual mutex locks around shared structs.
   - Sensor tasks package structured readings (`SensorPacket_t`) into `sensor_queue`.
2. **Event Groups & Bitmasks**:
   - Lightweight synchronization (e.g., signaling task alive states or fault conditions) without queue allocation overhead.
3. **Mutexes vs. Binary Semaphores**:
   - **Mutexes** include **Priority Inheritance** to prevent *Priority Inversion* (where a medium-priority task delays a high-priority task waiting on a low-priority task). Used to guard shared hardware buses (like I2C).
   - **Binary Semaphores** are used for ISR-to-Task synchronization (signaling DMA complete).

### Memory Management & Safety
- **Stack High Watermark**: Using `uxTaskGetStackHighWaterMark(TaskHandle_t)`, the gateway measures the minimum unused stack space (in 32-bit words) for every task since boot. This enables precise stack sizing and prevents stack overflows.
- **Stack Overflow Hook**: `vApplicationStackOverflowHook` catches overflow events immediately, halts operations, and triggers a safe reset.
- **Heap Allocation (`heap_4.c`)**: Combines adjacent free blocks to prevent heap fragmentation.

---

## 3. Peripheral Communication Protocols

### I2C & The MPU6050 IMU
The **Inter-Integrated Circuit (I2C)** bus is a synchronous, multi-master, multi-slave 2-wire serial bus:
- **SCL**: Serial Clock (driven by master, up to 400 kHz Fast Mode).
- **SDA**: Serial Data (bidirectional, open-drain with pull-up resistors).

#### MPU6050 Communication:
1. **Addressing**: 7-bit slave address `0x68` (when AD0 is GND) or `0x69` (when AD0 is 3.3V). Shifted left by 1 bit for R/W: `0xD0` for write, `0xD1` for read.
2. **Initialization Sequence**:
   - Read register `0x75` (`WHO_AM_I`). Expected value: `0x68`.
   - Write register `0x6B` (`PWR_MGMT_1` = `0x00`) to wake device from sleep.
   - Write register `0x1B` (`GYRO_CONFIG`): Set Full Scale Range ($\pm 250^\circ/\text{s}, \pm 500^\circ/\text{s}, \text{etc.}$).
   - Write register `0x1C` (`ACCEL_CONFIG`): Set Full Scale Range ($\pm 2g, \pm 4g, \text{etc.}$).
3. **Burst Read**: Reading 14 consecutive bytes starting from `0x3B` (`ACCEL_XOUT_H` to `GYRO_ZOUT_L`) in a single I2C transaction to minimize bus overhead.

### I2C Bus Lockup & 9-Clock Recovery
A notorious failure in embedded systems: if the MCU resets while an I2C slave is asserting an `ACK` or transmitting a `0` data bit, the slave will hold **SDA low** indefinitely, waiting for clocks that never arrive. Since the MCU cannot generate a START condition while SDA is low, the bus hangs forever.

#### 9-Clock Bus Clearing Algorithm:
1. Disable peripheral I2C1 and reconfigure SCL and SDA as GPIO open-drain with pull-ups.
2. Check if SDA is low (stuck).
3. If low, pulse the SCL line high and low up to **9 times**.
4. With each clock pulse, the slave shifts out one bit until it releases SDA high.
5. Generate an explicit STOP condition (SCL high while SDA transitions from low to high).
6. Re-initialize I2C1 hardware peripheral.

```
       1   2   3   4   5   6   7   8   9   STOP
SCL: _/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \_/ \____/¯¯¯
SDA: ___________________________________/¯¯¯¯¯¯¯
                                         ^ Slave releases SDA
```

---

### 1-Wire Protocol & The DHT11 Sensor
The DHT11 uses a single bidirectional data wire with open-collector signaling and precise microsecond timing.

#### Handshake & Data Packet:
1. **Start Signal**: Host pulls bus LOW for at least **18 ms**, then pulls HIGH for **20–40 µs** and switches to input mode.
2. **Response Signal**: DHT11 pulls LOW for **80 µs**, then HIGH for **80 µs**.
3. **Data Frame (40 bits / 5 bytes)**:
   - Byte 0: Humidity integer.
   - Byte 1: Humidity decimal (0 on DHT11).
   - Byte 2: Temperature integer.
   - Byte 3: Temperature decimal (0 on DHT11).
   - Byte 4: Checksum (`(Byte 0 + Byte 1 + Byte 2 + Byte 3) & 0xFF == Byte 4`).
4. **Bit Encoding**:
   - Every bit starts with a **50 µs LOW** pulse.
   - A subsequent HIGH pulse of **26–28 µs** represents bit `0`.
   - A subsequent HIGH pulse of **70 µs** represents bit `1`.
5. **Implementation via DWT**: We use `DWT->CYCCNT` to sample high-pulse duration:
   $$\text{duration} > 45\mu\text{s} \implies \text{bit } 1, \quad \text{otherwise } 0$$

---

### ADC & DMA Circular Buffering
- The potentiometer is connected to an analog pin (e.g. `PA0` / ADC1 Channel 5).
- **12-bit SAR ADC**: Converts analog voltages ($0\text{ to }3.3\text{V}$) into digital values ($0\text{ to }4095$).
- **DMA (Direct Memory Access)**:
  - Instead of generating CPU interrupts for every ADC conversion, DMA1 Channel 1 transfers samples continuously into an SRAM circular buffer.
  - The CPU is free to run RTOS tasks without servicing high-frequency ADC interrupts.

---

### UART & Ring Buffers with DMA
- **USART2** connects to the onboard ST-Link debugger, creating a Virtual COM Port (VCP) to the host PC at 115200 baud (8N1).
- **Non-blocking DMA Transmit**:
  - Telemetry frames are formatted into a ping-pong buffer and transmitted via UART DMA.
  - The task unblocks immediately; DMA signals completion via an ISR callback.
- **Ring Buffer (Circular FIFO)**:
  - Incoming CLI keystrokes are captured into a lockless ring buffer in the UART RX interrupt, allowing real-time CLI interaction without dropped characters.

---

## 4. Sensor Fusion & Math Algorithms

### Why Sensor Fusion?
- **Accelerometer**: Measures linear acceleration plus gravity. In static conditions, $\theta = \text{atan2}(a_y, a_z)$ provides an absolute tilt angle. However, vibrations and quick linear motions introduce high-frequency noise.
- **Gyroscope**: Measures angular velocity ($\omega = \frac{d\theta}{dt}$). Integrating velocity ($\theta = \int \omega \, dt$) yields very smooth angles that ignore linear acceleration. However, bias errors integrate over time, causing continuous **drift**.
- **Solution**: Combine the high-frequency response of the gyroscope with the low-frequency drift-free reference of the accelerometer.

### The Complementary Filter for Attitude Estimation
The Complementary Filter applies a High-Pass Filter to the integrated gyro angle and a Low-Pass Filter to the accelerometer angle:

$$\theta_{t} = \alpha \cdot \left(\theta_{t-1} + \omega_{gyro} \cdot \Delta t\right) + (1 - \alpha) \cdot \theta_{accel}$$

Where:
- $\alpha \approx \frac{\tau}{\tau + \Delta t}$ (typically $0.96 \text{ to } 0.98$, where $\tau$ is the filter time constant).
- $\Delta t$: Sampling period (e.g., $0.01\text{ s}$ for 100 Hz).
- Roll ($\phi$) and Pitch ($\theta$) from accelerometer:
  $$\text{Roll} = \text{atan2}(a_y, a_z) \cdot \frac{180^\circ}{\pi}$$
  $$\text{Pitch} = \text{atan2}(-a_x, \sqrt{a_y^2 + a_z^2}) \cdot \frac{180^\circ}{\pi}$$

---

### Dew Point & Heat Index Formulations

#### 1. Dew Point ($T_{dp}$) via the Magnus Formula:
$$\gamma(T, RH) = \frac{a \cdot T}{b + T} + \ln\left(\frac{RH}{100}\right)$$
$$T_{dp} = \frac{b \cdot \gamma(T, RH)}{a - \gamma(T, RH)}$$
Where constants are: $a = 17.27, \; b = 237.7^\circ\text{C}$.

#### 2. Heat Index (Feels-like Temperature):
Based on the simplified Rothfusz regression equation:
$$\text{HI} = 0.5 \cdot \left(T + 61.0 + ((T - 68.0) \cdot 1.2) + (RH \cdot 0.094)\right)$$
(Computed in Fahrenheit, then converted back to Celsius).

---

### Exponential Moving Average (EWMA) Filter
For analog potentiometer readings:
$$y[n] = \beta \cdot x[n] + (1 - \beta) \cdot y[n-1]$$
- With $\beta = 0.2$, high-frequency ADC noise is smoothed out while preserving snappy response to knob turns.

---

## 5. Watchdogs & System Reliability

### Why Kicking Watchdog in Idle Task is an Anti-Pattern
Many novice firmware projects feed the Independent Watchdog (`IWDG`) inside the FreeRTOS `vApplicationIdleHook` or a simple timer callback:
- **Flaw**: If an application task (e.g. `Task_Sensors`) enters an infinite loop at a priority higher than IDLE, the IDLE task never runs, triggering a reset. **BUT**, if the task hangs waiting on a resource or deadlocks with equal/lower priorities while another task keeps the timer running, the watchdog is continually fed even though the system is completely broken!

### Multi-Task Check-in Matrix
In this gateway, we implement a **Supervisor Task Pattern**:
1. Every critical task has a dedicated bit in an alive bitmask (e.g., `TASK_BIT_MPU`, `TASK_BIT_DHT`, `TASK_BIT_PROC`, `TASK_BIT_TELEM`).
2. Each task clears its bit upon normal execution cycle completion.
3. The `Task_Supervisor` runs at the highest priority. Every 200 ms:
   - It checks whether all required task bits were set within their expected deadlines.
   - **Only if all tasks reported healthy** does the supervisor reset the flags and kick the hardware `IWDG`.
   - If any task missed its deadline, the supervisor logs the failing task ID to non-volatile RAM and halts, letting `IWDG` reset the MCU safely.

---

## 6. Interactive Terminal & CLI Architecture

The CLI uses ANSI / VT100 escape codes to provide a live, terminal UI:
- `\x1b[2J\x1b[H`: Clear screen and move cursor to row 1, col 1.
- `\x1b[32m`: Green text (status OK).
- `\x1b[31m`: Red text (status FAULT).
- `\x1b[0m`: Reset colors.

### Commands Supported:
- `status`: Displays current system state, uptime, sensor health, and watchdog state.
- `top`: Prints FreeRTOS task run-time stats, CPU load percentage, and stack watermark.
- `stream <ansi|json|csv>`: Changes live UART telemetry mode.
- `calib`: Zero-calibrates the MPU6050 gyro and accel offsets while stationary.
- `inject_fault <mpu|dht|task>`: Injects artificial hardware or software faults to test recovery.
- `reboot`: Clean software reset via NVIC system reset (`NVIC_SystemReset()`).

---

## 7. Embedded Systems Interview Q&A (Project-Based)

### Q1: What happens if an I2C slave holds SDA low indefinitely, and how does your firmware recover?
> **Answer**: An I2C master cannot initiate a START condition when SDA is pulled low because START requires transitioning SDA from high to low while SCL is high. In our system, the I2C driver detects a timeout, disables the I2C peripheral, reconfigures SCL/SDA as GPIO open-drain, clocks SCL up to 9 times to flush the slave's internal shift register until SDA is released, generates an explicit STOP condition, and re-initializes I2C1.

### Q2: Why did you choose a Complementary Filter over an Extended Kalman Filter (EKF)?
> **Answer**: While an EKF provides optimal estimation under Gaussian noise, it requires matrix inversions and covariance propagation that consume significant CPU cycles and memory. The Complementary Filter achieves $>95\%$ of the orientation accuracy for pitch and roll with minimal floating-point operations, easily executing at 100 Hz on the Cortex-M4 with minimal CPU overhead.

### Q3: How do you prevent Priority Inversion when multiple FreeRTOS tasks access the UART or I2C bus?
> **Answer**: We use FreeRTOS **Mutexes** (`xSemaphoreCreateMutex()`) rather than binary semaphores to guard shared peripherals. FreeRTOS Mutexes implement **Priority Inheritance**: if a low-priority task holds the mutex and a high-priority task attempts to acquire it, the low-priority task temporarily inherits the higher priority, preventing intermediate-priority tasks from preempting it until the mutex is released.

### Q4: How is variable-length packet reception handled over UART without missing bytes?
> **Answer**: We use UART with the **IDLE Line Interrupt** and a lockless **Ring Buffer**. When characters stop arriving on the line for more than 1 character frame time, the hardware triggers an IDLE interrupt, waking the CLI task to parse the complete command string immediately without waiting for a fixed buffer size to fill.
