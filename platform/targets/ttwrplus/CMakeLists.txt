# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

#
# CMake configuration file for the T-TWR Plus OpenRTX target
#
cmake_minimum_required(VERSION 3.20.0)

target_sources(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/mcu/ESP32S3/drivers/delays.c

    ${OPENRTX_ROOT}/platform/targets/ttwrplus/platform.c
    ${OPENRTX_ROOT}/platform/targets/ttwrplus/pmu.cpp

    ${OPENRTX_ROOT}/platform/drivers/display/SH1106_ttwrplus.c
    ${OPENRTX_ROOT}/platform/drivers/keyboard/keyboard_ttwrplus.c
    ${OPENRTX_ROOT}/platform/drivers/baseband/radio_ttwrplus.cpp
    ${OPENRTX_ROOT}/platform/drivers/baseband/AT1846S_SA8x8.cpp
    ${OPENRTX_ROOT}/platform/drivers/baseband/SA8x8.c
    ${OPENRTX_ROOT}/platform/drivers/GPS/gps_zephyr.c
    ${OPENRTX_ROOT}/platform/drivers/GPS/nmea_rbuf.c
    ${OPENRTX_ROOT}/platform/drivers/audio/audio_ttwrplus.c
    ${OPENRTX_ROOT}/platform/drivers/NVM/flash_zephyr.c
    ${OPENRTX_ROOT}/platform/drivers/NVM/nvmem_ttwrplus.c

    ${OPENRTX_ROOT}/platform/drivers/stubs/cps_io_stub.c

    ${OPENRTX_ROOT}/subprojects/XPowersLib/src/XPowersLibInterface.cpp
)

target_include_directories(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/targets/ttwrplus

    ${OPENRTX_ROOT}/subprojects/XPowersLib/src
    ${OPENRTX_ROOT}/subprojects/XPowersLib/src/REG
)

target_compile_definitions(app PRIVATE PLATFORM_TTWRPLUS CONFIG_SCREEN_BRIGHTNESS CONFIG_UI_NO_KEYBOARD)
