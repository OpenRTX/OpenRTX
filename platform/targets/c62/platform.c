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

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <interfaces/audio.h>
#include <math.h>


// Reference the GPIO nodes
static const struct gpio_dt_spec speaker_enable = GPIO_DT_SPEC_GET(DT_PATH(gpio_controls, speaker_enable), gpios);
static const struct gpio_dt_spec dtmf_enable = GPIO_DT_SPEC_GET(DT_PATH(gpio_controls, dtmf_enable), gpios);
static const struct gpio_dt_spec button_ptt = GPIO_DT_SPEC_GET_OR(DT_NODELABEL(button_ptt), gpios, {0});

static const struct gpio_dt_spec led_white = GPIO_DT_SPEC_GET(DT_ALIAS(ledwhite), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(ledgreen), gpios);
static const struct gpio_dt_spec led_keyboard = GPIO_DT_SPEC_GET(DT_NODELABEL(ledkeyboard), gpios);

static const struct pwm_dt_spec pwm_lcd_backlight = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_lcd_backlight));
static const struct pwm_dt_spec pwm_rf_apc = PWM_DT_SPEC_GET(DT_NODELABEL(pwm_rf_apc));

const struct device *radio = DEVICE_DT_GET(DT_NODELABEL(bk4819_radio));
const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));


static hwInfo_t hwInfo =
{
    .name        = "c62",
    .hw_version  = 0,
    .uhf_band    = 1,
    .vhf_band    = 1,
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
	.channels =  BIT(2) ,  // BIT(2) Channel 2 (BAT_DET), BIT(1) Channel 1 (Charge detection)
	.buffer = adc_raw_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(adc_raw_buffer),
	.resolution = 11,  // 11-bit resolution
};


static int battery_init(void);


/**
 * Set display brightness using PWM on pin A02
 * PWM Period: 1000us (1kHz), adjustable duty cycle for brightness
 * 
 * @param brightness_percent: 0-100 (0 = off, 100 = full brightness)
 */
void platform_set_display_brightness(uint8_t brightness_percent)
{
    // Calculate duty cycle (1000us period = 1kHz)
    uint32_t period_us = 1000;  // 1kHz PWM frequency
	uint32_t adjusted_brightness_percent = 0;

	if (brightness_percent > 0) {
		// Apply non-linear mapping for better low-end brightness control
		adjusted_brightness_percent = roundf(brightness_percent * brightness_percent / 105.0) + 5;
	} 

    uint32_t pulse_us = (period_us * adjusted_brightness_percent) / 100;
    
    int ret = pwm_set_dt(&pwm_lcd_backlight, PWM_USEC(period_us), PWM_USEC(pulse_us));
    
    if (ret < 0) {
        printk("Failed to set display PWM: %d\n", ret);
    } else {
        printk("Display brightness set to %d%% (PWM %d%%)\n", brightness_percent, adjusted_brightness_percent);
    }
}


/**
 * Set TX power using PWM on pin A03
 * PWM Period: 1000us (1kHz), adjustable duty cycle for power level
 * 
 * @param power_percent: 0-100 (0 = min power, 100 = max power)
 */
void platform_set_tx_power(uint8_t power_percent)
{
	printk("Requested TX power: %d%%\n", power_percent);

	//TODO: Check if this works... May need to adjust mapping of power_percent to duty cycle based on actual power output
	//      vs duty cycle curve of the PA, and also consider frequency dependence of PA efficiency
    
    // Calculate duty cycle (1000us period = 1kHz)
    uint32_t period_us = 1000;  // 1kHz PWM frequency
    uint32_t pulse_us = (period_us * power_percent) / 100;
    
    int ret = pwm_set_dt(&pwm_rf_apc, PWM_USEC(period_us), PWM_USEC(pulse_us));
    
    if (ret < 0) {
        printk("Failed to set TX power PWM: %d\n", ret);
    } else {
        printk("TX power set to %d%%\n", power_percent);
    }
}


