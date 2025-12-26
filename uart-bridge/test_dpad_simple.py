#!/usr/bin/env python3
"""Simple D-Pad test - just UP"""
import serial, struct, time, sys

port = sys.argv[1] if len(sys.argv) > 1 else "COM3"
ser = serial.Serial(port, 115200)
time.sleep(2)

print("Sending NEUTRAL (0x08)...")
# Send with header
ser.write(bytes([0xAA])); ser.flush(); time.sleep(0.002)
ser.write(bytes([0x55])); ser.flush(); time.sleep(0.002)
packet = struct.pack('<HBBBBBB', 0, 0x08, 128, 128, 128, 128, 0)
for b in packet: ser.write(bytes([b])); ser.flush(); time.sleep(0.001)
time.sleep(1)

print("Sending UP (0x00)...")
ser.write(bytes([0xAA])); ser.flush(); time.sleep(0.002)
ser.write(bytes([0x55])); ser.flush(); time.sleep(0.002)
packet = struct.pack('<HBBBBBB', 0, 0x00, 128, 128, 128, 128, 0)
for b in packet: ser.write(bytes([b])); ser.flush(); time.sleep(0.001)

print("Holding UP for 5 seconds - watch Switch!")
time.sleep(5)

print("Sending NEUTRAL...")
ser.write(bytes([0xAA])); ser.flush(); time.sleep(0.002)
ser.write(bytes([0x55])); ser.flush(); time.sleep(0.002)
packet = struct.pack('<HBBBBBB', 0, 0x08, 128, 128, 128, 128, 0)
for b in packet: ser.write(bytes([b])); ser.flush(); time.sleep(0.001)

ser.close()
print("Done!")
