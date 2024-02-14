/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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

/***********************************************************************
 * bsp.cpp Part of the Miosix Embedded OS.
 * Board support package, this file initializes hardware.
 ************************************************************************/

#include <utility>
#include <sys/ioctl.h>
#include "interfaces/bsp.h"
#include "interfaces/delays.h"
#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"
#include "filesystem/file_access.h"
#include "filesystem/console/console_device.h"
#include "drivers/serial.h"
#include "drivers/stm32_rtc.h"
#include "board_settings.h"

using namespace std;

namespace miosix {

//
// PVD driver
//

static void configureLowVoltageDetect()
{
    PWR->CR |= PWR_CR_PVDE   //Low voltage detect enabled
             | PWR_CR_PLS_1
             | PWR_CR_PLS_0; //2.5V threshold
}

bool lowVoltageCheck()
{
    return (PWR->CSR & PWR_CSR_PVDO) ? false : true;
}

//
// class NonVolatileStorage
//

NonVolatileStorage& NonVolatileStorage::instance()
{
    static NonVolatileStorage singleton;
    return singleton;
}

bool NonVolatileStorage::erase()
{
    FastInterruptDisableLock dLock;
    if(IRQunlock()==false) return false;
    
    while(FLASH->SR & FLASH_SR_BSY) ;
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR=baseAddress;
    FLASH->CR |= FLASH_CR_STRT;
    while(FLASH->SR & FLASH_SR_BSY) ;
    FLASH->CR &= ~FLASH_CR_PER;
    
    FLASH->CR |= FLASH_CR_LOCK;
    
    for(int i=0;i<capacity();i++)
        if(*reinterpret_cast<unsigned char*>(baseAddress+i)!=0xff) return false;
    return true;
}

bool NonVolatileStorage::program(const void* data, int size, int offset)
{
    if(size<=0 || offset<0) return false;
    const char *ptr=reinterpret_cast<const char *>(data);
    size=min(size,capacity()-offset);
    
    FastInterruptDisableLock dLock;
    if(IRQunlock()==false) return false;
    
    bool result=true;
    for(int i=0;i<size;i+=2)
    {
        unsigned short a,b;
        //If size is odd, pad with an 0xff as we can only write halfwords
        a=(i==size-1) ? 0xff : ptr[i+1];
        b=ptr[i];
        //Note: bytes swapped to account for the cpu being little-endian
        unsigned short val=(a<<8) | b;
        volatile unsigned short *target=
                reinterpret_cast<volatile unsigned short*>(baseAddress+offset+i);
        while(FLASH->SR & FLASH_SR_BSY) ;
        FLASH->CR |= FLASH_CR_PG;
        *target=val;
        while(FLASH->SR & FLASH_SR_BSY) ;
        FLASH->CR &= ~FLASH_CR_PG;
        if(*target!=val) result=false;
    }
    
    FLASH->CR |= FLASH_CR_LOCK;
    return result;
}

void NonVolatileStorage::read(void* data, int size)
{
    size=min(size,capacity());
    memcpy(data,reinterpret_cast<void*>(baseAddress),size);
}

bool NonVolatileStorage::IRQunlock()
{
    if((FLASH->CR & FLASH_CR_LOCK)==0) return true;
    FLASH->KEYR=0x45670123;
    FLASH->KEYR=0xCDEF89AB;
    if((FLASH->CR & FLASH_CR_LOCK)==0) return true;
    return false;
}

//
// Initialization
//

void IRQbspInit()
{
    //Enable all gpios, as well as AFIO, SPI1
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
                    RCC_APB2ENR_AFIOEN;
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    RCC_SYNC();
    
    //All GPIOs default to input with pulldown
    GPIOA->CRL=0x88888888; GPIOA->CRH=0x88888888;
    GPIOB->CRL=0x88888888; GPIOB->CRH=0x88888888;
    GPIOC->CRH=0x88888888;
    GPIOD->CRL=0x88888888;
    
    redLed::mode(Mode::OUTPUT);
    yellowLed::mode(Mode::OUTPUT);
    ledOn();
    Rtc::instance(); //Starting the 32KHz oscillator takes time
    ledOff();

    configureLowVoltageDetect();

    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new STM32Serial(defaultSerial,defaultSerialSpeed,
        defaultSerialFlowctrl ? STM32Serial::RTSCTS : STM32Serial::NOFLOWCTRL)));
}

void bspInit2()
{
    //Nothing to do
}

//
// Shutdown and reboot
//

void shutdown()
{
    reboot(); //This board needs no shutdown support, so we reboot on shutdown
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

} //namespace miosix
