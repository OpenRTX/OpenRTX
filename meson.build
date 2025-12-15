##
## OpenRTX - Modular Open Source Radio Firmware
##
project('OpenRTX', ['c', 'cpp'],
  version : '0.4.3',
  default_options : ['warning_level=3', 'b_staticpic=false'])

##
## Optional defines, common to all souces (e.g. to enable debugging)
##

openrtx_def = {}

##
## UI font configuration, only one uncommented at a time
##

openrtx_def += {'FONT_UBUNTU_REGULAR': ''}
# openrtx_def += {'FONT_FREE_SANS': ''}

##
## Firmware configuration parameters
##

#openrtx_def += {}


##
## ----------------- Platform-independent source files -------------------------
##

##
## OpenRTX core sources
##

openrtx_src = ['openrtx/src/core/state.c',
               'openrtx/src/core/threads.c',
               'openrtx/src/core/battery.c',
               'openrtx/src/core/graphics.c',
               'openrtx/src/core/input.c',
               'openrtx/src/core/utils.c',
               'openrtx/src/core/queue.c',
               'openrtx/src/core/chan.c',
               'openrtx/src/core/gps.c',
               'openrtx/src/core/dsp.cpp',
               'openrtx/src/core/cps.c',
               'openrtx/src/core/crc.c',
               'openrtx/src/core/datetime.c',
               'openrtx/src/core/openrtx.c',
               'openrtx/src/core/audio_codec.c',
               'openrtx/src/core/audio_stream.c',
               'openrtx/src/core/audio_path.cpp',
               'openrtx/src/core/data_conversion.c',
               'openrtx/src/core/memory_profiling.cpp',
               'openrtx/src/core/voicePrompts.c',
               'openrtx/src/core/voicePromptUtils.c',
               'openrtx/src/core/voicePromptData.S',
               'openrtx/src/core/nvmem_access.c',
               'openrtx/src/rtx/rtx.cpp',
               'openrtx/src/rtx/OpMode_FM.cpp',
               'openrtx/src/rtx/OpMode_M17.cpp',
               'openrtx/src/protocols/M17/M17DSP.cpp',
               'openrtx/src/protocols/M17/M17Golay.cpp',
               'openrtx/src/protocols/M17/M17Callsign.cpp',
               'openrtx/src/protocols/M17/Callsign.cpp',
               'openrtx/src/protocols/M17/M17Modulator.cpp',
               'openrtx/src/protocols/M17/M17Demodulator.cpp',
               'openrtx/src/protocols/M17/M17FrameEncoder.cpp',
               'openrtx/src/protocols/M17/M17FrameDecoder.cpp',
               'openrtx/src/protocols/M17/M17LinkSetupFrame.cpp']

openrtx_inc = ['openrtx/include', 'platform']

##
## OpenRTX UI sources
##

ui_src_default = ['openrtx/src/ui/default/ui.c',
                  'openrtx/src/ui/default/ui_main.c',
                  'openrtx/src/ui/default/ui_menu.c',
                  'openrtx/src/ui/default/ui_strings.c']

ui_src_module17 = ['openrtx/src/ui/module17/ui.c',
                   'openrtx/src/ui/module17/ui_main.c',
                   'openrtx/src/ui/module17/ui_menu.c']

##
## Selection of main entrypoint
##
if get_option('test') != ''
  main_src     = 'tests/platform/'+get_option('test')+'.c'
  openrtx_def += {'RUNNING_TESTSUITE' : ''}
else
  main_src     = 'openrtx/src/main.c'
endif

##
## External libraries
##

# minmea, a lightweight GPS NMEA 0183 parser library
openrtx_src += ['lib/minmea/minmea.c']
openrtx_inc += ['lib/minmea/include']

# QDEC, a very simple, header only, quadrature decoding library
openrtx_inc += ['lib/qdec/include']

# CODEC2, open source speech codec, compile from source on embedded
codec2_proj = subproject('codec2')
if meson.is_cross_build()
  codec2_dep  = codec2_proj.get_variable('codec2_dep')
else
  cpp        = meson.get_compiler('cpp')
  codec2_dep = cpp.find_library('codec2')
endif

# XPowersLib, library for x-powers power management series
xpowerslib_proj = subproject('XPowersLib')

##
## RTOS
##
subdir('lib/miosix-kernel')

##
## Current git commit or tag
##
r = run_command('git', 'describe', '--tags', '--dirty', '--always')
if r.returncode() != 0
  # it failed
endif
git_version  = '"'+r.stdout().strip()+'"'
openrtx_def += {'GIT_VERSION': git_version}

##
## --------------------- Family-dependent source files -------------------------
##

##
## TYT MDx family
##

mdx_src = ['platform/drivers/NVM/W25Qx.c',
           'platform/drivers/NVM/nvmem_settings_MDx.c',
           'platform/drivers/NVM/nvmem_MDx.c',
           'platform/drivers/audio/audio_MDx.cpp',
           'platform/drivers/baseband/HR_Cx000.cpp',
           'platform/drivers/baseband/HR_C6000.cpp',
           'platform/drivers/tones/toneGenerator_MDx.cpp',
           'platform/drivers/SPI/spi_custom.c',
           'platform/drivers/SPI/spi_bitbang.c']

