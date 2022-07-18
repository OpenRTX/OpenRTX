##
## OpenRTX - Modular Open Source Radio Firmware
##
project('OpenRTX', ['c', 'cpp'],
  version : '0.3.3',
  default_options : ['warning_level=3', 'b_staticpic=false'])

##
## Optional defines, common to all souces (e.g. to enable debugging)
##

def = {}

##
## ----------------- Platform-independent source files -------------------------
##

##
## OpenRTX sources
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
               'openrtx/src/core/data_conversion.c',
               'openrtx/src/core/memory_profiling.cpp',
               'openrtx/src/ui/ui.c',
               'openrtx/src/ui/ui_main.c',
               'openrtx/src/ui/ui_mode.c',
               'openrtx/src/ui/ui_menu.c',
               'openrtx/src/rtx/rtx.cpp',
               'openrtx/src/rtx/OpMode_FM.cpp',
               'openrtx/src/rtx/OpMode_M17.cpp',
               'openrtx/src/protocols/M17/M17DSP.cpp',
               'openrtx/src/protocols/M17/M17Golay.cpp',
               'openrtx/src/protocols/M17/M17Callsign.cpp',
               'openrtx/src/protocols/M17/M17Modulator.cpp',
               'openrtx/src/protocols/M17/M17Demodulator.cpp',
               'openrtx/src/protocols/M17/M17FrameEncoder.cpp',
               'openrtx/src/protocols/M17/M17FrameDecoder.cpp',
               'openrtx/src/protocols/M17/M17Transmitter.cpp',
               'openrtx/src/protocols/M17/M17LinkSetupFrame.cpp']

openrtx_inc = ['openrtx/include',
               'openrtx/include/rtx',
               'openrtx/include/core',
               'openrtx/include/calibration',
               'openrtx/include/protocols',
               'openrtx/include/fonts/adafruit',
               'platform/drivers/ADC',
               'platform/drivers/NVM',
               'platform/drivers/GPS',
               'platform/drivers/USB',
               'platform/drivers/tones',
               'platform/drivers/baseband',
               'platform/drivers/backlight',
               'platform/drivers/chSelector']

main_src = []

# Add to sources either the main executable or a platform test
if get_option('test') != ''
  main_src += 'tests/platform/'+get_option('test')+'.c'
  def      += {'RUNNING_TESTSUITE' : ''}
else
  main_src += 'openrtx/src/main.c'
endif

##
## External libraries
##

# minmea, a lightweight GPS NMEA 0183 parser library
minmea_src = ['lib/minmea/minmea.c']
minmea_inc = ['lib/minmea/include']

# QDEC, a very simple, header only, quadrature decoding library
qdec_inc = ['lib/qdec/include']

# CODEC2, open source speech codec
# Compile from sources only when targeting a platform different from x86/x64
if meson.is_cross_build()
  codec2_proj = subproject('codec2')
  codec2_dep  = codec2_proj.get_variable('codec2_dep')
else
  codec2_dep  = dependency('codec2')
endif

# tinyusb, open source cross-platform USB stack for embedded system.
# Subproject included only for embedded targets
if meson.is_cross_build()
  tinyusb_proj = subproject('tinyusb')
  tinyusb_dep  = tinyusb_proj.get_variable('tinyusb_dep')
else
  tinyusb_dep = [ ]
endif

##
## RTOS
##
rtos_inc = ['lib/miosix-kernel',
            'lib/miosix-kernel/arch',
            'lib/miosix-kernel/arch/cortexM4_stm32f4/common',
            'lib/miosix-kernel/arch/cortexM4_stm32f4/M4_openrtx',
            'lib/miosix-kernel/config/arch/cortexM4_stm32f4/M4_openrtx']

src = openrtx_src + main_src + minmea_src
inc = openrtx_inc + minmea_inc + qdec_inc

# Include RTOS only for embedded targets
if meson.is_cross_build()
  inc = inc + rtos_inc
  def = def + {'DONT_USE_CMSIS_INIT': ''}
