set(REPO_ROOT ${CMAKE_CURRENT_LIST_DIR}/../../..)
set(ESP_PROFILE esp32s3_devkitc)
set(ESP_CHIP_PROFILE esp32)
set(API_OUT_ROOT ${REPO_ROOT})
set(API_INCLUDE_DIR ${API_OUT_ROOT}/bsp/api)
set(API_CONFIG ${REPO_ROOT}/config/clight/esp32.yaml)
set(MODULE_ROOT ${REPO_ROOT}/modules)
set(SERVICE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../services)
set(TINYUSB_VENDOR_ROOT ${REPO_ROOT}/vendor/tinyusb)

set(ESP32_ENABLE_TINYUSB_DEVICE OFF CACHE BOOL "Enable Clight TinyUSB device classes on ESP32-S3")
set(ESP32_ENABLE_TINYUSB_HOST OFF CACHE BOOL "Enable Clight TinyUSB host classes on ESP32-S3")

file(GLOB MODULE_DIR_CANDIDATES ${MODULE_ROOT}/*)
set(MODULE_INCLUDE_DIRS)
foreach(MODULE_DIR ${MODULE_DIR_CANDIDATES})
    if(IS_DIRECTORY ${MODULE_DIR})
        list(APPEND MODULE_INCLUDE_DIRS ${MODULE_DIR})
    endif()
endforeach()

execute_process(
    COMMAND ${PYTHON} ${REPO_ROOT}/tools/clight_codegen/generate_cpp_api.py
            --config ${API_CONFIG}
            --out-root ${API_OUT_ROOT}
    RESULT_VARIABLE CODEGEN_RESULT
)

if(NOT CODEGEN_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate ESP32 C++ API headers")
endif()

set(ESP32_TINYUSB_SOURCES)
if(ESP32_ENABLE_TINYUSB_DEVICE)
    if(NOT EXISTS ${TINYUSB_VENDOR_ROOT}/src/tusb.c)
        message(FATAL_ERROR "missing TinyUSB vendor source: ${TINYUSB_VENDOR_ROOT}")
    endif()
    list(APPEND ESP32_TINYUSB_SOURCES
        "${REPO_ROOT}/bsp/port/esp32/bsp_tinyusb_esp32.c"
        "${MODULE_ROOT}/TinyUsbDevice/TinyUsbDeviceCore.cpp"
        "${MODULE_ROOT}/TinyUsbDevice/TinyUsbCdc.cpp"
        "${MODULE_ROOT}/TinyUsbDevice/TinyUsbMscDevice.cpp"
        "${MODULE_ROOT}/TinyUsbDevice/TinyUsbHidDevice.cpp"
        "${MODULE_ROOT}/TinyUsbDevice/TinyUsbVendorDevice.cpp"
        "${MODULE_ROOT}/TinyUsbDevice/usb_descriptors.c"
        "${TINYUSB_VENDOR_ROOT}/src/tusb.c"
        "${TINYUSB_VENDOR_ROOT}/src/common/tusb_fifo.c"
        "${TINYUSB_VENDOR_ROOT}/src/device/usbd.c"
        "${TINYUSB_VENDOR_ROOT}/src/class/cdc/cdc_device.c"
        "${TINYUSB_VENDOR_ROOT}/src/class/msc/msc_device.c"
        "${TINYUSB_VENDOR_ROOT}/src/class/hid/hid_device.c"
        "${TINYUSB_VENDOR_ROOT}/src/class/vendor/vendor_device.c"
        "${TINYUSB_VENDOR_ROOT}/src/portable/synopsys/dwc2/dcd_dwc2.c"
        "${TINYUSB_VENDOR_ROOT}/src/portable/synopsys/dwc2/dwc2_common.c"
    )
endif()

if(ESP32_ENABLE_TINYUSB_HOST)
    if(NOT ESP32_ENABLE_TINYUSB_DEVICE)
        list(APPEND ESP32_TINYUSB_SOURCES
            "${REPO_ROOT}/bsp/port/esp32/bsp_tinyusb_esp32.c"
            "${TINYUSB_VENDOR_ROOT}/src/tusb.c"
            "${TINYUSB_VENDOR_ROOT}/src/common/tusb_fifo.c"
            "${TINYUSB_VENDOR_ROOT}/src/portable/synopsys/dwc2/dwc2_common.c"
        )
    endif()
    list(APPEND ESP32_TINYUSB_SOURCES
        "${MODULE_ROOT}/TinyUsbHost/TinyUsbHost.cpp"
        "${TINYUSB_VENDOR_ROOT}/src/host/usbh.c"
        "${TINYUSB_VENDOR_ROOT}/src/class/msc/msc_host.c"
        "${TINYUSB_VENDOR_ROOT}/src/class/hid/hid_host.c"
        "${TINYUSB_VENDOR_ROOT}/src/portable/synopsys/dwc2/hcd_dwc2.c"
    )
endif()

idf_component_register(
    SRCS
        "main.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_acmp_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_adc16_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_cam_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_dac_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_gpio_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_uart_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_spi_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_i2c_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_gptmr_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_i2s_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_jpeg_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_lcd_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_mcpwm_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_parlio_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_pcnt_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_ppa_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_pwm_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_pwmv2_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_rmt_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_sdio_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_sdm_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_sdmmc_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_sdspi_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_touch_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_tsens_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_usb_serial_jtag_esp32.cpp"
        "${REPO_ROOT}/bsp/port/esp32/bsp_wifi_esp32.cpp"
        "${SERVICE_ROOT}/NvsService.cpp"
        "${SERVICE_ROOT}/WifiService.cpp"
        "${SERVICE_ROOT}/HttpServerService.cpp"
        "${SERVICE_ROOT}/BleService.cpp"
        ${ESP32_TINYUSB_SOURCES}
        "${MODULE_ROOT}/Bmi088/Bmi088.cpp"
        "${MODULE_ROOT}/Mpu6050/Mpu6050.cpp"
        "${MODULE_ROOT}/Ssd1306/Ssd1306.cpp"
        "${MODULE_ROOT}/St7789/St7789.cpp"
        "${MODULE_ROOT}/Ec11/Ec11.cpp"
        "${MODULE_ROOT}/Mfrc522/Mfrc522.cpp"
        "${MODULE_ROOT}/Ws2812/Ws2812.cpp"
        "${MODULE_ROOT}/SdCard/SdCard.cpp"
        "${MODULE_ROOT}/LvglDisplay/LvglDisplay.cpp"
        "${MODULE_ROOT}/St77735/St77735.cpp"
    INCLUDE_DIRS
        "."
        "${API_INCLUDE_DIR}"
        "${REPO_ROOT}"
        "${REPO_ROOT}/bsp/interface"
        "${REPO_ROOT}/bsp/chip/${ESP_CHIP_PROFILE}"
        "${SERVICE_ROOT}"
        ${MODULE_INCLUDE_DIRS}
    REQUIRES
        driver
        esp_adc
        esp_driver_tsens
        esp_driver_usb_serial_jtag
        esp_event
        esp_http_server
        esp_netif
        esp_timer
        esp_wifi
        nvs_flash
        bt
        usb
)

if(ESP32_ENABLE_TINYUSB_DEVICE OR ESP32_ENABLE_TINYUSB_HOST)
    target_include_directories(${COMPONENT_LIB} PRIVATE
        "${TINYUSB_VENDOR_ROOT}/src"
        "${TINYUSB_VENDOR_ROOT}/src/portable/synopsys/dwc2"
    )
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        CFG_TUSB_MCU=OPT_MCU_ESP32S3
        BOARD_TUD_RHPORT=0
        BOARD_TUD_MAX_SPEED=OPT_MODE_FULL_SPEED
    )
endif()

if(ESP32_ENABLE_TINYUSB_HOST)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        CFG_TUH_ENABLED=1
        BOARD_TUH_RHPORT=0
        BOARD_TUH_MAX_SPEED=OPT_MODE_FULL_SPEED
        CLIGHT_TINYUSB_HOST_ENABLE=1
    )
    if(NOT ESP32_ENABLE_TINYUSB_DEVICE)
        target_compile_definitions(${COMPONENT_LIB} PRIVATE CFG_TUD_ENABLED=0)
    endif()
endif()

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20 -fno-exceptions -fno-rtti)
