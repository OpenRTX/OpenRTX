/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Silvano Seva IU2KWO                      *
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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "usbd_core.h"
#include "usb_defines.h"
#include "usbd_desc.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_req.h"
#include "stm32f4xx.h"

#include <interfaces/delays.h>
#include "usb_vcom.h"

/* Common USB OTG handle, also defined as 'extern' in other modules */
USB_OTG_CORE_HANDLE USB_OTG_dev;

/* 'Service' variables: command buffer, ... */
extern uint8_t USBD_DeviceDesc [USB_SIZ_DEVICE_DESC];
uint8_t CmdBuff[CDC_CMD_PACKET_SZE];
static uint32_t cdcCmd = 0xFF;
static uint32_t cdcLen = 0;
static __IO uint32_t usbd_cdc_AltSet = 0;

/* Buffer for OUT endpoint, the one receiving data from host */
uint8_t outEnpBuffer[CDC_DATA_OUT_PACKET_SIZE];

/* Circular buffer for incoming data enqueuement: each packet coming from host
 * is stored here, eventually erasing oldest data.
 * Buffer is statically allocated.
 */
struct rb
{
    uint8_t data[RX_RING_BUF_SIZE];
    size_t readPtr;
    size_t writePtr;
}
rxRingBuf;

bool txDone;    /* Flag for end of data transmission. */

