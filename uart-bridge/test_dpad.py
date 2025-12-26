#!/usr/bin/env python3
"""
D-Pad Test Script with Packet Header
Tests all 8 D-Pad directions
"""

import serial
import struct
import sys
import time

# D-Pad values (sent directly in lower 4 bits)
DPAD_UP        = 0x00
DPAD_UP_RIGHT  = 0x01
DPAD_RIGHT     = 0x02
DPAD_DN_RIGHT  = 0x03
DPAD_DOWN      = 0x04
DPAD_DN_LEFT   = 0x05
DPAD_LEFT      = 0x06
DPAD_UP_LEFT   = 0x07
DPAD_NEUTRAL   = 0x08  # Neutral state (matching GP2040-CE SWITCH_HAT_NOTHING)

def send_packet(ser, buttons=0, hat=DPAD_NEUTRAL, lx=128, ly=128, rx=128, ry=128):
    """Send a 10-byte packet with byte-by-byte transmission"""
    # Send first header byte
    ser.write(bytes([0xAA]))
    ser.flush()
    time.sleep(0.002)
    
    # Send second header byte
    ser.write(bytes([0x55]))
    ser.flush()
    time.sleep(0.002)
    
    # Send data bytes one at a time
    packet = struct.pack('<HBBBBBB', buttons, hat, lx, ly, rx, ry, 0)
    for byte in packet:
        ser.write(bytes([byte]))
        ser.flush()
        time.sleep(0.001)
    
    print(f"Sent: HAT=0x{hat:02X}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python test_dpad.py <serial_port>")
        print("Example: python test_dpad.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        print("\nTesting D-Pad directions...\n")
        
        tests = [
            ("UP", DPAD_UP),
            ("UP-RIGHT", DPAD_UP_RIGHT),
            ("RIGHT", DPAD_RIGHT),
            ("DOWN-RIGHT", DPAD_DN_RIGHT),
            ("DOWN", DPAD_DOWN),
            ("DOWN-LEFT", DPAD_DN_LEFT),
            ("LEFT", DPAD_LEFT),
            ("UP-LEFT", DPAD_UP_LEFT),
        ]
        
        for name, hat_value in tests:
            print(f"\n--- Testing D-Pad: {name} ---")
            send_packet(ser, hat=hat_value)
            time.sleep(2)
            
            # Send neutral
            print("    Releasing...")
            send_packet(ser, hat=DPAD_NEUTRAL)
            time.sleep(0.5)
        
        print("\n\nD-Pad test complete!")
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
