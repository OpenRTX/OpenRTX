/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <interfaces/gpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <MK22F51212.h>

#include "usb/usb.h"
#include "usb_vcom.h"
#include "usb/fsl_common.h"
#include "usb/usb_device.h"
#include "usb/usb_device_ch9.h"
#include "usb/usb_device_config.h"
#include "usb/usb_device_cdc_acm.h"
#include "usb/usb_device_descriptor.h"

/* Currently configured line coding */
#define LINE_CODING_SIZE (0x07)
#define LINE_CODING_DTERATE (115200)
#define LINE_CODING_CHARFORMAT (0x00)
#define LINE_CODING_PARITYTYPE (0x00)
#define LINE_CODING_DATABITS (0x08)

/* Communications feature */
#define COMM_FEATURE_DATA_SIZE (0x02)
#define STATUS_ABSTRACT_STATE (0x0000)
#define COUNTRY_SETTING (0x0000)

/* Notification of serial state */
#define NOTIF_PACKET_SIZE (0x08)
#define UART_BITMAP_SIZE (0x02)
#define NOTIF_REQUEST_TYPE (0xA1)

/* Define the types for application */
typedef struct
{
    usb_device_handle deviceHandle;     /* USB device handle.                                                                */
    volatile uint8_t attach;            /* A flag to indicate whether a usb device is attached. 1: attached, 0: not attached */
    uint8_t speed;                      /* Speed of USB device. USB_SPEED_FULL/USB_SPEED_LOW/USB_SPEED_HIGH.                 */
    volatile uint8_t startTransactions; /* A flag to indicate whether a CDC device is ready to transmit and receive data.    */
    uint8_t currentConfiguration;       /* Current configuration value.                                                      */
    uint8_t hasSentState;               /*!< 1: The device has primed the state in interrupt pipe, 0: Not primed the state.  */
    uint8_t currentInterfaceAlternateSetting [USB_CDC_VCOM_INTERFACE_COUNT]; /* Current alternate setting value for each interface. */
}
usb_cdc_vcom_struct_t;

/* Define the infomation relates to abstract control model */
typedef struct
{
    uint8_t serialStateBuf[NOTIF_PACKET_SIZE + UART_BITMAP_SIZE]; /* Serial state buffer of the CDC device to notify the
                                                                     serial state to host. */
    bool dtePresent;          /* A flag to indicate whether DTE is present.         */
    uint16_t breakDuration;   /* Length of time in milliseconds of the break signal */
    uint8_t dteStatus;        /* Status of data terminal equipment                  */
    uint8_t currentInterface; /* Current interface index.                           */
    uint16_t uartState;       /* UART state of the CDC device.                      */
}
usb_cdc_acm_info_t;

/* Data structure of virtual com device */
static usb_cdc_vcom_struct_t cdcVcom;

/* Line codinig of cdc device */
static uint8_t lineCoding[LINE_CODING_SIZE] =
{
    /* E.g. 0x00,0xC2,0x01,0x00 : 0x0001C200 is 115200 bits per second */
    (LINE_CODING_DTERATE >> 0U) & 0x000000FFU,
    (LINE_CODING_DTERATE >> 8U) & 0x000000FFU,
    (LINE_CODING_DTERATE >> 16U) & 0x000000FFU,
    (LINE_CODING_DTERATE >> 24U) & 0x000000FFU,
    LINE_CODING_CHARFORMAT,
    LINE_CODING_PARITYTYPE,
    LINE_CODING_DATABITS
};

/* Abstract state of cdc device */
static uint8_t abstractState[COMM_FEATURE_DATA_SIZE] =
{
    (STATUS_ABSTRACT_STATE >> 0U) & 0x00FFU,
    (STATUS_ABSTRACT_STATE >> 8U) & 0x00FFU
};

/* Country code of cdc device */
static uint8_t countryCode[COMM_FEATURE_DATA_SIZE] =
{
    (COUNTRY_SETTING >> 0U) & 0x00FFU,
    (COUNTRY_SETTING >> 8U) & 0x00FFU
};

