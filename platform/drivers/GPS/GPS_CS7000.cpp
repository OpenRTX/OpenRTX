/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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

#include <kernel/scheduler/scheduler.h>
#include <interfaces/delays.h>
#include <peripherals/gpio.h>
#include <peripherals/gps.h>
#include <hwconfig.h>
#include <string.h>
#include <miosix.h>
#include <rcc.h>

static size_t bufPos = 0;
static size_t maxPos = 0;
static char   *dataBuf;
static bool   receiving = false;

using namespace miosix;
static Thread *gpsWaiting = 0;

void __attribute__((used)) GpsUsartImpl()
{
    if(USART6->SR & USART_SR_RXNE)
    {
        char value = USART6->DR;

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
            // NMEA sentence received, turn off serial port
            USART6->CR1 &= ~USART_CR1_UE;

            if(gpsWaiting)
            {
                gpsWaiting->IRQwakeup();
                if(gpsWaiting->IRQgetPriority()>
                    Thread::IRQgetCurrentThread()->IRQgetPriority())
                        Scheduler::IRQfindNextThread();
                gpsWaiting = 0;
            }
        }
    }

    USART6->SR = 0;
}

void __attribute__((naked)) USART6_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z12GpsUsartImplv");
    restoreContext();
}


void gps_init(const uint16_t baud)
{
    gpio_setMode(GPS_RXD, ALTERNATE | ALTERNATE_FUNC(8));

    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
    __DSB();

    uint32_t quot = rcc_getBusClock(PERIPH_BUS_APB2);
    quot = (2 * quot) / baud;

    USART6->BRR  = (quot / 2) + (quot & 1);
    USART6->CR3 |= USART_CR3_ONEBIT;
    USART6->CR1  = USART_CR1_RE
                 | USART_CR1_RXNEIE;

    NVIC_SetPriority(USART6_IRQn, 14);
}

void gps_terminate()
{
    gps_disable();

    RCC->APB2ENR &= ~RCC_APB2ENR_USART6EN;
}

void gps_enable()
{
    // Enable IRQ
    NVIC_ClearPendingIRQ(USART6_IRQn);
    NVIC_EnableIRQ(USART6_IRQn);
}

void gps_disable()
{
    USART6->CR1 &= ~USART_CR1_UE;
    NVIC_DisableIRQ(USART6_IRQn);

    receiving = false;
    bufPos    = 0;
}

bool gps_detect(uint16_t timeout)
{
    (void) timeout;

    return true;
}

int gps_getNmeaSentence(char *buf, const size_t maxLength)
{
    memset(buf, 0x00, maxLength);
    bufPos  = 0;
    maxPos  = maxLength;
    dataBuf = buf;

    // Enable serial port
    USART6->CR1 |= USART_CR1_UE;

    return 0;
}

bool gps_nmeaSentenceReady()
{
    return (receiving == false) && (bufPos > 0);
}

void gps_waitForNmeaSentence()
{
    /*
     * Put the calling thread in waiting status until a complete sentence is ready.
     */
    {
        FastInterruptDisableLock dLock;
        gpsWaiting = Thread::IRQgetCurrentThread();
        do
        {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
        while(gpsWaiting);
    }
}
