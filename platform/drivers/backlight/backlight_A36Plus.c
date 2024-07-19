/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <interfaces/platform.h>
#include "gd32f3x0.h"
// #include <peripherals/gpio.h>
#include "../../mcu/GD32F330/drivers/gpio.h"
#include <hwconfig.h> 
#include "backlight.h"


void TIMER16_IRQHandler(void){
    if (timer_interrupt_flag_get(TIMER16, TIMER_INT_UP) != RESET){
        // gpio_bit_toggle(LCD_GPIO_PORT, LCD_GPIO_LIGHT_PIN);
        timer_flag_clear(TIMER16, TIMER_INT_UP);
    }
}

void backlight_init()
{
    timer_parameter_struct timer_initpara;
    timer_oc_parameter_struct time_ocpar;
    rcu_periph_clock_enable(RCU_TIMER16);
    rcu_periph_clock_enable(LCD_GPIO_RCU);

    gpio_af_set(LCD_GPIO_PORT, GPIO_AF_2, LCD_GPIO_LIGHT_PIN);
    gpio_mode_set(LCD_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, LCD_GPIO_LIGHT_PIN);
    gpio_output_options_set(LCD_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_GPIO_LIGHT_PIN);
    
    timer_deinit(TIMER16);
    /* TIMER16 configuration */
    timer_initpara.prescaler = (84 - 1);
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = (255 - 1);
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 1;
    timer_init(TIMER16, &timer_initpara);
    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER16);

    /* CH0 configuration in PWM mode */
    time_ocpar.outputstate  = TIMER_CCX_ENABLE;
    time_ocpar.outputnstate = TIMER_CCXN_ENABLE;
    time_ocpar.ocpolarity   = TIMER_OC_POLARITY_LOW;
    time_ocpar.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;
    time_ocpar.ocidlestate  = TIMER_OC_IDLE_STATE_HIGH;
    time_ocpar.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    

    timer_channel_output_config(TIMER16, TIMER_CH_0, &time_ocpar);
    timer_primary_output_config(TIMER16, ENABLE);

    // timer_channel_output_pulse_value_config(TIMER16, TIMER_CH_0, 0);
    timer_channel_output_mode_config(TIMER16, TIMER_CH_0, TIMER_OC_MODE_PWM1);
    timer_channel_output_shadow_config(TIMER16, TIMER_CH_0, TIMER_OC_SHADOW_DISABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER16);
    
    timer_enable(TIMER16);
    // timer_interrupt_enable(TIMER16, TIMER_INT_UP);
}

void backlight_terminate()
{
   rcu_periph_clock_disable(RCU_TIMER16);
}

