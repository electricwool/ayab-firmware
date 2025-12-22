#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simple serial monitor for encoder test firmware
Usage: python monitor_encoder.py COM11 115200
"""

import sys
import time
import io

# Force UTF-8 output on Windows
if sys.platform == 'win32':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')

def monitor_serial(port, baudrate):
    """Monitor serial port and print output"""
    import serial
    import serial.tools.list_ports
    
    ser = None
    try:
        print(f"Opening {port} at {baudrate} baud...")
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected! Press Ctrl+C to exit.\n")
        
        # Wait a moment for connection to stabilize
        time.sleep(0.5)
        
        # Clear any buffered data
        ser.reset_input_buffer()
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='replace').rstrip()
                    print(line)
                except UnicodeDecodeError:
                    pass  # Skip lines that can't be decoded
                    
    except serial.SerialException as e:
        print(f"Error: {e}")
        print(f"\nAvailable ports:")
        ports = serial.tools.list_ports.comports()
        for p in ports:
            print(f"  {p.device}: {p.description}")
        return 1
        
    except KeyboardInterrupt:
        print("\n\nDisconnected.")
        return 0
        
    finally:
        if ser is not None and ser.is_open:
            ser.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python monitor_encoder.py <PORT> [BAUDRATE]")
        print("Example: python monitor_encoder.py COM11 115200")
        sys.exit(1)
    
    port = sys.argv[1]
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200
    
    sys.exit(monitor_serial(port, baudrate))
