#!/usr/bin/env python3
"""
Keyboard to Serial Bridge for Nintendo Switch Controller

This script captures keyboard input on your PC and sends controller commands
to the uart-bridge Pico via serial, which forwards them to the Switch.

Requirements:
    pip install pyserial pynput

Usage:
    python keyboard_to_serial.py COM3

Where COM3 is your uart-bridge serial port (use /dev/ttyACM0 on Linux/Mac)
"""

import serial
import struct
import sys
import time
from pynput import keyboard
from collections import defaultdict

# Button definitions (matching uart-bridge protocol)
# Standard Nintendo Switch HID button order: B, A, Y, X, L, R, ZL, ZR, -, +, LS, RS, Home, Capture
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
BTN_GL      = (1 << 14)  # Grip Left / Back Left
BTN_GR      = (1 << 15)  # Grip Right / Back Right

# D-Pad values
DPAD_UP        = 0x00
DPAD_UP_RIGHT  = 0x01
DPAD_RIGHT     = 0x02
DPAD_DN_RIGHT  = 0x03
DPAD_DOWN      = 0x04
DPAD_DN_LEFT   = 0x05
DPAD_LEFT      = 0x06
DPAD_UP_LEFT   = 0x07
DPAD_NEUTRAL   = 0x08  # Neutral state (matching GP2040-CE SWITCH_HAT_NOTHING)

# Key mappings (customize these to your preference!)
KEY_MAPPINGS = {
    # D-Pad: WASD
    # 'e': ('dpad', 'up'),
    # 'd': ('dpad', 'down'),
    # 'w': ('dpad', 'left'),
    # 'r': ('dpad', 'right'),
    
    # Face buttons: IJKL
    'u': ('button', BTN_X),      # I -> X (top)
    'i': ('button', BTN_B),      # K -> Y (left)
    'j': ('button', BTN_Y),      # J -> A (right)
    'k': ('button', BTN_A),      # L -> B (bottom)
    
    # Shoulders: QERF
    'l': ('button', BTN_L),
    'f': ('button', BTN_R),
    't': ('button', BTN_ZL),
    's': ('button', BTN_ZR),
    
    # Grip/Back buttons: ZX
    'z': ('button', BTN_GL),
    'x': ('button', BTN_GR),
    
    # System buttons
    '1': ('button', BTN_MINUS),
    '2': ('button', BTN_PLUS),
    '3': ('button', BTN_LSTICK),
    '4': ('button', BTN_RSTICK),
    'h': ('button', BTN_HOME),
    'c': ('button', BTN_CAPTURE),
    
    # Arrow keys (alternative D-pad)
    keyboard.Key.up: ('dpad', 'up'),
    keyboard.Key.down: ('dpad', 'down'),
    keyboard.Key.left: ('dpad', 'left'),
    keyboard.Key.right: ('dpad', 'right'),
    
    # Left Analog Stick: Numpad (matching C firmware)
    # Note: pynput doesn't support numpad keys easily, so we use alternatives
    # You can customize these keys if needed
    'e': ('lstick', 'up'),      # Alternative for numpad 8
    'd': ('lstick', 'down'),    # Alternative for numpad 5
    'w': ('lstick', 'left'),    # Alternative for numpad 4
    'r': ('lstick', 'right'),   # Alternative for numpad 6
    
    # Right Analog Stick: (matching C firmware conceptually)
    'g': ('rstick', 'up'),      # Alternative mapping
    'v': ('rstick', 'down'),    # Alternative mapping
    'a': ('rstick', 'left'),    # Alternative mapping
    'y': ('rstick', 'right'),   # Alternative mapping
}

