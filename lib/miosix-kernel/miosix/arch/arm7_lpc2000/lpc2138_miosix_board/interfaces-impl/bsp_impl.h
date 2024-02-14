/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010 by Terraneo Federico                   *
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
* bsp_impl.h Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#ifndef BSP_IMPL_H
#define BSP_IMPL_H

#include "LPC213x.h"
#include "config/miosix_settings.h"

/**
\addtogroup Hardware
\{
*/

/**
Turn on LED connected to P0.31
*/
#define ledOn()    IOCLR0=(1<<31)

/**
Turn off LED connected to P0.31
*/
#define ledOff()   IOSET0=(1<<31)

/**
Polls the SD card sense GPIO
\return true if there is an uSD card in the socket. 
*/
#define sdCardSense() (!(IOPIN0 & (1<<11)))

/**
\return true if the  USB cable is connected.
*/
#define USBstatus() (IOPIN0 & (1<<15))

/**
Polls the button on the miosix board
\return true if the button is pushed.
*/
#define buttonEnter() (!(IOPIN0 & (1<<14)))

//All bits related to gpio pins of port 0 are @ 1
#define GPIO_0_MASK 0x7ee117fc

/**
Set one or more gpio pin of port 0.
If the pin is selected as output, it will become high.
Available pin are p0.2 p0.3 p0.4 p0.5 p0.6 p0.7 p0.8 p0.9 p0.10 p0.12
p0.16 p0.21 p0.22 p0.23 p0.25 p0.26 p0.27 p0.28 p0.29 p0.30.
To set a pin with number k, the k-th bit in the x variable must be set.
Example: to set pin 4,7 and 8 use:
\code pin_0_set( (1<<4) | (1<<7) | (1<<8) );\endcode
\param x unsigned int where bits to set are @ 1
*/
#define pin_0_set(x) { IOSET0=((x) & GPIO_0_MASK); }

/**
Clear one or more gpio pin of port 0.
If the pin is selected as output, it will become low.
Available pin are p0.2 p0.3 p0.4 p0.5 p0.6 p0.7 p0.8 p0.9 p0.10 p0.12
p0.16 p0.21 p0.22 p0.23 p0.25 p0.26 p0.27 p0.28 p0.29 p0.30.
To clear a pin with number k, the k-th bit in the x variable must be set.
Example: to clear pin 4,7 and 8 use:
\code pin_0_clr( (1<<4) | (1<<7) | (1<<8) );\endcode
\param x unsigned int where bits to clear are @ 1
*/
#define pin_0_clr(x) { IOCLR0=((x) & GPIO_0_MASK); }

/**
Read one or more gpio pin of port 0.
If the pin is selected as input, its value can be read.
Available pin are p0.2 p0.3 p0.4 p0.5 p0.6 p0.7 p0.8 p0.9 p0.10 p0.12
p0.16 p0.21 p0.22 p0.23 p0.25 p0.26 p0.27 p0.28 p0.29 p0.30.
If the pin with number k is high, the k-th bit in the x variable will be set.
The value of pin selected as output and of non-gpio pins is unspecified. 
Example: to read pin 9 use:
\code if(pin_0_read() & (1<<9)) { pin 9 is high } else { pin 9 is low }\endcode
\return unsigned int where bits set are @ 1
*/
#define pin_0_read() (IOPIN0)

/**
Modify the state of one or more pin of port 0, so that they become output.
Once the pin are output, you can write to them using pin_0_set() and pin_0_clr()
Available pin are p0.2 p0.3 p0.4 p0.5 p0.6 p0.7 p0.8 p0.9 p0.10 p0.12
p0.16 p0.21 p0.22 p0.23 p0.25 p0.26 p0.27 p0.28 p0.29 p0.30.
To change the state of a pin with number k, the k-th bit in the x variable must
be set.
Example: to set pin 4,7 and 8 as output use:
\code mode_0_out( (1<<4) | (1<<7) | (1<<8) );\endcode
\param x unsigned int where bits to set as output are @ 1
*/
#define mode_0_out(x) { IODIR0 |= ((x) & GPIO_0_MASK); }

