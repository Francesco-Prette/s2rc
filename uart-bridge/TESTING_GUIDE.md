# Testing Guide: Verifying Pico-to-Pico Communication

This guide helps you verify that your uart-bridge and simple-s2rc Picos are communicating correctly.

## Setup Overview

```
[PC Keyboard] --USB--> [uart-bridge Pico] --UART--> [simple-s2rc Pico] --USB--> [Nintendo Switch/PC]
                              GP0 (TX) ---------> GP1 (RX)
                              GP1 (RX) <--------- GP0 (TX)
                              GND <-------------> GND
```

## Method 1: Quick Visual Test (Easiest)

### What to Check:
1. **LEDs blink when you press keys** on the keyboard connected to uart-bridge
2. **Both Picos' LEDs should blink** when a key is pressed
3. **Controller input works** on the Switch/PC

### Expected Behavior:
- uart-bridge LED blinks when keyboard key is pressed
- simple-s2rc LED blinks when it receives UART data
- Controller responds to WASD, IJKL, etc.

### If Only One LED Blinks:
- Only uart-bridge blinks → UART connection problem
- Only simple-s2rc blinks → Keyboard not connected properly

---

## Method 2: Serial Monitor Test

### Setup:
1. Connect uart-bridge Pico to PC via USB
2. Open serial terminal (PuTTY, Arduino IDE Serial Monitor, etc.)
3. Connect to the uart-bridge's serial port (e.g., COM3)
4. Set baud rate: **115200**

### What to Test:

#### Test 1: Text Commands
Type these commands in the serial terminal:

```
A          → Should press button A
W          → Should press D-pad UP
A+W        → Should press A + D-pad UP
LX:255     → Should move left stick right
```

#### Expected Output:
```
> A
Sent: Buttons=0x0004 Hat=8 LX=128 LY=128 RX=128 RY=128
[KBD] Buttons=0x0004 Hat=8
```

#### Test 2: Keyboard Input
Press keys on USB keyboard connected to uart-bridge:

```
W key      → Should show: [KBD] Buttons=0x0000 Hat=0
S key      → Should show: [KBD] Buttons=0x0000 Hat=4
A key      → Should show: [KBD] Buttons=0x0000 Hat=6
D key      → Should show: [KBD] Buttons=0x0000 Hat=2
K key      → Should show: [KBD] Buttons=0x0002 Hat=8
```

### ⚠️ Look for These Bug Indicators:
```
❌ WRONG: [KBD] Buttons=0x0000 Hat=128  (0x80 - BUG!)
❌ WRONG: [KBD] Buttons=0x0000 Hat=96   (0x60 - BUG!)
❌ WRONG: [KBD] Buttons=0x0000 Hat=64   (0x40 - BUG!)

✅ CORRECT: [KBD] Buttons=0x0000 Hat=8  (0x08)
✅ CORRECT: [KBD] Buttons=0x0000 Hat=6  (0x06)
✅ CORRECT: [KBD] Buttons=0x0000 Hat=4  (0x04)
```

---

## Method 3: UART Monitor with USB-Serial Adapter

### Hardware Setup:
```
USB-Serial Adapter:
  RX ---> uart-bridge GP0 (TX) [tap the line, don't disconnect]
  GND --> Common GND

Or use a logic analyzer on GP0 and GP1
```

### Software Setup:
```bash
# Install Python dependencies
pip install pyserial

# Run the monitor
python uart_monitor.py COM3  # Windows
python uart_monitor.py /dev/ttyUSB0  # Linux
```

### Expected Output (Neutral State):
```
[18:30:45] Packet #1
  Raw: 00 00 08 80 80 80 80 00
  Buttons: NONE (0x0000) | D-Pad: NEUTRAL (0x08) | Left Stick: X=128 Y=128 | Right Stick: X=128 Y=128 | Vendor: 0x00
```

### Expected Output (W Key Pressed):
```
[18:30:46] Packet #2
  Raw: 00 00 00 80 80 80 80 00
  Buttons: NONE (0x0000) | D-Pad: UP (0x00) | Left Stick: X=128 Y=128 | Right Stick: X=128 Y=128 | Vendor: 0x00
```

### ⚠️ Bug Detection:
If you see this, the bug is present:
```
[18:30:46] Packet #3
  Raw: 00 00 80 80 80 80 80 00
  Buttons: NONE (0x0000) | D-Pad: INVALID(0x80) | Left Stick: X=128 Y=128 | Right Stick: X=128 Y=128 | Vendor: 0x00
    ⚠️  BUG DETECTED: Hat=0x80 (should be 0x08 for neutral)
```

