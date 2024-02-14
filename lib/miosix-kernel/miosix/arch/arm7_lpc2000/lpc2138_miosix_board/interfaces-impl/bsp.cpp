/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014                *
 *   by Terraneo Federico                                                  *
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
#include "core/interrupts.h"
#include "interfaces/delays.h"
#include "drivers/serial.h"
#include "drivers/sd_lpc2000.h"
#include "kernel/kernel.h"
#include "kernel/sync.h"
#include "interfaces/portability.h"
#include "config/miosix_settings.h"
#include "kernel/logging.h"
#include "filesystem/file_access.h"
#include "filesystem/console/console_device.h"

/*
******************
Failsafe pin state
******************
Pin marked as gpio are available for general purpose use
Pin marked as gpio / JTAG are not available if using JTAG in circuit debugger
Pin marked as LED can be accessed using ledOn(), ledOff() and ledRead()
Pin marked as enter/pgm button can be accessed using buttonEnter()
Pin marked as * have the restriction that cannot be driven low when the
microcontroller is in RESET because they trigger special functions, for more
info, read the LPC2138 datasheet.
Pin marked as # are open drain, that is, cannot force a high level without the
use of an external pull up resistor. For more info read LPC2138 datasheet.
All other pin are reserved since are used for USB, uSD...

p0.0  in    txd (USB,rs232)  p0.16 in    gpio         p1.16 in    gpio
p0.1  in    rxd (USB,rs232)  p0.17 out 0 sck  (uSD)   p1.17 in    gpio
p0.2  in  # gpio             p0.18 in    miso (uSD)   p1.18 in    gpio
p0.3  in  # gpio             p0.19 out 0 mosi (uSD)   p1.19 in    gpio
p0.4  in    gpio             p0.20 out 0 cs   (uSD)   p1.20 in  * gpio
p0.5  in    gpio             p0.21 in    gpio         p1.21 in    gpio
p0.6  in    gpio             p0.22 in    gpio         p1.22 in    gpio
p0.7  in    gpio             p0.23 in    gpio         p1.23 in    gpio
p0.8  in    gpio             p0.24 ---                p1.24 in    gpio
p0.9  in    gpio             p0.25 in    gpio         p1.25 in    gpio
p0.10 in    gpio             p0.26 in    gpio         p1.26 in  * gpio / JTAG
p0.11 in    card detect(uSD) p0.27 in    gpio         p1.27 in    gpio / JTAG
p0.12 in    gpio             p0.28 in    gpio         p1.28 in    gpio / JTAG
p0.13 out 0 +3V(B) en        p0.29 in    gpio         p1.29 in    gpio / JTAG
p0.14 in  * enter/pgm button p0.30 in    gpio         p1.30 in    gpio / JTAG
p0.15 in    #sleep (USB)     p0.31 out 1 LED          p1.31 in    gpio / JTAG
*/
#define IODIR0_failsafe 0x811a2000
#define IOCLR0_failsafe 0x7fffffff
#define IOSET0_failsafe 0x80000000
#define IODIR1_failsafe 0x0000ffff
#define IOCLR1_failsafe 0x00000000
#define IOSET1_failsafe 0xffffffff

//Power management macros
// enable 3v subsystem (WARNING: after a system reset wait 100mS before turning on)
#define subsystem_3v_on()   (IOSET0=(1<<13))
// disable 3v subsystem (WARNING: make sure uSD pins are @ their failsafe state)
#define subsystem_3v_off()  (IOCLR0=(1<<13))

