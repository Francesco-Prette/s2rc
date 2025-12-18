#ifndef SWITCH_DEVICE_H
#define SWITCH_DEVICE_H

#include <stdint.h>

#define SWITCH_TYPE_PRO_CONTROLLER 0x02

typedef struct {
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint8_t controllerType;
    uint8_t unknown00;
    uint8_t macAddress[6];
    uint8_t unknown01;
    uint8_t storedColors;
} SwitchDeviceInfo;

extern SwitchDeviceInfo deviceInfo;
void init_device_info(void);

#endif
