/***************************************************************************
 *   Copyright (C) 2014 by Terraneo Federico                               *
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
#include "drivers/sd_stm32f2_f4_f7.h"
#include "board_settings.h"
// #include "kernel/IRQDisplayPrint.h"

namespace miosix {

//
// Initialization
//

/**
 * The example code from ST checks for the busy flag after each command.
 * Interestingly I couldn't find any mention of this in the datasheet.
 */
static void sdramCommandWait()
{
    for(int i=0;i<0xffff;i++)
        if((FMC_Bank5_6->SDSR & FMC_SDSR_BUSY)==0) return;
}

void configureSdram()
{
    /*   SDRAM pins assignment
    PC0 -> FMC_SDNWE
    PD0  -> FMC_D2   | PE0  -> FMC_NBL0 | PF0  -> FMC_A0 | PG0 -> FMC_A10
    PD1  -> FMC_D3   | PE1  -> FMC_NBL1 | PF1  -> FMC_A1 | PG1 -> FMC_A11
    PD8  -> FMC_D13  | PE7  -> FMC_D4   | PF2  -> FMC_A2 | PG4 -> FMC_BA0
    PD9  -> FMC_D14  | PE8  -> FMC_D5   | PF3  -> FMC_A3 | PG5 -> FMC_BA1
    PD10 -> FMC_D15  | PE9  -> FMC_D6   | PF4  -> FMC_A4 | PG8 -> FC_SDCLK
    PD14 -> FMC_D0   | PE10 -> FMC_D7   | PF5  -> FMC_A5 | PG15 -> FMC_NCAS
    PD15 -> FMC_D1   | PE11 -> FMC_D8   | PF11 -> FC_NRAS
                     | PE12 -> FMC_D9   | PF12 -> FMC_A6
                     | PE13 -> FMC_D10  | PF13 -> FMC_A7
                     | PE14 -> FMC_D11  | PF14 -> FMC_A8
                     | PE15 -> FMC_D12  | PF15 -> FMC_A9
    PH2 -> FMC_SDCKE0| PI4 -> FMC_NBL2  |
    PH3 -> FMC_SDNE0 | PI5 -> FMC_NBL3  |

    // 32-bits Mode: D31-D16
    PH8 -> FMC_D16   | PI0 -> FMC_D24
    PH9 -> FMC_D17   | PI1 -> FMC_D25
    PH10 -> FMC_D18  | PI2 -> FMC_D26
    PH11 -> FMC_D19  | PI3 -> FMC_D27
    PH12 -> FMC_D20  | PI6 -> FMC_D28
    PH13 -> FMC_D21  | PI7 -> FMC_D29
    PH14 -> FMC_D22  | PI9 -> FMC_D30
    PH15 -> FMC_D23  | PI10 -> FMC_D31 */

    //Enable all gpios, passing clock
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN |
                    RCC_AHB1ENR_GPIOIEN | RCC_AHB1ENR_GPIOJEN |
                    RCC_AHB1ENR_GPIOKEN;
    RCC_SYNC();

    //First, configure SDRAM GPIOs, memory controller uses AF12
    GPIOC->AFR[0]=0x0000000c;
    GPIOD->AFR[0]=0x000000cc;
    GPIOD->AFR[1]=0xcc000ccc;
    GPIOE->AFR[0]=0xc00000cc;
    GPIOE->AFR[1]=0xcccccccc;
    GPIOF->AFR[0]=0x00cccccc;
    GPIOF->AFR[1]=0xccccc000;
    GPIOG->AFR[0]=0x00cc00cc;
    GPIOG->AFR[1]=0xc000000c;
    GPIOH->AFR[0]=0x0000cc00;
    GPIOH->AFR[1]=0xcccccccc;
    GPIOI->AFR[0]=0xcccccccc;
    GPIOI->AFR[1]=0x00000cc0;

    GPIOC->MODER=0x00000002;
    GPIOD->MODER=0xa02a000a;
    GPIOE->MODER=0xaaaa800a;
    GPIOF->MODER=0xaa800aaa;
    GPIOG->MODER=0x80020a0a;
    GPIOH->MODER=0xaaaa00a0;
    GPIOI->MODER=0x0028aaaa;

    /* GPIO speed register
    00: Low speed
    01: Medium speed
    10: High speed (50MHz)
    11: Very high speed (100MHz) */

    //Default to 50MHz speed for all GPIOs...
    GPIOA->OSPEEDR=0xaaaaaaaa;
    GPIOB->OSPEEDR=0xaaaaaaaa;
    GPIOC->OSPEEDR=0xaaaaaaaa | 0x00000003; //...but 100MHz for the SDRAM pins
    GPIOD->OSPEEDR=0xaaaaaaaa | 0xf03f000f;
    GPIOE->OSPEEDR=0xaaaaaaaa | 0xffffc00f;
    GPIOF->OSPEEDR=0xaaaaaaaa | 0xffc00fff;
    GPIOG->OSPEEDR=0xaaaaaaaa | 0xc0030f0f;
    GPIOH->OSPEEDR=0xaaaaaaaa | 0xffff00f0;
    GPIOI->OSPEEDR=0xaaaaaaaa | 0x003cffff;
    GPIOJ->OSPEEDR=0xaaaaaaaa;
    GPIOK->OSPEEDR=0xaaaaaaaa;

    //Second, actually start the SDRAM controller
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    RCC_SYNC();

    //SDRAM is a MT48LC4M32B2B5 -6A speed grade, connected to bank 1 (0xc0000000)
    FMC_Bank5_6->SDCR[0]=FMC_SDCR1_SDCLK_1// SDRAM runs @ half CPU frequency
                       | FMC_SDCR1_RBURST // Enable read burst
                       | 0                //  0 delay between reads after CAS
                       | 0                //  8 bit column address
                       | FMC_SDCR1_NR_0   // 12 bit row address
                       | FMC_SDCR1_MWID_1 // 32 bit data bus
                       | FMC_SDCR1_NB     //  4 banks
                       | FMC_SDCR1_CAS_0  //  3 cycle CAS latency
                       | FMC_SDCR1_CAS_1;

    #ifdef SYSCLK_FREQ_180MHz
    //One SDRAM clock cycle is 11.1ns
    FMC_Bank5_6->SDTR[0]=(6-1)<<12        // 6 cycle TRC  (66.6ns>60ns)
                       | (2-1)<<20        // 2 cycle TRP  (22.2ns>18ns)
                       | (2-1)<<0         // 2 cycle TMRD
                       | (7-1)<<4         // 7 cycle TXSR (77.7ns>67ns)
                       | (4-1)<<8         // 4 cycle TRAS (44.4ns>42ns)
                       | (2-1)<<16        // 2 cycle TWR
                       | (2-1)<<24;       // 2 cycle TRCD (22.2ns>18ns)
    #elif defined(SYSCLK_FREQ_168MHz)
    //One SDRAM clock cycle is 11.9ns
    FMC_Bank5_6->SDTR[0]=(6-1)<<12        // 6 cycle TRC  (71.4ns>60ns)
                       | (2-1)<<20        // 2 cycle TRP  (23.8ns>18ns)
                       | (2-1)<<0         // 2 cycle TMRD
                       | (6-1)<<4         // 6 cycle TXSR (71.4ns>67ns)
                       | (4-1)<<8         // 4 cycle TRAS (47.6ns>42ns)
                       | (2-1)<<16        // 2 cycle TWR
                       | (2-1)<<24;       // 2 cycle TRCD (23.8ns>18ns)
    #else
    #error No SDRAM timings for this clock
    #endif

    FMC_Bank5_6->SDCMR=  FMC_SDCMR_CTB1   // Enable bank 1
                       | 1;               // MODE=001 clock enabled
    sdramCommandWait();

    //ST and SDRAM datasheet agree a 100us delay is required here.
    delayUs(100);

    FMC_Bank5_6->SDCMR=  FMC_SDCMR_CTB1   // Enable bank 1
                       | 2;               // MODE=010 precharge all command
    sdramCommandWait();

    FMC_Bank5_6->SDCMR=  (8-1)<<5         // NRFS=8 SDRAM datasheet says
                                          // "at least two AUTO REFRESH cycles"
                       | FMC_SDCMR_CTB1   // Enable bank 1
                       | 3;               // MODE=011 auto refresh
    sdramCommandWait();

    FMC_Bank5_6->SDCMR=0x230<<9           // MRD=0x230:CAS latency=3 burst len=1
                       | FMC_SDCMR_CTB1   // Enable bank 1
                       | 4;               // MODE=100 load mode register
    sdramCommandWait();

    // 64ms/4096=15.625us
    #ifdef SYSCLK_FREQ_180MHz
    //15.625us*90MHz=1406-20=1386
    FMC_Bank5_6->SDRTR=1386<<1;
    #elif defined(SYSCLK_FREQ_168MHz)
    //15.625us*84MHz=1312-20=1292
    FMC_Bank5_6->SDRTR=1292<<1;
    #else
    #error No refresh timings for this clock
    #endif
}

