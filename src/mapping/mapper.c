#include "mapper.h"
#include <string.h>
#include "input/switch_buttons.h"

SwitchReport map_input_to_switch(ControllerState in) {
    SwitchReport out;
    memset(&out, 0, sizeof(out));

    // Pack buttons[2] into a uint16_t for bitwise operations
    uint16_t buttons = 0;

    if (in.buttons[0]) buttons |= SWITCH_A;
    if (in.buttons[1]) buttons |= SWITCH_B;
    if (in.buttons[2]) buttons |= SWITCH_X;
    if (in.buttons[3]) buttons |= SWITCH_Y;
    if (in.buttons[4]) buttons |= SWITCH_L;
    if (in.buttons[5]) buttons |= SWITCH_R;
    if (in.buttons[6]) buttons |= SWITCH_ZL;
    if (in.buttons[7]) buttons |= SWITCH_ZR;
    if (in.buttons[8]) buttons |= SWITCH_PLUS;
    if (in.buttons[9]) buttons |= SWITCH_MINUS;
    if (in.buttons[10]) buttons |= SWITCH_LSTICK;
    if (in.buttons[11]) buttons |= SWITCH_RSTICK;
    if (in.buttons[12]) buttons |= SWITCH_LPAD;
    if (in.buttons[13]) buttons |= SWITCH_RPAD;
    if (in.buttons[14]) buttons |= SWITCH_HOME;
    if (in.buttons[15]) buttons |= SWITCH_CAPTURE;

    // Split uint16_t into 2 bytes for buttons[2] array
    out.buttons[0] = buttons & 0xFF;         // lower 8 bits
    out.buttons[1] = (buttons >> 8) & 0xFF;  // upper 8 bits

    // Map analogs (clamp to -127..127)
    out.lx = (int8_t)(in.lx > 127 ? 127 : (in.lx < -127 ? -127 : in.lx));
    out.ly = (int8_t)(in.ly > 127 ? 127 : (in.ly < -127 ? -127 : in.ly));
    out.rx = (int8_t)(in.rx > 127 ? 127 : (in.rx < -127 ? -127 : in.rx));
    out.ry = (int8_t)(in.ry > 127 ? 127 : (in.ry < -127 ? -127 : in.ry));

    // Map triggers if your ControllerState has them
    // out.lt = in.lt;
    // out.rt = in.rt;

    return out;
}
