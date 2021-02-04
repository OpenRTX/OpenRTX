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

#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <hwconfig.h>
#include <string.h>
#include <os.h>
#include "GPS.h"

static int8_t detectStatus = -1;
size_t bufPos = 0;
size_t maxPos = 0;
char  *dataBuf;
bool   receiving = false;

OS_FLAG_GRP sentenceReady;
OS_ERR err;

void __attribute__((used)) USART3_IRQHandler()
{
    if(USART3->SR & USART_SR_RXNE)
    {
        char value = USART3->DR;

        if((receiving == false) && (value == '$') && (bufPos == 0))
        {
            receiving = true;
        }

        if(receiving)
        {
            if(bufPos == maxPos)
            {
                receiving = false;
            }

            char prevChar   = dataBuf[bufPos - 1];
            dataBuf[bufPos] = value;
            bufPos += 1;

            if((prevChar == '\r') && (value == '\n'))
            {
                receiving = false;
                bufPos -= 1;
            }
        }

        if((receiving == false) && (bufPos != 0))
        {
            uint8_t flag = (bufPos < maxPos) ? 0x01 : 0x02;
            OSFlagPost(&sentenceReady, flag, OS_OPT_POST_FLAG_SET, &err);
        }
    }

    USART3->SR = 0;
}


void gps_init(const uint16_t baud)
{
    gpio_setMode(GPS_EN,   OUTPUT);
    gpio_setMode(GPS_DATA, ALTERNATE);
    gpio_setAlternateFunction(GPS_DATA, 7);

    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    __DSB();

    const unsigned int quot = 2*42000000/baud;
    USART3->BRR = quot/2 + (quot & 1);
    USART3->CR3 |= USART_CR3_ONEBIT;
    USART3->CR1 = USART_CR1_RE
                | USART_CR1_RXNEIE;

    NVIC_ClearPendingIRQ(USART3_IRQn);
    NVIC_SetPriority(USART3_IRQn, 14);

    OSFlagCreate(&sentenceReady, "", 0, &err);
}

void gps_terminate()
{
    OSFlagDel(&sentenceReady, OS_OPT_DEL_NO_PEND, &err);
    gps_disable();
    RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN;
}

void gps_enable()
{
    gpio_setPin(GPS_EN);
    USART3->CR1 |= USART_CR1_UE;
}

void gps_disable()
{
    gpio_clearPin(GPS_EN);
    USART3->CR1 &= ~USART_CR1_UE;
    NVIC_DisableIRQ(USART3_IRQn);
}

bool gps_detect(uint16_t timeout)
{
    if(detectStatus == -1)
    {
        gpio_setMode(GPS_DATA, INPUT_PULL_DOWN);
        gpio_setMode(GPS_EN,   OUTPUT);
        gpio_setPin(GPS_EN);

        while((gpio_readPin(GPS_DATA) == 0) && (timeout > 0))
        {
            delayMs(1);
            timeout--;
        }

        gpio_clearPin(GPS_EN);
        gpio_setMode(GPS_EN, INPUT);

        if(timeout > 0)
        {
            detectStatus = 1;
        }
        else
        {
            detectStatus = 0;
        }
    }

    return (detectStatus == 1) ? true : false;
}

int gps_getNmeaSentence(char *buf, const size_t maxLength)
{
    if(detectStatus != 1) return -1;

    memset(buf, 0x00, maxLength);
    bufPos = 0;
    maxPos = maxLength;
    dataBuf = buf;

    NVIC_EnableIRQ(USART3_IRQn);

    OS_FLAGS status = OSFlagPend(&sentenceReady, 0x03, 0,
                                 OS_OPT_PEND_FLAG_SET_ANY |
                                 OS_OPT_PEND_FLAG_CONSUME |
                                 OS_OPT_PEND_BLOCKING, NULL, &err);

    NVIC_DisableIRQ(USART3_IRQn);

    if(status & 0x01)
    {
        return bufPos;
    }

    return -1;
}

