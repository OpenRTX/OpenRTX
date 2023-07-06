# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(openrtx)

target_compile_definitions(app
  PRIVATE
    PLATFORM_ZEPHYR
    GIT_VERSION="zephyr"
    FONT_UBUNTU_REGULAR
    SCREEN_WIDTH=160
    SCREEN_HEIGHT=128
    PIX_FMT_RGB565=
    VP_USE_FILESYSTEM
    CODEC2_MODE_EN_DEFAULT=0
    CODEC2_MODE_3200_EN=1
    CODEC2_MODE_1600_EN=1
    FREEDV_MODE_EN_DEFAULT=0
    M_PI=3.14159265358979323846f
)

target_include_directories(app
  PRIVATE
    # These are needed only for linux builds
    #/usr/include/codec2
    #/usr/include/readline

    ../../subprojects/codec2
    ../../subprojects/codec2/src

    ../../platform/targets/linux
    ../../platform/targets/linux/emulator

    ../../openrtx/include
    ../../openrtx/include/rtx
    ../../openrtx/include/core
    ../../openrtx/include/calibration
    ../../openrtx/include/protocols
    ../../openrtx/include/fonts/adafruit

    ../../lib/minmea/include

    ../../lib/qdec/include
)

target_sources(app
  PRIVATE
# These are needed only for linux builds
#    ../../platform/targets/linux/emulator/emulator.c
#    ../../platform/targets/linux/emulator/sdl_engine.c
#    ../../platform/drivers/display/display_libSDL.c
#    ../../platform/drivers/keyboard/keyboard_linux.c
#    ../../platform/drivers/NVM/nvmem_linux.c
#    ../../platform/drivers/baseband/radio_linux.cpp
#    ../../platform/drivers/audio/audio_linux.c
#    ../../platform/drivers/audio/inputStream_linux.cpp
#    ../../platform/drivers/audio/outputStream_linux.c
#    ../../platform/targets/linux/platform.c

    ../../platform/drivers/CPS/cps_io_libc.c
    ../../platform/drivers/GPS/GPS_linux.c
    ../../platform/drivers/NVM/nvmem_Zephyr.c
    ../../platform/drivers/audio/audio_zephyr.c
    ../../platform/drivers/audio/inputStream_Zephyr.cpp
    ../../platform/drivers/audio/outputStream_Zephyr.c
    ../../platform/drivers/baseband/radio_TTWR.cpp
    ../../platform/drivers/display/SH110x_TTWR.c
    ../../platform/drivers/keyboard/keyboard_Zephyr.c
    ../../platform/mcu/ESP32/drivers/delays.c
    ../../platform/mcu/ESP32/drivers/gpio.c
    ../../platform/mcu/ESP32/drivers/rtc.c
    ../../platform/targets/Zephyr/platform.c

    ../../openrtx/src/main.c
    ../../openrtx/src/core/state.c
    ../../openrtx/src/core/threads.c
    ../../openrtx/src/core/battery.c
    ../../openrtx/src/core/graphics.c
    ../../openrtx/src/core/input.c
    ../../openrtx/src/core/utils.c
    ../../openrtx/src/core/queue.c
    ../../openrtx/src/core/chan.c
    ../../openrtx/src/core/gps.c
    ../../openrtx/src/core/dsp.cpp
    ../../openrtx/src/core/cps.c
    ../../openrtx/src/core/crc.c
    ../../openrtx/src/core/datetime.c
    ../../openrtx/src/core/openrtx.c
    ../../openrtx/src/core/audio_codec.c
    ../../openrtx/src/core/audio_path.cpp
    ../../openrtx/src/core/data_conversion.c
    ../../openrtx/src/core/memory_profiling.cpp
    ../../openrtx/src/core/voicePrompts.c
    ../../openrtx/src/core/voicePromptUtils.c
    ../../openrtx/src/core/voicePromptData.S
    ../../openrtx/src/rtx/rtx.cpp
    ../../openrtx/src/rtx/OpMode_FM.cpp
    ../../openrtx/src/rtx/OpMode_M17.cpp
    ../../openrtx/src/protocols/M17/M17DSP.cpp
    ../../openrtx/src/protocols/M17/M17Golay.cpp
    ../../openrtx/src/protocols/M17/M17Callsign.cpp
    ../../openrtx/src/protocols/M17/M17Modulator.cpp
    ../../openrtx/src/protocols/M17/M17Demodulator.cpp
    ../../openrtx/src/protocols/M17/M17FrameEncoder.cpp
    ../../openrtx/src/protocols/M17/M17FrameDecoder.cpp
    ../../openrtx/src/protocols/M17/M17LinkSetupFrame.cpp
    ../../openrtx/src/ui/default/ui.c
    ../../openrtx/src/ui/default/ui_main.c
    ../../openrtx/src/ui/default/ui_menu.c
    ../../openrtx/src/ui/default/ui_strings.c

    ../../lib/minmea/minmea.c

    ../../subprojects/codec2/src/dump.c
    ../../subprojects/codec2/src/lpc.c
    ../../subprojects/codec2/src/nlp.c
    ../../subprojects/codec2/src/phase.c
    ../../subprojects/codec2/src/quantise.c
    ../../subprojects/codec2/src/postfilter.c
    ../../subprojects/codec2/src/codec2.c
    ../../subprojects/codec2/src/codec2_fft.c
    ../../subprojects/codec2/src/lsp.c
    ../../subprojects/codec2/src/sine.c
    ../../subprojects/codec2/src/interp.c
    ../../subprojects/codec2/src/kiss_fft.c
    ../../subprojects/codec2/src/kiss_fftr.c
    ../../subprojects/codec2/src/newamp1.c
    ../../subprojects/codec2/src/codebook.c
    ../../subprojects/codec2/src/codebookd.c
    ../../subprojects/codec2/src/pack.c
    ../../subprojects/codec2/src/codebooknewamp1.c
    ../../subprojects/codec2/src/codebooknewamp1_energy.c
)

# These are needed only for linux builds
#target_link_libraries(app PRIVATE /usr/lib/libreadline.so)
#target_link_libraries(app PRIVATE /usr/lib/libcodec2.so)
#target_link_libraries(app PRIVATE /usr/lib/libpulse.so)
#target_link_libraries(app PRIVATE /usr/lib/libpulse-simple.so)
#target_link_libraries(app PRIVATE /usr/lib/libSDL2.so)
