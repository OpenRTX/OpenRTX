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
#include "gd32f3x0.h"
#include "../platform/mcu/CMSIS/Device/GD/GD32F3x0/Include/gd32f3x0_usart.h"
#include "USART0.h"

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
//    while((USART0-> & (1 << 6)) == 0) ;
    while(usart_flag_get((uint32_t)USART0, USART_FLAG_TBE) == RESET);
}

/**
 * \internal
 * Interrupt handler function, called by USART0_IRQHandler.
 */
void __attribute__((noinline)) usart0irqImpl()
{
    char c;

    // New character received
    if(usart_flag_get(USART0, USART_FLAG_RBNE))
    {
        //Always read data, since this clears interrupt flags
        c = usart_data_receive(USART0);

        //If no error put data in buffer
        if(usart_flag_get(USART0, USART_FLAG_ORERR) == RESET)
        {
            if(rxQueue.tryPut(c) == false) {/*fifo overflow*/}
        }

        rxIdle = false;
    }

    // Idle line
    if(usart_flag_get(USART0, USART_FLAG_IDLE))
    {
        // Clear interrupt flags
        usart_flag_clear(USART0, USART_FLAG_IDLE);
        rxIdle = true;
    }

    // Enough data in buffer or idle line, awake thread
    if(usart_flag_get(USART0, USART_FLAG_RBNE) == RESET ||
       rxQueue.size() >= rxQueueMin)
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

void __attribute__((naked)) USART0_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart0irqImplv");
    restoreContext();
}

void usart0_init(unsigned int baudrate)
{
    rcu_periph_clock_enable(RCU_USART0);
 
    usart_deinit(USART0);

    usart_baudrate_set(USART0, baudrate);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_baudrate_set(USART0, 115200U);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    NVIC_SetPriority(USART0_IRQn, 15);         // Lowest priority for serial
    NVIC_EnableIRQ(USART0_IRQn);
}

void usart0_terminate()
{
    waitSerialTxFifoEmpty();

    NVIC_DisableIRQ(USART0_IRQn);

    //USART0->ctrl1 &= ~(1 << 13);
    usart_disable(USART0);
    //CRM->apb2en   &= ~(1 << 14);
    rcu_periph_clock_disable(RCU_USART0);
    __DSB();
}

ssize_t usart0_readBlock(void *buffer, size_t size, off_t where)
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

ssize_t usart0_writeBlock(void *buffer, size_t size, off_t where)
{
    (void) where;

    miosix::Lock< miosix::FastMutex > l(txMutex);
    const char *buf = reinterpret_cast< const char* >(buffer);
    for(size_t i = 0; i < size; i++)
    {
        while(usart_flag_get(USART0, USART_FLAG_TBE) == RESET);
        USART_TDATA(USART0) = *buf++;
    }

    return size;
}

void usart0_IRQwrite(const char *str)
{
    // We can reach here also with only kernel paused, so make sure
    // interrupts are disabled. This is important for the DMA case
    bool interrupts = areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();

    while(*str)
    {
        while(usart_flag_get(USART0, USART_FLAG_TBE) == RESET);
        USART_TDATA(USART0) = *str++;
    }

    waitSerialTxFifoEmpty();
    if(interrupts) fastEnableInterrupts();
}
