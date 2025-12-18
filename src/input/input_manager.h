#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <stdint.h>
#include "controller_state.h"

void input_init();
ControllerState input_read(void);

#endif
