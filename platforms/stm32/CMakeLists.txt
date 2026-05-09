cmake_minimum_required(VERSION 3.22)

set(STM32_PROFILE stm32)
set(STM32_CHIP_PROFILE stm32)
set(STM32_BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/board/stm32h723vgtx)
set(STM32_PROJECT_ROOT ${STM32_BOARD_ROOT})
set(STM32_OFFICIAL_ROOT ${STM32_BOARD_ROOT})

include(${STM32_BOARD_ROOT}/cmake/gcc-arm-none-eabi.cmake)

# Keep CubeMX-generated files untouched. Some generated toolchain files use
# CMAKE_SOURCE_DIR for the linker script, but this platform CMake lives one
# level above the board package.
set(STM32_LINKER_SCRIPT ${STM32_BOARD_ROOT}/STM32H723XG_FLASH.ld)
string(REPLACE
    "-T \"${CMAKE_SOURCE_DIR}/STM32H723XG_FLASH.ld\""
    "-T \"${STM32_LINKER_SCRIPT}\""
    CMAKE_C_LINK_FLAGS
    "${CMAKE_C_LINK_FLAGS}"
)
string(REPLACE
    "-T \"${CMAKE_SOURCE_DIR}/STM32H723XG_FLASH.ld\""
    "-T \"${STM32_LINKER_SCRIPT}\""
    CMAKE_CXX_LINK_FLAGS
    "${CMAKE_CXX_LINK_FLAGS}"
)

# Bare-metal toolchains often fail CMake ABI/link probes because the runtime
# stubs (_write/_sbrk/...) are only provided by the real target sources.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(stm32h723vgtx_core_control LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(STM32_ENABLE_TINYUSB_CDC "Enable TinyUSB CDC device module" OFF)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(CLIGHT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(CLIGHT_CONFIG ${CLIGHT_ROOT}/config/clight/${STM32_PROFILE}.yaml)
set(CLIGHT_OUT_ROOT ${CLIGHT_ROOT})
set(CLIGHT_API_INCLUDE_DIR ${CLIGHT_ROOT}/bsp/api)
set(CLIGHT_API_HEADER ${CLIGHT_API_INCLUDE_DIR}/Common.hpp)
set(CLIGHT_BOARD_IDS_HEADER ${CLIGHT_API_INCLUDE_DIR}/BoardIds.hpp)
set(CLIGHT_MANIFEST ${CLIGHT_ROOT}/manifest/${STM32_CHIP_PROFILE}.json)
set(MODULE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../modules)
set(TINYUSB_VENDOR_ROOT ${CLIGHT_ROOT}/vendor/tinyusb)

