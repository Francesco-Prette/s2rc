#!/usr/bin/env python3
"""
Button Test Script with Packet Header
Adds 0xAA 0x55 header for packet synchronization
"""

import serial
import struct
import sys
import time

# Button definitions
BTN_B       = (1 << 0)
BTN_A       = (1 << 1)
BTN_Y       = (1 << 2)
BTN_X       = (1 << 3)
BTN_L       = (1 << 4)
BTN_R       = (1 << 5)
BTN_ZL      = (1 << 6)
BTN_ZR      = (1 << 7)
BTN_MINUS   = (1 << 8)
BTN_PLUS    = (1 << 9)
BTN_LSTICK  = (1 << 10)
BTN_RSTICK  = (1 << 11)
BTN_HOME    = (1 << 12)
BTN_CAPTURE = (1 << 13)

DPAD_NEUTRAL = 0x08

def send_packet(ser, buttons, hat=DPAD_NEUTRAL, lx=128, ly=128, rx=128, ry=128):
    """Send a 10-byte packet: [0xAA, 0x55, buttons_low, buttons_high, hat, lx, ly, rx, ry, vendor]"""
    # Send first header byte
    ser.write(bytes([0xAA]))
    ser.flush()
    time.sleep(0.002)
    
    # Send second header byte
    ser.write(bytes([0x55]))
    ser.flush()
    time.sleep(0.002)
    
    # Send data bytes one at a time with small delays
    packet = struct.pack('<HBBBBBB', buttons, hat, lx, ly, rx, ry, 0)
    for byte in packet:
        ser.write(bytes([byte]))
        ser.flush()
        time.sleep(0.001)
    
    print(f"Sent: Buttons=0x{buttons:04X}, HAT=0x{hat:02X}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python test_buttons_with_header.py <serial_port>")
        print("Example: python test_buttons_with_header.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        print("\nTesting buttons with packet headers...")
        print("This requires updated firmware that supports packet headers!\n")
        
        tests = [
            ("B", BTN_B),
            ("A", BTN_A),
            ("Y", BTN_Y),
            ("X", BTN_X),
            ("L", BTN_L),
            ("R", BTN_R),
            ("ZL", BTN_ZL),
            ("ZR", BTN_ZR),
            ("MINUS (-)", BTN_MINUS),
            ("PLUS (+)", BTN_PLUS),
            ("L-STICK", BTN_LSTICK),
            ("R-STICK", BTN_RSTICK),
            ("HOME", BTN_HOME),
            ("CAPTURE", BTN_CAPTURE),
        ]
        
        for name, button in tests:
            print(f"\n--- Testing: {name} ---")
            send_packet(ser, button)
            time.sleep(1.5)
            
            # Send neutral
            send_packet(ser, 0)
            time.sleep(0.5)
        
        print("\n\nTest complete!")
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