/* CDC ACM information */
USB_DATA_ALIGNMENT static usb_cdc_acm_info_t usbCdcAcmInfo =
{{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0};

/* Data buffer for receiving and sending*/
USB_DATA_ALIGNMENT static uint8_t recvBuf[FS_CDC_VCOM_BULK_OUT_PACKET_SIZE];
USB_DATA_ALIGNMENT static uint8_t sendBuf[FS_CDC_VCOM_BULK_OUT_PACKET_SIZE];
static volatile uint32_t recvSize = 0;
static uint32_t usbBulkMaxPacketSize = FS_CDC_VCOM_BULK_OUT_PACKET_SIZE;

/*!
 * @brief Interrupt in pipe callback function.
 *
 * This function serves as the callback function for interrupt in pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCdcAcmInterruptIn(usb_device_handle handle,
                                         usb_device_endpoint_callback_message_struct_t *message,
                                         void *callbackParam)
{
    (void) handle;
    (void) callbackParam;
    (void) message;
    usb_status_t error = kStatus_USB_Error;
    cdcVcom.hasSentState = 0;
    return error;
}

/*!
 * @brief Bulk in pipe callback function.
 *
 * This function serves as the callback function for bulk in pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCdcAcmBulkIn(usb_device_handle handle,
                                    usb_device_endpoint_callback_message_struct_t *message,
                                    void *callbackParam)
{
    (void) callbackParam;
    usb_status_t error = kStatus_USB_Error;

    if ((message->length != 0) && (!(message->length % usbBulkMaxPacketSize)))
    {
        /* If the last packet is the size of endpoint, then send also zero-ended packet,
         ** meaning that we want to inform the host that we do not have any additional
         ** data, so it can flush the output.
         */
        USB_DeviceSendRequest(handle, USB_CDC_VCOM_BULK_IN_ENDPOINT, NULL, 0);
    }
    else if ((1 == cdcVcom.attach) && (1 == cdcVcom.startTransactions))
    {
        if ((message->buffer != NULL) || ((message->buffer == NULL) && (message->length == 0)))
        {
            /* User: add your own code for send complete event */
            /* Schedule buffer for next receive event */
            USB_DeviceRecvRequest(handle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, recvBuf, usbBulkMaxPacketSize);
        }
    }
    else
    {
    }
    return error;
}

/*!
 * @brief Bulk out pipe callback function.
 *
 * This function serves as the callback function for bulk out pipe.
 *
 * @param handle The USB device handle.
 * @param message The endpoint callback message
 * @param callbackParam The parameter of the callback.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCdcAcmBulkOut(usb_device_handle handle,
                                     usb_device_endpoint_callback_message_struct_t *message,
                                     void *callbackParam)
{
    (void) callbackParam;
    usb_status_t error = kStatus_USB_Error;

    if ((1 == cdcVcom.attach) && (1 == cdcVcom.startTransactions))
    {
        recvSize = message->length;

        if (!recvSize)
        {
            /* Schedule buffer for next receive event */
            USB_DeviceRecvRequest(handle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, recvBuf, usbBulkMaxPacketSize);
        }
    }
    return error;
}

