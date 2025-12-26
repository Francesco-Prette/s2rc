# Button Mapping Issue - Diagnostic and Fix Guide

## Problem Description

The `test_buttons.py` script sends button commands that activate the wrong buttons on the Nintendo Switch. However, the simple-s2rc Pico firmware works correctly when receiving the same UART data.

## Root Cause Analysis

The issue is a **button mapping mismatch** between what we *think* the HORI controller expects and what it *actually* expects.

### Current Assumptions (May Be Incorrect)

The code currently assumes the "Standard Nintendo Switch HID button order":

```
Bit 0:  B button
Bit 1:  A button  
Bit 2:  Y button
Bit 3:  X button
Bit 4:  L shoulder
Bit 5:  R shoulder
Bit 6:  ZL trigger
Bit 7:  ZR trigger
Bit 8:  MINUS (-)
Bit 9:  PLUS (+)
Bit 10: Left Stick Click
Bit 11: Right Stick Click
Bit 12: HOME
Bit 13: CAPTURE
Bit 14: GL (Grip Left)
Bit 15: GR (Grip Right)
```

### Reality

The actual HORI controller (VID: 0x0F0D, PID: 0x00C1) may use a **different button order** in its HID report. The USB descriptor in `simple-s2rc/src/usb_descriptors.c` defines 14 buttons, but doesn't specify their exact order.

## Diagnostic Process

### Step 1: Run the Diagnostic Tool

```powershell
cd uart-bridge
python diagnose_buttons.py COM3
```

Replace `COM3` with your bridge Pico's serial port.

### Step 2: Document the Results

The script will test each bit (0-15) individually. For each test:
1. A button will be pressed on the Switch
2. Note which button activates (A, B, X, Y, L, R, ZL, ZR, etc.)
3. Press Enter to continue to the next bit

### Step 3: Create the Mapping Table

Create a table like this:

| Bit Position | Switch Button Activated |
|--------------|------------------------|
| 0            | ?                      |
| 1            | ?                      |
| 2            | ?                      |
| 3            | ?                      |
| ...          | ...                    |

## Common HORI Controller Button Orders

Based on various HORI controllers, here are some known orderings:

### HORIPAD (Possible Alternative Mapping)
```
Bit 0:  Y button
Bit 1:  B button
Bit 2:  A button
Bit 3:  X button
Bit 4:  L shoulder
Bit 5:  R shoulder
Bit 6:  ZL trigger
Bit 7:  ZR trigger
Bit 8:  MINUS (-)
Bit 9:  PLUS (+)
Bit 10: Left Stick Click
Bit 11: Right Stick Click
Bit 12: HOME
Bit 13: CAPTURE
```

### Pro Controller Standard
```
Bit 0:  B button
Bit 1:  A button
Bit 2:  Y button
Bit 3:  X button
Bit 4:  L shoulder
Bit 5:  R shoulder
Bit 6:  ZL trigger
Bit 7:  ZR trigger
Bit 8:  MINUS (-)
Bit 9:  PLUS (+)
Bit 10: Left Stick Click
Bit 11: Right Stick Click
Bit 12: HOME
Bit 13: CAPTURE
```

## Fix Application

Once you've identified the correct mapping using the diagnostic tool, update the button definitions in:

1. **test_buttons.py** - Update BTN_* definitions
2. **uart-bridge/src/main.c** - Update BTN_* definitions  
3. **simple-s2rc/src/main.c** - Update BTN_* definitions
4. **UART_SWITCH_CONTROLLER_PROJECT.md** - Update documentation

### Example Fix (if Y and A are swapped)

If you discover that:
- Bit 0 actually activates **Y** (not B)
- Bit 2 actually activates **B** (not Y)

Then update the definitions:

```c
// OLD (incorrect)
#define BTN_B       (1 << 0)
#define BTN_Y       (1 << 2)

// NEW (correct)
#define BTN_Y       (1 << 0)
#define BTN_B       (1 << 2)
```

## Verification

After applying fixes:

1. Rebuild simple-s2rc firmware (if you changed main.c):
```powershell
cd simple-s2rc/build
cmake --build .
# Re-flash s2rc.uf2 to Pico #2
```

2. Test with the diagnostic tool again to confirm all buttons work correctly

3. Test with test_buttons.py:
```powershell
python test_buttons.py COM3
```

4. Verify each button name matches the Switch button that activates

## Alternative Quick Fix

If you don't want to rebuild firmware, you can create a "corrected" version of test_buttons.py that has the right mappings while keeping the firmware unchanged. This is useful if the simple-s2rc Pico is working fine and you just need the Python script to match.

## Notes

- The simple-s2rc firmware is just passing through the button data it receives
- The actual button interpretation happens at the Switch based on the USB descriptors
- The USB descriptors define 14 buttons in sequential order (Button 1-14)
- The HID descriptor doesn't name them - the Switch interprets based on the HORI VID/PID

## Next Steps

1. Run `diagnose_buttons.py` to identify the actual button mapping
2. Report findings here or update the code directly
3. Rebuild and reflash firmware if needed
4. Update all documentation with correct mappings
5. Test thoroughly with all buttons and combinations
