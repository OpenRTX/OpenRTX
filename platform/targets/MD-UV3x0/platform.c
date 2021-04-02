/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Silvano Seva IU2KWO                             *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/gpio.h>
#include <hwconfig.h>
#include <string.h>
#include <ADC1_MDx.h>
#include <calibInfo_MDx.h>
#include <interfaces/nvmem.h>
#include <interfaces/rtc.h>
#include <qdec.h>

mduv3x0Calib_t calibration;
hwInfo_t hwInfo;
static int8_t knob_pos = 0;

#ifdef ENABLE_BKLIGHT_DIMMING
void _Z29TIM1_TRG_COM_TIM11_IRQHandlerv()
{
    if(TIM11->SR & TIM_SR_CC1IF)
    {
        gpio_clearPin(LCD_BKLIGHT); /* Clear pin on compare match */
    }

    if(TIM11->SR & TIM_SR_UIF)
    {
        gpio_setPin(LCD_BKLIGHT);   /* Set pin on counter reload */
    }

    TIM11->SR = 0;
}
#endif

/*
 * Note that this interrupt handler currently assumes only the encoder will
 * ever cause this interrupt to fire
 */
void _Z20EXTI15_10_IRQHandlerv()
{
    /* State storage */
    static uint8_t last_state = 0;

    /* Read curent pin state */
    uint8_t pin_state = gpio_readPin(CH_SELECTOR_1)<<1 | gpio_readPin(CH_SELECTOR_0);
    /* Look up next state */
    uint8_t next_state = HALF_STEP_STATE_TRANSITIONS[last_state][pin_state];
    /* update state for next call */
    last_state = next_state & QDECODER_STATE_BITMASK;

    /* Mask out events to switch on */
    uint8_t event = next_state & QDECODER_EVENT_BITMASK;

    /* Update file global knob_pos variable */
    switch (event)
    {
        case QDECODER_EVENT_CW:
            knob_pos++;
            break;
        case QDECODER_EVENT_CCW:
            knob_pos--;
            break;
        default:
            break;
    }

    /* Clear pin change flags */
    EXTI->PR = EXTI_PR_PR11 | EXTI_PR_PR14;
}

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);

    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);

    gpio_setMode(CH_SELECTOR_0, INPUT_PULL_UP);
    gpio_setMode(CH_SELECTOR_1, INPUT_PULL_UP);

    EXTI->IMR  |= EXTI_IMR_MR11 | EXTI_IMR_MR14;
    EXTI->RTSR |= EXTI_RTSR_TR11 | EXTI_RTSR_TR14;
    EXTI->FTSR |= EXTI_FTSR_TR11 | EXTI_FTSR_TR14;

    SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI11_PB;
    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI14_PE;

    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_SetPriority(EXTI15_10_IRQn, 15);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    gpio_setMode(PTT_SW, INPUT_PULL_UP);

    gpio_setMode(PWR_SW, OUTPUT);

    /*
     * Initialise ADC1, for vbat, RSSI, ...
     * Configuration of corresponding GPIOs in analog input mode is done inside
     * the driver.
     */
    adc1_init();

    memset(&hwInfo, 0x00, sizeof(hwInfo));

    nvm_init();                      /* Initialise non volatile memory manager */
    nvm_readCalibData(&calibration); /* Load calibration data                  */
    nvm_loadHwInfo(&hwInfo);         /* Load hardware information data         */
    rtc_init();                      /* Initialise RTC                         */

    #ifdef ENABLE_BKLIGHT_DIMMING
    /*
     * Configure TIM11 for backlight PWM: Fpwm = 256Hz, 8 bit of resolution.
     * APB2 freq. is 84MHz but timer runs at twice this frequency, then:
     * PSC = 2564 to have Ftick = 65.52kHz
     * With ARR = 256, Fpwm is 256Hz;
     */
    RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;
    __DSB();

    TIM11->ARR = 255;
    TIM11->PSC = 2563;
    TIM11->CNT = 0;
    TIM11->CR1   |= TIM_CR1_ARPE;
    TIM11->CCMR1 |= TIM_CCMR1_OC1M_2
                 |  TIM_CCMR1_OC1M_1
                 |  TIM_CCMR1_OC1PE;
    TIM11->CCER  |= TIM_CCER_CC1E;
    TIM11->CCR1 = 0;
    TIM11->EGR  = TIM_EGR_UG;        /* Update registers            */
    TIM11->SR   = 0;                 /* Clear interrupt flags       */
    TIM11->DIER = TIM_DIER_CC1IE     /* Interrupt on compare match  */
                | TIM_DIER_UIE;      /* Interrupt on counter reload */
    TIM11->CR1 |= TIM_CR1_CEN;       /* Start timer                 */

    NVIC_ClearPendingIRQ(TIM1_TRG_COM_TIM11_IRQn);
    NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn,15);
    NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);
    #endif
}

void platform_terminate()
{
    /* Shut down backlight */
    gpio_clearPin(LCD_BKLIGHT);

    #ifdef ENABLE_BKLIGHT_DIMMING
    RCC->APB2ENR &= ~RCC_APB2ENR_TIM11EN;
    __DSB();
    #endif

    /* Shut down LEDs */
    gpio_clearPin(GREEN_LED);
    gpio_clearPin(RED_LED);

    /* Shut down all the modules */
    adc1_terminate();
    nvm_terminate();
    rtc_terminate();

    /* Finally, remove power supply */
    gpio_clearPin(PWR_SW);
}

float platform_getVbat()
{
    /*
     * Battery voltage is measured through an 1:3 voltage divider and
     * adc1_getMeasurement returns a value in mV. Thus, to have effective
     * battery voltage multiply by three and divide by 1000
     */
    return adc1_getMeasurement(0)*3.0f/1000.0f;
}

float platform_getMicLevel()
{
    return 0.0f;
}

float platform_getVolumeLevel()
{
    return adc1_getMeasurement(1);
}

int8_t platform_getChSelector()
{
    /*
     * The knob_pos variable is set in the EXTI15_10 interrupt handler
     * this is safe because interrupt nesting is not allowed.
     */
    return knob_pos;
}

bool platform_getPttStatus()
{
    /* PTT line has a pullup resistor with PTT switch closing to ground */
    return (gpio_readPin(PTT_SW) == 0) ? true : false;
}

void platform_ledOn(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpio_setPin(GREEN_LED);
            break;

        case RED:
            gpio_setPin(RED_LED);
            break;

        default:
            break;
    }
}

void platform_ledOff(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpio_clearPin(GREEN_LED);
            break;

        case RED:
            gpio_clearPin(RED_LED);
            break;

        default:
            break;
    }
}

void platform_beepStart(uint16_t freq)
{
    /* TODO */
    (void) freq;
}

void platform_beepStop()
{
    /* TODO */
}

void platform_setBacklightLevel(uint8_t level)
{
    /*
     * Little workaround for the following nasty behaviour: if CCR1 value is
     * zero, a waveform with 99% duty cycle is generated. This is because we are
     * emulating pwm with interrupts.
     */
    if(level > 1)
    {
        #ifdef ENABLE_BKLIGHT_DIMMING
        TIM11->CCR1 = level;
        TIM11->CR1 |= TIM_CR1_CEN;
        #else
        gpio_setPin(LCD_BKLIGHT);
        #endif
    }
    else
    {
        #ifdef ENABLE_BKLIGHT_DIMMING
        TIM11->CR1 &= ~TIM_CR1_CEN;
        #endif
        gpio_clearPin(LCD_BKLIGHT);
    }
}

const void *platform_getCalibrationData()
{
    return ((const void *) &calibration);
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
