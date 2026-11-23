/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <interfaces/audio.h>
#include <interfaces/nvmem.h>
#include "drivers/SPI/spi_zephyr.h"
#include "drivers/audio/csk6_codec.h"
#include "mcu/CSK6011B/drivers/lsf_controller.h"
#include "mcu/CSK6011B/drivers/lsf_dsp.h"

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
#define BAT_DET_SETTLE_MS 10 // Measured: settles to 1 LSB in 5 ms
#define ADC_NUM_CHANNELS 2
#define ADC_RESOLUTION 11
static int32_t adc_vref = 0;
static bool is_battery_charging = false;

PINCTRL_DT_DEFINE(DT_NODELABEL(adc0));
PINCTRL_DT_DEFINE(DT_CHOSEN(zephyr_console));

/**
 * Re-apply what the DSP firmware overwrites when it starts and sets up audio:
 * the battery ADC pins, which also read the power knob, and the console pins
 * and UART settings. Then wait for the battery reading to settle.
 */
static void restore_dsp_overwrites(void)
{
    const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    struct uart_config config;

    int adc = pinctrl_apply_state(PINCTRL_DT_DEV_CONFIG_GET(DT_NODELABEL(adc0)),
                                  PINCTRL_STATE_DEFAULT);
    int pins = pinctrl_apply_state(
        PINCTRL_DT_DEV_CONFIG_GET(DT_CHOSEN(zephyr_console)),
        PINCTRL_STATE_DEFAULT);
    /* The driver keeps its configuration, which the DSP leaves alone */
    int uart = uart_config_get(console, &config);
    if (uart == 0)
        uart = uart_configure(console, &config);
    if ((adc != 0) || (pins != 0) || (uart != 0))
        printk("Restore after DSP start failed: adc=%d pins=%d uart=%d\n", adc,
               pins, uart);

    sleepFor(0, BAT_DET_SETTLE_MS);
}

static int battery_init(void)
{
    if (!device_is_ready(adc_dev)) {
        printk("ADC device not ready\n");
        return -ENODEV;
    }

    // Configure ADC channel
    struct adc_channel_cfg channel_cfg = {
        .gain = ADC_GAIN_1,
        .reference = ADC_REF_INTERNAL,
        .channel_id = ADC_VBAT_CH,
    };

    int ret = adc_channel_setup(adc_dev, &channel_cfg);
    if (ret < 0) {
        printk("Failed to setup ADC channel: %d\n", ret);
        return ret;
    }

    channel_cfg.channel_id = ADC_CHARGE_CH;

    ret = adc_channel_setup(adc_dev, &channel_cfg);
    if (ret < 0) {
        printk("Failed to setup ADC channel: %d\n", ret);
        return ret;
    }

    // Read ADC reference voltage if supported
    adc_vref = adc_ref_internal(adc_dev);

    return 0;
}

void platform_init()
{
    /* Initialise non volatile memory manager */
    nvm_init();

    // Configure the PTT key as input with pull-up
    gpio_pin_configure_dt(&button_ptt, GPIO_INPUT);

    // Configure light pins as outputs and set initial state
    gpio_pin_configure_dt(&led_white, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_keyboard, GPIO_OUTPUT_INACTIVE);

    /* Init ADC for Battery reading: without it the power knob can't be read */
    if (battery_init() != 0)
        printk("Battery ADC init failed, the radio can't turn on\n");

    /* C62 can boot with power button off when charging.
     * That's why we put a wait here until the power button is switched on.
     */
    do {
        sleepFor(0, 10);
    } while (!platform_pwrButtonStatus());

    // Enable keyboard backlight
    gpio_pin_set_dt(&led_keyboard, 1);

    /* Initialise BK4819 SPI bus and transceiver */
    spiZephyr_init(c62_bk4819.spi);
    bk4819_init(&c62_bk4819);

    /*
     * Start the DSP only now, after the BK4819 microphone input has settled.
     * It changes pins while it starts and sets up audio: restore them after
     * audio_init(). Nothing reads the battery ADC before then.
     */
    int ret = lsfDsp_init();
    if (ret == 0)
        ret = lsfController_init();

    /* Initialise audio, which runs on the DSP */
    if (ret == 0)
        audio_init();

    restore_dsp_overwrites();
    if (ret != 0)
        printk("DSP start failed, no audio: %d\n", ret);
}

void platform_terminate()
{
    /*
     * C62 does not have a proper power on/off mechanism and the MCU is
     * always powered. Thus, for turn off, perform a system reset.
     *
     * With the charger connected the chip stays powered through the reset:
     * stop the DSP and reset the peripherals along with the core, or the
     * DSP's UART interrupt halts the next boot.
     */
    lsfDsp_terminate();
    sys_reboot(SYS_REBOOT_COLD);
}

/**
 * Read battery voltage using ADC2 on pin B08
 * @return: Battery voltage in millivolts (mV), or 0 on error
 */
uint16_t platform_getVbat()
{
    // Samples of the charge detection and battery channels, in this order
    static int16_t adc_raw_buffer[ADC_NUM_CHANNELS];
    const struct adc_sequence sequence = {
        .channels = BIT(ADC_CHARGE_CH) | BIT(ADC_VBAT_CH),
        .buffer = adc_raw_buffer,
        /* buffer size in bytes, not number of samples */
        .buffer_size = sizeof(adc_raw_buffer),
        .resolution = ADC_RESOLUTION,
    };

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
    adc_raw_to_millivolts(adc_vref, ADC_GAIN_1, ADC_RESOLUTION, &bat_det_value);

    // Adapt for voltage divider 200K and 100K
    uint32_t battery_voltage_mv = bat_det_value * 3;

    return (uint16_t)battery_voltage_mv;
}

uint8_t platform_getMicLevel()
{
    /* The meter is shown only in the M17 and DMR modes: scale it by the
     * software gain to show what the encoder sees, so full scale is clipping */
    return MIN((csk6codec_getMicPeak() * CONFIG_MIC_GAIN) >> 7, UINT8_MAX);
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
    return gpio_pin_get_dt(&button_ptt) > 0;
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
    /*
     * No BK4819 beeps: they come out of its AF output, which the speaker
     * mux doesn't select, and bk4819_beepStart() stops RX until the next
     * retune.
     */
    (void)freq;
}

void platform_beepStop()
{
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
