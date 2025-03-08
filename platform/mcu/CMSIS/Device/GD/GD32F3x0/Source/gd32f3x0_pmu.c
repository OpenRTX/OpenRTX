/*!
    \file    gd32f3x0_pmu.c
    \brief   PMU driver

    \version 2023-12-31, V2.3.0, firmware for GD32F3x0
*/

/*
    Copyright (c) 2021, GigaDevice Semiconductor Inc.

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

#include "gd32f3x0_pmu.h"


/*!
    \brief      reset PMU register
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_deinit(void)
{
    /* reset PMU */
    rcu_periph_reset_enable(RCU_PMURST);
    rcu_periph_reset_disable(RCU_PMURST);
}

/*!
    \brief      select low voltage detector threshold
    \param[in]  lvdt_n:
                only one parameter can be selected which is shown as below:
      \arg        PMU_LVDT_0: voltage threshold is 2.1V
      \arg        PMU_LVDT_1: voltage threshold is 2.3V
      \arg        PMU_LVDT_2: voltage threshold is 2.4V
      \arg        PMU_LVDT_3: voltage threshold is 2.6V
      \arg        PMU_LVDT_4: voltage threshold is 2.7V
      \arg        PMU_LVDT_5: voltage threshold is 2.9V
      \arg        PMU_LVDT_6: voltage threshold is 3.0V
      \arg        PMU_LVDT_7: voltage threshold is 3.1V
    \param[out] none
    \retval     none
*/
void pmu_lvd_select(uint32_t lvdt_n)
{
    /* disable LVD */
    PMU_CTL &= ~PMU_CTL_LVDEN;
    /* clear LVDT bits */
    PMU_CTL &= ~PMU_CTL_LVDT;
    /* set LVDT bits according to lvdt_n */
    PMU_CTL |= lvdt_n;
    /* enable LVD */
    PMU_CTL |= PMU_CTL_LVDEN;
}

/*!
    \brief      select LDO output voltage
                these bits set by software when the main PLL closed
    \param[in]  ldo_output:
                only one parameter can be selected which is shown as below:
      \arg        PMU_LDOVS_LOW: LDO output voltage low mode
      \arg        PMU_LDOVS_MID: LDO output voltage mid mode
      \arg        PMU_LDOVS_HIGH: LDO output voltage high mode
    \param[out] none
    \retval     none
*/
void pmu_ldo_output_select(uint32_t ldo_output)
{
    PMU_CTL &= ~PMU_CTL_LDOVS;
    PMU_CTL |= ldo_output;
}

/*!
    \brief      disable PMU lvd
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_lvd_disable(void)
{
    /* disable LVD */
    PMU_CTL &= ~PMU_CTL_LVDEN;
}

/*!
    \brief      enable low-driver mode in deep-sleep mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_lowdriver_mode_enable(void)
{
    PMU_CTL &= ~PMU_CTL_LDEN;
    PMU_CTL |= PMU_LOWDRIVER_ENABLE;
}

/*!
    \brief      disable low-driver mode in deep-sleep mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_lowdriver_mode_disable(void)
{
    PMU_CTL &= ~PMU_CTL_LDEN;
    PMU_CTL |= PMU_LOWDRIVER_DISABLE;
}

/*!
    \brief      enable high-driver mode
                this bit set by software only when IRC8M or HXTAL used as system clock
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_highdriver_mode_enable(void)
{
    PMU_CTL |= PMU_CTL_HDEN;
}

/*!
    \brief      disable high-driver mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_highdriver_mode_disable(void)
{
    PMU_CTL &= ~PMU_CTL_HDEN;
}

/*!
    \brief      switch high-driver mode
                this bit set by software only when IRC8M or HXTAL used as system clock
    \param[in]  highdr_switch:
                only one parameter can be selected which is shown as below:
      \arg        PMU_HIGHDR_SWITCH_NONE: disable high-driver mode switch
      \arg        PMU_HIGHDR_SWITCH_EN: enable high-driver mode switch
    \param[out] none
    \retval     none
*/
void pmu_highdriver_switch_select(uint32_t highdr_switch)
{
    /* wait for HDRF flag to be set */
    while(SET != pmu_flag_get(PMU_FLAG_HDR)) {
    }
    PMU_CTL &= ~PMU_CTL_HDS;
    PMU_CTL |= highdr_switch;
}

