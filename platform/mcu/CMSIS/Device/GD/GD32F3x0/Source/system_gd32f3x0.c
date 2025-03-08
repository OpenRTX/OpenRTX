/*!
    \file  system_gd32f3x0.c
    \brief CMSIS Cortex-M4 Device Peripheral Access Layer Source File for
           GD32F3x0 Device Series
*/

/*  Copyright (c) 2012 ARM LIMITED
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

/* This file refers the CMSIS standard, some adjustments are made according to GigaDevice chips */

#include "gd32f3x0.h"

/* system frequency define */
#define __IRC8M           (IRC8M_VALUE)            /* internal 8 MHz RC oscillator frequency */
#define __HXTAL           (HXTAL_VALUE)            /* high speed crystal oscillator frequency */
#define __SYS_OSC_CLK     (__IRC8M)                /* main oscillator frequency */

#define VECT_TAB_OFFSET  (uint32_t)0x00            /* vector table base offset */

/* select a system clock by uncommenting the following line */
#if defined (GD32F310)
//#define __SYSTEM_CLOCK_8M_HXTAL              (__HXTAL)
//#define __SYSTEM_CLOCK_8M_IRC8M              (__IRC8M)
#define __SYSTEM_CLOCK_72M_PLL_HXTAL         (uint32_t)(72000000)
//#define __SYSTEM_CLOCK_72M_PLL_IRC8M_DIV2    (uint32_t)(72000000)
//#define __SYSTEM_CLOCK_72M_PLL_IRC48M_DIV2     (uint32_t)(72000000)
#endif /* GD32F310 */

#if defined (GD32F330)
//#define __SYSTEM_CLOCK_8M_HXTAL              (__HXTAL)
//#define __SYSTEM_CLOCK_8M_IRC8M              (__IRC8M)
//#define __SYSTEM_CLOCK_72M_PLL_HXTAL         (uint32_t)(72000000)
//#define __SYSTEM_CLOCK_72M_PLL_IRC8M_DIV2    (uint32_t)(72000000)
//#define __SYSTEM_CLOCK_72M_PLL_IRC48M_DIV2     (uint32_t)(72000000)
#define __SYSTEM_CLOCK_84M_PLL_HXTAL           (uint32_t)(108000000)
//#define __SYSTEM_CLOCK_84M_PLL_IRC8M_DIV2    (uint32_t)(84000000)
#endif /* GD32F330 */

#if defined (GD32F350)
//#define __SYSTEM_CLOCK_8M_HXTAL              (__HXTAL)
//#define __SYSTEM_CLOCK_8M_IRC8M              (__IRC8M)
//#define __SYSTEM_CLOCK_72M_PLL_HXTAL         (uint32_t)(72000000)
//#define __SYSTEM_CLOCK_72M_PLL_IRC8M_DIV2    (uint32_t)(72000000)
//#define __SYSTEM_CLOCK_84M_PLL_HXTAL         (uint32_t)(84000000)
//#define __SYSTEM_CLOCK_84M_PLL_IRC8M_DIV2    (uint32_t)(84000000)
//#define __SYSTEM_CLOCK_96M_PLL_HXTAL         (uint32_t)(96000000)
//#define __SYSTEM_CLOCK_96M_PLL_IRC8M_DIV2      (uint32_t)(96000000)
//#define __SYSTEM_CLOCK_96M_PLL_IRC48M_DIV2     (uint32_t)(96000000)
#define __SYSTEM_CLOCK_108M_PLL_HXTAL        (uint32_t)(108000000)
//#define __SYSTEM_CLOCK_108M_PLL_IRC8M_DIV2   (uint32_t)(108000000)
#endif /* GD32F350 */

#define SEL_IRC8M       0x00
#define SEL_HXTAL       0x01
#define SEL_PLL         0x02
#define RCU_MODIFY(__delay)     do{                                     \
                                    volatile uint32_t i;                \
                                    if(0 != __delay){                   \
                                        RCU_CFG0 |= RCU_AHB_CKSYS_DIV2; \
                                        for(i=0; i<__delay; i++){       \
                                        }                               \
                                        RCU_CFG0 |= RCU_AHB_CKSYS_DIV4; \
                                        for(i=0; i<__delay; i++){       \
                                        }                               \
                                    }                                   \
                                }while(0)

