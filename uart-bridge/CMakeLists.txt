# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(uart_bridge C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

# Two firmware options:
# 1. main_pc_keyboard.c - Simple USB CDC serial bridge (for PC keyboard via Python script)
# 2. main.c - Full USB host support (for direct USB keyboard connection)
# Comment/uncomment based on your needs:

add_executable(uart_bridge
    src/main_pc_keyboard.c
)

# To use direct USB keyboard instead, comment out above and uncomment:
# add_executable(uart_bridge
#     src/main.c
# )

pico_set_program_name(uart_bridge "uart_bridge")
pico_set_program_version(uart_bridge "0.1")

# For PC keyboard version (main_pc_keyboard.c):
# Enable USB stdio for serial communication with PC
# UART0 is used for controller data output to Switch Pico
pico_enable_stdio_uart(uart_bridge 0)
pico_enable_stdio_usb(uart_bridge 1)

# Add the standard library to the build
target_link_libraries(uart_bridge
    pico_stdlib
    hardware_uart
)

# If using main.c (USB keyboard host), uncomment and use these instead:
# pico_enable_stdio_uart(uart_bridge 1)
# pico_enable_stdio_usb(uart_bridge 0)
# target_link_libraries(uart_bridge
#     pico_stdlib
#     hardware_uart
#     tinyusb_host
#     tinyusb_board
# )

# Add the standard include files to the build
target_include_directories(uart_bridge PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# Add TinyUSB configuration for device mode
target_compile_definitions(uart_bridge PRIVATE
    CFG_TUSB_MCU=OPT_MCU_RP2040
    PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0
)

pico_add_extra_outputs(uart_bridge)