/* USB CDC device Configuration Descriptor */
uint8_t usbd_cdc_CfgDesc[USB_CDC_CONFIG_DESC_SIZ] =
{
    /*Configuration Descriptor*/
    0x09,   /* bLength: Configuration Descriptor size */
    USB_CONFIGURATION_DESCRIPTOR_TYPE,      /* bDescriptorType: Configuration */
    USB_CDC_CONFIG_DESC_SIZ,                /* wTotalLength:no of returned bytes */
    0x00,
    0x02,   /* bNumInterfaces: 2 interface */
    0x01,   /* bConfigurationValue: Configuration value */
    0x00,   /* iConfiguration: Index of string descriptor describing the configuration */
    0xC0,   /* bmAttributes: self powered */
    0x32,   /* MaxPower 0 mA */

    /*Interface Descriptor */
    0x09,   /* bLength: Interface Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: Interface */
    /* Interface descriptor type */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x01,   /* bNumEndpoints: One endpoints used */
    0x02,   /* bInterfaceClass: Communication Interface Class */
    0x02,   /* bInterfaceSubClass: Abstract Control Model */
    0x01,   /* bInterfaceProtocol: Common AT commands */
    0x00,   /* iInterface: */

    /*Header Functional Descriptor*/
    0x05,   /* bLength: Endpoint Descriptor size */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x00,   /* bDescriptorSubtype: Header Func Desc */
    0x10,   /* bcdCDC: spec release number */
    0x01,

    /*Call Management Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x01,   /* bDescriptorSubtype: Call Management Func Desc */
    0x00,   /* bmCapabilities: D0+D1 */
    0x01,   /* bDataInterface: 1 */

    /*ACM Functional Descriptor*/
    0x04,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,   /* bmCapabilities */

    /*Union Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x06,   /* bDescriptorSubtype: Union func desc */
    0x00,   /* bMasterInterface: Communication class interface */
    0x01,   /* bSlaveInterface0: Data Class Interface */

    /*Endpoint 2 Descriptor*/
    0x07,                           /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,   /* bDescriptorType: Endpoint */
    CDC_CMD_EP,                     /* bEndpointAddress */
    0x03,                           /* bmAttributes: Interrupt */
    LOBYTE(CDC_CMD_PACKET_SZE),     /* wMaxPacketSize: */
    HIBYTE(CDC_CMD_PACKET_SZE),
    0x10,                           /* bInterval: */

    /*Data class interface descriptor*/
    0x09,   /* bLength: Endpoint Descriptor size */
    USB_INTERFACE_DESCRIPTOR_TYPE,  /* bDescriptorType: */
    0x01,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x02,   /* bNumEndpoints: Two endpoints used */
    0x0A,   /* bInterfaceClass: CDC */
    0x00,   /* bInterfaceSubClass: */
    0x00,   /* bInterfaceProtocol: */
    0x00,   /* iInterface: */

    /*Endpoint OUT Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
    CDC_OUT_EP,                        /* bEndpointAddress */
    0x02,                              /* bmAttributes: Bulk */
    LOBYTE(CDC_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
    HIBYTE(CDC_DATA_MAX_PACKET_SIZE),
    0x00,                              /* bInterval: ignore for Bulk transfer */

    /*Endpoint IN Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    USB_ENDPOINT_DESCRIPTOR_TYPE,      /* bDescriptorType: Endpoint */
    CDC_IN_EP,                         /* bEndpointAddress */
    0x02,                              /* bmAttributes: Bulk */
    LOBYTE(CDC_DATA_MAX_PACKET_SIZE),  /* wMaxPacketSize: */
    HIBYTE(CDC_DATA_MAX_PACKET_SIZE),
    0x00                               /* bInterval: ignore for Bulk transfer */
};

/* The following structures groups all needed parameters to be configured for
 * the ComPort. These parameters can modified on the fly by the host through
 * CDC class command class requests.
 */
typedef struct
{
  uint32_t bitrate;
  uint8_t  format;
  uint8_t  paritytype;
  uint8_t  datatype;
    uint8_t changed;
}LINE_CODING;

/* USB virtual com settings: 115200 baud, 8n1 */
LINE_CODING linecoding =
{
    115200, /* baud rate */
    0x00,   /* stop bits-1 */
    0x00,   /* parity - none */
    0x08,   /* nb. of bits 8 */
    1       /* Changed flag */
};

/* USB CDC callbacks */
static uint8_t  usbd_cdc_Init        (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_cdc_DeInit      (void  *pdev, uint8_t cfgidx);
static uint8_t  usbd_cdc_Setup       (void  *pdev, USB_SETUP_REQ *req);
static uint8_t  usbd_cdc_EP0_RxReady (void *pdev);
static uint8_t  usbd_cdc_DataIn      (void *pdev, uint8_t epnum);
static uint8_t  usbd_cdc_DataOut     (void *pdev, uint8_t epnum);
static uint8_t  usbd_cdc_SOF         (void *pdev);
static uint8_t  *USBD_cdc_GetCfgDesc (uint8_t speed, uint16_t *length);

USBD_Class_cb_TypeDef USBD_CDC_cb =
{
  usbd_cdc_Init,
  usbd_cdc_DeInit,
  usbd_cdc_Setup,
  NULL,                 /* EP0_TxSent, */
  usbd_cdc_EP0_RxReady,
  usbd_cdc_DataIn,
  usbd_cdc_DataOut,
  usbd_cdc_SOF,
  NULL,
  NULL,
  USBD_cdc_GetCfgDesc
};

/* 'Service' function for CDC callbacks */
static uint16_t VCP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len);

/******************************************************************************
 *                                                                            *
 *              Implementation of USB vcom functions                          *
 *                                                                            *
 ******************************************************************************/

int vcom_init()
{
    rxRingBuf.readPtr = 0;
    rxRingBuf.writePtr = 0;

    USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb,
              &USR_cb);

    return 0;
}

ssize_t vcom_writeBlock(const void* buf, size_t len)
{
    txDone = false;
    DCD_EP_Tx (&USB_OTG_dev, CDC_IN_EP, (uint8_t*) buf, len);

    uint16_t timeout = 0;

    while(!txDone)
    {
        delayMs(1);
        timeout++;
        if(timeout > 500)
        {
            DCD_EP_Flush(&USB_OTG_dev, CDC_IN_EP);
            return -1;
        }
    }

    return len;
}

ssize_t vcom_readBlock(void* buf, size_t len)
{
    uint8_t *b = ((uint8_t *) buf);
    size_t i;
    for(i = 0; i < len; i++)
    {
        /* Terminate if all data available has been popped out */
        if(rxRingBuf.readPtr == rxRingBuf.writePtr) break;
        b[i] = rxRingBuf.data[rxRingBuf.readPtr];
        rxRingBuf.readPtr = (rxRingBuf.readPtr + 1)%RX_RING_BUF_SIZE;
    }

    return i;
}

