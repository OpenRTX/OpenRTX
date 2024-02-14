/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014                *
 *   by Terraneo Federico                                                  *
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

#include <cstring>
#include <errno.h>
#include <termios.h>
#include "serial_lpc2000.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "interfaces/portability.h"
#include "filesystem/ioctl.h"
#include "LPC213x.h"

namespace miosix {

/// Pointer to serial port classes to let interrupts access the classes
static LPC2000Serial *ports[2]={0};

/**
 * \internal interrupt routine for usart0 actual implementation
 */
void __attribute__((noinline)) usart0irqImpl()
{
    if(ports[0]) ports[0]->IRQhandleInterrupt();
    VICVectAddr=0xff;//Restart VIC
}

/**
 * \internal interrupt routine for usart0
 */
void __attribute__((interrupt("IRQ"),naked)) usart0irq()
{
    saveContextFromIrq();
	asm volatile("bl _ZN6miosix13usart0irqImplEv");
    restoreContext();
}

/**
 * \internal interrupt routine for usart1 actual implementation
 */
void __attribute__((noinline)) usart1irqImpl()
{
    if(ports[1]) ports[1]->IRQhandleInterrupt();
    VICVectAddr=0xff;//Restart VIC
}

/**
 * \internal interrupt routine for usart1
 */
void __attribute__ ((interrupt("IRQ"),naked)) usart1irq()
{
    saveContextFromIrq();
	asm volatile("bl _ZN6miosix13usart1irqImplEv");
    restoreContext();
}

//
// class LPC2000Serial
//

// A note on the baudrate/500: the buffer is selected so as to withstand
// 20ms of full data rate. In the 8N1 format one char is made of 10 bits.
// So (baudrate/10)*0.02=baudrate/500
LPC2000Serial::LPC2000Serial(int id, int baudrate) : Device(Device::TTY),
        txQueue(swTxQueue), rxQueue(hwRxQueueLen+baudrate/500),
        txWaiting(nullptr), rxWaiting(nullptr), idle(true)
{
    InterruptDisableLock dLock;
    if(id<0 || id>1 || ports[id]!=0) errorHandler(UNEXPECTED);
    ports[id]=this;
    if(id==0)
    {
        serial=reinterpret_cast<Usart16550*>(0xe000c000);
        PCONP|=(1<<3);//Enable UART0 peripheral
        PINSEL0&=~(0xf);//Clear bits 0 to 3 of PINSEL0
        PINSEL0|=0x5;//Set p0.0 as TXD and p0.1 as RXD
        //Init VIC
        VICSoftIntClr=(1<<6);//Clear uart0 interrupt flag (if previously set)
        VICIntSelect&=~(1<<6);//uart0=IRQ
        VICIntEnable=(1<<6);//uart0 interrupt ON
        VICVectCntl2=0x20 | 0x6;//Slot 2 of VIC used by uart0
        VICVectAddr2=reinterpret_cast<unsigned int>(&usart0irq);
    } else {
        serial=reinterpret_cast<Usart16550*>(0xe0010000);
        PCONP|=(1<<4);//Enable UART1 peripheral
        PINSEL0&=~(0xf0000);//Clear bits 16 to 19 of PINSEL0
        PINSEL0|=0x50000;//Set p0.8 as TXD and p0.9 as RXD
        //Init VIC
        VICSoftIntClr=(1<<7);//Clear uart1 interrupt flag (if previously set)
        VICIntSelect&=~(1<<7);//uart1=IRQ
        VICIntEnable=(1<<7);//uart1 interrupt ON
        VICVectCntl3=0x20 | 0x7;//Slot 3 of VIC used by uart1
        VICVectAddr3=reinterpret_cast<unsigned int>(&usart1irq);
    }
    serial->LCR=0x3;//DLAB disabled
    //0x07= fifo enabled, reset tx and rx hardware fifos
    //0x80= uart rx fifo trigger level 8 characters
    serial->FCR=0x07 | 0x80;
    serial->LCR=0x83;//8data bit, 1stop, no parity, DLAB enabled
    int div=TIMER_CLOCK/16/baudrate;
    serial->DLL=div & 0xff;
    serial->DLM=(div>>8) & 0xff;
    serial->LCR=0x3;//DLAB disabled
    serial->IER=0x7;//Enable RLS, RDA, CTI and THRE interrupts
}

ssize_t LPC2000Serial::readBlock(void *buffer, size_t size, off_t where)
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

ssize_t LPC2000Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> l(txMutex);
    FastInterruptDisableLock dLock;
    size_t len=size;
    const char *buf=reinterpret_cast<const char*>(buffer);
    while(len>0)
    {
        //If no data in software and hardware queue
        if((serial->LSR & (1<<5)) && (txQueue.isEmpty()))
        {
            //Fill hardware queue first
            for(int i=0;i<hwTxQueueLen;i++)
            {
                serial->THR=*buf++;
                len--;
                if(len==0) break;
            }
        } else {
            if(txQueue.tryPut(*buf))
            {
                buf++;
                len--;
            } else {
                txWaiting=Thread::IRQgetCurrentThread();
                while(txWaiting) Thread::IRQenableIrqAndWait(dLock);
            }
        }
    }
    return size;
}

