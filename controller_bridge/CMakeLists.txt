cmake_minimum_required(VERSION 3.13)
project(controller_bridge C)

set(CMAKE_C_STANDARD 11)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Source files
set(SOURCES
    src/main.c
    src/serial_port.c
    src/controller_state.c
    src/config.c
    src/input_handler.c
)

# Platform-specific sources
if(WIN32)
    list(APPEND SOURCES src/platform/windows_input.c)
    list(APPEND SOURCES src/platform/windows_serial.c)
elseif(UNIX AND NOT APPLE)
    list(APPEND SOURCES src/platform/linux_input.c)
    list(APPEND SOURCES src/platform/posix_serial.c)
elseif(APPLE)
    list(APPEND SOURCES src/platform/macos_input.c)
    list(APPEND SOURCES src/platform/posix_serial.c)
endif()

# Create executable
add_executable(controller_bridge ${SOURCES})

# Include directories
target_include_directories(controller_bridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Platform-specific libraries
if(WIN32)
    # Use xinput9_1_0 which is included with Windows Vista and later
    # instead of xinput (xinput1_3.dll) which requires DirectX SDK
    target_link_libraries(controller_bridge PRIVATE
        xinput9_1_0
        dinput8
        dxguid
        setupapi
    )
elseif(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(controller_bridge PRIVATE
        Threads::Threads
        m
    )
    if(NOT APPLE)
        # Linux-specific libraries
        target_link_libraries(controller_bridge PRIVATE
            udev
        )
    endif()
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(controller_bridge PRIVATE /W4)
else()
    target_compile_options(controller_bridge PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation
install(TARGETS controller_bridge
    RUNTIME DESTINATION bin
)

# Copy default config on first build
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config/default_config.ini
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/controller_bridge.ini
    COPYONLY
)