/******************************************************************************
 *                                                                            *
 *              Implementation of USB CDC callbacks                           *
 *                                                                            *
 ******************************************************************************/

static uint8_t  usbd_cdc_Init (void  *pdev, uint8_t cfgidx)
{
    (void) cfgidx;
    uint8_t *pbuf;

    /* Open EP IN */
    DCD_EP_Open(pdev, CDC_IN_EP, CDC_DATA_IN_PACKET_SIZE, USB_OTG_EP_BULK);

    /* Open EP OUT */
    DCD_EP_Open(pdev, CDC_OUT_EP, CDC_DATA_OUT_PACKET_SIZE, USB_OTG_EP_BULK);

    /* Open Command IN EP */
    DCD_EP_Open(pdev, CDC_CMD_EP, CDC_CMD_PACKET_SZE, USB_OTG_EP_INT);

    pbuf = (uint8_t *)USBD_DeviceDesc;
    pbuf[4] = DEVICE_CLASS_CDC;
    pbuf[5] = DEVICE_SUBCLASS_CDC;

    /* Prepare Out endpoint to receive next packet */
    DCD_EP_PrepareRx(pdev, CDC_OUT_EP, outEnpBuffer, CDC_DATA_OUT_PACKET_SIZE);

    return USBD_OK;
}

static uint8_t  usbd_cdc_DeInit (void  *pdev, uint8_t cfgidx)
{
    (void) cfgidx;

    /* Open EP IN */
    DCD_EP_Close(pdev, CDC_IN_EP);

    /* Open EP OUT */
    DCD_EP_Close(pdev, CDC_OUT_EP);

    /* Open Command IN EP */
    DCD_EP_Close(pdev,CDC_CMD_EP);

    return USBD_OK;
}

static uint8_t  usbd_cdc_Setup (void  *pdev, USB_SETUP_REQ *req)
{
  uint16_t len=USB_CDC_DESC_SIZ;
  uint8_t  *pbuf=usbd_cdc_CfgDesc + 9;

    switch (req->bmRequest & USB_REQ_TYPE_MASK)
    {
        /* CDC Class Requests -------------------------------*/
        case USB_REQ_TYPE_CLASS :
            /* Check if the request is a data setup packet */
            if (req->wLength)
            {
                /* Check if the request is Device-to-Host */
                if (req->bmRequest & 0x80)
                {
                    /* Get the data to be sent to Host from interface layer */
                    VCP_Ctrl(req->bRequest, CmdBuff, req->wLength);

                    /* Send the data to the host */
                    USBD_CtlSendData (pdev, CmdBuff, req->wLength);
                }
                else /* Host-to-Device requeset */
                {
                    /* Set the value of the current command to be processed */
                    cdcCmd = req->bRequest;
                    cdcLen = req->wLength;

                    /* Prepare the reception of the buffer over EP0
                       Next step: the received data will be managed in
                       usbd_cdc_EP0_TxSent() function.
                    */
                    USBD_CtlPrepareRx (pdev, CmdBuff, req->wLength);
                }
            }
            else /* No Data request */
            {
                /* Transfer the command to the interface layer */
                VCP_Ctrl(req->bRequest, NULL, 0);
            }

            return USBD_OK;

        default:
            USBD_CtlError (pdev, req);
            return USBD_FAIL;

        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest)
            {
                case USB_REQ_GET_DESCRIPTOR:
                    if( (req->wValue >> 8) == CDC_DESCRIPTOR_TYPE)
                    {
                        pbuf = usbd_cdc_CfgDesc + 9 + (9 * USBD_ITF_MAX_NUM);
                        len = MIN(USB_CDC_DESC_SIZ , req->wLength);
                    }

                    USBD_CtlSendData (pdev, pbuf, len);
                    break;

                case USB_REQ_GET_INTERFACE:
                    USBD_CtlSendData (pdev, (uint8_t *)&usbd_cdc_AltSet, 1);
                    break;

                case USB_REQ_SET_INTERFACE :
                    if ((uint8_t)(req->wValue) < USBD_ITF_MAX_NUM)
                    {
                        usbd_cdc_AltSet = (uint8_t)(req->wValue);
                    }
                    else
                    {
                        /* Call the error management function
                         * (command will be nacked)
                         */
                        USBD_CtlError (pdev, req);
                    }
                    break;
            }
    }
    return USBD_OK;
}

