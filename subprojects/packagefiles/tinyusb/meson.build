project('tinyusb', 'c')

#
# HACK
# This is a workaround for meson not propagating the include directories of the
# main project to the subprojects. To avoid double-inclusion we force the
# include paths needed to compile tinyusb and already present in the main
# program via GCC compile arguments.
#
add_project_arguments('-I../platform/mcu/CMSIS/Device/ST/STM32F4xx/Include',
                      language : 'c')
add_project_arguments('-I../platform/mcu/CMSIS/Include', language : 'c')
add_project_arguments('-w', language : 'c')

tinyusb_inc = include_directories('openrtx', 'src', 'hw')

tinyusb_src = ['src/tusb.c',
               'src/device/usbd.c',
               'src/common/tusb_fifo.c',
               'src/device/usbd_control.c',
               'src/class/cdc/cdc_device.c',
               'src/portable/st/synopsys/dcd_synopsys.c',
               'src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c']

# Define CPU type and clock speed via GCC command line arguments               
add_project_arguments('-DSTM32F405xx',       language : 'c')
add_project_arguments('-DHSE_VALUE=8000000', language : 'c')



tinyusb = static_library('tinyusb',
                         tinyusb_src,
                         include_directories : tinyusb_inc,
                         install : false)

tinyusb_dep = declare_dependency(include_directories : tinyusb_inc,
                                 link_with : tinyusb)