##
## GDx family: Radioddity GD-77 and Baofeng DM-1801
##

gdx_src = ['platform/targets/GDx/platform.c',
           'platform/targets/GDx/hwconfig.c',
           'platform/drivers/NVM/W25Qx.c',
           'platform/drivers/NVM/AT24Cx_GDx.c',
           'platform/drivers/NVM/nvmem_GDx.c',
           'platform/drivers/CPS/cps_io_native_GDx.c',
           'platform/drivers/ADC/ADC0_GDx.c',
           'platform/drivers/backlight/backlight_GDx.c',
           'platform/drivers/baseband/radio_GDx.cpp',
           'platform/drivers/baseband/HR_Cx000.cpp',
           'platform/drivers/baseband/HR_C6000.cpp',
           'platform/drivers/baseband/AT1846S_GDx.cpp',
           'platform/drivers/baseband/HR_C6000_GDx.cpp',
           'platform/drivers/display/UC1701_GDx.c',
           'platform/drivers/keyboard/keyboard_GDx.c',
           'platform/drivers/audio/audio_GDx.c',
           'platform/drivers/SPI/spi_custom.c',
           'platform/drivers/SPI/spi_bitbang.c',
           'platform/drivers/SPI/spi_mk22.c']

gdx_inc = ['platform/targets/GDx']

##
## --------------------- MCU-dependent source files ----------------------------
##

##
## STM32F405
##

stm32f405_src = ['platform/mcu/STM32F4xx/boot/startup.cpp',
                 'platform/mcu/STM32F4xx/boot/bsp.cpp',
                 'platform/mcu/STM32F4xx/boot/libc_integration.cpp',
                 'platform/mcu/STM32F4xx/drivers/usb/usb_bsp.c',
                 'platform/mcu/STM32F4xx/drivers/usb/usb_core.c',
                 'platform/mcu/STM32F4xx/drivers/usb/usb_dcd.c',
                 'platform/mcu/STM32F4xx/drivers/usb/usb_dcd_int.c',
                 'platform/mcu/STM32F4xx/drivers/usb/usbd_desc.c',
                 'platform/mcu/STM32F4xx/drivers/usb/usbd_core.c',
                 'platform/mcu/STM32F4xx/drivers/usb/usbd_ioreq.c',
                 'platform/mcu/STM32F4xx/drivers/usb/usbd_req.c',
                 'platform/mcu/STM32F4xx/drivers/usb/usbd_usr.c',
                 'platform/mcu/STM32F4xx/drivers/usb_vcom.c',
                 'platform/mcu/STM32F4xx/drivers/delays.cpp',
                 'platform/mcu/STM32F4xx/drivers/rtc.c',
                 'platform/mcu/STM32F4xx/drivers/USART3.cpp',
                 'platform/mcu/STM32F4xx/drivers/flash.c',
                 'platform/mcu/STM32F4xx/drivers/rng.c',
                 'platform/mcu/STM32F4xx/drivers/rcc.c',
                 'platform/mcu/STM32F4xx/drivers/i2c_stm32.c',
                 'platform/drivers/ADC/adc_stm32f4.c',
                 'platform/drivers/SPI/spi_stm32f4.c',
                 'platform/drivers/GPIO/gpio_stm32.c',
                 'platform/drivers/audio/stm32_dac.cpp',
                 'platform/drivers/audio/stm32_adc.cpp',
                 'platform/drivers/audio/stm32_pwm.cpp',
                 'platform/drivers/keyboard/cap1206.c',
                 'platform/mcu/CMSIS/Device/ST/STM32F4xx/Source/system_stm32f4xx.c']

stm32f405_inc = ['platform/mcu/CMSIS/Include',
                 'platform/mcu/CMSIS/Device/ST/STM32F4xx/Include',
                 'platform/mcu/STM32F4xx',
                 'platform/mcu/STM32F4xx/drivers',
                 'platform/mcu/STM32F4xx/drivers/usb']

stm32f405_def = {'STM32F405xx': '', 'HSE_VALUE':'8000000'}

stm32f405_src += miosix_cm4f_src
stm32f405_inc += miosix_cm4f_inc
stm32f405_def += miosix_cm4f_def

##
## MK22FN512
##

mk22fn512_src = ['platform/mcu/MK22FN512xxx12/boot/startup.cpp',
                 'platform/mcu/MK22FN512xxx12/boot/bsp.cpp',
                 'platform/mcu/MK22FN512xxx12/boot/libc_integration.cpp',
                 'platform/mcu/MK22FN512xxx12/drivers/delays.cpp',
                 'platform/mcu/MK22FN512xxx12/drivers/I2C0.c',
                 'platform/mcu/MK22FN512xxx12/drivers/rng.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_cdc_acm.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_ch9.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_dci.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_descriptor.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_khci.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_osa_bm.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb_vcom.c',
                 'platform/drivers/SPI/spi_mk22.c',
                 'platform/drivers/GPIO/gpio_mk22.c',
                 'platform/mcu/CMSIS/Device/NXP/MK22FN512xxx12/Source/system_MK22F51212.c']

mk22fn512_inc = ['platform/mcu/CMSIS/Include',
                 'platform/mcu/CMSIS/Device/NXP/MK22FN512xxx12/Include',
                 'platform/mcu/MK22FN512xxx12',
                 'platform/mcu/MK22FN512xxx12/drivers']

