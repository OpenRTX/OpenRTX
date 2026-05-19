/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#ifndef ARCH_REGISTERS_IMPL_H
#define	ARCH_REGISTERS_IMPL_H

//Always include stm32f4xx.h before core_cm4.h, there's some nasty dependency
#include "stm32f4xx.h"
#include "core_cm4.h"
#include "system_stm32f4xx.h"

#define RCC_SYNC() //Workaround for a bug in stm32f42x

#endif	//ARCH_REGISTERS_IMPL_H
