##
## OpenRTX - Modular Open Source Radio Firmware
##
project('OpenRTX', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

##
## Here are listed all the platform independent source files
##
rtos_src = ['rtos/uC-OS3/Source/__dbg_uCOS-III.c',
            'rtos/uC-OS3/Source/os_cfg_app.c',
            'rtos/uC-OS3/Source/os_core.c',
            'rtos/uC-OS3/Source/os_dbg.c',
            'rtos/uC-OS3/Source/os_flag.c',
            'rtos/uC-OS3/Source/os_mem.c',
            'rtos/uC-OS3/Source/os_msg.c',
            'rtos/uC-OS3/Source/os_mutex.c',
            'rtos/uC-OS3/Source/os_prio.c',
            'rtos/uC-OS3/Source/os_q.c',
            'rtos/uC-OS3/Source/os_sem.c',
            'rtos/uC-OS3/Source/os_stat.c',
            'rtos/uC-OS3/Source/os_task.c',
            'rtos/uC-OS3/Source/os_tick.c',
            'rtos/uC-OS3/Source/os_time.c',
            'rtos/uC-OS3/Source/os_tmr.c',
            'rtos/uC-OS3/Source/os_var.c',
            'rtos/uC-OS3/Cfg/os_app_hooks.c',
            'rtos/uC-CPU/cpu_core.c',
            'rtos/uC-LIB/lib_ascii.c',
            'rtos/uC-LIB/lib_math.c',
            'rtos/uC-LIB/lib_mem.c',
            'rtos/uC-LIB/lib_str.c']
src = rtos_src

##
## Includes
##
interfaces = ['openrtx/include/interfaces']
rtos_inc = ['rtos/uC-OS3/Source',
            'rtos/uC-OS3/Cfg',
            'rtos/uC-CPU',
            'rtos/uC-CPU/Cfg',
            'rtos/uC-LIB',
            'rtos/uC-LIB/Cfg']
inc = interfaces + rtos_inc

##
## Definitions
##
def = {'DONT_USE_CMSIS_INIT': ''}

##
## Platform specializations
##

## Linux
linux_src = src + ['tests/platform/x64_uC.c',
                   'platform/drivers/display/display_libSDL.c',
                   'openrtx/src/graphics/graphics_rgb565.c',
                   'rtos/uC-OS3/Ports/POSIX/os_cpu_c.c',
                   'rtos/uC-CPU/Posix/cpu_c.c']


linux_def = def + {'SCREEN_WIDTH': '160', 'SCREEN_HEIGHT': '128', 'PIX_FMT_RGB565': ''}

linux_inc = inc + ['rtos/uC-OS3/Ports/POSIX',
                   'rtos/uC-CPU/Posix']

if not meson.is_cross_build()
  sdl_dep = dependency('SDL2')
  threads_dep = dependency('threads')
  linux_dep = [sdl_dep, threads_dep]
else
  linux_dep = []
endif


## TYT MD380
md380_src = src + ['tests/platform/stm32f4_uC.c',
                   'platform/mcu/STM32F4xx/boot/startup.c',
                   'platform/mcu/STM32F4xx/boot/libc_integration.c',
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
                   'platform/mcu/STM32F4xx/drivers/delays.c',
                   'platform/mcu/STM32F4xx/drivers/gpio.c',
                   'platform/mcu/CMSIS/Device/ST/STM32F4xx/Source/system_stm32f4xx.c',
                   'platform/drivers/display/HX83XX_md380.c',
                   'openrtx/src/graphics/graphics_rgb565.c',
                   'rtos/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/os_cpu_c.c',
                   'rtos/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/os_cpu_a.s',
                   'rtos/uC-CPU/ARM-Cortex-M/ARMv7-M/cpu_c.c',
                   'rtos/uC-CPU/ARM-Cortex-M/ARMv7-M/cpu_a.s']

md380_def = def + {'SCREEN_WIDTH': '160', 'SCREEN_HEIGHT': '128', 'STM32F40_41xxx': ''}

md380_inc = inc + ['platform/mcu/CMSIS/Include',
                   'platform/mcu/CMSIS/Device/ST/STM32F4xx/Include',
                   'platform/mcu/STM32F4xx',
                   'platform/mcu/STM32F4xx/drivers/usb',
                   'rtos/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M',
                   'rtos/uC-CPU/ARM-Cortex-M/ARMv7-M']

##
## Compilation defines
##
linux_args = []
foreach k, v : linux_def
  if v == ''
    linux_args += '-D@0@'.format(k)
  else
    linux_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

md380_args = []
foreach k, v : md380_def
  if v == ''
    md380_args += '-D@0@'.format(k)
  else
    md380_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

linux = {'sources': linux_src,
         'c_args': linux_args,
         'include_directories': linux_inc,
         'dependencies': linux_dep}

md380 = {'sources': md380_src,
         'c_args': md380_args,
         'include_directories': md380_inc}

##
## Select your radio model among the supported ones: MD-380, MD-UV380
##
executable('openrtx-linux', kwargs:linux)
executable('openrtx-md380', kwargs:md380)
executable('openrtx-mduv380', kwargs:md380)
