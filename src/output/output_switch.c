#include "output_switch.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <string.h>
#include "switch_hid_descriptor.h"


void switch_output_init() {
// Initialize TinyUSB device stack
printf("switch_output: initializing TinyUSB device...\n");
tusb_init();
}


void switch_send_report(const SwitchReport *r) {
    if (!tud_hid_ready()) return;

    switch_hid_report_t rep;
    rep.report_id = SWITCH_REPORT_ID;
    uint16_t buttons = r->buttons[0] | (r->buttons[1] << 8);
    rep.buttons   = buttons;
    rep.hat       = (r->hat & 0x0F);

    rep.lx = r->lx + 128;
    rep.ly = r->ly + 128;
    rep.rx = r->rx + 128;
    rep.ry = r->ry + 128;

    rep.lt = r->lt;
    rep.rt = r->rt;

    tud_hid_report(SWITCH_REPORT_ID,
                   (const uint8_t *)&rep,
                   sizeof(rep));
}