/* set the system clock frequency and declare the system clock configuration function */
#ifdef __SYSTEM_CLOCK_8M_HXTAL
uint32_t SystemCoreClock = __SYSTEM_CLOCK_8M_HXTAL;
static void system_clock_8m_hxtal(void);

#elif defined (__SYSTEM_CLOCK_72M_PLL_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_72M_PLL_HXTAL;
static void system_clock_72m_hxtal(void);

#elif defined (__SYSTEM_CLOCK_72M_PLL_IRC8M_DIV2)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_72M_PLL_IRC8M_DIV2;
static void system_clock_72m_irc8m(void);

#elif defined (__SYSTEM_CLOCK_72M_PLL_IRC48M_DIV2)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_72M_PLL_IRC48M_DIV2;
static void system_clock_72m_irc48m(void);

#elif defined (__SYSTEM_CLOCK_84M_PLL_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_84M_PLL_HXTAL;
static void system_clock_84m_hxtal(void);

#elif defined (__SYSTEM_CLOCK_84M_PLL_IRC8M_DIV2)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_84M_PLL_IRC8M_DIV2;
static void system_clock_84m_irc8m(void);

#elif defined (__SYSTEM_CLOCK_96M_PLL_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_96M_PLL_HXTAL;
static void system_clock_96m_hxtal(void);

#elif defined (__SYSTEM_CLOCK_96M_PLL_IRC8M_DIV2)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_96M_PLL_IRC8M_DIV2;
static void system_clock_96m_irc8m(void);

#elif defined (__SYSTEM_CLOCK_96M_PLL_IRC48M_DIV2)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_96M_PLL_IRC48M_DIV2;
static void system_clock_96m_irc48m(void);

#elif defined (__SYSTEM_CLOCK_108M_PLL_HXTAL)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_108M_PLL_HXTAL;
static void system_clock_108m_hxtal(void);

#elif defined (__SYSTEM_CLOCK_108M_PLL_IRC8M_DIV2)
uint32_t SystemCoreClock = __SYSTEM_CLOCK_108M_PLL_IRC8M_DIV2;
static void system_clock_108m_irc8m(void);

#else
uint32_t SystemCoreClock = __SYSTEM_CLOCK_8M_IRC8M;
static void system_clock_8m_irc8m(void);
#endif /* __SYSTEM_CLOCK_8M_HXTAL */

/* configure the system clock */
static void system_clock_config(void);

/*!
    \brief      setup the microcontroller system, initialize the system
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SystemInit(void)
{
#if (defined(GD32F350))
    RCU_APB2EN |= BIT(0);
    CMP_CS |= (CMP_CS_CMP1MSEL | CMP_CS_CMP0MSEL);
#endif /* GD32F350 */
    if(((FMC_OBSTAT & OB_OBSTAT_PLEVEL_HIGH) != OB_OBSTAT_PLEVEL_HIGH) &&
            (((FMC_OBSTAT >> 13) & 0x1) == SET)) {
        FMC_KEY = UNLOCK_KEY0;
        FMC_KEY = UNLOCK_KEY1 ;
        FMC_OBKEY = UNLOCK_KEY0;
        FMC_OBKEY = UNLOCK_KEY1 ;
        FMC_CTL |= FMC_CTL_OBER;
        FMC_CTL |= FMC_CTL_START;
        while((uint32_t)0x00U != (FMC_STAT & FMC_STAT_BUSY));
        FMC_CTL &= ~FMC_CTL_OBER;
        FMC_CTL |= FMC_CTL_OBPG;
        if((FMC_OBSTAT & OB_OBSTAT_PLEVEL_HIGH) == OB_OBSTAT_PLEVEL_NO) {
            OB_SPC = FMC_NSPC;
        } else if((FMC_OBSTAT & OB_OBSTAT_PLEVEL_HIGH) == OB_OBSTAT_PLEVEL_LOW) {
            OB_SPC = FMC_LSPC;
        }
        OB_USER = OB_USER_DEFAULT & ((uint8_t)(FMC_OBSTAT >> 8));
        OB_DATA0 = ((uint8_t)(FMC_OBSTAT >> 16));
        OB_DATA1 = ((uint8_t)(FMC_OBSTAT >> 24));
        OB_WP0 = ((uint8_t)(FMC_WP));
        OB_WP1 = ((uint8_t)(FMC_WP >> 8));
        while((uint32_t)0x00U != (FMC_STAT & FMC_STAT_BUSY));
        FMC_CTL &= ~FMC_CTL_OBPG;
        FMC_CTL &= ~FMC_CTL_OBWEN;
        FMC_CTL |= FMC_CTL_LK;
    }
    /* FPU settings */
