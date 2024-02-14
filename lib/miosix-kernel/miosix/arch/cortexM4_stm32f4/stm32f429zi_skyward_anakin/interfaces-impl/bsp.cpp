/***************************************************************************
 *   Copyright (C) 2016 by Terraneo Federico                               *
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
#include "drivers/stm32_sgm.h"
#include "board_settings.h"

namespace miosix {

//
// Initialization
//

/**
 * The example code from ST checks for the busy flag after each command.
 * Interestingly I couldn't find any mention of this in the datsheet.
 */
static void sdramCommandWait()
{
    for(int i=0;i<0xffff;i++)
        if((FMC_Bank5_6->SDSR & FMC_SDSR_BUSY)==0) return;
}

void configureSdram()
{
    //Enable all gpios
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN;
    RCC_SYNC();
    
    //First, configure SDRAM GPIOs
    GPIOB->AFR[0]=0x0cc00000;
    GPIOC->AFR[0]=0x0000000c;
    GPIOD->AFR[0]=0x000000cc;
    GPIOD->AFR[1]=0xcc000ccc;
    GPIOE->AFR[0]=0xc00000cc;
    GPIOE->AFR[1]=0xcccccccc;
    GPIOF->AFR[0]=0x00cccccc;
    GPIOF->AFR[1]=0xccccc000;
    GPIOG->AFR[0]=0x00cc00cc;
    GPIOG->AFR[1]=0xc000000c;

    GPIOB->MODER=0x00002800;
    GPIOC->MODER=0x00000002;
    GPIOD->MODER=0xa02a000a;
    GPIOE->MODER=0xaaaa800a;
    GPIOF->MODER=0xaa800aaa;
    GPIOG->MODER=0x80020a0a;
    
    GPIOA->OSPEEDR=0xaaaaaaaa; //Default to 50MHz speed for all GPIOs...
    GPIOB->OSPEEDR=0xaaaaaaaa | 0x00003c00; //...but 100MHz for the SDRAM pins
    GPIOC->OSPEEDR=0xaaaaaaaa | 0x00000003;
    GPIOD->OSPEEDR=0xaaaaaaaa | 0xf03f000f;
    GPIOE->OSPEEDR=0xaaaaaaaa | 0xffffc00f;
    GPIOF->OSPEEDR=0xaaaaaaaa | 0xffc00fff;
    GPIOG->OSPEEDR=0xaaaaaaaa | 0xc0030f0f;
    GPIOH->OSPEEDR=0xaaaaaaaa;
    
    //Since we'we un-configured PB3/PB4 from the default at boot TDO,NTRST,
    //finish the job and remove the default pull-up
    GPIOB->PUPDR=0;
    
    //Second, actually start the SDRAM controller
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    RCC_SYNC();
    
    //SDRAM is a AS4C4M16SA-6TAN, connected to bank 2 (0xd0000000)
    //Some bits in SDCR[1] are don't care, and the have to be set in SDCR[0],
    //they aren't just don't care, the controller will fail if they aren't at 0
    FMC_Bank5_6->SDCR[0]=FMC_SDCR1_SDCLK_1// SDRAM runs @ half CPU frequency
                       | FMC_SDCR1_RBURST // Enable read burst
                       | 0;               //  0 delay between reads after CAS
    FMC_Bank5_6->SDCR[1]=0                //  8 bit column address
                       | FMC_SDCR1_NR_0   // 12 bit row address
                       | FMC_SDCR1_MWID_0 // 16 bit data bus
                       | FMC_SDCR1_NB     //  4 banks
                       | FMC_SDCR1_CAS_1; //  2 cycle CAS latency (TCK>9+0.5ns [1])
    
    #ifdef SYSCLK_FREQ_180MHz
    //One SDRAM clock cycle is 11.1ns
    //Some bits in SDTR[1] are don't care, and the have to be set in SDTR[0],
    //they aren't just don't care, the controller will fail if they aren't at 0
    FMC_Bank5_6->SDTR[0]=(6-1)<<12        // 6 cycle TRC  (66.6ns>60ns)
                       | (2-1)<<20;       // 2 cycle TRP  (22.2ns>18ns)
    FMC_Bank5_6->SDTR[1]=(2-1)<<0         // 2 cycle TMRD
                       | (6-1)<<4         // 6 cycle TXSR (66.6ns>61.5+0.5ns [1])
                       | (4-1)<<8         // 4 cycle TRAS (44.4ns>42ns)
                       | (2-1)<<16        // 2 cycle TWR
                       | (2-1)<<24;       // 2 cycle TRCD (22.2ns>18ns)
    #elif defined(SYSCLK_FREQ_168MHz)
    //One SDRAM clock cycle is 11.9ns
    //Some bits in SDTR[1] are don't care, and the have to be set in SDTR[0],
    //they aren't just don't care, the controller will fail if they aren't at 0
    FMC_Bank5_6->SDTR[0]=(6-1)<<12        // 6 cycle TRC  (71.4ns>60ns)
                       | (2-1)<<20;       // 2 cycle TRP  (23.8ns>18ns)
    FMC_Bank5_6->SDTR[1]=(2-1)<<0         // 2 cycle TMRD
                       | (6-1)<<4         // 6 cycle TXSR (71.4ns>61.5+0.5ns [1])
                       | (4-1)<<8         // 4 cycle TRAS (47.6ns>42ns)
                       | (2-1)<<16        // 2 cycle TWR
                       | (2-1)<<24;       // 2 cycle TRCD (23.8ns>18ns)
    #else
    #error No SDRAM timings for this clock
    #endif
    //NOTE [1]: the timings for TCK and TIS depend on rise and fall times
    //(see note 9 and 10 on datasheet). Timings are adjusted accordingly to
    //the measured 2ns rise and fall time

    FMC_Bank5_6->SDCMR=  FMC_SDCMR_CTB2   // Enable bank 2
                       | 1;               // MODE=001 clock enabled
    sdramCommandWait();
    
    //SDRAM datasheet requires 200us delay here (note 11), here we use 10% more
    delayUs(220);

    FMC_Bank5_6->SDCMR=  FMC_SDCMR_CTB2   // Enable bank 2
                       | 2;               // MODE=010 precharge all command
    sdramCommandWait();

    //FIXME: note 11 on SDRAM datasheet says extended mode register must be set,
    //but the ST datasheet does not seem to explain how
    FMC_Bank5_6->SDCMR=0x220<<9           // MRD=0x220:CAS latency=2 burst len=1
                       | FMC_SDCMR_CTB2   // Enable bank 2
                       | 4;               // MODE=100 load mode register
    sdramCommandWait();
	
    FMC_Bank5_6->SDCMR=  (4-1)<<5         // NRFS=8 SDRAM datasheet requires
                                          // a minimum of 2 cycles, here we use 4
                       | FMC_SDCMR_CTB2   // Enable bank 2
                       | 3;               // MODE=011 auto refresh
    sdramCommandWait();

    // 32ms/4096=7.8125us, but datasheet says to round that to 7.8us
    #ifdef SYSCLK_FREQ_180MHz
    //7.8us*90MHz=702-20=682
    FMC_Bank5_6->SDRTR=682<<1;
    #elif defined(SYSCLK_FREQ_168MHz)
    //7.8us*84MHz=655-20=635
    FMC_Bank5_6->SDRTR=635<<1;
    #else
    #error No refresh timings for this clock
    #endif
}

