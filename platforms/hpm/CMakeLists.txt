cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME template_app)

if(NOT DEFINED BOARD_TYPE)
    set(BOARD_TYPE "hpm5e31_lqfp")
endif()

if(NOT DEFINED BOARD)
    set(BOARD ${BOARD_TYPE})
endif()
set(BOARD ${BOARD_TYPE} CACHE STRING "Active board name" FORCE)

option(APP_ENABLE_LWIP_TCPECHO "Enable lwIP TCP echo demo on hpm5e31_lqfp" OFF)
option(APP_ENABLE_LWIP_HTTP_SERVER "Enable lwIP HTTP server demo on hpm5e31_lqfp" OFF)
option(APP_ENABLE_ETHERCAT_IO "Enable EtherCAT IO demo on hpm5e31_lqfp" ON)
option(APP_ENABLE_TINYUSB_CDC "Enable TinyUSB CDC device module on hpm5e31_lqfp" OFF)
option(APP_ENABLE_CLIGHT_SELFTEST "Run Clight API/BSP self-test on hpm5e31_lqfp" OFF)

if((APP_ENABLE_LWIP_TCPECHO OR APP_ENABLE_LWIP_HTTP_SERVER OR APP_ENABLE_ETHERCAT_IO OR APP_ENABLE_TINYUSB_CDC OR APP_ENABLE_CLIGHT_SELFTEST) AND NOT BOARD_TYPE STREQUAL "hpm5e31_lqfp")
    message(FATAL_ERROR "ABORT:\n ENET/ECAT/USB demo switches are only supported on BOARD_TYPE=hpm5e31_lqfp")
endif()

if(APP_ENABLE_LWIP_TCPECHO AND APP_ENABLE_LWIP_HTTP_SERVER)
    message(FATAL_ERROR "ABORT:\n Enable only one lwIP demo app at a time: TCP echo or HTTP server")
endif()

message(STATUS "Building for board: ${BOARD_TYPE}")
message(STATUS "BOARD: ${BOARD}")
message(STATUS "APP_ENABLE_LWIP_TCPECHO: ${APP_ENABLE_LWIP_TCPECHO}")
message(STATUS "APP_ENABLE_LWIP_HTTP_SERVER: ${APP_ENABLE_LWIP_HTTP_SERVER}")
message(STATUS "APP_ENABLE_ETHERCAT_IO: ${APP_ENABLE_ETHERCAT_IO}")
message(STATUS "APP_ENABLE_TINYUSB_CDC: ${APP_ENABLE_TINYUSB_CDC}")
message(STATUS "APP_ENABLE_CLIGHT_SELFTEST: ${APP_ENABLE_CLIGHT_SELFTEST}")

if(APP_ENABLE_TINYUSB_CDC)
    set(CONFIG_TINYUSB 1)
    set(CONFIG_USB_DEVICE 1)
    set(CONFIG_USB_DEVICE_CDC 1)
endif()

