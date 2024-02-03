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

##
## TYT MDx family
##

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

##
## GDx family: Radioddity GD-77 and Baofeng DM-1801
##

set(gdx_sources
    drivers/NVM/W25Qx.c
    drivers/NVM/AT24Cx_GDx.c
    drivers/NVM/spiFlash_GDx.c
    drivers/NVM/nvmem_GDx.c
    drivers/CPS/cps_io_native_GDx.c
    drivers/ADC/ADC0_GDx.c
    drivers/backlight/backlight_GDx.c
    drivers/baseband/radio_GDx.cpp
    drivers/baseband/HR_Cx000.cpp
    drivers/baseband/AT1846S_GDx.cpp
    drivers/baseband/HR_C6000_GDx.cpp
    drivers/display/UC1701_GDx.c
    drivers/keyboard/keyboard_GDx.c
    drivers/audio/audio_GDx.c
)

##
## STM32F405
##

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

##
## MK22FN512
##

set(mk22fn512_sources 
    mcu/MK22FN512xxx12/boot/startup.cpp
    mcu/MK22FN512xxx12/boot/bsp.cpp
    mcu/MK22FN512xxx12/boot/libc_integration.cpp
    mcu/MK22FN512xxx12/drivers/gpio.c
    mcu/MK22FN512xxx12/drivers/delays.cpp
    mcu/MK22FN512xxx12/drivers/I2C0.c
    mcu/MK22FN512xxx12/drivers/rng.c
    mcu/MK22FN512xxx12/drivers/usb/usb_device_cdc_acm.c
    mcu/MK22FN512xxx12/drivers/usb/usb_device_ch9.c
    mcu/MK22FN512xxx12/drivers/usb/usb_device_dci.c
    mcu/MK22FN512xxx12/drivers/usb/usb_device_descriptor.c
    mcu/MK22FN512xxx12/drivers/usb/usb_device_khci.c
    mcu/MK22FN512xxx12/drivers/usb/usb_osa_bm.c
    mcu/MK22FN512xxx12/drivers/usb_vcom.c
    mcu/CMSIS/Device/NXP/MK22FN512xxx12/Source/system_MK22F51212.c
)

set(mk22fn512_includes
    mcu/CMSIS/Include
    mcu/CMSIS/Device/NXP/MK22FN512xxx12/Include
    mcu/MK22FN512xxx12
    mcu/MK22FN512xxx12/drivers
)

set(mk22fn512_defines )

##
## Board specific
##

set(md3x0_includes targets/MD-3x0)
set(md3x0_defines -DPLATFORM_MD3x0 -Dtimegm=mktime)
set(md3x0_sources
    ${mdx_sources}
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
    ${mdx_sources}
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

set(md9600_includes targets/MD-9600)
set(md9600_defines -DPLATFORM_MD9600)
set(md9600_sources
    ${mdx_sources}
    # MD-9600
    drivers/CPS/cps_io_native_MD9600.c
    drivers/NVM/nvmem_MD9600.c
    drivers/NVM/spiFlash_MD9600.c
    drivers/baseband/radio_MD9600.cpp
    drivers/chSelector/chSelector_MD9600.c
    drivers/display/ST7567_MD9600.c
    drivers/keyboard/keyboard_MD9600.c
    targets/MD-9600/platform.c
)

##
## Radioddity GD-77
##
set(gd77_includes targets/GD-77)
set(gd77_defines -DPLATFORM_GD77)
set(gd77_sources
    ${gdx_sources}
    # GD-77
    targets/GD-77/platform.c
)

set(linux_sources
    drivers/CPS/cps_io_libc.c
    drivers/GPS/GPS_linux.c
    drivers/NVM/nvmem_linux.c
    drivers/NVM/posix_file.c
    drivers/audio/audio_linux.c
    drivers/audio/file_source.c
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

    target_sources(Platform PRIVATE ${md3x0_sources} ${stm32f405_sources})
    target_include_directories(Platform PUBLIC ${md3x0_includes} ${stm32f405_includes})
    target_compile_definitions(Platform PUBLIC ${md3x0_defines} ${stm32f405_defines})
    target_link_libraries(Platform PUBLIC Miosix)

elseif(OPENRTX_TARGET STREQUAL "mduv3x0")

    target_sources(Platform PRIVATE ${mduv3x0_sources} ${stm32f405_sources})
    target_include_directories(Platform PUBLIC ${mduv3x0_includes} ${stm32f405_includes})
    target_compile_definitions(Platform PUBLIC ${mduv3x0_defines} ${stm32f405_defines})
    target_link_libraries(Platform PUBLIC Miosix qdec)

elseif(OPENRTX_TARGET STREQUAL "md9600")

    target_sources(Platform PRIVATE ${md9600_sources} ${stm32f405_sources})
    target_include_directories(Platform PUBLIC ${md9600_includes} ${stm32f405_includes})
    target_compile_definitions(Platform PUBLIC ${md9600_defines} ${stm32f405_defines})
    target_link_libraries(Platform PUBLIC Miosix qdec)

elseif(OPENRTX_TARGET STREQUAL "gd77")

    target_sources(Platform PRIVATE ${gd77_sources} ${mk22fn512_sources})
    target_include_directories(Platform PUBLIC ${gd77_includes} ${mk22fn512_includes})
    target_compile_definitions(Platform PUBLIC ${gd77_defines} ${mk22fn512_defines})
    target_link_libraries(Platform PUBLIC Miosix qdec)

elseif(OPENRTX_TARGET STREQUAL "linux")

    target_sources(Platform PRIVATE ${linux_sources})
    target_include_directories(Platform PUBLIC ${linux_includes})

endif()
