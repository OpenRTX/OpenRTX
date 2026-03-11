# SPDX-License-Identifier: GPL-3.0-or-later

#
# CMake configuration file for the Retevis C62 OpenRTX target
#
cmake_minimum_required(VERSION 3.20.0)


target_sources(app
  PRIVATE


    ${OPENRTX_ROOT}/platform/mcu/CSK6011B/drivers/delays.c

    ${OPENRTX_ROOT}/platform/targets/c62/platform.c

    # ${OPENRTX_ROOT}/platform/drivers/baseband/bk4819.c // TODO: reference to gpio-native.h
    
    # ${OPENRTX_ROOT}/platform/drivers/NVM/flash_zephyr.c
    
    ${OPENRTX_ROOT}/platform/drivers/stubs/cps_io_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/radio_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/display_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/nvmem_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/keyboard_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/audio_stub.c



)

target_include_directories(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/targets/c62
)

target_compile_definitions(app PRIVATE PLATFORM_C62 CONFIG_SCREEN_BRIGHTNESS)
