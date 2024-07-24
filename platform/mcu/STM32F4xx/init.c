/***************************************************************************
 *   Copyright (C) 2024 by Morgan Diepart ON4MOD                           *
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
 * This function is called at the end of the SystemInit function.
 * 
 * The SystemInitHook sets the vector table offset.
 */

#include "arch/cortexM4_stm32f4/stm32f405_generic/interfaces-impl/arch_registers_impl.h"
#include "CMSIS/Device/ST/STM32F4xx/Include/system_stm32f4xx.h"

extern void (* const __Vectors[])();

void SystemInitHook(void)
{
    SCB->VTOR = (unsigned int)(&__Vectors); // Relocates ISR vector   
}