#if (__FPU_PRESENT == 1U) && (__FPU_USED == 1U)
    SCB->CPACR |= ((3UL << 10 * 2) | (3UL << 11 * 2)); /* set CP10 and CP11 Full Access */
#endif

    /* enable IRC8M */
    RCU_CTL0 |= RCU_CTL0_IRC8MEN;
    while(0U == (RCU_CTL0 & RCU_CTL0_IRC8MSTB)) {
    }
    
    RCU_MODIFY(0x50);

    RCU_CFG0 &= ~(RCU_CFG0_SCS);
    RCU_CTL0 &= ~(RCU_CTL0_HXTALEN | RCU_CTL0_CKMEN | RCU_CTL0_PLLEN | RCU_CTL0_HXTALBPS);

    /* reset RCU */
    RCU_CFG0 &= ~(RCU_CFG0_SCS | RCU_CFG0_AHBPSC | RCU_CFG0_APB1PSC | RCU_CFG0_APB2PSC | \
                  RCU_CFG0_ADCPSC | RCU_CFG0_CKOUTSEL | RCU_CFG0_CKOUTDIV | RCU_CFG0_PLLDV);
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLDV);
#if (defined(GD32F350))
    RCU_CFG0 &= ~(RCU_CFG0_USBFSPSC);
    RCU_CFG2 &= ~(RCU_CFG2_CECSEL | RCU_CFG2_USBFSPSC2);
#endif /* GD32F350 */

    RCU_CFG1 &= ~(RCU_CFG1_PREDV | RCU_CFG1_PLLMF5 | RCU_CFG1_PLLPRESEL);
    RCU_CFG2 &= ~(RCU_CFG2_USART0SEL | RCU_CFG2_ADCSEL);
    RCU_CFG2 &= ~RCU_CFG2_IRC28MDIV;
    RCU_CFG2 &= ~RCU_CFG2_ADCPSC2;
    RCU_CTL1 &= ~RCU_CTL1_IRC28MEN;
    RCU_ADDCTL &= ~RCU_ADDCTL_IRC48MEN;
    RCU_INT = 0x00000000U;
    RCU_ADDINT = 0x00000000U;

    /* configure system clock */
    system_clock_config();

#ifdef VECT_TAB_SRAM
    nvic_vector_table_set(NVIC_VECTTAB_RAM, VECT_TAB_OFFSET);
#else
    nvic_vector_table_set(NVIC_VECTTAB_FLASH, VECT_TAB_OFFSET);
#endif
}

