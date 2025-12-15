#include "input_manager.h"
#include "usb_host/usb_host_drivers.h"


static ControllerState last_state;


void input_init() {
usb_host_init();
}


ControllerState input_read() {
// Poll the host interface for a controller
last_state = usb_host_poll_controller();
return last_state;
}