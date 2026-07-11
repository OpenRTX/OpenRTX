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

# TODO: This is temporarily running on a subset of the repo while we "ratchet up"
# the codebase; see https://github.com/OpenRTX/OpenRTX/issues/346
#
# Hey you! Did you update the coding style on one of the files below? Remove it
# from the list to enforce the new formatting on new commits.
EXCLUDE_FILES=$(cat <<-EOF
openrtx/include/calibration/calibInfo_CS7000.h
openrtx/include/calibration/calibInfo_GDx.h
openrtx/include/calibration/calibInfo_MDx.h
openrtx/include/calibration/calibInfo_Mod17.h
openrtx/include/core/audio_path.h
openrtx/include/core/audio_stream.h
openrtx/include/core/backup.h
openrtx/include/core/chan.h
openrtx/include/core/cps.h
openrtx/include/core/ctcssDetector.hpp
openrtx/include/core/datetime.h
openrtx/include/core/event.h
openrtx/include/core/fir.hpp
openrtx/include/core/goertzel.hpp
openrtx/include/core/gps.h
openrtx/include/core/graphics.h
openrtx/include/core/iir.hpp
openrtx/include/core/queue.h
openrtx/include/core/ringbuf.hpp
openrtx/include/core/settings.h
openrtx/include/core/threads.h
openrtx/include/interfaces/audio.h
openrtx/include/interfaces/keyboard.h
openrtx/include/interfaces/platform.h
openrtx/include/peripherals/adc.h
openrtx/include/peripherals/gpio.h
openrtx/include/peripherals/i2c.h
openrtx/include/peripherals/spi.h
openrtx/include/protocols/M17/ClockRecovery.hpp
openrtx/include/protocols/M17/CodePuncturing.hpp
openrtx/include/protocols/M17/Constants.hpp
openrtx/include/protocols/M17/ConvolutionalEncoder.hpp
openrtx/include/protocols/M17/Correlator.hpp
openrtx/include/protocols/M17/DSP.hpp
openrtx/include/protocols/M17/Datatypes.hpp
openrtx/include/protocols/M17/Decorrelator.hpp
openrtx/include/protocols/M17/Demodulator.hpp
openrtx/include/protocols/M17/DevEstimator.hpp
openrtx/include/protocols/M17/FrameEncoder.hpp
openrtx/include/protocols/M17/Golay.hpp
openrtx/include/protocols/M17/Interleaver.hpp
openrtx/include/protocols/M17/LinkSetupFrame.hpp
openrtx/include/protocols/M17/Modulator.hpp
openrtx/include/protocols/M17/Prbs.hpp
openrtx/include/protocols/M17/PwmCompensator.hpp
openrtx/include/protocols/M17/StreamFrame.hpp
openrtx/include/protocols/M17/Synchronizer.hpp
openrtx/include/protocols/M17/Utils.hpp
openrtx/include/protocols/M17/Viterbi.hpp
openrtx/include/rtx/OpMode.hpp
openrtx/include/rtx/OpMode_FM.hpp
openrtx/include/rtx/OpMode_M17.hpp
openrtx/include/rtx/rtx.h
openrtx/include/ui/EnglishStrings.h
openrtx/include/ui/SpanishStrings.h
openrtx/include/ui/ui_default.h
openrtx/include/ui/ui_mod17.h
openrtx/include/ui/ui_strings.h
openrtx/src/core/audio_path.cpp
openrtx/src/core/audio_stream.c
openrtx/src/core/backup.c
openrtx/src/core/battery.c
openrtx/src/core/chan.c
openrtx/src/core/cps.c
openrtx/src/core/data_conversion.c
openrtx/src/core/datetime.c
openrtx/src/core/gps.c
openrtx/src/core/graphics.c
openrtx/src/core/openrtx.c
openrtx/src/core/queue.c
openrtx/src/core/threads.c
openrtx/src/core/utils.c
openrtx/src/core/xmodem.c
openrtx/src/main.c
openrtx/src/protocols/M17/DSP.cpp
openrtx/src/protocols/M17/Demodulator.cpp
openrtx/src/protocols/M17/FrameEncoder.cpp
openrtx/src/protocols/M17/Golay.cpp
openrtx/src/protocols/M17/LinkSetupFrame.cpp
openrtx/src/protocols/M17/MetaText.cpp
openrtx/src/protocols/M17/Modulator.cpp
openrtx/src/rtx/OpMode_FM.cpp
openrtx/src/rtx/OpMode_M17.cpp
openrtx/src/rtx/rtx.cpp
openrtx/src/ui/default/ui.c
openrtx/src/ui/default/ui_main.c
openrtx/src/ui/default/ui_menu.c
openrtx/src/ui/default/ui_strings.c
openrtx/src/ui/module17/ui.c
openrtx/src/ui/module17/ui_main.c
openrtx/src/ui/module17/ui_menu.c
platform/drivers/ADC/ADC0_GDx.c
platform/drivers/ADC/adc_at32.c
platform/drivers/ADC/adc_at32.h
platform/drivers/ADC/adc_stm32.h
platform/drivers/ADC/adc_stm32f4.c
platform/drivers/ADC/adc_stm32h7.c
platform/drivers/CPS/cps_data_GDx.h
platform/drivers/CPS/cps_data_MD3x0.h
platform/drivers/CPS/cps_data_MDUV3x0.h
platform/drivers/CPS/cps_io_libc.c
platform/drivers/CPS/cps_io_native_GDx.c
platform/drivers/CPS/cps_io_native_MD3x0.c
platform/drivers/CPS/cps_io_native_MD9600.c
platform/drivers/CPS/cps_io_native_MDUV3x0.c
platform/drivers/CPS/cps_io_native_Mod17.c
platform/drivers/GPIO/gpio_at32f423.c
platform/drivers/GPIO/gpio_at32f423.h
platform/drivers/GPIO/gpio_mk22.c
platform/drivers/GPIO/gpio_mk22.h
platform/drivers/GPIO/gpio_shiftReg.c
platform/drivers/GPIO/gpio_shiftReg.h
platform/drivers/GPIO/gpio_stm32.c
platform/drivers/GPIO/gpio_stm32.h
platform/drivers/GPS/gps_linux.c
platform/drivers/GPS/gps_stm32.cpp
platform/drivers/GPS/gps_zephyr.c
platform/drivers/GPS/nmea_rbuf.c
platform/drivers/GPS/nmea_rbuf.h
platform/drivers/NVM/AT24Cx.h
platform/drivers/NVM/AT24Cx_GDx.c
platform/drivers/NVM/W25Qx.c
platform/drivers/NVM/W25Qx.h
platform/drivers/NVM/eeep.c
platform/drivers/NVM/eeep.h
platform/drivers/NVM/flash_zephyr.c
platform/drivers/NVM/flash_zephyr.h
platform/drivers/NVM/nvmem_CS7000.c
platform/drivers/NVM/nvmem_GDx.c
platform/drivers/NVM/nvmem_MDx.c
platform/drivers/NVM/nvmem_Mod17.c
platform/drivers/NVM/nvmem_linux.c
platform/drivers/NVM/nvmem_settings_MDx.c
platform/drivers/NVM/nvmem_ttwrplus.c
platform/drivers/SPI/spi_bitbang.c
platform/drivers/SPI/spi_bitbang.h
platform/drivers/SPI/spi_custom.c
platform/drivers/SPI/spi_custom.h
platform/drivers/SPI/spi_mk22.c
platform/drivers/SPI/spi_mk22.h
platform/drivers/SPI/spi_stm32.h
platform/drivers/SPI/spi_stm32f4.c
platform/drivers/SPI/spi_stm32h7.c
platform/drivers/USB/usb_MDx.cpp
platform/drivers/USB/usb_descriptors.c
platform/drivers/audio/Cx000_dac.cpp
platform/drivers/audio/Cx000_dac.h
platform/drivers/audio/MAX9814_Mod17.cpp
platform/drivers/audio/audio_CS7000.cpp
platform/drivers/audio/audio_GDx.c
platform/drivers/audio/audio_MDx.cpp
platform/drivers/audio/audio_Mod17.c
platform/drivers/audio/audio_linux.c
platform/drivers/audio/audio_ttwrplus.c
platform/drivers/audio/stm32_adc.cpp
platform/drivers/audio/stm32_adc.h
platform/drivers/audio/stm32_dac.cpp
platform/drivers/audio/stm32_dac.h
platform/drivers/audio/stm32_pwm.cpp
platform/drivers/audio/stm32_pwm.h
platform/drivers/backlight/backlight.h
platform/drivers/backlight/backlight_CS7000.c
platform/drivers/backlight/backlight_GDx.c
platform/drivers/backlight/backlight_MDx.c
platform/drivers/baseband/AK2365A.c
platform/drivers/baseband/AK2365A.h
platform/drivers/baseband/AT1846S.h
platform/drivers/baseband/AT1846S_GDx.cpp
platform/drivers/baseband/AT1846S_SA8x8.cpp
platform/drivers/baseband/AT1846S_UV3x0.cpp
platform/drivers/baseband/HR_C5000.h
platform/drivers/baseband/HR_C5000_MDx.cpp
platform/drivers/baseband/HR_C6000.cpp
platform/drivers/baseband/HR_C6000.h
platform/drivers/baseband/HR_C6000_CS7000.cpp
platform/drivers/baseband/HR_C6000_GDx.cpp
platform/drivers/baseband/HR_C6000_UV3x0.cpp
platform/drivers/baseband/HR_Cx000.cpp
platform/drivers/baseband/HR_Cx000.h
platform/drivers/baseband/MCP4551.c
platform/drivers/baseband/SA8x8.c
platform/drivers/baseband/radioUtils.h
platform/drivers/baseband/radio_CS7000.cpp
platform/drivers/baseband/radio_GDx.cpp
platform/drivers/baseband/radio_MD3x0.cpp
platform/drivers/baseband/radio_MD9600.cpp
platform/drivers/baseband/radio_Mod17.cpp
platform/drivers/baseband/radio_UV3x0.cpp
platform/drivers/baseband/radio_linux.cpp
platform/drivers/baseband/radio_ttwrplus.cpp
platform/drivers/chSelector/chSelector_MD9600.c
platform/drivers/chSelector/chSelector_UV3x0.c
platform/drivers/display/HX8353_MD3x.cpp
platform/drivers/display/SH1106_ttwrplus.c
platform/drivers/display/SH110x_Mod17.c
platform/drivers/display/SSD1306_Mod17.c
platform/drivers/display/SSD1309_Mod17.c
platform/drivers/display/ST7567_MD9600.c
platform/drivers/display/ST7735R_CS7000.c
platform/drivers/display/UC1701_GDx.c
platform/drivers/display/display_Mod17.c
platform/drivers/display/display_libSDL.c
platform/drivers/keyboard/cap1206.c
platform/drivers/keyboard/cap1206.h
platform/drivers/keyboard/cap1206_regs.h
platform/drivers/keyboard/keyboard_CS7000.c
platform/drivers/keyboard/keyboard_DM1701.c
platform/drivers/keyboard/keyboard_GDx.c
platform/drivers/keyboard/keyboard_MD3x.c
platform/drivers/keyboard/keyboard_MD9600.c
platform/drivers/keyboard/keyboard_Mod17.c
platform/drivers/keyboard/keyboard_linux.c
platform/drivers/keyboard/keyboard_ttwrplus.c
platform/drivers/stubs/audio_stub.c
platform/drivers/stubs/cps_io_stub.c
platform/drivers/stubs/display_stub.c
platform/drivers/stubs/inputStream_stub.c
platform/drivers/stubs/keyboard_stub.c
platform/drivers/stubs/nvmem_stub.c
platform/drivers/stubs/outputStream_stub.c
platform/drivers/stubs/radio_stub.c
platform/drivers/tones/toneGenerator_MDx.cpp
platform/drivers/tones/toneGenerator_MDx.h
platform/mcu/AT32F423/boot/arch_registers_impl.h
platform/mcu/AT32F423/boot/bsp.cpp
platform/mcu/AT32F423/boot/libc_integration.cpp
platform/mcu/AT32F423/boot/startup.cpp
platform/mcu/AT32F423/drivers/USART6.cpp
platform/mcu/AT32F423/drivers/USART6.h
platform/mcu/AT32F423/drivers/delays.cpp
platform/mcu/ESP32S3/drivers/delays.c
platform/mcu/MK22FN512xxx12/boot/arch_registers_impl.h
platform/mcu/MK22FN512xxx12/boot/bsp.cpp
platform/mcu/MK22FN512xxx12/boot/libc_integration.cpp
platform/mcu/MK22FN512xxx12/boot/startup.cpp
platform/mcu/MK22FN512xxx12/drivers/I2C0.c
platform/mcu/MK22FN512xxx12/drivers/I2C0.h
platform/mcu/MK22FN512xxx12/drivers/delays.cpp
platform/mcu/MK22FN512xxx12/drivers/rng.c
platform/mcu/STM32F4xx/boot/arch_registers_impl.h
platform/mcu/STM32F4xx/boot/bsp.cpp
platform/mcu/STM32F4xx/boot/libc_integration.cpp
platform/mcu/STM32F4xx/boot/startup.cpp
platform/mcu/STM32F4xx/drivers/DmaStream.hpp
platform/mcu/STM32F4xx/drivers/Timer.hpp
platform/mcu/STM32F4xx/drivers/USART3.cpp
platform/mcu/STM32F4xx/drivers/USART3.h
platform/mcu/STM32F4xx/drivers/delays.cpp
platform/mcu/STM32F4xx/drivers/flash.c
platform/mcu/STM32F4xx/drivers/flash.h
platform/mcu/STM32F4xx/drivers/rcc.c
platform/mcu/STM32F4xx/drivers/rcc.h
platform/mcu/STM32F4xx/drivers/rng.c
platform/mcu/STM32F4xx/drivers/rtc.c
platform/mcu/STM32F4xx/drivers/timers.h
platform/mcu/STM32H7xx/boot/arch_registers_impl.h
platform/mcu/STM32H7xx/boot/bsp.cpp
platform/mcu/STM32H7xx/boot/libc_integration.cpp
platform/mcu/STM32H7xx/boot/startup.cpp
platform/mcu/STM32H7xx/drivers/DmaStream.hpp
platform/mcu/STM32H7xx/drivers/Lptim.hpp
platform/mcu/STM32H7xx/drivers/delays.cpp
platform/mcu/STM32H7xx/drivers/rcc.cpp
platform/mcu/STM32H7xx/drivers/rcc.h
platform/mcu/x86_64/drivers/delays.c
platform/mcu/x86_64/drivers/rng.cpp
platform/targets/CS7000-PLUS/hwconfig.c
platform/targets/CS7000-PLUS/pinmap.h
platform/targets/CS7000-PLUS/platform.c
platform/targets/CS7000/hwconfig.c
platform/targets/CS7000/pinmap.h
platform/targets/CS7000/platform.c
platform/targets/DM-1701/hwconfig.c
platform/targets/DM-1701/pinmap.h
platform/targets/DM-1701/platform.c
platform/targets/GDx/hwconfig.c
platform/targets/GDx/pinmap_DM1801.h
platform/targets/GDx/pinmap_GD77.h
platform/targets/GDx/platform.c
platform/targets/MD-3x0/hwconfig.c
platform/targets/MD-3x0/pinmap.h
platform/targets/MD-3x0/platform.c
platform/targets/MD-9600/hwconfig.c
platform/targets/MD-9600/pinmap.h
platform/targets/MD-9600/platform.c
platform/targets/MD-UV3x0/hwconfig.c
platform/targets/MD-UV3x0/pinmap.h
platform/targets/MD-UV3x0/platform.c
platform/targets/Module17/pinmap.h
platform/targets/Module17/platform.c
platform/targets/linux/emulator/emulator.c
platform/targets/linux/emulator/emulator.h
platform/targets/linux/emulator/sdl_engine.c
platform/targets/linux/platform.c
platform/targets/ttwrplus/platform.c
platform/targets/ttwrplus/pmu.cpp
tests/platform/boot_test.c
tests/platform/calib_read.c
tests/platform/display_test.c
tests/platform/gimmi_ridimmi.c
tests/platform/gpio_demo.c
tests/platform/keyboard_test.c
tests/platform/nvm_dump.c
tests/platform/oversample_test.c
tests/platform/platform_test.c
tests/platform/stm32f405_flash.c
tests/platform/stm32h743_flash.c
tests/platform/tonegen.c
EOF
)

EXCLUDE_DIRS=$(cat <<-EOF
lib
platform/mcu/CMSIS
platform/mcu/MK22FN512xxx12/drivers/usb
platform/mcu/STM32F4xx/drivers/usb
openrtx/include/fonts
scripts
subprojects
EOF
)

EXCLUDE_REGEX=$(printf '%s\n' "$EXCLUDE_DIRS" | paste -sd'|' -)
FILE_LIST=$(
    git ls-files \
    | grep -E '\.(c|cpp|h|hpp)$' \
    | grep -Ev $(printf '^(%s)' "$EXCLUDE_REGEX") \
    | grep -vxF -f <(printf '%s\n' "$EXCLUDE_FILES")
)

CHECK_ARGS=""
if [ "$1" == "--check" ]; then
    CHECK_ARGS="--dry-run -Werror"
fi

clang-format $CHECK_ARGS -i $FILE_LIST