endif

## Add git commit or tag to print in OpenRTX
r = run_command('git', 'describe', '--tags', '--dirty')
if r.returncode() != 0
  # it failed
endif
git_version = '"'+r.stdout().strip()+'"'
def = def + {'GIT_VERSION': git_version}

##
## --------------------- Family-dependent source files -------------------------
##

##
## TYT MDx family
##

mdx_src = ['openrtx/src/core/xmodem.c',
           'openrtx/src/core/backup.c',
           'platform/drivers/ADC/ADC1_MDx.c',
           'platform/drivers/GPS/GPS_MDx.cpp',
           'platform/drivers/NVM/W25Qx.c',
           'platform/drivers/NVM/nvmem_settings_MDx.c',
           'platform/drivers/audio/audio_MDx.c',
           'platform/drivers/audio/inputStream_MDx.cpp',
           'platform/drivers/audio/outputStream_MDx.cpp',
           'platform/drivers/baseband/HR_Cx000.cpp',
           'platform/drivers/backlight/backlight_MDx.c',
           'platform/drivers/tones/toneGenerator_MDx.cpp',
           'platform/drivers/USB/usb_MDx.cpp',
           'platform/drivers/USB/usb_descriptors.c']

##
## GDx family: Radioddity GD-77 and Baofeng DM-1801
##

gdx_src = ['openrtx/src/core/xmodem.c',
           'openrtx/src/core/backup.c',
           'platform/drivers/NVM/W25Qx.c',
           'platform/drivers/NVM/AT24Cx_GDx.c',
           'platform/drivers/NVM/spiFlash_GDx.c',
           'platform/drivers/NVM/nvmem_GDx.c',
           'platform/drivers/CPS/cps_io_native_GDx.c',
           'platform/drivers/ADC/ADC0_GDx.c',
           'platform/drivers/backlight/backlight_GDx.c',
           'platform/drivers/baseband/radio_GDx.cpp',
           'platform/drivers/baseband/HR_Cx000.cpp',
           'platform/drivers/baseband/AT1846S_GDx.cpp',
           'platform/drivers/baseband/HR_C6000_GDx.cpp',
           'platform/drivers/display/UC1701_GDx.c',
           'platform/drivers/keyboard/keyboard_GDx.c',
           'platform/drivers/audio/audio_GDx.c',
           'platform/drivers/audio/inputStream_GDx.c',
           'platform/drivers/audio/outputStream_GDx.c']

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
                 'platform/mcu/STM32F4xx/drivers/gpio.c',
                 'platform/mcu/STM32F4xx/drivers/usb_vcom.c',
                 'platform/mcu/STM32F4xx/drivers/delays.cpp',
                 'platform/mcu/STM32F4xx/drivers/rtc.c',
                 'platform/mcu/STM32F4xx/drivers/SPI2.c',
                 'platform/mcu/STM32F4xx/drivers/USART3.cpp',
                 'platform/mcu/STM32F4xx/drivers/flash.c',
                 'platform/mcu/CMSIS/Device/ST/STM32F4xx/Source/system_stm32f4xx.c']

stm32f405_inc = ['platform/mcu/CMSIS/Include',
                 'platform/mcu/CMSIS/Device/ST/STM32F4xx/Include',
                 'platform/mcu/STM32F4xx',
                 'platform/mcu/STM32F4xx/drivers',
                 'platform/mcu/STM32F4xx/drivers/usb']

stm32f405_def = {'STM32F405xx': '', 'HSE_VALUE':'8000000',
                 '_POSIX_PRIORITY_SCHEDULING':''}

##
## MK22FN512
##

