#ifndef MAPPER_H
#define MAPPER_H

#include "../input/input_manager.h"
#include "controller_state.h"
#include "switch_report.h"

SwitchReport map_input_to_switch(ControllerState in);

#endif
