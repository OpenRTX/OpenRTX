/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Ailunce HD2 platform layer -- minimal bring-up (display / backlight /
 * keyboard / RTC / power).  The Miosix bsp already brings up clocks, the
 * watchdog-off, the UART0 console, the LEDs and the PTB13 power self-latch,
 * so this file only owns the OpenRTX-facing HAL and the power/PTT sense.
 */

#include "interfaces/platform.h"
#include "peripherals/gpio.h"
#include "peripherals/rtc.h"
#include "drivers/ADC/adcHrc7000.h"
#include "drivers/GPS/gps_HD2.h"
#include "drivers/backlight/backlight.h"
#include "hwconfig.h"
#include "pinmap.h"

/* The on-chip ADC instance (adc1), its mutex, and the hwInfo descriptor live in
 * hwconfig.c; ADC_VBAT_CH is in hwconfig.h (enum AdcChannels).  hwInfo is
 * extern'd here rather than in hwconfig.h to avoid a hwconfig.h <-> platform.h
 * include cycle. */
extern const hwInfo_t hwInfo;

void platform_init()
{
    /* Establish the V2.1.3 SoC pad-mux + GPIO baseline for PTA/PTB.  The Miosix
     * kernel already brought up the PLL, timers and UART0; here we only own the
     * pad mux / GPIO directions the vendor firmware sets.  (Clocks are NOT
     * re-touched under Miosix -- re-running the PLL init hangs the kernel
     * timer.)  These are whole-register vendor snapshot values; the upper DIPLEX
     * bits are write-only, so never read-modify-write them.  The PTC/LCD pad-mux
     * + bus directions are owned by the display driver (display_init). */
    SOCSYS->IO_DIPLEX0 = 0x00000060u; /* PTA: vendor pad-mux baseline */
    SOCSYS->IO_DIPLEX1 = 0x00000007u; /* PTA: SPI0 pads (vendor baseline) */

    /* PTA GPIO direction (DDR) + output levels (DR): the V2.1.3 vendor boot
     * snapshot.  Apart from the rotary encoder (PTA0/1, inputs), these pads'
     * roles aren't reverse-engineered; kept verbatim as the known-good state. */
    GPIOA->DDR = 0x003401e0u;
    GPIOA->DR |= 0x001401e0u;

    GPIOB->DDR = 0x3d7ae51fu; /* incl. knob/PTT sense as input */
    GPIOB->DR |= 0x00506414u; /* preserves the PTB13 power latch */

    adcHrc7000_init(&adc1);   /* on-chip ADC (battery on ADC_VBAT_CH) */
    rtc_init();
}

void platform_terminate()
{
    backlight_terminate();
    rtc_terminate();

    /* The power self-latch drop + SOCSYS system soft-reset is done atomically
     * (IRQs off) by the miosix bsp shutdown() when main() returns -- doing the
     * latch-drop here first would risk a partial brown-out before the reset. */
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}

bool platform_pwrButtonStatus()
{
    return gpio_readPin(PWR_SW) == 0; /* knob LOW = on */
}

bool platform_getPttStatus()
{
    return gpio_readPin(PTT_SW) == 0; /* active-low */
}

void platform_ledOn(led_t led)
{
    switch (led) {
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
    switch (led) {
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

/* --- Battery voltage (on-chip ADC channel 2) ----------------------------- */
uint16_t platform_getVbat()
{
    /* adc_getVoltage returns the pin voltage in uV; the pack is behind a 1:3
     * divider, so multiply by 3 and convert uV -> mV.  The state update task
     * low-pass filters this, so no averaging is done here. */
    return (uint16_t)((adc_getVoltage(&adc1, ADC_VBAT_CH) * 3u) / 1000u);
}

/* --- Other analog/audio sensing not wired in the minimal bring-up --------- */
uint8_t platform_getMicLevel()
{
    return 0;
}
uint8_t platform_getVolumeLevel()
{
    return 0;
}
int8_t platform_getChSelector()
{
    return 0;
}
void platform_beepStart(uint16_t freq)
{
    (void)freq;
}
void platform_beepStop()
{
}

/* --- RTC ----------------------------------------------------------------- */
datetime_t platform_getCurrentTime()
{
    return rtc_getTime();
}

void platform_setTime(datetime_t t)
{
    rtc_setTime(t);
}

/* --- GPS (UART2, polled) ------------------------------------------------- */
const struct gpsDevice *platform_initGps()
{
    return gps_HD2_init();
}
