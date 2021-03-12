/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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
#include <libopencm3/usb/dfu.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <hwconfig.h>
#include "W25Qx.h"

#define NULL 0

#define DFU_INTERFACE_NUMBER 0
#define CONFIGURATION_VALUE 1
// FIXME: inset a proper address here
#define SPI_FLASH_BASE 0x0
#define SPI_FLASH_SIZE 0x1000000
#define BLOCK_SIZE 512

static uint32_t erased_sectors = 0;

static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_DFU,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x0483,
    .idProduct = 0xdf11,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x83,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 16,
    .bInterval = 255,
} };

static const struct usb_endpoint_descriptor data_endp[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x01,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x82,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
} };

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];
uint8_t usbd_remote_wakeup_enabled = 0;

const struct usb_dfu_descriptor dfu_function = {
    .bLength = sizeof(struct usb_dfu_descriptor),
    .bDescriptorType = DFU_FUNCTIONAL,
    .bmAttributes = (
        // download capable (bitCanDnload)
        USB_DFU_CAN_DOWNLOAD |
        // upload capable (bitCanUpload)
        USB_DFU_CAN_UPLOAD |
        // device is able to communicate via USB after Manifestation phase.
        // (bitManifestationTolerant)
        USB_DFU_MANIFEST_TOLERANT
        // Device will perform a bus detach-attach sequence when it receives
        // a DFU_DETACH request. The host must not issue a USB Reset.
        // (bitWillDetach)
        // USB_DFU_WILL_DETACH
    ),
    // Time, in milliseconds, that the device will wait after receipt of the
    // DFU_DETACH request. If this time elapses without a USB reset, then the
    // device will terminate the Reconfiguration phase and revert back to
    // normal operation. This represents the maximum time that the device can
    // wait (depending on its timers, etc.). The host may specify a shorter
    // timeout in the DFU_DETACH request.
    .wDetachTimeout = 0,
    // Maximum number of bytes that the device can accept per control-write
    // transaction.
    .wTransferSize = BLOCK_SIZE,
    // Numeric expression identifying the version of the DFU Specification
    // release.
    .bcdDFUVersion = 0x0110,
};

const struct usb_interface_descriptor dfu_interface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = DFU_INTERFACE_NUMBER,
    // Alternate setting. Must be zero.
    .bAlternateSetting = 0,
    // Only the control pipe is used.
    .bNumEndpoints = 0,
    // DFU
    .bInterfaceClass = USB_CLASS_DFU,
    // Device Firmware Upgrade Code
    .bInterfaceSubClass = 1,
    // DFU mode protocol.
    .bInterfaceProtocol = 2,
    // Index of string descriptor for this interface
    .iInterface = 4,

    .extra = &dfu_function,
    .extralen = sizeof(dfu_function),
};

const struct usb_interface ifaces[] = {
    {
    .num_altsetting = 1,
    .altsetting = &dfu_interface,
    }
};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = USB_CONFIG_ATTR_DEFAULT |
                    USB_CONFIG_ATTR_SELF_POWERED |
                    USB_CONFIG_ATTR_REMOTE_WAKEUP,
    .bMaxPower = 0x32,

    .interface = ifaces,
};

static const char * usb_strings[] = {
    "OpenRTX",
    "",
    "",
    "DFU 1.1 @SPI Flash Memory",
};

char usb_serial_number[25];

static enum usbd_request_return_codes control_device_get_status_callback(
    usbd_device *dev,
    struct usb_setup_data *req,
    uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *, struct usb_setup_data * )
) {
    (void)dev;
    (void)complete;

    // 9.4.5 Get Status
    if(req->bRequest == USB_REQ_GET_STATUS) {
        *len = 2;
        (*buf)[0] = USB_DEV_STATUS_SELF_POWERED;
        if(usbd_remote_wakeup_enabled)
            (*buf)[0] |= USB_DEV_STATUS_REMOTE_WAKEUP;
        (*buf)[1] = 0;
        return USBD_REQ_HANDLED;
    }

    return USBD_REQ_NEXT_CALLBACK;
}

