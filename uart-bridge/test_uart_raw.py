#!/usr/bin/env python3
"""
Raw UART packet inspection tool
Sends known button values and displays what gets received
"""

import serial
import struct
import sys
import time

def send_and_display(ser, buttons, description):
    """Send packet and show raw bytes"""
    packet = struct.pack('<HBBBBBB', buttons, 0x08, 128, 128, 128, 128, 0)
    ser.write(packet)
    
    print(f"\n{description}")
    print(f"  Intended buttons: 0x{buttons:04X} (binary: {bin(buttons)})")
    print(f"  Sent bytes: {' '.join(f'{b:02X}' for b in packet)}")
    print(f"  Byte 0 (low):  0x{packet[0]:02X} = {packet[0]:08b}")
    print(f"  Byte 1 (high): 0x{packet[1]:02X} = {packet[1]:08b}")
    time.sleep(1.5)

def main():
    if len(sys.argv) < 2:
        print("Usage: python test_uart_raw.py <serial_port>")
        sys.exit(1)
    
    port = sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        print(f"Connected to {port}")
        print("\n" + "="*70)
        print("UART PACKET ANALYSIS")
        print("="*70)
        print("\nThis will send button values and show the raw bytes.")
        print("Compare with what you see on Switch!\n")
        
        # Test each bit individually
        for bit in range(8):
            buttons = (1 << bit)
            send_and_display(ser, buttons, f"TEST: Bit {bit} set")
        
        # Test a high bit
        send_and_display(ser, 0x0100, "TEST: Bit 8 set (high byte)")
        send_and_display(ser, 0x0200, "TEST: Bit 9 set (high byte)")
        
        # Send neutral
        ser.write(struct.pack('<HBBBBBB', 0, 0x08, 128, 128, 128, 128, 0))
        
        print("\n" + "="*70)
        print("Analysis complete. Did the Switch respond to bits 8-15?")
        print("="*70)
        
        ser.close()
        
    except serial.SerialException as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