file(GLOB MODULE_DIR_CANDIDATES CONFIGURE_DEPENDS ${MODULE_ROOT}/*)
set(MODULE_INCLUDE_DIRS)
foreach(MODULE_DIR ${MODULE_DIR_CANDIDATES})
    if(IS_DIRECTORY ${MODULE_DIR})
        list(APPEND MODULE_INCLUDE_DIRS ${MODULE_DIR})
    endif()
endforeach()

add_custom_target(clight_codegen_stm32 ALL
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/clight_codegen/generate_cpp_api.py
            --config ${CLIGHT_CONFIG}
            --out-root ${CLIGHT_OUT_ROOT}
    BYPRODUCTS
        ${CLIGHT_API_HEADER}
        ${CLIGHT_BOARD_IDS_HEADER}
        ${CLIGHT_MANIFEST}
    DEPENDS
        ${CLIGHT_CONFIG}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/clight_codegen/generate_cpp_api.py
    COMMENT "Generating STM32 C++ API"
    VERBATIM
)

set(STM32_HAL_SOURCES
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_adc_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_exti.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_fdcan.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_hsem.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_i2c_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pcd_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_spi_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_usb.c
)

set(STM32_VENDOR_SOURCES
    ${STM32_PROJECT_ROOT}/Core/Src/dma.c
    ${STM32_PROJECT_ROOT}/Core/Src/fdcan.c
    ${STM32_PROJECT_ROOT}/Core/Src/gpio.c
    ${STM32_PROJECT_ROOT}/Core/Src/spi.c
    ${STM32_PROJECT_ROOT}/Core/Src/stm32h7xx_hal_msp.c
    ${STM32_PROJECT_ROOT}/Core/Src/stm32h7xx_it.c
    ${STM32_PROJECT_ROOT}/Core/Src/system_stm32h7xx.c
    ${STM32_PROJECT_ROOT}/Core/Src/tim.c
    ${STM32_PROJECT_ROOT}/Core/Src/usart.c
    ${STM32_BOARD_ROOT}/Core/Src/syscalls.c
    ${STM32_BOARD_ROOT}/Core/Src/sysmem.c
    ${STM32_BOARD_ROOT}/startup_stm32h723xx.s
)

set(STM32_BSP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_adc16_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_comp_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_cordic_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_crc_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_dac_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_dcmi_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_dfsdm_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_dma2d_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_dmav2_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_enet_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_ewdg_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_fdcan_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_fmac_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_gpio_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_hcd_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_i2s_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_i2c_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_jpeg_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_ltdc_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_mdma_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_mmc_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_opamp_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_ospi_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_pcd_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_pssi_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_dma_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_pwm_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_qeiv2_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_qspi_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_rng_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_rtc_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_sai_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_sd_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_sdio_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_smartcard_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_smbus_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_spi_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_tim_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_uart_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_usart_stm32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_wwdg_stm32.cpp
)

set(STM32_TINYUSB_SOURCES)
if(STM32_ENABLE_TINYUSB_CDC)
    if(NOT EXISTS ${TINYUSB_VENDOR_ROOT}/src/tusb.c)
        message(FATAL_ERROR "ABORT:\n missing TinyUSB vendor source: ${TINYUSB_VENDOR_ROOT}")
    endif()

    set(STM32_TINYUSB_SOURCES
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbDeviceCore.cpp
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbCdc.cpp
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbMscDevice.cpp
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbHidDevice.cpp
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbVendorDevice.cpp
        ${MODULE_ROOT}/TinyUsbDevice/usb_descriptors.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/port/stm32/bsp_tinyusb_stm32.c
        ${TINYUSB_VENDOR_ROOT}/src/tusb.c
        ${TINYUSB_VENDOR_ROOT}/src/common/tusb_fifo.c
        ${TINYUSB_VENDOR_ROOT}/src/device/usbd.c
        ${TINYUSB_VENDOR_ROOT}/src/class/cdc/cdc_device.c
        ${TINYUSB_VENDOR_ROOT}/src/class/msc/msc_device.c
        ${TINYUSB_VENDOR_ROOT}/src/class/hid/hid_device.c
        ${TINYUSB_VENDOR_ROOT}/src/class/vendor/vendor_device.c
        ${TINYUSB_VENDOR_ROOT}/src/portable/synopsys/dwc2/dcd_dwc2.c
        ${TINYUSB_VENDOR_ROOT}/src/portable/synopsys/dwc2/dwc2_common.c
    )
endif()

add_executable(stm32_core_control
    ${CMAKE_CURRENT_SOURCE_DIR}/main/board_startup.c
    ${CMAKE_CURRENT_SOURCE_DIR}/main/main.cpp
    ${STM32_HAL_SOURCES}
    ${STM32_VENDOR_SOURCES}
    ${STM32_BSP_SOURCES}
    ${STM32_TINYUSB_SOURCES}
    ${MODULE_ROOT}/Bmi088/Bmi088.cpp
    ${MODULE_ROOT}/Buzzer/Buzzer.cpp
    ${MODULE_ROOT}/ClightSelfTest/ClightSelfTest.cpp
    ${MODULE_ROOT}/Ws2812/Ws2812.cpp
)

add_dependencies(stm32_core_control clight_codegen_stm32)

add_custom_command(TARGET stm32_core_control POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:stm32_core_control> $<TARGET_FILE_DIR:stm32_core_control>/stm32_core_control.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32_core_control> $<TARGET_FILE_DIR:stm32_core_control>/stm32_core_control.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:stm32_core_control>
    COMMENT "Generating STM32 flash images"
)

target_compile_definitions(stm32_core_control PRIVATE
    USE_PWR_LDO_SUPPLY
    USE_HAL_DRIVER
    STM32H723xx
    STM32_ENABLE_TINYUSB_CDC=$<BOOL:${STM32_ENABLE_TINYUSB_CDC}>
    $<$<CONFIG:Debug>:DEBUG>
)

set(STM32_HAL_CONF ${STM32_PROJECT_ROOT}/Core/Inc/stm32h7xx_hal_conf.h)
file(READ ${STM32_HAL_CONF} STM32_HAL_CONF_TEXT)
if(NOT STM32_HAL_CONF_TEXT MATCHES "(^|\n)[ \t]*#define[ \t]+HAL_ADC_MODULE_ENABLED")
    target_compile_definitions(stm32_core_control PRIVATE HAL_ADC_MODULE_ENABLED)
endif()

if(STM32_ENABLE_TINYUSB_CDC)
    target_compile_definitions(stm32_core_control PRIVATE
        CFG_TUSB_MCU=OPT_MCU_STM32H7
        BOARD_TUD_RHPORT=0
        BOARD_TUD_MAX_SPEED=OPT_MODE_FULL_SPEED
        CFG_TUD_MEM_DCACHE_ENABLE=0
    )
endif()

target_include_directories(stm32_core_control PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/main
    ${CLIGHT_API_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/interface
    ${CMAKE_CURRENT_SOURCE_DIR}/../../bsp/chip/${STM32_CHIP_PROFILE}
    ${MODULE_INCLUDE_DIRS}
    ${STM32_PROJECT_ROOT}/Core/Inc
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Inc
    ${STM32_OFFICIAL_ROOT}/Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    ${STM32_OFFICIAL_ROOT}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
    ${STM32_OFFICIAL_ROOT}/Drivers/CMSIS/Include
)

if(STM32_ENABLE_TINYUSB_CDC)
    target_include_directories(stm32_core_control PRIVATE
        ${TINYUSB_VENDOR_ROOT}/src
        ${TINYUSB_VENDOR_ROOT}/src/portable/synopsys/dwc2
    )
endif()

target_compile_options(stm32_core_control PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)
