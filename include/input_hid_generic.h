#pragma once

#include <stdint.h>            // for uint8_t, uint16_t
#include "controller_state.h"  // for ControllerState

#ifdef __cplusplus
extern "C" {
#endif

ControllerState hid_parse_generic(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
