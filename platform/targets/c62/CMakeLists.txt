# SPDX-License-Identifier: GPL-3.0-or-later

#
# CMake configuration file for the Retevis C62 OpenRTX target
#
cmake_minimum_required(VERSION 3.20.0)

zephyr_compile_definitions(PSRAM_CHIP_SIZE=4) # 4MBytes PSRAM!

target_sources(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/mcu/CSK6011B/drivers/delays.c

    ${OPENRTX_ROOT}/platform/drivers/stubs/nvmem_stub.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/cps_io_stub.c

    ${OPENRTX_ROOT}/platform/drivers/display/display_c62.c
    ${OPENRTX_ROOT}/platform/drivers/baseband/radio_C62.cpp
    ${OPENRTX_ROOT}/platform/drivers/keyboard/keyboard_c62.c

    ${OPENRTX_ROOT}/platform/targets/c62/platform.c
    ${OPENRTX_ROOT}/platform/targets/c62/audio_c62.c
    ${OPENRTX_ROOT}/platform/targets/c62/dsp.c
    ${OPENRTX_ROOT}/platform/targets/c62/lsf.c
)

add_subdirectory(services)
zephyr_library_link_libraries(lsf_client)
zephyr_linker_sources(DATA_SECTIONS lsf_service.ld)

target_include_directories(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/targets/c62
)

target_compile_definitions(app PRIVATE PLATFORM_C62)
