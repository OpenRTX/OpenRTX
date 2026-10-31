# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

#
# CMake configuration file for the Retevis C62 OpenRTX target
#
cmake_minimum_required(VERSION 3.20.0)

zephyr_compile_definitions(PSRAM_CHIP_SIZE=4) # 4MBytes PSRAM!

target_sources(app
  PRIVATE
    ${OPENRTX_ROOT}/platform/mcu/CSK6011B/drivers/delays.c

    ${OPENRTX_ROOT}/platform/drivers/GPIO/gpio_zephyr.c
    ${OPENRTX_ROOT}/platform/drivers/GPIO/gpio_bk4819.c
    ${OPENRTX_ROOT}/platform/drivers/SPI/spi_zephyr.c
    ${OPENRTX_ROOT}/platform/drivers/SPI/spi_zephyr_bus.c
    ${OPENRTX_ROOT}/platform/drivers/I2C/i2c_bitbang.c

    ${OPENRTX_ROOT}/platform/drivers/baseband/BK4819.c
    ${OPENRTX_ROOT}/platform/drivers/baseband/BK1080.c

    ${OPENRTX_ROOT}/platform/drivers/NVM/nvmem_C62.c
    ${OPENRTX_ROOT}/platform/targets/c62/calib_C62.c
    ${OPENRTX_ROOT}/platform/drivers/stubs/cps_io_stub.c

    ${OPENRTX_ROOT}/platform/drivers/display/display_c62.c
    ${OPENRTX_ROOT}/platform/drivers/baseband/radio_C62.cpp
    ${OPENRTX_ROOT}/platform/drivers/keyboard/keyboard_c62.c
    ${OPENRTX_ROOT}/platform/drivers/audio/audio_c62.c
    ${OPENRTX_ROOT}/platform/drivers/audio/csk6_codec.c

    ${OPENRTX_ROOT}/platform/targets/c62/platform.c
    ${OPENRTX_ROOT}/platform/targets/c62/hwconfig.c
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

#
# DSP firmware, flashed separately at 0x100000 (dsp_firmware partition). The
# firmware does not boot without it, and it must match the audio code, so the
# build pins one image by hash: by default it is downloaded from OpenRTX's
# lsf-zephyr-sdk, or C62_DSP_FIRMWARE (CMake or environment variable) gives a
# local copy for offline builds. Either way it ends up next to zephyr.bin.
#
set(C62_DSP_FIRMWARE_URL
    "https://raw.githubusercontent.com/OpenRTX/lsf-zephyr-sdk/2ac605527dcde3e90c814bb4175546cc7fbd36da/resources/dsp_firmware.bin")
set(C62_DSP_FIRMWARE_SHA256
    "9579b617f048c646465118f6d0b9e1b765b4d7d811ff9c8649c5754fe45a8903")
set(C62_DSP_FIRMWARE_OUT ${CMAKE_BINARY_DIR}/zephyr/dsp_firmware.bin)

if(NOT DEFINED C62_DSP_FIRMWARE AND DEFINED ENV{C62_DSP_FIRMWARE})
  set(C62_DSP_FIRMWARE $ENV{C62_DSP_FIRMWARE})
endif()

# A relative path is from the OpenRTX root. Check the hash here, as a
# mismatch in file(DOWNLOAD) stops with CMake's own error.
if(C62_DSP_FIRMWARE)
  get_filename_component(c62_dsp_local "${C62_DSP_FIRMWARE}" ABSOLUTE
                         BASE_DIR "${OPENRTX_ROOT}")
  if(EXISTS "${c62_dsp_local}")
    file(SHA256 "${c62_dsp_local}" c62_dsp_local_sha256)
  endif()
  if(NOT "${c62_dsp_local_sha256}" STREQUAL "${C62_DSP_FIRMWARE_SHA256}")
    message(FATAL_ERROR "C62_DSP_FIRMWARE ${c62_dsp_local} is missing or is "
                        "not the pinned DSP firmware (sha256 "
                        "${C62_DSP_FIRMWARE_SHA256}).")
  endif()
  configure_file("${c62_dsp_local}" "${C62_DSP_FIRMWARE_OUT}" COPYONLY)
else()
  # Skipped when the file is already there with the expected hash
  file(DOWNLOAD "${C62_DSP_FIRMWARE_URL}" "${C62_DSP_FIRMWARE_OUT}"
       EXPECTED_HASH SHA256=${C62_DSP_FIRMWARE_SHA256}
       TLS_VERIFY ON
       STATUS c62_dsp_status)
  list(GET c62_dsp_status 0 c62_dsp_error)
  if(c62_dsp_error)
    list(GET c62_dsp_status 1 c62_dsp_message)
    message(FATAL_ERROR "Could not get the C62 DSP firmware from "
                        "${C62_DSP_FIRMWARE_URL} (${c62_dsp_message}). Set "
                        "C62_DSP_FIRMWARE to a local copy with sha256 "
                        "${C62_DSP_FIRMWARE_SHA256}.")
  endif()
endif()

message(STATUS "C62 DSP firmware: ${C62_DSP_FIRMWARE_OUT}, flash at 0x100000")