static uint8_t  usbd_cdc_EP0_RxReady (void  *pdev)
{
    (void) pdev;

    if (cdcCmd != NO_CMD)
    {
        VCP_Ctrl(cdcCmd, CmdBuff, cdcLen);
        cdcCmd = NO_CMD;
    }

    return USBD_OK;
}

static uint8_t  usbd_cdc_DataIn (void *pdev, uint8_t epnum)
{
    (void) pdev;
    (void) epnum;

    txDone = true;

    return USBD_OK;
}

static uint8_t  usbd_cdc_DataOut (void *pdev, uint8_t epnum)
{
    /* Get size of received data */
    size_t len = ((USB_OTG_CORE_HANDLE*)pdev)->dev.out_ep[epnum].xfer_count;

    /* Transfer data to RX ring buffer */
    for(size_t i = 0; i < len; i++)
    {
        rxRingBuf.data[rxRingBuf.writePtr] = outEnpBuffer[i];
        if((rxRingBuf.writePtr + 1) == rxRingBuf.readPtr)
        {
            // Buffer full, pop one byte from tail.
            rxRingBuf.readPtr = (rxRingBuf.readPtr + 1)%RX_RING_BUF_SIZE;
        }
        rxRingBuf.writePtr = (rxRingBuf.writePtr + 1)%RX_RING_BUF_SIZE;
    }

    /* Prepare Out endpoint to receive next packet */
    DCD_EP_PrepareRx(pdev, CDC_OUT_EP, outEnpBuffer, CDC_DATA_OUT_PACKET_SIZE);

  return USBD_OK;
}

static uint8_t  usbd_cdc_SOF (void *pdev)
{
    (void) pdev;
    return USBD_OK;
}

static uint8_t  *USBD_cdc_GetCfgDesc (uint8_t speed, uint16_t *length)
{
    (void) speed;
    (void) length;

    *length = sizeof (usbd_cdc_CfgDesc);
    return usbd_cdc_CfgDesc;
}

static uint16_t VCP_Ctrl (uint32_t Cmd, uint8_t* Buf, uint32_t Len)
{
    (void) Len;

    /* NOTE:commands not  needed for this driver:
     *  SEND_ENCAPSULATED_COMMAND
     *  GET_ENCAPSULATED_RESPONSE
     *  SET_COMM_FEATURE
     *  GET_COMM_FEATURE
     *  CLEAR_COMM_FEATURE
     *  SET_LINE_CODING
     *  GET_LINE_CODING
     *  SET_CONTROL_LINE_STATE
     *  SEND_BREAK
     */

    if(Cmd == SET_LINE_CODING)
    {
        linecoding.bitrate = (uint32_t)(Buf[0] | (Buf[1] << 8)
                                     | (Buf[2] << 16) | (Buf[3] << 24));
        linecoding.format = Buf[4];
        linecoding.paritytype = Buf[5];
        linecoding.datatype = Buf[6];
        linecoding.changed = 1;
    }
    else if(Cmd == GET_LINE_CODING)
    {
        Buf[0] = (uint8_t)(linecoding.bitrate);
        Buf[1] = (uint8_t)(linecoding.bitrate >> 8);
        Buf[2] = (uint8_t)(linecoding.bitrate >> 16);
        Buf[3] = (uint8_t)(linecoding.bitrate >> 24);
        Buf[4] = linecoding.format;
        Buf[5] = linecoding.paritytype;
        Buf[6] = linecoding.datatype;
    }

    return USBD_OK;
}
