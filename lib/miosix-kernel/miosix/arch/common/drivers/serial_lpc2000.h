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

#ifndef SERIAL_LPC2000_H
#define SERIAL_LPC2000_H

#include "filesystem/console/console_device.h"
#include "kernel/sync.h"
#include "kernel/queue.h"
#include "interfaces/delays.h"

namespace miosix {

/**
 * Serial port class for LPC2000 microcontrollers.
 * 
 * Classes of this type are reference counted, must be allocated on the heap
 * and managed through intrusive_ref_ptr<FileBase>
 */
class LPC2000Serial : public Device
{
public:
    /**
     * Constructor, initializes the serial port.
     * Calls errorHandler(UNEXPECTED) if id is not in the correct range, or when
     * attempting to construct multiple objects with the same id. That is,
     * it is possible to instantiate only one instance of this class for each
     * hardware USART.
     * \param id 0=USART0, 1=USART1
     * \param baudrate serial port baudrate.
     */
    LPC2000Serial(int id, int baudrate);
    
    /**
     * Read a block of data
     * \param buffer buffer where read data will be stored
     * \param size buffer size
     * \param where where to read from
     * \return number of bytes read or a negative number on failure
     */
    ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    /**
     * Write a block of data
     * \param buffer buffer where take data to write
     * \param size buffer size
     * \param where where to write to
     * \return number of bytes written or a negative number on failure
     */
    ssize_t writeBlock(const void *buffer, size_t size, off_t where);
    
    /**
     * Write a string.
     * An extension to the Device interface that adds a new member function,
     * which is used by the kernel on console devices to write debug information
     * before the kernel is started or in case of serious errors, right before
     * rebooting.
     * Can ONLY be called when the kernel is not yet started, paused or within
     * an interrupt. This default implementation ignores writes.
     * \param str the string to write. The string must be NUL terminated.
     */
    void IRQwrite(const char *str);
    
    /**
     * Performs device-specific operations
     * \param cmd specifies the operation to perform
     * \param arg optional argument that some operation require
     * \return the exact return value depends on CMD, -1 is returned on error
     */
    int ioctl(int cmd, void *arg);
    
    /**
     * \internal the serial port interrupts call this member function.
     * Never call this from user code.
     */
    void IRQhandleInterrupt();
    
    /**
     * Destructor
     */
    ~LPC2000Serial();
    
private:
    /**
     * Wait until all characters have been written to the serial port
     */
    void waitSerialTxFifoEmpty()
    {
        while((serial->LSR & (1<<6))==0) ;
        
        //This delay has been added to fix a quirk on the Miosix board. When
        //writing a message to the console and rebooting, if the reboot happens
        //too fast with respect to the last character sent out of the serial
        //port, the FT232 gets confused and the last charcters are lost,
        //probably from the FT232 buffer. Using delayMs() to be callable from IRQ
        delayMs(2);
    }
    
    /**
     * The registers of the USART in struct form, to make a generic driver
     */
    struct Usart16550
    {
        //Offset 0x00
        union {
            volatile unsigned char RBR;
            volatile unsigned char THR;
            volatile unsigned char DLL;
        };
        char padding0[3];
        //Offset 0x04
        union {
            volatile unsigned char IER;
            volatile unsigned char DLM;
        };
        char padding1[3];
        //Offset 0x08
        union {
            volatile unsigned char IIR;
            volatile unsigned char FCR;
        };
        char padding2[3];
        //Offset 0x0c
        volatile unsigned char LCR;
        char padding3[3];
        //Offset 0x10
        volatile unsigned char MCR; //Only USART1 has this
        char padding4[3];
        //Offset 0x14
        volatile unsigned char LSR;
        char padding5[7];
        //Offset 0x1c
        volatile unsigned char SCR;
        char padding6[19];
        //Offset 0x30
        volatile unsigned char TER;
    };
    
    //Configure the software queue here
    static const int swTxQueue=32;///< Size of tx software queue

    //The hardware queues cannot be modified, since their length is hardware-specific
    static const int hwTxQueueLen=16;
    static const int hwRxQueueLen=8;

    FastMutex txMutex;///< Mutex used to guard the tx queue
    FastMutex rxMutex;///< Mutex used to guard the rx queue

    DynUnsyncQueue<char>  txQueue;///< Rx software queue
    DynUnsyncQueue<char>  rxQueue;///< Rx software queue
    Thread *txWaiting;  ///< Thread waiting on rx queue
    Thread *rxWaiting;  ///< Thread waiting on rx queue
    bool idle;          ///< Receiver idle
    
    Usart16550 *serial; ///< Serial port registers
};

} //namespace miosix

#endif //SERIAL_LPC2000_H
