#pragma once
#include <stdint.h>

typedef struct {
    int16_t lx;
    int16_t ly;
    int16_t rx;
    int16_t ry;
    uint16_t buttons[2];
    uint8_t hat;
} ControllerState;
