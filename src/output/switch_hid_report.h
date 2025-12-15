#ifndef SWITCH_HID_REPORT_H
#define SWITCH_HID_REPORT_H


#include <stdint.h>


#define SWITCH_A (1 << 0)
#define SWITCH_B (1 << 1)
#define SWITCH_X (1 << 2)
#define SWITCH_Y (1 << 3)


typedef struct {
uint16_t buttons;
int8_t lx, ly;
int8_t rx, ry;
uint8_t lt, rt;
} SwitchReport;


#endif