/*!
    \brief      configure the system clock
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_config(void)
{
#ifdef __SYSTEM_CLOCK_8M_HXTAL
    system_clock_8m_hxtal();
#elif defined (__SYSTEM_CLOCK_72M_PLL_HXTAL)
    system_clock_72m_hxtal();
#elif defined (__SYSTEM_CLOCK_72M_PLL_IRC8M_DIV2)
    system_clock_72m_irc8m();
#elif defined (__SYSTEM_CLOCK_72M_PLL_IRC48M_DIV2)
    system_clock_72m_irc48m();
#elif defined (__SYSTEM_CLOCK_84M_PLL_HXTAL)
    system_clock_84m_hxtal();
#elif defined (__SYSTEM_CLOCK_84M_PLL_IRC8M_DIV2)
    system_clock_84m_irc8m();
#elif defined (__SYSTEM_CLOCK_96M_PLL_HXTAL)
    system_clock_96m_hxtal();
#elif defined (__SYSTEM_CLOCK_96M_PLL_IRC8M_DIV2)
    system_clock_96m_irc8m();
#elif defined (__SYSTEM_CLOCK_96M_PLL_IRC48M_DIV2)
    system_clock_96m_irc48m();
#elif defined (__SYSTEM_CLOCK_108M_PLL_HXTAL)
    system_clock_108m_hxtal();
#elif defined (__SYSTEM_CLOCK_108M_PLL_IRC8M_DIV2)
    system_clock_108m_irc8m();
#else
    system_clock_8m_irc8m();
#endif /* __SYSTEM_CLOCK_8M_HXTAL */
}