mk22fn512_def = {'MK22FN512xx': ''}

mk22fn512_src += miosix_cm4f_src
mk22fn512_inc += miosix_cm4f_inc
mk22fn512_def += miosix_cm4f_def

##
## STM32H743
##

stm32h743_src = ['platform/mcu/STM32H7xx/boot/startup.cpp',
                 'platform/mcu/STM32H7xx/boot/bsp.cpp',
                 'platform/mcu/STM32H7xx/boot/libc_integration.cpp',
                 'platform/mcu/STM32H7xx/drivers/rcc.cpp',
                 'platform/mcu/STM32H7xx/drivers/delays.cpp',
                 'platform/drivers/GPIO/gpio_stm32.c',
                 'platform/drivers/ADC/adc_stm32h7.c',
                 'platform/drivers/SPI/spi_stm32h7.c',
                 'platform/drivers/GPS/gps_stm32.cpp',
                 'platform/drivers/GPS/nmea_rbuf.c',
                 'platform/drivers/audio/stm32_dac.cpp',
                 'platform/drivers/audio/stm32_adc.cpp',
                 'platform/mcu/CMSIS/Device/ST/STM32H7xx/Source/system_stm32h7xx.c']

stm32h743_inc = ['platform/mcu/CMSIS/Include',
                 'platform/mcu/CMSIS/Device/ST/STM32H7xx/Include',
                 'platform/mcu/STM32H7xx',
                 'platform/mcu/STM32H7xx/drivers']

stm32h743_def = {'STM32H743xx': '', 'HSE_VALUE':'25000000'}

stm32h743_src += miosix_cm7_src
stm32h743_inc += miosix_cm7_inc
stm32h743_def += miosix_cm7_def

##
## ----------------------- Platform specializations ----------------------------
##

##
## Linux
##
linux_src = ['platform/targets/linux/emulator/emulator.c',
             'platform/targets/linux/emulator/sdl_engine.c',
             'platform/drivers/display/display_libSDL.c',
             'platform/drivers/keyboard/keyboard_linux.c',
             'platform/drivers/NVM/nvmem_linux.c',
             'platform/drivers/GPS/gps_linux.c',
             'platform/mcu/x86_64/drivers/delays.c',
             'platform/mcu/x86_64/drivers/rng.cpp',
             'platform/drivers/baseband/radio_linux.cpp',
             'platform/drivers/audio/audio_linux.c',
             'platform/drivers/audio/file_source.c',
             'platform/targets/linux/platform.c',
             'platform/drivers/CPS/cps_io_libc.c',
             'platform/drivers/NVM/posix_file.c']

linux_inc = ['platform/targets/linux',
             'platform/targets/linux/emulator']

linux_def = {'PLATFORM_LINUX': '', 'VP_USE_FILESYSTEM':''}

sdl_dep     = dependency('SDL2',     required: false)
threads_dep = dependency('threads',  required: false)
linux_src  += openrtx_src
linux_inc  += openrtx_inc
linux_def  += openrtx_def
linux_def  += {'sniprintf':'snprintf', 'vsniprintf':'vsnprintf'}

#
# Standard UI
#
linux_default_src = linux_src + ui_src_default
linux_default_def = linux_def + {'CONFIG_SCREEN_WIDTH': '160', 'CONFIG_SCREEN_HEIGHT': '128', 'CONFIG_PIX_FMT_RGB565': '',
                                 'CONFIG_GPS': '', 'CONFIG_RTC': ''}
linux_small_def   = linux_def + {'CONFIG_SCREEN_WIDTH': '128', 'CONFIG_SCREEN_HEIGHT': '64', 'CONFIG_PIX_FMT_BW': '',
                                 'CONFIG_GPS': '', 'CONFIG_RTC': ''}

#
# Module17 UI
#
linux_mod17_src = linux_src + ui_src_module17
linux_mod17_def = linux_def + {'CONFIG_SCREEN_WIDTH': '128', 'CONFIG_SCREEN_HEIGHT': '64', 'CONFIG_PIX_FMT_BW': ''}

linux_c_args   = ['-ffunction-sections', '-fdata-sections', '-std=gnu17']
linux_cpp_args = ['-ffunction-sections', '-fdata-sections', '-std=c++14']
linux_l_args   = ['-lm', '-lreadline']

# MacOS Clang linker uses -deadstrip instead of --gc-sections
if build_machine.system() == 'darwin'
  linux_l_args += '-Wl,-dead_strip'
else
  linux_l_args += '-Wl,--gc-sections'
endif

# Add AddressSanitizer if required
if get_option('asan')
  linux_c_args   += '-fsanitize=address'
  linux_cpp_args += '-fsanitize=address'
  linux_l_args   += '-fsanitize=address'
endif

# Add Undefined Behaviour Sanitizer if required
if get_option('ubsan')
  linux_c_args   += '-fsanitize=undefined'
  linux_cpp_args += '-fsanitize=undefined'
  linux_l_args   += '-fsanitize=undefined'
endif


