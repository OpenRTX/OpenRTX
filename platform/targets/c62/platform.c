/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <interfaces/audio.h>

static const hwInfo_t hwInfo = {
    .name = "c62",
    .hw_version = 0,
    .uhf_band = 1,
    .vhf_band = 1,
    .uhf_maxFreq = 480,
    .uhf_minFreq = 400,
    .vhf_maxFreq = 174,
    .vhf_minFreq = 137,
};

// ADC
#define ADC_RAW_CHARGING_THRESHOLD 2100
static int32_t adc_vref = 0;
static bool is_battery_charging = false;

static int battery_init(void)
{
    if (!device_is_ready(adc_dev)) {
        printk("ADC device not ready\n");
        return -1;
    }

    /* Apply pin configuration for ADC pins
       because it is overwritten by DSP firmware execution,
       we need to re-apply it here */
    struct adc_csk6_cfg {
        uint8_t pin;
        const struct pinctrl_dev_config *pcfg;
    };
    const struct adc_csk6_cfg *config = adc_dev->config;
    pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

    // Configure ADC channel
    struct adc_channel_cfg channel_cfg = {
        .gain = ADC_GAIN_1,
        .reference = ADC_REF_INTERNAL,
        .channel_id = 2, // ADC2 channel (pin B08)
    };

    int ret = adc_channel_setup(adc_dev, &channel_cfg);
    if (ret < 0) {
        printk("Failed to setup ADC channel: %d\n", ret);
        return -2;
    }

    channel_cfg.channel_id = 1; // ADC1 channel (Charge detection)

    ret = adc_channel_setup(adc_dev, &channel_cfg);
    if (ret < 0) {
        printk("Failed to setup ADC channel: %d\n", ret);
        return -2;
    }

    // Read ADC reference voltage if supported
    adc_vref = adc_ref_internal(adc_dev);

    return 0;
}

void platform_init_csk6()
{
    // Configure the PTT key as input with pull-up
    gpio_pin_configure_dt(&button_ptt, GPIO_INPUT);

    // Configure light pins as outputs and set initial state
    gpio_pin_configure_dt(&led_white, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_keyboard, GPIO_OUTPUT_INACTIVE);

    /* Init ADC for Battery reading */
    battery_init();

    /* C62 can boot with power button off when charging.
     * That's why we put a wait here until the power button is switched on.
     */
    do {
        sleepFor(0, 10);
    } while (!platform_pwrButtonStatus());

    // Enable keyboard backlight
    gpio_pin_set_dt(&led_keyboard, 1);

    /* Initialise BK4819 transceiver */
    bk4819_init(&c62_bk4819);

    /* Initialise audio */
    audio_init();
}

void platform_terminate()
{
    /*
     * C62 does not have a proper power on/off mechanism and the MCU is
     * always powered. Thus, for turn off, perform a system reset.
     */
    NVIC_SystemReset();
    while (1)
        ;
}

/**
 * Read battery voltage using ADC2 on pin B08
 * @return: Battery voltage in millivolts (mV), or 0 on error
 */
uint16_t platform_getVbat()
{
#define ADC_NUM_CHANNELS 2
    static int16_t adc_raw_buffer[ADC_NUM_CHANNELS];
    static struct adc_sequence sequence = {
        /* individual channels will be added below */
        /* BIT(2) Channel 2 (BAT_DET), BIT(1) Channel 1 (Charge detection) */
        .channels = BIT(2) | BIT(1),
        .buffer = adc_raw_buffer,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(adc_raw_buffer),
        .resolution = 11, // 11-bit resolution
    };

    // Reset ADC channels to read
    sequence.channels = BIT(1) | BIT(2);

    // Read ADC value
    int ret = adc_read(adc_dev, &sequence);
    if (ret < 0) {
        printk("Failed to read ADC: %d\n", ret);
        return 0;
    }

    if (adc_raw_buffer[0] > ADC_RAW_CHARGING_THRESHOLD) {
        if (!is_battery_charging) {
            is_battery_charging = true;
            printk("\n** Charging started **\n");
        }
    } else {
        if (is_battery_charging) {
            is_battery_charging = false;
            printk("\n** Charging stopped **\n");
        }
    }

    int32_t bat_det_value =
        adc_raw_buffer[1]
        - 2048; // Adjust for 11-bit ADC with bipolar range (-2048 to 2047)
    adc_raw_to_millivolts(adc_vref, ADC_GAIN_1, 11 /* adb resolution */,
                          &bat_det_value);

    // Adapt for voltage divider 200K and 100K
    uint32_t battery_voltage_mv = bat_det_value * 3;

    //printk("Battery: %d mV\n", battery_voltage_mv);
    return (uint16_t)battery_voltage_mv;
}

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

bool platform_getPttStatus()
{
    //return gpio_pin_get_dt(&button_ptt); // This may brick your radio! Verify what i did in radio_C62.cpp before running this!
    return false;
}

bool platform_pwrButtonStatus()
{
    /* Return true if battery voltage is above 3V.
     * If a voltage lower than 3V would be used for comparison,
     * the radio would take longer to detect the power off when
     * the charger is connected.
    */
    return platform_getVbat() > 3000;
}

void platform_ledOn(led_t led)
{
    switch (led) {
        case WHITE:
            gpio_pin_set_dt(&led_white, 1);
            break;
        case GREEN:
            gpio_pin_set_dt(&led_green, 1);
            break;
        case RED:
            // Controlled by PA activation
            break;
        default:
            break;
    }
}

void platform_ledOff(led_t led)
{
    switch (led) {
        case WHITE:
            gpio_pin_set_dt(&led_white, 0);
            break;
        case GREEN:
            gpio_pin_set_dt(&led_green, 0);
            break;
        case RED:
            // Controlled by PA dactivation
            break;
        default:
            break;
    }
}

void platform_beepStart(uint16_t freq)
{
    BK4819_BeepStart(&c62_bk4819, freq, true);
}

void platform_beepStop()
{
    BK4819_BeepStop(&c62_bk4819);
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
