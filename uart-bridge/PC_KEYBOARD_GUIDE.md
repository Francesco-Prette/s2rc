# PC Keyboard to Nintendo Switch Controller

This guide shows you how to use your **PC keyboard** to control your Nintendo Switch through the uart-bridge system.

## System Architecture

```
┌─────────────┐   Keyboard    ┌─────────────┐
│     PC      │   Events      │   Python    │
│  Keyboard   ├──────────────►│   Script    │
└─────────────┘               └──────┬──────┘
                                     │ USB Serial
                                     │ (115200 baud)
                                     ▼
                              ┌─────────────┐
                              │uart-bridge  │
                              │    Pico     │
                              └──────┬──────┘
                                     │ UART
                                     │ GP0/GP1
                                     ▼
                              ┌─────────────┐
                              │ simple-s2rc │
                              │    Pico     │
                              └──────┬──────┘
                                     │ USB
                                     ▼
                              ┌─────────────┐
                              │   Nintendo  │
                              │   Switch    │
                              └─────────────┘
```

## What You Need

### Hardware:
1. **2x Raspberry Pi Pico** (uart-bridge + simple-s2rc)
2. **3x Jumper wires** (UART connection between Picos)
3. **2x USB cables** (PC to uart-bridge, simple-s2rc to Switch)

### Software:
1. **Python 3.7+** (already on your system)
2. **Python packages**: `pyserial` and `pynput`

## Quick Setup (5 Minutes!)

### Step 1: Install Python Packages

```powershell
pip install pyserial pynput
```

### Step 2: Connect Hardware

```
┌─────────────┐                    ┌─────────────┐
│uart-bridge  │                    │simple-s2rc  │
│    Pico     │                    │    Pico     │
│             │                    │             │
│  GP0 (TX)───┼───────────────────►│ GP1 (RX)    │
│  GP1 (RX)◄──┼────────────────────┤ GP0 (TX)    │
│  GND ───────┼────────────────────┤ GND         │
│             │                    │             │
│  USB ───────┼──► To PC          │             │
│             │                    │  USB ───────┼──► To Switch
└─────────────┘                    └─────────────┘
```

**Connections:**
- uart-bridge GP0 → simple-s2rc GP1
- uart-bridge GP1 → simple-s2rc GP0
- uart-bridge GND → simple-s2rc GND
- uart-bridge USB → PC
- simple-s2rc USB → Nintendo Switch

### Step 3: Find Your Serial Port

**Windows:**
1. Open Device Manager
2. Look under "Ports (COM & LPT)"
3. Note the COM port (e.g., COM3)

**Linux:**
```bash
ls /dev/ttyACM*
# Usually /dev/ttyACM0
```

**Mac:**
```bash
ls /dev/cu.usbmodem*
# Something like /dev/cu.usbmodem14101
```

### Step 4: Run the Script

```powershell
# Windows
cd C:\Users\francesco.prette\repos\personal\uart-bridge
python keyboard_to_serial.py COM3

# Linux/Mac
cd ~/uart-bridge
python3 keyboard_to_serial.py /dev/ttyACM0
```

### Step 5: Play!

Press keys on your keyboard, and they'll control the Switch!

## Keyboard Layout

```
╔════════════════════════════════════════════════╗
║           DEFAULT KEYBOARD MAPPING             ║
╠════════════════════════════════════════════════╣
║  Movement (D-Pad):                             ║
║    W A S D  or  Arrow Keys                     ║
║                                                ║
║  Face Buttons:                                 ║
║         I (X - top button)                     ║
║    J (Y)   K (B)   (L=A is at bottom)          ║
║         L (A - bottom button)                  ║
║                                                ║
║  Shoulder Buttons:                             ║
║    Q = L      E = R                            ║
║    R = ZL     F = ZR                           ║
║                                                ║
║  System Buttons:                               ║
║    1 = MINUS (-)     2 = PLUS (+)              ║
║    3 = L-Stick       4 = R-Stick               ║
║    H = HOME          C = CAPTURE               ║
║                                                ║
║  Special:                                      ║
║    ESC = Quit program                          ║
╚════════════════════════════════════════════════╝
```

## How It Works

### Key Holding:
- **Press and hold W** → D-Pad Up stays active
- **Release W** → D-Pad releases
- Works for ALL keys automatically!

### Combinations:
- **Hold W+D** → D-Pad Up-Right (diagonal)
- **Hold W+L** → Move up while pressing A button
- **Hold Q+E** → Both shoulder buttons pressed

### Update Rate:
- **125Hz** (8ms between updates)
- Very low latency for responsive gameplay

## Example Usage

```powershell
# Start the script
python keyboard_to_serial.py COM3

# You'll see:
============================================================
  KEYBOARD TO SWITCH CONTROLLER
============================================================

Keyboard Layout:
  D-Pad: WASD or Arrow Keys
  Buttons: I=X, K=B, J=Y, L=A
  Shoulders: Q=L, E=R, R=ZL, F=ZR
  System: 1=-, 2=+, 3=LS, 4=RS, H=Home, C=Capture

** Hold keys to keep buttons pressed! **

Press ESC to quit
============================================================

# Press keys and see output:
[KEY PRESS] w -> D-Pad up
[SENT] Buttons=0x0000 HAT=Up

[KEY PRESS] l -> Button 0x0004
[SENT] Buttons=0x0004 HAT=Up

[KEY RELEASE] w
[SENT] Buttons=0x0004 HAT=Neutral

# Press ESC to quit gracefully
```

