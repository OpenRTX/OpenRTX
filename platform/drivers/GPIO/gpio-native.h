/***************************************************************************
 *   Copyright (C) 2024 - 2025 by Silvano Seva IU2KWO                      *
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

#ifndef GPIO_NATIVE_H
#define GPIO_NATIVE_H

#if defined(STM32F405xx)
#include "stm32f4xx.h"
#include "drivers/GPIO/gpio_stm32.h"
#elif defined(STM32H743xx)
#include "stm32h7xx.h"
#include "drivers/GPIO/gpio_stm32.h"
#elif defined(MK22FN512xx)
#include "drivers/GPIO/gpio_mk22.h"
#endif

#endif /* GPIO_NATIVE_H */
