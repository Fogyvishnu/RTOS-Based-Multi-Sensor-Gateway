# Makefile for RTOS-Based Multi-Sensor Gateway (STM32L433RC-P)

# Shell configuration
SHELL := /bin/bash

# Target definitions
.PHONY: all firmware flash monitor test clean help

# Default target: build firmware
all: firmware

# Build firmware using PlatformIO
firmware:
	pio run

# Flash firmware to connected Nucleo board via ST-Link
flash:
	pio run --target upload

# Open serial monitor (115200 baud)
monitor:
	pio device monitor -b 115200

# Run host unit test suite on PC (CTest / CMake)
test:
	cmake -B build-test -S tests
	cmake --build build-test
	ctest --test-dir build-test --output-on-failure

# Clean build directories
clean:
	pio run --target clean 2>/dev/null || true
	rm -rf build build-test .pio

# Show help menu
help:
	@echo "=================================================================="
	@echo " RTOS-Based Multi-Sensor Gateway - Build System"
	@echo "=================================================================="
	@echo "  make (or make firmware) : Compile embedded firmware with PlatformIO"
	@echo "  make flash              : Upload firmware to STM32 Nucleo board"
	@echo "  make monitor            : Open interactive serial monitor (115200)"
	@echo "  make test               : Build and run host unit tests (CTest)"
	@echo "  make clean              : Remove build and test output files"
	@echo "=================================================================="
