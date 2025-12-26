# UART Bridge for Nintendo Switch Controller

This is the bridge Pico that connects to your PC via USB and forwards controller commands to the Switch Pico via UART.

**NEW: USB Keyboard Support!** You can now plug a USB keyboard directly into this Pico and use it to control the Switch!

## Hardware Setup

### This Pico (Bridge)
- **USB**: Connect to your computer for serial commands OR connect a USB keyboard via USB OTG adapter
- **GP0 (UART TX)**: Connect to Switch Pico GP1 (RX)
- **GP1 (UART RX)**: Connect to Switch Pico GP0 (TX)
- **GND**: Connect to Switch Pico GND

### Wiring
```
  This Pico (Bridge)     Switch Pico
  ┌──────────┐          ┌──────────┐
  │ GP0 (TX) ├─────────>│ GP1 (RX) │
  │ GP1 (RX) │<─────────┤ GP0 (TX) │
  │    GND   ├──────────┤   GND    │
  │    USB   │          │   USB    │
  └────┬─────┘          └────┬─────┘
       │                     │
   To PC or                To Switch
   USB Keyboard
```

**Note**: You can use a USB hub to connect both your PC (for serial terminal) and a keyboard simultaneously, or use a USB OTG adapter with micro-USB for keyboard input.

## Building

### Prerequisites
- CMake 3.13 or higher
- Pico SDK (set PICO_SDK_PATH environment variable)
- ARM GCC compiler

### Build Steps

1. **Set up environment** (if not already done):
   ```powershell
   # Set your Pico SDK path
   $env:PICO_SDK_PATH = "C:\path\to\pico-sdk"
   ```

2. **Create build directory**:
   ```powershell
   cd uart-bridge
   mkdir build
   cd build
   ```

3. **Configure and build**:
   ```powershell
   cmake ..
   cmake --build .
   ```

4. **Flash the firmware**:
   - Hold BOOTSEL button on Pico while plugging into USB
   - Copy `build\uart_bridge.uf2` to the Pico drive
   - Pico will reboot automatically

## Usage

This bridge supports **TWO input modes** simultaneously:

### Mode 1: USB Keyboard Input

Connect a USB keyboard to the Pico via USB OTG adapter and use it like a game controller!

#### Default Keyboard Layout:
```
╔═══════════════════════════════════════════╗
║          KEYBOARD MAPPING                  ║
╠═══════════════════════════════════════════╣
║ D-Pad (Movement):                         ║
║   W A S D  or  Arrow Keys                 ║
║                                           ║
║ Face Buttons:                             ║
║   I = X (top)                             ║
║   J = Y (left)    K = B (right)           ║
║   L = A (bottom)                          ║
║                                           ║
║ Shoulder Buttons:                         ║
║   Q = L    E = R                          ║
║   R = ZL   F = ZR                         ║
║                                           ║
║ System Buttons:                           ║
║   1 = MINUS (-)    2 = PLUS (+)           ║
║   3 = Left Stick   4 = Right Stick        ║
║   H = HOME         C = CAPTURE            ║
╚═══════════════════════════════════════════╝
```

#### How Key Holding Works:
- **Press and hold any key** → Button stays pressed on the Switch
- **Release the key** → Button releases on the Switch
- **No special configuration needed** - this works automatically!
- Supports up to **6 simultaneous keys** (standard USB keyboard limitation)

#### Example Uses:
- Hold W to walk forward
- Hold W+I to walk forward while attacking
- Hold Q+E to block with both shoulders
- Press H to go to home menu

### Mode 2: Serial Text Commands

Connect to the Pico via USB serial terminal and type commands.

1. Connect the Pico to your PC via USB
2. Open a serial terminal at **115200 baud**:
   - Windows: PuTTY, TeraTerm, or Windows Terminal
   - Linux/Mac: `screen /dev/ttyACM0 115200` or `minicom`

#### Single Buttons
```
A           - Press A button
B           - Press B button
X           - Press X button
Y           - Press Y button
```

#### Multiple Buttons (use + to combine)
```
A+B         - Press A and B together
L+R         - Press L and R together
ZL+ZR       - Press ZL and ZR together
```

#### D-Pad
```
U           - D-Pad Up
D           - D-Pad Down
L           - D-Pad Left
R           - D-Pad Right
UL          - D-Pad Up-Left
UR          - D-Pad Up-Right
DL          - D-Pad Down-Left
DR          - D-Pad Down-Right
```

#### Analog Sticks (values 0-255, center=128)
```
LX:255      - Left stick full right
LX:0        - Left stick full left
LY:255      - Left stick full down
LY:0        - Left stick full up
RX:200      - Right stick partially right
RY:100      - Right stick partially up
```

#### Complex Combinations
```
A+LX:255+LY:0           - Press A while holding left stick up-right
ZR+RX:255+RY:128        - Hold ZR while moving right stick right
H                       - Press HOME button
C                       - Press CAPTURE button
+                       - Press PLUS button
-                       - Press MINUS button
```

#### Special Commands
```
help        - Display help message
```

## Button Reference

| Command/Key | Button |
|-------------|--------|
| Y / J | Y button |
| B / K | B button |
| A / L | A button |
| X / I | X button |
| L / Q | L shoulder |
| R / E | R shoulder |
| ZL / R | ZL trigger |
| ZR / F | ZR trigger |
| - / 1 | MINUS button |
| + / 2 | PLUS button |
| LS / 3 | Left stick click |
| RS / 4 | Right stick click |
| H | HOME button |
| C | CAPTURE button |
| W/↑ | D-Pad Up |
| S/↓ | D-Pad Down |
| A/← | D-Pad Left |
| D/→ | D-Pad Right |

