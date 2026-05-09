cmake_minimum_required(VERSION 3.20)

project(clight_hpm_template LANGUAGES NONE)

set(CLIGHT_HPM_BOARD "" CACHE STRING "Local HPM board package name under platforms/hpm/boards")
set(CLIGHT_HPM_BOARD_DIR "" CACHE PATH "Optional absolute path to a local HPM board package")

if(NOT CLIGHT_HPM_BOARD AND NOT CLIGHT_HPM_BOARD_DIR)
    message(STATUS
        "Clight HPM template configured without a board package. "
        "Generate one under platforms/hpm/boards/<board_name> or set CLIGHT_HPM_BOARD_DIR. "
        "See docs/03_generated_files.md.")
    return()
endif()

if(NOT CLIGHT_HPM_BOARD_DIR)
    set(CLIGHT_HPM_BOARD_DIR "${CMAKE_CURRENT_LIST_DIR}/boards/${CLIGHT_HPM_BOARD}")
endif()

if(NOT EXISTS "${CLIGHT_HPM_BOARD_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "HPM board package not found: ${CLIGHT_HPM_BOARD_DIR}. "
        "Generate board.c/clock.c/pinmux.c and CMakeLists.txt with the HPM tools.")
endif()

message(STATUS "Using local HPM board package: ${CLIGHT_HPM_BOARD_DIR}")
enable_language(C CXX)
add_subdirectory("${CLIGHT_HPM_BOARD_DIR}" "${CMAKE_BINARY_DIR}/clight_hpm_board")
