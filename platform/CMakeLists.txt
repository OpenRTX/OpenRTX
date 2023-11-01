if(OPENRTX_TARGET STREQUAL "md3x0")
set(mdx_srcs
    # MDx source
    drivers/ADC/ADC1_MDx.c
    drivers/GPS/GPS_MDx.cpp
    drivers/NVM/W25Qx.c
    drivers/NVM/nvmem_settings_MDx.c
    drivers/audio/audio_MDx.c
    drivers/baseband/HR_Cx000.cpp
    drivers/tones/toneGenerator_MDx.cpp

    # MD3x0 sources
    drivers/NVM/nvmem_MD3x0.c
    drivers/CPS/cps_io_native_MD3x0.c
    drivers/NVM/spiFlash_MD3x.c
    drivers/baseband/SKY72310.c
    drivers/baseband/radio_MD3x0.cpp
    drivers/baseband/HR_C5000_MDx.cpp
    drivers/keyboard/keyboard_MD3x.c
    drivers/display/HX8353_MD3x.cpp
    drivers/backlight/backlight_MDx.c
    targets/MD-3x0/platform.c
)

set(stm32_srcs
    # STM32
    mcu/STM32F4xx/boot/startup.cpp
    mcu/STM32F4xx/boot/bsp.cpp
    mcu/STM32F4xx/boot/libc_integration.cpp
    mcu/STM32F4xx/drivers/usb/usb_bsp.c
    mcu/STM32F4xx/drivers/usb/usb_core.c
    mcu/STM32F4xx/drivers/usb/usb_dcd.c
    mcu/STM32F4xx/drivers/usb/usb_dcd_int.c
    mcu/STM32F4xx/drivers/usb/usbd_desc.c
    mcu/STM32F4xx/drivers/usb/usbd_core.c
    mcu/STM32F4xx/drivers/usb/usbd_ioreq.c
    mcu/STM32F4xx/drivers/usb/usbd_req.c
    mcu/STM32F4xx/drivers/usb/usbd_usr.c
    mcu/STM32F4xx/drivers/gpio.c
    mcu/STM32F4xx/drivers/usb_vcom.c
    mcu/STM32F4xx/drivers/delays.cpp
    mcu/STM32F4xx/drivers/rtc.c
    mcu/STM32F4xx/drivers/SPI2.c
    mcu/STM32F4xx/drivers/USART3.cpp
    mcu/STM32F4xx/drivers/flash.c
    drivers/audio/stm32_dac.cpp
    drivers/audio/stm32_adc.cpp
    drivers/audio/stm32_pwm.cpp
    mcu/CMSIS/Device/ST/STM32F4xx/Source/system_stm32f4xx.c
)

add_library(Platform STATIC
    ${mdx_srcs}
    ${stm32_srcs}
)

target_include_directories(Platform PUBLIC
    mcu/CMSIS/Include
    mcu/CMSIS/Device/ST/STM32F4xx/Include
    mcu/STM32F4xx
    mcu/STM32F4xx/drivers
    mcu/STM32F4xx/drivers/usb

    targets/MD-3x0
    drivers/ADC
    drivers/NVM
    drivers/GPS
    drivers/USB
    drivers/tones
    drivers/baseband
    drivers/backlight
    drivers/chSelector
)

target_link_libraries(Platform PUBLIC Miosix Core)

elseif(OPENRTX_TARGET STREQUAL "linux")
add_library(Platform STATIC
    drivers/CPS/cps_io_libc.c
    drivers/GPS/GPS_linux.c
    drivers/NVM/nvmem_linux.c
    drivers/audio/audio_linux.c
    drivers/baseband/radio_linux.cpp
    drivers/display/display_libSDL.c
    drivers/keyboard/keyboard_linux.c
    mcu/x86_64/drivers/delays.c
    targets/linux/emulator/emulator.c
    targets/linux/emulator/sdl_engine.c
    targets/linux/platform.c
)

target_include_directories(Platform PUBLIC
    targets/linux
    targets/linux/emulator
)

target_link_libraries(Platform PUBLIC Core)
endif()
