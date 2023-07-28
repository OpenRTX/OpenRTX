# SPDX-License-Identifier: GPL-3.0-or-later

#
# CMake configuration file for the T-TWR Plus OpenRTX target
#
cmake_minimum_required(VERSION 3.20.0)

target_sources(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/mcu/ESP32S3/drivers/delays.c

    ${OPENRTX_ROOT}/platform/targets/ttwrplus/platform.c

    ${OPENRTX_ROOT}/platform/drivers/display/SH1106_ttwrplus.c

    ${OPENRTX_ROOT}/platform/drivers/stubs/audio_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/cps_io_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/keyboard_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/nvmem_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/radio_stub.c
)

target_include_directories(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/targets/ttwrplus
)

target_compile_definitions(app PRIVATE PLATFORM_TTWRPLUS)
