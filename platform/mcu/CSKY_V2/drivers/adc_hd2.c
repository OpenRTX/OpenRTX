/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Polling driver for the HR_C7000 on-chip ADC (Ailunce HD2 / CSKY V2 ck803s).
 *
 * --------------------------------------------------------------------------
 * Register map -- ALL offsets verified two ways:
 *   (1) HR_C7000 manual "4.13.4 ADC register overview / 寄存器表4.22"
 *       (vendor/hr_c7000_manual.md lines ~2664-2693), ADC base 0x140D0000
 *       (manual line 196: "0x140D 0000 .. ADC 64KB").
 *   (2) Vendor V2.1.3 firmware FUN_0305ae94 disassembly
 *       (tmp/v213_disasm.txt lines 121159-121221).  In that function the
 *       base register is loaded with `movih r2, 5133` (5133 == 0x140d), i.e.
 *       0x140D0000, and the channel-dispatch reads the data registers below.
 *
 *   offset  name (table 4.22)     this driver's use
 *   ------  -------------------   --------------------------------------------
 *   0x14    ADC_START             write 1 to start a single conversion
 *                                 (disasm 305aec4: st.w r2(=1),(r1,0x14))
 *   0x18    ADC_CTRL_STATE        status: bit0 = ADC_CTRL_BUSY,
 *                                 bits[5:1] (0x3e) = SAMP_FSM_STATE.  Vendor
 *                                 spin-waits while either is nonzero
 *                                 (disasm 305aea4-305aeb8).
 *   0x24    ADC_CH_VLD            channel-enable bitmask = (1 << channel)
 *                                 (disasm 305aec2: st.w (1<<ch),(r1,0x24))
 *   0x30    ADC_DATA_AB           channels 0 (bits[9:0]) / 1 (bits[25:16])
 *   0x34    ADC_DATA_CD           channels 2 (bits[9:0]) / 3 (bits[25:16])
 *   0x38    ADC_DATA_EF           channels 4 / 5
 *   0x3c    ADC_DATA_GH           channels 6 / 7
 *                                 (disasm 305aede-305af46: per-channel reads,
 *                                  `andi r4,r4,1023` for the even channel,
 *                                  `zext r4,r4,25,16` for the odd channel.)
 *
 * The battery pack sits on channel 2 -> ADC_DATA_CD bits[9:0]
 * (vendor calls FUN_0305ae94(2); see 0300d000_v2_1_3_app.c:93939).
 *
 * NOTE: unlike the vendor (which gates the whole accessor behind an OS
 * semaphore and waits for an interrupt), we busy-poll ADC_CTRL_STATE with a
 * bounded guard loop, mirroring the clk_init_pll bring-up style in
 * platform.c.  The conversion completes in microseconds, so the brief
 * busy-poll does not meaningfully stall the caller's thread.
 * --------------------------------------------------------------------------
 */

#include "drivers/adc_hd2.h"
#include "hd2_regs.h"

/* ADC_BASE, ADC_CTRL/INTR/START/CTRL_STATE/CTRL_STOP/CH_VLD/SEOC_TIME/
 * P2S_EN/DATA_AB/CD/EF/GH come from hd2_regs.h.  Battery is on channel 2
 * -> ADC_DATA_CD bits[9:0]. */

#define ADC_STATE_BUSY      0x01u   /* ADC_CTRL_BUSY                 */
#define ADC_STATE_FSM       0x3eu   /* SAMP_FSM_STATE (bits[5:1])    */
#define ADC_SAMPLE_MASK     0x3ffu  /* 10-bit result                 */

/* Bounded busy-wait: the conversion is 200 KSPS (~5us), so a few thousand
 * spin iterations at the ck803s clock is far more than one conversion needs.
 * If the controller never goes idle we bail rather than hang the caller. */
#define ADC_GUARD_ITERS     200000u

/*
 * Wait for the controller to be idle (BUSY and SAMP_FSM clear), matching the
 * vendor's two-stage spin at disasm 305aea4..305aeb8.  Returns 1 on idle,
 * 0 on timeout.
 */
static int adc_wait_idle(void)
{
    for (uint32_t guard = 0; guard < ADC_GUARD_ITERS; ++guard)
    {
        uint32_t st = ADC_CTRL_STATE;
        if ((st & (ADC_STATE_BUSY | ADC_STATE_FSM)) == 0u)
            return 1;
    }
    return 0;
}

