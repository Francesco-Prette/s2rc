#!/usr/bin/env python3
"""
Corrected Button Test Script
Based on diagnostic findings: System buttons are in bits 0-5
Face/shoulder buttons should be in bits 6-13 (to be confirmed)
"""

import serial
import struct
import sys
import time

# CORRECTED Button definitions based on diagnostic
BTN_MINUS   = (1 << 0)  # Confirmed: Bit 0 = MINUS
BTN_PLUS    = (1 << 1)  # Confirmed: Bit 1 = PLUS
BTN_LSTICK  = (1 << 2)  # Confirmed: Bit 2 = L-Stick Click
BTN_RSTICK  = (1 << 3)  # Confirmed: Bit 3 = R-Stick Click
BTN_HOME    = (1 << 4)  # Confirmed: Bit 4 = HOME
BTN_CAPTURE = (1 << 5)  # Confirmed: Bit 5 = CAPTURE

# Face and shoulder buttons - GUESSING positions 6-13
# These need to be verified with TEST_MODE or further diagnostics
BTN_B       = (1 << 6)   # Guess: Bit 6
BTN_A       = (1 << 7)   # Guess: Bit 7
BTN_Y       = (1 << 8)   # Guess: Bit 8
BTN_X       = (1 << 9)   # Guess: Bit 9
BTN_L       = (1 << 10)  # Guess: Bit 10
BTN_R       = (1 << 11)  # Guess: Bit 11
BTN_ZL      = (1 << 12)  # Guess: Bit 12
BTN_ZR      = (1 << 13)  # Guess: Bit 13

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
        print("Usage: python test_buttons_corrected.py <serial_port>")
        print("Example: python test_buttons_corrected.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        print("\n" + "="*60)
        print("CORRECTED BUTTON TEST (Based on Diagnostic)")
        print("="*60)
        print("\nConfirmed working buttons:")
        print("  Bit 0 = MINUS (-)")
        print("  Bit 1 = PLUS (+)")
        print("  Bit 2 = L-Stick Click")
        print("  Bit 3 = R-Stick Click")
        print("  Bit 4 = HOME")
        print("  Bit 5 = CAPTURE")
        print("\nTesting face/shoulder buttons (guessing bits 6-13)...\n")
        
        tests = [
            ("MINUS button (confirmed)", BTN_MINUS, DPAD_NEUTRAL),
            ("PLUS button (confirmed)", BTN_PLUS, DPAD_NEUTRAL),
            ("L-STICK button (confirmed)", BTN_LSTICK, DPAD_NEUTRAL),
            ("R-STICK button (confirmed)", BTN_RSTICK, DPAD_NEUTRAL),
            ("B button (guess: bit 6)", BTN_B, DPAD_NEUTRAL),
            ("A button (guess: bit 7)", BTN_A, DPAD_NEUTRAL),
            ("Y button (guess: bit 8)", BTN_Y, DPAD_NEUTRAL),
            ("X button (guess: bit 9)", BTN_X, DPAD_NEUTRAL),
            ("L button (guess: bit 10)", BTN_L, DPAD_NEUTRAL),
            ("R button (guess: bit 11)", BTN_R, DPAD_NEUTRAL),
            ("ZL button (guess: bit 12)", BTN_ZL, DPAD_NEUTRAL),
            ("ZR button (guess: bit 13)", BTN_ZR, DPAD_NEUTRAL),
            ("D-Pad UP", 0, DPAD_UP),
            ("D-Pad RIGHT", 0, DPAD_RIGHT),
            ("D-Pad DOWN", 0, DPAD_DOWN),
            ("D-Pad LEFT", 0, DPAD_LEFT),
        ]
        
        for name, button, hat in tests:
            print(f"\n--- Testing: {name} ---")
            send_packet(ser, button, hat)
            time.sleep(1.5)
            
            # Send neutral
            send_packet(ser, 0, DPAD_NEUTRAL)
            time.sleep(0.5)
        
        print("\n\nTest complete!")
        print("\nIMPORTANT: Please report which buttons activated for bits 6-13!")
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
