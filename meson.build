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
fonts = ['openrtx/include/fonts']
inc = interfaces + rtos_inc + fonts

##
## Definitions
##
def = {'DONT_USE_CMSIS_INIT': ''}

##
## Platform specializations
##

## Linux
linux_src = src + ['openrtx/src/main.c',
                   'openrtx/src/bootstrap.c',
                   'openrtx/src/state.c',
                   'openrtx/src/ui.c',
                   'platform/drivers/display/display_libSDL.c',
                   'platform/mcu/x86_64/drivers/gpio.c',
                   'platform/mcu/x86_64/drivers/delays.c',
                   'platform/targets/linux/platform.c',
                   'openrtx/src/graphics/graphics_bw.c',
                   'rtos/uC-OS3/Ports/POSIX/os_cpu_c.c',
                   'rtos/uC-CPU/Posix/cpu_c.c']


linux_def = def + {'SCREEN_WIDTH': '160', 'SCREEN_HEIGHT': '128', 'PIX_FMT_BW': ''}

linux_inc = inc + ['rtos/uC-OS3/Ports/POSIX',
                   'rtos/uC-CPU/Posix',
                   'platform/targets/linux']

if not meson.is_cross_build()
  sdl_dep = dependency('SDL2')
  threads_dep = dependency('threads')
  linux_dep = [sdl_dep, threads_dep]
else
  linux_dep = []
endif


## TYT MD380
md380_src = src + ['openrtx/src/main.c',
                   'openrtx/src/bootstrap.c',
                   'openrtx/src/state.c',
                   'openrtx/src/ui.c',
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
                   'platform/mcu/CMSIS/Device/ST/STM32F4xx/Source/system_stm32f4xx.c',
                   'platform/drivers/display/HX83XX_MDxx380.c',
                   'platform/drivers/keyboard/keyboard_MDxx380.c',
                   'platform/targets/MD380/platform.c',
                   'platform/targets/MD380/adc1.c',
                   'openrtx/src/graphics/graphics_rgb565.c',
                   'rtos/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/os_cpu_c.c',
                   'rtos/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/os_cpu_a.s',
                   'rtos/uC-CPU/ARM-Cortex-M/ARMv7-M/cpu_c.c',
                   'rtos/uC-CPU/ARM-Cortex-M/ARMv7-M/cpu_a.s']

md380_def = def + {'STM32F40_41xxx': ''}

md380_inc = inc + ['platform/mcu/CMSIS/Include',
                   'platform/mcu/CMSIS/Device/ST/STM32F4xx/Include',
                   'platform/mcu/STM32F4xx',
                   'platform/mcu/STM32F4xx/drivers',
                   'platform/mcu/STM32F4xx/drivers/usb',
                   'platform/targets/MD380',
                   'rtos/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M',
                   'rtos/uC-CPU/ARM-Cortex-M/ARMv7-M']


## TYT MD-UV380
mduv380_src = src + ['openrtx/src/main.c',
                      'openrtx/src/bootstrap.c',
                      'openrtx/src/state.c',
                      'openrtx/src/ui.c',
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
                      'platform/mcu/CMSIS/Device/ST/STM32F4xx/Source/system_stm32f4xx.c',
                      'platform/drivers/display/HX83XX_MDxx380.c',
                      'platform/drivers/keyboard/keyboard_MDxx380.c',
                      'platform/targets/MD-UV380/platform.c',
                      'platform/targets/MD-UV380/adc1.c',
                      'openrtx/src/graphics/graphics_rgb565.c',
                      'rtos/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/os_cpu_c.c',
                      'rtos/uC-OS3/Ports/ARM-Cortex-M/ARMv7-M/os_cpu_a.s',
                      'rtos/uC-CPU/ARM-Cortex-M/ARMv7-M/cpu_c.c',
                      'rtos/uC-CPU/ARM-Cortex-M/ARMv7-M/cpu_a.s']

mduv380_def = def + {'STM32F40_41xxx': ''}

mduv380_inc = inc + ['platform/mcu/CMSIS/Include',
		      'platform/mcu/CMSIS/Device/ST/STM32F4xx/Include',
		      'platform/mcu/STM32F4xx',
		      'platform/mcu/STM32F4xx/drivers',
		      'platform/mcu/STM32F4xx/drivers/usb',
		      'platform/targets/MD-UV380',
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

mduv380_args = []
foreach k, v : mduv380_def
  if v == ''
    mduv380_args += '-D@0@'.format(k)
  else
    mduv380_args += '-D@0@=@1@'.format(k, v)
  endif
endforeach

linux_opts = {'sources': linux_src,
              'c_args': linux_args,
              'include_directories': linux_inc,
              'dependencies': linux_dep,
              'link_args' : '--entry=systemBootstrap'}

md380_opts = {'sources': md380_src,
              'c_args': md380_args,
              'include_directories': md380_inc}


mduv380_opts = {'sources': mduv380_src,
		'c_args': mduv380_args,
		'include_directories': mduv380_inc}

##
## Targets
##
targets = [

  {'name': 'linux',
   'opts': linux_opts,
   'flashable': false},

  {'name': 'md380',
   'opts': md380_opts,
   'flashable': true,
   'wrap': 'MD380',
   'load_addr': '0x0800C000'},

  {'name': 'md390',
   'opts': md380_opts,
   'flashable': true,
   'wrap': 'MD390',
   'load_addr': '0x0800C000'},

  {'name': 'mduv380g',
   'opts': mduv380_opts,
   'flashable': true,
   'wrap': 'UV3X0',
   'load_addr': '0x0800C000'},

  ]

objcopy = find_program('objcopy', required:false, disabler:true)
radio_tool = find_program('radio_tool', required:false, disabler:true)

foreach t : targets

  name = 'openrtx_'+t['name']
  exe = executable(name, kwargs:t['opts'])

  if t['flashable']

    bin = custom_target(name+'_bin',
      output : name+'_bin',
      input : exe,
      command : [objcopy, '-O', 'binary', '@INPUT@', '@OUTPUT@'])

      wrap = custom_target(name+'_wrap',
        output : name+'_wrap',
        input : bin,
        command : [radio_tool,
                   '--wrap',
                   '-o', '@OUTPUT@',
                   '-r', t['wrap'],
                   '-s', t['load_addr']+':@INPUT@'])

      custom_target(name+'_flash',
        input : wrap,
        output : name+'_flash',
        command : [radio_tool, '-d', '0', '-f', '-i', '@INPUT@'])
  endif
endforeach