/*
 * One-time ADC controller reset + config -- the step our driver was missing.
 * LIVE-VERIFIED 2026-06-01: without the ADC_CTRL reset-release pulse the
 * conversion FSM stays held (registers respond but DATA=0); with it, ch2
 * reads a stable ~0x357.  Replays vendor adc_controller_init (FUN_0305af7c):
 * DIPLEX2 ADC pin-mux, the 0->8(PD_FORCE)->idle->0 reset pulse, then config.
 */
static int g_adc_inited = 0;

void adc_hd2_init(void)
{
    SOCSYS_IO_DIPLEX2 &= 0xffc7ffffu;   /* clear DIPLEX2 bits 19-21: ADC input pin-mux */
    ADC_CTRL      = 0u;
    ADC_CTRL      = 8u;              /* PD_FORCE_GIGH soft-reset  *** the missing step *** */
    ADC_CTRL_STOP = 2u;
    (void)adc_wait_idle();           /* wait BUSY + SAMP_FSM clear */
    ADC_CTRL      = 0u;              /* release reset -> normal operation */
    ADC_SEOC_TIME = 0xa20u;          /* ADC_PD_SEOC_TIME */
    ADC_P2S_EN    = 0u;
    ADC_INTR      = 0u;
    ADC_CH_VLD    = 0u;
    g_adc_inited  = 1;
}

uint16_t adc_hd2_sample(uint8_t channel)
{
    if (channel > 7u)
        return 0;

    if (!g_adc_inited)
        adc_hd2_init();

    /* 1) make sure the previous conversion has drained */
    if (!adc_wait_idle())
        return 0;

    /* 2) select the channel (bitmask) and kick a single conversion.
     *    Order matches FUN_0305ae94: ADC_CH_VLD then ADC_START. */
    ADC_CH_VLD = (1u << channel);
    ADC_START  = 1u;

    /* 3) wait for the conversion to complete (controller idle again) */
    if (!adc_wait_idle())
        return 0;

    /* 4) read the data register for this channel pair and extract the
     *    correct half.  Even channels live in bits[9:0]; odd channels in
     *    bits[25:16]. */
    uint32_t data;
    switch (channel >> 1)
    {
        case 0:  data = ADC_DATA_AB; break;   /* ch 0/1 */
        case 1:  data = ADC_DATA_CD; break;   /* ch 2/3 */
        case 2:  data = ADC_DATA_EF; break;   /* ch 4/5 */
        default: data = ADC_DATA_GH; break;   /* ch 6/7 */
    }

    if (channel & 1u)
        data >>= 16;

    return (uint16_t)(data & ADC_SAMPLE_MASK);
}

/*
 * Battery channel + 16-sample sliding average.
 *
 * Ported from vendor FUN_0305853c (0300d000_v2_1_3_app.c:93939-93959):
 *   raw   = FUN_0305ae94(2);          // 10-bit channel-2 sample
 *   raw <<= 2;  raw &= 0x3fff << 2;   // pre-scale by 4
 *   <shift window of 16 uint16 samples, accumulate sum>
 *   avg   = (sum & 0x7ffff) >> 4;     // divide by 16
 * The averaged value `avg` (~0xbf4 at 7.4V) is what feeds the voltage and
 * percentage math.
 */
#define ADC_BATT_CHANNEL    2u
#define ADC_AVG_WINDOW      16u

uint16_t adc_hd2_battery_raw_avg(void)
{
    static uint16_t window[ADC_AVG_WINDOW];
    static int      seeded = 0;

    uint16_t sample = (uint16_t)((adc_hd2_sample(ADC_BATT_CHANNEL) & 0x3fffu) << 2);

    if (!seeded)
    {
        for (uint32_t i = 0; i < ADC_AVG_WINDOW; ++i)
            window[i] = sample;
        seeded = 1;
    }
    else
    {
        /* shift the window down by one and append the new sample */
        for (uint32_t i = 0; i < ADC_AVG_WINDOW - 1u; ++i)
            window[i] = window[i + 1u];
        window[ADC_AVG_WINDOW - 1u] = sample;
    }

    uint32_t sum = 0;
    for (uint32_t i = 0; i < ADC_AVG_WINDOW; ++i)
        sum += window[i];

    return (uint16_t)((sum & 0x7ffffu) >> 4);   /* sum / 16 */
}
