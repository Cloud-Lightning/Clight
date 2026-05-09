cmake_minimum_required(VERSION 3.20)

project(clight_stm32_template LANGUAGES C CXX ASM)

message(FATAL_ERROR
    "Clight STM32 platform is a public template only. Generate or provide a CubeMX board package under platforms/stm32/board/<board_name> and use a product CMake. See docs/03_generated_files.md.")
