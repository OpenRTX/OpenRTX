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
#include "board_settings.h"
#include "hrtb.h"
#include "vht.h"
namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    //
    // Setup GPIOs
    //
    CMU->HFPERCLKEN0|=CMU_HFPERCLKEN0_GPIO;
    GPIO->CTRL=GPIO_CTRL_EM4RET; //GPIOs keep their state in EM4
    
    redLed::mode(Mode::OUTPUT_LOW);
    greenLed::mode(Mode::OUTPUT_LOW);
    userButton::mode(Mode::Mode::INPUT_PULL_UP_FILTER);
    loopback32KHzIn::mode(Mode::INPUT);
    loopback32KHzOut::mode(Mode::OUTPUT);
    
    #if WANDSTEM_HW_REV>=13
    voltageSelect::mode(Mode::OUTPUT_LOW); //Default VDD=2.3V
    #endif

    #if WANDSTEM_HW_REV>13
    powerSwitch::mode(Mode::OUTPUT_LOW);
    #endif
    
    internalSpi::mosi::mode(Mode::OUTPUT_LOW);
    internalSpi::miso::mode(Mode::INPUT_PULL_DOWN);   //To prevent it floating
    internalSpi::sck::mode(Mode::OUTPUT_LOW);
    
    transceiver::cs::mode(Mode::OUTPUT_LOW);
    transceiver::reset::mode(Mode::OUTPUT_LOW);
    transceiver::vregEn::mode(Mode::OUTPUT_LOW);
    transceiver::gpio1::mode(Mode::INPUT_PULL_DOWN);  //To prevent it floating
    transceiver::gpio2::mode(Mode::INPUT_PULL_DOWN);  //To prevent it floating
    transceiver::excChB::mode(Mode::INPUT_PULL_DOWN); //To prevent it floating
    #if WANDSTEM_HW_REV<13
    transceiver::gpio4::mode(Mode::INPUT_PULL_DOWN);  //To prevent it floating
    #endif
    transceiver::stxon::mode(Mode::OUTPUT_LOW);
    
    #if WANDSTEM_HW_REV>10
    //Flash is gated, keeping low prevents current from flowing in gated domain
    flash::cs::mode(Mode::OUTPUT_LOW);
    flash::hold::mode(Mode::OUTPUT_LOW);
    #else
    //Flash not power gated in earlier boards
    flash::cs::mode(Mode::OUTPUT_HIGH);
    flash::hold::mode(Mode::OUTPUT_HIGH);
    #endif
    
    currentSense::enable::mode(Mode::OUTPUT_LOW);
    //currentSense sense pin remains disabled as it is an analog channel
    
    //
    // Setup clocks, as when we get here we're still running with HFRCO
    //

    //HFXO startup time seems slightly dependent on supply voltage, with
    //higher voltage resulting in longer startup time (changes by a few us at
    //most). Also, HFXOBOOST greatly affects startup time, as shown in the
    //following table
    //BOOST sample#1  sample#2
    //100%    94us     100us
    // 80%   104us     111us
    // 70%   117us     125us
    // 50%   205us     223us

    //Configure oscillator parameters for HFXO and LFXO
    unsigned int dontChange=CMU->CTRL & CMU_CTRL_LFXOBUFCUR;
    CMU->CTRL=CMU_CTRL_HFLE                  //We run at a frequency > 32MHz
            | CMU_CTRL_CLKOUTSEL1_LFXOQ      //Used for the 32KHz loopback
            | CMU_CTRL_LFXOTIMEOUT_16KCYCLES //16K cyc timeout for LFXO startup
            | CMU_CTRL_LFXOBOOST_70PCENT     //Use recomended value
            | CMU_CTRL_HFXOTIMEOUT_1KCYCLES  //1K cyc timeout for HFXO startup
            | CMU_CTRL_HFXOBUFCUR_BOOSTABOVE32MHZ //We run at a freq > 32MHz
            | CMU_CTRL_HFXOBOOST_70PCENT     //We want a startup time >=100us
            | dontChange;                    //Don't change some of the bits
            
    //Start HFXO and LFXO.
    //The startup of the HFXO oscillator was measured and takes less than 125us
    CMU->OSCENCMD=CMU_OSCENCMD_HFXOEN | CMU_OSCENCMD_LFXOEN;
    
    //Configure flash wait states and dividers so that it's safe to run at 48MHz
    CMU->HFCORECLKDIV=CMU_HFCORECLKDIV_HFCORECLKLEDIV; //We run at a freq >32MHz
    MSC->READCTRL=MSC_READCTRL_MODE_WS2; //Two wait states for f>32MHz
    MSC->WRITECTRL=MSC_WRITECTRL_RWWEN;  //Enable FLASH read while write support
    
    ledOn();
    #ifndef JTAG_DISABLE_SLEEP
    //Reuse the LED blink at boot to wait for the LFXO 32KHz oscillator startup
    //SWitching temporarily the CPU to run off of the 32KHz XTAL is the easiest
    //way to sleep while it locks, as it stalls the CPU and peripherals till the
    //oscillator is stable
    CMU->CMD=CMU_CMD_HFCLKSEL_LFXO;
    #else //JTAG_DISABLE_SLEEP
    while((CMU->STATUS & CMU_STATUS_LFXORDY)==0) ;
    #endif //JTAG_DISABLE_SLEEP
    ledOff();
    
    //Then switch immediately to HFXO, so that we (finally) run at 48MHz
    CMU->CMD=CMU_CMD_HFCLKSEL_HFXO;
    
    //Disable HFRCO since we don't need it anymore
    CMU->OSCENCMD=CMU_OSCENCMD_HFRCODIS;
    
    //Put the LFXO frequency on the loopback pin
    CMU->ROUTE=CMU_ROUTE_LOCATION_LOC1 //32KHz out is on PD8
             | CMU_ROUTE_CLKOUT1PEN;   //Enable pin
    
    //The LFA and LFB clock trees are connected to the LFXO
    CMU->LFCLKSEL=CMU_LFCLKSEL_LFB_LFXO | CMU_LFCLKSEL_LFA_LFXO;
    
    //This function initializes the SystemCoreClock variable. It is put here
    //so as to get the right value
    SystemCoreClockUpdate();
    
    //
    // Setup serial port
    //
    DefaultConsole::instance().IRQset(intrusive_ref_ptr<Device>(
        new EFM32Serial(defaultSerial,defaultSerialSpeed)));
}

void bspInit2()
{
    #ifndef DISABLE_FLOPSYNCVHT
    VHT::instance().start();
    #endif //DISABLE_FLOPSYNCVHT
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
    
    //Serial port is causing some residual consumption
    USART0->CMD=USART_CMD_TXDIS | USART_CMD_RXDIS;
    USART0->ROUTE=0;
    debugConnector::tx::mode(Mode::DISABLED);
    debugConnector::rx::mode(Mode::DISABLED);
    
    //Sequence to enter EM4
    for(int i=0;i<5;i++)
    {
        EMU->CTRL=2<<2;
        EMU->CTRL=3<<2;
    }
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