namespace miosix {

/**
\internal
Put the cpu to power down mode. Used in shutdown()
*/
static inline void goPowerDown()
{
    PCON|=PD;
}
    
#ifdef WITH_RTC
//Commented below in this file
static void rtcInit();
#endif //WITH_RTC

//
// Initialization
//

void IRQbspInit()
{
    //Initialize the system PLL
    setPllFreq(PLL_4X);//Set cpu freq. through pll @ 14.7456MHz * 4 = 59MHz
    set_apb_ratio(APB_DIV_4);//Set peripheral clock ratio 1:4
    //Some registers are only accessed in read-modify-write. Since software reset
    // ( system_reboot() ) does not put those register in a known state, unlike
    //hardware reset, we need to clear them.
    //Clearing PINSEL registers. All pin are GPIO by default
    PINSEL0=0;
    PINSEL1=0;
    //Now setting pin to their failsafe (initialization) pin state
    IODIR0=IODIR0_failsafe;
    IOCLR0=IOCLR0_failsafe;
    IOSET0=IOSET0_failsafe;
    IODIR1=IODIR1_failsafe;
    IOCLR1=IOCLR1_failsafe;
    IOSET1=IOSET1_failsafe;
    
    //Now wait 100ms
    ledOn();
    delayMs(100);
    ledOff();
    //Enable 3v subsystem
    subsystem_3v_on();
    delayMs(50);
    //Peripheral are enabled using an ondemand strategy to save power.
    //So by default, they are disabled.
    PCONP=0;
    
    //Setting the VIC to a known state
    VICSoftIntClr=0xffffffff;//Clear all pending interrupt flags
    VICIntEnClr=0xffffffff;//All interrupts are disabled
    VICIntSelect=0;//All interrupts are assigned to IRQ, not FIQ
    //spurious interrupt handler
    VICDefVectAddr=(unsigned long)&default_IRQ_Routine;
    //Init RTC (if selected)
    #ifdef WITH_RTC
    rtcInit();
    #endif //WITH_RTC
    //Init serial port
    DefaultConsole::instance().IRQset(
        intrusive_ref_ptr<Device>(new LPC2000Serial(0,SERIAL_PORT_SPEED)));
}

void bspInit2()
{
    #ifdef WITH_FILESYSTEM
    #ifdef WITH_DEVFS
    intrusive_ref_ptr<DevFs> devFs=basicFilesystemSetup(SPISDDriver::instance());
    #ifdef AUX_SERIAL
    devFs->addDevice(AUX_SERIAL,
        intrusive_ref_ptr<Device>(new LPC2000Serial(1,AUX_SERIAL_SPEED)));
    #endif //AUX_SERIAL
    #else //WITH_DEVFS
    basicFilesystemSetup();
    #endif //WITH_DEVFS
    #endif //WITH_FILESYSTEM
}

//
// RTC time support
//

#ifdef WITH_RTC
/**
Initializes the RTC
*/
static void rtcInit()
{
    PCONP|=PCRTC;
    CCR=(1<<0) | (1<<4);//Clock enabled, clock source is 32KHz xtal
    CIIR=0;
}

Time rtcGetTime()
{
    Time t;
    unsigned int t0,t1;
    //Reading is tricky because time can overflow while reading
    do {
        t0=CTIME0;
        t1=CTIME1;
    } while(t0!=CTIME0);
    t.sec=(unsigned char)(t0 & 0x3f);
    t.min=(unsigned char)((t0>>8) & 0x3f);
    t.hour=(unsigned char)((t0>>16) & 0x1f);
    t.dow=(DayOfWeek)((t0>>24) & 0x7);
    t.day=(unsigned char)(t1 & 0x1f);
    t.month=(unsigned char)((t1>>8) & 0xf);
    t.year=(unsigned int)((t1>>16) & 0xfff);
    return t;
}

void rtcSetTime(Time t)
{
    PauseKernelLock lock;//The RTC is a shared resource ;)
    CCR&=~(1<<0);//Stop RTC clock
    SEC=(int)t.sec;
    MIN=(int)t.min;
    HOUR=(int)t.hour;
    DOM=(int)t.day;
    DOW=(int)t.dow;
    MONTH=(int)t.month;
    YEAR=(int)t.year;
    CCR|=(1<<0);//Restart RTC clock
}
#endif //WITH_RTC

//
// Shutdown and reboot
//

/**
\internal
Shutdown system.
\param and_return if true, this function returns after wakeup, if false calls
system_reboot() immediately after wakeup
\param t wakeup time, only allowed if WITH_RTC is #define'd
*/
static void _shutdown(bool and_return, Time *t)
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    #ifdef WITH_FILESYSTEM
    if(and_return==false) FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    pauseKernel();

