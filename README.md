# Simple Switch to Remote Control (S2RC)

Control your Nintendo Switch and Switch 2 remotely using two Raspberry Pi Picos connected via UART.

## Road map

- [x] Support any controller input
- [x] Easy binding setup + save config
- [ ] Use only 1 pico (USB UART TTL instead of the second pico)
- [ ] Turn on Switch 2 with the s2rc
- [ ] 3d model of the case

## Supported feature

- [x] Controll Switch 1/2 with keyboard
- [x] Turn on Switch 1 remote
- [ ] Key Gr/Gl for Switch 2
- [x] Should work on Windows, MacOS and Linux (Tested only on Windows)
- [x] Multiple controller supported (Tested on ps5 controller and xbox)

## Setup for full remote controll

- Video capture
- Any remote controll of the remote machine with the video capture (like parsec)
- For Switch 2 a remote switch on and off the power of the console to turn it on (the home key doesn't work Switch 2 sleeping mode)
- S2rc connected to the remote machine and Switch

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

## Getting Started

### Download Pre-Built Software

**Controller Bridge Application** (for your PC):

Download the latest release from: **[GitHub Releases](https://github.com/Francesco-Prette/s2rc/releases)**

- **Windows**: `controller_bridge-windows-x64.zip`
- **Linux**: `controller_bridge-linux-x64.tar.gz`  
- **macOS**: `controller_bridge-macos-x64.tar.gz`

**Pico Firmware** (UF2 files):

Download the pre-built firmware files from the release page:
- `s2rc.uf2` - For Pico #2 (connects to Switch)
- `uart_bridge.uf2` - For Pico #1 (connects to PC)

### Quick Setup

1. **Flash the Picos**:
   - Hold BOOTSEL button on Pico #2, plug into PC, copy `s2rc.uf2` to it
   - Hold BOOTSEL button on Pico #1, plug into PC, copy `uart_bridge.uf2` to it
   
2. **Connect hardware**:
   - Pico #2 → Nintendo Switch (appears as "HORIPAD for Nintendo Switch")
   - Pico #1 → Your PC via USB
   - Wire the UARTs between Picos (see Hardware Setup above)

3. **Run setup wizard**:
   ```bash
   # Extract the downloaded zip/tar.gz, then:
   ./controller_bridge --setup
   ```
   
   The wizard will:
   - **Ask for your COM port** (e.g., COM10 on Windows)
   - Let you choose keyboard or controller input
   - Configure custom button mappings
   - Save configuration to a `.ini` file

4. **Start controlling**:
   ```bash
   ./controller_bridge my_config.ini
   ```

Now you can control your Switch! 🎮

## Using the Controller Bridge

### Configuration Wizard

The wizard provides an easy setup process:

**Serial Port**: Prompts for your COM port automatically
- **Windows**: COM1, COM3, COM10, etc. (check Device Manager)
- **Linux**: /dev/ttyACM0, /dev/ttyUSB0, etc.
- **macOS**: /dev/tty.usbmodem*

**Input Modes**:
1. **Keyboard Mode** - Map keys to Switch buttons
2. **Controller Mode** - Use PS4/PS5/Xbox controller with optional analog stick calibration

### Commands

#### Buttons-mapping
```
j           - Press A button
k           - Press B button
u           - Press X button
i           - Press Y button
```

#### D-Pad
```
arrow-up           - D-Pad Up
arrow-down         - D-Pad Down
arrow-left         - D-Pad Left (note: might conflict with L button in parser)
arrow-right        - D-Pad Right (note: might conflict with R button in parser)
```

#### Analog Sticks
```
r           - Left stick full right
w           - Left stick full left
d           - Left stick full down
e           - Left stick full up
```

## LED Indicators

### Pico #2 (Switch Controller)
- **Fast blink on startup**: Initialization complete
- **Blinks when data received**: Indicates UART communication

### Pico #1 (Bridge)
- **Blinks when command sent**: Confirms data transmitted to Switch Pico

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
- No pairing required - works immediately when plugged in and press a key
- Low latency design suitable for real-time control
- Can be extended to support scripting, automation, or custom input devices

## Expert Only: Building from Source

### Building the Pico Firmware

#### Prerequisites
- Raspberry Pi Pico SDK installed
- CMake and build tools

#### For Pico #2 (Switch Controller):
```bash
cd build
cmake ..
cmake --build .
```

This creates `s2rc.uf2` - flash this to the Pico connected to the Switch.

#### For Pico #1 (Bridge):
```bash
cd uart-bridge/build
cmake ..
cmake --build .
```

This creates `uart_bridge.uf2` - flash this to the Pico connected to your PC.

### Building the Controller Bridge Application

#### Windows
```bash
cd controller_bridge
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### Linux / macOS
```bash
cd controller_bridge
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable will be in `build/` or `build/Release/` directory.

For detailed release build instructions, see [RELEASE.md](RELEASE.md).

## License

Based on TinyUSB examples and Pico SDK. See respective licenses.

## Author

Francesco Prette with some AI help. Special thanks to the project GP2040-CE
