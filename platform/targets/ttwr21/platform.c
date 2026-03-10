/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
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
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "core/gps.h"
#include "sa8x8-fw-sa868s-uhf.h"

#include "drivers/GPS/gps_zephyr.h"
#include "drivers/baseband/SA8x8.h"
#include "drivers/program/program_RL78.h"
#include "drivers/PMU/AXP2101_zephyr.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/uart.h>

enum ttwr21_band {
  VHF = 0,
  MHZ350 = 1,
  UHF = 2,
};

#define BUTTON_PTT_NODE DT_NODELABEL(button_ptt)
#define BUTTON_MODE_NODE DT_NODELABEL(button_mode)

static const struct gpio_dt_spec button_ptt = GPIO_DT_SPEC_GET_OR(BUTTON_PTT_NODE, gpios, {0});
static const struct gpio_dt_spec button_mode = GPIO_DT_SPEC_GET_OR(BUTTON_MODE_NODE, gpios, {0});

static const struct device *const qdec_dev = DEVICE_DT_GET(DT_ALIAS(qdec0));
static const struct device *const led_dev  = DEVICE_DT_GET(DT_ALIAS(led0));

// This is cross-references in keyboard_ttwrplus.c to implement volume control
uint8_t volume_level = 125;

static hwInfo_t hwInfo =
{
    .name        = "ttwr21",
    .hw_version  = 0,
    .uhf_band    = 0,
    .vhf_band    = 0,
    .uhf_maxFreq = 0,
    .uhf_minFreq = 0,
    .vhf_maxFreq = 0,
    .vhf_minFreq = 0,
};

// RGB led color data
static struct led_rgb led_color = {0};

static void gpsEnable(void *priv)
{
    (void) priv;
    pmu_setGPSPower(true);
}

static void gpsDisable(void *priv)
{
    (void) priv;
    pmu_setGPSPower(false);
}

static const struct gpsDevice gps =
{
    .enable = gpsEnable,
    .disable = gpsDisable,
    .getSentence = gpsZephyr_getNmeaSentence
};

#define I2C_DEV_NODE    DT_ALIAS(i2c_0)

// Try reading i2c register to detect radio band
// VHF = 0x3c
// UHF = 0x3d
// 300MHz = ???
int8_t detect_band()
{
    static const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
    if (!i2c_dev) {
        printk("I2C device not found!\n");
        return -1;
    }

    uint8_t val;
    if(!i2c_reg_read_byte(i2c_dev, 0x3c, 0x00, &val)) {
        return VHF;
    } else if(!i2c_reg_read_byte(i2c_dev, 0x3d, 0x00, &val)) {
        return UHF;
    } else {
        printk("Error detecting supported band!\n");
    }
}

