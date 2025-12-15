#include "mapper.h"
#include <string.h>


SwitchReport map_input_to_switch(ControllerState in) {
SwitchReport out;
memset(&out, 0, sizeof(out));


// Simple example mapping - adapt to your controller/button layout
if (in.buttons[0]) out.buttons |= SWITCH_A;
if (in.buttons[1]) out.buttons |= SWITCH_B;


// Map analogs (clamp to -127..127)
out.lx = (int8_t) (in.lx > 127 ? 127 : (in.lx < -127 ? -127 : in.lx));
out.ly = (int8_t) (in.ly > 127 ? 127 : (in.ly < -127 ? -127 : in.ly));
out.rx = (int8_t) (in.rx > 127 ? 127 : (in.rx < -127 ? -127 : in.rx));
out.ry = (int8_t) (in.ry > 127 ? 127 : (in.ry < -127 ? -127 : in.ry));


return out;
}