/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/stm32/desig.h>
#include <interfaces/platform.h>
#include <interfaces/gpio.h>
#include <interfaces/usb.h>
#include <hwconfig.h>
#include "cdc-acm_descriptors.h"
#include "usb_bsp.h"

/*
 * Circular buffer for incoming data enqueuement: each packet coming from host
 * is stored here, eventually erasing oldest data.
 * Buffer is statically allocated.
 */
#define RX_RING_BUF_SIZE 1024

struct rb
{
    uint8_t data[RX_RING_BUF_SIZE];
    size_t readPtr;
    size_t writePtr;
}
rxRingBuf;

char usb_serial_number[25];                         /* String for device serial number */
uint8_t usbd_control_buffer[128];                   /* Buffer for USB control endpoint */
const char * usb_strings[] = {"OpenRTX", "", ""};   /* USB decription strings          */

usbd_device *usbd_dev;                              /* USB device handle               */

/*
 * USB interrupt handler, name mangled according to C++ scheme
 */
void _Z17OTG_FS_IRQHandlerv(void)
{
    usbd_poll(usbd_dev);
}

enum usbd_request_return_codes cdcacm_control_request(usbd_device *usbd_dev,
    struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
    void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
    (void)complete;
    (void)buf;
    (void)usbd_dev;

    switch (req->bRequest)
    {
        case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
        /*
         * This Linux cdc_acm driver requires this to be implemented
         * even though it's optional in the CDC spec, and we don't
         * advertise it in the ACM functional descriptor.
         */
        return USBD_REQ_HANDLED;
        break;

        case USB_CDC_REQ_SET_LINE_CODING:
        if (*len < sizeof(struct usb_cdc_line_coding))
        {
            return USBD_REQ_NOTSUPP;
        }

        return USBD_REQ_HANDLED;
        break;
    }

    return USBD_REQ_NOTSUPP;
}

void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)ep;

    uint8_t buf[64];
    uint16_t len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

    /* Transfer data to RX ring buffer */
    for(size_t i = 0; i < len; i++)
    {
        rxRingBuf.data[rxRingBuf.writePtr] = buf[i];
        if((rxRingBuf.writePtr + 1) == rxRingBuf.readPtr)
        {
            /* Buffer full, pop one byte from tail. */
            rxRingBuf.readPtr = (rxRingBuf.readPtr + 1) % RX_RING_BUF_SIZE;
        }
        rxRingBuf.writePtr = (rxRingBuf.writePtr + 1) % RX_RING_BUF_SIZE;
    }
}

void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
    (void)wValue;

    usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
    usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

    usbd_register_control_callback(usbd_dev,
                                   USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
                                   USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                                   cdcacm_control_request);
}

void usb_init()
{
    usb_bsp_init();

    const hwInfo_t* hwinfo = platform_getHwInfo();
    desig_get_unique_id_as_string(usb_serial_number, sizeof(usb_serial_number));
    usb_strings[1] = hwinfo->name;
    usb_strings[2] = usb_serial_number;

    usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config,
            usb_strings, 3,
            usbd_control_buffer, sizeof(usbd_control_buffer));

    usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);
}

void usb_terminate()
{
    usb_bsp_terminate();
}

ssize_t usb_vcom_writeBlock(const void* buf, size_t len)
{
   uint16_t status = usbd_ep_write_packet(usbd_dev, 0x82, buf, len);
   if(status == 0) return -1;
   return ((ssize_t) status);
}

ssize_t usb_vcom_readBlock(void* buf, size_t len)
{
    uint8_t *b = ((uint8_t *) buf);
    size_t i;
    for(i = 0; i < len; i++)
    {
        /* Terminate if all data available has been popped out */
        if(rxRingBuf.readPtr == rxRingBuf.writePtr) break;
        b[i] = rxRingBuf.data[rxRingBuf.readPtr];
        rxRingBuf.readPtr = (rxRingBuf.readPtr + 1) % RX_RING_BUF_SIZE;
    }

    return i;
}