void platform_init()
{
    int ret;

    // Setup GPIO for PTT and rotary encoder
    if (!gpio_is_ready_dt(&button_ptt))
        printk("Error: button device %s is not ready\n", button_ptt.port->name);

    if ((ret = gpio_pin_configure_dt(&button_ptt, GPIO_INPUT)))
        printk("Error %d: failed to configure %s pin %d\n", ret,
               button_ptt.port->name, button_ptt.pin);

    if (!gpio_is_ready_dt(&button_mode))
        printk("Error: button device %s is not ready\n", button_mode.port->name);

    if ((ret = gpio_pin_configure_dt(&button_mode, GPIO_INPUT)))
        printk("Error %d: failed to configure %s pin %d\n", ret,
               button_mode.port->name, button_mode.pin);

    // Rotary encoder is read using hardware pulse counter
    if (!device_is_ready(qdec_dev))
        printk("Qdec device is not ready\n");

    // Initialize LED
    if (!device_is_ready(led_dev))
        printk("LED device %s is not ready\n", led_dev->name);

    // Update LED
    if ((ret = led_strip_update_rgb(led_dev, &led_color, 1)))
        printk("LED not updated: %d\n", ret);

    // Initialize PMU
    pmu_init();

    // Detect radio model and set hwInfo accordingly
    enum ttwr21_band band = detect_band();
    switch(band) {
        case VHF:
            hwInfo.vhf_band = 1;
            hwInfo.vhf_minFreq = 134;
            hwInfo.vhf_maxFreq = 174;
            break;
        case UHF:
            hwInfo.uhf_band = 1;
            hwInfo.uhf_minFreq = 400;
            hwInfo.uhf_maxFreq = 480;
            break;
        case MHZ350:
            printk("Error: 350MHz band not supported!\n");
            break;
    }

    if (gpio_pin_get_dt(&button_mode)) {
        led_color.r = 0x50;
        led_color.g = 0x50;
        led_color.b = 0x00;
        led_strip_update_rgb(led_dev, &led_color, 1);

        pmu_setBasebandProgramPower(true);

        // If band is VHF we need to patch the firmware
        if (band == VHF)
            sa8x8_fw_sa868s_uhf[sa8x8_fw_sa868s_uhf_patch_offset] = 'V';

        // Flash a supported baseband firmware
        if (rl78_flash(sa8x8_fw_sa868s_uhf, sa8x8_fw_sa868s_uhf_len)) {
            led_color.r = 0x50;
            led_color.g = 0x00;
            led_color.b = 0x00;
            printk("Failed to flash baseband\n");
        } else {
            led_color.r = 0x00;
            led_color.g = 0x50;
            led_color.b = 0x00;
        }

        led_strip_update_rgb(led_dev, &led_color, 1);

        delayMs(3000);

        led_color.r = 0x00;
        led_color.g = 0x00;
        led_color.b = 0x00;

        led_strip_update_rgb(led_dev, &led_color, 1);

        pmu_setBasebandProgramPower(false);
    }

    // Enable power to baseband
    pmu_setBasebandPower(true);

    // Initialize baseband
    if (sa8x8_init() == -2) {
        // if (sa8x8_init()) {
        //     printk("Error detecting baseband after firmware update\n");
        // }
    }

    // Confirm that SA8x8 has the fw of the corresponding band
    const char *model = sa8x8_getModel();
    if(!strncmp(model, "SA868S-VHF", 10)) {
        if(!hwInfo.vhf_band) {
            printk("Mismatch between radio band and baseband firmware!");
        }
    } else if(!strncmp(model, "SA868S-UHF", 10)) {
        if(!hwInfo.uhf_band) {
            printk("Mismatch between radio band and baseband firmware!");
        }
    } else {
        printk("Error detecting baseband model\n");
    }

    // Connect SA868s to PA
    pmu_setAudioSwitch(false);

    // XXX: test ADC
    // start_adc_test();
}

void platform_terminate()
{
    pmu_terminate();
}

uint16_t platform_getVbat()
{
    return pmu_getVbat();
}

uint8_t platform_getMicLevel()
{
    return 0;
}

uint8_t platform_getVolumeLevel()
{
    return volume_level;
}

int8_t platform_getChSelector()
{
    int rc = sensor_sample_fetch(qdec_dev);
    if (rc != 0)
    {
        printk("Failed to fetch sample (%d)\n", rc);
        return 0;
    }

    struct sensor_value val;
    rc = sensor_channel_get(qdec_dev, SENSOR_CHAN_ROTATION, &val);
    if (rc != 0)
    {
        printk("Failed to get data (%d)\n", rc);
        return 0;
    }

    // The esp32-pcnt sensor returns a signed 16-bit value: we remap it into a
    // signed 8-bit one.
    return (int8_t) val.val1;
}

bool platform_getPttStatus()
{
    return gpio_pin_get_dt(&button_ptt);
}

bool platform_pwrButtonStatus()
{
    // Long press of the power on button triggers a shutdown
    uint8_t btnStatus = pmu_pwrBtnStatus();
    if(btnStatus == 2)
        return false;

    return true;
}

void platform_ledOn(led_t led)
{
    int ret = 0;

    switch(led)
    {
        case GREEN:
            led_color.g = 0xff;
            break;
        case RED:
            led_color.r = 0xff;
            break;
        default:
            break;
    }

    ret = led_strip_update_rgb(led_dev, &led_color, 1);
    if (ret)
        printk("LED not updated: %d\n", ret);
}

void platform_ledOff(led_t led)
{
    int ret = 0;

    switch(led)
    {
        case GREEN:
            led_color.g = 0x00;
            break;
        case RED:
            led_color.r = 0x00;
            break;
        default:
            break;
    }

    ret = led_strip_update_rgb(led_dev, &led_color, 1);
    if (ret)
        printk("LED not updated: %d\n", ret);
}

void platform_beepStart(uint16_t freq)
{
    (void) freq;
}

void platform_beepStop()
{
    ;
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}

const struct gpsDevice *platform_initGps()
{
    gpsZephyr_init();

    return &gps;
}

