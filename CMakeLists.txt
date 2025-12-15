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

cmake_minimum_required(VERSION 3.13)


include(cmake/pico_sdk_import.cmake)


project(pico-controller-converter C CXX ASM)


pico_sdk_init()


add_executable(converter
src/main.c


src/usb_host/usb_host_init.c
src/usb_host/usb_host_drivers.c


src/input/input_manager.c
src/input/input_hid_generic.c


src/output/output_switch.c


src/mapping/mapper.c
)


target_include_directories(converter PRIVATE
${CMAKE_CURRENT_LIST_DIR}/include
${CMAKE_CURRENT_LIST_DIR}/src
)


target_link_libraries(converter
pico_stdlib
tinyusb_device
tinyusb_board
pico_host_usb
)


pico_add_extra_outputs(converter)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

target_sources(${PROJECT_NAME} PRIVATE
    src/usb_host/input_hid_generic.c
)