# Makefile for RTOS-Based Multi-Sensor Gateway (STM32L433RC-P)

TARGET = rtos_gateway
DEBUG = 1
OPT = -Og

BUILD_DIR = build

# C Sources
C_SOURCES = \
Core/Src/main.c \
Core/Src/stm32l4xx_it.c \
Drivers/BSP/bsp_nucleo_l433rc.c \
Drivers/MPU6050/mpu6050.c \
Drivers/DHT11/dht11.c \
Drivers/Potentiometer/potentiometer.c \
Middleware/RingBuffer/ring_buffer.c \
App/Src/fault_manager.c \
App/Src/sensor_fusion.c \
App/Src/task_supervisor.c \
App/Src/task_sensors.c \
App/Src/task_processing.c \
App/Src/task_telemetry.c \
App/Src/task_cli.c

# C Includes
C_INCLUDES = \
-ICore/Inc \
-IDrivers/BSP \
-IDrivers/MPU6050 \
-IDrivers/DHT11 \
-IDrivers/Potentiometer \
-IMiddleware/RingBuffer \
-IApp/Inc

# Toolchain definitions
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# CPU & Architecture Flags (ARM Cortex-M4 with single-precision FPU)
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Compiler definitions
C_DEFS = \
-DSTM32L433xx \
-DUSE_HAL_DRIVER \
-DFREERTOS

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Host Test Target
.PHONY: test all clean flash

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

test:
	cmake -B build-test -S tests
	cmake --build build-test
	ctest --test-dir build-test --output-on-failure

clean:
	rm -rf $(BUILD_DIR) build-test

flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x08000000
