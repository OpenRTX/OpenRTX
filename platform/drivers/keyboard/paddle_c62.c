/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <hwconfig.h>
#include "core/cw_keyer.h"
#include "interfaces/paddle.h"
#include "gpio.h"

#include <array>
/*
 * Paddle on the 3.5 mm jack J4: tip dit, ring dah, sleeve common, as paddles
 * are wired (C62 MCU schematic, https://github.com/JKI757/C62_Teardown, file
 * Schematics/C62__MCU.pdf). The jack has no ground:
 *
 * - the sleeve is the PTT line, GPIO_A_15 through 2 kOhm, with a 10 kOhm
 *   pull-up to 3.3 V and a capacitor to ground;
 * - the tip has its own 10 kOhm pull-up to 3.3 V;
 * - the ring is the external microphone, fed from MICBIAS through 2.4 kOhm.
 *
 * The sleeve reads high whichever paddle is closed, but each paddle adds a
 * pull-up charging the sleeve capacitor: the tip a 10 kOhm one, the ring a
 * stronger one through the microphone bias. The line is therefore read
 * dynamically: driven low to discharge the capacitor, released, and timed
 * until it reads high again. Measured on a C62, the time is 14-15 us with the
 * paddles open, 4 us with dit, 2 us with dah and 0.75 us with both, the same
 * while transmitting at 5 W. Against the open time, measured when the CW mode
 * starts, a time under 1/12 of it is both paddles, under 1/5 dah and under
 * 1/2 dit. A line that does not rise is held low by the PTT key, and reads as
 * no paddle.
 *
 * The pin has other functions, such as I2S, and the DSP firmware sets up pins
 * when it starts audio. A pin that is no longer a GPIO is not discharged and
 * reads high at once, faster than any paddle: the pin is then configured
 * again and the open time measured again.
 */

// GPIO port of the PTT line, for direct register access: the Zephyr driver
// reconfigures the pin mux on each direction change, too slowly to time
#define PTT_PORT ((GPIO_RegDef *)DT_REG_ADDR(DT_NODELABEL(gpioa)))

#define DISCHARGE_US 20 // Line driven low, 6 time constants of 2 kOhm, 1.5 nF
#define TIMEOUT_US 100  // Line held low after this long
#define CALIB_READS 9   // Reads with the paddles open, for the median
#define BOTH_DIV 12     // Both paddles under 1/12 of the open time
#define DAH_DIV 5       // Dah under 1/5 of the open time
#define DIT_DIV 2       // Dit under 1/2 of the open time
#define MIN_RISE_NS 300 // Faster than both paddles (750 ns): not a GPIO
#define STABLE_READS 3  // Reads in a row before a new paddle state counts

static uint32_t openNs = 0; // Rise time with the paddles open, 0 if not known

/*
 * Debouncing: a read straddling a bounce of the paddle contact gives a rise
 * time between two states, a dah read as a dit for example, so a new state
 * counts only after STABLE_READS reads in a row.
 */
static uint8_t reported;  // Paddles reported
static uint8_t candidate; // Paddles read last
static uint8_t readCount; // Reads in a row of the candidate

/**
 * Discharge the PTT line and time its rise.
 *
 * @param dischargeUs: time the line is driven low, in us.
 * @param timeoutUs: longest wait for the line to rise, in us.
 * @return rise time in ns, timeoutUs in ns if the line stayed low.
 */
static uint32_t riseTime(uint32_t dischargeUs, uint32_t timeoutUs)
{
    GPIO_RegDef *port = PTT_PORT;
    const uint32_t mask = BIT(button_ptt.pin);
    const uint32_t limit = k_us_to_cyc_ceil32(timeoutUs);
    uint32_t elapsed = 0;

    // Other threads change the port direction too, as the bit-banged buses
    unsigned int key = irq_lock();
    port->REG_DOUTCLEAR.all = mask;
    port->REG_CHANNELDIR.all |= mask;
    irq_unlock(key);

    k_busy_wait(dischargeUs);

    key = irq_lock();
    uint32_t start = k_cycle_get_32();
    port->REG_CHANNELDIR.all &= ~mask;
    do {
        elapsed = k_cycle_get_32() - start;
    } while (((port->REG_DATAIN.all & mask) == 0) && (elapsed < limit));
    irq_unlock(key);

    return k_cyc_to_ns_floor32(elapsed);
}

/**
 * Measure the rise time with the paddles open, if it is plausible.
 */
static void calibrate(void)
{
    std::array<uint32_t, CALIB_READS> reads;

    for (size_t i = 0; i < CALIB_READS; i++)
        reads[i] = riseTime(DISCHARGE_US, TIMEOUT_US);

    qsort(reads.begin(), reads.end(), sizeof(reads[0]),
          [](const uint32_t a, const uint32_t b) {
        if (a > b)
            return 1;
        if (a < b)
            return -1;
        return 0;};

    uint32_t median = reads[CALIB_READS / 2];
    if ((median >= MIN_RISE_NS) && (median < TIMEOUT_US * 1000U))
        openNs = median;
}

void paddle_init(void)
{
    // GPIO input with the pull-up, as platform_init_csk6() sets the PTT key
    gpio_pin_configure_dt(&button_ptt, GPIO_INPUT);
    openNs = 0;
    reported = 0;
    candidate = 0;
    readCount = 0;
    calibrate();
}

void paddle_terminate(void)
{
    // Back to the PTT key configuration of platform_init_csk6()
    gpio_pin_configure_dt(&button_ptt, GPIO_INPUT);
}

/**
 * Tell the paddles closed from a rise time.
 *
 * @param ns: rise time, in ns.
 * @return paddles closed, enum cwPaddle bits.
 */
static uint8_t classify(uint32_t ns)
{
    if ((ns * BOTH_DIV) < openNs)
        return CW_PADDLE_DIT | CW_PADDLE_DAH;
    if ((ns * DAH_DIV) < openNs)
        return CW_PADDLE_DAH;
    if ((ns * DIT_DIV) < openNs)
        return CW_PADDLE_DIT;

    return 0;
}

uint8_t paddle_read(void)
{
    uint32_t ns = riseTime(DISCHARGE_US, TIMEOUT_US);

    if (ns < MIN_RISE_NS) {
        paddle_init();
        return 0;
    }

    if (openNs == 0) {
        calibrate();
        return 0;
    }

    uint8_t paddles = classify(ns);
    if (paddles != candidate) {
        candidate = paddles;
        readCount = 0;
    }

    if (readCount < STABLE_READS)
        readCount++;
    if (readCount == STABLE_READS)
        reported = candidate;

    return reported;
}