##
## TYT MD-3x0 family
##
md3x0_src = ['platform/drivers/CPS/cps_io_native_MD3x0.c',
             'platform/drivers/baseband/SKY72310.c',
             'platform/drivers/baseband/radio_MD3x0.cpp',
             'platform/drivers/baseband/HR_C5000_MDx.cpp',
             'platform/drivers/keyboard/keyboard_MD3x.c',
             'platform/drivers/display/HX8353_MD3x.cpp',
             'platform/drivers/backlight/backlight_MDx.c',
             'platform/drivers/GPS/gps_stm32.cpp',
             'platform/drivers/GPS/nmea_rbuf.c',
             'platform/targets/MD-3x0/platform.c',
             'platform/targets/MD-3x0/hwconfig.c']

md3x0_inc = ['platform/targets/MD-3x0']
md3x0_def = {'PLATFORM_MD3x0': '', 'timegm': 'mktime'}

md3x0_src += openrtx_src + stm32f405_src + ui_src_default + mdx_src
md3x0_inc += openrtx_inc + stm32f405_inc
md3x0_def += openrtx_def + stm32f405_def

##
## TYT MD-UV380
##
mduv3x0_src = ['platform/drivers/CPS/cps_io_native_MDUV3x0.c',
               'platform/targets/MD-UV3x0/platform.c',
               'platform/targets/MD-UV3x0/hwconfig.c',
               'platform/drivers/GPS/gps_stm32.cpp',
               'platform/drivers/GPS/nmea_rbuf.c',
               'platform/drivers/keyboard/keyboard_MD3x.c',
               'platform/drivers/display/HX8353_MD3x.cpp',
               'platform/drivers/backlight/backlight_MDx.c',
               'platform/drivers/chSelector/chSelector_UV3x0.c',
               'platform/drivers/audio/Cx000_dac.cpp',
               'platform/drivers/baseband/radio_UV3x0.cpp',
               'platform/drivers/baseband/AT1846S_UV3x0.cpp',
               'platform/drivers/baseband/HR_C6000_UV3x0.cpp']

mduv3x0_inc = ['platform/targets/MD-UV3x0']
mduv3x0_def =  {'PLATFORM_MDUV3x0': '', 'timegm': 'mktime'}

mduv3x0_src += openrtx_src + stm32f405_src + ui_src_default + mdx_src
mduv3x0_inc += openrtx_inc + stm32f405_inc
mduv3x0_def += openrtx_def + stm32f405_def

##
## TYT MD-9600
##
md9600_src = ['platform/targets/MD-9600/platform.c',
              'platform/targets/MD-9600/hwconfig.c',
              'platform/drivers/GPS/gps_stm32.cpp',
              'platform/drivers/GPS/nmea_rbuf.c',
              'platform/drivers/display/ST7567_MD9600.c',
              'platform/drivers/keyboard/keyboard_MD9600.c',
              'platform/drivers/backlight/backlight_MDx.c',
              'platform/drivers/chSelector/chSelector_MD9600.c',
              'platform/drivers/baseband/radio_MD9600.cpp',
              'platform/drivers/CPS/cps_io_native_MD9600.c']

md9600_inc = ['platform/targets/MD-9600']
md9600_def = {'PLATFORM_MD9600': ''}

md9600_src += openrtx_src + stm32f405_src + ui_src_default + mdx_src
md9600_inc += openrtx_inc + stm32f405_inc
md9600_def += openrtx_def + stm32f405_def

##
## Radioddity GD-77
##
gd77_src = []
gd77_inc = []
gd77_def = {'PLATFORM_GD77': ''}

gd77_src += openrtx_src + mk22fn512_src + ui_src_default + gdx_src
gd77_inc += openrtx_inc + mk22fn512_inc + gdx_inc
gd77_def += openrtx_def + mk22fn512_def

##
## Baofeng DM-1801
##
dm1801_src = []
dm1801_inc = []
dm1801_def = {'PLATFORM_DM1801': ''}

dm1801_src += openrtx_src + mk22fn512_src + ui_src_default + gdx_src
dm1801_inc += openrtx_inc + mk22fn512_inc + gdx_inc
dm1801_def += openrtx_def + mk22fn512_def

##
## Module 17
##
mod17_src = ['platform/targets/Module17/platform.c',
             'platform/drivers/display/display_Mod17.c',
             'platform/drivers/display/SH110x_Mod17.c',
             'platform/drivers/display/SSD1309_Mod17.c',
             'platform/drivers/keyboard/keyboard_Mod17.c',
             'platform/drivers/NVM/nvmem_Mod17.c',
             'platform/drivers/CPS/cps_io_native_Mod17.c',
             'platform/drivers/baseband/radio_Mod17.cpp',
             'platform/drivers/audio/audio_Mod17.c',
             'platform/drivers/audio/MAX9814_Mod17.cpp',
             'platform/drivers/baseband/MCP4551.c']

mod17_inc = ['platform/targets/Module17']
mod17_def = {'PLATFORM_MOD17': ''}

mod17_src += openrtx_src + stm32f405_src + ui_src_module17
mod17_inc += openrtx_inc + stm32f405_inc
mod17_def += openrtx_def + stm32f405_def

