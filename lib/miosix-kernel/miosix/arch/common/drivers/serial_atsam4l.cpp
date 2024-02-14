/***************************************************************************
 *   Copyright (C) 2020 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <limits>
#include <cstring>
#include <errno.h>
#include <termios.h>
#include "serial_atsam4l.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "interfaces/portability.h"
#include "filesystem/ioctl.h"

using namespace std;
using namespace miosix;

static const int numPorts=4; //USART0 to USART3

/// Pointer to serial port classes to let interrupts access the classes
static ATSAMSerial *ports[numPorts]={0};

/**
 * \internal interrupt routine for usart2 rx actual implementation
 */
void __attribute__((noinline)) usart2rxIrqImpl()
{
   if(ports[2]) ports[2]->IRQhandleInterrupt();
}

/**
 * \internal interrupt routine for usart2 rx
 */
void __attribute__((naked)) USART2_Handler()
{
    saveContext();
    asm volatile("bl _Z15usart2rxIrqImplv");
    restoreContext();
}

namespace miosix {

//
// class ATSAMSerial
//

// A note on the baudrate/500: the buffer is selected so as to withstand
// 20ms of full data rate. In the 8N1 format one char is made of 10 bits.
// So (baudrate/10)*0.02=baudrate/500
ATSAMSerial::ATSAMSerial(int id, int baudrate)
    : Device(Device::TTY), rxQueue(rxQueueMin+baudrate/500), rxWaiting(0),
    idle(true), portId(id)
{
    if(id!=2 || ports[portId]) errorHandler(UNEXPECTED);
    
    {
        InterruptDisableLock dLock;
        ports[portId]=this;
        
        port=USART2;
        
        //TODO: USART2 hardcoded
        PM->PM_UNLOCK = PM_UNLOCK_KEY(0xaa) | PM_UNLOCK_ADDR(PM_PBAMASK_OFFSET);
        PM->PM_PBAMASK |= PM_PBAMASK_USART2;
        NVIC_SetPriority(USART2_IRQn,15);//Lowest priority for serial
        NVIC_EnableIRQ(USART2_IRQn);
    }

    port->US_BRGR = ((SystemCoreClock/baudrate/4)+1)/2; //TODO: fractional part
    port->US_MR = US_MR_FILTER      // Filter input with majority of 3 samples
                | US_MR_OVER        // 8 cycles oversample
                | US_MR_PAR_NONE    // No parity
                | US_MR_CHRL_8      // 8 bit char
                | US_MR_USCLKS_MCK  // CLK_USART is clock source
                | US_MR_MODE_NORMAL;// Just a plain usart, please

    port->US_RTOR=10; //Timeout 10 bits (one char time)
    port->US_IER = US_IER_RXRDY | US_IER_TIMEOUT;

    port->US_CR = US_CR_TXEN | US_CR_RXEN;
}

ssize_t ATSAMSerial::readBlock(void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> l(rxMutex);
    char *buf=reinterpret_cast<char*>(buffer);
    size_t result=0;
    FastInterruptDisableLock dLock;
    for(;;)
    {
        //Try to get data from the queue
        for(;result<size;result++)
        {
            if(rxQueue.tryGet(buf[result])==false) break;
            //This is here just not to keep IRQ disabled for the whole loop
            FastInterruptEnableLock eLock(dLock);
        }
        if(idle && result>0) break;
        if(result==size) break;
        //Wait for data in the queue
        do {
            rxWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        } while(rxWaiting);
    }
    return result;
}

ssize_t ATSAMSerial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> l(txMutex);
    const char *buf=reinterpret_cast<const char*>(buffer);
    for(size_t i=0;i<size;i++)
    {
        while((port->US_CSR & US_CSR_TXRDY) == 0) ;
        port->US_THR =*buf++;
    }
    return size;
}

void ATSAMSerial::IRQwrite(const char *str)
{
    // We can reach here also with only kernel paused, so make sure
    // interrupts are disabled.
    bool interrupts=areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();
    while(*str)
    {
        while((port->US_CSR & US_CSR_TXRDY) == 0) ;
        port->US_THR = *str++;
    }
    waitSerialTxFifoEmpty();
    if(interrupts) fastEnableInterrupts();
}

int ATSAMSerial::ioctl(int cmd, void *arg)
{
    if(reinterpret_cast<unsigned>(arg) & 0b11) return -EFAULT; //Unaligned
    termios *t=reinterpret_cast<termios*>(arg);
    switch(cmd)
    {
        case IOCTL_SYNC:
            waitSerialTxFifoEmpty();
            return 0;
        case IOCTL_TCGETATTR:
            t->c_iflag=IGNBRK | IGNPAR;
            t->c_oflag=0;
            t->c_cflag=CS8;
            t->c_lflag=0;
            return 0;
        case IOCTL_TCSETATTR_NOW:
        case IOCTL_TCSETATTR_DRAIN:
        case IOCTL_TCSETATTR_FLUSH:
            //Changing things at runtime unsupported, so do nothing, but don't
            //return error as console_device.h implements some attribute changes
            return 0;
        default:
            return -ENOTTY; //Means the operation does not apply to this descriptor
    }
}

void ATSAMSerial::IRQhandleInterrupt()
{
    bool wake=false;
    unsigned int status=port->US_CSR;
    
    if(status & US_CSR_RXRDY)
    {
        wake=true;
        //Always read the char (resets flags), but put it in the queue only if
        //no framing error
        char c=port->US_RHR;
        if((status & US_CSR_FRAME) == 0)
        {
            if(rxQueue.tryPut(c & 0xff)==false) /*fifo overflow*/;
            idle=false;
        }
    }
    if(status & US_CSR_TIMEOUT)
    {
        wake=true;
        port->US_CR=US_CR_STTTO;
        idle=true;
    }
    
    if(wake && rxWaiting)
    {
        rxWaiting->IRQwakeup();
        if(rxWaiting->IRQgetPriority()>
            Thread::IRQgetCurrentThread()->IRQgetPriority())
                Scheduler::IRQfindNextThread();
        rxWaiting=0;
    }
}

ATSAMSerial::~ATSAMSerial()
{
    waitSerialTxFifoEmpty();
    
    port->US_CR = US_CR_TXDIS | US_CR_RXDIS;
    port->US_IDR = US_IDR_RXRDY | US_IDR_TIMEOUT;
    
    InterruptDisableLock dLock;
    ports[portId]=nullptr;
    
    //TODO: USART2 hardcoded
    NVIC_DisableIRQ(USART2_IRQn);
    NVIC_ClearPendingIRQ(USART2_IRQn);
    PM->PM_UNLOCK = PM_UNLOCK_KEY(0xaa) | PM_UNLOCK_ADDR(PM_PBAMASK_OFFSET);
    PM->PM_PBAMASK &= ~PM_PBAMASK_USART2;
}

void ATSAMSerial::waitSerialTxFifoEmpty()
{
    while((port->US_CSR & US_CSR_TXEMPTY) == 0) ;
}

} //namespace miosix
