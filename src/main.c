#include "pico/stdlib.h"
#include "input/input_manager.h"
#include "mapping/mapper.h"
#include "output/output_switch.h"
#include "config.h"
#include <stdio.h>
#include <stdint.h>
#include "../include/controller_state.h"
#include "../include/switch_report.h"



int main() {
stdio_init_all();
printf("pico-controller-converter: starting...\n");


input_init();
switch_output_init();


while (true) {
ControllerState in = input_read();
SwitchReport out = map_input_to_switch(in);
switch_send_report(&out);


sleep_ms(POLL_MS);
}


return 0;
}