/*!
 * @brief Get the setup packet buffer.
 *
 * This function provides the buffer for setup packet.
 *
 * @param handle The USB device handle.
 * @param setupBuffer The pointer to the address of setup packet buffer.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetSetupBuffer(usb_device_handle handle, usb_setup_struct_t **setupBuffer)
{
    (void) handle;
    static uint32_t cdcVcomSetup[2];
    if (NULL == setupBuffer)
    {
        return kStatus_USB_InvalidParameter;
    }
    *setupBuffer = (usb_setup_struct_t *)&cdcVcomSetup;
    return kStatus_USB_Success;
}

/*!
 * @brief Get the setup packet data buffer.
 *
 * This function gets the data buffer for setup packet.
 *
 * @param handle The USB device handle.
 * @param setup The pointer to the setup packet.
 * @param length The pointer to the length of the data buffer.
 * @param buffer The pointer to the address of setup packet data buffer.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceGetClassReceiveBuffer(usb_device_handle handle,
                                             usb_setup_struct_t *setup,
                                             uint32_t *length,
                                             uint8_t **buffer)
{
    (void) handle;
    (void) setup;
    static uint8_t setupOut[8];
    if ((NULL == buffer) || ((*length) > sizeof(setupOut)))
    {
        return kStatus_USB_InvalidRequest;
    }
    *buffer = setupOut;
    return kStatus_USB_Success;
}

/*!
 * @brief Configure remote wakeup feature.
 *
 * This function configures the remote wakeup feature.
 *
 * @param handle The USB device handle.
 * @param enable 1: enable, 0: disable.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceConfigureRemoteWakeup(usb_device_handle handle, uint8_t enable)
{
    (void) handle;
    (void) enable;
    return kStatus_USB_InvalidRequest;
}

/*!
 * @brief CDC class specific callback function.
 *
 * This function handles the CDC class specific requests.
 *
 * @param handle The USB device handle.
 * @param setup The pointer to the setup packet.
 * @param length The pointer to the length of the data buffer.
 * @param buffer The pointer to the address of setup packet data buffer.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceProcessClassRequest(usb_device_handle handle,
                                           usb_setup_struct_t *setup,
                                           uint32_t *length,
                                           uint8_t **buffer)
{
    usb_status_t error = kStatus_USB_InvalidRequest;

    usb_cdc_acm_info_t *acmInfo = &usbCdcAcmInfo;
    uint32_t len;
    uint16_t *uartBitmap;
    if (setup->wIndex != USB_CDC_VCOM_COMM_INTERFACE_INDEX)
    {
        return error;
    }

    switch (setup->bRequest)
    {
        case USB_DEVICE_CDC_REQUEST_SEND_ENCAPSULATED_COMMAND:
            break;
        case USB_DEVICE_CDC_REQUEST_GET_ENCAPSULATED_RESPONSE:
            break;
        case USB_DEVICE_CDC_REQUEST_SET_COMM_FEATURE:
            if (USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE == setup->wValue)
            {
                *buffer = abstractState;
            }
            else if (USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING == setup->wValue)
            {
                *buffer = countryCode;
            }
            else
            {
            }
            error = kStatus_USB_Success;
            break;
        case USB_DEVICE_CDC_REQUEST_GET_COMM_FEATURE:
            if (USB_DEVICE_CDC_FEATURE_ABSTRACT_STATE == setup->wValue)
            {
                *buffer = abstractState;
                *length = COMM_FEATURE_DATA_SIZE;
            }
            else if (USB_DEVICE_CDC_FEATURE_COUNTRY_SETTING == setup->wValue)
            {
                *buffer = countryCode;
                *length = COMM_FEATURE_DATA_SIZE;
            }
            else
            {
            }
            error = kStatus_USB_Success;
            break;
        case USB_DEVICE_CDC_REQUEST_CLEAR_COMM_FEATURE:
            break;
        case USB_DEVICE_CDC_REQUEST_GET_LINE_CODING:
            *buffer = lineCoding;
            *length = LINE_CODING_SIZE;
            error = kStatus_USB_Success;
            break;
        case USB_DEVICE_CDC_REQUEST_SET_LINE_CODING:
            *buffer = lineCoding;
            error = kStatus_USB_Success;
            break;
        case USB_DEVICE_CDC_REQUEST_SET_CONTROL_LINE_STATE:
        {
            error = kStatus_USB_Success;
            acmInfo->dteStatus = setup->wValue;
            /* activate/deactivate Tx carrier */
            if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION)
            {
                acmInfo->uartState |= USB_DEVICE_CDC_UART_STATE_TX_CARRIER;
            }
            else
            {
                acmInfo->uartState &= (uint16_t)~USB_DEVICE_CDC_UART_STATE_TX_CARRIER;
            }

            /* activate carrier and DTE */
            if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE)
            {
                acmInfo->uartState |= USB_DEVICE_CDC_UART_STATE_RX_CARRIER;
            }
            else
            {
                acmInfo->uartState &= (uint16_t)~USB_DEVICE_CDC_UART_STATE_RX_CARRIER;
            }

            /* Indicates to DCE if DTE is present or not */
            acmInfo->dtePresent = (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE) ? true : false;

            /* Initialize the serial state buffer */
            acmInfo->serialStateBuf[0] = NOTIF_REQUEST_TYPE;                        /* bmRequestType */
            acmInfo->serialStateBuf[1] = USB_DEVICE_CDC_REQUEST_SERIAL_STATE_NOTIF; /* bNotification */
            acmInfo->serialStateBuf[2] = 0x00;                                      /* wValue */
            acmInfo->serialStateBuf[3] = 0x00;
            acmInfo->serialStateBuf[4] = 0x00; /* wIndex */
            acmInfo->serialStateBuf[5] = 0x00;
            acmInfo->serialStateBuf[6] = UART_BITMAP_SIZE; /* wLength */
            acmInfo->serialStateBuf[7] = 0x00;
            /* Notifiy to host the line state */
            acmInfo->serialStateBuf[4] = setup->wIndex;
            /* Lower byte of UART BITMAP */
            uartBitmap = (uint16_t *)&acmInfo->serialStateBuf[NOTIF_PACKET_SIZE + UART_BITMAP_SIZE - 2];
            *uartBitmap = acmInfo->uartState;
            len = (uint32_t)(NOTIF_PACKET_SIZE + UART_BITMAP_SIZE);
            if (0 == cdcVcom.hasSentState)
            {
                USB_DeviceSendRequest(handle, USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT, acmInfo->serialStateBuf, len);
                cdcVcom.hasSentState = 1;
            }
            /* Update status */
            if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_CARRIER_ACTIVATION)
            {
                /*    To do: CARRIER_ACTIVATED */
            }
            else
            {
                /* To do: CARRIER_DEACTIVATED */
            }
            if (acmInfo->dteStatus & USB_DEVICE_CDC_CONTROL_SIG_BITMAP_DTE_PRESENCE)
            {
                /* DTE_ACTIVATED */
                if (1 == cdcVcom.attach)
                {
                    cdcVcom.startTransactions = 1;
                }
            }
            else
            {
                /* DTE_DEACTIVATED */
                if (1 == cdcVcom.attach)
                {
                    cdcVcom.startTransactions = 0;
                }
            }
        }
        break;
        case USB_DEVICE_CDC_REQUEST_SEND_BREAK:
            break;
        default:
            break;
    }

    return error;
}