static enum usbd_request_return_codes control_device_feature_callback(
    usbd_device *dev,
    struct usb_setup_data *req,
    uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *, struct usb_setup_data * )
) {
    (void)dev;
    (void)buf;
    (void)len;
    (void)complete;

    // 9.4.1 Clear Feature
    if(req->bRequest == USB_REQ_CLEAR_FEATURE) {
        switch (req->wValue) {
            // case USB_FEAT_ENDPOINT_HALT:
            //  return USBD_REQ_HANDLED;
            case USB_FEAT_DEVICE_REMOTE_WAKEUP:
                usbd_remote_wakeup_enabled = 0;
                return USBD_REQ_HANDLED;
            // case USB_FEAT_TEST_MODE:
            //  return USBD_REQ_HANDLED;
        }
    }

    // 9.4.9 Set Feature
    if(req->bRequest == USB_REQ_SET_FEATURE) {
        switch (req->wValue) {
            // case USB_FEAT_ENDPOINT_HALT:
            //  return USBD_REQ_HANDLED;
            case USB_FEAT_DEVICE_REMOTE_WAKEUP:
                usbd_remote_wakeup_enabled = 1;
                return USBD_REQ_HANDLED;
            // case USB_FEAT_TEST_MODE:
            //  return USBD_REQ_HANDLED;
        }
    }

    return USBD_REQ_NEXT_CALLBACK;
}

struct dfu_getstatus_payload {
    // An indication of the status resulting from the execution of the most
    // recent request.
    uint8_t bStatus;
    // Minimum time, in milliseconds, that the host should wait before sending
    // a subsequent DFU_GETSTATUS request.
    uint8_t bwPollTimeout[3];
    // An indication of the state that the device is going to enter immediately
    // following transmission of this response. (By the time the host receives
    // this information, this is the current state of the device.)
    uint8_t bState;
    // Index of status description in string table.
    uint8_t iString;
} __attribute__((packed));

uint8_t dfu_status = DFU_STATUS_OK;
uint8_t dfu_state = STATE_DFU_IDLE;
uint32_t dfu_address = SPI_FLASH_BASE;
uint32_t dfu_bytes = 0;
uint16_t dfu_block_num = 0;
uint8_t dfu_buffer[512] = { 0 };

void dfu_write(uint8_t *data, uint32_t length) {
    printf("Writing to address %d\n\r", dfu_address);
}