#ifdef __SYSTEM_CLOCK_8M_HXTAL
/*!
    \brief      configure the system clock to 8M by HXTAL
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_8m_hxtal(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* enable HXTAL */
    RCU_CTL0 |= RCU_CTL0_HXTALEN;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do {
        timeout++;
        stab_flag = (RCU_CTL0 & RCU_CTL0_HXTALSTB);
    } while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));
    /* if fail */
    if(0U == (RCU_CTL0 & RCU_CTL0_HXTALSTB)) {
        return;
    }

    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV1;

    /* select HXTAL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_HXTAL;

    /* wait until HXTAL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_HXTAL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_72M_PLL_HXTAL)
/*!
    \brief      configure the system clock to 72M by PLL which selects HXTAL as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_72m_hxtal(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* enable HXTAL */
    RCU_CTL0 |= RCU_CTL0_HXTALEN;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do {
        timeout++;
        stab_flag = (RCU_CTL0 & RCU_CTL0_HXTALSTB);
    } while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));
    /* if fail */
    if(0U == (RCU_CTL0 & RCU_CTL0_HXTALSTB)) {
        return;
    }
    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    /* PLL = HXTAL * 9 = 72 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_HXTAL_IRC48M | (RCU_PLL_MUL9 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLLPRESEL_HXTAL);
    RCU_CFG1 |= (RCU_PLL_MUL9 & RCU_CFG1_PLLMF5);
    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}


#elif defined (__SYSTEM_CLOCK_72M_PLL_IRC8M_DIV2)
/*!
    \brief      configure the system clock to 72M by PLL which selects IRC8M/2 as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_72m_irc8m(void)
{
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;
    /* PLL = (IRC8M/2) * 18 = 72 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_IRC8M_DIV2 | (RCU_PLL_MUL18 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLL_MUL18 & RCU_CFG1_PLLMF5);
    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_72M_PLL_IRC48M_DIV2)
/*!
    \brief      configure the system clock to 72M by PLL which selects IRC48M/2 as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_72m_irc48m(void)
{
    /* enable IRC48M */
    RCU_ADDCTL |= RCU_ADDCTL_IRC48MEN;

    /* wait until IRC48M is stable*/
    while(0U == (RCU_ADDCTL & RCU_ADDCTL_IRC48MSTB)) {
    }
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;
    /* PLL = (IRC48M/2) * 3 = 96 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_HXTAL_IRC48M | (RCU_PLL_MUL3 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLLPRESEL_IRC48M | RCU_PLL_PREDV2);
    RCU_CFG1 |= (RCU_PLL_MUL3 & RCU_CFG1_PLLMF5);
    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)){
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_84M_PLL_HXTAL)
/*!
    \brief      configure the system clock to 84M by PLL which selects HXTAL as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_84m_hxtal(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;
    /* enable HXTAL */
    RCU_CTL0 |= RCU_CTL0_HXTALEN;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do {
        timeout++;
        stab_flag = (RCU_CTL0 & RCU_CTL0_HXTALSTB);
    } while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));
    /* if fail */
    if(0U == (RCU_CTL0 & RCU_CTL0_HXTALSTB)) {
        return;
    }
    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    /* PLL = HXTAL /2 * 21 = 84 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_HXTAL_IRC48M | (RCU_PLL_MUL21 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLLPRESEL_HXTAL | RCU_PLL_PREDV2);
    RCU_CFG1 |= (RCU_PLL_MUL21 & RCU_CFG1_PLLMF5);
    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_84M_PLL_IRC8M_DIV2)
/*!
    \brief      configure the system clock to 84M by PLL which selects IRC8M/2 as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_84m_irc8m(void)
{
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;
    /* PLL = (IRC8M/2) * 21 = 84 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_IRC8M_DIV2 | (RCU_PLL_MUL21 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLL_MUL21 & RCU_CFG1_PLLMF5);
    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_96M_PLL_HXTAL)
/*!
    \brief      configure the system clock to 96M by PLL which selects HXTAL as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_96m_hxtal(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;
    /* enable HXTAL */
    RCU_CTL0 |= RCU_CTL0_HXTALEN;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do {
        timeout++;
        stab_flag = (RCU_CTL0 & RCU_CTL0_HXTALSTB);
    } while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));
    /* if fail */
    if(0U == (RCU_CTL0 & RCU_CTL0_HXTALSTB)) {
        return;
    }
    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    /* PLL = HXTAL /2 * 24 = 96 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_HXTAL_IRC48M | (RCU_PLL_MUL24 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLLPRESEL_HXTAL | RCU_PLL_PREDV2);
    RCU_CFG1 |= (RCU_PLL_MUL24 & RCU_CFG1_PLLMF5);

    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_96M_PLL_IRC8M_DIV2)
/*!
    \brief      configure the system clock to 96M by PLL which selects IRC8M/2 as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_96m_irc8m(void)
{
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;
    /* PLL = (IRC8M/2) * 24 = 96 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_IRC8M_DIV2 | (RCU_PLL_MUL24 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLL_MUL24 & RCU_CFG1_PLLMF5);

    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_96M_PLL_IRC48M_DIV2)
/*!
    \brief      configure the system clock to 96M by PLL which selects IRC48M/2 as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_96m_irc48m(void)
{
    /* enable IRC48M */
    RCU_ADDCTL |= RCU_ADDCTL_IRC48MEN;

    /* wait until IRC48M is stable*/
    while(0U == (RCU_ADDCTL & RCU_ADDCTL_IRC48MSTB)) {
    }
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;
    /* PLL = (IRC48M/2) * 4 = 96 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_HXTAL_IRC48M | (RCU_PLL_MUL4 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLLPRESEL_IRC48M | RCU_PLL_PREDV2);
    RCU_CFG1 |= (RCU_PLL_MUL4 & RCU_CFG1_PLLMF5);

    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_108M_PLL_HXTAL)
/*!
    \brief      configure the system clock to 84M by PLL which selects HXTAL as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_108m_hxtal(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* enable HXTAL */
    RCU_CTL0 |= RCU_CTL0_HXTALEN;

    /* wait until HXTAL is stable or the startup time is longer than HXTAL_STARTUP_TIMEOUT */
    do {
        timeout++;
        stab_flag = (RCU_CTL0 & RCU_CTL0_HXTALSTB);
    } while((0U == stab_flag) && (HXTAL_STARTUP_TIMEOUT != timeout));
    /* if fail */
    if(0U == (RCU_CTL0 & RCU_CTL0_HXTALSTB)) {
        return;
    }
    /* HXTAL is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;

    /* PLL = HXTAL /2 * 27 = 108 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_HXTAL_IRC48M | (RCU_PLL_MUL27 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLLPRESEL_HXTAL | RCU_PLL_PREDV2);
    RCU_CFG1 |= (RCU_PLL_MUL27 & RCU_CFG1_PLLMF5);

    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#elif defined (__SYSTEM_CLOCK_108M_PLL_IRC8M_DIV2)
/*!
    \brief      configure the system clock to 108M by PLL which selects IRC8M/2 as its clock source
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_108m_irc8m(void)
{
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/2 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV2;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV2;
    /* PLL = (IRC8M/2) * 27 = 108 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLSEL | RCU_CFG0_PLLMF | RCU_CFG0_PLLMF4 | RCU_CFG0_PLLPREDV);
    RCU_CFG1 &= ~(RCU_CFG1_PLLPRESEL | RCU_CFG1_PLLMF5 | RCU_CFG1_PREDV);
    RCU_CFG0 |= (RCU_PLLSRC_IRC8M_DIV2 | (RCU_PLL_MUL27 & (~RCU_CFG1_PLLMF5)));
    RCU_CFG1 |= (RCU_PLL_MUL27 & RCU_CFG1_PLLMF5);

    /* enable PLL */
    RCU_CTL0 |= RCU_CTL0_PLLEN;

    /* wait until PLL is stable */
    while(0U == (RCU_CTL0 & RCU_CTL0_PLLSTB)) {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while(0U == (RCU_CFG0 & RCU_SCSS_PLL)) {
    }
}