/*!
    \brief      low-driver mode when use low power LDO
    \param[in]  mode:
                only one parameter can be selected which is shown as below:
      \arg        PMU_NORMALDR_LOWPWR: normal-driver when use low power LDO
      \arg        PMU_LOWDR_LOWPWR: low-driver mode enabled when LDEN is 11 and use low power LDO
    \param[out] none
    \retval     none
*/
void pmu_lowpower_driver_config(uint32_t mode)
{
    PMU_CTL &= ~PMU_CTL_LDLP;
    PMU_CTL |= mode;
}

/*!
    \brief      low-driver mode when use normal power LDO
    \param[in]  mode:
                only one parameter can be selected which is shown as below:
      \arg        PMU_NORMALDR_NORMALPWR: normal-driver when use low power LDO
      \arg        PMU_LOWDR_NORMALPWR: low-driver mode enabled when LDEN is 11 and use low power LDO
    \param[out] none
    \retval     none
*/
void pmu_normalpower_driver_config(uint32_t mode)
{
    PMU_CTL &= ~PMU_CTL_LDNP;
    PMU_CTL |= mode;
}

/*!
    \brief      PMU work at sleep mode
    \param[in]  sleepmodecmd:
                only one parameter can be selected which is shown as below:
      \arg        WFI_CMD: use WFI command
      \arg        WFE_CMD: use WFE command
    \param[out] none
    \retval     none
*/
void pmu_to_sleepmode(uint8_t sleepmodecmd)
{
    /* clear sleepdeep bit of Cortex-M4 system control register */
    SCB->SCR &= ~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);

    /* select WFI or WFE command to enter sleep mode */
    // if(WFI_CMD == sleepmodecmd) {
    //     __WFI();
    // } else {
    //     __WFE();
    // }
}

/*!
    \brief      PMU work at deepsleep mode
    \param[in]  ldo:
                only one parameter can be selected which is shown as below:
      \arg        PMU_LDO_NORMAL: LDO operates normally when pmu enter deepsleep mode
      \arg        PMU_LDO_LOWPOWER: LDO work at low power mode when pmu enter deepsleep mode
    \param[in]  lowdrive:
                only one parameter can be selected which is shown as below:
      \arg        PMU_LOWDRIVER_ENABLE: low-driver mode enable in deep-sleep mode
      \arg        PMU_LOWDRIVER_DISABLE: low-driver mode disable in deep-sleep mode
    \param[in]  deepsleepmodecmd:
                only one parameter can be selected which is shown as below:
      \arg        WFI_CMD: use WFI command
      \arg        WFE_CMD: use WFE command
    \param[out] none
    \retval     none
*/
void pmu_to_deepsleepmode(uint32_t ldo, uint32_t lowdrive, uint8_t deepsleepmodecmd)
{
    static uint32_t reg_snap[ 4 ];
    /* clear stbmod and ldolp bits */
    PMU_CTL &= ~((uint32_t)(PMU_CTL_STBMOD | PMU_CTL_LDOLP | PMU_CTL_LDEN | PMU_CTL_LDNP | PMU_CTL_LDLP));

    /* set ldolp bit according to pmu_ldo */
    PMU_CTL |= ldo;

    /* low drive mode config in deep-sleep mode */
    if(PMU_LOWDRIVER_ENABLE == lowdrive) {
        if(PMU_LDO_NORMAL == ldo) {
            PMU_CTL |= (uint32_t)(PMU_CTL_LDEN | PMU_CTL_LDNP);
        } else {
            PMU_CTL |= (uint32_t)(PMU_CTL_LDEN | PMU_CTL_LDLP);
        }
    }

    /* set sleepdeep bit of Cortex-M4 system control register */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    reg_snap[0] = REG32(0xE000E010U);
    reg_snap[1] = REG32(0xE000E100U);
    reg_snap[2] = REG32(0xE000E104U);
    reg_snap[3] = REG32(0xE000E108U);

    REG32(0xE000E010U) &= 0x00010004U;
    REG32(0xE000E180U)  = 0XB7FFEF19U;
    REG32(0xE000E184U)  = 0XFFFFFBFFU;
    REG32(0xE000E188U)  = 0xFFFFFFFFU;

    /* select WFI or WFE command to enter deepsleep mode */
    if(WFI_CMD == deepsleepmodecmd) {
        __WFI();
    } else {
        __SEV();
        __WFE();
        __WFE();
    }

    REG32(0xE000E010U) = reg_snap[0];
    REG32(0xE000E100U) = reg_snap[1];
    REG32(0xE000E104U) = reg_snap[2];
    REG32(0xE000E108U) = reg_snap[3];

    /* reset sleepdeep bit of Cortex-M4 system control register */
    SCB->SCR &= ~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);
}

