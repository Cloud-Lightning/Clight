cmake_minimum_required(VERSION 3.20)

project(clight_hpm_template LANGUAGES C CXX)

message(FATAL_ERROR
    "Clight HPM platform is a public template only. Generate a local HPM board package under platforms/hpm/boards/<board_name> and provide the product HPM SDK CMake. See docs/03_generated_files.md.")