mk22fn512_src = ['platform/mcu/MK22FN512xxx12/boot/startup.cpp',
                 'platform/mcu/MK22FN512xxx12/boot/bsp.cpp',
                 'platform/mcu/MK22FN512xxx12/boot/libc_integration.cpp',
                 'platform/mcu/MK22FN512xxx12/drivers/gpio.c',
                 'platform/mcu/MK22FN512xxx12/drivers/delays.cpp',
                 'platform/mcu/MK22FN512xxx12/drivers/I2C0.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_cdc_acm.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_ch9.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_dci.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_descriptor.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_device_khci.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb/usb_osa_bm.c',
                 'platform/mcu/MK22FN512xxx12/drivers/usb_vcom.c',
                 'platform/mcu/CMSIS/Device/NXP/MK22FN512xxx12/Source/system_MK22F51212.c']

mk22fn512_inc = ['platform/mcu/CMSIS/Include',
                 'platform/mcu/CMSIS/Device/NXP/MK22FN512xxx12/Include',
                 'platform/mcu/MK22FN512xxx12/drivers']

mk22fn512_def = {'_POSIX_PRIORITY_SCHEDULING':''}

##
## ----------------------- Platform specializations ----------------------------
##

##
## Linux
##
linux_platform_src = ['platform/targets/linux/emulator/emulator.c',
                      'platform/targets/linux/emulator/sdl_engine.c',
                      'platform/drivers/display/display_libSDL.c',
                      'platform/drivers/keyboard/keyboard_linux.c',
                      'platform/drivers/NVM/nvmem_linux.c',
                      'platform/drivers/GPS/GPS_linux.c',
                      'platform/mcu/x86_64/drivers/gpio.c',
                      'platform/mcu/x86_64/drivers/delays.c',
                      'platform/mcu/x86_64/drivers/rtc.c',
                      'platform/drivers/baseband/radio_linux.cpp',
                      'platform/drivers/audio/audio_linux.cpp',
                      'platform/drivers/audio/inputStream_linux.cpp',
                      'platform/drivers/audio/outputStream_linux.c',
                      'platform/targets/linux/platform.c',
                      'platform/drivers/CPS/cps_io_libc.c']

linux_src = src + linux_platform_src


# GDx family display emulation
#linux_def = def + {'SCREEN_WIDTH': '128', 'SCREEN_HEIGHT': '64', 'PIX_FMT_BW': ''}
# MDx family display emulation
linux_def = def + {'SCREEN_WIDTH': '160', 'SCREEN_HEIGHT': '128', 'PIX_FMT_RGB565': ''}

linux_inc = inc + ['platform/targets/linux',
                   'platform/targets/linux/emulator']

if not meson.is_cross_build()
  sdl_dep = dependency('SDL2')
  threads_dep = dependency('threads')
  linux_dep = [sdl_dep, threads_dep, codec2_dep]
else
  linux_dep = [ ]
endif

##
## TYT MD-3x0 family
##
md3x0_src = src + mdx_src + stm32f405_src + ['platform/drivers/NVM/nvmem_MD3x0.c',
                                             'platform/drivers/CPS/cps_io_native_MD3x0.c',
                                             'platform/drivers/NVM/spiFlash_MD3x.c',
                                             'platform/drivers/baseband/SKY72310.c',
                                             'platform/drivers/baseband/radio_MD3x0.cpp',
                                             'platform/drivers/baseband/HR_C5000_MDx.cpp',
                                             'platform/drivers/keyboard/keyboard_MD3x.c',
                                             'platform/drivers/display/HX8353_MD3x.cpp',
                                             'platform/targets/MD-3x0/platform.c']

md3x0_inc = inc + stm32f405_inc + ['platform/targets/MD-3x0']
md3x0_def = def + stm32f405_def + {'PLATFORM_MD3x0': '', 'timegm': 'mktime'}

