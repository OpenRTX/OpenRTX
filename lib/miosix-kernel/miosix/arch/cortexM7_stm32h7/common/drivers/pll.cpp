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

#include "interfaces/arch_registers.h"
#include "pll.h"

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
                    | RCC_PLLCKSELR_DIVM1_2     //M=5 (25MHz/5)5MHz
                    | RCC_PLLCKSELR_PLLSRC_HSE; //HSE selected as PLL source
    RCC->PLL1DIVR = (2-1)<<24 // R=2
                  | (8-1)<<16 // Q=8
                  | (2-1)<<9  // P=2
                  | (160-1);  // N=160
    RCC->PLLCFGR |= RCC_PLLCFGR_PLL1RGE_2 // Pll ref clock between 4 and 8MHz
                  | RCC_PLLCFGR_DIVP1EN   // Enable output P
                  | RCC_PLLCFGR_DIVQ1EN   // Enable output Q
                  | RCC_PLLCFGR_DIVR1EN;  // Enable output R
    RCC->CR |= RCC_CR_PLL1ON;
    while((RCC->CR & RCC_CR_PLL1RDY)==0) ; //Wait
    
    //Before increasing the fequency set dividers 
    RCC->D1CFGR = RCC_D1CFGR_D1CPRE_DIV1  //CPU clock /1
                | RCC_D1CFGR_D1PPRE_DIV2  //D1 APB3   /2
                | RCC_D1CFGR_HPRE_DIV2;   //D1 AHB    /2
    RCC->D2CFGR = RCC_D2CFGR_D2PPRE2_DIV2 //D2 APB2   /2
                | RCC_D2CFGR_D2PPRE1_DIV2;//D2 APB1   /2
    RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV2; //D3 APB4   /2
    
    //And increase FLASH wait states
    FLASH->ACR = FLASH_ACR_WRHIGHFREQ_2   //Settings for FLASH freq=200MHz
               | FLASH_ACR_LATENCY_3WS;
    
    //Finally, increase frequency
    RCC->CFGR |= RCC_CFGR_SW_PLL1;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1) ; //Wait
}
