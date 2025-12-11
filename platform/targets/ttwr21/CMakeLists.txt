# SPDX-License-Identifier: GPL-3.0-or-later

#
# CMake configuration file for the T-TWR Plus OpenRTX target
#
cmake_minimum_required(VERSION 3.20.0)

# Download sa8x8-fw from the web and package it as a header file
set(SA8X8FW "sa8x8-fw-sa868s-uhf")
file(DOWNLOAD
    https://github.com/OpenRTX/sa8x8-fw/releases/download/v1.4.1/sa8x8-fw-sa868s-uhf.s37
    ${CMAKE_CURRENT_LIST_DIR}/${SA8X8FW}.s37
    EXPECTED_HASH SHA512=0e046c745683cf204112feef97ab9cab8718d12b63fa2990a15ae054be28c26b18ae7a3f3518d0c81727426478e08580ce9a67e8f244d31f4401d7430a2e70e4
    SHOW_PROGRESS
)
execute_process(COMMAND objcopy -I srec -O binary --gap-fill 0xff --pad-to 0x4000 ${CMAKE_CURRENT_LIST_DIR}/${SA8X8FW}.s37 ${CMAKE_CURRENT_LIST_DIR}/${SA8X8FW}.bin)
execute_process(COMMAND awk -b -f ${OPENRTX_ROOT}/scripts/bin2c.awk ${CMAKE_CURRENT_LIST_DIR}/${SA8X8FW}.bin)

target_sources(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/mcu/ESP32S3/drivers/delays.c

    ${OPENRTX_ROOT}/platform/targets/ttwr21/platform.c

    ${OPENRTX_ROOT}/platform/drivers/GPS/gps_zephyr.c
    ${OPENRTX_ROOT}/platform/drivers/GPS/nmea_rbuf.c
    ${OPENRTX_ROOT}/platform/drivers/NVM/flash_zephyr.c
    ${OPENRTX_ROOT}/platform/drivers/NVM/nvmem_ttwr.c
    ${OPENRTX_ROOT}/platform/drivers/PMU/AXP2101_zephyr.cpp
    ${OPENRTX_ROOT}/platform/drivers/audio/audio_ttwr.c
    ${OPENRTX_ROOT}/platform/drivers/baseband/AT1846S_SA8x8.cpp
    ${OPENRTX_ROOT}/platform/drivers/baseband/SA8x8.c
    ${OPENRTX_ROOT}/platform/drivers/baseband/radio_ttwr.cpp
    ${OPENRTX_ROOT}/platform/drivers/display/SH1106_ttwr.c
    ${OPENRTX_ROOT}/platform/drivers/keyboard/keyboard_ttwr.c
    ${OPENRTX_ROOT}/platform/drivers/program/program_RL78.c
    ${OPENRTX_ROOT}/platform/drivers/ADC/adc_zephyr.c

    ${OPENRTX_ROOT}/platform/drivers/stubs/cps_io_stub.c

    ${OPENRTX_ROOT}/subprojects/XPowersLib/src/XPowersLibInterface.cpp
)

target_include_directories(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/targets/ttwr21

    ${OPENRTX_ROOT}/subprojects/XPowersLib/src
    ${OPENRTX_ROOT}/subprojects/XPowersLib/src/REG
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(app PRIVATE PLATFORM_TTWR CONFIG_SCREEN_BRIGHTNESS CONFIG_UI_NO_KEYBOARD)
