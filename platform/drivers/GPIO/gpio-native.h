/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0) \
 || defined(PLATFORM_MD9600) || defined(PLATFORM_MOD17) \
 || defined(PLATFORM_DM1701)
#include <gpio_stm32.h>
#elif defined(PLATFORM_GD77) || defined(PLATFORM_DM1801)
#include <gpio_mk22.h>
#endif

#endif /* GPIO_NATIVE_H */