#else
/*!
    \brief      configure the system clock to 8M by IRC8M
    \param[in]  none
    \param[out] none
    \retval     none
*/
static void system_clock_8m_irc8m(void)
{
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV1;
    /* APB1 = AHB */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV1;

    /* select IRC8M as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_IRC8M;

    /* wait until IRC8M is selected as system clock */
    while(0U != (RCU_CFG0 & RCU_SCSS_IRC8M)) {
    }
}
#endif /* __SYSTEM_CLOCK_8M_HXTAL */

/*!
    \brief      update the SystemCoreClock with current core clock retrieved from cpu registers
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SystemCoreClockUpdate(void)
{
    uint32_t sws = 0U;
    uint32_t pllmf = 0U, pllmf4 = 0U, pllmf5 = 0U, pllsel = 0U, pllpresel = 0U, prediv = 0U, idx = 0U, clk_exp = 0U;
    /* exponent of AHB clock divider */
    const uint8_t ahb_exp[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

    sws = GET_BITS(RCU_CFG0, 2, 3);
    switch(sws) {
    /* IRC8M is selected as CK_SYS */
    case SEL_IRC8M:
        SystemCoreClock = IRC8M_VALUE;
        break;
    /* HXTAL is selected as CK_SYS */
    case SEL_HXTAL:
        SystemCoreClock = HXTAL_VALUE;
        break;
    /* PLL is selected as CK_SYS */
    case SEL_PLL:
        /* get the value of PLLMF[3:0] */
        pllmf = GET_BITS(RCU_CFG0, 18, 21);
        pllmf4 = GET_BITS(RCU_CFG0, 27, 27);
        pllmf5 = GET_BITS(RCU_CFG1, 31, 31);
        /* high 16 bits */
        if((0U == pllmf4) && (0U == pllmf5)) {
            pllmf += 2U;
        }
        if((1U == pllmf4) && (0U == pllmf5)) {
            pllmf += 17U;
        }
        if((0U == pllmf4) && (1U == pllmf5)) {
            pllmf += 33U;
        }
        if((1U == pllmf4) && (1U == pllmf5)) {
            pllmf += 49U;
        }
        /* PLL clock source selection, HXTAL or IRC8M/2 */
        pllsel = GET_BITS(RCU_CFG0, 16, 16);
        if(0U != pllsel) {
            prediv = (GET_BITS(RCU_CFG1, 0, 3) + 1U);
            if(0U == pllpresel) {
                SystemCoreClock = (HXTAL_VALUE / prediv) * pllmf;
            } else {
                SystemCoreClock = (IRC48M_VALUE / prediv) * pllmf;
            }
        } else {
            SystemCoreClock = (IRC8M_VALUE >> 1) * pllmf;
        }
        break;
    /* IRC8M is selected as CK_SYS */
    default:
        SystemCoreClock = IRC8M_VALUE;
        break;
    }
    /* calculate AHB clock frequency */
    idx = GET_BITS(RCU_CFG0, 4, 7);
    clk_exp = ahb_exp[idx];
    SystemCoreClock >>= clk_exp;
}