void IRQbspInit()
{

    /* force Safe Guard Memory constructor call */
    SGM::instance();
    
    /*If using SDRAM GPIOs are enabled by configureSdram(), else enable them here */
    #ifndef __ENABLE_XRAM
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN;
    RCC_SYNC();
    #endif /* __ENABLE_XRAM */

    using namespace leds;
    led0::mode(Mode::OUTPUT);
    led1::mode(Mode::OUTPUT);
    led2::mode(Mode::OUTPUT);
    led3::mode(Mode::OUTPUT);
    led4::mode(Mode::OUTPUT);
    led5::mode(Mode::OUTPUT);
    led6::mode(Mode::OUTPUT);
    led7::mode(Mode::OUTPUT);
    led8::mode(Mode::OUTPUT);
    led9::mode(Mode::OUTPUT);
    
    using namespace sensors;
    fxas21002::cs::mode(Mode::OUTPUT);
    fxas21002::cs::high();
    fxas21002::int1::mode(Mode::INPUT);
    fxas21002::int2::mode(Mode::INPUT);
    
    lps331::cs::mode(Mode::OUTPUT);
    lps331::cs::high();
    lps331::int1::mode(Mode::INPUT);
    lps331::int2::mode(Mode::INPUT);
    
    lsm9ds::csg::mode(Mode::OUTPUT);
    lsm9ds::csg::high();
    lsm9ds::csm::mode(Mode::OUTPUT);
    lsm9ds::csm::high();
    lsm9ds::drdyg::mode(Mode::INPUT);
    lsm9ds::int1g::mode(Mode::INPUT);
    lsm9ds::int1m::mode(Mode::INPUT);
    lsm9ds::int2m::mode(Mode::INPUT);
    
    max21105::cs::mode(Mode::OUTPUT);
    max21105::cs::high();
    max21105::int1::mode(Mode::INPUT);
    max21105::int2::mode(Mode::INPUT);
    
    max31856::cs::mode(Mode::OUTPUT);
    max31856::cs::high();
    max31856::drdy::mode(Mode::INPUT);
    max31856::fault::mode(Mode::INPUT);
    
    mpl3115::int1::mode(Mode::INPUT);
    mpl3115::int2::mode(Mode::INPUT);
    
    mpu9250::cs::mode(Mode::OUTPUT);
    mpu9250::cs::high();
    mpu9250::int1::mode(Mode::INPUT);
    
    ms5803::cs::mode(Mode::OUTPUT);
    ms5803::cs::high();
    
    eth::cs::mode(Mode::OUTPUT);
    eth::cs::high();
    eth::int1::mode(Mode::INPUT);

    ledOn();
    delayMs(100);
    ledOff();
    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new STM32Serial(defaultSerial,defaultSerialSpeed,
        defaultSerialFlowctrl ? STM32Serial::RTSCTS : STM32Serial::NOFLOWCTRL)));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    intrusive_ref_ptr<DevFs> devFs=basicFilesystemSetup(SDIODriver::instance());
    devFs->addDevice("gps",
        intrusive_ref_ptr<Device>(new STM32Serial(2,115200)));

    //TODO: STM32Serial configures USART2 on its default pins, which are
    //instead connected to the lps331 interrupt pins. For now we'll
    //manually deconfigure the old pins and reconfigure the new ones, but
    //a better way would be desirable
    {
        FastInterruptDisableLock dLock;
        // Deconfigure old pins (that the STM32Serial driver thinks it's usart
        sensors::lps331::int1::mode(Mode::INPUT);
        sensors::lps331::int2::mode(Mode::INPUT);
        // Configure new pins, which is where the usart actually is
        piksi::rx::alternateFunction(7);
        piksi::tx::alternateFunction(7);
        piksi::rx::mode(Mode::ALTERNATE);
        piksi::tx::mode(Mode::ALTERNATE);
    }
    #endif //WITH_FILESYSTEM
}

//
// Shutdown and reboot
//

/**
 * For safety reasons, we never want the anakin to shutdown.
 * When requested to shutdown, we reboot instead.
 */
void shutdown()
{
    reboot();
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
