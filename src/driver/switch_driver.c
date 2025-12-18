#include "switch_driver.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <string.h>
#include "switch_report.h"


static SwitchReport switchReport;

void switch_init(void) {
    memset(&switchReport, 0, sizeof(switchReport));
    switchReport.report_id = 0x30;
    // Initialize Pico LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, true);
}

bool send_switch_report(void) {
    if (!tud_hid_ready()) return false;
    return tud_hid_report(switchReport.report_id, &switchReport, sizeof(switchReport));
}

void process_controller(void) {
    static uint32_t last_time = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    // Every 10 seconds, press D-pad right
    if (now - last_time > 10000) {
        switchReport.hat = 0x08;       // Right
        switchReport.buttons[0] = 0x03; // R+L pressed
        send_switch_report();

        switchReport.hat = 0x00;       // release D-pad
        switchReport.buttons[0] = 0x00; // release R+L
        last_time = now;
    }

    // Blink Pico LED every 500 ms
    gpio_put(PICO_DEFAULT_LED_PIN, (now / 500) % 2);
}
