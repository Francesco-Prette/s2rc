#ifndef USB_HOST_DRIVERS_H
#define USB_HOST_DRIVERS_H


#include "../input/input_manager.h"


void usb_host_init();
ControllerState usb_host_poll_controller();


#endif