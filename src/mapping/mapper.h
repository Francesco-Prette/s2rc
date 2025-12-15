#ifndef MAPPER_H
#define MAPPER_H


#include "../input/input_manager.h"
#include "../output/switch_hid_report.h"


SwitchReport map_input_to_switch(ControllerState in);


#endif