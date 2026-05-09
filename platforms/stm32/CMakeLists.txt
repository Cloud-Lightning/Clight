cmake_minimum_required(VERSION 3.20)

project(clight_stm32_template LANGUAGES NONE)

set(CLIGHT_STM32_BOARD "" CACHE STRING "Local STM32CubeMX board package name under platforms/stm32/board")
set(CLIGHT_STM32_BOARD_DIR "" CACHE PATH "Optional absolute path to a local STM32CubeMX board package")

if(NOT CLIGHT_STM32_BOARD AND NOT CLIGHT_STM32_BOARD_DIR)
    message(STATUS
        "Clight STM32 template configured without a CubeMX board package. "
        "Generate one under platforms/stm32/board/<board_name> or set CLIGHT_STM32_BOARD_DIR. "
        "See docs/03_generated_files.md.")
    return()
endif()

if(NOT CLIGHT_STM32_BOARD_DIR)
    set(CLIGHT_STM32_BOARD_DIR "${CMAKE_CURRENT_LIST_DIR}/board/${CLIGHT_STM32_BOARD}")
endif()

if(NOT EXISTS "${CLIGHT_STM32_BOARD_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "STM32 board package not found: ${CLIGHT_STM32_BOARD_DIR}. "
        "Generate Core/Drivers/startup/linker files with STM32CubeMX.")
endif()

message(STATUS "Using local STM32 board package: ${CLIGHT_STM32_BOARD_DIR}")
enable_language(C CXX ASM)
add_subdirectory("${CLIGHT_STM32_BOARD_DIR}" "${CMAKE_BINARY_DIR}/clight_stm32_board")
