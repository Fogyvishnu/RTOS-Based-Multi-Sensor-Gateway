#!/usr/bin/env python3
"""
Live Hardware Diagnostic Test Script for RTOS Multi-Sensor Gateway
"""

import serial
import time
import sys

def main():
    port = "/dev/ttyACM0"
    baud = 115200

    print(f"[*] Opening serial port {port} @ {baud}...")
    try:
        ser = serial.Serial(port, baud, timeout=1.0)
    except Exception as e:
        print(f"[-] Failed to open {port}: {e}")
        sys.exit(1)

    # Flush input buffer
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print("[*] Listening for incoming data for 3 seconds...")
    t_end = time.time() + 3.0
    received = b""
    while time.time() < t_end:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            received += chunk

    print(f"[*] Total bytes received on boot: {len(received)}")
    if received:
        print("--- RAW DATA START ---")
        print(received.decode('utf-8', errors='replace'))
        print("--- RAW DATA END ---")

    # Send help command
    print("\n[*] Sending 'help' command to CLI...")
    ser.reset_input_buffer()
    ser.write(b"help\r\n")
    ser.flush()
    time.sleep(0.3)

    resp = ser.read(ser.in_waiting or 400)
    print(f"[*] CLI Response to 'help': ({len(resp)} bytes)")
    if resp:
        print(resp.decode('utf-8', errors='replace'))

    # Send status command
    print("\n[*] Sending 'status' command to CLI...")
    ser.reset_input_buffer()
    ser.write(b"status\r\n")
    ser.flush()
    time.sleep(0.3)

    resp2 = ser.read(ser.in_waiting or 400)
    print(f"[*] CLI Response to 'status': ({len(resp2)} bytes)")
    if resp2:
        print(resp2.decode('utf-8', errors='replace'))

    # Send stream json command
    print("\n[*] Sending 'stream json' command to CLI...")
    ser.reset_input_buffer()
    ser.write(b"stream json\r\n")
    ser.flush()
    time.sleep(1.0)

    resp3 = ser.read(ser.in_waiting or 600)
    print(f"[*] Stream JSON Response: ({len(resp3)} bytes)")
    if resp3:
        print(resp3.decode('utf-8', errors='replace'))

    # Return to ANSI mode
    print("\n[*] Restoring ANSI streaming mode...")
    ser.write(b"stream ansi\r\n")
    ser.flush()
    time.sleep(0.3)

    ser.close()
    print("\n[*] Hardware test finished successfully.")

if __name__ == "__main__":
    main()