##
## Connect Systems CS70000
##
cs7000_src = ['platform/drivers/NVM/nvmem_CS7000.c',
             'platform/drivers/NVM/W25Qx.c',
             'platform/drivers/NVM/eeep.c',
             'platform/drivers/stubs/cps_io_stub.c',
             'platform/drivers/baseband/AK2365A.c',
             'platform/drivers/baseband/SKY72310.c',
             'platform/drivers/baseband/HR_C6000.cpp',
             'platform/drivers/baseband/HR_Cx000.cpp',
             'platform/drivers/baseband/HR_C6000_CS7000.cpp',
             'platform/drivers/baseband/radio_CS7000.cpp',
             'platform/drivers/keyboard/keyboard_CS7000.c',
             'platform/drivers/display/ST7735R_CS7000.c',
             'platform/drivers/backlight/backlight_CS7000.c',
             'platform/drivers/audio/Cx000_dac.cpp',
             'platform/drivers/audio/audio_CS7000.cpp',
             'platform/drivers/GPIO/gpio_shiftReg.c',
             'platform/drivers/SPI/spi_custom.c',
             'platform/drivers/SPI/spi_bitbang.c',
             'platform/drivers/GPS/gps_stm32.cpp',
             'platform/drivers/GPS/nmea_rbuf.c',
             'platform/targets/CS7000/hwconfig.c',
             'platform/targets/CS7000/platform.c']

cs7000_inc = ['platform/targets/CS7000']
cs7000_def = {'PLATFORM_CS7000': '', 'timegm': 'mktime'}

cs7000_src += openrtx_src + stm32f405_src + ui_src_default
cs7000_inc += openrtx_inc + stm32f405_inc
cs7000_def += openrtx_def + stm32f405_def

##
## Connect Systems CS70000-PLUS
##
cs7000p_src = ['platform/drivers/NVM/nvmem_CS7000.c',
               'platform/drivers/NVM/W25Qx.c',
               'platform/drivers/NVM/eeep.c',
               'platform/drivers/baseband/HR_C6000.cpp',
               'platform/drivers/baseband/HR_Cx000.cpp',
               'platform/drivers/baseband/HR_C6000_CS7000.cpp',
               'platform/drivers/baseband/radio_CS7000.cpp',
               'platform/drivers/baseband/AK2365A.c',
               'platform/drivers/baseband/SKY72310.c',
               'platform/drivers/audio/Cx000_dac.cpp',
               'platform/drivers/audio/audio_CS7000.cpp',
               'platform/drivers/stubs/cps_io_stub.c',
               'platform/drivers/display/ST7735R_CS7000.c',
               'platform/drivers/keyboard/keyboard_CS7000.c',
               'platform/drivers/backlight/backlight_CS7000.c',
               'platform/drivers/GPIO/gpio_shiftReg.c',
               'platform/drivers/SPI/spi_custom.c',
               'platform/drivers/SPI/spi_bitbang.c',
               'platform/targets/CS7000-PLUS/hwconfig.c',
               'platform/targets/CS7000-PLUS/platform.c']

cs7000p_inc = ['platform/targets/CS7000-PLUS']
cs7000p_def = {'PLATFORM_CS7000P': '', 'timegm': 'mktime'}

cs7000p_src += openrtx_src + stm32h743_src + miosix_cm7_src + ui_src_default
cs7000p_inc += openrtx_inc + stm32h743_inc + miosix_cm7_inc
cs7000p_def += openrtx_def + stm32h743_def + miosix_cm7_def

##
## Baofeng DM-1701
##
dm1701_src = ['platform/targets/DM-1701/platform.c',
              'platform/targets/DM-1701/hwconfig.c',
              'platform/drivers/keyboard/keyboard_DM1701.c',
              'platform/drivers/CPS/cps_io_native_MDUV3x0.c',
              'platform/drivers/display/HX8353_MD3x.cpp',
              'platform/drivers/backlight/backlight_MDx.c',
              'platform/drivers/chSelector/chSelector_UV3x0.c',
              'platform/drivers/audio/Cx000_dac.cpp',
              'platform/drivers/baseband/radio_UV3x0.cpp',
              'platform/drivers/baseband/AT1846S_UV3x0.cpp',
              'platform/drivers/baseband/HR_C6000_UV3x0.cpp']

dm1701_inc = ['platform/targets/DM-1701']
dm1701_def = {'PLATFORM_DM1701': '', 'timegm': 'mktime'}

dm1701_src += openrtx_src + stm32f405_src + ui_src_default + mdx_src
dm1701_inc += openrtx_inc + stm32f405_inc
dm1701_def += openrtx_def + stm32f405_def

##
## -------------------------- Compilation arguments ----------------------------
##

linux_c_args         = linux_c_args
linux_cpp_args       = linux_cpp_args
linux_small_c_args   = linux_c_args
linux_small_cpp_args = linux_cpp_args
linux_mod17_c_args   = linux_c_args
linux_mod17_cpp_args = linux_cpp_args

foreach k, v : linux_default_def
  if v == ''
    linux_c_args   += '-D@0@'.format(k)
    linux_cpp_args += '-D@0@'.format(k)
  else
    linux_c_args   += '-D@0@=@1@'.format(k, v)
    linux_cpp_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

foreach k, v : linux_small_def
  if v == ''
    linux_small_c_args   += '-D@0@'.format(k)
    linux_small_cpp_args += '-D@0@'.format(k)
  else
    linux_small_c_args   += '-D@0@=@1@'.format(k, v)
    linux_small_cpp_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