void LPC2000Serial::IRQwrite(const char *str)
{
    while((*str)!='\0')
    {
        //Wait until the hardware fifo is ready to accept one char
        while(!(serial->LSR & (1<<5))) ; //wait
        serial->THR=*str++;
    }
    waitSerialTxFifoEmpty();
}

int LPC2000Serial::ioctl(int cmd, void* arg)
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

void LPC2000Serial::IRQhandleInterrupt()
{
    char c;
    bool hppw=false;
    bool wakeup=false;
    switch(serial->IIR & 0xf)
    {
        case 0x6: //RLS
            c=serial->LSR;//Read LSR to clear interrupt
            c=serial->RBR;//Read RBR to discard char that caused error
            break;
        case 0x4: //RDA
            //Note: read one less char than HARDWARE_RX_QUEUE_LENGTH as the
            //CTI interrupt only occurs if there's at least one character in
            //the buffer, and we always want the CTI interrupt
            for(int i=0;i<hwRxQueueLen-1;i++)
                if(rxQueue.tryPut(serial->RBR)==false) /*fifo overflow*/;
            wakeup=true;
            idle=false;
            break;
        case 0xc: //CTI
            while(serial->LSR & (1<<0))
                if(rxQueue.tryPut(serial->RBR)==false) /*fifo overflow*/;
            wakeup=true;
            idle=true;
            break;
        case 0x2: //THRE
            for(int i=0;i<hwTxQueueLen;i++)
            {
                if(txWaiting)
                {
                    txWaiting->IRQwakeup();
                    if(txWaiting->IRQgetPriority()>
                        Thread::IRQgetCurrentThread()->IRQgetPriority()) hppw=true;
                    txWaiting=nullptr;
                }
                if(txQueue.tryGet(c)==false) break; //If software queue empty, stop
                serial->THR=c;
            }
            break;
    }
    if(wakeup && rxWaiting)
    {
        rxWaiting->IRQwakeup();
        if(rxWaiting->IRQgetPriority()>
                Thread::IRQgetCurrentThread()->IRQgetPriority()) hppw=true;
        rxWaiting=nullptr;
    }
    if(hppw) Scheduler::IRQfindNextThread();
}

LPC2000Serial::~LPC2000Serial()
{
    waitSerialTxFifoEmpty();
    
    InterruptDisableLock dLock;
    //Disable UART0
    serial->LCR=0;//DLAB disabled
    serial->FCR=0;
    
    int id=0;
    if(ports[0]==this) id=0;
    else if(ports[1]==this) id=1;
    else errorHandler(UNEXPECTED);
    ports[id]=0;
    
    if(id==0)
    {
        //Disable VIC
        VICIntEnClr=(1<<6);
        //Disable PIN
        PINSEL0&=~(0xf);//Clear bits 0 to 3 of PINSEL0
    } else {
        //Disable VIC
        VICIntEnClr=(1<<7);
        //Disable PIN
        PINSEL0&=~(0xf0000);//Clear bits 16 to 19 of PINSEL0
    }
}

} //namespace miosix
