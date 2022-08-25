/***************************************************************************
 *   Copyright (C) 2015 by Terraneo Federico                               *
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
#include "serial_efm32.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "interfaces/portability.h"
#include "interfaces/gpio.h"
#include "filesystem/ioctl.h"

using namespace std;
using namespace miosix;

static const int numPorts=1; //Supporting only USART0 for now

//Hope GPIO mapping doesn't change among EFM32 microcontrollers, otherwise
//we'll fix that with #ifdefs
typedef Gpio<GPIOE_BASE,10> u0tx;
typedef Gpio<GPIOE_BASE,11> u0rx;

/// Pointer to serial port classes to let interrupts access the classes
static EFM32Serial *ports[numPorts]={0};

/**
 * \internal interrupt routine for usart0 rx actual implementation
 */
void __attribute__((noinline)) usart0rxIrqImpl()
{
   if(ports[0]) ports[0]->IRQhandleInterrupt();
}

/**
 * \internal interrupt routine for usart0 rx
 */
void __attribute__((naked)) USART0_RX_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart0rxIrqImplv");
    restoreContext();
}

namespace miosix {

//
// class EFM32Serial
//

// A note on the baudrate/500: the buffer is selected so as to withstand
// 20ms of full data rate. In the 8N1 format one char is made of 10 bits.
// So (baudrate/10)*0.02=baudrate/500
EFM32Serial::EFM32Serial(int id, int baudrate)
        : Device(Device::TTY), rxQueue(rxQueueMin+baudrate/500), rxWaiting(0),
        portId(id), baudrate(baudrate)
{
    if(id<0 || id>=numPorts || ports[id]!=0) errorHandler(UNEXPECTED);
    
    {
        InterruptDisableLock dLock;
        ports[id]=this;
        
        switch(id)
        {
            case 0:
                u0tx::mode(Mode::OUTPUT_HIGH);
                u0rx::mode(Mode::INPUT_PULL_UP_FILTER);
                
                CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_USART0;
                port=USART0;
                
                port->IEN=USART_IEN_RXDATAV;
                port->IRCTRL=0; //USART0 also has IrDA mode
                
                NVIC_SetPriority(USART0_RX_IRQn,15);//Lowest priority for serial
                NVIC_EnableIRQ(USART0_RX_IRQn);
                break;
        }
    }
    
    port->CTRL=USART_CTRL_TXBIL_HALFFULL; //Use the buffer more efficiently
    port->FRAME=USART_FRAME_STOPBITS_ONE
            | USART_FRAME_PARITY_NONE
            | USART_FRAME_DATABITS_EIGHT;
    port->TRIGCTRL=0;
    port->INPUT=0;
    port->I2SCTRL=0;
    port->ROUTE=USART_ROUTE_LOCATION_LOC0 //Default location
              | USART_ROUTE_TXPEN         //Enable TX pin
              | USART_ROUTE_RXPEN;        //Enable RX pin
    unsigned int periphClock=SystemHFClockGet()/(1<<(CMU->HFPERCLKDIV & 0xf));
    //The number we need is periphClock/baudrate/16-1, but with two bits of
    //fractional part. We divide by 2 instead of 16 to have 3 bit of fractional
    //part. We use the additional fractional bit to add one to round towards
    //the nearest. This gets us a little more precision. Then we subtract 8
    //which is one with three fractional bits. Then we shift to fit the integer
    //part in bits 20:8 and the fractional part in bits 7:6, masking away the
    //third fractional bit. Easy, isn't it? Not quite.
    port->CLKDIV=((((periphClock/baudrate/2)+1)-8)<<5) & 0x1fffc0;
    port->CMD=USART_CMD_CLEARRX
            | USART_CMD_CLEARTX
            | USART_CMD_TXTRIDIS
            | USART_CMD_RXBLOCKDIS
            | USART_CMD_MASTEREN
            | USART_CMD_TXEN
            | USART_CMD_RXEN;
}

ssize_t EFM32Serial::readBlock(void *buffer, size_t size, off_t where)
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
        //Don't block if we have at least one char
        //This is required for \n detection
        if(result>0) break; 
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

ssize_t EFM32Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> l(txMutex);
    const char *buf=reinterpret_cast<const char*>(buffer);
    for(size_t i=0;i<size;i++)
    {
        while((port->STATUS & USART_STATUS_TXBL)==0) ;
        port->TXDATA=*buf++;
    }
    return size;
}

void EFM32Serial::IRQwrite(const char *str)
{
    // We can reach here also with only kernel paused, so make sure
    // interrupts are disabled.
    bool interrupts=areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();
    while(*str)
    {
        while((port->STATUS & USART_STATUS_TXBL)==0) ;
        port->TXDATA=*str++;
    }
    waitSerialTxFifoEmpty();
    if(interrupts) fastEnableInterrupts();
}

int EFM32Serial::ioctl(int cmd, void* arg)
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

void EFM32Serial::IRQhandleInterrupt()
{
    bool atLeastOne=false;
    while(port->STATUS & USART_STATUS_RXDATAV)
    {
        unsigned int c=port->RXDATAX;
        if((c & (USART_RXDATAX_FERR | USART_RXDATAX_PERR))==0)
        {
            atLeastOne=true;
            if(rxQueue.tryPut(c & 0xff)==false) /*fifo overflow*/;
        }
    }
    if(atLeastOne && rxWaiting)
    {
        rxWaiting->IRQwakeup();
        if(rxWaiting->IRQgetPriority()>
            Thread::IRQgetCurrentThread()->IRQgetPriority())
                Scheduler::IRQfindNextThread();
        rxWaiting=0;
    }
    
}

EFM32Serial::~EFM32Serial()
{
    waitSerialTxFifoEmpty();
    
    port->CMD=USART_CMD_TXDIS
            | USART_CMD_RXDIS;
    port->ROUTE=0;

    InterruptDisableLock dLock;
    ports[portId]=0;
    switch(portId)
    {
        case 0:
            NVIC_DisableIRQ(USART0_RX_IRQn);
            NVIC_ClearPendingIRQ(USART0_RX_IRQn);
            
            u0tx::mode(Mode::DISABLED);
            u0rx::mode(Mode::DISABLED);
            
            CMU->HFPERCLKEN0 &= ~CMU_HFPERCLKEN0_USART0;
            break;
    }
}

void EFM32Serial::waitSerialTxFifoEmpty()
{
    //The documentation states that the TXC bit goes to one as soon as a
    //transmission is complete. However, this bit is initially zero, so if we
    //call this function before transmitting, the loop will wait forever. As a
    //solution, add a timeout having as value the time needed to send three
    //bytes (the current one in the shift register plus the two in the buffer).
    //The +1 is to produce rounding on the safe side, the 30 is the time to send
    //three char through the port, including start and stop bits.
    int timeout=(SystemCoreClock/baudrate+1)*30;
    while(timeout-->0 && (port->STATUS & USART_STATUS_TXC)==0) ;
}

} //namespace miosix