class ControllerState:
    """Tracks the current state of all controller inputs"""
    def __init__(self):
        self.buttons = 0
        self.dpad_up = False
        self.dpad_down = False
        self.dpad_left = False
        self.dpad_right = False
        
        # Analog stick direction flags
        self.lstick_up = False
        self.lstick_down = False
        self.lstick_left = False
        self.lstick_right = False
        self.rstick_up = False
        self.rstick_down = False
        self.rstick_left = False
        self.rstick_right = False
        
        self.lx = 128  # Center
        self.ly = 128  # Center
        self.rx = 128  # Center
        self.ry = 128  # Center
    
    def update_stick_positions(self):
        """Calculate stick positions from direction flags"""
        # Left stick
        if self.lstick_up:
            self.ly = 0  # Y-axis inverted: 0 = up
        elif self.lstick_down:
            self.ly = 255  # Y-axis inverted: 255 = down
        else:
            self.ly = 128  # Center
        
        if self.lstick_left:
            self.lx = 0  # 0 = left
        elif self.lstick_right:
            self.lx = 255  # 255 = right
        else:
            self.lx = 128  # Center
        
        # Right stick
        if self.rstick_up:
            self.ry = 0  # Y-axis inverted: 0 = up
        elif self.rstick_down:
            self.ry = 255  # Y-axis inverted: 255 = down
        else:
            self.ry = 128  # Center
        
        if self.rstick_left:
            self.rx = 0  # 0 = left
        elif self.rstick_right:
            self.rx = 255  # 255 = right
        else:
            self.rx = 128  # Center
    
    def get_hat(self):
        """Calculate HAT value from individual D-pad directions"""
        if self.dpad_up and self.dpad_right:
            return DPAD_UP_RIGHT
        elif self.dpad_up and self.dpad_left:
            return DPAD_UP_LEFT
        elif self.dpad_down and self.dpad_right:
            return DPAD_DN_RIGHT
        elif self.dpad_down and self.dpad_left:
            return DPAD_DN_LEFT
        elif self.dpad_up:
            return DPAD_UP
        elif self.dpad_down:
            return DPAD_DOWN
        elif self.dpad_left:
            return DPAD_LEFT
        elif self.dpad_right:
            return DPAD_RIGHT
        else:
            return DPAD_NEUTRAL
    
    def to_bytes(self):
        """Convert state to 10-byte UART protocol packet with headers"""
        # Header bytes (0xAA 0x55) + 8 data bytes
        return struct.pack('<BBHBBBBBB', 
                          0xAA, 0x55,  # Header for packet synchronization
                          self.buttons, 
                          self.get_hat(), 
                          self.lx, self.ly, 
                          self.rx, self.ry, 
                          0)  # vendor byte

