/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IUNUO,                     *
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

#include <kernel/scheduler/scheduler.h>
#include <kernel/queue.h>
#include <miosix.h>
#include <at32f421.h>
#include "USART1.h"

using namespace miosix;

static constexpr int rxQueueMin = 16; //  Minimum queue size

static DynUnsyncQueue< char > rxQueue(128);  // Queue for incoming data
static Thread   *rxWaiting = 0;              // Thread waiting on RX
static bool      rxIdle    = true;           // Flag for RX idle
static FastMutex rxMutex;                    // Mutex locked during reception
static FastMutex txMutex;                    // Mutex locked during transmission

/**
 * \internal
 * Wait until all characters have been written to the serial port.
 * Needs to be callable from interrupts disabled (it is used in IRQwrite)
 */
static inline void waitSerialTxFifoEmpty()
{
    while((USART1->sts & (1 << 6)) == 0) ;
}

/**
 * \internal
 * Interrupt handler function, called by USART1_IRQHandler.
 */
void __attribute__((noinline)) usart1irqImpl()
{
    unsigned int status = USART1->sts;
    char c;

    // New character received
    if(status & (1 << 5))
    {
        //Always read data, since this clears interrupt flags
        c = USART1->dt;

        //If no error put data in buffer
        if((status & (1 << 1)) == 0)
        {
            if(rxQueue.tryPut(c) == false) {/*fifo overflow*/}
        }

        rxIdle = false;
    }

    // Idle line
    if(status & (1 << 4))
    {
        c = USART1->dt; //clears interrupt flags
        rxIdle = true;
    }

    // Enough data in buffer or idle line, awake thread
    if((status & (1 << 4)) || rxQueue.size() >= rxQueueMin)
    {
        if(rxWaiting)
        {
            rxWaiting->IRQwakeup();
            if(rxWaiting->IRQgetPriority()>
                Thread::IRQgetCurrentThread()->IRQgetPriority())
                    Scheduler::IRQfindNextThread();
            rxWaiting = 0;
        }
    }
}

void __attribute__((naked)) USART1_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart1irqImplv");
    restoreContext();
}


void usart1_init(unsigned int baudrate)
{
    CRM->apb2en |= (1 << 14);
    __DSB();

    // Get current frequency of APB2 clock
    unsigned int freq = SystemCoreClock;
    if(CRM->cfg_bit.apb2div != 0)
        freq /= ((CRM->cfg_bit.apb2div & 0x03) + 1);

    const unsigned int quot = 2*freq/baudrate; // 2*freq for round to nearest
    USART1->baudr = quot/2 + (quot & 1);       // Round to nearest
    USART1->ctrl1 = (1 << 13)                  // Enable port
                  | (1 <<  5)                  // Interrupt on data received
                  | (1 <<  4)                  // Interrupt on idle line
                  | (1 <<  3)                  // Transmission enbled
                  | (1 <<  2);                 // Reception enabled

    NVIC_SetPriority(USART1_IRQn, 15);         // Lowest priority for serial
    NVIC_EnableIRQ(USART1_IRQn);
}

void usart1_terminate()
{
    waitSerialTxFifoEmpty();

    NVIC_DisableIRQ(USART1_IRQn);

    USART1->ctrl1 &= ~(1 << 13);
    CRM->apb2en   &= ~(1 << 14);
    __DSB();
}

ssize_t usart1_readBlock(void *buffer, size_t size, off_t where)
{
    (void) where;

    miosix::Lock< miosix::FastMutex > l(rxMutex);
    char *buf = reinterpret_cast< char* >(buffer);
    size_t result = 0;
    FastInterruptDisableLock dLock;

    for(;;)
    {
        //Try to get data from the queue
        for(; result < size; result++)
        {
            if(rxQueue.tryGet(buf[result])==false) break;
            //This is here just not to keep IRQ disabled for the whole loop
            FastInterruptEnableLock eLock(dLock);
        }
        if(rxIdle && result > 0) break;
        if(result == size) break;
        //Wait for data in the queue
        do {
            rxWaiting = Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        } while(rxWaiting);
    }

    return result;
}

ssize_t usart1_writeBlock(void *buffer, size_t size, off_t where)
{
    (void) where;

    miosix::Lock< miosix::FastMutex > l(txMutex);
    const char *buf = reinterpret_cast< const char* >(buffer);
    for(size_t i = 0; i < size; i++)
    {
        while((USART1->sts & (1 << 7)) == 0) ;
        USART1->dt = *buf++;
    }

    return size;
}

void usart1_IRQwrite(const char *str)
{
    // We can reach here also with only kernel paused, so make sure
    // interrupts are disabled. This is important for the DMA case
    bool interrupts = areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();

    while(*str)
    {
        while((USART1->sts & (1 << 7)) == 0) ;
        USART1->dt = *str++;
    }

    waitSerialTxFifoEmpty();
    if(interrupts) fastEnableInterrupts();
}