    //wait button release
    while(buttonEnter()) delayMs(20);

    //sleep 100ms
    delayMs(100);

    //Disable interrupts
    disableInterrupts();
    
    //Clearing PINSEL registers. All pin are GPIO by default
    PINSEL0=0;
    PINSEL1=0;
    //Restore failsafe pin state (only non-gpio pin)
    IOSET0=IOSET0_failsafe & (~GPIO_0_MASK);
    IOCLR0=IOCLR0_failsafe & (~GPIO_0_MASK);
    IODIR0=((IODIR0 & GPIO_0_MASK) | (IODIR0_failsafe & (~GPIO_0_MASK)));
    //Not changing IODIR1, IOCLR1 and IOSET1 because are all gpio

    IODIR0|=(1<<18);//making p0.18 uSD miso an output, so if no card present it
    //is not floating
	
    //Set wakup time
    #ifdef WITH_RTC
    if(t!=NULL)
    {
        unsigned char tmp=((t->wakeup_mask) & 0xf);//sec, min, hour, day
        tmp|=(1<<4) | (1<<5);//Day of month and day of year not compared for alarm
        if((t->wakeup_mask) & (1<<4)) tmp|=(1<<6);//month
        if((t->wakeup_mask) & (1<<5)) tmp|=(1<<7);//year
        AMR=tmp;
        ALSEC=(int)(t->sec);
        ALMIN=(int)(t->min);
        ALHOUR=(int)(t->hour);
        ALDOM=(int)(t->day);
        ALDOW=0;//Not used
        ALDOY=0;//Not used
        ALMON=(int)(t->month);
        ALYEAR=(int)(t->year);
    } else {
        AMR=0xff;//Time wakeup disabled
    }
    ILR=0x3;//Clear RTC alarm interrupt flag
    #endif //WITH_RTC

    #ifdef WAKEUP_DELAY
    bool skip_delay=false;
    sleep_again: //We jump here if WAKEUP_DELAY is #define'd and we do not hold
    //down p0.14 enough
    #endif //WAKEUP_DELAY

    //now power down system
    PCON=BODPDM;//Disable BOD to save power
    #ifdef WITH_RTC
    INTWAKE=(1<<1) | (1<<15);//EINT1 (p0.14) + RTC selected
    #else //WITH_RTC
    INTWAKE=(1<<1);//EINT1 (p0.14) selected
    #endif //WITH_RTC
    EXTMODE=0;//Interrupt level sensitive
    EXTPOLAR=0;//Interrupt is active low
    EXTINT=(1<<1);//Clear flag for int1
    PINSEL0|=(1<<29);//p0.14 external interrupt
    goPowerDown();
    PINSEL0&=~(1<<29);//p0.14 standard I/O

    #if defined(WITH_RTC) && defined(WAKEUP_DELAY)
    //If we woke because of a RTC timeout, no button delay
    if(ILR & (1<<1)) skip_delay=true;
    #endif //WITH_RTC

    #ifdef WAKEUP_DELAY
    if(skip_delay==false)
    {
        //user must hold down enter button for 2 seconds
        int i;
        for(i=0;i<20;i++)
        {
            //Waiting 100/4 because PLL is not yet enabled, clock frequency is low
            delayMs(100/4);
            if(!buttonEnter()) goto sleep_again;
        }
    }
    #endif//WAKEUP_DELAY

    if(and_return==false) miosix_private::IRQsystemReboot();

    //Initialize the system PLL (Power down mode resets pll to 1x)
    setPllFreq(PLL_4X);//Set cpu freq. through pll @ 14.7456MHz * 4 = 59MHz
    set_apb_ratio(APB_DIV_4);//Set peripheral clock ratio 1:4

    IODIR0&=~(1<<18);//restoring p0.18 uSD miso as input.

