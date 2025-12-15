#pragma once
#include <stdint.h>

typedef struct {
    uint8_t report_id;
    uint8_t buttons[3];
    uint8_t hat;
    int8_t lx;
    int8_t ly;
    int8_t rx;
    int8_t ry;
} SwitchReport;
