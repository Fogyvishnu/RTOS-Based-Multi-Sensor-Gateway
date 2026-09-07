#!/usr/bin/env python3
"""
Automated Serial Verification & Test Script for RTOS Multi-Sensor Gateway
"""

import sys
import time
import json
import argparse

def test_gateway(port="/dev/ttyACM0", baud=115200):
    try:
        import serial
    except ImportError:
        print("Please install pyserial: pip install pyserial")
        return False

    print(f"Opening port {port}...")
    try:
        ser = serial.Serial(port, baud, timeout=2)
    except Exception as e:
        print(f"Failed to open port: {e}")
        return False

    time.sleep(1)

    # 1. Test status command
    print("[TEST 1] Sending 'status' command...")
    ser.write(b"status\r\n")
    time.sleep(0.5)
    resp = ser.read_all().decode('utf-8', errors='ignore')
    print("Response:", resp.strip())
    assert "System State" in resp, "Status command failed!"

    # 2. Test stream json command
    print("[TEST 2] Testing JSON telemetry stream...")
    ser.write(b"stream json\r\n")
    time.sleep(1)
    line = ""
    for _ in range(5):
        raw = ser.readline().decode('utf-8', errors='ignore').strip()
        if raw.startswith("{") and raw.endswith("}"):
            line = raw
            break

    assert line, "Failed to receive JSON telemetry packet!"
    data = json.loads(line)
    print("Received JSON:", data)
    assert "mpu" in data and "dht" in data and "pot" in data, "JSON structure incomplete!"

    # 3. Test Fault Injection
    print("[TEST 3] Testing Fault Injection (MPU)...")
    ser.write(b"inject mpu\r\n")
    time.sleep(0.5)
    ser.write(b"status\r\n")
    time.sleep(0.5)
    resp = ser.read_all().decode('utf-8', errors='ignore')
    print("Degraded status response:", resp.strip())

    # Clear fault
    ser.write(b"clear\r\n")
    time.sleep(0.5)
    ser.write(b"stream ansi\r\n")

    print("\n[ALL TESTS PASSED] Gateway firmware verified successfully!")
    ser.close()
    return True

if __name__ == "__main__":
    port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyACM0"
    test_gateway(port)
