#include "switch_device.h"
#include <string.h>

SwitchDeviceInfo deviceInfo;

static const uint8_t STATIC_MAC[6] = {0x7C, 0xBB, 0x8A, 0x01, 0x02, 0x03};

void init_device_info(void) {
    deviceInfo.majorVersion = 0x04;
    deviceInfo.minorVersion = 0x91;
    deviceInfo.controllerType = SWITCH_TYPE_PRO_CONTROLLER;
    deviceInfo.unknown00 = 0x02;
    memcpy(deviceInfo.macAddress, STATIC_MAC, 6);
    deviceInfo.unknown01 = 0x01;
    deviceInfo.storedColors = 0x02;
}
