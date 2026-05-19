/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "peripherals/gpio.h"
#include "hwconfig.h"
#include <qdec.h>
#include "chSelector.h"

static uint8_t last_state = 0;  /* State storage */
static int8_t knob_pos    = 0;  /* Knob position */


/* Name of interrupt handler is mangled for C++ compatibility */
void _Z20EXTI15_10_IRQHandlerv()
{
    if(EXTI->PR & (EXTI_PR_PR11 | EXTI_PR_PR14))
    {
        /* Clear interrupt flags */
        EXTI->PR = EXTI_PR_PR11 | EXTI_PR_PR14;

        /* Read curent pin state */
        uint8_t pin_state = (gpio_readPin(CH_SELECTOR_1) << 1)
                          |  gpio_readPin(CH_SELECTOR_0);

        /* Look up next state */
        uint8_t next_state = HALF_STEP_STATE_TRANSITIONS[last_state][pin_state];

        /* update state for next call */
        last_state = next_state & QDECODER_STATE_BITMASK;

        /* Mask out events to switch on */
        uint8_t event = next_state & QDECODER_EVENT_BITMASK;

        /* Update knob_pos variable */
        switch(event)
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
    }
}


void chSelector_init()
{
    gpio_setMode(CH_SELECTOR_0, INPUT_PULL_UP);
    gpio_setMode(CH_SELECTOR_1, INPUT_PULL_UP);

    /*
     * Configure GPIO interrupts: encoder signal is on PB11 and PE14
     */
    EXTI->IMR  |= EXTI_IMR_MR11  | EXTI_IMR_MR14;
    EXTI->RTSR |= EXTI_RTSR_TR11 | EXTI_RTSR_TR14;
    EXTI->FTSR |= EXTI_FTSR_TR11 | EXTI_FTSR_TR14;

    SYSCFG->EXTICR[2] |= SYSCFG_EXTICR3_EXTI11_PB;
    SYSCFG->EXTICR[3] |= SYSCFG_EXTICR4_EXTI14_PE;

    NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    NVIC_SetPriority(EXTI15_10_IRQn, 15);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void chSelector_terminate()
{
    EXTI->IMR &= ~(EXTI_IMR_MR1 | EXTI_IMR_MR14);
    NVIC_DisableIRQ(EXTI15_10_IRQn);
}


/*
 * This function is defined in platform.h
 */
int8_t platform_getChSelector()
{
    /*
     * The knob_pos variable is set in the EXTI15_10 interrupt handler
     * this is safe because interrupt nesting is not allowed.
     */
    return knob_pos;
}