##
## TYT MD-UV380
##
mduv3x0_src = src + mdx_src + stm32f405_src + ['platform/drivers/NVM/nvmem_MDUV3x0.c',
                                               'platform/drivers/NVM/spiFlash_MD3x.c',
                                               'platform/drivers/CPS/cps_io_native_MDUV3x0.c',
                                               'platform/targets/MD-UV3x0/platform.c',
                                               'platform/drivers/keyboard/keyboard_MD3x.c',
                                               'platform/drivers/display/HX8353_MD3x.cpp',
                                               'platform/drivers/chSelector/chSelector_UV3x0.c',
                                               'platform/drivers/baseband/radio_UV3x0.cpp',
                                               'platform/drivers/baseband/AT1846S_UV3x0.cpp',
                                               'platform/drivers/baseband/HR_C6000_UV3x0.cpp']

mduv3x0_inc = inc + stm32f405_inc + ['platform/targets/MD-UV3x0']
mduv3x0_def = def + stm32f405_def + {'PLATFORM_MDUV3x0': '', 'timegm': 'mktime'}

##
## TYT MD-9600
##
md9600_src = src + mdx_src + stm32f405_src + ['platform/targets/MD-9600/platform.c',
                                              'platform/drivers/display/ST7567_MD9600.c',
                                              'platform/drivers/keyboard/keyboard_MD9600.c',
                                              'platform/drivers/chSelector/chSelector_MD9600.c',
                                              'platform/drivers/baseband/radio_MD9600.cpp',
                                              'platform/drivers/NVM/nvmem_MD9600.c',
                                              'platform/drivers/NVM/spiFlash_MD9600.c',
                                              'platform/drivers/CPS/cps_io_native_MD9600.c']

md9600_inc = inc + stm32f405_inc + ['platform/targets/MD-9600']
md9600_def = def + stm32f405_def + {'PLATFORM_MD9600': ''}

##
## Radioddity GD-77
##
gd77_src = src + gdx_src + mk22fn512_src + ['platform/targets/GD-77/platform.c']

gd77_inc = inc + mk22fn512_inc + ['platform/targets/GD-77']
gd77_def = def + mk22fn512_def + {'PLATFORM_GD77': ''}

##
## Baofeng DM-1801
##
dm1801_src = src + gdx_src + mk22fn512_src + ['platform/targets/DM-1801/platform.c']

dm1801_inc = inc + mk22fn512_inc + ['platform/targets/DM-1801']
dm1801_def = def + mk22fn512_def + {'PLATFORM_DM1801': ''}

##
## Module 17
##
mod17_src = src + stm32f405_src + ['platform/targets/Module17/platform.c',
                                   'platform/drivers/display/SH110x_Mod17.c',
                                   'platform/drivers/ADC/ADC1_Mod17.c',
                                   'platform/drivers/keyboard/keyboard_Mod17.c',
                                   'platform/drivers/NVM/nvmem_Mod17.c',
                                   'platform/drivers/CPS/cps_io_native_Mod17.c',
                                   'platform/drivers/baseband/radio_Mod17.cpp',
                                   'platform/drivers/audio/inputStream_Mod17.cpp',
                                   'platform/drivers/audio/outputStream_Mod17.cpp',
                                   'platform/drivers/audio/audio_Mod17.c',
                                   'platform/drivers/baseband/MCP4551_Mod17.cpp']

mod17_inc = inc + stm32f405_inc + ['platform/targets/Module17']
mod17_def = def + stm32f405_def + {'PLATFORM_MOD17': ''}

##
## -------------------------- Compilation arguments ----------------------------
##

linux_c_args   = ['-DPLATFORM_LINUX']
linux_cpp_args = ['-std=c++14', '-DPLATFORM_LINUX']
linux_l_args   = ['-lm', '-lreadline']

# Add AddressSanitizer if required
if get_option('asan')
  linux_c_args += '-fsanitize=address'
  linux_cpp_args += '-fsanitize=address'
  linux_l_args += '-fsanitize=address'
endif
# Add Undefined Behaviour Sanitizer if required
if get_option('ubsan')
  linux_c_args += '-fsanitize=undefined'
  linux_cpp_args += '-fsanitize=undefined'
  linux_l_args += '-fsanitize=undefined'
endif