/*!
 * @brief USB device callback function.
 *
 * This function handles the usb device specific requests.
 *
 * @param handle          The USB device handle.
 * @param event           The USB device event type.
 * @param param           The parameter of the device specific request.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    uint8_t *temp8 = (uint8_t *)param;

    switch (event)
    {
        case kUSB_DeviceEventBusReset:
        {
            USB_DeviceControlPipeInit(cdcVcom.deviceHandle);
            cdcVcom.attach = 0;
        }
        break;
        case kUSB_DeviceEventSetConfiguration:
            if (param)
            {
                cdcVcom.attach = 1;
                cdcVcom.currentConfiguration = *temp8;
                if (USB_CDC_VCOM_CONFIGURE_INDEX == (*temp8))
                {
                    usb_device_endpoint_init_struct_t epInitStruct;
                    usb_device_endpoint_callback_struct_t endpointCallback;

                    /* Initiailize endpoint for interrupt pipe */
                    endpointCallback.callbackFn = USB_DeviceCdcAcmInterruptIn;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_INTERRUPT;
                    epInitStruct.endpointAddress = USB_CDC_VCOM_INTERRUPT_IN_ENDPOINT |
                                                   (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    if (USB_SPEED_HIGH == cdcVcom.speed)
                    {
                        epInitStruct.maxPacketSize = HS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;
                    }
                    else
                    {
                        epInitStruct.maxPacketSize = FS_CDC_VCOM_INTERRUPT_IN_PACKET_SIZE;
                    }

                    USB_DeviceInitEndpoint(cdcVcom.deviceHandle, &epInitStruct, &endpointCallback);

                    /* Initiailize endpoints for bulk pipe */
                    endpointCallback.callbackFn = USB_DeviceCdcAcmBulkIn;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_BULK;
                    epInitStruct.endpointAddress =
                        USB_CDC_VCOM_BULK_IN_ENDPOINT | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    if (USB_SPEED_HIGH == cdcVcom.speed)
                    {
                        epInitStruct.maxPacketSize = HS_CDC_VCOM_BULK_IN_PACKET_SIZE;
                    }
                    else
                    {
                        epInitStruct.maxPacketSize = FS_CDC_VCOM_BULK_IN_PACKET_SIZE;
                    }

                    USB_DeviceInitEndpoint(cdcVcom.deviceHandle, &epInitStruct, &endpointCallback);

                    endpointCallback.callbackFn = USB_DeviceCdcAcmBulkOut;
                    endpointCallback.callbackParam = handle;

                    epInitStruct.zlt = 0;
                    epInitStruct.transferType = USB_ENDPOINT_BULK;
                    epInitStruct.endpointAddress =
                        USB_CDC_VCOM_BULK_OUT_ENDPOINT | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT);
                    if (USB_SPEED_HIGH == cdcVcom.speed)
                    {
                        epInitStruct.maxPacketSize = HS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
                    }
                    else
                    {
                        epInitStruct.maxPacketSize = FS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
                    }

                    USB_DeviceInitEndpoint(cdcVcom.deviceHandle, &epInitStruct, &endpointCallback);

                    if (USB_SPEED_HIGH == cdcVcom.speed)
                    {
                        usbBulkMaxPacketSize = HS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
                    }
                    else
                    {
                        usbBulkMaxPacketSize = FS_CDC_VCOM_BULK_OUT_PACKET_SIZE;
                    }
                    /* Schedule buffer for receive */
                    USB_DeviceRecvRequest(handle, USB_CDC_VCOM_BULK_OUT_ENDPOINT, recvBuf,
                                          usbBulkMaxPacketSize);
                }
            }
            break;
        default:
            break;
    }

    return error;
}

