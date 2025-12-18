#pragma once
#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

#include <stdint.h>

typedef struct {
    uint8_t buttons[2]; // buttons[0]: R/L, buttons[1]: unused
    int16_t lx, ly, rx, ry;
    uint8_t dpad;       // bitfield: 0x01=up, 0x02=down, 0x04=left, 0x08=right
} ControllerState;

#endif