class KeyboardController:
    """Handles keyboard input and sends controller state via serial"""
    
    def __init__(self, serial_port, baud_rate=115200):
        self.state = ControllerState()
        self.pressed_keys = set()
        self.ser = serial.Serial(serial_port, baud_rate, timeout=1)
        time.sleep(2)  # Wait for serial connection to stabilize
        print(f"Connected to {serial_port} at {baud_rate} baud")
        print("\n" + "="*60)
        print("  KEYBOARD TO SWITCH CONTROLLER")
        print("="*60)
        print("\nKeyboard Layout:")
        print("  D-Pad: WASD or Arrow Keys")
        print("  Buttons: I=X, K=B, J=Y, L=A")
        print("  Shoulders: Q=L, E=R, R=ZL, F=ZR")
        print("  Grip/Back: Z=GL, X=GR")
        print("  System: 1=-, 2=+, 3=LS, 4=RS, H=Home, C=Capture")
        print("  Left Stick: O/P/N/B (Up/Down/Left/Right)")
        print("  Right Stick: G/V/A/Y (Up/Down/Left/Right)")
        print("\n** Hold keys to keep buttons/sticks pressed! **")
        print("\nPress ESC to quit")
        print("="*60 + "\n")
        
        # Start keyboard listener
        self.listener = keyboard.Listener(
            on_press=self.on_press,
            on_release=self.on_release)
        self.listener.start()
        
        # Update loop
        self.running = True
        self.last_state = b''
        
    def get_key_char(self, key):
        """Convert key to character for lookup"""
        if hasattr(key, 'char') and key.char:
            return key.char.lower()
        return key
    
    def on_press(self, key):
        """Handle key press event"""
        # ESC to quit
        if key == keyboard.Key.esc:
            self.running = False
            return False
        
        key_char = self.get_key_char(key)
        
        # Avoid duplicate presses
        if key_char in self.pressed_keys:
            return
        
        self.pressed_keys.add(key_char)
        
        # Update state based on key mapping
        if key_char in KEY_MAPPINGS:
            mapping_type, mapping_value = KEY_MAPPINGS[key_char]
            
            if mapping_type == 'button':
                self.state.buttons |= mapping_value
                print(f"[KEY PRESS] {key_char} -> Button 0x{mapping_value:04X}")
            
            elif mapping_type == 'dpad':
                if mapping_value == 'up':
                    self.state.dpad_up = True
                elif mapping_value == 'down':
                    self.state.dpad_down = True
                elif mapping_value == 'left':
                    self.state.dpad_left = True
                elif mapping_value == 'right':
                    self.state.dpad_right = True
                print(f"[KEY PRESS] {key_char} -> D-Pad {mapping_value}")
            
            elif mapping_type == 'lstick':
                if mapping_value == 'up':
                    self.state.lstick_up = True
                elif mapping_value == 'down':
                    self.state.lstick_down = True
                elif mapping_value == 'left':
                    self.state.lstick_left = True
                elif mapping_value == 'right':
                    self.state.lstick_right = True
                self.state.update_stick_positions()
                print(f"[KEY PRESS] {key_char} -> Left Stick {mapping_value}")
            
            elif mapping_type == 'rstick':
                if mapping_value == 'up':
                    self.state.rstick_up = True
                elif mapping_value == 'down':
                    self.state.rstick_down = True
                elif mapping_value == 'left':
                    self.state.rstick_left = True
                elif mapping_value == 'right':
                    self.state.rstick_right = True
                self.state.update_stick_positions()
                print(f"[KEY PRESS] {key_char} -> Right Stick {mapping_value}")
            
            self.send_state()
    
    def on_release(self, key):
        """Handle key release event"""
        key_char = self.get_key_char(key)
        
        if key_char not in self.pressed_keys:
            return
        
        self.pressed_keys.remove(key_char)
        
        # Update state based on key mapping
        if key_char in KEY_MAPPINGS:
            mapping_type, mapping_value = KEY_MAPPINGS[key_char]
            
            if mapping_type == 'button':
                self.state.buttons &= ~mapping_value
                print(f"[KEY RELEASE] {key_char}")
            
            elif mapping_type == 'dpad':
                if mapping_value == 'up':
                    self.state.dpad_up = False
                elif mapping_value == 'down':
                    self.state.dpad_down = False
                elif mapping_value == 'left':
                    self.state.dpad_left = False
                elif mapping_value == 'right':
                    self.state.dpad_right = False
                print(f"[KEY RELEASE] {key_char}")
            
            elif mapping_type == 'lstick':
                if mapping_value == 'up':
                    self.state.lstick_up = False
                elif mapping_value == 'down':
                    self.state.lstick_down = False
                elif mapping_value == 'left':
                    self.state.lstick_left = False
                elif mapping_value == 'right':
                    self.state.lstick_right = False
                self.state.update_stick_positions()
                print(f"[KEY RELEASE] {key_char}")
            
            elif mapping_type == 'rstick':
                if mapping_value == 'up':
                    self.state.rstick_up = False
                elif mapping_value == 'down':
                    self.state.rstick_down = False
                elif mapping_value == 'left':
                    self.state.rstick_left = False
                elif mapping_value == 'right':
                    self.state.rstick_right = False
                self.state.update_stick_positions()
                print(f"[KEY RELEASE] {key_char}")
            
            self.send_state()
    
    def send_state(self, force=False):
        """Send current controller state to serial port"""
        packet = self.state.to_bytes()
        
        # Send if state changed or forced
        if force or packet != self.last_state:
            self.ser.write(packet)
            self.ser.flush()  # Ensure immediate transmission
            self.last_state = packet
            
            # Show current state (only on actual changes, not forced updates)
            if packet != self.last_state or force:
                hat = self.state.get_hat()
                hat_names = {
                    0x00: 'Up', 0x01: 'Up-Right', 0x02: 'Right', 0x03: 'Down-Right',
                    0x04: 'Down', 0x05: 'Down-Left', 0x06: 'Left', 0x07: 'Up-Left',
                    0x08: 'Neutral'
                }
                hat_name = hat_names.get(hat, f'Unknown(0x{hat:02X})')
                # print(f"[SENT] Buttons=0x{self.state.buttons:04X} HAT={hat_name}")
    
    def run(self):
        """Main loop - keep sending state at high frequency"""
        try:
            while self.running:
                # Send state continuously for immediate responsiveness
                self.send_state(force=True)
                time.sleep(0.001)  # 1ms = 1000Hz update rate for instant response
                
        except KeyboardInterrupt:
            pass
        finally:
            print("\n\nShutting down...")
            # Send neutral state on exit
            self.state = ControllerState()
            self.send_state()
            self.ser.close()
            print("Disconnected. Goodbye!")

def main():
    if len(sys.argv) < 2:
        print("Usage: python keyboard_to_serial.py <serial_port>")
        print("\nExamples:")
        print("  Windows: python keyboard_to_serial.py COM3")
        print("  Linux:   python keyboard_to_serial.py /dev/ttyACM0")
        print("  Mac:     python keyboard_to_serial.py /dev/cu.usbmodem14101")
        sys.exit(1)
    
    serial_port = sys.argv[1]
    
    try:
        controller = KeyboardController(serial_port)
        controller.run()
    except serial.SerialException as e:
        print(f"Error: Could not open serial port {serial_port}")
        print(f"Details: {e}")
        print("\nMake sure:")
        print("  1. The uart-bridge Pico is connected to your PC")
        print("  2. You have the correct port name")
        print("  3. No other program is using the serial port")
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
