project('tinyusb', 'c')

#
# HACK
# This is a workaround for meson not propagating the include directories of the
# main project to the subprojects. To avoid double-inclusion we force the
# include paths needed to compile tinyusb and already present in the main
# program via GCC compile arguments.
#
add_project_arguments('-I../platform/mcu/CMSIS/Include', language : 'c')
add_project_arguments('-w', language : 'c')

tinyusb_inc = include_directories('openrtx', 'src', 'hw')

tinyusb_src = ['src/tusb.c',
               'src/device/usbd.c',
               'src/common/tusb_fifo.c',
               'src/device/usbd_control.c',
               'src/class/cdc/cdc_device.c']

driver = get_option('usb_driver')

if driver == 'auto'
  # Infer from host_machine.cpu() as a fallback. cpu() strings are not
  # standardised in Meson; set -Dtinyusb:usb_driver=<driver> explicitly
  # in the parent build or cross-file to avoid depending on this mapping.
  cpu = host_machine.cpu()
  if cpu == 'cortex-m7'
    driver = 'dwc2'
  elif cpu == 'cortex-m4'
    driver = 'stm32_fsdev'
  else
    warning('tinyusb: cannot infer USB driver from cpu \'' + cpu + '\'; '
          + 'set -Dtinyusb:usb_driver=<driver> explicitly. Defaulting to stm32_fsdev.')
    driver = 'stm32_fsdev'
  endif
endif

if driver == 'dwc2'
  # STM32H743: OTG FS via Synopsys DWC2 core
  add_project_arguments('-I../platform/mcu/CMSIS/Device/ST/STM32H7xx/Include', language : 'c')
  add_project_arguments('-DSTM32H743xx',        language : 'c')
  add_project_arguments('-DHSE_VALUE=25000000', language : 'c')
  tinyusb_src += ['src/portable/synopsys/dwc2/dcd_dwc2.c',
                  'src/portable/synopsys/dwc2/dwc2_common.c']
elif driver == 'stm32_fsdev'
  # STM32F405: OTG FS via ST full-speed device controller
  add_project_arguments('-I../platform/mcu/CMSIS/Device/ST/STM32F4xx/Include', language : 'c')
  add_project_arguments('-DSTM32F405xx',       language : 'c')
  add_project_arguments('-DHSE_VALUE=8000000', language : 'c')
  tinyusb_src += ['src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c']
endif

tinyusb = static_library('tinyusb',
                         tinyusb_src,
                         include_directories : tinyusb_inc,
                         install : false)

tinyusb_dep = declare_dependency(include_directories : tinyusb_inc,
                                 link_with : tinyusb)
