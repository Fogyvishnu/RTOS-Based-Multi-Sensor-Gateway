#!/usr/bin/env python3
"""
Interactive Companion Dashboard for RTOS-Based Multi-Sensor Gateway
Visualizes real-time telemetry from STM32 NUCLEO-L433RC-P over Serial/UART.
"""

import sys
import time
import json
import math
import argparse

def draw_bar(val, min_val, max_val, width=20, fill_char="=", empty_char=" "):
    val = max(min_val, min(max_val, val))
    ratio = (val - min_val) / (max_val - min_val) if max_val > min_val else 0
    filled = int(ratio * width)
    return f"[{fill_char * filled}{empty_char * (width - filled)}]"

def draw_horizon(angle, width=20):
    angle = max(-90.0, min(90.0, angle))
    center = width // 2
    ratio = angle / 90.0
    pos = center + int(ratio * center)
    pos = max(0, min(width - 1, pos))
    
    chars = ["-"] * width
    chars[center] = "|"
    chars[pos] = "#" if pos == center else "^"
    return f"[{''.join(chars)}]"

def render_dashboard(data):
    # Clear screen and jump home
    sys.stdout.write("\033[2J\033[H")
    
    uptime = data.get("uptime", 0)
    state = data.get("state", "UNKNOWN")
    faults = data.get("faults", 0)
    i2c_rec = data.get("i2c_rec", 0)
    
    mpu = data.get("mpu", {})
    dht = data.get("dht", {})
    pot = data.get("pot", {})
    
    roll = mpu.get("roll", 0.0)
    pitch = mpu.get("pitch", 0.0)
    temp = dht.get("temp", 0.0)
    hum = dht.get("hum", 0.0)
    dew = dht.get("dew", 0.0)
    hi = dht.get("hi", 0.0)
    
    pot_raw = pot.get("raw", 0)
    pot_v = pot.get("v", 0.0)
    pot_pct = pot.get("pct", 0.0)
    
    # State color
    color_green = "\033[1;32m"
    color_yellow = "\033[1;33m"
    color_red = "\033[1;31m"
    color_cyan = "\033[1;36m"
    color_reset = "\033[0m"
    
    if state == "NORMAL":
        state_badge = f"{color_green}[ NORMAL ]{color_reset}"
    elif state == "DEGRADED":
        state_badge = f"{color_yellow}[ DEGRADED ]{color_reset}"
    else:
        state_badge = f"{color_red}[ {state} ]{color_reset}"
        
    hrs = uptime // 3600
    mins = (uptime % 3600) // 60
    secs = uptime % 60
    
    print(f"{color_cyan}================================================================================{color_reset}")
    print(f"        STM32L433 REAL-TIME MULTI-SENSOR GATEWAY - PYTHON LIVE DASHBOARD        ")
    print(f"{color_cyan}================================================================================{color_reset}")
    print(f" System State: {state_badge}  | Uptime: {hrs:02d}:{mins:02d}:{secs:02d} | Fault Mask: 0x{faults:04X}")
    print(f" Watchdog: {color_green}[ACTIVE]{color_reset}    | I2C Bus Recoveries: {i2c_rec}")
    print(f"--------------------------------------------------------------------------------")
    print(f" [MPU6050 6-DOF IMU]")
    print(f"   Accel (g):  X={mpu.get('ax', 0.0):+0.2f}  Y={mpu.get('ay', 0.0):+0.2f}  Z={mpu.get('az', 0.0):+0.2f}")
    print(f"   Gyro (dps): X={mpu.get('gx', 0.0):+0.1f}  Y={mpu.get('gy', 0.0):+0.1f}  Z={mpu.get('gz', 0.0):+0.1f}")
    print(f"   Orientation:")
    print(f"     Roll:  {roll:+6.1f}°  {draw_horizon(roll, 24)}")
    print(f"     Pitch: {pitch:+6.1f}°  {draw_horizon(pitch, 24)}")
    print(f"--------------------------------------------------------------------------------")
    print(f" [DHT11 Environmental Sensor]")
    print(f"   Temperature: {temp:5.1f} °C  {draw_bar(temp, 0.0, 50.0, 20)} (0-50°C)")
    print(f"   Humidity:    {hum:5.1f} %   {draw_bar(hum, 0.0, 100.0, 20)} (0-100%)")
    print(f"   Dew Point:   {dew:5.1f} °C  | Feels-Like: {hi:5.1f} °C")
    print(f"--------------------------------------------------------------------------------")
    print(f" [Potentiometer Analog Input]")
    print(f"   ADC: {pot_raw:4d} / 4095  ({pot_v:0.2f} V)  {draw_bar(pot_pct, 0.0, 100.0, 24)} {pot_pct:5.1f} %")
    print(f"{color_cyan}================================================================================{color_reset}")
    print(" Press Ctrl+C to quit. Send 'help' or 'stream ansi' via serial terminal.")
    sys.stdout.flush()

def run_demo():
    print("Starting simulation/demo mode...")
    t = 0
    uptime = 0
    while True:
        try:
            roll = 25.0 * math.sin(t * 0.1)
            pitch = 15.0 * math.cos(t * 0.08)
            pot_pct = 50.0 + 40.0 * math.sin(t * 0.05)
            mock_data = {
                "uptime": uptime,
                "state": "NORMAL",
                "faults": 0,
                "mpu": {
                    "ax": 0.02 * math.sin(t),
                    "ay": -0.05 * math.cos(t),
                    "az": 0.98,
                    "gx": 0.5 * math.cos(t),
                    "gy": -0.3 * math.sin(t),
                    "gz": 0.0,
                    "roll": roll,
                    "pitch": pitch
                },
                "dht": {
                    "temp": 24.5 + math.sin(t * 0.02) * 2.0,
                    "hum": 55.0 + math.cos(t * 0.02) * 5.0,
                    "dew": 14.8,
                    "hi": 24.7
                },
                "pot": {
                    "raw": int((pot_pct / 100.0) * 4095),
                    "v": (pot_pct / 100.0) * 3.3,
                    "pct": pot_pct
                },
                "i2c_rec": 0
            }
            render_dashboard(mock_data)
            t += 1
            uptime += 1
            time.sleep(0.1)
        except KeyboardInterrupt:
            print("\nExiting dashboard demo.")
            break

def run_serial(port, baud):
    try:
        import serial
    except ImportError:
        print("Error: 'pyserial' package not found. Install via: pip install pyserial")
        sys.exit(1)

    print(f"Connecting to {port} @ {baud} baud...")
    ser = serial.Serial(port, baud, timeout=1)
    
    # Request JSON streaming
    ser.write(b"stream json\r\n")
    
    while True:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line.startswith("{") and line.endswith("}"):
                data = json.loads(line)
                render_dashboard(data)
            elif line:
                # Direct print if raw text
                pass
        except KeyboardInterrupt:
            print("\nDisconnecting...")
            break
        except Exception:
            pass

def main():
    parser = argparse.ArgumentParser(description="Live Dashboard for RTOS Multi-Sensor Gateway")
    parser.add_argument("--port", default="/dev/ttyACM0", help="Serial port (e.g. /dev/ttyACM0, COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--demo", action="store_true", help="Run in simulation/demo mode without hardware")
    args = parser.parse_args()

    if args.demo:
        run_demo()
    else:
        try:
            run_serial(args.port, args.baud)
        except Exception as e:
            print(f"Could not open {args.port} ({e}). Running in demo mode instead...\n")
            time.sleep(1)
            run_demo()

if __name__ == "__main__":
    main()
