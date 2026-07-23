# SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# OpenRTX minimal bring-up for the Ailunce HD2 (HR_C7000 / CK803S), built on
# the vendor Miosix kernel consumed as a dependency.  This build tree lives in
# the OpenRTX target dir; the miosix repo carries no OpenRTX/app code.
#
# Scope: display + backlight + keyboard + RTC + battery + GPS + the UI.  No
# radio / DMR / audio / M17 / NVM -- those are stubbed (hd2_stubs.c) so the
# core + UI link.
cmake_minimum_required(VERSION 3.21)

# Kernel dependency.  miosix-kernel is normally a sibling of OpenRTX under
# vendor/ (../../../../miosix-kernel/miosix); that relative path is the default.
# Override it when the kernel lives elsewhere:
#   cmake -DMIOSIX_KPATH=/path/to/miosix-kernel/miosix ...
set(MIOSIX_KPATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../miosix-kernel/miosix"
    CACHE PATH "Path to the miosix kernel tree")
set(CMAKE_TOOLCHAIN_FILE ${MIOSIX_KPATH}/cmake/Toolchains/gcc-csky.cmake)
set(CMAKE_BUILD_TYPE "Release")

project(OpenRtxHd2Min C CXX ASM)

set(MIOSIX_BOARD hrc7000_hd2)
set(MIOSIX_USER_CONFIG_PATH ${MIOSIX_KPATH}/config)

add_subdirectory(${MIOSIX_KPATH} miosix EXCLUDE_FROM_ALL)

# OpenRTX repo root: this dir is <root>/platform/targets/HD2, so up 3.
set(ORTX ${CMAKE_SOURCE_DIR}/../../..)

# --- Hardware-agnostic OpenRTX core (boot spine + graphics + input) ---------
# Mirrors the proven HD2 core list.  Kept intact rather than trimmed further:
# the boot threshold is a ~130 KB image (heavily-stripped images don't boot on
# this SoC), and trimming these only multiplies the stub surface below.
set(ORTX_CORE
    ${ORTX}/openrtx/src/core/state.c
    ${ORTX}/openrtx/src/core/threads.c
    ${ORTX}/openrtx/src/core/battery.c
    ${ORTX}/openrtx/src/core/graphics.c
    ${ORTX}/openrtx/src/core/input.c
    ${ORTX}/openrtx/src/core/utils.c
    ${ORTX}/openrtx/src/core/queue.c
    ${ORTX}/openrtx/src/core/chan.c
    ${ORTX}/openrtx/src/core/gps.c
    ${ORTX}/openrtx/src/core/cps.c
    ${ORTX}/openrtx/src/core/crc.c
    ${ORTX}/openrtx/src/core/datetime.c
    ${ORTX}/openrtx/src/core/openrtx.c
    ${ORTX}/openrtx/src/core/data_conversion.c
    ${ORTX}/openrtx/src/core/nvmem_access.c
    ${ORTX}/openrtx/src/core/memory_profiling.cpp
    ${ORTX}/openrtx/src/core/audio_path.cpp
    ${ORTX}/openrtx/src/core/audio_stream.c
    ${ORTX}/openrtx/src/core/voicePromptUtils.c)
# Deliberately omitted (stubbed in hd2_stubs.c): voicePrompts.c + audio_codec.c
# + dsp.cpp (codec2's large static state trips the boot-hang the HD2 build
# avoids), and rtx.cpp + OpMode_* (radio stack).

# --- Default UI -------------------------------------------------------------
set(ORTX_UI
    ${ORTX}/openrtx/src/ui/default/ui.c
    ${ORTX}/openrtx/src/ui/default/ui_main.c
    ${ORTX}/openrtx/src/ui/default/ui_menu.c
    ${ORTX}/openrtx/src/ui/default/ui_strings.c)

# --- HD2 platform + the four minimal drivers --------------------------------
set(ORTX_HD2
    ${ORTX}/platform/targets/HD2/platform.c
    ${ORTX}/platform/targets/HD2/hwconfig.c
    ${ORTX}/platform/drivers/display/ST7735S_HD2.c
    ${ORTX}/platform/drivers/keyboard/keyboard_HD2.c
    ${ORTX}/platform/drivers/backlight/backlight_HD2.c
    ${ORTX}/platform/drivers/GPIO/gpio_hrc7000.c
    ${ORTX}/platform/mcu/HR_C7000/drivers/rtc_hd2.c
    ${ORTX}/platform/mcu/HR_C7000/drivers/i2c_hrc7000.c
    ${ORTX}/platform/drivers/ADC/adcHrc7000.c
    ${ORTX}/platform/mcu/HR_C7000/drivers/delays.cpp
    ${ORTX}/platform/drivers/GPS/gps_HD2.c
    ${ORTX}/platform/drivers/GPS/nmea_rbuf.c)

# --- Shared driver stubs for subsystems the minimal build leaves out --------
# Codeplug + settings NVM have generic OpenRTX stubs; use them rather than
# hand-rolled copies.  Radio / audio / stream stubs are NOT needed: the core
# never calls those wrappers in this build, so --gc-sections drops them.
set(ORTX_STUBS
    ${ORTX}/platform/drivers/stubs/cps_io_stub.c
    ${ORTX}/platform/drivers/stubs/nvmem_stub.c)

# --- Vendored helper libs the core pulls in ---------------------------------
set(ORTX_LIBS
    ${ORTX}/lib/minmea/minmea.c
    ${ORTX}/lib/QRCode/qrcode.c)

add_executable(openrtx_hd2
    ${ORTX}/openrtx/src/main.c
    ${CMAKE_SOURCE_DIR}/hd2_glue.cpp
    ${CMAKE_SOURCE_DIR}/hd2_core_stubs.c
    ${ORTX_CORE} ${ORTX_UI} ${ORTX_HD2} ${ORTX_STUBS} ${ORTX_LIBS})

target_include_directories(openrtx_hd2 PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${ORTX}/openrtx/include
    ${ORTX}/platform
    ${ORTX}/platform/targets/HD2
    ${ORTX}/platform/mcu/HR_C7000
    ${ORTX}/lib/minmea/include
    ${ORTX}/lib/QRCode/include
    ${ORTX}/lib/qdec/include)

target_compile_definitions(openrtx_hd2 PRIVATE
    PLATFORM_HD2 _MIOSIX FONT_UBUNTU_REGULAR timegm=mktime
    "GIT_VERSION=\"openrtx-hd2-min\"")

miosix_link_target(openrtx_hd2)
