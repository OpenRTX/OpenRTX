/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Silvano Seva IU2KWO                      *
 *                            and Niccolò Izzo IU2KIN                      *
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

#include "ADC0_A36plus.h"

#include <hwconfig.h>
#include "printf.h"

void adc0_init()
{
    rcu_periph_clock_enable(RCU_ADC);
    adc_deinit();
    rcu_adc_clock_config(RCU_ADCCK_APB2_DIV8);
    adc_resolution_config(0x2000000);   
    adc_data_alignment_config(0);
    adc_channel_length_config(1, 1);
    adc_regular_channel_config(0, 0, 5);
    adc_external_trigger_source_config(1, 0xe0000);
    adc_external_trigger_config(1, 1);
    adc_enable();
    adc_calibration_enable();
    return;
}

void adc0_terminate()
{
    return;
}

uint16_t adc0_getRawSample(uint8_t ch)
{
    adc_regular_channel_config(0, 1, 5);
    adc_software_trigger_enable(1);
    char bVar1;
    do
    {
        bVar1 = adc_flag_get(ADC_FLAG_EOC);
    } while (!bVar1);
    adc_interrupt_flag_clear(ADC_FLAG_EOC);
    return adc_regular_data_read();
}

uint32_t adc0_getMeasurement(uint8_t ch)
{
    uint32_t sample = (uint32_t)(adc0_getRawSample(ch) * 0x1d5);
    return sample/10;
}