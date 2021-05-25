project('libopencm3', 'c')

libopencm3_inc = include_directories('include')
libopencm3_src = ['lib/usb/usb.c',
                  'lib/usb/usb_control.c',
                  'lib/usb/usb_dwc_common.c',
                  'lib/usb/usb_f107.c',
                  'lib/usb/usb_standard.c',
                  'lib/stm32/common/desig_common_all.c',
                  'lib/stm32/common/desig_common_v1.c',
                  'lib/stm32/common/rcc_common_all.c']

# Configure libopencm3 for STM32F4, TODO: add different options for other MCUs if needed
add_project_arguments('-DSTM32F4', language : 'c')

libopencm3 = static_library('libopencm3',
                            libopencm3_src,
                            include_directories : libopencm3_inc,
                            install : false)

libopencm3_dep = declare_dependency(include_directories : libopencm3_inc,
                                    link_with : libopencm3)
