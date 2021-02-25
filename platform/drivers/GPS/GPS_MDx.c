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
#include <interfaces/gps.h>
#include <hwconfig.h>
#include <string.h>
#include <os.h>

int8_t detectStatus = -1;
size_t bufPos = 0;
size_t maxPos = 0;
char  *dataBuf;
bool   receiving = false;

OS_FLAG_GRP sentenceReady;

#ifdef PLATFORM_MD3x0
#define PORT USART3
#else
#define PORT USART1
#endif

#ifdef PLATFORM_MD3x0
void __attribute__((used)) USART3_IRQHandler()
#else
void __attribute__((used)) USART1_IRQHandler()
#endif
{
    if(PORT->SR & USART_SR_RXNE)
    {
        char value = PORT->DR;

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
            OS_ERR err;
            OSFlagPost(&sentenceReady, flag, OS_OPT_POST_FLAG_SET, &err);
        }
    }

    PORT->SR = 0;
}


void gps_init(const uint16_t baud)
{
    gpio_setMode(GPS_EN,   OUTPUT);
    gpio_setMode(GPS_DATA, ALTERNATE);
    gpio_setAlternateFunction(GPS_DATA, 7);

    #ifdef PLATFORM_MD3x0
    const unsigned int quot = 2*42000000/baud;  /* APB1 clock is 42MHz */
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    #else
    const unsigned int quot = 2*84000000/baud;  /* APB2 clock is 84MHz */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    #endif
    __DSB();

    PORT->BRR = quot/2 + (quot & 1);
    PORT->CR3 |= USART_CR3_ONEBIT;
    PORT->CR1 = USART_CR1_RE
                | USART_CR1_RXNEIE;

    #ifdef PLATFORM_MD3x0
    NVIC_ClearPendingIRQ(USART3_IRQn);
    NVIC_SetPriority(USART3_IRQn, 14);
    #else
    NVIC_ClearPendingIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 14);
    #endif

    OS_ERR err;
    OSFlagCreate(&sentenceReady, "", 0, &err);
}

void gps_terminate()
{
    OS_ERR err;
    OSFlagDel(&sentenceReady, OS_OPT_DEL_NO_PEND, &err);
    gps_disable();

    #ifdef PLATFORM_MD3x0
    RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN;
    #else
    RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
    #endif
}

void gps_enable()
{
    gpio_setPin(GPS_EN);
    PORT->CR1 |= USART_CR1_UE;
}

void gps_disable()
{
    gpio_clearPin(GPS_EN);
    PORT->CR1 &= ~USART_CR1_UE;

    #ifdef PLATFORM_MD3x0
    NVIC_DisableIRQ(USART3_IRQn);
    #else
    NVIC_DisableIRQ(USART1_IRQn);
    #endif
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

    #ifdef PLATFORM_MD3x0
    NVIC_EnableIRQ(USART3_IRQn);
    #else
    NVIC_EnableIRQ(USART1_IRQn);
    #endif

    OS_ERR err;
    OS_FLAGS status = OSFlagPend(&sentenceReady, 0x03, 0,
                                 OS_OPT_PEND_FLAG_SET_ANY |
                                 OS_OPT_PEND_FLAG_CONSUME |
                                 OS_OPT_PEND_BLOCKING, NULL, &err);

    #ifdef PLATFORM_MD3x0
    NVIC_DisableIRQ(USART3_IRQn);
    #else
    NVIC_DisableIRQ(USART1_IRQn);
    #endif

    if(status & 0x01)
    {
        return bufPos;
    }

    return -1;
}