foreach k, v : linux_mod17_def
  if v == ''
    linux_mod17_c_args   += '-D@0@'.format(k)
    linux_mod17_cpp_args += '-D@0@'.format(k)
  else
    linux_mod17_c_args   += '-D@0@=@1@'.format(k, v)
    linux_mod17_cpp_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

md3x0_args = []
foreach k, v : md3x0_def
  if v == ''
    md3x0_args += '-D@0@'.format(k)
  else
    md3x0_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

mduv3x0_args = []
foreach k, v : mduv3x0_def
  if v == ''
    mduv3x0_args += '-D@0@'.format(k)
  else
    mduv3x0_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

gd77_args = []
foreach k, v : gd77_def
  if v == ''
    gd77_args += '-D@0@'.format(k)
  else
    gd77_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

dm1801_args = []
foreach k, v : dm1801_def
  if v == ''
    dm1801_args += '-D@0@'.format(k)
  else
    dm1801_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

md9600_args = []
foreach k, v : md9600_def
  if v == ''
    md9600_args += '-D@0@'.format(k)
  else
    md9600_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

mod17_args = []
foreach k, v : mod17_def
  if v == ''
    mod17_args += '-D@0@'.format(k)
  else
    mod17_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

cs7000_args = []
foreach k, v : cs7000_def
  if v == ''
    cs7000_args += '-D@0@'.format(k)
  else
    cs7000_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

cs7000p_args = []
foreach k, v : cs7000p_def
  if v == ''
    cs7000p_args += '-D@0@'.format(k)
  else
    cs7000p_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

dm1701_args = []
foreach k, v : dm1701_def
  if v == ''
    dm1701_args += '-D@0@'.format(k)
  else
    dm1701_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

linux_opts = {
  'sources'            : linux_default_src,
  'include_directories': linux_inc,
  'dependencies'       : [sdl_dep, threads_dep, codec2_dep],
  'c_args'             : linux_c_args,
  'cpp_args'           : linux_cpp_args,
  'link_args'          : linux_l_args
}

linux_small_opts = {
    'sources'            : linux_default_src,
    'include_directories': linux_inc,
    'dependencies'       : [sdl_dep, threads_dep, codec2_dep],
    'c_args'             : linux_small_c_args,
    'cpp_args'           : linux_small_cpp_args,
    'link_args'          : linux_l_args
}

linux_mod17_opts = {
    'sources'            : linux_mod17_src,
    'include_directories': linux_inc,
    'dependencies'       : [sdl_dep, threads_dep, codec2_dep],
    'c_args'             : linux_mod17_c_args,
    'cpp_args'           : linux_mod17_cpp_args,
    'link_args'          : linux_l_args
}

md3x0_opts = {
  'sources'            : md3x0_src,
  'include_directories': md3x0_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : md3x0_args,
  'cpp_args'           : md3x0_args,
  'link_args'          : ['-Wl,-T../platform/mcu/STM32F4xx/linker_script_MDx.ld']
}

mduv3x0_opts = {
  'sources'            : mduv3x0_src,
  'include_directories': mduv3x0_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : mduv3x0_args,
  'cpp_args'           : mduv3x0_args,
  'link_args'          : ['-Wl,-T../platform/mcu/STM32F4xx/linker_script_MDx.ld']
}

gd77_opts = {
  'sources'            : gd77_src,
  'include_directories': gd77_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : gd77_args,
  'cpp_args'           : gd77_args,
  'link_args'          : ['-Wl,-T../platform/mcu/MK22FN512xxx12/linker_script.ld']
}

dm1801_opts = {
  'sources'            : dm1801_src,
  'include_directories': dm1801_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : dm1801_args,
  'cpp_args'           : dm1801_args,
  'link_args'          : ['-Wl,-T../platform/mcu/MK22FN512xxx12/linker_script.ld']
}

md9600_opts = {
  'sources'            : md9600_src,
  'include_directories': md9600_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : md9600_args,
  'cpp_args'           : md9600_args,
  'link_args'          : ['-Wl,-T../platform/mcu/STM32F4xx/linker_script_MDx.ld']
}

mod17_opts = {
  'sources'            : mod17_src,
  'include_directories': mod17_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : mod17_args,
  'cpp_args'           : mod17_args,
  'link_args'          : ['-Wl,-T../platform/mcu/STM32F4xx/stm32_1m+192k_rom.ld']
}

ttwrplus_opts = {
  'sources'            : ['CMakeLists.txt'],
  'include_directories': [],
  'dependencies'       : [codec2_dep],
  'c_args'             : [],
  'cpp_args'           : [],
  'link_args'          : [],
}

cs7000_opts = {
  'sources'            : cs7000_src,
  'include_directories': cs7000_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : cs7000_args,
  'cpp_args'           : cs7000_args,
  'link_args'          : ['-Wl,-T../platform/mcu/STM32F4xx/stm32_1m+192k_rom.ld']
}

cs7000p_opts = {
  'sources'            : cs7000p_src,
  'include_directories': cs7000p_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : cs7000p_args,
  'cpp_args'           : cs7000p_args,
  'link_args'          : ['-Wl,-T../platform/mcu/STM32H7xx/linker_script_cs7000p.ld',
                          '-Wl,--print-memory-usage']
}