## Customizing Key Mappings

Edit `keyboard_to_serial.py` and modify the `KEY_MAPPINGS` dictionary:

```python
KEY_MAPPINGS = {
    # Change any key mapping you want!
    'w': ('dpad', 'up'),         # W key → D-Pad up
    'space': ('button', BTN_A),  # Space → A button
    'enter': ('button', BTN_B),  # Enter → B button
    # ... add your own!
}
```

**Available buttons:**
- `BTN_A`, `BTN_B`, `BTN_X`, `BTN_Y`
- `BTN_L`, `BTN_R`, `BTN_ZL`, `BTN_ZR`
- `BTN_MINUS`, `BTN_PLUS`
- `BTN_LSTICK`, `BTN_RSTICK`
- `BTN_HOME`, `BTN_CAPTURE`

**D-Pad directions:**
- `'up'`, `'down'`, `'left'`, `'right'`

## Troubleshooting

### Script won't start - "No module named 'serial'"
```powershell
pip install pyserial pynput
```

### Script won't start - "No module named 'pynput'"
```powershell
pip install pynput
```

### Can't find COM port
1. Make sure uart-bridge Pico is plugged into PC via USB
2. Check Device Manager (Windows) or `ls /dev/tty*` (Linux/Mac)
3. Try unplugging and replugging the Pico

### Keys not working on Switch
1. Verify uart-bridge is receiving commands (watch script output)
2. Check UART wiring between Picos (TX→RX, RX→TX, GND→GND)
3. Make sure simple-s2rc is connected to Switch and recognized
4. Try pressing different keys to see if any work

### Keyboard not detected by script
1. The script needs focus - click on the terminal window
2. Try running as administrator (Windows) or with sudo (Linux)
3. Make sure no other program is capturing keyboard (like games)

### Keys repeat too fast
Edit `keyboard_to_serial.py` and change the update rate:
```python
time.sleep(0.008)  # Change to 0.016 for slower updates
```

### Keys are "sticky" or lag
1. This is normal - keys stay pressed while held!
2. Make sure to release keys to clear
3. Press ESC to quit and reset everything

## Advanced Usage

### Running in Background
```powershell
# Windows - run minimized
start /min python keyboard_to_serial.py COM3

# Linux/Mac - run in background
python3 keyboard_to_serial.py /dev/ttyACM0 &
```

### Multiple Controllers
You can run multiple instances with different serial ports:
```powershell
# Terminal 1
python keyboard_to_serial.py COM3

# Terminal 2 (if you have another uart-bridge)
python keyboard_to_serial.py COM4
```

### Macro Support (Coming Soon!)
You can extend the script to add macros:
```python
# Example: Press M to perform combo
if key == 'm':
    # Press A, wait, press B, wait, etc.
    self.state.buttons = BTN_A
    self.send_state()
    time.sleep(0.1)
    self.state.buttons = BTN_B
    self.send_state()
```

## Protocol Details

### Serial Communication:
- **Baud Rate**: 115200
- **Packet Size**: 8 bytes
- **Format**: Binary (not text commands!)

### Packet Structure:
```
Byte 0-1: Buttons (uint16_t, little-endian)
Byte 2:   HAT/D-Pad (0=Up, 1=UpRight, ..., 8=Neutral)
Byte 3:   Left Stick X (0-255, 128=center)
Byte 4:   Left Stick Y (0-255, 128=center)
Byte 5:   Right Stick X (0-255, 128=center)
Byte 6:   Right Stick Y (0-255, 128=center)
Byte 7:   Vendor byte (always 0)
```

## Tips for Best Experience

1. **For Gaming:**
   - Use a mechanical keyboard for better feel
   - Position keyboard comfortably
   - Practice the layout before playing

2. **For Development:**
   - Watch the script output to see state changes
   - Test individual keys first
   - Then try combinations

3. **Performance:**
   - Close unnecessary programs
   - Use wired keyboard (not wireless)
   - Keep USB cables short

4. **Customization:**
   - Map keys to match your favorite games
   - Create different mapping profiles
   - Share your mappings with friends!

## Comparison: PC Keyboard vs USB Host

| Feature | PC Keyboard (This) | USB Host (Pico) |
|---------|-------------------|-----------------|
| Setup | Easy (Python script) | Complex (firmware mod) |
| Latency | ~10-15ms | ~8ms |
| Keyboard | Any PC keyboard | Specific USB keyboards |
| Cost | Free | USB OTG adapter needed |
| Flexibility | Easy to customize | Requires recompile |
| **Best For** | Development, Testing | Standalone use |

**Recommendation:** Start with PC keyboard approach (this guide) for ease of use and customization!

## What's Next?

Once you're comfortable with the basics:

- ✅ Create custom key mappings for different games
- ✅ Add analog stick control (edit script to map keys to stick positions)
- ✅ Create macros for complex button sequences
- ✅ Share your setup with the community

## See Also

- [simple-s2rc README](../simple-s2rc/README.md) - Switch controller firmware
- [uart-bridge README](README.md) - Bridge firmware documentation
- [Python pynput docs](https://pynput.readthedocs.io/) - Keyboard library

## License

MIT License - Feel free to modify and share!

---

**Questions or Issues?** Open an issue on GitHub or check the troubleshooting section above.

**Happy Gaming! 🎮**
