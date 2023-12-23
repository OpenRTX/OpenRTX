/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#ifndef CRM_H
#define CRM_H

#include <at32f421.h>

#ifdef __cplusplus
extern "C" {
#endif

void crm_adc_clock_div_set(crm_adc_div_type div_value)
{
	CRM->cfg_bit.adcdiv_l = div_value & 0x03;
	CRM->cfg_bit.adcdiv_h = (div_value >> 2) & 0x01;
}

crm_sclk_type crm_sysclk_switch_status_get(void)
{
	return (crm_sclk_type)CRM->cfg_bit.sclksts;
}

void crm_clocks_freq_get(crm_clocks_freq_type *clocks_struct)
{
	uint32_t pll_mult = 0, pll_mult_h = 0, pll_clock_source = 0, temp = 0, div_value = 0;
	uint32_t pllrcsfreq = 0, pll_ms = 0, pll_ns = 0, pll_fr = 0;
	crm_sclk_type sclk_source;

	static const uint8_t sclk_ahb_div_table[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
	static const uint8_t ahb_apb1_div_table[8] = {0, 0, 0, 0, 1, 2, 3, 4};
	static const uint8_t ahb_apb2_div_table[8] = {0, 0, 0, 0, 1, 2, 3, 4};
	static const uint8_t adc_div_table[8] = {2, 4, 6, 8, 2, 12, 8, 16};

	/* get sclk source */
	sclk_source = crm_sysclk_switch_status_get();

	switch (sclk_source) {
	case CRM_SCLK_HICK:
		if(((CRM->misc2_bit.hick_to_sclk) != RESET) && ((CRM->misc1_bit.hickdiv) != RESET))
			clocks_struct->sclk_freq = HICK_VALUE * 6;
		else
			clocks_struct->sclk_freq = HICK_VALUE;
		break;
	case CRM_SCLK_HEXT:
		clocks_struct->sclk_freq = HEXT_VALUE;
		break;
	case CRM_SCLK_PLL:
		pll_clock_source = CRM->cfg_bit.pllrcs;
		if(CRM->pll_bit.pllcfgen == FALSE)
		{
			/* get multiplication factor */
			pll_mult = CRM->cfg_bit.pllmult_l;
			pll_mult_h = CRM->cfg_bit.pllmult_h;

			/* process high bits */
			if((pll_mult_h != 0U) || (pll_mult == 15U))
			{
				pll_mult += ((16U * pll_mult_h) + 1U);
			}
			else
			{
				pll_mult += 2U;
			}

			if (pll_clock_source == 0x00)
			{
				/* hick divided by 2 selected as pll clock entry */
				clocks_struct->sclk_freq = (HICK_VALUE >> 1) * pll_mult;
			}
			else
			{
				/* hext selected as pll clock entry */
				if (CRM->cfg_bit.pllhextdiv != RESET)
				{
					/* hext clock divided by 2 */
					clocks_struct->sclk_freq = (HEXT_VALUE / 2) * pll_mult;
				}
				else
				{
					clocks_struct->sclk_freq = HEXT_VALUE * pll_mult;
				}
			}
		}
		else
		{
			pll_ms = CRM->pll_bit.pllms;
			pll_ns = CRM->pll_bit.pllns;
			pll_fr = CRM->pll_bit.pllfr;

			if (pll_clock_source == 0x00)
			{
				/* hick divided by 2 selected as pll clock entry */
				pllrcsfreq = (HICK_VALUE >> 1);
			}
			else
			{
				/* hext selected as pll clock entry */
				if (CRM->cfg_bit.pllhextdiv != RESET)
				{
					/* hext clock divided by 2 */
					pllrcsfreq = (HEXT_VALUE / 2);
				}
				else
				{
					pllrcsfreq = HEXT_VALUE;
				}
			}
			clocks_struct->sclk_freq = (uint32_t)(((uint64_t)pllrcsfreq * pll_ns) / (pll_ms * (0x1 << pll_fr)));
		}
		break;
	default:
		clocks_struct->sclk_freq = HICK_VALUE;
		break;
	}

	/* compute sclk, ahbclk, abp1clk apb2clk and adcclk frequencies */
	/* get ahb division */
	temp = CRM->cfg_bit.ahbdiv;
	div_value = sclk_ahb_div_table[temp];
	/* ahbclk frequency */
	clocks_struct->ahb_freq = clocks_struct->sclk_freq >> div_value;

	/* get apb1 division */
	temp = CRM->cfg_bit.apb1div;
	div_value = ahb_apb1_div_table[temp];
	/* apb1clk frequency */
	clocks_struct->apb1_freq = clocks_struct->ahb_freq >> div_value;

	/* get apb2 division */
	temp = CRM->cfg_bit.apb2div;
	div_value = ahb_apb2_div_table[temp];
	/* apb2clk frequency */
	clocks_struct->apb2_freq = clocks_struct->ahb_freq >> div_value;

	/* get adc division */
	temp = CRM->cfg_bit.adcdiv_h;
	temp = ((temp << 2) | (CRM->cfg_bit.adcdiv_l));
	div_value = adc_div_table[temp];
	/* adcclk clock frequency */
	clocks_struct->adc_freq = clocks_struct->apb2_freq / div_value;
}

void crm_periph_clock_enable(crm_periph_clock_type value, confirm_state new_state)
{
	if (new_state) {
		CRM_REG(value) |= CRM_REG_BIT(value);
	} else {
		CRM_REG(value) &= ~(CRM_REG_BIT(value));
	}
}

void crm_periph_reset(crm_periph_reset_type value, confirm_state new_state)
{
	if (new_state) {
		CRM_REG(value) |= (CRM_REG_BIT(value));
	} else {
		CRM_REG(value) &= ~(CRM_REG_BIT(value));
	}
}

#ifdef __cplusplus
}
#endif

#endif /* CRM_H */