set(CLIGHT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(WORKSPACE_ROOT ${CLIGHT_ROOT}/..)
set(APP_ROOT ${WORKSPACE_ROOT}/app)
set(BOARD_SEARCH_PATH ${CLIGHT_ROOT}/platforms/hpm/boards)
unset(BOARD_SEARCH_DIR CACHE)
set(RV_ARCH "rv32imafdc_zicsr_zifencei")
set(RV_ABI "ilp32d")
set(CUSTOM_TARGET_TRIPLET "riscv32-elf")
set(HPM_SDK_LD_NO_NANO_SPECS 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
find_package(Python3 COMPONENTS Interpreter REQUIRED)

project(${PROJECT_NAME})

set(BSP_IF_DIR ${CLIGHT_ROOT}/bsp/interface)
set(BSP_CHIP_ROOT ${CLIGHT_ROOT}/bsp/chip)
set(BSP_PORT_DIR ${CLIGHT_ROOT}/bsp/port/hpm)
set(MODULE_ROOT ${CLIGHT_ROOT}/modules)
set(VENDOR_ROOT ${CLIGHT_ROOT}/vendor)
set(CLIGHT_CODEGEN_DIR ${CLIGHT_ROOT}/tools/clight_codegen)

file(GLOB MODULE_DIR_CANDIDATES CONFIGURE_DEPENDS ${MODULE_ROOT}/*)
set(MODULE_INCLUDE_DIRS)
foreach(MODULE_DIR ${MODULE_DIR_CANDIDATES})
    if(IS_DIRECTORY ${MODULE_DIR})
        list(APPEND MODULE_INCLUDE_DIRS ${MODULE_DIR})
    endif()
endforeach()

if(BOARD_TYPE STREQUAL "hpm5e31_lqfp")
    set(BSP_PROFILE hpm)
    set(CLIGHT_PROFILE hpm)
    set(APP_ENTRY_SRC ${CLIGHT_ROOT}/platforms/hpm/main/main.cpp)
else()
    message(FATAL_ERROR "ABORT:\n unsupported BOARD_TYPE: ${BOARD_TYPE}")
endif()

set(BSP_CONFIG_DIR ${BSP_CHIP_ROOT}/${BSP_PROFILE})
set(BSP_CONFIG_FILE ${BSP_CONFIG_DIR}/board_config.h)
set(CLIGHT_CONFIG ${CLIGHT_ROOT}/config/clight/${CLIGHT_PROFILE}.yaml)
set(CLIGHT_OUT_ROOT ${CLIGHT_ROOT})
set(CLIGHT_API_INCLUDE_DIR ${CLIGHT_ROOT}/bsp/api)
set(CLIGHT_API_HEADER ${CLIGHT_API_INCLUDE_DIR}/Common.hpp)
set(CLIGHT_BOARD_IDS_HEADER ${CLIGHT_API_INCLUDE_DIR}/BoardIds.hpp)
set(CLIGHT_MANIFEST ${CLIGHT_ROOT}/manifest/${CLIGHT_PROFILE}.json)

if(NOT EXISTS ${BSP_CONFIG_FILE})
    message(FATAL_ERROR "ABORT:\n missing Clight BSP config: ${BSP_CONFIG_FILE}")
endif()

if(NOT EXISTS ${CLIGHT_CONFIG})
    message(FATAL_ERROR "ABORT:\n missing clight profile: ${CLIGHT_CONFIG}")
endif()

add_custom_target(clight_codegen ALL
    COMMAND ${Python3_EXECUTABLE}
            ${CLIGHT_CODEGEN_DIR}/generate_cpp_api.py
            --config ${CLIGHT_CONFIG}
            --out-root ${CLIGHT_OUT_ROOT}
    BYPRODUCTS
        ${CLIGHT_API_HEADER}
        ${CLIGHT_BOARD_IDS_HEADER}
        ${CLIGHT_MANIFEST}
    DEPENDS
        ${CLIGHT_CONFIG}
        ${CLIGHT_CODEGEN_DIR}/generate_cpp_api.py
    COMMENT "Generating clight C++ API"
    VERBATIM
)

set(APP_SOURCES
    ${APP_ENTRY_SRC}
    ${BSP_PORT_DIR}/bsp_acmp_hpm.cpp
    ${BSP_PORT_DIR}/bsp_adc16_hpm.cpp
    ${BSP_PORT_DIR}/bsp_gpio_hpm.cpp
    ${BSP_PORT_DIR}/bsp_gptmr_hpm.cpp
    ${BSP_PORT_DIR}/bsp_pwm_hpm.cpp
    ${BSP_PORT_DIR}/bsp_pwmv2_hpm.cpp
    ${BSP_PORT_DIR}/bsp_trgm_hpm.cpp
    ${BSP_PORT_DIR}/bsp_uart_hpm.cpp
    ${BSP_PORT_DIR}/bsp_i2c_hpm.cpp
    ${BSP_PORT_DIR}/bsp_spi_hpm.cpp
    ${BSP_PORT_DIR}/bsp_crc_hpm.cpp
    ${BSP_PORT_DIR}/bsp_ewdg_hpm.cpp
    ${BSP_PORT_DIR}/bsp_eui_hpm.cpp
    ${BSP_PORT_DIR}/bsp_rng_hpm.cpp
    ${BSP_PORT_DIR}/bsp_rtc_hpm.cpp
    ${BSP_PORT_DIR}/bsp_dac_hpm.cpp
    ${BSP_PORT_DIR}/bsp_dmav2_hpm.cpp
    ${BSP_PORT_DIR}/bsp_i2s_hpm.cpp
    ${BSP_PORT_DIR}/bsp_irq_hpm.c
    ${BSP_PORT_DIR}/bsp_l1c_hpm.cpp
    ${BSP_PORT_DIR}/bsp_lobs_hpm.cpp
    ${BSP_PORT_DIR}/bsp_mbx_hpm.cpp
    ${BSP_PORT_DIR}/bsp_mchtmr_hpm.cpp
    ${BSP_PORT_DIR}/bsp_sai_hpm.cpp
    ${BSP_PORT_DIR}/bsp_sdio_hpm.cpp
    ${BSP_PORT_DIR}/bsp_sd_hpm.cpp
    ${BSP_PORT_DIR}/bsp_mmc_hpm.cpp
    ${BSP_PORT_DIR}/bsp_owr_hpm.cpp
    ${BSP_PORT_DIR}/bsp_ospi_hpm.cpp
    ${BSP_PORT_DIR}/bsp_pdgo_hpm.cpp
    ${BSP_PORT_DIR}/bsp_plb_hpm.cpp
    ${BSP_PORT_DIR}/bsp_plic_hpm.cpp
    ${BSP_PORT_DIR}/bsp_pllctlv2_hpm.cpp
    ${BSP_PORT_DIR}/bsp_ppi_hpm.cpp
    ${BSP_PORT_DIR}/bsp_ppor_hpm.cpp
    ${BSP_PORT_DIR}/bsp_ptpc_hpm.cpp
    ${BSP_PORT_DIR}/bsp_qeiv2_hpm.cpp
    ${BSP_PORT_DIR}/bsp_qeov2_hpm.cpp
    ${BSP_PORT_DIR}/bsp_qspi_hpm.cpp
    ${BSP_PORT_DIR}/bsp_sdm_hpm.cpp
    ${BSP_PORT_DIR}/bsp_enet_hpm.cpp
    ${BSP_PORT_DIR}/bsp_ecat_hpm.cpp
    ${BSP_PORT_DIR}/bsp_mcan_hpm.cpp
    ${BSP_PORT_DIR}/bsp_synt_hpm.cpp
    ${BSP_PORT_DIR}/bsp_tsns_hpm.cpp
    ${BSP_PORT_DIR}/bsp_dma_dispatch_hpm.cpp
    ${MODULE_ROOT}/ClightSelfTest/ClightSelfTest.cpp
    ${MODULE_ROOT}/St7789/St7789.cpp
    ${MODULE_ROOT}/St77735/St77735.cpp
)

if(APP_ENABLE_TINYUSB_CDC)
    list(APPEND APP_SOURCES
        ${BSP_PORT_DIR}/bsp_tinyusb_hpm.c
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbDeviceCore.cpp
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbCdc.cpp
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbMscDevice.cpp
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbHidDevice.cpp
        ${MODULE_ROOT}/TinyUsbDevice/TinyUsbVendorDevice.cpp
        ${MODULE_ROOT}/TinyUsbDevice/usb_descriptors.c
    )
endif()

if(APP_ENABLE_LWIP_TCPECHO OR APP_ENABLE_LWIP_HTTP_SERVER)
    set(LWIP_VENDOR_DIR ${VENDOR_ROOT}/hpm/lwip)
    set(LWIP_UPSTREAM_DIR ${LWIP_VENDOR_DIR}/upstream/src)
    set(LWIP_VENDOR_ROOT ${LWIP_VENDOR_DIR}/hpm_lwip)
    set(LWIP_PORT_DIR ${LWIP_VENDOR_ROOT}/ports/baremetal/single)
    set(LWIP_COMMON_DIR ${LWIP_VENDOR_ROOT}/common/single)
    set(LWIP_COMMON_SHARED_DIR ${LWIP_VENDOR_ROOT}/common)
    set(ENET_PHY_VENDOR_DIR ${VENDOR_ROOT}/hpm/enet_phy)

    set(LWIP_UPSTREAM_SOURCES
        ${LWIP_UPSTREAM_DIR}/core/init.c
        ${LWIP_UPSTREAM_DIR}/core/def.c
        ${LWIP_UPSTREAM_DIR}/core/dns.c
        ${LWIP_UPSTREAM_DIR}/core/inet_chksum.c
        ${LWIP_UPSTREAM_DIR}/core/ip.c
        ${LWIP_UPSTREAM_DIR}/core/mem.c
        ${LWIP_UPSTREAM_DIR}/core/memp.c
        ${LWIP_UPSTREAM_DIR}/core/netif.c
        ${LWIP_UPSTREAM_DIR}/core/pbuf.c
        ${LWIP_UPSTREAM_DIR}/core/raw.c
        ${LWIP_UPSTREAM_DIR}/core/stats.c
        ${LWIP_UPSTREAM_DIR}/core/sys.c
        ${LWIP_UPSTREAM_DIR}/core/altcp.c
        ${LWIP_UPSTREAM_DIR}/core/altcp_alloc.c
        ${LWIP_UPSTREAM_DIR}/core/altcp_tcp.c
        ${LWIP_UPSTREAM_DIR}/core/tcp.c
        ${LWIP_UPSTREAM_DIR}/core/tcp_in.c
        ${LWIP_UPSTREAM_DIR}/core/tcp_out.c
        ${LWIP_UPSTREAM_DIR}/core/timeouts.c
        ${LWIP_UPSTREAM_DIR}/core/udp.c
        ${LWIP_UPSTREAM_DIR}/core/ipv4/autoip.c
        ${LWIP_UPSTREAM_DIR}/core/ipv4/dhcp.c
        ${LWIP_UPSTREAM_DIR}/core/ipv4/etharp.c
        ${LWIP_UPSTREAM_DIR}/core/ipv4/icmp.c
        ${LWIP_UPSTREAM_DIR}/core/ipv4/igmp.c
        ${LWIP_UPSTREAM_DIR}/core/ipv4/ip4_frag.c
        ${LWIP_UPSTREAM_DIR}/core/ipv4/ip4.c
        ${LWIP_UPSTREAM_DIR}/core/ipv4/ip4_addr.c
        ${LWIP_UPSTREAM_DIR}/netif/ethernet.c
    )

    set(ENET_PHY_VENDOR_SOURCES
        ${ENET_PHY_VENDOR_DIR}/rtl8211/hpm_rtl8211.c
    )

    set(LWIP_INCLUDE_DIRS
        ${ENET_PHY_VENDOR_DIR}
        ${ENET_PHY_VENDOR_DIR}/rtl8211
        ${LWIP_UPSTREAM_DIR}
        ${LWIP_UPSTREAM_DIR}/include
        ${LWIP_UPSTREAM_DIR}/include/lwip/apps
        ${LWIP_PORT_DIR}
        ${LWIP_PORT_DIR}/arch
        ${LWIP_COMMON_SHARED_DIR}
        ${LWIP_COMMON_DIR}
    )

    set(LWIP_APP_SOURCES
        ${LWIP_PORT_DIR}/arch/sys_arch.c
        ${LWIP_PORT_DIR}/ethernetif.c
        ${LWIP_COMMON_DIR}/common.c
        ${LWIP_COMMON_DIR}/netconf.c
    )

    if(NOT APP_ENABLE_ETHERCAT_IO)
        list(APPEND LWIP_APP_SOURCES
            ${APP_ROOT}/src/assets/GifAssets.cpp
        )
    endif()

    if(APP_ENABLE_LWIP_TCPECHO)
        set(LWIP_TCPECHO_DIR ${LWIP_VENDOR_DIR}/apps/lwip_tcpecho)
        list(APPEND LWIP_APP_SOURCES
            ${LWIP_TCPECHO_DIR}/src/app/tcp_echo.c
        )
        list(APPEND LWIP_INCLUDE_DIRS
            ${LWIP_TCPECHO_DIR}/inc
            ${LWIP_TCPECHO_DIR}/inc/app
        )
    endif()

    if(APP_ENABLE_LWIP_HTTP_SERVER)
        set(LWIP_HTTP_SERVER_DIR ${LWIP_VENDOR_DIR}/apps/lwip_http_server)
        list(APPEND LWIP_APP_SOURCES
            ${LWIP_UPSTREAM_DIR}/apps/http/httpd.c
            ${LWIP_UPSTREAM_DIR}/apps/http/fs.c
        )
        list(APPEND LWIP_INCLUDE_DIRS
            ${LWIP_HTTP_SERVER_DIR}/inc
        )
    endif()

    list(APPEND APP_SOURCES ${LWIP_UPSTREAM_SOURCES} ${ENET_PHY_VENDOR_SOURCES} ${LWIP_APP_SOURCES})
endif()

if(APP_ENABLE_ETHERCAT_IO)
    set(EEPROM_VENDOR_DIR ${VENDOR_ROOT}/hpm/eeprom_emulation)
    set(ECAT_VENDOR_DIR ${VENDOR_ROOT}/hpm/ethercat)
    set(ECAT_PORT_DIR ${ECAT_VENDOR_DIR}/port)
    set(ECAT_APP_DIR ${ECAT_VENDOR_DIR}/application)
    set(ECAT_SSC_DIR ${ECAT_VENDOR_DIR}/SSC/Src)

    set(ECAT_APP_SOURCES
        ${EEPROM_VENDOR_DIR}/eeprom_emulation.c
        ${EEPROM_VENDOR_DIR}/hpm_nor_flash.c
        ${ECAT_PORT_DIR}/hpm_ecat_hw.c
        ${ECAT_PORT_DIR}/hpm_ecat_e2p_emulation.c
        ${ECAT_VENDOR_DIR}/hpm_ecat_phy_lan8720.c
        ${ECAT_APP_DIR}/digital_io.c
        ${APP_ROOT}/src/assets/GifAssets.cpp
        ${APP_ROOT}/src/assets/BuildAsset.cpp
        ${APP_ROOT}/src/assets/ZioAsset.cpp
        ${APP_ROOT}/src/assets/RiderLogoAssets.cpp
        ${ECAT_SSC_DIR}/coeappl.c
        ${ECAT_SSC_DIR}/ecatappl.c
        ${ECAT_SSC_DIR}/ecatcoe.c
        ${ECAT_SSC_DIR}/ecatslv.c
        ${ECAT_SSC_DIR}/mailbox.c
        ${ECAT_SSC_DIR}/objdef.c
        ${ECAT_SSC_DIR}/sdoserv.c
    )

    set(ECAT_INCLUDE_DIRS
        ${EEPROM_VENDOR_DIR}
        ${ECAT_VENDOR_DIR}
        ${ECAT_PORT_DIR}
        ${ECAT_APP_DIR}
        ${ECAT_SSC_DIR}
    )

    list(APPEND APP_SOURCES ${ECAT_APP_SOURCES})
endif()

sdk_app_src(${APP_SOURCES})
add_dependencies(app clight_codegen)

sdk_compile_options("-mext-dsp")
if(APP_ENABLE_LWIP_TCPECHO OR APP_ENABLE_LWIP_HTTP_SERVER OR APP_ENABLE_ETHERCAT_IO)
    sdk_compile_options("-Os")
endif()
sdk_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>")
sdk_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>")
sdk_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>")

sdk_ld_options("-Wl,--allow-multiple-definition")
sdk_ld_options("-Wl,--print-memory-usage")
sdk_ld_options("-lm")
sdk_ld_options("-lc")

sdk_inc(${WORKSPACE_ROOT})
sdk_inc(${CLIGHT_ROOT})
sdk_inc(${BSP_IF_DIR})
sdk_inc(${BSP_CONFIG_DIR})
foreach(MODULE_INCLUDE_DIR ${MODULE_INCLUDE_DIRS})
    sdk_inc(${MODULE_INCLUDE_DIR})
endforeach()
sdk_inc(${CLIGHT_API_INCLUDE_DIR})
sdk_inc(${APP_ROOT}/src)
sdk_inc(${CLIGHT_ROOT}/platforms/hpm/boards/${BOARD_TYPE})

if(APP_ENABLE_LWIP_TCPECHO OR APP_ENABLE_LWIP_HTTP_SERVER)
    sdk_compile_definitions(-D__USE_RTL8211=1)
    sdk_compile_definitions(-D__ENABLE_ENET_RECEIVE_INTERRUPT=0)
    sdk_compile_definitions(-D__DISABLE_AUTO_NEGO=0)
    sdk_compile_definitions(-DUSE_LWIPOPTS_APP_H=1)
    sdk_compile_definitions(-DLWIP_SUPPORT_CUSTOM_PBUF=1)
    if(APP_ENABLE_LWIP_TCPECHO)
        sdk_compile_definitions(-DAPP_ENABLE_LWIP_TCPECHO=1)
    else()
        sdk_compile_definitions(-DAPP_ENABLE_LWIP_TCPECHO=0)
    endif()
    if(APP_ENABLE_LWIP_HTTP_SERVER)
        sdk_compile_definitions(-DAPP_ENABLE_LWIP_HTTP_SERVER=1)
    else()
        sdk_compile_definitions(-DAPP_ENABLE_LWIP_HTTP_SERVER=0)
    endif()
    foreach(LWIP_INCLUDE_DIR ${LWIP_INCLUDE_DIRS})
        sdk_inc(${LWIP_INCLUDE_DIR})
    endforeach()
else()
    sdk_compile_definitions(-DAPP_ENABLE_LWIP_TCPECHO=0)
    sdk_compile_definitions(-DAPP_ENABLE_LWIP_HTTP_SERVER=0)
endif()

if(APP_ENABLE_ETHERCAT_IO)
    sdk_compile_definitions(-DAPP_ENABLE_ETHERCAT_IO=1)
    sdk_compile_definitions(-DCONFIG_EEPROM_EMULATION=1)
    sdk_compile_definitions(-DESC_EEPROM_EMULATION=1)
    sdk_compile_definitions(-D_PIC24=0 -DCONTROLLER_16BIT=0 -DCONTROLLER_32BIT=1)
    sdk_compile_definitions(-DESC_16BIT_ACCESS=0 -DESC_32BIT_ACCESS=1 -DMBX_16BIT_ACCESS=0)
    sdk_compile_definitions(-DUSE_DEFAULT_MAIN=0)
    foreach(ECAT_INCLUDE_DIR ${ECAT_INCLUDE_DIRS})
        sdk_inc(${ECAT_INCLUDE_DIR})
    endforeach()
else()
    sdk_compile_definitions(-DAPP_ENABLE_ETHERCAT_IO=0)
endif()

if(APP_ENABLE_TINYUSB_CDC)
    sdk_compile_definitions(-DAPP_ENABLE_TINYUSB_CDC=1)
else()
    sdk_compile_definitions(-DAPP_ENABLE_TINYUSB_CDC=0)
endif()

if(APP_ENABLE_CLIGHT_SELFTEST)
    sdk_compile_definitions(-DAPP_ENABLE_CLIGHT_SELFTEST=1)
else()
    sdk_compile_definitions(-DAPP_ENABLE_CLIGHT_SELFTEST=0)
endif()

generate_ses_project()