static enum usbd_request_return_codes dfu_control_request(
    usbd_device * usbd_dev,
    struct usb_setup_data *req,
    uint8_t ** buf,
    uint16_t * len,
    void(**complete)(usbd_device * usbd_dev, struct usb_setup_data * req)
) {
    uint8_t interface_number;

    (void)usbd_dev;
    (void)complete;

    interface_number = req->wIndex;
    if (interface_number != DFU_INTERFACE_NUMBER)
        return USBD_REQ_NOTSUPP;

    if(
        ((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
        && (req->bRequest == DFU_DNLOAD)
    ) {

        //if(req->wLength) {
        //    if(req->wLength > dfu_function.wTransferSize) {
        //        dfu_state = STATE_DFU_ERROR;
        //        dfu_status = DFU_STATUS_ERR_UNKNOWN;
        //        return USBD_REQ_NOTSUPP;
        //    }

        //    if(dfu_address + req->wLength > MAIN_MEMORY_MAX) {
        //        dfu_state = STATE_DFU_ERROR;
        //        dfu_status = DFU_STATUS_ERR_ADDRESS;
        //        return USBD_REQ_HANDLED;
        //    }

        //    dfu_status = DFU_STATUS_OK;
        //    dfu_state = STATE_DFU_DNLOAD_SYNC;

        //    dfu_write(*buf, req->wLength);

        //    dfu_block_num = req->wValue;
        //    dfu_bytes += req->wLength;

        //    return USBD_REQ_HANDLED;
        //} else {
        //    dfu_status = DFU_STATUS_OK;
        //    dfu_state = STATE_DFU_MANIFEST_SYNC;
        //    return USBD_REQ_HANDLED;
        //}
    }

    if(
        ((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
        && (req->bRequest == DFU_UPLOAD)
    ) {
        // If the host is trying to read past the flash size, halt transaction
        if (req->wValue * 512 >= SPI_FLASH_SIZE)
        {
            *len = 0;
            return USBD_REQ_HANDLED;
        }
        // Otherwise read from MD380 SPI Flash
        W25Qx_wakeup();
        delayUs(5);
        W25Qx_readData(dfu_address + req->wValue * 512, dfu_buffer, req->wLength);
        *buf = dfu_buffer;
        return USBD_REQ_HANDLED;
    }

    if(
        ((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
        && (req->bRequest == DFU_GETSTATUS)
    ) {
        struct dfu_getstatus_payload *status_payload;

        if(req->wLength != sizeof(struct dfu_getstatus_payload))
            return USBD_REQ_NOTSUPP;

        status_payload = (struct dfu_getstatus_payload *)(*buf);

        status_payload->bStatus = dfu_status;
        status_payload->bwPollTimeout[0] = 0;
        status_payload->bwPollTimeout[1] = 0;
        status_payload->bwPollTimeout[2] = 0;
        if(dfu_state == STATE_DFU_DNLOAD_SYNC) {
            dfu_state = STATE_DFU_DNLOAD_IDLE;
        } else if(dfu_state == STATE_DFU_MANIFEST_SYNC) {
            dfu_reset();
        }
        status_payload->bState = dfu_state;
        status_payload->iString = 0;

        *len = sizeof(struct dfu_getstatus_payload);

        return USBD_REQ_HANDLED;
    }

    if(
        ((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
        && (req->bRequest == DFU_CLRSTATUS)
    ) {
        if (dfu_state == STATE_DFU_ERROR) {
            dfu_reset();
        }
        return USBD_REQ_HANDLED;
    }

    if(
        ((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN)
        && (req->bRequest == DFU_GETSTATE)
    ) {
        *buf[0] = dfu_state;
        *len = 1;
        return USBD_REQ_HANDLED;
    }

    if(
        ((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_OUT)
        && (req->bRequest == DFU_ABORT)
    ) {
        dfu_reset();
        return USBD_REQ_HANDLED;
    }

    return USBD_REQ_NEXT_CALLBACK;
}

void dfu_reset(void) {
    dfu_status = DFU_STATUS_OK;
    dfu_state = STATE_DFU_IDLE;
    dfu_address = SPI_FLASH_BASE;
    dfu_bytes = 0;
    dfu_block_num = 0;
    erased_sectors = 0;
}

void reset_callback(void) {
}

void dfu_set_config_callback(usbd_device *usbd_dev) {
    dfu_reset();

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
        USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        dfu_control_request
    );

    usbd_register_reset_callback(usbd_dev, reset_callback);
}

static void dfu_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
    if(wValue != CONFIGURATION_VALUE)
        return;

    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
        USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        control_device_get_status_callback
    );

    usbd_remote_wakeup_enabled = 0;
    usbd_register_control_callback(
        usbd_dev,
        USB_REQ_TYPE_OUT | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_DEVICE,
        USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
        control_device_feature_callback
    );

    dfu_set_config_callback(usbd_dev);
}

int main()
{
    platform_init();

    // Get radio model and MCU serial number
    const hwInfo_t* hwinfo = platform_getHwInfo();
    desig_get_unique_id_as_string(usb_serial_number, sizeof(usb_serial_number));
    usb_strings[1] = hwinfo->name;
    usb_strings[2] = usb_serial_number;

    usbd_device *usbd_dev;
    usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config,
            usb_strings, 4,
            usbd_control_buffer, sizeof(usbd_control_buffer));

    usbd_register_set_config_callback(usbd_dev, dfu_set_config);

    while (1)
    {
        usbd_poll(usbd_dev);
    }
}
