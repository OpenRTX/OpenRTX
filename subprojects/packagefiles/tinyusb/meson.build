# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later

project('tinyusb', 'c')

# Source lists only. Each MCU that uses TinyUSB builds them in the main
# project with its own device controller driver and MCU defines, so radios
# with different MCUs can share one build directory.

tinyusb_inc = include_directories('src')

# The device stack and the CDC-ACM class
tinyusb_device_src = files('src/tusb.c',
                           'src/common/tusb_fifo.c',
                           'src/device/usbd.c',
                           'src/device/usbd_control.c',
                           'src/class/cdc/cdc_device.c')

# Synopsys DWC2 controller: the OTG FS port of the STM32F4 and STM32H7
tinyusb_dwc2_src = files('src/portable/synopsys/dwc2/dcd_dwc2.c',
                         'src/portable/synopsys/dwc2/dwc2_common.c')
