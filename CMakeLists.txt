# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openrtx)

execute_process(COMMAND git describe --tags --dirty
                OUTPUT_VARIABLE GIT_VER_ID
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE)

target_compile_definitions(app
  PRIVATE
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
    openrtx/include/rtx
    openrtx/include/core
    openrtx/include/calibration
    openrtx/include/protocols
    openrtx/include/fonts/adafruit
    openrtx/include/fonts/symbols
    platform/drivers/ADC
    platform/drivers/NVM
    platform/drivers/GPS
    platform/drivers/USB
    platform/drivers/tones
    platform/drivers/baseband
    platform/drivers/backlight
    platform/drivers/chSelector

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
    openrtx/src/rtx/rtx.cpp
    openrtx/src/rtx/OpMode_FM.cpp
    openrtx/src/rtx/OpMode_M17.cpp
    openrtx/src/protocols/M17/M17DSP.cpp
    openrtx/src/protocols/M17/M17Golay.cpp
    openrtx/src/protocols/M17/M17Callsign.cpp
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