dm1701_opts = {
  'sources'            : dm1701_src,
  'include_directories': dm1701_inc,
  'dependencies'       : [codec2_dep],
  'c_args'             : dm1701_args,
  'cpp_args'           : dm1701_args,
  'link_args'          : ['-Wl,-T../platform/mcu/STM32F4xx/linker_script_MDx.ld']
}

##
## ---------------------------- Compilation targets ----------------------------
##

targets = [
  {
    'name'     : 'linux',
    'opts'     : linux_opts,
    'flashable': false
  },
  {
    'name'     : 'linux_smallscreen',
    'opts'     : linux_small_opts,
    'flashable': false
  },
  {
    'name'     : 'linux_mod17',
    'opts'     : linux_mod17_opts,
    'flashable': false
  },
  {
    'name'     : 'md3x0',
    'opts'     : md3x0_opts,
    'flashable': true,
    'wrap'     : 'MD380',
    'load_addr': '0x0800C000'
  },
  {
    'name'     : 'mduv3x0',
    'opts'     : mduv3x0_opts,
    'flashable': true,
    'wrap'     : 'UV3X0',
    'load_addr': '0x0800C000'
  },
  {
    'name'     : 'gd77',
    'opts'     : gd77_opts,
    'flashable': true,
    'wrap'     : 'GD-77',
    'load_addr': '0x0800C000'
  },
  {
    'name'     : 'dm1801',
    'opts'     : dm1801_opts,
    'flashable': true,
    'wrap'     : 'DM-1801',
    'load_addr': '0x0800C000'
  },
  {
    'name'     : 'md9600',
    'opts'     : md9600_opts,
    'flashable': true,
    'wrap'     : 'MD9600',
    'load_addr': '0x0800C000'
  },
  {
    'name'     : 'mod17',
    'opts'     : mod17_opts,
    'flashable': true,
    'wrap'     : ' ',
    'load_addr': ' '
  },
  {
    'name'     : 'ttwrplus',
    'opts'     : ttwrplus_opts,
    'flashable': true,
    'wrap'     : ' ',
    'load_addr': ' '
  },
  {
    'name'     : 'cs7000',
    'opts'     : cs7000_opts,
    'flashable': true,
    'wrap'     : 'cs7000',
    'load_addr': '0x08000000'
  },
  {
    'name'     : 'cs7000p',
    'opts'     : cs7000p_opts,
    'flashable': true,
    'wrap'     : ' ',
    'load_addr': '0x08100000'
  },
  {
    'name'     : 'dm1701',
    'opts'     : dm1701_opts,
    'flashable': true,
    'wrap'     : 'DM1701',
    'load_addr': '0x0800C000'
  },
]

radio_tool  = find_program('radio_tool')
bin2sgl     = find_program('scripts/bin2sgl.py',               required:false, disabler:true)
objcopy     = find_program('objcopy',                          required:false, disabler:true)
gd77_loader = find_program('scripts/gd-77_firmware_loader.py', required:false, disabler:true)
dfu_util    = find_program('dfu-util',                         required:false, disabler:true)
west        = find_program('west',                             required:false, disabler:true)
uf2conv     = find_program('scripts/uf2conv.py',               required:false, disabler:true)
dfu_convert = find_program('scripts/dfu-convert.py',           required:false, disabler:true)
cs7000_wrap = find_program('scripts/cs7000_wrap.py',           required:false, disabler:true)

