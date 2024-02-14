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
#include "board_settings.h"

using namespace std;

//
// Interrupts
//

static unsigned char digits[4]={0};

void TIM3_IRQHandler()
{
    TIM3->SR=0; //Clear IRQ flag
	static int i=0;
    GPIOB->ODR=(0x0f00 & ~(1<<(i+8))) | digits[i];
    if(++i>=4) i=0;
}

namespace miosix {

//
// LED Display driver
//

static void initDisplay()
{
    //Start the IRQ that will drive the 4 digit LED display
    TIM3->DIER=TIM_DIER_UIE;
    TIM3->CNT=0;
    TIM3->PSC=0;
    TIM3->ARR=60000; //24MHz/60000=400Hz 
    TIM3->CR1=TIM_CR1_CEN;
    NVIC_SetPriority(TIM3_IRQn,15); //Lowest priority for timer IRQ
    NVIC_EnableIRQ(TIM3_IRQn);
}

void clearDisplay()
{
    memset(digits,0,4);
}

void showNumber(float number)
{
    static const unsigned char plusErr[]= {0x00,0x79,0x50,0x50}; // " Err"
    static const unsigned char minusErr[]={0x40,0x79,0x50,0x50}; // "-Err"
    static const unsigned char digitTbl[]=
    {
    //  0    1    2    3    4    5    6    7    8    9
        0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
    };
    int num=static_cast<int>(number*100.0f + (number>=0.0f ? 0.5f : -0.5f));
    //Last digit is 4 to account for rounding number that don't fit in 4 digits
    if(num>99994)      memcpy(digits,plusErr,4);
    else if(num<-9994) memcpy(digits,minusErr,4);
    else {
        int decimal=1;
        if(num>9999)      { decimal=2; num=(num+5)/10; } //+5 is for rounding
        else if(num<-999) { decimal=2; num=(num-5)/10; } //-5 is for rounding
        //Now num is always in the range -999 to 9999, make it positive
        if(num<0) num=-num;
        unsigned char temp[4];
        temp[3]=digitTbl[num % 10]; num/=10;
        temp[2]=digitTbl[num % 10]; num/=10;
        temp[1]=digitTbl[num % 10]; num/=10;
        if(number<0) temp[0]=0x40; // "-"
        else if(num>0) temp[0]=digitTbl[num]; else temp[0]=0x00; // " "
        temp[decimal] |= 0x80; //Add '.'
        memcpy(digits,temp,4);
    }
}

void showLowVoltageIndicator()
{
    static const char bat[]={0x7c,0x77,0x78,0x0}; //"bAt "
    memcpy(digits,bat,4);
}

//
// AD7789 driver
//

unsigned char spi1sendRecv(unsigned char x)
{
    SPI1->DR=x;
    while((SPI1->SR & SPI_SR_RXNE)==0) ;
    return SPI1->DR;
}

static void initAdc()
{
    SPI1->CR1=SPI_CR1_SSM  //No HW cs
            | SPI_CR1_SSI
            | SPI_CR1_SPE  //SPI enabled
            | SPI_CR1_BR_0 //SPI clock 24/4=6 MHz
            | SPI_CR1_MSTR //Master mode
            | SPI_CR1_CPOL //AD7789 datasheet specifies SCK default high
            | SPI_CR1_CPHA;//AD7789 datasheet specifies sampling on rising edge
    delayUs(1);
    cs::low();
    spi1sendRecv(0x10); //Write to mode register
    spi1sendRecv(0x06); //Continuous conversion, unipolar mode
    cs::high();
    delayUs(1);
}

unsigned char readStatusReg()
{
    cs::low();
    spi1sendRecv(0x08);
    unsigned char result=spi1sendRecv();
    cs::high();
    delayUs(1);
    return result;
}

unsigned int readAdcValue()
{
    cs::low();
    spi1sendRecv(0x38);
    unsigned int result=spi1sendRecv();
    result<<=8;
    result|=spi1sendRecv();
    result<<=8;
    result|=spi1sendRecv();
    cs::high();
    delayUs(1);
    return result;
}

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

bool NonVolatileStorage::program(const void* data, int size)
{
    const char *ptr=reinterpret_cast<const char *>(data);
    size=min(size,capacity());
    
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
                reinterpret_cast<volatile unsigned short*>(baseAddress+i);
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
    //Enable all gpios, as well as AFIO, SPI1, TIM3
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
                    RCC_APB2ENR_AFIOEN | RCC_APB2ENR_SPI1EN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN | RCC_APB1ENR_PWREN;
    RCC_SYNC();

    //Board has no JTAG nor SWD, and those pins are used
    //HSE is not used, remap PD0/PD1 in order to avoid leaving them floating
    AFIO->MAPR |= AFIO_MAPR_SWJ_CFG_2 | AFIO_MAPR_PD01_REMAP;

    //Note: all OUT pins speed limited to 2MHz except SPI and UART, that are
    //limited to 10MHz. This has been done to reduce power supply "noise"
    //PA5 AF out, PA6 AF out, PA7 in pulldown, PA9 AF out, PA10 in pu, other OUT
    GPIOA->CRL=0x98912222;
    GPIOA->CRH=0x22222892;
    GPIOA->ODR=0x0410;     //Enable pullup on PA10, and set PA4 high (SPI CS)
    GPIOB->CRL=0x22222222; //Port B : all out
    GPIOB->CRH=0x22222222;
    GPIOB->ODR=0x0f00;     //Keep display off at boot
    GPIOC->CRH=0x22222222; //PC13 through PC15: all out
    GPIOD->CRL=0x22222222; //PD0 and PD1 all out
    
    initAdc();
    configureLowVoltageDetect();

    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new STM32Serial(defaultSerial,defaultSerialSpeed,
        defaultSerialFlowctrl ? STM32Serial::RTSCTS : STM32Serial::NOFLOWCTRL)));
    
    //The serial port drver reconfigures PA9 to 50MHz AF out and PA10 to
    //floating in, but we want them as configured previously, so override
    GPIOA->CRH=0x22222892;
    GPIOA->ODR=0x0410;     //Enable pullup on PA10, and set PA4 high (SPI CS)
}

void bspInit2()
{
    initDisplay();
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