// ADC2 Timer ISR Demo - 5 Second Recording
#include "drivers/ADC/adc_esp32.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define MIC_CH_SEL_GPIO 17  // GPIO17 controls mic channel select
#define SAMPLE_RATE 8000     // Request 8 kHz for Codec2 MODE_3200
#define RECORDING_SECONDS 1
#define TOTAL_SAMPLES (SAMPLE_RATE * RECORDING_SECONDS)  // 40,000 samples
#define CHUNK_SIZE 256  // Read 256 samples at a time

static void adc_demo_thread(void *, void *, void *);

K_THREAD_DEFINE(adc_demo_tid, 4096, adc_demo_thread, NULL, NULL, NULL, 7, 0, 0);

static void adc_demo_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    // Wait for system to stabilize
    k_sleep(K_SECONDS(2));

    printk("\n=== ADC2 Timer ISR - Mic Recording Demo ===\n");
    printk("Recording %d seconds of audio at %d Hz\n", RECORDING_SECONDS, SAMPLE_RATE);
    printk("Total samples: %d\n", TOTAL_SAMPLES);
    printk("Streaming mode (no buffering)\n\n");

    // Configure GPIO17 (MIC_CH_SEL) as output
    const struct gpio_dt_spec mic_sel = {
        .port = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
        .pin = MIC_CH_SEL_GPIO,
        .dt_flags = GPIO_ACTIVE_HIGH
    };

    if (!device_is_ready(mic_sel.port)) {
        printk("ERROR: GPIO device not ready\n");
        return;
    }

    int ret = gpio_pin_configure_dt(&mic_sel, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("ERROR: Failed to configure GPIO%d: %d\n", MIC_CH_SEL_GPIO, ret);
        return;
    }

    // Set MIC_CH_SEL HIGH
    gpio_pin_set_dt(&mic_sel, 1);
    printk("MIC_CH_SEL (GPIO17) set HIGH\n");

    // Initialize ADC timer sampler
    ret = adc_timer_init(SAMPLE_RATE);
    if (ret != 0) {
        printk("ERROR: ADC timer init failed: %d\n", ret);
        return;
    }

    // Allocate buffer to store ALL samples (no printing during recording)
    int16_t *all_samples = k_malloc(TOTAL_SAMPLES * sizeof(int16_t));
    if (!all_samples) {
        printk("ERROR: Failed to allocate sample buffer (%d samples = %d bytes)\n",
               TOTAL_SAMPLES, TOTAL_SAMPLES * sizeof(int16_t));
        adc_timer_stop();
        return;
    }

    // Start ADC timer NOW - right before recording loop
    ret = adc_timer_start();
    if (ret != 0) {
        printk("ERROR: ADC timer start failed: %d\n", ret);
        k_free(all_samples);
        return;
    }

        printk("*** RECORDING STARTED (collecting %d samples, buffer allocated) ***\n", TOTAL_SAMPLES);

    size_t samples_recorded = 0;

    // Fast recording loop - just read samples into buffer, NO printing
    while (samples_recorded < TOTAL_SAMPLES) {
        size_t available = adc_timer_get_available_count();

        if (available > 0) {
            size_t to_read = TOTAL_SAMPLES - samples_recorded;
            if (to_read > available) {
                to_read = available;
            }

            size_t read = adc_timer_read_samples(&all_samples[samples_recorded], to_read);
            samples_recorded += read;
        } else {
            k_yield();
        }
    }

    printk("*** RECORDING COMPLETE (%d samples captured) ***\n", samples_recorded);

    // Stop ADC timer (will print statistics)
    adc_timer_stop();

    // Now print all samples offline (no time pressure)
    printk("\n=== RAW AUDIO DATA (16-bit PCM, %d Hz, Mono) ===\n", SAMPLE_RATE);
    printk("To convert: python3 hex_to_wav_fixed.py output.txt audio.wav [gain]\n\n");
    printk("BEGIN_RAW_AUDIO_DATA\n");

    uint16_t min_value = 0xFFFF;
    uint16_t max_value = 0;

    for (size_t i = 0; i < samples_recorded; i++) {
        uint16_t val = (uint16_t)all_samples[i];
        printk("%04X\n", val);
        if (val < min_value) min_value = val;
        if (val > max_value) max_value = val;
    }

    printk("END_RAW_AUDIO_DATA\n");
    printk("\nADC value range: min=0x%04X (%u), max=0x%04X (%u)\n",
           min_value, min_value, max_value, max_value);

    printk("\n=== Recording demo complete ===\n");

    k_free(all_samples);
}
