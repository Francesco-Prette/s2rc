#!/usr/bin/env python3
"""
Button Mapping Diagnostic Tool
Tests each bit position (0-15) to determine actual HORI controller button mapping
"""

import serial
import struct
import sys
import time

def send_packet(ser, buttons, hat=0x08, lx=128, ly=128, rx=128, ry=128):
    """Send an 8-byte UART packet"""
    packet = struct.pack('<HBBBBBB', buttons, hat, lx, ly, rx, ry, 0)
    ser.write(packet)

def main():
    if len(sys.argv) < 2:
        print("Usage: python diagnose_buttons.py <serial_port>")
        print("Example: python diagnose_buttons.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        print("\n" + "="*60)
        print("HORI CONTROLLER BUTTON MAPPING DIAGNOSTIC")
        print("="*60)
        print("\nThis script will test each bit position (0-15) individually.")
        print("Watch your Switch screen and note which button activates!")
        print("Press Enter after each test to continue...\n")
        
        for bit in range(16):
            button_value = (1 << bit)
            print(f"\n--- Testing BIT {bit} (value: 0x{button_value:04X}) ---")
            print(f"Binary: {bin(button_value)}")
            
            # Send button press
            send_packet(ser, button_value)
            print("Button PRESSED - Check Switch screen!")
            
            input("Press Enter when you've noted which button activated...")
            
            # Send neutral
            send_packet(ser, 0)
            time.sleep(0.3)
        
        print("\n" + "="*60)
        print("DIAGNOSTIC COMPLETE!")
        print("="*60)
        print("\nPlease report which Switch button was activated for each bit:")
        print("This will help us create the correct button mapping.\n")
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
