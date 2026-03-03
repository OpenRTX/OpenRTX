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

// ULP ADC Sampler - 5 Second Mic Recording Demo (Streaming)
// Records microphone input and outputs RAW audio for Audacity
#include "drivers/ADC/adc_ulp_sampler.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define MIC_CH_SEL_GPIO 17  // GPIO17 controls mic channel select
#define SAMPLE_RATE 8000    // 8 kHz sample rate
#define RECORDING_SECONDS 5
#define TOTAL_SAMPLES (SAMPLE_RATE * RECORDING_SECONDS)  // 40,000 samples
#define CHUNK_SIZE 256  // Read 256 samples at a time

static void ulp_demo_thread(void *, void *, void *);
// CPU-based ADC test
extern void adc_cpu_busy_loop_test(void);

extern void adc_cpu_busy_loop_test(void);
K_THREAD_DEFINE(ulp_demo_tid, 4096, ulp_demo_thread, NULL, NULL, NULL, 7, 0, 0);

static void ulp_demo_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    // Wait for system to stabilize
    k_sleep(K_SECONDS(2));

    printk("\n=== ULP ADC Sampler - Mic Recording Demo ===\n");
    printk("Recording %d seconds of audio at %d Hz\n", RECORDING_SECONDS, SAMPLE_RATE);
    printk("Total samples: %d\n", TOTAL_SAMPLES);
    printk("Streaming mode (no buffering)\n\n");

    // Configure GPIO17 (MIC_CH_SEL) as output and set HIGH
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

    // Set MIC_CH_SEL HIGH (active)
    gpio_pin_set_dt(&mic_sel, 1);
    printk("MIC_CH_SEL (GPIO17) set HIGH\n");

    // Initialize ULP sampler
    esp_err_t err = ulp_adc_sampler_init();
    if (err != ESP_OK) {
        printk("ERROR: ULP sampler init failed: %d\n", err);
        return;
    }

    // Start ULP sampler at 8 kHz
    err = ulp_adc_sampler_start(SAMPLE_RATE);
    if (err != ESP_OK) {
        printk("ERROR: ULP sampler start failed: %d\n", err);
        return;
    }

    printk("ULP sampler started at %d Hz\n", SAMPLE_RATE);
    printk("Recording will start in 1 second...\n\n");
    
    // Debug: Read ULP variables
    k_sleep(K_MSEC(500));
    printk("\n=== ULP Debug Info ===\n");
    volatile uint32_t *last_raw = (volatile uint32_t*)(0x50000000 + 0x510);
    volatile uint32_t *conv_fail = (volatile uint32_t*)(0x50000000 + 0x50c);
    volatile uint32_t *wakeup_cnt = (volatile uint32_t*)(0x50000000 + 0x514);
    volatile uint32_t *sample_cnt = (volatile uint32_t*)(0x50000000 + 0x520);
    printk("Last raw ADC register: 0x%08X\n", *last_raw);
    printk("Conversion failures: %u\n", *conv_fail);
    printk("Wakeup counter: %u\n", *wakeup_cnt);
    printk("Sample counter: %u\n", *sample_cnt);
    printk("======================\n\n");
    
    k_sleep(K_SECONDS(1));

    // Allocate small chunk buffer for streaming
    int16_t *chunk = k_malloc(CHUNK_SIZE * sizeof(int16_t));
    if (!chunk) {
        printk("ERROR: Failed to allocate chunk buffer\n");
        return;
    }

    printk("*** RECORDING STARTED ***\n");
    printk("\n=== RAW AUDIO DATA (16-bit signed PCM, %d Hz, Mono) ===\n", SAMPLE_RATE);
    printk("To import in Audacity:\n");
    printk("1. Copy data between BEGIN/END markers to audio.hex\n");
    printk("2. Convert: for line in file: value=int(line,16); write(value.to_bytes(2,'little'))\n");
    printk("3. Import Raw: Signed 16-bit PCM, Little-endian, Mono, %d Hz\n\n", SAMPLE_RATE);

    printk("BEGIN_RAW_AUDIO_DATA\n");

    uint32_t samples_recorded = 0;
    uint16_t min_value = 0xFFFF;
    uint16_t max_value = 0;
    uint32_t start_time = k_uptime_get_32();

    // Record for 5 seconds - stream data directly
    while (samples_recorded < TOTAL_SAMPLES) {
        // Read available samples
        size_t available = ulp_adc_get_available_count();
        if (available > 0) {
            size_t to_read = TOTAL_SAMPLES - samples_recorded;
            if (to_read > CHUNK_SIZE) to_read = CHUNK_SIZE;
            if (to_read > available) to_read = available;

            size_t read = ulp_adc_read_samples(chunk, to_read);
            
            // Stream samples immediately
            for (size_t i = 0; i < read; i++) {
                printk("%04X\n", (uint16_t)chunk[i]);
                uint16_t val = (uint16_t)chunk[i];
                if (val < min_value) min_value = val;
                if (val > max_value) max_value = val;
            }
            
            samples_recorded += read;
        } else {
            k_sleep(K_USEC(100));  // Short sleep if no samples
        }
    }

    uint32_t elapsed_ms = k_uptime_get_32() - start_time;

    printk("END_RAW_AUDIO_DATA\n");
    printk("\n*** RECORDING COMPLETE ***\n");
    printk("Recorded %u samples in %u ms\n", samples_recorded, elapsed_ms);
    printk("Actual sample rate: %u Hz\n", (unsigned int)((samples_recorded * 1000) / elapsed_ms));
    printk("ADC value range: min=0x%04X (%u), max=0x%04X (%u)\n", min_value, min_value, max_value, max_value);

    // Debug: Print ULP counters after recording
    printk("\n=== ULP Debug Info (After Recording) ===\n");
    printk("ULP wakeup counter: %u\n", ulp_adc_get_wakeup_counter());
    printk("ULP sample counter: %u\n", ulp_adc_get_sample_counter());
    printk("Write ptr: %u, Read ptr: %u\n", ulp_adc_get_write_ptr(), ulp_adc_get_read_ptr());
    printk("Overflow count: %u\n", ulp_adc_get_overflow_count());
    printk("Missed samples: %u\n", ulp_adc_get_missed_samples());
    printk("=========================================\n\n");
    printk("\n=== Recording demo complete ===\n");

    // Stop ULP sampler
    ulp_adc_sampler_stop();

    k_free(chunk);
}