/*!
 * @brief USB configure endpoint function.
 *
 * This function configure endpoint status.
 *
 * @param handle The USB device handle.
 * @param ep Endpoint address.
 * @param status A flag to indicate whether to stall the endpoint. 1: stall, 0: unstall.
 *
 * @return A USB error code or kStatus_USB_Success.
 */
usb_status_t USB_DeviceConfigureEndpointStatus(usb_device_handle handle, uint8_t ep, uint8_t status)
{
    if (status)
    {
        return USB_DeviceStallEndpoint(handle, ep);
    }
    else
    {
        return USB_DeviceUnstallEndpoint(handle, ep);
    }
}

/*!
 * @brief USB Interrupt service routine.
 *
 * This function serves as the USB interrupt service routine.
 *
 * @return None.
 */
void USB0_IRQHandler(void)
{
    USB_DeviceKhciIsrFunction(cdcVcom.deviceHandle);
}


/******************************************************************************
 *                                                                            *
 *              Implementation of USB vcom functions                          *
 *                                                                            *
 ******************************************************************************/

void vcom_init()
{
    SIM->CLKDIV2 = SIM_CLKDIV2_USBDIV(0)
                 | SIM_CLKDIV2_USBFRAC(0);
    SIM->SOPT2 = SIM_SOPT2_USBSRC(1U)
               | SIM_SOPT2_PLLFLLSEL(3U);
    SIM->SCGC4 |= SIM_SCGC4_USBOTG(1);

    USB0->CLK_RECOVER_IRC_EN = 0x03U;
    USB0->CLK_RECOVER_CTRL |= USB_CLK_RECOVER_CTRL_CLOCK_RECOVER_EN_MASK;

    cdcVcom.speed = USB_SPEED_FULL;
    cdcVcom.attach = 0;
    cdcVcom.deviceHandle = NULL;

    if (USB_DeviceInit(kUSB_ControllerKhci0,USB_DeviceCallback,
                                &cdcVcom.deviceHandle) != kStatus_USB_Success)
    {
        return;
    }

    NVIC_SetPriority(USB0_IRQn, 3);
    NVIC_EnableIRQ(USB0_IRQn);

    USB_DeviceRun(cdcVcom.deviceHandle);
}

ssize_t vcom_writeBlock(const void *buf, size_t len)
{
    if((cdcVcom.attach == 1) && (cdcVcom.startTransactions == 1))
    {
        uint32_t bytesToSend = len;
        while(bytesToSend > 0)
        {
            uint32_t xFerLen = (bytesToSend > FS_CDC_VCOM_BULK_OUT_PACKET_SIZE)
                             ? FS_CDC_VCOM_BULK_OUT_PACKET_SIZE : bytesToSend;

            memcpy(sendBuf, buf, xFerLen);
            usb_status_t st = USB_DeviceSendRequest(cdcVcom.deviceHandle,
                                                  USB_CDC_VCOM_BULK_IN_ENDPOINT,
                                                  sendBuf, xFerLen);
            if(st != kStatus_USB_Success) return -1;
            bytesToSend -= xFerLen;
        }
    }

    return len;
}

ssize_t vcom_readBlock(void* buf, size_t len)
{
    if(recvSize != 0)
    {
        size_t toTransfer = (len < recvSize) ? len : recvSize;
        memcpy(buf, recvBuf, toTransfer);
        recvSize = 0;
        return ((ssize_t) toTransfer);
    }

    return 0;
}
