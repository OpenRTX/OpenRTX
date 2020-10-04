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
            'rtos/uC-OS3/Ports/POSIX/os_cpu_c.c',
            'rtos/uC-OS3/Cfg/os_app_hooks.c',
            'rtos/uC-CPU/cpu_core.c',
            'rtos/uC-CPU/Posix/cpu_c.c',
            'rtos/uC-LIB/lib_ascii.c',
            'rtos/uC-LIB/lib_math.c',
            'rtos/uC-LIB/lib_mem.c',
            'rtos/uC-LIB/lib_str.c']
src = rtos_src

##
## Headers
##
interfaces = ['openrtx/include/interfaces']
rtos_inc = ['rtos/uC-OS3/Source', 'rtos/uC-OS3/Ports/POSIX', 'rtos/uC-OS3/Cfg',
            'rtos/uC-CPU/Posix',  'rtos/uC-CPU', 'rtos/uC-CPU/Cfg',             \
            'rtos/uC-LIB', 'rtos/uC-LIB/Cfg']
inc = interfaces + rtos_inc

##
## Definitions
##
def = {'DONT_USE_CMSIS_INIT': ''}

##
## Dependencies
##
sdl_dep = dependency('SDL2')
threads_dep = dependency('threads')
dep = [sdl_dep, threads_dep]

##
## Platform specialization
##
linux_src = src + ['tests/platform/x64_uC.c',
                   'platform/drivers/display/display_libSDL.c']
linux_def = def + {'SCREEN_WIDTH': 160, 'SCREEN_HEIGHT': 128}

##
## Compute define string
##
linux_args = []
foreach k, v : linux_def
  linux_args += '-D@0@=@1@'.format(k, v)
endforeach

linux = {'sources': linux_src,
         'c_args': linux_args,
         'include_directories': inc,
         'dependencies': dep}

##
## Select your radio model among the supported ones: MD-380, MD-UV380
##
executable('openrtx-linux', kwargs:linux)