void platform_init_csk6()
{
    printk("0x49 de OE3ANC from OPENRTX on the C62\n");    
    
    // Configure the PTT key as input with pull-up
    gpio_pin_configure_dt(&button_ptt, GPIO_INPUT);

	// Configure light pins as outputs and set initial state
    gpio_pin_configure_dt(&led_white, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_keyboard, GPIO_OUTPUT_INACTIVE);

    // Configure speaker enable (don't set it as output if you want to use SWD debugging!)
    gpio_pin_configure_dt(&speaker_enable, GPIO_OUTPUT);

    gpio_pin_configure_dt(&dtmf_enable, GPIO_OUTPUT);


	// DT_EN (Audio routing control for BK4819): 0 = DSP left channel output to AMP, 1 = DTMF from BK4819 to AMP
    gpio_pin_set_dt(&dtmf_enable, 0);     // DSP left channel output to amplifier

	// Enable keyboard backlight
	gpio_pin_set_dt(&led_keyboard, 1);

	/* Initialise audio */
	audio_init();

	/* Init ADC for Battery reading */
	battery_init();

    //Activate to test Broadcast FM Radio (92.2MHz)
    //BK1080_Init(92200000, 0);
	//gpio_pin_set_dt(&speaker_enable, 1); 

	// Test beep
	//BK4819_BeepStart(500, true);
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
        .channel_id = 2,  // ADC2 channel (pin B08)
    };
    
    int ret = adc_channel_setup(adc_dev, &channel_cfg);
    if (ret < 0) {
        printk("Failed to setup ADC channel: %d\n", ret);
        return -2;
    }

	channel_cfg.channel_id = 1;  // ADC1 channel (Charge detection)

	ret = adc_channel_setup(adc_dev, &channel_cfg);
    if (ret < 0) {
        printk("Failed to setup ADC channel: %d\n", ret);
        return -2;
    }
	
	// Read ADC reference voltage if supported
	adc_vref = adc_ref_internal(adc_dev);
	if (adc_vref > 0) {
		printk("ADC reference voltage: %d mV\n", adc_vref);
	} else {
		printk("ADC reference voltage not available\n");
	}
	
	return 0;
}


/**
 * Read battery voltage using ADC2 on pin B08
 * @return: Battery voltage in millivolts (mV), or 0 on error
 */
uint16_t platform_getVbat()
{
	#if 0
	sequence.channels = BIT(1); // Read ADC1 channel for charge detection
    // Read ADC value
    int ret = adc_read(adc_dev, &sequence);
    if (ret < 0) {
        printk("Failed to read ADC: %d\n", ret);
        return 0;
    }

	int32_t raw_value = adc_raw_buffer[0];

	int32_t mv_value = raw_value - 2048;
	adc_raw_to_millivolts(adc_vref, ADC_GAIN_1, 11 /* adb resolution */, &mv_value);
	printk(" adc1(%d) = %d mV  ", raw_value, mv_value);

	if (raw_value > 2100) {   // TODO: Adjust threshold based on actual measurements of charge detection pin voltage when charging vs not charging
		printf("Charging\n");
	} else {
		printf("Not charging\n");
	}


	sequence.channels = BIT(2); // Switch to ADC2 for battery voltage reading
	    // Read ADC value
    ret = adc_read(adc_dev, &sequence);
    if (ret < 0) {
        printk("Failed to read ADC: %d\n", ret);
        return 0;
    }

	raw_value = adc_raw_buffer[0];
	
	mv_value = raw_value - 2048; // Adjust for 11-bit ADC with bipolar range (-2048 to 2047)
	adc_raw_to_millivolts(adc_vref, ADC_GAIN_1, 11 /* adb resolution */, &mv_value);
	printk(" adc2(%d) = %d mV  ", raw_value, mv_value);

    
    // Convert ADC value to voltage (assuming 3.3V reference, 11-bit ADC)
    uint32_t battery_voltage_mv = ((uint32_t)adc_raw_buffer[0] * 3300) / 4096;  // 2048 = 2^11 for 11-bit resolution (0-2047) / 12-bit for negative values (-2048 to 2047)
    
    // Adapt for voltage divider 200K and 100K
    battery_voltage_mv = battery_voltage_mv * 3;
    
    printk("Battery ADC raw: %d, Battery: %d mV\n", adc_raw_buffer[0], battery_voltage_mv);

    return (uint16_t)voltage_mv;
#else
	return 7200; // Return fixed value until voltage reading is working reliably
#endif
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
    return true;
}

void platform_ledOn(led_t led)
{
    switch(led) {
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
    switch(led) {
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
    BK4819_BeepStart(freq, true);
}

void platform_beepStop()
{
    BK4819_BeepStop();
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