---

## Method 4: Wireshark USB Capture

### Setup:
1. Install USBPcap (Windows) or use usbmon (Linux)
2. Start Wireshark with admin privileges
3. Capture on USBPcap interface
4. Apply filter: `usb.idVendor == 0x0f0d && usb.idProduct == 0x00c1`

### What to Look For:

Find "URB_INTERRUPT in" packets and look at the **last 8 bytes** of data:

#### Correct Format (After Fix):
```
Bytes: 00 00 08 80 80 80 80 00
       ↑  ↑  ↑  ↑  ↑  ↑  ↑  ↑
       Button Button Hat LX LY RX RY Vendor
       LSB    MSB
```

- Byte 2 (Hat) values: **0x00-0x08** ✅
- Neutral position: **0x08** ✅

#### Wrong Format (Before Fix):
```
Bytes: 00 00 80 80 80 80 80 00
            ↑
            Hat = 0x80 ❌ (Should be 0x08!)
```

- Byte 2 (Hat) values: **0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80** ❌

---

## Method 5: Controller Tester Website

### Setup:
1. Connect simple-s2rc Pico to your PC (not Switch)
2. Open browser to: https://gamepad-tester.com or https://hardwaretester.com/gamepad
3. Press keys on keyboard connected to uart-bridge

### Expected Results:

| Key | Button/Direction Should Light Up |
|-----|----------------------------------|
| W   | D-pad UP                         |
| S   | D-pad DOWN                       |
| A   | D-pad LEFT                       |
| D   | D-pad RIGHT                      |
| I   | Button X (Triangle/Y on display) |
| K   | Button B (Circle/A on display)   |
| J   | Button Y (Square/X on display)   |
| L   | Button A (Cross/B on display)    |
| Q   | L Button                         |
| E   | R Button                         |

### If D-pad Doesn't Work:
- Buttons work but D-pad doesn't → **Bug is present**
- Nothing works → Check UART connections
- LED blinks but nothing happens → Check USB connection to PC/Switch

---

## Troubleshooting

### Problem: LEDs blink but no controller input

**Possible Causes:**
1. ❌ simple-s2rc has the old buggy code (needs reflashing)
2. ❌ UART baud rate mismatch (both must be 115200)
3. ❌ Wrong UART pins connected

**Solution:**
- Rebuild and reflash simple-s2rc with the fixed code
- Verify both Picos use 115200 baud
- Check wiring: GP0 → GP1, GP1 → GP0, GND → GND

### Problem: Only button keys work, WASD doesn't work as D-pad

**Cause:** The bug is present (hat value being bit-shifted)

**Solution:** Reflash simple-s2rc with fixed code

### Problem: No communication at all

**Check:**
1. UART wiring: TX → RX, RX → TX crossover
2. Common ground connected
3. Both Picos powered on
4. Correct GPIO pins (GP0 and GP1)

### Problem: Intermittent communication

**Check:**
1. Loose wiring connections
2. Power supply issues
3. Both Picos on same ground reference

---

## Quick Diagnostic Checklist

- [ ] uart-bridge LED blinks when keys pressed
- [ ] simple-s2rc LED blinks when uart-bridge sends data
- [ ] Serial monitor shows keyboard events
- [ ] Serial monitor shows Hat values 0x00-0x08 (NOT 0x80!)
- [ ] Controller tester website shows all inputs
- [ ] D-pad responds to WASD keys
- [ ] Buttons respond to IJKL keys

If all items checked ✅ → **Communication is working correctly!**

---

## Reference: Valid Hat Switch Values

| Value | Direction    | Binary | Hex  |
|-------|--------------|--------|------|
| 0     | UP           | 0000   | 0x00 |
| 1     | UP-RIGHT     | 0001   | 0x01 |
| 2     | RIGHT        | 0010   | 0x02 |
| 3     | DOWN-RIGHT   | 0011   | 0x03 |
| 4     | DOWN         | 0100   | 0x04 |
| 5     | DOWN-LEFT    | 0101   | 0x05 |
| 6     | LEFT         | 0110   | 0x06 |
| 7     | UP-LEFT      | 0111   | 0x07 |
| 8     | NEUTRAL      | 1000   | 0x08 |

**Any other value (0x10, 0x20, ..., 0x80) is INVALID!**
