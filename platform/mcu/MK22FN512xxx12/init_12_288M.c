/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO,                     *
 *                                Federico Terraneo,                       *
 *                                Morgan Diepart ON4MOD                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/*
 * This file is mainly used to define the SystemInitHook function.
 * This function is needed for targets that use a 12.288 MHz external crystal.
 * The SystemInitHook function is called at the end of the SystemInit function 
 * in NXP's CMSIS pack files. In this case, it will initialize the clock tree to
 * full speed, that is, a system clock of 119.808 MHz.
 * 
 * The SystemInitHook also sets the vector table offset.
 */

#include "arch/cortexM4_nxpmk22/nxpmk22f51212_generic/interfaces-impl/arch_registers_impl.h"
#include "CMSIS/Device/NXP/MK22FN512xxx12/Include/system_MK22F51212.h"

extern void (* const __Vectors[])();

void SystemInitHook(void)
{
    // Initialise clock tree for full speed run, since SystemInit by NXP does not.
    //
    // Clock tree configuration:
    // - External clock input: 12.288MHz from HR_C6000 resonator
    // - PLL input divider: 4 -> PLL reference clock is 3.072MHz
    // - PLL multiplier: 39 -> PLL output clock is 119.808MHz
    //
    // - Core and system clock @ 119.808MHz
    // - Bus clock @ 59.904MHz
    // - FlexBus clock @ 29.952MHz
    // - Flash clock @ 23.962MHz

    SIM->CLKDIV1 = SIM_CLKDIV1_OUTDIV2(1)   // Bus clock divider = 2
                 | SIM_CLKDIV1_OUTDIV3(3)   // FlexBus clock divider = 4
                 | SIM_CLKDIV1_OUTDIV4(4);  // Flash clock divider = 5

    MCG->C2 |= MCG_C2_RANGE(2);             // Very high frequency range
    OSC->CR |= OSC_CR_ERCLKEN(1);           // Enable external reference clock

    MCG->C6 |= MCG_C6_VDIV0(15);            // PLL multiplier set to 39
    MCG->C5 |= MCG_C5_PRDIV0(3)             // Divide PLL ref clk by 4
            |  MCG_C5_PLLCLKEN0(1);         // Enable PLL clock

    SMC->PMPROT = SMC_PMPROT_AHSRUN(1);     // Allow HSRUN mode
    SMC->PMCTRL = SMC_PMCTRL_RUNM(3);       // Switch to HSRUN mode
    while((SMC->PMSTAT & 0x80) == 0) ;      // Wait till switch to HSRUN is effective

    MCG->C6 |= MCG_C6_PLLS(1);              // Connect PLL to MCG source

    SCB->VTOR = (unsigned int)(&__Vectors); // Relocates ISR vector   
}