foreach k, v : linux_def
  if v == ''
    linux_c_args += '-D@0@'.format(k)
    linux_cpp_args += '-D@0@'.format(k)
  else
    linux_c_args += '-D@0@=@1@'.format(k, v)
    linux_cpp_args += '-D@0@=@1@'.format(k, v)
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

linux_opts = {'sources': linux_src,
              'c_args': linux_c_args,
              'cpp_args' : linux_cpp_args,
              'include_directories': linux_inc,
              'dependencies': linux_dep,
              'link_args' : linux_l_args}

md3x0_opts = {'sources' : md3x0_src,
              'c_args'  : md3x0_args,
              'cpp_args': md3x0_args,
              'link_args' : ['-Wl,-T../platform/mcu/STM32F4xx/linker_script_MDx.ld',
                             '-Wl,--print-memory-usage'],
              'dependencies': [codec2_dep, tinyusb_dep],
              'include_directories': md3x0_inc}

mduv3x0_opts = {'sources': mduv3x0_src,
                'c_args': mduv3x0_args,
                'cpp_args': mduv3x0_args,
                'link_args' : ['-Wl,-T../platform/mcu/STM32F4xx/linker_script_MDx.ld',
                               '-Wl,--print-memory-usage'],
                'dependencies': [codec2_dep, tinyusb_dep],
                'include_directories': mduv3x0_inc}

gd77_opts = {'sources': gd77_src,
             'c_args': gd77_args,
             'cpp_args': gd77_args,
             'link_args' : ['-Wl,-T../platform/mcu/MK22FN512xxx12/linker_script.ld',
                            '-Wl,--print-memory-usage'],
             'dependencies': [codec2_dep],
             'include_directories':gd77_inc}

dm1801_opts = {'sources': dm1801_src,
               'c_args': dm1801_args,
               'cpp_args': dm1801_args,
               'link_args' : ['-Wl,-T../platform/mcu/MK22FN512xxx12/linker_script.ld',
                              '-Wl,--print-memory-usage'],
               'dependencies': [codec2_dep],
               'include_directories':dm1801_inc}

md9600_opts = {'sources': md9600_src,
               'c_args': md9600_args,
               'cpp_args': md9600_args,
               'link_args' : ['-Wl,-T../platform/mcu/STM32F4xx/linker_script_MDx.ld',
                              '-Wl,--print-memory-usage'],
               'dependencies': [codec2_dep, tinyusb_dep],
               'include_directories': md9600_inc}

mod17_opts  = {'sources': mod17_src,
               'c_args': mod17_args,
               'cpp_args': mod17_args,
               'link_args' : ['-Wl,-T../platform/mcu/STM32F4xx/linker_script_Mod17.ld',
                              '-Wl,--print-memory-usage'],
               'dependencies': [codec2_dep, tinyusb_dep],
               'include_directories': mod17_inc}

##
## ---------------------------- Compilation targets ----------------------------
##

targets = [

  {'name': 'linux',
   'opts': linux_opts,
   'flashable': false},

  {'name': 'md3x0',
   'opts': md3x0_opts,
   'flashable': true,
   'wrap': 'MD380',
   'load_addr': '0x0800C000'},

  {'name': 'mduv3x0',
   'opts': mduv3x0_opts,
   'flashable': true,
   'wrap': 'UV3X0',
   'load_addr': '0x0800C000'},

  {'name': 'gd77',
   'opts': gd77_opts,
   'flashable': true,
   'wrap': 'UV3X0',
   'load_addr': '0x0800C000'},

  {'name': 'dm1801',
   'opts': dm1801_opts,
   'flashable': true,
   'wrap': 'UV3X0',
   'load_addr': '0x0800C000'},

  {'name': 'md9600',
   'opts': md9600_opts,
   'flashable': true,
   'wrap': 'MD9600',
   'load_addr': '0x0800C000'},

   {'name': 'mod17',
   'opts': mod17_opts,
   'flashable': true,
   'wrap': ' ',
   'load_addr': ' '},
  ]

