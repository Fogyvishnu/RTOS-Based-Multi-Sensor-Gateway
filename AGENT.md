# RTOS-Based Multi-Sensor Gateway

## 1. Project Overview

This project is a professional embedded systems portfolio project designed to demonstrate practical skills in:

- Embedded C
- STM32 development
- ARM Cortex-M4
- FreeRTOS
- UART
- I2C
- ADC
- GPIO
- Interrupts
- DMA
- Watchdog timers
- Inter-task communication
- Embedded debugging

The system is a **Real-Time Multi-Sensor Gateway**.

It collects data from multiple sensors, processes the information using FreeRTOS tasks, detects communication or sensor failures, and sends telemetry to a PC through UART.

---

# 2. Project Goal

Build a modular embedded system capable of collecting data from:

- MPU6050
- DHT11
- Potentiometer

The STM32 should:

1. Acquire sensor data.
2. Validate sensor communication.
3. Process raw sensor values.
4. Exchange data between FreeRTOS tasks.
5. Detect faults.
6. Send telemetry through UART.
7. Monitor system health.
8. Support future DMA and watchdog functionality.

# 3. Hardware 

- STM32 NUCLEO L433RCP
- MPU6050
- DHT11
- Potentiometer