// static IRQDisplayPrint *irq_display;
void IRQbspInit()
{
    //If using SDRAM GPIOs are enabled by configureSdram(), else enable them here
    #ifndef __ENABLE_XRAM
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN |
                    RCC_AHB1ENR_GPIOIEN | RCC_AHB1ENR_GPIOJEN |
                    RCC_AHB1ENR_GPIOKEN;
    RCC_SYNC();
    #endif //__ENABLE_XRAM

    _led::mode(Mode::OUTPUT);
    ledOn();
    delayMs(100);
    ledOff();
    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new STM32Serial(defaultSerial,defaultSerialSpeed,
        defaultSerialFlowctrl ? STM32Serial::RTSCTS : STM32Serial::NOFLOWCTRL)));
//     irq_display = new IRQDisplayPrint();
//     DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(irq_display));
}

// void* printIRQ(void *argv)
// {
// 	intrusive_ref_ptr<IRQDisplayPrint> irqq(irq_display);
// 	irqq.get()->printIRQ();
// 	return NULL;
// }

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    basicFilesystemSetup(SDIODriver::instance());
    #endif //WITH_FILESYSTEM
//     Thread::create(printIRQ, 2048);
}

//
// Shutdown and reboot
//

/**
This function disables filesystem (if enabled), serial port (if enabled) and
puts the processor in deep sleep mode.<br>
Wakeup occurs when PA.0 goes high, but instead of sleep(), a new boot happens.
<br>This function does not return.<br>
WARNING: close all files before using this function, since it unmounts the
filesystem.<br>
When in shutdown mode, power consumption of the miosix board is reduced to ~
5uA??, however, true power consumption depends on what is connected to the GPIO
pins. The user is responsible to put the devices connected to the GPIO pin in the
minimal power consumption mode before calling shutdown(). Please note that to
minimize power consumption all unused GPIO must not be left floating.
*/
void shutdown()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    disableInterrupts();

    for(;;) ;
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
