# Button Mapping Issue - Findings and Solution

## Diagnostic Results Summary

Based on testing with `diagnose_buttons.py`, here are the **confirmed** button mappings:

| Bit Position | Switch Button Activated | Status |
|--------------|------------------------|---------|
| 0            | MINUS (-)              | ✓ Works |
| 1            | PLUS (+)               | ✓ Works |
| 2            | Left Stick Click       | ✓ Works |
| 3            | Right Stick Click      | ✓ Works |
| 4            | HOME                   | ✓ Works |
| 5            | CAPTURE                | ✓ Works |
| 6-15         | No response            | ✗ Not working |

## The Problem

The face buttons (A, B, X, Y) and shoulder buttons (L, R, ZL, ZR) which should be in bits 6-13 are **not responding at all** when sent via UART.

This is the opposite of what was documented - we expected:
- Bits 0-3: Face buttons (B, A, Y, X)
- Bits 4-7: Shoulder buttons (L, R, ZL, ZR)
- Bits 8-13: System buttons (-, +, LS, RS, Home, Capture)

But the actual mapping shows system buttons in bits 0-5, with face/shoulder buttons not responding.

## Root Cause Analysis

There are several possible explanations:

### Theory 1: HID Descriptor Defines Different Button Order
The USB HID descriptor in `simple-s2rc/src/usb_descriptors.c` defines 14 buttons (Report Count = 14), but doesn't specify their semantic meaning. The Switch interprets these 14 buttons based on the HORI VID/PID, and it may expect:
- Buttons 1-6 (bits 0-5): System buttons
- Buttons 7-14 (bits 6-13): Face/shoulder buttons

### Theory 2: Firmware is Masking High Bits
In `simple-s2rc/src/main.c` line with button parsing:
```c
current_report.buttons = (uart_buffer[0] | (uart_buffer[1] << 8)) & 0x3FFF;
```
The `& 0x3FFF` masks to 14 bits (0-13), which should be correct. But perhaps the issue is elsewhere.

### Theory 3: Different Button Layout Expected
The HORI controller descriptor may define buttons in the opposite order than we think. Let me check if enabling TEST_MODE in the firmware reveals the actual mapping.

## Solution Approach

### Step 1: Enable TEST_MODE in Firmware

Edit `simple-s2rc/src/main.c` and uncomment line 20:
```c
#define TEST_MODE_ENABLED
```

This will make the Pico cycle through all buttons automatically. Watch your Switch to see which physical buttons activate for each bit.

### Step 2: Rebuild and Flash Firmware

```powershell
cd simple-s2rc/build
cmake --build .
# Flash the new s2rc.uf2 to Pico #2
```

### Step 3: Document the Actual Mapping

The TEST_MODE will cycle through:
- B (bit 0), A (bit 1), Y (bit 2), X (bit 3)
- L (bit 4), R (bit 5), ZL (bit 6), ZR (bit 7)
- MINUS (bit 8), PLUS (bit 9), L-STICK (bit 10), R-STICK (bit 11)
- GL (bit 14), GR (bit 15)
- Then D-Pad directions

Watch which actual Switch buttons activate and record them.

### Step 4: Update All Button Definitions

Once you know the correct mapping, update the `#define BTN_*` values in:

1. **test_buttons.py**
2. **test_buttons_corrected.py** 
3. **uart-bridge/src/main.c**
4. **simple-s2rc/src/main.c**
5. **UART_SWITCH_CONTROLLER_PROJECT.md**

### Step 5: Rebuild Both Firmwares

```powershell
# Rebuild uart-bridge
cd uart-bridge/build
cmake --build .

# Rebuild simple-s2rc (disable TEST_MODE first!)
cd ../../simple-s2rc
# Edit src/main.c - comment out TEST_MODE_ENABLED
cd build
cmake --build .

# Flash both .uf2 files to respective Picos
```

## Alternative: Quick Python-Only Fix

If you don't want to rebuild firmware and the simple-s2rc Pico works correctly, you can just fix the Python scripts to use the confirmed mapping:

```python
# CORRECTED definitions for test_buttons.py
BTN_MINUS   = (1 << 0)  # Bit 0
BTN_PLUS    = (1 << 1)  # Bit 1
BTN_LSTICK  = (1 << 2)  # Bit 2
BTN_RSTICK  = (1 << 3)  # Bit 3
BTN_HOME    = (1 << 4)  # Bit 4
BTN_CAPTURE = (1 << 5)  # Bit 5

# Face/shoulder need to be determined via TEST_MODE
BTN_B       = (1 << ??)
BTN_A       = (1 << ??)
BTN_Y       = (1 << ??)
BTN_X       = (1 << ??)
BTN_L       = (1 << ??)
BTN_R       = (1 << ??)
BTN_ZL      = (1 << ??)
BTN_ZR      = (1 << ??)
```

## Files Created for Diagnostics

1. **diagnose_buttons.py** - Tests each bit individually
2. **diagnose_buttons_v2.py** - Enhanced diagnostic (if buttons are in different bytes)
3. **test_buttons_corrected.py** - Test script with corrected system button mappings
4. **BUTTON_MAPPING_FIX.md** - Detailed guide on the issue
5. **FINDINGS_AND_SOLUTION.md** - This file

## Current Status

✓ System buttons confirmed (bits 0-5)
✗ Face/shoulder buttons not working (bits 6-13)
⚠ Need to enable TEST_MODE to find actual face/shoulder button positions

## Next Steps

1. **Enable TEST_MODE** in simple-s2rc/src/main.c
2. **Rebuild and flash** the firmware
3. **Watch Switch screen** as buttons cycle through
4. **Document** which Switch buttons activate for bits 0-15
5. **Update** all button definitions with correct mapping
6. **Test** with test_buttons_corrected.py to verify

## Questions to Answer

1. Do face buttons (A, B, X, Y) show up in bits 6-9 when using TEST_MODE?
2. Do shoulder buttons (L, R, ZL, ZR) show up in bits 10-13 when using TEST_MODE?
3. Or are they in a completely different order?
4. Are bits 14-15 (GL, GR) even used by this controller?

Once we have answers to these questions, we can fix all the code.
