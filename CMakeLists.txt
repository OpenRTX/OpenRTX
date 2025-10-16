# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20.0)

#
# Zephyr build system imposes to have a fixed folder structure for the target-specific
# files, that is [...]/boards/ARCH/BOARDNAME because it infers the target
# architecture by looking at the name of the parent folder of the board folder.
#
# This does not make sense, because:
# 1) The folder with the target-specific files should be put anywhere, given that
#    the project using Zephyr may have its own conventions.
# 2) Inferring the target architecture only from the folder tree is an idiocy.
#
# So: to make Zephyr play happily in his sandpit without complaints, we create a
# board_root subfolder inside the build one and we copy the board-specific config
# files there from our platform/targets tree creating the necessary folder tree.
#
# NOTE: the board-specific CMakeLists.txt files have to use the ${OPENRTX_ROOT}
# variable as the base path. E.g. ${OPENRTX_ROOT}/platform/drivers/somedriver.c
#
if (${BOARD} STREQUAL "ttwrplus")
    set(ARCH xtensa)
    set(CONF_FILE ${CMAKE_CURRENT_LIST_DIR}/platform/mcu/ESP32S3/zephyr.conf)
endif()

#
# Create the board_root folder and the proper folder tree for the targets
#
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_LIST_DIR}/platform/targets/${BOARD}
                ${CMAKE_CURRENT_LIST_DIR}/build/board_root/boards/${ARCH}/${BOARD})

#
# Set the BOARD_ROOT and OPENRTX_ROOT variables
#
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR}/build/board_root)
set(OPENRTX_ROOT ${CMAKE_CURRENT_LIST_DIR})

#
# Finally, set up the project
#
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openrtx)

execute_process(COMMAND git describe --tags --dirty --always
                OUTPUT_VARIABLE GIT_VER_ID
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)

target_compile_definitions(app
  PRIVATE
    _GNU_SOURCE
    FONT_UBUNTU_REGULAR
    CODEC2_MODE_EN_DEFAULT=0
    FREEDV_MODE_EN_DEFAULT=0
    CODEC2_MODE_3200_EN=1
    M_PI=3.14159265358979323846f
    GIT_VERSION="${GIT_VER_ID}"
)

target_include_directories(app
  PRIVATE

    openrtx/include
    platform

    subprojects/codec2
    subprojects/codec2/src

    lib/minmea/include
    lib/qdec/include
)

target_sources(app
  PRIVATE

    openrtx/src/main.c
    openrtx/src/core/state.c
    openrtx/src/core/threads.c
    openrtx/src/core/battery.c
    openrtx/src/core/graphics.c
    openrtx/src/core/input.c
    openrtx/src/core/utils.c
    openrtx/src/core/queue.c
    openrtx/src/core/chan.c
    openrtx/src/core/gps.c
    openrtx/src/core/dsp.cpp
    openrtx/src/core/cps.c
    openrtx/src/core/crc.c
    openrtx/src/core/datetime.c
    openrtx/src/core/openrtx.c
    openrtx/src/core/audio_codec.c
    openrtx/src/core/audio_stream.c
    openrtx/src/core/audio_path.cpp
    openrtx/src/core/data_conversion.c
    openrtx/src/core/memory_profiling.cpp
    openrtx/src/core/voicePrompts.c
    openrtx/src/core/voicePromptUtils.c
    openrtx/src/core/voicePromptData.S
    openrtx/src/core/nvmem_access.c
    openrtx/src/rtx/rtx.cpp
    openrtx/src/rtx/OpMode_FM.cpp
    openrtx/src/rtx/OpMode_M17.cpp
    openrtx/src/protocols/M17/M17DSP.cpp
    openrtx/src/protocols/M17/M17Golay.cpp
    openrtx/src/protocols/M17/M17Callsign.cpp
    openrtx/src/protocols/M17/Callsign.cpp
    openrtx/src/protocols/M17/M17Modulator.cpp
    openrtx/src/protocols/M17/M17Demodulator.cpp
    openrtx/src/protocols/M17/M17FrameEncoder.cpp
    openrtx/src/protocols/M17/M17FrameDecoder.cpp
    openrtx/src/protocols/M17/M17LinkSetupFrame.cpp

    openrtx/src/ui/default/ui.c
    openrtx/src/ui/default/ui_main.c
    openrtx/src/ui/default/ui_menu.c
    openrtx/src/ui/default/ui_strings.c

    subprojects/codec2/src/dump.c
    subprojects/codec2/src/lpc.c
    subprojects/codec2/src/nlp.c
    subprojects/codec2/src/phase.c
    subprojects/codec2/src/quantise.c
    subprojects/codec2/src/postfilter.c
    subprojects/codec2/src/codec2.c
    subprojects/codec2/src/codec2_fft.c
    subprojects/codec2/src/lsp.c
    subprojects/codec2/src/sine.c
    subprojects/codec2/src/interp.c
    subprojects/codec2/src/kiss_fft.c
    subprojects/codec2/src/kiss_fftr.c
    subprojects/codec2/src/newamp1.c
    subprojects/codec2/src/codebook.c
    subprojects/codec2/src/codebookd.c
    subprojects/codec2/src/pack.c
    subprojects/codec2/src/codebooknewamp1.c
    subprojects/codec2/src/codebooknewamp1_energy.c

    lib/minmea/minmea.c
)
