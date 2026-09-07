# Real-Time Multi-Sensor Gateway (STM32 & FreeRTOS)

[![ARM Cortex-M4](https://img.shields.io/badge/CPU-ARM%20Cortex--M4%20%40%2080MHz-blue.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32l433rc.html)
[![RTOS](https://img.shields.io/badge/RTOS-FreeRTOS-orange.svg)](https://www.freertos.org/)
[![CI](https://github.com/Fogyvishnu/RTOS-Based-Multi-Sensor-Gateway/actions/workflows/ci.yml/badge.svg)](https://github.com/Fogyvishnu/RTOS-Based-Multi-Sensor-Gateway/actions)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)]()

An industrial-grade, fault-tolerant **Real-Time Multi-Sensor Gateway** developed for the **STM32 NUCLEO-L433RC-P** microcontroller using **FreeRTOS**. 

The gateway acquires data from an **MPU6050** 6-DOF IMU (I2C), **DHT11** environmental sensor (1-wire GPIO), and **Potentiometer** (ADC with DMA), applies digital filtering and **sensor fusion** (Complementary filter for Roll/Pitch estimation), monitors system health through a **supervisor watchdog matrix**, and streams telemetry over UART with an interactive **VT100 CLI & Dashboard**.

---

## Key Features & Creative Differentiators

1. **Interactive VT100 / ANSI Live Dashboard & Shell**:
   - Real-time ASCII gauges, 3D attitude artificial horizon, FreeRTOS task runtime statistics (`top`), memory high-watermark monitors, and switchable output formats (`ANSI`, `JSON`, `CSV`).
2. **Sensor Fusion Engine**:
   - Complementary filter executing at 100 Hz on the ARM Cortex-M4 hardware FPU, combining accelerometer and gyroscope inputs for drift-free Roll/Pitch attitude estimation.
   - Microclimate analysis: Magnus formula for Dew Point and NOAA Rothfusz regression for Heat Index (feels-like temperature).
   - Exponentially Weighted Moving Average (EWMA) digital low-pass filtering for analog ADC sampling.
3. **Resilient Fault Management & Self-Healing**:
   - **I2C 9-Clock Bus Recovery**: Detects I2C bus lockups (e.g. slave holding SDA low) and executes an automatic 9-clock bus-clearing sequence to restore communication without rebooting the system.
   - **Degraded Operating Modes**: Dynamic state machine (`NORMAL` $\rightarrow$ `DEGRADED` $\rightarrow$ `CRITICAL`). If one sensor fails, the gateway continues streaming healthy sensors with explicit error telemetry.
   - **Multi-Task Watchdog Matrix**: A dedicated supervisor task monitors execution heartbeats from all worker tasks. The hardware Independent Watchdog (`IWDG`) is kicked *only* if all critical tasks report healthy within their deadlines.
   - **Interactive Fault Injection**: Test fault tolerance live during interviews or demos using CLI commands (`inject mpu`, `inject dht`, `inject pot`, `inject task`).
4. **Comprehensive Educational Guide**:
   - Includes [`LEARNING_GUIDE.md`](./LEARNING_GUIDE.md), detailing ARM Cortex-M4 internals, FreeRTOS scheduling, peripheral mechanics, mathematical sensor fusion, and project-based interview Q&As.
5. **Dual-Target Architecture**:
   - Embedded target firmware for STM32L433RC-P.
   - Host-based unit test suite (CMake / CTest) runnable on Linux/macOS/Windows without hardware attached.
6. **Companion Python Visualizer**:
   - Real-time desktop terminal visualizer ([`tools/dashboard.py`](./tools/dashboard.py)) with auto-detection of ST-Link serial ports and built-in simulation demo mode (`--demo`).

---

## System Architecture

```mermaid
graph TD
    subgraph Hardware Layer
        MPU[MPU6050 IMU - I2C1]
        DHT[DHT11 Sensor - GPIO]
        POT[Potentiometer - ADC1]
        UART_HW[ST-Link VCP - USART2 DMA]
        WDT_HW[Hardware IWDG]
    end

    subgraph Driver & HAL Layer
        I2C_DRV[I2C Driver + 9-Clock Recovery]
        DHT_DRV[DHT11 Single-Wire Driver]
        ADC_DRV[ADC Circular DMA Driver]
        UART_DRV[UART RingBuffer + DMA TX]
    end

    subgraph FreeRTOS Core & Tasks
        T_MPU[Task MPU6050 - 100 Hz]
        T_DHT[Task DHT11 - 0.5 Hz]
        T_ADC[Task Analog/Pot - 50 Hz]
        
        Q_RAW[(Sensor Data Queue)]
        
        T_PROC[Task Data Fusion & Processing - 50 Hz<br/>Complementary Filter | State Machine]
        
        Q_TELEM[(Telemetry Queue)]
        
        T_GATEWAY[Task Gateway & Telemetry - 10 Hz<br/>JSON / ANSI / CSV Stream]
        
        T_CLI[Task CLI Shell - Event-driven<br/>Interactive Shell | Fault Injection]
        
        T_SUP[Task Supervisor - Priority 5<br/>Check-in Matrix | IWDG Refresh]
    end

    MPU --> I2C_DRV --> T_MPU
    DHT --> DHT_DRV --> T_DHT
    POT --> ADC_DRV --> T_ADC
    
    T_MPU --> Q_RAW
    T_DHT --> Q_RAW
    T_ADC --> Q_RAW
    
    Q_RAW --> T_PROC
    T_PROC --> Q_TELEM
    Q_TELEM --> T_GATEWAY
    T_GATEWAY --> UART_DRV --> UART_HW
    
    UART_HW --> UART_DRV --> T_CLI
    
    T_MPU -. Check-in .-> T_SUP
    T_DHT -. Check-in .-> T_SUP
    T_ADC -. Check-in .-> T_SUP
    T_PROC -. Check-in .-> T_SUP
    T_GATEWAY -. Check-in .-> T_SUP
    T_SUP --> WDT_HW
```

---

## Hardware Pinout & Wiring Guide (Nucleo-L433RC-P)

| Signal | STM32 Pin | Connected Device | Wiring / Pin Description |
| :--- | :--- | :--- | :--- |
| **I2C1 SCL** | `PB8` | MPU6050 SCL | Connect to SCL on MPU6050 (4.7kΩ pull-up to 3.3V) |
| **I2C1 SDA** | `PB9` | MPU6050 SDA | Connect to SDA on MPU6050 (4.7kΩ pull-up to 3.3V) |
| **MPU VCC/GND**| `3V3 / GND` | MPU6050 VCC/GND | 3.3V supply and common ground |
| **DHT11 DATA** | `PA1` | DHT11 DATA pin | Bidirectional data line (4.7kΩ pull-up to 3.3V) |
| **DHT11 Power**| `3V3 / GND` | DHT11 VCC/GND | 3.3V supply and common ground |
| **ADC1_IN5** | `PA0` (A0) | Potentiometer | Wiper (center pin) of 10kΩ potentiometer |
| **Pot Power** | `3V3 / GND` | Potentiometer | Outer legs connected to 3.3V and GND |
| **USART2 TX/RX**| `PA2 / PA3` | ST-Link Debugger | Internal to Nucleo board (Virtual COM Port) |
| **User LED** | `PB13` (LD4) | Green LED | Onboard status heartbeat |
| **User Button** | `PC13` (B1) | Blue Button | Mode switch / hardware interaction |

---

## FreeRTOS Task Design & Priorities

| Task Name | Priority | Period / Rate | Stack Size | Primary IPC Mechanism | Responsibility |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`Supervisor`** | 5 (Highest) | 200 ms | 256 words | Atomic bitmask | Verifies task check-in deadlines; kicks `IWDG`; toggles status LED. |
| **`Processing`** | 4 | 20 ms (50 Hz) | 512 words | `sensor_queue` $\rightarrow$ `telemetry_queue` | Runs Complementary filter; calculates Roll/Pitch; updates state machine. |
| **`MPU6050`** | 4 | 10 ms (100 Hz) | 384 words | `sensor_queue` (send) | 14-byte burst read; checks WHO_AM_I; initiates I2C bus recovery if hung. |
| **`ADC_Pot`** | 3 | 20 ms (50 Hz) | 256 words | `sensor_queue` (send) | Reads ADC1 circular DMA buffer; runs EWMA digital filter. |
| **`Telemetry`** | 3 | 100 ms (10 Hz) | 512 words | `telemetry_queue` (recv) | Formats ANSI / JSON / CSV; triggers non-blocking UART DMA transmit. |
| **`DHT11`** | 2 | 2000 ms (0.5 Hz)| 256 words | `sensor_queue` (send) | 1-wire timing with DWT; 40-bit frame decode; checksum check. |
| **`CLI`** | 2 | Event-driven | 512 words | Ring Buffer (UART RX) | Non-blocking command shell; handles user input and fault injection. |

---

## Live VT100 Terminal Interface Preview

When connected via any standard serial terminal (e.g. `picocom -b 115200 /dev/ttyACM0` or PuTTY):

```
================================================================================
       STM32L433 REAL-TIME MULTI-SENSOR GATEWAY (FreeRTOS ARM Cortex-M4)       
================================================================================
 State: [NORMAL]  | Uptime: 00:04:22 | Heap Free: 21450 B | Faults: 0x0000
 Watchdog: [ARMED] | I2C Bus Recoveries: 0
--------------------------------------------------------------------------------
 [MPU6050] Accel (g):   X:+0.02  Y:-0.03  Z:+0.99 | Status: OK
           Gyro (dps):  X:+0.1   Y:-0.2   Z:+0.0  | Die Temp: 26.2 C
           Orientation: Roll:   -1.7 deg  [--------|---#------]
                        Pitch:  +0.9 deg  [---------#-^-------]
--------------------------------------------------------------------------------
 [DHT11]   Temp: 24.5 C | Humidity: 55.0 % | Dew Point: 14.8 C | Feels-Like: 24.7 C
           Status: OK
--------------------------------------------------------------------------------
 [POT]     ADC Raw: 2048 (1.65 V)  [==========          ]  50.0 %
================================================================================
 CLI Commands: type 'help' for command list | 'stream [json|ansi|csv]'
 CLI> 
```

---

## Interactive CLI Commands

Type any of the following into the serial console:

| Command | Description | Example Output |
| :--- | :--- | :--- |
| `help` | Prints available command list and usage | List of commands |
| `status` | Reports current state, active fault bits, and bus recoveries | `System State: NORMAL (Code 1), Fault Mask: 0x0000` |
| `stream json` | Switches UART telemetry to continuous JSON stream | `{"uptime":15,"state":"NORMAL","mpu":{...}}` |
| `stream ansi` | Switches UART telemetry to interactive VT100 dashboard | Live terminal interface |
| `stream csv` | Switches UART telemetry to CSV format | `15000,1,0000,-1.7,0.9,24.5,55.0,1.65,50.0` |
| `top` | Prints FreeRTOS task runtime statistics and stack watermarks | Task list with stack high-watermarks |
| `inject mpu` | Injects MPU6050 communication timeout (tests bus recovery) | `[INJECTED] MPU6050 communication timeout fault` |
| `inject dht` | Injects DHT11 checksum failure (tests degraded mode) | `[INJECTED] DHT11 checksum failure fault` |
| `inject pot` | Injects out-of-bounds ADC reading | `[INJECTED] Potentiometer ADC fault` |
| `inject task` | Injects task starvation (demonstrates supervisor watchdog) | `[INJECTED] Task starvation fault` |
| `clear` | Clears all artificially injected faults | `[CLEARED] All faults cleared` |
| `reboot` | Triggers clean MCU software reset via `NVIC_SystemReset()` | `[REBOOT] Initiating system reset...` |

---

## Getting Started & Build Instructions

### 1. Host Unit Tests (Run on Linux/macOS without hardware)
Verify sensor fusion math, EWMA digital filtering, Magnus dew point formulas, and fault management state machine:
```bash
# Configure and run unit tests
cmake -B build-test -S tests
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

### 2. Building & Flashing via PlatformIO
```bash
# Build firmware
pio run

# Flash to Nucleo board
pio run --target upload

# Open serial monitor
pio device monitor -b 115200
```

### 3. Building via GNU ARM Toolchain & Makefile
```bash
# Compile firmware
make -j4

# Flash via OpenOCD or st-flash
make flash
```

### 4. Running the Python Companion Dashboard
```bash
# Run in standalone demo mode (no hardware required!)
python3 tools/dashboard.py --demo

# Connect to real Nucleo board over serial
python3 tools/dashboard.py --port /dev/ttyACM0 --baud 115200
```

---

## Educational Deep-Dive

For a complete technical explanation of the engineering concepts used in this project, read [`LEARNING_GUIDE.md`](./LEARNING_GUIDE.md). It covers:
- **ARM Cortex-M4**: Harvard architecture, FPU, DWT cycle counter.
- **FreeRTOS Internals**: Priority scheduling, IPC queues, priority inversion & mutex inheritance, stack high-watermark sizing.
- **Peripheral Protocols**: I2C bus hang mechanics & 9-clock unsticking, 1-wire microsecond pulse capture, ADC circular DMA.
- **Mathematical Formulations**: Complementary filter derivation, Magnus dew point formula, NOAA heat index.
- **System Safety**: Why kicking watchdogs in Idle tasks is an anti-pattern, supervisor check-in matrices.
- **Embedded Interview Questions & Answers**: Common questions based on this architecture.
