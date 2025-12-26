# Simple Switch to Remote Control (S2RC)

Control your Nintendo Switch remotely using two Raspberry Pi Picos connected via UART.

## System Architecture

```
Computer (USB Serial)
    ↓
Pico #1 (Bridge/Sender)
    ↓ (UART @ 115200 baud)
Pico #2 (Switch Controller)
    ↓ (USB)
Nintendo Switch
```

## Hardware Setup

### Pico #1 (Bridge - Connected to PC)
- **USB**: Connected to your computer
- **GP0 (UART0 TX)**: Connect to Pico #2 GP1
- **GP1 (UART0 RX)**: Connect to Pico #2 GP0
- **GND**: Connect to Pico #2 GND

### Pico #2 (Switch Controller)
- **USB**: Connected to Nintendo Switch
- **GP0 (UART0 TX)**: Connect to Pico #1 GP1
- **GP1 (UART0 RX)**: Connect to Pico #1 GP0
- **GND**: Connect to Pico #1 GND

### Wiring Diagram
```
  Pico #1 (PC)          Pico #2 (Switch)
  ┌──────────┐          ┌──────────┐
  │ GP0 (TX) ├─────────>│ GP1 (RX) │
  │ GP1 (RX) │<─────────┤ GP0 (TX) │
  │    GND   ├──────────┤   GND    │
  │    USB   │          │   USB    │
  └────┬─────┘          └────┬─────┘
       │                     │
    To PC               To Switch
```

## Building the Firmware

### For Pico #2 (Switch Controller):
```powershell
cd C:\Users\francesco.prette\repos\personal\simple-s2rc\build
cmake ..
cmake --build .
```

This creates `s2rc.uf2` - flash this to the Pico connected to the Switch.

### For Pico #1 (Bridge):
```powershell
cd C:\Users\francesco.prette\repos\personal\uart-bridge
mkdir build
cd build
cmake ..
cmake --build .
```

This creates `uart_bridge.uf2` - flash this to the Pico connected to your PC.

See the [uart-bridge README](../../uart-bridge/README.md) for detailed usage instructions.

## UART Protocol

The protocol uses **8 bytes per packet** sent at 115200 baud:

| Byte | Description | Values |
|------|-------------|--------|
| 0-1  | Buttons (uint16_t, little endian) | See button definitions below |
| 2    | D-Pad (HAT) | 0=Up, 1=UpRight, 2=Right, 3=DownRight, 4=Down, 5=DownLeft, 6=Left, 7=UpLeft, 8=Neutral |
| 3    | Left Stick X | 0-255 (128 = center) |
| 4    | Left Stick Y | 0-255 (128 = center) |
| 5    | Right Stick X | 0-255 (128 = center) |
| 6    | Right Stick Y | 0-255 (128 = center) |
| 7    | Vendor byte | Always 0 |

### Button Bit Map (uint16_t)
```
Bit 0:  Y
Bit 1:  B
Bit 2:  A
Bit 3:  X
Bit 4:  L
Bit 5:  R
Bit 6:  ZL
Bit 7:  ZR
Bit 8:  MINUS (-)
Bit 9:  PLUS (+)
Bit 10: L-Stick Click
Bit 11: R-Stick Click
Bit 12: HOME
Bit 13: CAPTURE
```

## Using the Bridge Controller

Once both Picos are flashed and connected:

1. Connect Pico #2 to the Nintendo Switch (should appear as "HORIPAD for Nintendo Switch")
2. Connect Pico #1 to your PC
3. Open a serial terminal (PuTTY, screen, minicom, etc.) at **115200 baud**
4. Type commands to control the Switch

### Command Examples

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
L           - D-Pad Left (note: might conflict with L button in parser)
R           - D-Pad Right (note: might conflict with R button in parser)
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

### Special Commands
```
help        - Display help message
```

## LED Indicators

### Pico #2 (Switch Controller)
- **Fast blink on startup**: Initialization complete
- **Blinks when data received**: Indicates UART communication

### Pico #1 (Bridge)
- **Blinks when command sent**: Confirms data transmitted to Switch Pico

## Troubleshooting

### Switch doesn't recognize the controller
- Make sure Pico #2 has the correct firmware (s2rc.uf2)
- Verify USB connection to Switch is solid
- Try a different USB cable or port

### No response to commands
1. Check UART wiring (TX to RX, RX to TX, common GND)
2. Verify both Picos are powered on
3. Check serial terminal is at 115200 baud
4. Watch LED indicators - they should blink on activity
5. Type 'help' to verify serial communication works

### Commands seem to stick
- The protocol sends the state once - commands are not held
- To release buttons, send neutral state (just press Enter with no command)
- For continuous button press, you need to send the command repeatedly

## Advanced: Creating Python Controller Script

You can control the Switch from Python:

```python
import serial
import struct
import time

# Open serial connection to Pico #1
ser = serial.Serial('COM3', 115200, timeout=1)  # Adjust COM port
time.sleep(2)  # Wait for Pico to initialize

def send_state(buttons=0, hat=0x08, lx=128, ly=128, rx=128, ry=128):
    """Send 8-byte controller state"""
    packet = struct.pack('<HBBBBBB', buttons, hat, lx, ly, rx, ry, 0)
    ser.write(packet)
    time.sleep(0.01)  # Small delay between commands

# Example: Press A button
BTN_A = (1 << 2)
send_state(buttons=BTN_A)
time.sleep(0.5)

# Release all buttons
send_state()
time.sleep(0.5)

# Move left stick right while pressing B
BTN_B = (1 << 1)
send_state(buttons=BTN_B, lx=255)
time.sleep(1)

ser.close()
```

## Technical Details

### USB Device Information
- **Vendor ID**: 0x0F0D (HORI)
- **Product ID**: 0x00C1 (HORIPAD for Nintendo Switch)
- **Manufacturer**: "HORI CO.,LTD."
- **Product Name**: "HORIPAD for Nintendo Switch"

### Report Rate
- HID reports sent at **125Hz** (8ms intervals)
- UART operates at **115200 baud**
- ~14,400 bytes/second theoretical throughput
- ~1,800 controller updates/second maximum (8 bytes per update)

## Notes

- The controller appears as a HORI controller to the Switch (officially licensed)
- Works with all Switch games that support USB controllers
- No pairing required - works immediately when plugged in
- Low latency design suitable for real-time control
- Can be extended to support scripting, automation, or custom input devices

## License

Based on TinyUSB examples and Pico SDK. See respective licenses.
