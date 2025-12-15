#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H


#include <stdint.h>


typedef struct {
uint8_t buttons[32];
int16_t lx, ly;
int16_t rx, ry;
uint8_t lt, rt;
} ControllerState;


void input_init();
ControllerState input_read();


#endif