/**
Modify the state of one or more pin of port 0, so that they become input.
Once the pin are input, you can read them using pin_0_read()
Available pin are p0.2 p0.3 p0.4 p0.5 p0.6 p0.7 p0.8 p0.9 p0.10 p0.12
p0.16 p0.21 p0.22 p0.23 p0.25 p0.26 p0.27 p0.28 p0.29 p0.30.
To change the state of a pin with number k, the k-th bit in the x variable must
be set.
Example: to set pin 4,7 and 8 as input use:
\code mode_0_in( (1<<4) | (1<<7) | (1<<8) );\endcode
\param x unsigned int where bits to set as input are @ 1
*/
#define mode_0_in(x) { IODIR0 &= ((x) | (~GPIO_0_MASK)); }

/**
Set one or more gpio pin of port 1.
If the pin is selected as output, it will become high.
Available pin are in range from p0.16 to p0.31.
To set a pin with number k, the k-th bit in the x variable must be set.
Example: to set pin 16 and 18 use:
\code pin_1_set( (1<<16) | (1<<18) );\endcode
\param x unsigned int where bits to set are @ 1
*/
#define pin_1_set(x) { IOSET1=(x); }

/**
Clear one or more gpio pin of port 1.
If the pin is selected as output, it will become low.
Available pin are in range from p0.16 to p0.31.
To clear a pin with number k, the k-th bit in the x variable must be set.
Example: to clear pin 16 and 18 use:
\code pin_1_clr( (1<<16) | (1<<18) );\endcode
\param x unsigned int where bits to clear are @ 1
*/
#define pin_1_clr(x) { IOCLR1=(x); }

/**
Read one or more gpio pin of port 1.
If the pin is selected as input, its value can be read.
Available pin are in range from p0.16 to p0.31.
If the pin with number k is high, the k-th bit in the x variable will be set.
The value of pin selected as output and of non-gpio pins is unspecified. 
Example: to read pin 19 use:
\code if(pin_1_read() & (1<<19)) { pin 19 is high } else { pin 19 is low }\endcode
\return unsigned int where bits set are @ 1
*/
#define pin_1_read() (IOPIN1)

/**
Modify the state of one or more pin of port 1, so that they become output.
Once the pin are output, you can write to them using pin_1_set() and pin_1_clr()
Available pin are in range from p0.16 to p0.31.
To change the state of a pin with number k, the k-th bit in the x variable must
be set.
Example: to set pin 16 and 18 as output use:
\code mode_1_out( (1<<16) | (1<<18) );\endcode
\param x unsigned int where bits to set as output are @ 1
*/
#define mode_1_out(x) { IODIR1 |= (x); }

/**
Modify the state of one or more pin of port 1, so that they become input.
Once the pin are input, you can read them using pin_1_read()
Available pin are in range from p0.16 to p0.31.
To change the state of a pin with number k, the k-th bit in the x variable must
be set.
Example: to set pin 16 and 18 as input use:
\code mode_1_in( (1<<16) | (1<<18) );\endcode
\param x unsigned int where bits to set as input are @ 1
*/
#define mode_1_in(x) { IODIR1 &= (x); }

//Bits of PCONP register
#define PCTIM0 (1<<1)
#define PCTIM1 (1<<2)
#define PCUART0 (1<<3)
#define PCUART1 (1<<4)
#define PCPWM0 (1<<5)
#define PCI2C0 (1<<7)
#define PCSPI0 (1<<8)
#define PCRTC (1<<9)
#define PCSPI1 (1<<10)
#define PCAD0 (1<<12)
#define PCI2C1 (1<<19)
#define PCAD1 (1<<20)

//Bits of PCON register
#define IDL (1<<0)
#define PD (1<<1)
#define BODPDM (1<<2)
#define BOGD (1<<3)
#define BORD (1<<4)

///No operation instruction.
///Expands to an assembler "nop".
#define nop() { asm volatile("nop"::); }

/**
\}
*/

