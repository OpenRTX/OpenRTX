/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <kernel/scheduler/scheduler.h>
#include "peripherals/gpio.h"
#include <kernel/queue.h>
#include <miosix.h>
#include "USART3.h"

#define U3TXD GPIOD,8
#define U3RXD GPIOD,8

using namespace miosix;

static constexpr int rxQueueMin = 16; //  Minimum queue size

DynUnsyncQueue< char > rxQueue(128);  // Queue for incoming data
Thread *rxWaiting = 0;                // Thread waiting on RX
bool   rxIdle     = true;             // Flag for RX idle
FastMutex rxMutex;                    // Mutex locked during reception
FastMutex txMutex;                    // Mutex locked during transmission

/**
 * \internal
 * Wait until all characters have been written to the serial port.
 * Needs to be callable from interrupts disabled (it is used in IRQwrite)
 */
inline void waitSerialTxFifoEmpty()
{
    while((USART3->SR & USART_SR_TC) == 0) ;
}

/**
 * \internal
 * Interrupt handler function, called by USART3_IRQHandler.
 */
void __attribute__((noinline)) usart3irqImpl()
{
    unsigned int status = USART3->SR;
    char c;

    if(status & USART_SR_RXNE)
    {
        //Always read data, since this clears interrupt flags
        c = USART3->DR;

        //If no error put data in buffer
        if((status & USART_SR_FE) == 0)
        {
            if(rxQueue.tryPut(c) == false) {/*fifo overflow*/}
        }

        rxIdle = false;
    }

    if(status & USART_SR_IDLE)
    {
        c = USART3->DR; //clears interrupt flags
        rxIdle = true;
    }

    if((status & USART_SR_IDLE) || rxQueue.size() >= rxQueueMin)
    {
        //Enough data in buffer or idle line, awake thread
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

void usart3_init(unsigned int baudrate)
{
    gpio_setMode(U3RXD, ALTERNATE | ALTERNATE_FUNC(7));
    gpio_setMode(U3TXD, ALTERNATE | ALTERNATE_FUNC(7));

    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    __DSB();

    unsigned int freq = SystemCoreClock;
    if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/= 1<<(((RCC->CFGR >> 10) & 0x3)+1);

    const unsigned int quot = 2*freq/baudrate; // 2*freq for round to nearest
    USART3->BRR  = quot/2 + (quot & 1);        // Round to nearest
    USART3->CR3 |= USART_CR3_ONEBIT;
    USART3->CR1  = USART_CR1_UE                // Enable port
                 | USART_CR1_RXNEIE            // Interrupt on data received
                 | USART_CR1_IDLEIE            // Interrupt on idle line
                 | USART_CR1_TE                // Transmission enbled
                 | USART_CR1_RE;               // Reception enabled

    NVIC_SetPriority(USART3_IRQn,15);          // Lowest priority for serial
    NVIC_EnableIRQ(USART3_IRQn);
}

void usart3_terminate()
{
    waitSerialTxFifoEmpty();

    NVIC_DisableIRQ(USART3_IRQn);

    USART3->CR1  &= ~USART_CR1_UE;
    RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN;
    __DSB();
}

ssize_t usart3_readBlock(void *buffer, size_t size, off_t where)
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

ssize_t usart3_writeBlock(void *buffer, size_t size, off_t where)
{
    (void) where;

    miosix::Lock< miosix::FastMutex > l(txMutex);
    const char *buf = reinterpret_cast< const char* >(buffer);
    for(size_t i = 0; i < size; i++)
    {
        while((USART3->SR & USART_SR_TXE) == 0) ;
        USART3->DR = *buf++;
    }

    return size;
}

void usart3_IRQwrite(const char *str)
{
    // We can reach here also with only kernel paused, so make sure
    // interrupts are disabled. This is important for the DMA case
    bool interrupts = areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();

    while(*str)
    {
        while((USART3->SR & USART_SR_TXE) == 0) ;
        USART3->DR = *str++;
    }

    waitSerialTxFifoEmpty();
    if(interrupts) fastEnableInterrupts();
}
