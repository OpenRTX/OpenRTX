/***************************************************************************
 *   Copyright (C) 2020 by Terraneo Federico                               *
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
 *   by the GNU General Public License. However the suorce code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/ 

#pragma once

//The Atmel provided init code is empty anyway
#define DONT_USE_CMSIS_INIT

//Miosix replacement that provides SystemCoreClock
#include "drivers/clock.h"

//This file in turn includes core_cm4.h
#include "CMSIS/Device/Atmel/sam4l/include/sam4lc2a.h"

//This register is mentioned in the document but a definition is not provided
#define PDBG (*((volatile unsigned int*)0xe0042000))
#define PDBG_PEVC (1<<2)
#define PDBG_AST  (1<<1)
#define PDBG_WDT  (1<<0)