namespace miosix {

/**
\addtogroup Hardware
\{
*/

/**
\enum DayOfWeek
Day of week field in Time struct 
*/
enum DayOfWeek
{
    MON,///<Monday
    TUE,///<Tuesday
    WED,///<Wednesday
    THU,///<Thursday
    FRI,///<Friday
    SAT,///<Saturday
    SUN ///<Sunday
};

/**
\struct Time
Time struct, used to get/set time.
It can also be passed to sleep() to automatically wakeup the device at any
given time.
*/
struct Time
{
    unsigned char sec;  ///< Seconds, in range 0..59
    unsigned char min;  ///< Minutes, in range 0..59
    unsigned char hour; ///< Hours, in range 0..23
    unsigned char day;  ///< Day, in range 1..31
    unsigned char month;///< Month, in range 1..12
    unsigned int year;  ///< Year, in range 0..4095 ;)
    DayOfWeek dow;///<Day of week. This is not taken into account for wakeup time
    /**
    This is only used for wakeup time
    - bit 0 = if @ 1, sec is not considered for wakeup time
    - bit 1 = if @ 1, min is not considered for wakeup time
    - bit 2 = if @ 1, hour is not considered for wakeup time 
    - bit 3 = if @ 1, day is not considered for wakeup time
    - bit 4 = if @ 1, month is not considered for wakeup time
    - bit 5 = if @ 1, year is not considered for wakeup time
    */
    unsigned char wakeup_mask;
};

#ifdef WITH_RTC
/**
Read time from RTC
\return current time
*/
Time rtcGetTime();

/**
Set current time
\param t time that will be written into RTC
*/
void rtcSetTime(Time t);
#endif //WITH_RTC

/**
This function disables filesystem (if enabled), serial port (if enabled) and
puts the processor in deep sleep mode. After wakeup, the original status of
filesystem and serial port is restored.<br>If t is NULL, wakeup only
occurs when p0.14 goes low. If t is a valid time, wakeup occurs when that time
is reached OR when p0.14 goes low, whichever occurs first.<br>
Note that time support is only enabled if WITH_RTC is \#define'd in
miosix_settings.h. If it is not defined, time will be ignored.<br>
WARNING: close all files before using this function, since it unmounts the
filesystem.<br>
Note: if using the wakeup time, it must be at least 2 seconds after the moment
where sleep() is called, otherwise wakeup will not occur.<br>
When in sleep mode, power consumption of the miosix board is reduced to ~ 150uA,
however, true power consumption depends on what is connected to the GPIO pins.
The user is responsible to put the devices connected to the GPIO pin in the
minimal power consumption mode before calling sleep(). Please note that to
minimize power consumption all unused GPIO must be set as output high.
\param t wakeup time, or NULL
*/
void sleep(Time *t);

/**
\internal
Constants to pass to setPllFreq(), and values returned by getPllFreq()
This enum is used by the system and should not be called by user code.
*/
enum PllValues
{
    PLL_4X=4,   ///<\internal AHB clock frequency = xtal freq. multiplied by 4
    PLL_2X=2,   ///<\internal AHB clock frequency = xtal freq. multiplied by 2
    PLL_OFF=0,  ///<\internal AHB clock frequency = xtal freq.
    PLL_UNDEF=-1///<\internal Returned by getPllFreq() in case of errors
};

/**
\internal
Set PLL frequency
<br>Warning! Disable interrupts before changing PLL frequency
<br>Warning! This is meant to work with a 14.7456MHz xtal, and is UNTESTED with other values
<br>Warning! Changing PLL frequency may cause problems to hardware drivers
\param r the desired PLL settings

This function is used by the system and should not be called by user code.
*/
void setPllFreq(PllValues r);

/**
\internal
Get PLL settings
Warning! This is meant to work with a 14.7456MHz xtal, and is UNTESTED with other values
\return the current PLL settings

This function is used by the system and should not be called by user code.
*/
PllValues getPllFreq();

/**
\internal
Constants to pass to set_apb_ratio(), and values returned by get_apb_ratio()
This enum is used by the system and should not be called by user code.
*/
enum APBValues
{
    APB_DIV_1=1,///<\internal AHB and APB run @ same speed
    APB_DIV_2=2,///<\internal APB runs at 1/2 of AHB frequency
    APB_DIV_4=0 ///<\internal APB runs at 1/4 of AHB frequency
};

/**
\internal
Set apb ratio (how much peripherals are slowed down with respect to cpu)
Warning! Changing apb ratio may cause problems to hardware drivers
\param r desired apb ratio

This function is used by the system and should not be called by user code.
*/
inline void set_apb_ratio(APBValues r)
{
    VPBDIV=(r);
}

/**
\internal
Get apb ratio (how much peripherals are slowed down with respect to cpu)
\return current apb ratio

This function is used by the system and should not be called by user code.
*/
inline APBValues get_apb_ratio()
{
    return (APBValues)(VPBDIV & 0x3);
}

/**
\}
*/

} //namespace miosix

#endif //BSP_IMPL_H
