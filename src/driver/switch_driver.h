#pragma once
#ifndef SWITCH_DRIVER_H
#define SWITCH_DRIVER_H

#include "controller_state.h"
#include <stdbool.h>

void switch_init(void);
bool send_switch_report(void);
void process_controller(void);

#endif
