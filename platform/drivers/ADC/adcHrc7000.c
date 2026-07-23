/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Driver for the HR_C7000 on-chip ADC (base 0x140d0000, manual 4.13),
 * implementing the standard peripherals/adc.h interface.  Single-shot 10-bit
 * conversions, busy-polled (no RTOS/semaphore).  On the Ailunce HD2 the battery
 * pack sits on channel 2 (ADC_HW->DATA_CD[9:0]).
 *
 * The reset-release pulse in adcHrc7000_init() is essential: without it the
 * conversion FSM stays held and DATA reads 0 (live-verified 2026-06-01; with it
 * ch2 reads a stable ~0x350).
 */

#include "adcHrc7000.h"
#include "registers.h"
#include <pthread.h>
#include <stdbool.h>

#define ADC_STATE_BUSY 0x01u    /* ADC_CTRL_BUSY              */
#define ADC_STATE_FSM 0x3eu     /* SAMP_FSM_STATE (bits[5:1]) */
#define ADC_SAMPLE_MASK 0x3ffu  /* 10-bit result             */
#define ADC_GUARD_ITERS 200000u /* bounded busy-wait guard   */

/* Poll CTRL_STATE until the conversion FSM reaches the requested phase:
 *   waitBusy = false -> BUSY + SAMP_FSM clear (conversion idle / complete);
 *   waitBusy = true  -> BUSY or SAMP_FSM set (conversion has started).
 * Returns 1 when the phase is reached, 0 on the bounded-guard timeout. */
static int adc_wait(bool waitBusy)
{
    for (uint32_t guard = 0; guard < ADC_GUARD_ITERS; ++guard) {
        bool busy = (ADC_HW->CTRL_STATE & (ADC_STATE_BUSY | ADC_STATE_FSM))
                 != 0u;
        if (busy == waitBusy)
            return 1;
    }
    return 0;
}

int adcHrc7000_init(const struct Adc *adc)
{
    ADC_HW->CTRL = 0u;
    ADC_HW->CTRL = 8u; /* PD_FORCE soft-reset (the missing step) */
    ADC_HW->CTRL_STOP = 2u;
    (void)adc_wait(false);
    ADC_HW->CTRL = 0u; /* release reset -> normal operation */
    ADC_HW->SEOC_TIME = 0xa20u;
    ADC_HW->P2S_EN = 0u;
    ADC_HW->INTR = 0u;
    ADC_HW->CH_VLD = 0u;

    if (adc->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *)adc->mutex, NULL);

    return 0;
}

void adcHrc7000_terminate(const struct Adc *adc)
{
    if (adc->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *)adc->mutex);
}

uint16_t adcHrc7000_sample(const struct Adc *adc, const uint32_t channel)
{
    (void)adc; /* single ADC block on the HR_C7000 */

    if (channel > 7u)
        return 0;
    if (!adc_wait(false))
        return 0;

    ADC_HW->CH_VLD = (1u << channel);
    ADC_HW->START = 1u;

    /* Wait for the FSM to actually enter BUSY before polling for completion.
     * Best-effort (bounded): if the conversion already finished we fall through
     * to the idle-wait below, which latches the just-finished result.  Without
     * this the idle-wait can observe the still-idle FSM and return the PREVIOUS
     * conversion's DATA -- on the first sample that is the reset value 0, read
     * by the battery monitor as a flat pack (spurious low-battery alarm). */
    (void)adc_wait(true);
    if (!adc_wait(false))
        return 0;

    uint32_t data;
    switch (channel >> 1) {
        case 0:
            data = ADC_HW->DATA_AB;
            break; /* ch 0/1 */
        case 1:
            data = ADC_HW->DATA_CD;
            break; /* ch 2/3 */
        case 2:
            data = ADC_HW->DATA_EF;
            break; /* ch 4/5 */
        default:
            data = ADC_HW->DATA_GH;
            break; /* ch 6/7 */
    }
    if (channel & 1u)
        data >>= 16;

    return (uint16_t)(data & ADC_SAMPLE_MASK);
}
