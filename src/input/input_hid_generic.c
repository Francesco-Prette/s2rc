#include <string.h>
#include "input_manager.h"


void parse_generic_hid(const uint8_t *report, uint16_t len, ControllerState *out) {
// Minimal example parser: adapt to actual HID report layout for your controller
memset(out, 0, sizeof(ControllerState));


if (len < 6) return;


// Example mapping (change per device):
// report[0] = LX (0-255)
// report[1] = LY
// report[2] = RX
// report[3] = RY
// report[4] = buttons low
// report[5] = buttons high


out->lx = (int16_t)report[0] - 128;
out->ly = (int16_t)report[1] - 128;
out->rx = (int16_t)report[2] - 128;
out->ry = (int16_t)report[3] - 128;


uint16_t b = report[4] | (report[5] << 8);
out->buttons[0] = (b & 0x01) ? 1 : 0; // A
out->buttons[1] = (b & 0x02) ? 1 : 0; // B
}