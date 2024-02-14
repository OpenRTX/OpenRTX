/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
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

#include "interfaces/bsp.h"

#include <inttypes.h>
#include <sys/ioctl.h>

#include <cstdlib>

#include "board_settings.h"
#include "config/miosix_settings.h"
#include "drivers/serial.h"
#include "drivers/serial_stm32.h"
#include "drivers/sd_stm32f2_f4_f7.h"
#include "filesystem/console/console_device.h"
#include "filesystem/file_access.h"
#include "interfaces/arch_registers.h"
#include "interfaces/delays.h"
#include "interfaces/portability.h"
#include "kernel/kernel.h"
#include "kernel/logging.h"
#include "kernel/sync.h"

namespace miosix {

//
// Initialization
//

static void sdramCommandWait()
{
    for(int i=0;i<0xffff;i++)
        if((FMC_Bank5_6->SDSR & FMC_SDSR_BUSY)==0)
            return;
}

void configureSdram()
{
    // Enable all gpios, passing clock
    RCC->AHB1ENR |=
        RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN |
        RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
        RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN | RCC_AHB1ENR_GPIOIEN |
        RCC_AHB1ENR_GPIOJEN | RCC_AHB1ENR_GPIOKEN;
    RCC_SYNC();

    // On the Discovery F769NI, the SDRAM pins are:
    // - PG8:  FMC_SDCLK
    // - PH2:  FMC_SDCKE0
    // - PH3:  FMC_SDNE0
    // - PF0:  FMC_A0
    // - PF1:  FMC_A1
    // - PF2:  FMC_A2
    // - PF3:  FMC_A3
    // - PF4:  FMC_A4
    // - PF5:  FMC_A5
    // - PF12: FMC_A6
    // - PF13: FMC_A7
    // - PF14: FMC_A8
    // - PF15: FMC_A9
    // - PG0:  FMC_A10
    // - PG1:  FMC_A11
    // - PG2:  FMC_A12
    // - PD14: FMC_D0
    // - PD15: FMC_D1
    // - PD0:  FMC_D2
    // - PD1:  FMC_D3
    // - PE7:  FMC_D4
    // - PE8:  FMC_D5
    // - PE9:  FMC_D6
    // - PE10: FMC_D7
    // - PE11: FMC_D8
    // - PE12: FMC_D9
    // - PE13: FMC_D10
    // - PE14: FMC_D11
    // - PE15: FMC_D12
    // - PD8:  FMC_D13
    // - PD9:  FMC_D14
    // - PD10: FMC_D15
    // - PH8:  FMC_D16
    // - PH9:  FMC_D17
    // - PH10: FMC_D18
    // - PH11: FMC_D19
    // - PH12: FMC_D20
    // - PH13: FMC_D21
    // - PH14: FMC_D22
    // - PH15: FMC_D23
    // - PI0:  FMC_D24
    // - PI1:  FMC_D25
    // - PI2:  FMC_D26
    // - PI3:  FMC_D27
    // - PI6:  FMC_D28
    // - PI7:  FMC_D29
    // - PI9:  FMC_D30
    // - PI10: FMC_D31
    // - PG4:  FMC_BA0
    // - PG5:  FMC_BA1
    // - PF11: FMC_SDNRAS
    // - PG15: FMC_SDNCAS
    // - PH5:  FMC_SDNWE
    // - PE0:  FMC_NBL0
    // - PE1:  FMC_NBL1
    // - PI4:  FMC_NBL2
    // - PI5:  FMC_NBL3

    // All SDRAM GPIOs needs to be configured with alternate function 12 and
    // maximum speed

    // Alternate functions
    GPIOD->AFR[0] = 0x000000cc;
    GPIOD->AFR[1] = 0xcc000ccc;
    GPIOE->AFR[0] = 0xc00000cc;
    GPIOE->AFR[1] = 0xcccccccc;
    GPIOF->AFR[0] = 0x00cccccc;
    GPIOF->AFR[1] = 0xccccc000;
    GPIOG->AFR[0] = 0x00cc0ccc;
    GPIOG->AFR[1] = 0xc000000c;
    GPIOH->AFR[0] = 0x00c0cc00;
    GPIOH->AFR[1] = 0xcccccccc;
    GPIOI->AFR[0] = 0xcccccccc;
    GPIOI->AFR[1] = 0x00000cc0;

    // Mode
    GPIOD->MODER = 0xa02a000a;
    GPIOE->MODER = 0xaaaa800a;
    GPIOF->MODER = 0xaa800aaa;
    GPIOG->MODER = 0x80020a2a;
    GPIOH->MODER = 0xaaaa08a0;
    GPIOI->MODER = 0x0028aaaa;

    // Speed (high speed for all, very high speed for SDRAM pins)
    GPIOA->OSPEEDR = 0xaaaaaaaa;
    GPIOB->OSPEEDR = 0xaaaaaaaa;
    GPIOC->OSPEEDR = 0xaaaaaaaa;
    GPIOD->OSPEEDR = 0xaaaaaaaa | 0xf03f000f;
    GPIOE->OSPEEDR = 0xaaaaaaaa | 0xffffc00f;
    GPIOF->OSPEEDR = 0xaaaaaaaa | 0xffc00fff;
    GPIOG->OSPEEDR = 0xaaaaaaaa | 0xc0030f3f;
    GPIOH->OSPEEDR = 0xaaaaaaaa | 0xffff0cf0;
    GPIOI->OSPEEDR = 0xaaaaaaaa | 0x003cffff;
    GPIOJ->OSPEEDR = 0xaaaaaaaa;
    GPIOK->OSPEEDR = 0xaaaaaaaa;

    // Enable the SDRAM controller clock
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    RCC_SYNC();

    // The SDRAM is a IS42S32400F-6BL
    // HCLK = 216MHz -> SDRAM clock = HCLK/2 = 133MHz

    // 1. Memory device features
    FMC_Bank5_6->SDCR[0] = 0                  // 0 delay after CAS latency
                         | FMC_SDCR1_RBURST   // Enable read bursts
                         | FMC_SDCR1_SDCLK_1  // SDCLK = HCLK / 2
                         | 0                  // Write accesses allowed
                         | FMC_SDCR1_CAS_1    // 2 cycles CAS latency
                         | FMC_SDCR1_NB       // 4 internal banks
                         | FMC_SDCR1_MWID_1   // 32 bit data bus
                         | FMC_SDCR1_NR_0     // 12 bit row address
                         | 0;                 // 8 bit column address

    // 2. Memory device timings
    #ifdef SYSCLK_FREQ_216MHz
    // SDRAM timings. One clock cycle is 9.26ns
    FMC_Bank5_6->SDTR[0] =
          (2 - 1) << FMC_SDTR1_TRCD_Pos   // 2 cycles TRCD (18.52ns > 18ns)
        | (2 - 1) << FMC_SDTR1_TRP_Pos    // 2 cycles TRP  (18.52ns > 18ns)
        | (2 - 1) << FMC_SDTR1_TWR_Pos    // 2 cycles TWR  (18.52ns > 12ns)
        | (7 - 1) << FMC_SDTR1_TRC_Pos    // 7 cycles TRC  (64.82ns > 60ns)
        | (5 - 1) << FMC_SDTR1_TRAS_Pos   // 5 cycles TRAS (46.3ns  > 42ns)
        | (8 - 1) << FMC_SDTR1_TXSR_Pos   // 8 cycles TXSR (74.08ns > 70ns)
        | (2 - 1) << FMC_SDTR1_TMRD_Pos;  // 2 cycles TMRD (18.52ns > 12ns)
    #else
    #error No SDRAM timings for this clock
    #endif

    // 3. Enable the bank 1 clock
    FMC_Bank5_6->SDCMR = FMC_SDCMR_MODE_0   // Clock Configuration Enable
                       | FMC_SDCMR_CTB1;  // Bank 1
    sdramCommandWait();

    // 4. Wait during command execution
    delayUs(100);

    // 5. Issue a "Precharge All" command
    FMC_Bank5_6->SDCMR = FMC_SDCMR_MODE_1   // Precharge all
                       | FMC_SDCMR_CTB1;  // Bank 1
    sdramCommandWait();

    // 6. Issue Auto-Refresh commands
    FMC_Bank5_6->SDCMR = FMC_SDCMR_MODE_1 | FMC_SDCMR_MODE_0  // Auto-Refresh
                       | FMC_SDCMR_CTB1                     // Bank 1
                       | (8 - 1) << FMC_SDCMR_NRFS_Pos;     // 2 Auto-Refresh
    sdramCommandWait();

    // 7. Issue a Load Mode Register command
    FMC_Bank5_6->SDCMR = FMC_SDCMR_MODE_2               /// Load mode register
                       | FMC_SDCMR_CTB1               // Bank 1
                       | 0x220 << FMC_SDCMR_MRD_Pos;  // CAS = 2, burst = 1
    sdramCommandWait();

    // 8. Program the refresh rate (4K / 64ms)
    // 64ms / 4096 = 15.625us
    #ifdef SYSCLK_FREQ_216MHz
    // 15.625us * 108MHz = 1687 - 20 = 1667
    FMC_Bank5_6->SDRTR = 1667 << FMC_SDRTR_COUNT_Pos;
    #else
    #error No SDRAM refresh timings for this clock
    #endif
}

void IRQbspInit()
{
    //If using SDRAM GPIOs are enabled by configureSdram(), else enable them here
    #ifndef __ENABLE_XRAM
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN |
                    RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOFEN |
                    RCC_AHB1ENR_GPIOGEN | RCC_AHB1ENR_GPIOHEN |
                    RCC_AHB1ENR_GPIOIEN | RCC_AHB1ENR_GPIOJEN;
    RCC_SYNC();
    #endif  //__ENABLE_XRAM

    userLed1::mode(Mode::OUTPUT);
    userLed2::mode(Mode::OUTPUT);
    userLed3::mode(Mode::OUTPUT);
    userBtn::mode(Mode::INPUT);
    sdCardDetect::mode(Mode::INPUT);

    ledOn();
    delayMs(100);
    ledOff();

    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(new STM32Serial(
        defaultSerial, defaultSerialSpeed, STM32Serial::NOFLOWCTRL)));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    basicFilesystemSetup(SDIODriver::instance());
    #endif  // WITH_FILESYSTEM
}

//
// Shutdown and reboot
//

void shutdown()
{
    ioctl(STDOUT_FILENO, IOCTL_SYNC, 0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif  // WITH_FILESYSTEM

    disableInterrupts();
    for(;;) ;
}

void reboot()
{
    ioctl(STDOUT_FILENO, IOCTL_SYNC, 0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif  // WITH_FILESYSTEM

    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

}  // namespace miosix
