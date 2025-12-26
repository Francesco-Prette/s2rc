#!/usr/bin/env python3
"""
Direct Button Test Script
Sends raw UART packets to test specific buttons
"""

import serial
import struct
import sys
import time

# Button definitions (Standard Nintendo Switch HID order)
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
BTN_GL      = (1 << 14)
BTN_GR      = (1 << 15)

# D-Pad values
DPAD_UP        = 0x00
DPAD_UP_RIGHT  = 0x01
DPAD_RIGHT     = 0x02
DPAD_DN_RIGHT  = 0x03
DPAD_DOWN      = 0x04
DPAD_DN_LEFT   = 0x05
DPAD_LEFT      = 0x06
DPAD_UP_LEFT   = 0x07
DPAD_NEUTRAL   = 0x08

def send_packet(ser, buttons, hat, lx=128, ly=128, rx=128, ry=128):
    """Send an 8-byte UART packet"""
    packet = struct.pack('<HBBBBBB', buttons, hat, lx, ly, rx, ry, 0)
    ser.write(packet)
    print(f"Sent: Buttons=0x{buttons:04X}, HAT=0x{hat:02X}, Raw bytes: {' '.join(f'{b:02X}' for b in packet)}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python test_buttons.py <serial_port>")
        print("Example: python test_buttons.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        print("\nTesting buttons one by one...")
        print("Watch the Switch to see which button activates!\n")
        
        tests = [
            ("B button", BTN_B, DPAD_NEUTRAL),
            ("A button", BTN_A, DPAD_NEUTRAL),
            ("Y button", BTN_Y, DPAD_NEUTRAL),
            ("X button", BTN_X, DPAD_NEUTRAL),
            ("L button", BTN_L, DPAD_NEUTRAL),
            ("R button", BTN_R, DPAD_NEUTRAL),
            ("ZL button", BTN_ZL, DPAD_NEUTRAL),
            ("ZR button", BTN_ZR, DPAD_NEUTRAL),
            ("MINUS button", BTN_MINUS, DPAD_NEUTRAL),
            ("PLUS button", BTN_PLUS, DPAD_NEUTRAL),
            ("L-STICK button", BTN_LSTICK, DPAD_NEUTRAL),
            ("R-STICK button", BTN_RSTICK, DPAD_NEUTRAL),
            ("D-Pad UP", 0, DPAD_UP),
            ("D-Pad RIGHT", 0, DPAD_RIGHT),
            ("D-Pad DOWN", 0, DPAD_DOWN),
            ("D-Pad LEFT", 0, DPAD_LEFT),
        ]
        
        for name, button, hat in tests:
            print(f"\n--- Testing: {name} ---")
            send_packet(ser, button, hat)
            time.sleep(0.5)
            
            # Send neutral
            send_packet(ser, 0, DPAD_NEUTRAL)
            time.sleep(0.5)
        
        print("\n\nTest complete!")
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