    //Now wait 50ms
    ledOn();
    delayMs(50);
    ledOff();
    //Enable 3v subsystem
    subsystem_3v_on();
    delayMs(50);
    
    //Re-enable interrupts
    enableInterrupts();

    restartKernel();
}

void sleep(Time *t)
{
    _shutdown(true,t);
}

/**
This function disables filesystem (if enabled), serial port (if enabled) and
puts the processor in deep sleep mode.<br>
Wakeup occurs when p0.14 goes low, but instead of sleep(), a new boot happens.
<br>This function does not return.<br>
WARNING: close all files before using this function, since it unmounts the
filesystem.<br>
When in shutdown mode, power consumption of the miosix board is reduced to ~
150uA, however, true power consumption depends on what is connected to the GPIO
pins. The user is responsible to put the devices connected to the GPIO pin in the
minimal power consumption mode before calling shutdown(). Please note that to
minimize power consumption all unused GPIO must be set as output high.
*/
void shutdown()
{
    _shutdown(false,NULL);
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    
    #ifdef WITH_FILESYSTEM
    FilesystemManager::instance().umountAll();
    #endif //WITH_FILESYSTEM

    disableInterrupts();
    //Clearing PINSEL registers. All pin are GPIO by default
    PINSEL0=0;
    PINSEL1=0;
    //Restore failsafe pin state (only non-gpio pin)
    IOSET0=IOSET0_failsafe & (~GPIO_0_MASK);
    IOCLR0=IOCLR0_failsafe & (~GPIO_0_MASK);
    IODIR0=((IODIR0 & GPIO_0_MASK) | (IODIR0_failsafe & (~GPIO_0_MASK)));
    //Not changing IODIR1, IOCLR1 and IOSET1 because are all gpio
    delayMs(100);
    miosix_private::IRQsystemReboot();
}

//
// System PLL
//

#define PLOCK 0x400

/**
\internal
Generates the PLL feed sequence
*/
static inline void feed()
{
    PLLFEED=0xAA;
    PLLFEED=0x55;
}

void setPllFreq(PllValues r)
{
    PLLCON=0x0;//PLL OFF
    feed();
    switch(r)
    {
        case PLL_2X://PLL=2*F
            // Enabling MAM
            MAMCR=0x0;//MAM Disabled
            MAMTIM=0x2;//Flash access is 2 cclk
            MAMCR=0x2;//MAM Enabled
            // Setting PLL
            PLLCFG=0x41;// M=2 P=4 ( FCCO=14.7456*M*2*P=235MHz )
            feed();
            PLLCON=0x1;//PLL Enabled
            feed();
            while(!(PLLSTAT & PLOCK));//Wait for PLL to lock
            PLLCON=0x3;//PLL Connected
            feed();
            break;
        case PLL_4X://PLL=4*F
            // Enabling MAM
            MAMCR=0x0;//MAM Disabled
            MAMTIM=0x3;//Flash access is 3 cclk
            MAMCR=0x2;//MAM Enabled
            // Setting PLL
            PLLCFG=0x23;// M=4 P=2 ( FCCO=14.7456*M*2*P=235MHz )
            feed();
            PLLCON=0x1;//PLL Enabled
            feed();
            while(!(PLLSTAT & PLOCK));//Wait for PLL to lock
            PLLCON=0x3;//PLL Connected
            feed();
            break;
        default://PLL OFF
            // Enabling MAM
            MAMCR=0x0;//MAM Disabled
            MAMTIM=0x1;//Flash access is 1 cclk
            MAMCR=0x2;//MAM Enabled
            break;
    }
}

PllValues getPllFreq()
{
    //If "pll off" or "pll not connected" return PLL_OFF
    if((PLLSTAT & 0x300)!=0x300) return PLL_OFF;
    switch(PLLSTAT & 0x7f)
    {
        case 0x41: return PLL_2X;
        case 0x23: return PLL_4X;
        default:   return PLL_UNDEF;//PLL undefined value
    }
}

} //namespace miosix
