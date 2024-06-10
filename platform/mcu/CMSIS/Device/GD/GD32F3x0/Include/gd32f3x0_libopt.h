/*!
    \file    gd32f3x0_libopt.h
    \brief   library optional for gd32f3x0

    \version 2023-12-31, V2.3.0, firmware for GD32F3x0
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

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

#ifndef GD32F3X0_LIBOPT_H
#define GD32F3X0_LIBOPT_H

#include "gd32f3x0_adc.h"
#include "gd32f3x0_crc.h"
#include "gd32f3x0_ctc.h"
#include "gd32f3x0_dbg.h"
#include "gd32f3x0_dma.h"
#include "gd32f3x0_exti.h"
#include "gd32f3x0_fmc.h"
#include "gd32f3x0_gpio.h"
#include "gd32f3x0_syscfg.h"
#include "gd32f3x0_i2c.h"
#include "gd32f3x0_fwdgt.h"
#include "gd32f3x0_pmu.h"
#include "gd32f3x0_rcu.h"
#include "gd32f3x0_rtc.h"
#include "gd32f3x0_spi.h"
#include "gd32f3x0_timer.h"
#include "gd32f3x0_usart.h"
#include "gd32f3x0_wwdgt.h"
#include "gd32f3x0_misc.h"
#include "gd32f3x0_tsi.h"

#ifdef GD32F350
#include "gd32f3x0_cec.h"
#include "gd32f3x0_cmp.h"
#include "gd32f3x0_dac.h"
#endif /* GD32F350 */

#endif /* GD32F3X0_LIBOPT_H */