## LED Indicator

- **Blinks when command sent**: Confirms data transmitted to Switch Pico
- **Blinks when keyboard key pressed**: Confirms keyboard input received

## UART Protocol

The bridge sends 8-byte packets to the Switch Pico:

| Byte | Description |
|------|-------------|
| 0-1  | Button state (uint16_t, little endian) |
| 2    | D-Pad HAT (0=Up, 1=UpRight, ..., 8=Neutral) |
| 3    | Left stick X (0-255) |
| 4    | Left stick Y (0-255) |
| 5    | Right stick X (0-255) |
| 6    | Right stick Y (0-255) |
| 7    | Vendor byte (always 0) |

## Troubleshooting

### Keyboard Issues

**Keyboard not detected:**
- Make sure you're using a USB OTG adapter (micro-USB to USB-A female)
- Try a different keyboard (some keyboards may not be HID-compliant)
- Check serial output for "[USB] Keyboard mounted" message
- Ensure Pico is getting enough power (use powered USB hub if needed)

**Keys not working:**
- Verify keyboard is detected (check serial output)
- Try pressing different keys to see if any work
- Some keyboards have special modes - try a basic keyboard first
- Check that Switch Pico is running and receiving UART data

**Keys seem stuck:**
- Keyboard maintains state while held - this is intentional!
- To clear, simply release all keys
- Or unplug and replug the keyboard

### Serial Terminal Issues

**Can't see serial output:**
- Make sure you're connected at 115200 baud
- Try a different serial terminal application
- Check that the Pico is properly powered via USB

**Commands not working:**
1. Verify UART wiring (TX to RX, RX to TX, common GND)
2. Check LED blinks when sending commands
3. Make sure Switch Pico is also running the correct firmware
4. Test with simple commands like `A` or `B` first

**LED doesn't blink:**
- Check that the command is valid (type `help` for syntax)
- Ensure you're pressing Enter after the command
- Try the example commands from the help menu

## Advanced Usage

### Customizing Keyboard Mappings

You can modify the key mappings in `src/main.c`:

```c
static key_mapping_t key_mappings[] = {
    // D-Pad: WASD
    {HID_KEY_W, 0, DPAD_UP, false},
    // Add your custom mappings here
    {HID_KEY_SPACE, BTN_A, 0, true},  // Space bar -> A button
    // ... more mappings
};
```

HID keycodes are defined in TinyUSB's `tusb.h` header. Common keys:
- Letters: `HID_KEY_A` through `HID_KEY_Z`
- Numbers: `HID_KEY_1` through `HID_KEY_0`
- Function keys: `HID_KEY_F1` through `HID_KEY_F12`
- Modifiers: `HID_KEY_CONTROL_LEFT`, `HID_KEY_SHIFT_LEFT`, etc.
- Special: `HID_KEY_SPACE`, `HID_KEY_ENTER`, `HID_KEY_ESCAPE`, etc.

### Python Script Example (Serial Mode)

```python
import serial
import struct
import time

# Open serial port
ser = serial.Serial('COM3', 115200)  # Adjust COM port
time.sleep(2)

def send_command(cmd):
    """Send text command to bridge"""
    ser.write((cmd + '\n').encode())
    time.sleep(0.1)

# Press A button
send_command('A')

# Press A and B together
send_command('A+B')

# Move left stick and press button
send_command('A+LX:255')

ser.close()
```

### Direct Binary Protocol

Instead of text commands, you can send raw 8-byte packets:

```python
import serial
import struct

ser = serial.Serial('COM3', 115200)

# Button definitions
BTN_A = (1 << 2)

# Send state: buttons, hat, lx, ly, rx, ry, vendor
packet = struct.pack('<HBBBBBB', BTN_A, 0x08, 128, 128, 128, 128, 0)
ser.write(packet)

ser.close()
```

## Technical Details

### USB Host Configuration
- Supports USB HID keyboards
- Up to 6 simultaneous key presses (USB keyboard standard)
- Keyboard reports processed at ~125Hz
- Low latency: <8ms from key press to UART transmission

### Key State Management
- Keys are continuously polled while held
- State updates sent to Switch at 125Hz (8ms intervals)
- Automatic key release detection
- Supports diagonal D-Pad inputs (e.g., W+D = up-right)

### Performance
- Keyboard input latency: <8ms
- Serial command processing: immediate
- UART transmission: 115200 baud (~14KB/s)
- LED feedback on all input events

## Project Structure

```
uart-bridge/
├── CMakeLists.txt           - Build configuration (with TinyUSB host)
├── pico_sdk_import.cmake    - Pico SDK integration
├── README.md                - This file
├── src/
│   ├── main.c              - Bridge firmware with keyboard support
│   └── tusb_config.h       - TinyUSB configuration
└── build/                   - Build output (created by CMake)
    └── uart_bridge.uf2     - Firmware file to flash
```

## Tips for Best Experience

1. **For gaming**: Use a mechanical keyboard for better tactile feedback
2. **For testing**: Start with serial commands, then add keyboard
3. **Key layout**: The WASD+IJKL layout mimics fight stick layouts
4. **Customization**: Edit `key_mappings[]` in main.c for your preferred layout
5. **Multiple inputs**: You can use keyboard AND serial commands simultaneously

## License

Based on Pico SDK examples. See Pico SDK license for details.

## See Also

- [simple-s2rc](../simple-s2rc/README.md) - The Switch controller firmware
- [TinyUSB Documentation](https://docs.tinyusb.org/) - USB host library documentation
- [Pico SDK Documentation](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html)
