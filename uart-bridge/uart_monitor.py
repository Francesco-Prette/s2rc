#!/usr/bin/env python3
"""
UART Monitor - Monitors communication between uart-bridge and simple-s2rc
Connect a USB-to-Serial adapter to the UART lines between the two Picos
to capture and decode the traffic.

Wiring:
  USB-Serial RX -> Pico GP0 (TX from uart-bridge)
  USB-Serial GND -> Common GND
  
Or use a logic analyzer on GP0 (TX) and GP1 (RX) between the Picos.
"""

import serial
import sys
import time
import argparse

# Button bit definitions (Standard Nintendo Switch HID order)
BUTTONS = {
    0: "B", 1: "A", 2: "Y", 3: "X",
    4: "L", 5: "R", 6: "ZL", 7: "ZR",
    8: "MINUS", 9: "PLUS", 10: "LSTICK", 11: "RSTICK",
    12: "HOME", 13: "CAPTURE", 14: "GL", 15: "GR"
}

# Hat switch values
HAT_DIRECTIONS = {
    0x00: "UP",
    0x01: "UP-RIGHT",
    0x02: "RIGHT",
    0x03: "DOWN-RIGHT",
    0x04: "DOWN",
    0x05: "DOWN-LEFT",
    0x06: "LEFT",
    0x07: "UP-LEFT",
    0x08: "NEUTRAL"
}

def decode_buttons(button_value):
    """Decode button bitmask into list of pressed buttons"""
    pressed = []
    for bit, name in BUTTONS.items():
        if button_value & (1 << bit):
            pressed.append(name)
    return pressed if pressed else ["NONE"]

def decode_packet(data):
    """Decode 8-byte UART packet"""
    if len(data) != 8:
        return f"INVALID PACKET LENGTH: {len(data)} bytes"
    
    # Parse packet
    buttons = data[0] | (data[1] << 8)
    hat = data[2]
    lx = data[3]
    ly = data[4]
    rx = data[5]
    ry = data[6]
    vendor = data[7]
    
    # Decode
    pressed_buttons = decode_buttons(buttons)
    hat_dir = HAT_DIRECTIONS.get(hat, f"INVALID(0x{hat:02X})")
    
    # Format output
    output = []
    output.append(f"Buttons: {', '.join(pressed_buttons)} (0x{buttons:04X})")
    output.append(f"D-Pad: {hat_dir} (0x{hat:02X})")
    output.append(f"Left Stick: X={lx} Y={ly}")
    output.append(f"Right Stick: X={rx} Y={ry}")
    output.append(f"Vendor: 0x{vendor:02X}")
    
    # Warning for buggy values
    warnings = []
    if hat > 0x08 and hat != 0x80:
        warnings.append(f"⚠️  INVALID HAT VALUE: 0x{hat:02X}")
    if hat == 0x80:
        warnings.append(f"⚠️  BUG DETECTED: Hat=0x80 (should be 0x08 for neutral)")
    if hat in [0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70]:
        warnings.append(f"⚠️  BUG: Hat incorrectly bit-shifted! Value=0x{hat:02X}")
    
    result = " | ".join(output)
    if warnings:
        result += "\n    " + " ".join(warnings)
    
    return result

def monitor_uart(port, baudrate=115200):
    """Monitor UART traffic and decode packets"""
    import serial as pyserial
    try:
        ser = pyserial.Serial(port, baudrate, timeout=1)
        print(f"Monitoring UART on {port} @ {baudrate} baud")
        print("Waiting for 8-byte packets...\n")
        
        packet_count = 0
        buffer = bytearray()
        
        while True:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                buffer.extend(data)
                
                # Process complete 8-byte packets
                while len(buffer) >= 8:
                    packet = buffer[:8]
                    buffer = buffer[8:]
                    
                    packet_count += 1
                    timestamp = time.strftime("%H:%M:%S")
                    
                    print(f"[{timestamp}] Packet #{packet_count}")
                    print(f"  Raw: {' '.join(f'{b:02X}' for b in packet)}")
                    print(f"  {decode_packet(packet)}")
                    print()
            
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print("\nMonitoring stopped by user")
    except pyserial.SerialException as e:
        print(f"Error: {e}")
        print("\nAvailable ports:")
        import serial.tools.list_ports
        for port in serial.tools.list_ports.comports():
            print(f"  {port.device}: {port.description}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

def main():
    parser = argparse.ArgumentParser(description='Monitor UART traffic between Picos')
    parser.add_argument('port', help='Serial port (e.g., COM3, /dev/ttyUSB0)')
    parser.add_argument('-b', '--baudrate', type=int, default=115200,
                       help='Baud rate (default: 115200)')
    
    args = parser.parse_args()
    monitor_uart(args.port, args.baudrate)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("UART Monitor - Decode Pico-to-Pico communication")
        print("\nUsage: python uart_monitor.py <port> [-b baudrate]")
        print("\nExample:")
        print("  Windows: python uart_monitor.py COM3")
        print("  Linux:   python uart_monitor.py /dev/ttyUSB0")
        print("\nAvailable ports:")
        try:
            import serial.tools.list_ports
            for port in serial.tools.list_ports.comports():
                print(f"  {port.device}: {port.description}")
        except ImportError:
            print("  Install pyserial: pip install pyserial")
        sys.exit(1)
    
    main()
