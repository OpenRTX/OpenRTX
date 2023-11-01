set(general_includes
    drivers/ADC
    drivers/NVM
    drivers/GPS
    drivers/USB
    drivers/tones
    drivers/baseband
    drivers/backlight
    drivers/chSelector
)

set(mdx_sources
    # MDx source
    drivers/ADC/ADC1_MDx.c
    drivers/GPS/GPS_MDx.cpp
    drivers/NVM/W25Qx.c
    drivers/NVM/nvmem_settings_MDx.c
    drivers/audio/audio_MDx.c
    drivers/baseband/HR_Cx000.cpp
    drivers/tones/toneGenerator_MDx.cpp
)

set(md3x0_includes targets/MD-3x0)
set(md3x0_defines -DPLATFORM_MD3x0 -Dtimegm=mktime)
set(md3x0_sources
    # MD-3x0 sources
    drivers/CPS/cps_io_native_MD3x0.c
    drivers/NVM/nvmem_MD3x0.c
    drivers/NVM/spiFlash_MD3x.c
    drivers/backlight/backlight_MDx.c
    drivers/baseband/HR_C5000_MDx.cpp
    drivers/baseband/SKY72310.c
    drivers/baseband/radio_MD3x0.cpp
    drivers/display/HX8353_MD3x.cpp
    drivers/keyboard/keyboard_MD3x.c
    targets/MD-3x0/platform.c
)

set(mduv3x0_includes targets/MD-UV3x0)
set(mduv3x0_defines -DPLATFORM_MDUV3x0 -Dtimegm=mktime)
set(mduv3x0_sources
    # MD-UV3x0
    drivers/CPS/cps_io_native_MDUV3x0.c
    drivers/NVM/nvmem_MDUV3x0.c
    drivers/NVM/spiFlash_MD3x.c
    drivers/backlight/backlight_MDx.c
    drivers/baseband/AT1846S_UV3x0.cpp
    drivers/baseband/HR_C6000_UV3x0.cpp
    drivers/baseband/radio_UV3x0.cpp
    drivers/chSelector/chSelector_UV3x0.c
    drivers/display/HX8353_MD3x.cpp
    drivers/keyboard/keyboard_MD3x.c
    targets/MD-UV3x0/platform.c
)

set(stm32f405_includes
    mcu/CMSIS/Device/ST/STM32F4xx/Include
    mcu/CMSIS/Include
    mcu/STM32F4xx
    mcu/STM32F4xx/drivers
    mcu/STM32F4xx/drivers/usb
)

set(stm32f405_defines -DSTM32F405xx -DHSE_VALUE=8000000)
set(stm32f405_sources
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

set(linux_sources
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

set(linux_includes
    targets/linux
    targets/linux/emulator
)

add_library(Platform STATIC)
target_include_directories(Platform PUBLIC ${general_includes})
target_link_libraries(Platform PUBLIC Core)

if(OPENRTX_TARGET STREQUAL "md3x0")

    target_sources(Platform PRIVATE ${mdx_sources} ${md3x0_sources} ${stm32f405_sources})
    target_include_directories(Platform PUBLIC ${md3x0_includes} ${stm32f405_includes})
    target_compile_definitions(Platform PUBLIC ${md3x0_defines} ${stm32f405_defines})
    target_link_libraries(Platform PUBLIC Miosix)

elseif(OPENRTX_TARGET STREQUAL "mduv3x0")

    target_sources(Platform PRIVATE ${mdx_sources} ${mduv3x0_sources} ${stm32f405_sources})
    target_include_directories(Platform PUBLIC ${mduv3x0_includes} ${stm32f405_includes})
    target_compile_definitions(Platform PUBLIC ${mduv3x0_defines} ${stm32f405_defines})
    target_link_libraries(Platform PUBLIC Miosix qdec)

elseif(OPENRTX_TARGET STREQUAL "linux")

    target_sources(Platform PRIVATE ${linux_sources})
    target_include_directories(Platform PUBLIC ${linux_includes})

endif()
