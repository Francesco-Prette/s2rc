#include "usb_host_drivers.h"
#include <string.h>
#include "input_hid_generic.h"
#include "controller_state.h"


ControllerState usb_host_poll_controller() {
ControllerState s;
memset(&s, 0, sizeof(s));


// TODO: integrate TinyUSB host API (tuh_* functions) to detect devices and read HID reports.
// Pseudocode:
// - call tuh_task();
// - when HID report arrives, call parse_generic_hid(report, len, &s);


return s;
}