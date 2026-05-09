cmake_minimum_required(VERSION 3.16)

set(CLIGHT_ESP32_TARGET "esp32s3" CACHE STRING "ESP-IDF target used by the Clight ESP32 template")

if(NOT DEFINED ENV{IDF_PATH})
    project(clight_esp32_template LANGUAGES NONE)
    message(STATUS
        "Clight ESP32 template configured without ESP-IDF. "
        "Activate ESP-IDF so IDF_PATH is set, then run idf.py from this directory.")
    return()
endif()

if(CLIGHT_ESP32_TARGET MATCHES "^esp32c|^esp32h|^esp32p")
    set(CLIGHT_ESP32_COMPILER_PREFIX "riscv32-esp-elf")
else()
    set(CLIGHT_ESP32_COMPILER_PREFIX "xtensa-${CLIGHT_ESP32_TARGET}-elf")
endif()

find_program(CLIGHT_ESP32_GCC NAMES "${CLIGHT_ESP32_COMPILER_PREFIX}-gcc")
if(NOT CLIGHT_ESP32_GCC)
    project(clight_esp32_template LANGUAGES NONE)
    message(STATUS
        "ESP-IDF path is set, but ${CLIGHT_ESP32_COMPILER_PREFIX}-gcc is not in PATH. "
        "Run the ESP-IDF export script, then build with idf.py.")
    return()
endif()

set(IDF_TARGET "${CLIGHT_ESP32_TARGET}" CACHE STRING "ESP-IDF target")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(clight_esp32_template)
