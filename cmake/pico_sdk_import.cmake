# Placeholder: user should set PICO_SDK_PATH or provide the real import file.
if(NOT DEFINED PICO_SDK_PATH)
message(FATAL_ERROR "Please set PICO_SDK_PATH to your pico-sdk location or add an import file here.")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)