#!/bin/bash -e
#
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors

function show_help {
    echo "OpenRTX clang-format tool."
    echo ""
    echo "Usage: $0 [--check] [--help]"
    echo ""
    echo "To apply clang-format style to the whole codebase, run without arguments"
    echo "To check if the codebase is compliant, run with --check"
}

if [ $# -gt 1 ]; then
    show_help
    exit 1
fi

if [ $# -eq 1 ]; then
    if [ "$1" == "--help" ]; then
        show_help
        exit 0
    fi

    if [ "$1" != "--check" ]; then
        echo "Invalid argument $1"
        show_help
        exit 1
    fi
fi

# FILE_LIST=$(git ls-files | egrep '\.(c|cpp|h)$' | egrep -v 'lib/|subprojects/|platform/mcu')
# TODO: This is temporarily running on a subset of the repo while we "ratchet up"
# the codebase; see https://github.com/OpenRTX/OpenRTX/issues/346
#
# Hey you! Have a new source file, or one that you've made changes to? Add it to
# the list below to enforce the new formatting.
FILE_LIST=$(cat <<-EOF
openrtx/include/core/audio_codec.h
openrtx/include/core/battery.h
openrtx/include/core/beeps.h
openrtx/include/core/crc.h
openrtx/include/core/dsp.h
openrtx/include/core/data_conversion.h
openrtx/include/core/datatypes.h
openrtx/include/core/memory_profiling.h
openrtx/include/core/nvmem_access.h
openrtx/include/core/openrtx.h
openrtx/include/core/ui.h
openrtx/include/core/utils.h
openrtx/include/core/xmodem.h
openrtx/include/fonts/symbols/symbols.h
openrtx/include/interfaces/cps_io.h
openrtx/include/interfaces/delays.h
openrtx/include/interfaces/display.h
openrtx/include/interfaces/nvmem.h
openrtx/include/interfaces/radio.h
openrtx/include/peripherals/gps.h
openrtx/include/peripherals/rng.h
openrtx/include/peripherals/rtc.h
openrtx/include/protocols/M17/Callsign.hpp
openrtx/include/protocols/M17/M17FrameDecoder.hpp
openrtx/src/core/dsp.cpp
openrtx/src/core/nvmem_access.c
openrtx/src/core/memory_profiling.cpp
openrtx/src/protocols/M17/Callsign.cpp
openrtx/src/protocols/M17/M17FrameDecoder.cpp
platform/drivers/ADC/ADC0_GDx.h
platform/drivers/audio/file_source.h
platform/drivers/audio/file_source.c
platform/drivers/audio/MAX9814.h
platform/drivers/baseband/MCP4551.h
platform/drivers/baseband/SA8x8.h
platform/drivers/chSelector/chSelector.h
platform/drivers/display/SH110x_Mod17.h
platform/drivers/display/SSD1306_Mod17.h
platform/drivers/display/SSD1309_Mod17.h
platform/drivers/GPIO/gpio-native.h
platform/drivers/GPS/gps_stm32.h
platform/drivers/GPS/gps_zephyr.h
platform/drivers/USB/usb.h
platform/targets/GDx/hwconfig.h
platform/targets/linux/emulator/sdl_engine.h
platform/targets/ttwrplus/pmu.h
tests/platform/mic_test.c
tests/platform/codec2_encode_test.c
tests/unit/M17_callsign.cpp
EOF
)

CHECK_ARGS=""
if [ "$1" == "--check" ]; then
    CHECK_ARGS="--dry-run -Werror"
fi

clang-format $CHECK_ARGS -i $FILE_LIST
