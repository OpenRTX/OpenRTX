/***************************************************************************
 *   Copyright (C) 2020 by Silvano Seva                                    *
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
#include "hwmapping.h"

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
// Initialization
//

void IRQbspInit()
{
    //Enable all gpios and AFIO
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
    ledOn();
    delayMs(100);
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
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);

    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    //Cut power to whole system
    poweroff::mode(Mode::OUTPUT);
    poweroff::high();
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
