#ifndef OUTPUT_SWITCH_H
#define OUTPUT_SWITCH_H

#include "switch_report.h"

void switch_output_init();
void switch_send_report(const SwitchReport *r);

#endif
