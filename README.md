# pico-controller-converter (starter)


Goal: Read a controller connected on a USB-A port (host mode) and present a Nintendo Switch-compatible HID device to the console (device mode). This project is a **starter skeleton** that wires together the major components (USB host, input parsing, mapping, TinyUSB HID device backend). It is intended for Pico / Pico W (RP2040) and the official Pico SDK.


## Included files
- CMakeLists.txt
- cmake/pico_sdk_import.cmake (placeholder)
- include/config.h
- src/main.c
- src/usb_host/usb_host_init.c
- src/usb_host/usb_host_drivers.c
- src/usb_host/usb_host_drivers.h
- src/input/input_manager.c
- src/input/input_manager.h
- src/input/input_hid_generic.c
- src/mapping/mapper.c
- src/mapping/mapper.h
- src/output/output_switch.c
- src/output/output_switch.h
- src/output/switch_hid_report.h


## Quick build (Linux, with Pico SDK installed)


1. Install Pico SDK, TinyUSB and pico-host-usb (see their docs).
2. Create a build folder:


```bash
mkdir build
cd build
cmake ..
make -j4
```


3. Copy `converter.uf2` to your Pico/RP2040 flash (follow Pico SDK instructions), or use picotool.


## Notes
- This is **not** fully working driver code. It is a well-structured starting point. Key TODOs are marked in source files (USB host parsing, HID report filling). Use TinyUSB host capabilities (`tuh_...`) for host side and TinyUSB device (`tud_...`) for device side.
- You will need to configure TinyUSB for dual role or use two USB controllers (some RP2040 boards/boards support host + device with additional wiring).


## Next steps
- Implement host-side HID parsing for specific controllers (Xbox, DualShock, Switch Pro).
- Create a robust Switch HID descriptor matching the console’s expectations (use GP2040-CE or SwitchKMAdapter as reference).


---
```