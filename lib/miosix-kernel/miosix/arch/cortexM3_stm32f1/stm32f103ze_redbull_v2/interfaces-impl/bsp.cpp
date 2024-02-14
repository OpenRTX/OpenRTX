/***************************************************************************
 *   Copyright (C) 2011 by Yury Kuchura                                    *
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
#include "drivers/sd_stm32f1.h"
#include "board_settings.h"

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    //Enable clocks to all ports
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPDEN |
                    RCC_APB2ENR_IOPEEN | RCC_APB2ENR_IOPFEN |
                    RCC_APB2ENR_IOPGEN | RCC_APB2ENR_AFIOEN;
    RCC_SYNC();
    //Set ports
    leds::led1::high();
    leds::led1::mode(Mode::OUTPUT);
    leds::led2::high();
    leds::led2::mode(Mode::OUTPUT);
    leds::led3::high();
    leds::led3::mode(Mode::OUTPUT);
    leds::led4::high();
    leds::led4::mode(Mode::OUTPUT);
    leds::led5::high();
    leds::led5::mode(Mode::OUTPUT);

    disp::backlight::mode(Mode::OUTPUT);

    disp::d0::mode(Mode::ALTERNATE);
    disp::d1::mode(Mode::ALTERNATE);
    disp::d2::mode(Mode::ALTERNATE);
    disp::d3::mode(Mode::ALTERNATE);
    disp::d4::mode(Mode::ALTERNATE);
    disp::d5::mode(Mode::ALTERNATE);
    disp::d6::mode(Mode::ALTERNATE);
    disp::d7::mode(Mode::ALTERNATE);
    disp::d8::mode(Mode::ALTERNATE);
    disp::d9::mode(Mode::ALTERNATE);
    disp::d10::mode(Mode::ALTERNATE);
    disp::d11::mode(Mode::ALTERNATE);
    disp::d12::mode(Mode::ALTERNATE);
    disp::d13::mode(Mode::ALTERNATE);
    disp::d14::mode(Mode::ALTERNATE);
    disp::d15::mode(Mode::ALTERNATE);
    disp::rd::mode(Mode::ALTERNATE);
    disp::wr::mode(Mode::ALTERNATE);
    disp::cs::mode(Mode::ALTERNATE);
    disp::rs::mode(Mode::ALTERNATE);

    //Chip select to serial flash: setting it to 1
    spi1::nss::high();
    spi1::nss::mode(Mode::OUTPUT);

    //Touchscreen interrupt signal
    spi2::touchint::pullup();
    spi2::ntouchss::mode(Mode::INPUT_PULL_UP_DOWN);

    //Configure buttons
    buttons::wakeup::mode(Mode::INPUT_PULL_UP_DOWN);
    buttons::wakeup::pullup();
    buttons::tamper::mode(Mode::INPUT_PULL_UP_DOWN);
    buttons::tamper::pullup();

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
    basicFilesystemSetup(SDIODriver::instance());
    #endif //WITH_FILESYSTEM
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
/*
    SCB->SCR |= SCB_SCR_SLEEPDEEP;
    PWR->CR |= PWR_CR_PDDS; //Select standby mode
    PWR->CR |= PWR_CR_CWUF;
    __NOP();
    __NOP();
    PWR->CSR |= PWR_CSR_EWUP; //Enable PA.0 as wakeup source
    //FIXME: wakeup via PA.0 is not working
    __WFI(); 
*/
    NVIC_SystemReset();
    for(;;) ; //Never reach here
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
