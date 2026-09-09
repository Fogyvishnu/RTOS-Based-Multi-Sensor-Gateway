#!/usr/bin/env python3
import serial
import time
import sys

def main():
    port = "/dev/ttyACM0"
    baud = 115200

    print(f"[*] Connecting to {port} @ {baud}...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except Exception as e:
        print(f"[-] Port error: {e}")
        sys.exit(1)

    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print("[*] Listening for 3 seconds...")
    t_end = time.time() + 3.0
    while time.time() < t_end:
        b = ser.read(ser.in_waiting or 1)
        if b:
            sys.stdout.write(b.decode('utf-8', errors='replace'))
            sys.stdout.flush()

    print("\n\n[*] ---> Sending 'help\\r\\n'")
    ser.write(b"help\r\n")
    ser.flush()

    t_end = time.time() + 1.5
    while time.time() < t_end:
        b = ser.read(ser.in_waiting or 1)
        if b:
            sys.stdout.write(b.decode('utf-8', errors='replace'))
            sys.stdout.flush()

    print("\n\n[*] ---> Sending 'status\\r\\n'")
    ser.write(b"status\r\n")
    ser.flush()

    t_end = time.time() + 1.5
    while time.time() < t_end:
        b = ser.read(ser.in_waiting or 1)
        if b:
            sys.stdout.write(b.decode('utf-8', errors='replace'))
            sys.stdout.flush()

    print("\n\n[*] ---> Sending 'stream json\\r\\n'")
    ser.write(b"stream json\r\n")
    ser.flush()

    t_end = time.time() + 2.0
    while time.time() < t_end:
        b = ser.read(ser.in_waiting or 1)
        if b:
            sys.stdout.write(b.decode('utf-8', errors='replace'))
            sys.stdout.flush()

    print("\n\n[*] ---> Restoring 'stream ansi\\r\\n'")
    ser.write(b"stream ansi\r\n")
    ser.flush()

    t_end = time.time() + 1.5
    while time.time() < t_end:
        b = ser.read(ser.in_waiting or 1)
        if b:
            sys.stdout.write(b.decode('utf-8', errors='replace'))
            sys.stdout.flush()

    ser.close()
    print("\n[*] Interactive test complete.")

if __name__ == "__main__":
    main()
