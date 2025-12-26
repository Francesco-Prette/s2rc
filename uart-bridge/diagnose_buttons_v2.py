#!/usr/bin/env python3
"""
Enhanced Button Mapping Diagnostic Tool
Tests different interpretations of the button data to find face buttons
"""

import serial
import struct
import sys
import time

def send_packet_raw(ser, data):
    """Send raw 8-byte UART packet"""
    ser.write(data)
    print(f"Sent raw bytes: {' '.join(f'{b:02X}' for b in data)}")

def send_packet(ser, buttons, hat=0x08, lx=128, ly=128, rx=128, ry=128):
    """Send an 8-byte UART packet"""
    packet = struct.pack('<HBBBBBB', buttons, hat, lx, ly, rx, ry, 0)
    ser.write(packet)
    print(f"Sent: Buttons=0x{buttons:04X}, HAT=0x{hat:02X}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python diagnose_buttons_v2.py <serial_port>")
        print("Example: python diagnose_buttons_v2.py COM3")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        print("\n" + "="*60)
        print("ENHANCED BUTTON MAPPING DIAGNOSTIC")
        print("="*60)
        
        # Based on first diagnostic, we know:
        # Bit 0 = MINUS (-)
        # Bit 1 = PLUS (+)
        # Bit 2 = Left Stick Click
        # Bit 3 = Right Stick Click
        # Bit 4 = HOME
        # Bit 5 = CAPTURE
        # Bits 6-15 = Nothing detected
        
        print("\nPrevious findings:")
        print("  Bit 0 = MINUS (-)")
        print("  Bit 1 = PLUS (+)")
        print("  Bit 2 = Left Stick Click")
        print("  Bit 3 = Right Stick Click")
        print("  Bit 4 = HOME")
        print("  Bit 5 = CAPTURE")
        print("  Bits 6-15 = No response")
        
        print("\n" + "="*60)
        print("THEORY: Face/shoulder buttons might be in different bytes!")
        print("="*60)
        print("\nLet's test if buttons are actually in bytes 3-6 (stick data)")
        print("instead of bytes 0-1 (button data)...\n")
        
        input("Press Enter to start testing alternative interpretations...")
        
        # Test 1: What if face buttons are in the LX position (byte 3)?
        print("\n=== TEST 1: Setting byte 3 (LX position) to different values ===")
        for val in [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80]:
            packet = struct.pack('<HBBBBBB', 0, 0x08, val, 128, 128, 128, 0)
            print(f"\nByte 3 = 0x{val:02X}")
            ser.write(packet)
            response = input("Did anything happen? (y/n or describe): ")
            if response.lower() == 'y':
                print(f"  -> Found something at byte 3, value 0x{val:02X}!")
            send_packet(ser, 0)
            time.sleep(0.2)
        
        # Test 2: What if they're in LY position (byte 4)?
        print("\n=== TEST 2: Setting byte 4 (LY position) to different values ===")
        for val in [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80]:
            packet = struct.pack('<HBBBBBB', 0, 0x08, 128, val, 128, 128, 0)
            print(f"\nByte 4 = 0x{val:02X}")
            ser.write(packet)
            response = input("Did anything happen? (y/n or describe): ")
            if response.lower() == 'y':
                print(f"  -> Found something at byte 4, value 0x{val:02X}!")
            send_packet(ser, 0)
            time.sleep(0.2)
        
        # Test 3: Try testing with combined high bits in button word
        print("\n=== TEST 3: Testing higher bits with system buttons ===")
        print("(This combines known working bits with untested high bits)")
        
        tests = [
            (0x0040, "Bit 6 with MINUS"),  # Bit 6 + Bit 0
            (0x0080, "Bit 7 with MINUS"),  # Bit 7 + Bit 0
            (0x0100, "Bit 8 with MINUS"),  # Bit 8 + Bit 0
            (0x0200, "Bit 9 with MINUS"),  # Bit 9 + Bit 0
            (0x0400, "Bit 10 with MINUS"), # Bit 10 + Bit 0
            (0x0800, "Bit 11 with MINUS"), # Bit 11 + Bit 0
            (0x1000, "Bit 12 with MINUS"), # Bit 12 + Bit 0
            (0x2000, "Bit 13 with MINUS"), # Bit 13 + Bit 0
        ]
        
        for button_val, desc in tests:
            print(f"\n{desc}: 0x{button_val:04X}")
            send_packet(ser, button_val)
            response = input("Did TWO buttons press? Which ones?: ")
            if len(response) > 1:
                print(f"  -> Bit may represent: {response}")
            send_packet(ser, 0)
            time.sleep(0.2)
        
        # Test 4: D-Pad test
        print("\n=== TEST 4: D-Pad verification ===")
        dpad_tests = [
            (0x00, "UP"),
            (0x02, "RIGHT"),
            (0x04, "DOWN"),
            (0x06, "LEFT"),
        ]
        
        for hat_val, direction in dpad_tests:
            print(f"\nTesting D-Pad {direction}")
            send_packet(ser, 0, hat_val)
            time.sleep(1)
            send_packet(ser, 0, 0x08)
            time.sleep(0.3)
        
        print("\n" + "="*60)
        print("DIAGNOSTIC COMPLETE!")
        print("="*60)
        print("\nPlease report any findings from the tests above.")
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