radio_tool  = find_program('radio_tool')
objcopy     = find_program('objcopy',                          required:false, disabler:true)
if build_machine.system() == 'linux'
  bin2sgl     = find_program('scripts/bin2sgl.Linux',          required:false, disabler:true)
elif build_machine.system() == 'windows'
  bin2sgl     = find_program('scripts/bin2sgl.exe',            required:false, disabler:true)
endif
gd77_loader = find_program('scripts/gd-77_firmware_loader.py', required:false, disabler:true)
dfu_util    = find_program('dfu-util',                         required:false, disabler:true)

foreach t : targets

  name = 'openrtx_'+t['name']
  exe  = executable(name, kwargs:t['opts'])

  if t['flashable']

    bin = custom_target(name+'_bin',
                        output  : name+'_bin',
                        input   : exe,
                        command : [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@'])

    # Handle GD77 custom wrapping and flashing tools
    if name == 'openrtx_gd77'

      if build_machine.system() == 'linux'
            wrap = custom_target(name+'_wrap',
                                 output  : name+'_wrap.sgl',
                                 input   : bin,
                                 command : [bin2sgl, '-f', '@INPUT@', '&&',
                                            'mv', '@INPUT@.sgl', '@OUTPUT@'])
      elif build_machine.system() == 'windows'
            wrap = custom_target(name+'_wrap',
                                 output  : name+'_bin.sgl',
                                 input   : bin,
                                 command : [bin2sgl, '@INPUT@'])
      endif

      custom_target(name+'_flash',
                    input   : wrap,
                    output  : name+'_flash',
                    command : [gd77_loader, '-f', '@INPUT@'])

     # Handle DM1801 custom wrapping and flashing tools
    elif name == 'openrtx_dm1801'

      if build_machine.system() == 'linux'
            wrap = custom_target(name+'_wrap',
                                 output  : name+'_wrap.sgl',
                                 input   : bin,
                                 command : [bin2sgl, '-f', '@INPUT@',
                                            '-m', 'DM-1801', '&&', 'mv',
                                            '@INPUT@.sgl', '@OUTPUT@'])
      elif build_machine.system() == 'windows'
            wrap = custom_target(name+'_wrap',
                                 output  : name+'_bin.sgl',
                                 input   : bin,
                                 command : [bin2sgl, '@INPUT@', '-m DM-1801'])
      endif

      custom_target(name+'_flash',
                    input   : wrap,
                    output  : name+'_flash',
                    command : [gd77_loader, '-f', '@INPUT@', '-m', 'DM-1801',])

    # Module17 also uses dfu-tool for flashing
    elif name == 'openrtx_mod17'

      custom_target(name+'_flash',
                    input   : bin,
                    output  : name+'_flash',
                    command : [dfu_util, '-d', '0483:df11', '-a', '0',
                               '-D', '@INPUT@', '-s', '0x08000000'])

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
endforeach

##
## ----------------------------------- Unit Tests ------------------------------
##

unit_test_opts = {'c_args'             : linux_c_args,
                  'cpp_args'           : linux_cpp_args,
                  'include_directories': linux_inc,
                  'dependencies'       : linux_dep,
                  'link_args'          : linux_l_args}
unit_test_src = openrtx_src + minmea_src + linux_platform_src

m17_golay_test = executable('m17_golay_test',
                            sources : unit_test_src + ['tests/unit/M17_golay.cpp'],
                            kwargs  : unit_test_opts)

m17_viterbi_test = executable('m17_viterbi_test',
                               sources : unit_test_src + ['tests/unit/M17_viterbi.cpp'],
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

test('M17 Golay Unit Test',   m17_golay_test)
test('M17 Viterbi Unit Test', m17_viterbi_test)
test('M17 Demodulator Test',  m17_demodulator_test)
test('M17 RRC Test',          m17_rrc_test)
test('Codeplug Test',         cps_test)
test('Linux InputStream Test', linux_inputStream_test)
