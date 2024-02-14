/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014 by Terraneo Federico and               *
 *                                     Marsella Daniele                    *
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

#include "interfaces/deep_sleep.h"
#include "interfaces/os_timer.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "drivers/rtc.h"
#include "miosix.h"
#include <algorithm>


/// /def DEBUG_DEEP_SLEEP
/// This debug symbol makes the deep sleep routine to make
/// a led blink when the board enter the stop mode
/// #define DEBUG_DEEP_SLEEP

using namespace std;

namespace miosix {

static Rtc* rtc = nullptr;

static void IRQsetSystemClock()
{
    RCC->CR |= RCC_CR_HSEON | RCC_CR_HSION;
    while((RCC->CR & RCC_CR_HSERDY)==0) ;
    while((RCC->CR & RCC_CR_HSIRDY)==0) ;

    /* Select HSI */
    RCC->CFGR &= ~(RCC_CFGR_SW);
    while((RCC->CFGR & RCC_CFGR_SWS ) != RCC_CFGR_SWS_HSI) ;

    //Configure PLL and turn it on
    const int m=HSE_VALUE/1000000;
    const int n=336;
    const int p=2;
    const int q=7;
    RCC->PLLCFGR=m | (n<<6) | (((p/2)-1)<<16) | RCC_PLLCFGR_PLLSRC_HSE | (q<<24);
    RCC->CR |= RCC_CR_PLLON;
    while((RCC->CR & RCC_CR_PLLRDY)==0) ;

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    FLASH->ACR &= ~FLASH_ACR_LATENCY;

    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

    FLASH->ACR= FLASH_ACR_ICEN
              | FLASH_ACR_DCEN
              | FLASH_ACR_LATENCY_5WS;

    /* Select the main PLL as system clock source */
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    while((RCC->CFGR & RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL) ;
}
  
static void IRQenterStopMode()
{
    // Safe procedure to enter stopmode
    RTC->CR &= ~(RTC_CR_WUTIE);
    RTC->ISR &= ~(RTC_ISR_WUTF);
    PWR->CSR &= ~(PWR_CSR_WUF);
    EXTI->PR = 0xffff; // reset pending bits
    RTC->CR |= RTC_CR_WUTIE;
    rtc->startWakeupTimer(); 
    //Enter stop mode by issuing a WFE
    PWR->CR |= PWR_CR_FPDS  //Flash in power down while in stop
             | PWR_CR_LPDS; //Regulator in low power mode while in stop
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //Select stop mode
    __SEV();
    __WFE();
    __WFE(); // fast fix to a bug of STM32F4
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; //Unselect stop mode
    IRQsetSystemClock();
    //Configure PLL and turn it on
    rtc->stopWakeupTimer();
}

void IRQdeepSleepInit()
{
    rtc = &Rtc::instance();
    rtc->setWakeupInterrupt();
}
  
bool IRQdeepSleep(long long int abstime)
{
    long long reltime = abstime - IRQgetTime();
    reltime = max(reltime - rtc->stopModeOffsetns, 0LL);
    if(reltime < rtc->getMinimumDeepSleepPeriod())
    {
        // Too late for deep-sleep, use normal sleep
        return false;
    } else {
#ifdef DEBUG_DEEP_SLEEP
        _led::high();
#endif
      
        rtc->setWakeupInterrupt();
        long long wut_ticks = rtc->wkp_tc.ns2tick(reltime);
        unsigned int remaining_wakeups = wut_ticks / 0xffff;
        unsigned int last_wut = wut_ticks % 0xffff;
        while(remaining_wakeups > 0)
        {
            rtc->setWakeupTimer(0xffff);
            IRQenterStopMode();
            remaining_wakeups--;
        }
        rtc->setWakeupTimer(last_wut);
        IRQenterStopMode();
#ifdef DEBUG_DEEP_SLEEP
        _led::low();
#endif
        internal::IRQosTimerSetTime(abstime);
    }
    return true;
}

bool IRQdeepSleep()
{
    return IRQdeepSleep(3600000000000); //Just wait a long time, 3600s
}

} //namespace miosix
