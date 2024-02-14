/***************************************************************************
 *   Copyright (C) 2017 by Marsella Daniele                                *
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

#include "rtc.h"
#include <miosix.h>
#include <sys/ioctl.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/kernel.h>
#include <kernel/logging.h>

using namespace miosix;


namespace miosix {

//
// class Rtc
//
void IRQrtcInit() {
    Rtc::instance(); // create the singleton
}

Rtc& Rtc::instance()
{
    static Rtc singleton;
    return singleton;
}

//   unsigned short int Rtc::getSSR() 
//   {
//     short int ss = 0;
//     {
//       FastInterruptDisableLock dlock;
//       ss = IRQgetSSR();
//     }
//     return ss;
//   }

//   unsigned long long int Rtc::getTime()
//   {
//     long long int time = 0;
//     {
//       FastInterruptDisableLock dlock;
//       time = IRQgetTime();      
//     }
//     return time;
//   }

//   unsigned long long int Rtc::getDate()
//   {
//     long long int date = 0;
//     {
//       FastInterruptDisableLock dlock;
//       date = IRQgetDate();
//     }
//     return date;
//   }

// // Return the value of RTC_SSR register
//  unsigned short int Rtc::IRQgetSSR()
//  {
//    while((RTC->ISR & RTC_ISR_RSF) == 0);
//    unsigned short int ssr = RTC->SSR & RTC_SSR_SS;
//    RTC->ISR &= ~(RTC_ISR_RSF);
//    return ssr;
//   }

//   // Return day time as microseconds
//   unsigned long long int Rtc::IRQgetTime()
//   {
//     while(( RTC->ISR & RTC_ISR_RSF) == 0);
//     unsigned short int hour_tens = 0x00000000 | (RTC->TSTR & RTC_TSTR_HT);
//     unsigned short int hour_units = 0x00000000 | (RTC->TSTR & RTC_TSTR_HU);
//     unsigned short int minute_tens = 0x00000000 | (RTC->TSTR & RTC_TSTR_MNT);
//     unsigned short int minute_units = 0x00000000 | (RTC->TSTR & RTC_TSTR_MNU);
//     unsigned short int second_tens = 0x00000000 | (RTC->TSTR & RTC_TSTR_ST);
//     unsigned short int second_units = 0x00000000 | (RTC->TSTR & RTC_TSTR_SU);
//     unsigned short int ss_value = 0x00000000 | (RTC->SSR & RTC_SSR_SS);
//     unsigned long long int time_microsecs = ( hour_tens * 10 + hour_units) * 3600 * 1000000
//       + ( minute_tens * 10 + minute_units) * 60 * 1000000
//       + ( second_tens * 10 + second_units ) * 1000000
//       + ( prescaler_s - ss_value )/(prescaler_s + 1);
//     RTC->ISR &= ~(RTC_ISR_RSF);
//     if ( (RTC->TSTR & RTC_TSTR_PM) == 0 ) 
//       return time_microsecs;
//     else
//       return time_microsecs + 12 * 3600000000;
//   }

//   // \todo
//   unsigned long long int Rtc::IRQgetDate() 
//   {
//     unsigned long long int date_secs = 0;
//     while(( RTC->ISR & RTC_ISR_RSF) == 0);
//     unsigned short int hour_tens = 0x00000000 | (RTC->TR & RTC_TR_HT);
//     unsigned short int hour_units = 0x00000000 | (RTC->TR & RTC_TR_HU);
//     unsigned short int minute_tens = 0x00000000 | (RTC->TR & RTC_TR_MNT);
//     unsigned short int minute_units = 0x00000000 | (RTC->TR & RTC_TR_MNU);
//     unsigned short int second_tens = 0x00000000 | (RTC->TR & RTC_TR_ST);
//     unsigned short int second_units = 0x00000000 | (RTC->TR & RTC_TR_SU);
//     unsigned short int subseconds = 0x00000000 | (RTC->SSR & RTC_SSR_SS);
//     unsigned short int year_tens = 0x000000000 | (RTC->DR & RTC_DR_YT);
//     unsigned short int year_units = 0x000000000 | (RTC->DR & RTC_DR_YU);
//     unsigned short int month_tens = 0x000000000 | (RTC->DR & RTC_DR_MT);
//     unsigned short int month_units = 0x000000000 | (RTC->DR & RTC_DR_MU);
//     unsigned short int day_tens = 0x000000000 | (RTC->DR & RTC_DR_DT);
//     unsigned short int day_units = 0x000000000 | (RTC->DR & RTC_DR_DU);
//     // \todo
//     RTC->ISR &= ~(RTC_ISR_RSF);
//     return date_secs;
//   }

void Rtc::setWakeupInterrupt() 
{
    EXTI->EMR &= ~(1<<11);
    EXTI->RTSR &= ~(1<<11);
    EXTI->EMR |= (1<<22);
    EXTI->RTSR |= (1<<22);
    EXTI->FTSR &= ~(1<<22);
    RTC->CR |= RTC_CR_WUTIE;
    wakeupOverheadNs = 100000;
}

void Rtc::setWakeupTimer(unsigned short int wut_value)
{
    RTC->CR &= ~RTC_CR_WUTE;
    while( (RTC->ISR & RTC_ISR_WUTWF ) == 0 );
    RTC->CR |= (RTC_CR_WUCKSEL_0 | RTC_CR_WUCKSEL_1);
    RTC->CR &= ~(RTC_CR_WUCKSEL_2) ; //! select RTC/2 clock for wakeup
    RTC->WUTR = wut_value & RTC_WUTR_WUT;  
}

void Rtc::startWakeupTimer()
{
    RTC->CR |= RTC_CR_WUTE;
}

void  Rtc::stopWakeupTimer()
{
    RTC->CR &= ~RTC_CR_WUTE;
    RTC->ISR &= ~RTC_ISR_WUTF;
}

long long Rtc::getWakeupOverhead() 
{
    return wakeupOverheadNs;
}

long long Rtc::getMinimumDeepSleepPeriod() 
{
    return minimumDeepSleepPeriod;
}

Rtc::Rtc() :
    clock_freq(32768),
    wkp_clock_period(1000000000 * 2 / clock_freq),
    wkp_tc(clock_freq / 2)
{
    {
        InterruptDisableLock dl;
        RCC->APB1ENR |= RCC_APB1ENR_PWREN;
        PWR->CR |= PWR_CR_DBP;

        // Without the next two lines the BDCR bit for LSEON
        // is ignored and the next while loop never exits
        RCC->BDCR = RCC_BDCR_BDRST;
        RCC->BDCR = 0;

        RCC->BDCR |= RCC_BDCR_RTCEN       //RTC enabled
                   | RCC_BDCR_LSEON       //External 32KHz oscillator enabled
                   | RCC_BDCR_RTCSEL_0;   //Select LSE as clock source for RTC
        RCC->BDCR &= ~(RCC_BDCR_RTCSEL_1);
        RCC->CFGR |= (0b1<<21);
    }
    ledOn();
    while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait for LSE to start
    ledOff();
    // Enable write on RTC_ISR register
    RTC->WPR = 0xCA;
    RTC->WPR = 0x53;

    RTC->CR &= ~(RTC_CR_BYPSHAD);
    RTC->PRER = (128 <<16) | ( 256<<0); // default prescaler
    RTC->ISR |= RTC_ISR_INIT;
    while((RTC->ISR & RTC_ISR_INITF)== 0) ; // wait clock and calendar initialization
    RTC->TR = (RTC_TR_SU & 0x0)  | (RTC_TR_ST & 0x0) | (RTC_TR_MNU & 0x0)
            | (RTC_TR_MNT & 0x0) | (RTC_TR_HU & 0x0) | (RTC_TR_HT & 0x0); 
    RTC->DR = (1<<0) | (1<<8) | (1<<13) | (9<<16) | (1<<20); // initialized to 1/1/2019

    RTC->CR &= ~(RTC_CR_FMT); // Use 24-hour format
    RTC->ISR &= ~(RTC_ISR_INIT);
    prescaler_s = 0x00000000 |  (RTC->PRER & RTC_PRER_PREDIV_S);
    // NVIC_SetPriority(RTC_WKUP_IRQn,10);
    // NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

} //namespace miosix
