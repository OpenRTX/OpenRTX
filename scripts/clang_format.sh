#!/bin/bash -e

#  Copyright (C) 2021 by Alain Carlucci.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>

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
# TODO: This is temporarily running on a subset of the repo while we "ratchet up" the codebase; see https://github.com/OpenRTX/OpenRTX/issues/346
# Hey you! Have a new source file, or one that you've made changes to? Add it to the list below to enforce the new formatting.
FILE_LIST=$(cat <<-EOF
openrtx/include/core/audio_codec.h
openrtx/include/core/battery.h
openrtx/include/core/beeps.h
openrtx/include/core/crc.h
openrtx/include/core/data_conversion.h
openrtx/include/core/datatypes.h
openrtx/include/core/memory_profiling.h
openrtx/include/core/openrtx.h
openrtx/include/core/ui.h
openrtx/include/core/utils.h
openrtx/include/core/xmodem.h
openrtx/include/fonts/symbols/symbols.h
openrtx/include/interfaces/cps_io.h
openrtx/include/interfaces/delays.h
openrtx/include/interfaces/display.h
openrtx/include/interfaces/radio.h
openrtx/include/peripherals/gps.h
openrtx/include/peripherals/rng.h
openrtx/include/peripherals/rtc.h
openrtx/src/core/memory_profiling.cpp
platform/drivers/ADC/ADC0_GDx.h
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
EOF
)

CHECK_ARGS=""
if [ "$1" == "--check" ]; then
    CHECK_ARGS="--dry-run -Werror"
fi

clang-format $CHECK_ARGS -i $FILE_LIST
