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

// Polling intervals
#define POLL_MS 10000
#define LED_PIN 25

//int main(void)
//{
//    stdio_init_all();
//
//    while (true) {
//        ControllerState in = input_read();
//        SwitchReport out = map_input_to_switch(in);
//        switch_send_report(&out);
//        sleep_ms(POLL_MS);
//    }
//return 0;
//}

int main() {
    stdio_init_all();
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, true);

    while (true) {
        // Blink LED
        gpio_put(LED_PIN, true);
        sleep_ms(100);
        gpio_put(LED_PIN, false);

        // Prepare Switch report
        SwitchReport rep = {0};   // clear all fields

        // Press R + L
        rep.buttons[0] = SWITCH_L & 0xFF;           // lower 8 bits
        rep.buttons[1] = (SWITCH_R >> 8) & 0xFF;    // upper 8 bits if needed

        rep.hat = 8;  // neutral D-Pad
        rep.lx = 0;
        rep.ly = 0;
        rep.rx = 0;
        rep.ry = 0;
        rep.lt = 0;
        rep.rt = 0;

        // Send HID report
        switch_send_report(&rep);

        // Wait 10 seconds
        sleep_ms(POLL_MS);
    }
}