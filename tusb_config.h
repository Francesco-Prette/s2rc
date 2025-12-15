#pragma once

#define CFG_TUSB_MCU              OPT_MCU_RP2040
#define CFG_TUSB_RHPORT0_MODE    (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#define CFG_TUD_EP_MAX           8

#define CFG_TUD_HID              1
#define CFG_TUD_CDC              0
#define CFG_TUD_MSC              0
#define CFG_TUD_MIDI             0
#define CFG_TUD_VENDOR           0

#define CFG_TUD_HID_EP_BUFSIZE   64