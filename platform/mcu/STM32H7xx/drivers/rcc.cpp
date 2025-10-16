/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stm32h7xx.h"
#include "rcc.h"

/**
 * Clock tree configuration:
 *
 * - input clock from 25MHz external crystal
 * - VCO clock 800MHz
 * - PLL_P clock 400MHz
 * - PLL_Q clock 100MHz
 * - PLL_R clock 400MHz
 *
 * Clocks derived from PLL_P output:
 * - CPU clock 400MHz
 * - AHB1/2/3 clock 200MHz
 * - APB1/2/4 clock 200MHz
 */

void startPll()
{
    //In the STM32H7 DVFS was introduced (chapter 6, Power control)
    //The default voltage is the lowest, switch to highest to run @ 400MHz
    //QUIRK: it looks like VOS can only be set by first setting SCUEN to 0.
    //This isn't documented anywhere in the reference manual
    PWR->CR3 &= ~PWR_CR3_SCUEN;
    PWR->D3CR = PWR_D3CR_VOS_1 | PWR_D3CR_VOS_0;
    while((PWR->D3CR & PWR_D3CR_VOSRDY)==0) ; //Wait

    //Enable HSE oscillator
    RCC->CR |= RCC_CR_HSEON;
    while((RCC->CR & RCC_CR_HSERDY)==0) ; //Wait

    //Start the PLL
    RCC->PLLCKSELR |= RCC_PLLCKSELR_DIVM1_0
                   |  RCC_PLLCKSELR_DIVM1_2     //M=5 (25MHz/5)5MHz
                   |  RCC_PLLCKSELR_PLLSRC_HSE; //HSE selected as PLL source
    RCC->PLL1DIVR = (2-1) << 24 // R=2
                  | (8-1) << 16 // Q=8
                  | (2-1) << 9  // P=2
                  | (160-1);    // N=160
    RCC->PLLCFGR |= RCC_PLLCFGR_PLL1RGE_2 // Pll ref clock between 4 and 8MHz
                 |  RCC_PLLCFGR_DIVP1EN   // Enable output P
                 |  RCC_PLLCFGR_DIVQ1EN   // Enable output Q
                 |  RCC_PLLCFGR_DIVR1EN;  // Enable output R
    RCC->CR |= RCC_CR_PLL1ON;
    while((RCC->CR & RCC_CR_PLL1RDY)==0) ; //Wait

    //Before increasing the fequency set dividers
    RCC->D1CFGR = RCC_D1CFGR_D1CPRE_DIV1  //CPU clock /1
                | RCC_D1CFGR_D1PPRE_DIV1  //D1 APB3   /1
                | RCC_D1CFGR_HPRE_DIV2;   //D1 AHB    /2
    RCC->D2CFGR = RCC_D2CFGR_D2PPRE2_DIV1 //D2 APB2   /1
                | RCC_D2CFGR_D2PPRE1_DIV1;//D2 APB1   /1
    RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV1; //D3 APB4   /1

    //And increase FLASH wait states
    FLASH->ACR = FLASH_ACR_WRHIGHFREQ_1   //Settings for FLASH freq=200MHz
               | FLASH_ACR_LATENCY_3WS;

    //Finally, increase frequency
    RCC->CFGR |= RCC_CFGR_SW_PLL1;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1) ; //Wait

    //Configure PLL2 to have 168MHz on P output
    RCC->PLLCKSELR |= 25 << RCC_PLLCKSELR_DIVM2_Pos;    // M=25, VCO input 1MHz
    RCC->PLL2DIVR   = (168-1);                          // N=168, P,Q,R = 1
    RCC->PLLCFGR   |= RCC_PLLCFGR_PLL2VCOSEL            // Medium VCO range
                   |  RCC_PLLCFGR_DIVP2EN;
    RCC->CR |= RCC_CR_PLL2ON;                           // Start PLL2
    while((RCC->CR & RCC_CR_PLL2RDY)==0) ;              // Wait until ready

    // Configure USART6 kernel clock source to use PCLK2
    // USART16SEL field is bits [5:3]: 000 = pclk2
    RCC->D2CCIP2R &= ~RCC_D2CCIP2R_USART16SEL;
}

uint32_t getBusClock(const uint8_t bus)
{
    switch(bus) {
        case PERIPH_BUS_AHB:
            return 200000000;  // AHB: CPU(400MHz) / HPRE_DIV2 = 200MHz
            break;

        case PERIPH_BUS_APB1:
            return 200000000;  // APB1: AHB(200MHz) / D2PPRE1_DIV1 = 200MHz
            break;

        case PERIPH_BUS_APB2:
            return 200000000;  // APB2: AHB(200MHz) / D2PPRE2_DIV1 = 200MHz
            break;

        case PERIPH_BUS_APB3:
            return 200000000;  // APB3: AHB(200MHz) / D1PPRE_DIV1 = 200MHz
            break;

        case PERIPH_BUS_APB4:
            return 200000000;  // APB4: AHB(200MHz) / D3PPRE_DIV1 = 200MHz
            break;
    }

    return 0;
}