foreach t : targets

  name = 'openrtx_'+t['name']

  # ttwrplus is a Zephyr target, we compile it using west and package it in uf2 format
  if name == 'openrtx_ttwrplus'

      if build_machine.system() == 'windows'

          txt = custom_target('Copy CMakeLists.txt',
                              input            : 'CMakeLists.txt',
                              output           : 'CMakeLists.txt',
                              command          : ['xcopy', '@INPUT@', '@OUTPUT@'],
                              install          : false,
                              build_by_default : true)
      else  # Unix-like OS that has cp

          txt = custom_target('Copy CMakeLists.txt',
                              input            : 'CMakeLists.txt',
                              output           : 'CMakeLists.txt',
                              command          : ['cp', '@INPUT@', '@OUTPUT@'],
                              install          : false,
                              build_by_default : true)
      endif


      bin = custom_target(name,
                          input   : txt,
                          output  : name+'_bin',
                          command : [west,
                                    'build',
                                    '-b', 'ttwrplus',
                                    '-d', '.', '..'])

      uf2 = custom_target(name+'_uf2',
                          input   : bin,
                          output  : name+'.uf2',
                          command : [uf2conv,
                                    'zephyr/zephyr.bin',
                                    '-c',
                                    '-f', 'ESP32S3',
                                    '-b', '0x0',
                                    '-o', '@OUTPUT@'])

      custom_target(name+'_flash',
                    input   : uf2,
                    output  : name+'_flash',
                    command : [uf2conv, '-D', '@INPUT@'])

  else

    target_opts = t['opts']
    exe = executable(name,
                     sources             : target_opts['sources'] + main_src,
                     include_directories : target_opts['include_directories'],
                     dependencies        : target_opts['dependencies'],
                     c_args              : target_opts['c_args'],
                     cpp_args            : target_opts['cpp_args'],
                     link_args           : target_opts['link_args'])

    if t['flashable']

      bin = custom_target(name+'_bin',
                          output  : name+'_bin',
                          input   : exe,
                          command : [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@'])

      # Handle GD77 custom wrapping and flashing tools
      if name == 'openrtx_gd77' or name == 'openrtx_dm1801'

        wrap = custom_target(name+'_wrap',
                              output  : name+'_bin.sgl',
                              input   : bin,
                              command : [bin2sgl,
                                        '--in', '@INPUT@',
                                        '--out', '@OUTPUT@',
                                        '--mode', t['wrap']])

        custom_target(name+'_flash',
                      input   : wrap,
                      output  : name+'_flash',
                      command : [gd77_loader, '-f', '@INPUT@', '-m', t['wrap']])

      # Module17 uses dfu-tool for flashing
      elif name == 'openrtx_mod17'

        custom_target(name+'_flash',
                      input   : bin,
                      output  : name+'_flash',
                      command : [dfu_util,
                                 '-d', '0483:df11',
                                 '-a', '0',
                                 '-D', '@INPUT@',
                                 '-s', '0x08000000'])

      elif name == 'openrtx_cs7000' or name == 'openrtx_cs7000p'

        custom_target(name+'_dfu',
                      output  : name+'_dfu',
                      input   : bin,
                      command : [dfu_convert, '-b', t['load_addr']+':@INPUT@', '@OUTPUT@'])

        custom_target(name+'_wrap',
                      output  : name+'_wrap',
                      input   : bin,
                      command : [cs7000_wrap, '@INPUT@', '@OUTPUT@'])

        custom_target(name+'_flash',
                      input   : bin,
                      output  : name+'_flash',
                      command : [dfu_util,
                                 '-d', '0483:df11',
                                 '-a', '0',
                                 '-D', '@INPUT@',
                                 '-s', t['load_addr']])
      else

        wrap = custom_target(name+'_wrap',
                             output  : name+'_wrap',
                             input   : bin,
                             command : [radio_tool,
                                        '--wrap',
                                        '-o', '@OUTPUT@',
                                        '-r', t['wrap'],
                                        '-s', t['load_addr']+':@INPUT@'])

        custom_target(name+'_flash',
                      input   : wrap,
                      output  : name+'_flash',
                      command : [radio_tool, '-d', '0', '-f', '-i', '@INPUT@'])

      endif

    endif

  endif
endforeach

##
## ----------------------------------- Unit Tests ------------------------------
##

unit_test_opts = {'c_args'             : linux_c_args,
                  'cpp_args'           : linux_cpp_args,
                  'include_directories': linux_inc,
                  'dependencies'       : [sdl_dep, threads_dep, codec2_dep],
                  'link_args'          : linux_l_args}

unit_test_src = linux_src + ui_src_default

m17_golay_test = executable('m17_golay_test',
                            sources : unit_test_src + ['tests/unit/M17_golay.cpp'],
                            kwargs  : unit_test_opts)

m17_viterbi_test = executable('m17_viterbi_test',
                               sources : unit_test_src + ['tests/unit/M17_viterbi.cpp'],
                               kwargs  : unit_test_opts)

m17_callsign_test = executable('m17_callsign_test',
                      sources : unit_test_src + ['tests/unit/M17_callsign.cpp'],
                      kwargs  : unit_test_opts)
                 
m17_demodulator_test = executable('m17_demodulator_test',
                            sources: unit_test_src + ['tests/unit/M17_demodulator.cpp'],
                            kwargs: unit_test_opts)

m17_rrc_test = executable('m17_rrc_test',
                          sources: unit_test_src + ['tests/unit/M17_rrc.cpp'],
                          kwargs: unit_test_opts)

cps_test = executable('cps_test',
                      sources : unit_test_src + ['tests/unit/cps.c'],
                      kwargs  : unit_test_opts)

linux_inputStream_test = executable('linux_inputStream_test',
                                    sources : unit_test_src + ['tests/unit/linux_inputStream_test.cpp'],
                                    kwargs  : unit_test_opts)

sine_test = executable('sine_test',
                      sources : unit_test_src + ['tests/unit/play_sine.c'],
                      kwargs  : unit_test_opts)

vp_test = executable('vp_test',
                      sources : unit_test_src + ['tests/unit/voice_prompts.c'],
                      kwargs  : unit_test_opts)

minmea_conversion_test = executable('minmea_conversion_test',
                                     sources : unit_test_src + ['tests/unit/convert_minmea_coord.c'],
                                     kwargs  : unit_test_opts)

test('M17 Golay Unit Test',   m17_golay_test)
test('M17 Viterbi Unit Test', m17_viterbi_test)
## test('M17 Demodulator Test',  m17_demodulator_test) # Skipped for now as this test no longer works after an M17 refactor
test('M17 RRC Test',          m17_rrc_test)
test('M17 Callsign Unit Test',          m17_callsign_test)
test('Codeplug Test',         cps_test)
## test('Linux InputStream Test', linux_inputStream_test)
## test('Sine Test',             sine_test)
## test('Voice Prompts Test',    vp_test) # Skipped for now as this test no longer works
test('minmea conversion Test', minmea_conversion_test)
