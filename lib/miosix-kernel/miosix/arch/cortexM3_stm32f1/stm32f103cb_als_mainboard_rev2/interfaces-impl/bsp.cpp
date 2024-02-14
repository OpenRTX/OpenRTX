/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014, 2015 by Terraneo Federico             *
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


#include <cstdlib>
#include <inttypes.h>
#include <sys/ioctl.h>
#include "interfaces/bsp.h"
#include "kernel/kernel.h"
#include "kernel/sync.h"
#include "interfaces/delays.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "config/miosix_settings.h"
#include "kernel/logging.h"
#include "filesystem/file_access.h"
#include "filesystem/console/console_device.h"
#include "drivers/serial.h"
#include "drivers/dcc.h"
#include "board_settings.h"

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    //Enable all GPIOs
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN
                  | RCC_APB2ENR_IOPBEN
                  | RCC_APB2ENR_IOPCEN
                  | RCC_APB2ENR_IOPDEN
                  | RCC_APB2ENR_AFIOEN;
    RCC_SYNC();
    //Port config (H=high, L=low, PU=pullup, PD=pulldown)
	//  |  PORTA       |  PORTB      |  PORTC  |  PORTD  |
	//--+--------------+-------------+---------+---------+
	// 0| IN           | IN PD       | -       | IN PD   |
	// 1| IN PU        | IN PD       | -       | IN PD   |
	// 2| IN           | IN PD       | -       | -       |
	// 3| IN PD        | IN PD       | -       | -       |
	// 4| OUT H  10MHz | IN PD       | -       | -       |
	// 5| AF5    10MHz | IN PD       | -       | -       |
	// 6| AF5    10MHz | IN PD       | -       | -       |
	// 7| AF5    10MHz | IN PD       | -       | -       |
	// 8| OUT L  10MHz | IN PD       | -       | -       |
	// 9| AF7   400KHz | IN PD       | -       | -       |
	//10| IN PU        | IN PD       | -       | -       | PA10 was AF7PU
	//11| IN PD        | OUT L 10MHz | -       | -       |
	//12| IN PD        | OUT L 10MHz | -       | -       |
	//13| OUT L 400KHz | OUT L 10MHz | IN PD   | -       |
	//14| OUT H 400KHz | OUT L 10MHz | AF0     | -       |
	//15| OUT L 400KHz | OUT L 10MHz | AF0     | -       |
    
    GPIOA->CRL=0x99918484;
    GPIOA->CRH=0x222888a1;
    GPIOB->CRL=0x88888888;
    GPIOB->CRH=0x11111888;
    GPIOC->CRH=0x99844444;
    GPIOD->CRL=0x44444488;
    
    GPIOA->ODR=0x4412;
    GPIOB->ODR=0x0000;
    GPIOC->ODR=0x0000;
    GPIOD->ODR=0x0000;
    
    AFIO->MAPR=AFIO_MAPR_SWJ_CFG_2 | AFIO_MAPR_PD01_REMAP;
    
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    RCC_SYNC();
    PWR->CR |= PWR_CR_DBP     //Enable access to RTC registers
             | PWR_CR_PLS_0   //Select 2.3V trigger point for low battery
             | PWR_CR_PVDE    //Enable low battery detection
             | PWR_CR_LPDS;   //Put regulator in low power when entering stop
    RCC->BDCR |= RCC_BDCR_LSEON    //External 32KHz oscillator enabled
              | RCC_BDCR_RTCEN     //RTC enabled
              | RCC_BDCR_RTCSEL_0; //Select LSE as clock source for RTC
    while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1=SPI_CR1_SSM  //handle CS in software
           | SPI_CR1_SSI   //internal CS tied high
           | SPI_CR1_SPE   //SPI enabled (speed 8MHz)
           | SPI_CR1_MSTR; //Master mode

    ledOn();
    delayMs(100);
    ledOff();
    
    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new STM32Serial(defaultSerial,defaultSerialSpeed,
        defaultSerialFlowctrl ? STM32Serial::RTSCTS : STM32Serial::NOFLOWCTRL)));
}

void bspInit2()
{
    //No SDIO peripheral in medium-density stm32, so no filesystem
}

static void spi1send(unsigned char data)
{
    SPI1->DR=data;
    while((SPI1->SR & SPI_SR_RXNE)==0) ; //Wait
}

//
// Shutdown and reboot
//

void shutdown()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    disableInterrupts();
    
    //Put outputs in low power mode
    led::low();
    hpled::high();
    sen::low();
    
    //Put cam in low power mode
    cam::en::low();
    cam::cs::mode(Mode::INPUT);   cam::cs::pulldown();
    cam::sck::mode(Mode::INPUT);  cam::sck::pulldown();
    cam::miso::mode(Mode::INPUT); cam::miso::pulldown();
    cam::mosi::mode(Mode::INPUT); cam::mosi::pulldown();
    
    //Put nrf in low power mode
    nrf::ce::low();
    nrf::cs::low();
    spi1send(0x20 | 0); //Write to reg 0
    spi1send(0x8);      //PWR_UP=0
    nrf::cs::high();
    nrf::miso::mode(Mode::INPUT); nrf::miso::pulldown(); //nrf miso goes tristate if cs high
    
    EXTI->IMR=0;                       //All IRQs masked
    EXTI->PR=0x7fffff;                 //Clear eventual pending request
    SCB->SCR |= SCB_SCR_SLEEPDEEP;     //Select stop mode
    __WFI(); //And it goes to sleep till a reset
    //Should never reach here
    miosix_private::IRQsystemReboot();
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

} //namespace miosix