/*!
    \brief      pmu work at standby mode
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_to_standbymode(void)
{
    /* set stbmod bit */
    PMU_CTL |= PMU_CTL_STBMOD;

    /* reset wakeup flag */
    PMU_CTL |= PMU_CTL_WURST;

    /* set sleepdeep bit of Cortex-M4 system control register */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    REG32( 0xE000E010U ) &= 0x00010004U;
    REG32( 0xE000E180U ) = 0XFFFFFFFBU;
    REG32( 0xE000E184U ) = 0XFFFFFFFFU;
    REG32( 0xE000E188U ) = 0xFFFFFFFFU;

    /* select WFI command to enter standby mode */
    __WFI();
}

/*!
    \brief      enable wakeup pin
    \param[in]  wakeup_pin:
                one or more parameters can be selected which are shown as below:
      \arg        PMU_WAKEUP_PIN0: WKUP Pin 0 (PA0)
      \arg        PMU_WAKEUP_PIN1: WKUP Pin 1 (PC13)
      \arg        PMU_WAKEUP_PIN4: WKUP Pin 4 (PC5)
      \arg        PMU_WAKEUP_PIN5: WKUP Pin 5 (PB5)
      \arg        PMU_WAKEUP_PIN6: WKUP Pin 6 (PB15)
    \param[out] none
    \retval     none
*/
void pmu_wakeup_pin_enable(uint32_t wakeup_pin)
{
    PMU_CS |= wakeup_pin;
}

/*!
    \brief      disable wakeup pin
    \param[in]  wakeup_pin:
                one or more parameters can be selected which are shown as below:
      \arg        PMU_WAKEUP_PIN0: WKUP Pin 0 (PA0)
      \arg        PMU_WAKEUP_PIN1: WKUP Pin 1 (PC13)
      \arg        PMU_WAKEUP_PIN4: WKUP Pin 4 (PC5)
      \arg        PMU_WAKEUP_PIN5: WKUP Pin 5 (PB5)
      \arg        PMU_WAKEUP_PIN6: WKUP Pin 6 (PB15)
    \param[out] none
    \retval     none
*/
void pmu_wakeup_pin_disable(uint32_t wakeup_pin)
{
    PMU_CS &= ~(wakeup_pin);
}

/*!
    \brief      enable backup domain write
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_backup_write_enable(void)
{
    PMU_CTL |= PMU_CTL_BKPWEN;
}

/*!
    \brief      disable backup domain write
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pmu_backup_write_disable(void)
{
    PMU_CTL &= ~PMU_CTL_BKPWEN;
}

/*!
    \brief      get flag state
    \param[in]  flag:
                only one parameter can be selected which is shown as below:
      \arg        PMU_FLAG_WAKEUP: wakeup flag
      \arg        PMU_FLAG_STANDBY: standby flag
      \arg        PMU_FLAG_LVD: lvd flag
      \arg        PMU_FLAG_LDOVSR: LDO voltage select ready flag
      \arg        PMU_FLAG_HDR: high-driver ready flag
      \arg        PMU_FLAG_HDSR: high-driver switch ready flag
      \arg        PMU_FLAG_LDR: low-driver mode ready flag
    \param[out] none
    \retval     FlagStatus SET or RESET
*/
FlagStatus pmu_flag_get(uint32_t flag)
{
    FlagStatus ret_status = RESET;

    if(PMU_CS & flag) {
        ret_status = SET;
    }

    return ret_status;
}

/*!
    \brief      clear flag bit
    \param[in]  flag:
                one or more parameters can be selected which are shown as below:
      \arg        PMU_FLAG_RESET_WAKEUP: reset wakeup flag
      \arg        PMU_FLAG_RESET_STANDBY: reset standby flag
    \param[out] none
    \retval     none
*/
void pmu_flag_clear(uint32_t flag)
{
    if(RESET != (flag & PMU_FLAG_RESET_WAKEUP)) {
        /* reset wakeup flag */
        PMU_CTL |= PMU_CTL_WURST;
    }
    if(RESET != (flag & PMU_FLAG_RESET_STANDBY)) {
        /* reset standby flag */
        PMU_CTL |= PMU_CTL_STBRST;
    }
}
