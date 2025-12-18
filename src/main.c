#include <stdbool.h>
#include "input/input_manager.h"
#include "mapping/mapper.h"
#include "output/output_switch.h"
#include "config.h"
#include <stdio.h>
#include <stdint.h>
#include "../include/controller_state.h"
#include "../include/switch_report.h"
#include "pico/stdlib.h"
#include "input/switch_buttons.h"
#include "driver/switch_driver.h"
#include "tusb.h"

int main() {
    switch_init();
    tusb_init();

    while (1) {
        tud_task();          // TinyUSB background task
        process_controller();
        sleep_ms(10);
    }
}
