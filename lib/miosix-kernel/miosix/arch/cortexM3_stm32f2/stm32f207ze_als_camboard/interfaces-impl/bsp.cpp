/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014 by Terraneo Federico                   *
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
    //Enable all gpios
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN;
    RCC_SYNC();
	//Port config (H=high, L=low, PU=pullup, PD=pulldown)
	//  |  PORTA  |  PORTB  |  PORTC  |  PORTD  |  PORTE  |  PORTF  |  PORTG  |
	//--+---------+---------+---------+---------+---------+---------+---------+
	// 0| IN PD   | IN PD   | OUT L   | AF12    | AF12    | AF12    | AF12    |
	// 1| IN PD   | IN PD   | IN      | AF12    | AF12    | AF12    | AF12    |
	// 2| IN PD   | IN      | IN      | IN PD   | IN PD   | AF12    | AF12    |
	// 3| IN PD   | AF5     | IN PD   | IN PD   | IN PD   | AF12    | AF12    |
	// 4| AF13    | AF5     | IN PD   | AF12    | AF13    | AF12    | AF12    |
	// 5| IN PD   | AF5     | IN PD   | AF12    | AF13    | AF12    | AF12    |
	// 6| AF13    | AF13    | AF13    | IN      | AF13    | IN PD   | IN PD   |
	// 7| IN PD   | AF13    | AF13    | AF12    | AF12    | IN PD   | IN PD   |
	// 8| AF0     | OUT H   | AF13    | AF12    | AF12    | IN PD   | IN PD   |
	// 9| AF7     | IN PD   | AF13    | AF12    | AF12    | IN PD   | IN PD   |
	//10| AF7     | IN PD   | AF6     | AF12    | AF12    | IN PD   | IN PD   |
	//11| IN PD   | IN PD   | AF6     | AF12    | AF12    | IN PD   | IN PD   |
	//12| IN PD   | IN PD   | AF6     | AF12    | AF12    | AF12    | IN PD   |
	//13| AF0 PU  | IN PD   | IN      | AF12    | AF12    | AF12    | IN PD   |
	//14| AF0 PD  | IN PD   | IN PD   | AF12    | AF12    | AF12    | IN PD   |
	//15| AF6 PU  | IN PD   | IN PD   | AF12    | AF12    | AF12    | IN PD   |
    
    GPIOA->OSPEEDR=0xaaaaaaaa; //Default to 50MHz speed for all GPIOS
    GPIOB->OSPEEDR=0xaaaaaaaa; //Except SRAM GPIOs that run @ 100MHz
    GPIOC->OSPEEDR=0xaaaaaaaa;
    GPIOD->OSPEEDR=0xffffefaf;
    GPIOE->OSPEEDR=0xffffeaaf;
    GPIOF->OSPEEDR=0xffaaafff;
    GPIOG->OSPEEDR=0xaaaaafff;
    
    GPIOA->MODER=0xa82a2200;
    GPIOB->MODER=0x0001aa80;
    GPIOC->MODER=0x02aaa001;
    GPIOD->MODER=0xaaaa8a0a;
    GPIOE->MODER=0xaaaaaa0a;
    GPIOF->MODER=0xaa000aaa;
    GPIOG->MODER=0x00000aaa;
    
    GPIOA->PUPDR=0x668088aa;
    GPIOB->PUPDR=0xaaa8000a;
    GPIOC->PUPDR=0xa0000a80;
    GPIOD->PUPDR=0x000000a0;
    GPIOE->PUPDR=0x000000a0;
    GPIOF->PUPDR=0x00aaa000;
    GPIOG->PUPDR=0xaaaaa000;
    
    GPIOA->ODR=0x00000000;
    GPIOB->ODR=0x00000100;
    GPIOC->ODR=0x00002000;
    GPIOD->ODR=0x00000000;
    GPIOE->ODR=0x00000000;
    GPIOF->ODR=0x00000000;
    GPIOG->ODR=0x00000000;
    
    GPIOA->AFR[0]= 0 |  0<<4 |  0<<8 |  0<<12 | 13<<16 |  0<<20 | 13<<24 |  0<<28;
    GPIOA->AFR[1]= 0 |  7<<4 |  7<<8 |  0<<12 |  0<<16 |  0<<20 |  0<<24 |  6<<28;
    GPIOB->AFR[0]= 0 |  0<<4 |  0<<8 |  5<<12 |  5<<16 |  5<<20 | 13<<24 | 13<<28;
    GPIOB->AFR[1]= 0 |  0<<4 |  0<<8 |  0<<12 |  0<<16 |  0<<20 |  0<<24 |  0<<28;
    GPIOC->AFR[0]= 0 |  0<<4 |  0<<8 |  0<<12 |  0<<16 |  0<<20 | 13<<24 | 13<<28;
    GPIOC->AFR[1]=13 | 13<<4 |  6<<8 |  6<<12 |  6<<16 |  0<<20 |  0<<24 |  0<<28;
    GPIOD->AFR[0]=12 | 12<<4 |  0<<8 |  0<<12 | 12<<16 | 12<<20 |  0<<24 | 12<<28;
    GPIOD->AFR[1]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 | 12<<20 | 12<<24 | 12<<28;
    GPIOE->AFR[0]=12 | 12<<4 |  0<<8 |  0<<12 | 13<<16 | 13<<20 | 13<<24 | 12<<28;
    GPIOE->AFR[1]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 | 12<<20 | 12<<24 | 12<<28;
    GPIOF->AFR[0]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 | 12<<20 |  0<<24 |  0<<28;
    GPIOF->AFR[1]= 0 |  0<<4 |  0<<8 |  0<<12 | 12<<16 | 12<<20 | 12<<24 | 12<<28;
    GPIOG->AFR[0]=12 | 12<<4 | 12<<8 | 12<<12 | 12<<16 | 12<<20 |  0<<24 |  0<<28;
    GPIOG->AFR[1]= 0 |  7<<4 |  7<<8 | 10<<12 | 10<<16 |  0<<20 |  0<<24 |  0<<28;

    //Configure FSMC for IS62WC51216BLL-55
    RCC->AHB3ENR=RCC_AHB3ENR_FSMCEN;
    RCC_SYNC();
    volatile uint32_t& BCR1=FSMC_Bank1->BTCR[0];
    volatile uint32_t& BTR1=FSMC_Bank1->BTCR[1];
    volatile uint32_t& BWTR1=FSMC_Bank1E->BWTR[0];
    BCR1= FSMC_BCR1_EXTMOD //Extended mode
        | FSMC_BCR1_WREN   //Write enabled
        | FSMC_BCR1_MWID_0 //16 bit bus
        | FSMC_BCR1_MBKEN; //Bank enabled
    BTR1= FSMC_BTR1_DATAST_2  //DATAST=4
        | FSMC_BTR1_ADDSET_0  //ADDSET=3
        | FSMC_BTR1_ADDSET_1;
    //Read takes 7 + 2 (min time CS high) = 9 cycles
    BWTR1=FSMC_BWTR1_DATAST_2
        | FSMC_BWTR1_DATAST_0 //DATAST=5
        | FSMC_BWTR1_ADDSET_0;//ADDSET=1
    //Write takes 6 + 1 (WE high to CS high) + 1 (min time CS high) = 8 cycles 
    
    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
    #ifndef STDOUT_REDIRECTED_TO_DCC
        new STM32Serial(defaultSerial,defaultSerialSpeed,
        defaultSerialFlowctrl ? STM32Serial::RTSCTS : STM32Serial::NOFLOWCTRL)));
    #else //STDOUT_REDIRECTED_TO_DCC
        new ARMDCC));
    #endif //STDOUT_REDIRECTED_TO_DCC
}

void bspInit2()
{
//     #ifdef WITH_FILESYSTEM
//     basicFilesystemSetup();
//     #endif //WITH_FILESYSTEM
}

//
// Shutdown and reboot
//

void shutdown()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    disableInterrupts();
    for(;;) __WFI();
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

} //namespace miosix
