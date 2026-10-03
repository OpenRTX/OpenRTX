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
#define ADC_NUM_CHANNELS 2
static int32_t adc_vref = 0;
static int16_t adc_raw_buffer[ADC_NUM_CHANNELS];
struct adc_sequence sequence = {
    /* individual channels will be added below */
    .channels = BIT(
        2), // BIT(2) Channel 2 (BAT_DET), BIT(1) Channel 1 (Charge detection)
    .buffer = adc_raw_buffer,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(adc_raw_buffer),
    .resolution = 11, // 11-bit resolution
};

static int battery_init(void);

void platform_init_csk6()
{
    // Configure the PTT key as input with pull-up
    gpio_pin_configure_dt(&button_ptt, GPIO_INPUT);

    // Configure light pins as outputs and set initial state
    gpio_pin_configure_dt(&led_white, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_keyboard, GPIO_OUTPUT_INACTIVE);

    // Enable keyboard backlight
    gpio_pin_set_dt(&led_keyboard, 1);

    /* Initialise BK4819 transceiver */
    bk4819_init(&c62_bk4819);

    /* Initialise audio */
    audio_init();

    /* Init ADC for Battery reading */
    battery_init();
}

void platform_terminate()
{
    ;
}

static int battery_init(void)
{
    if (!device_is_ready(adc_dev)) {
        printk("ADC device not ready\n");
        return -1;
    }

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

/**
 * Read battery voltage using ADC2 on pin B08
 * @return: Battery voltage in millivolts (mV), or 0 on error
 */
uint16_t platform_getVbat()
{
    return 7200; // Return fixed value until voltage reading is working reliably
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
    return gpio_pin_get_dt(&button_ptt);
}

bool platform_pwrButtonStatus()
{
    return true;
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
