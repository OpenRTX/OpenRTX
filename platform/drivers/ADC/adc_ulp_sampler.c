/***************************************************************************
 *   Copyright (C) 2026 by OpenRTX                                         *
 *   Hardware timer-based ADC2 sampling with ULP buffering                 *
 ***************************************************************************/

#include "adc_ulp_sampler.h"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ulp_adc_sampler, LOG_LEVEL_INF);

#ifdef CONFIG_ULP_ENABLED

#include "ulp_adc_sampler_syms.h"
#include <driver/rtc_io.h>

// Forward declarations of ESP-IDF ULP functions
extern int ulp_riscv_load_binary(const uint8_t* program_binary, size_t program_size_bytes);
extern int ulp_riscv_run(void);
extern int ulp_set_wakeup_period(size_t period_index, uint32_t period_us);

// Hardware timer functions for ADC2 triggering
extern int adc_timer_init(volatile uint16_t* shared_adc_ptr, volatile uint32_t* data_flag_ptr, uint32_t sample_rate_hz);
extern int adc_timer_start(void);
extern int adc_timer_stop(void);
extern void adc_timer_deinit(void);

#define BUFFER_SIZE 512
#define BUFFER_MASK (BUFFER_SIZE - 1)

// ADC peripheral registers
#define SENS_SAR_PERI_CLK_GATE_CONF_REG 0x60008904
#define SENS_IOMUX_CLK_EN (1 << 31)
#define SENS_SARADC_CLK_EN (1 << 30)
#define SENS_SAR_ATTEN2_REG 0x60008838
#define ADC2_CH4_ATTEN_SHIFT (4 * 2)
#define ADC_ATTEN_DB_11 3

// Pointers to ULP variables in RTC memory
static volatile uint32_t* ulp_write_ptr = ULP_VAR_PTR(ULP_write_ptr_OFFSET);
static volatile uint32_t* ulp_read_ptr = ULP_VAR_PTR(ULP_read_ptr_OFFSET);
static volatile uint32_t* ulp_sample_counter = ULP_VAR_PTR(ULP_sample_counter_OFFSET);
static volatile uint32_t* ulp_overflow_count = ULP_VAR_PTR(ULP_overflow_count_OFFSET);
static volatile uint32_t* ulp_enabled = ULP_VAR_PTR(ULP_enabled_OFFSET);
static volatile uint32_t* ulp_wakeup_counter = ULP_VAR_PTR(ULP_wakeup_counter_OFFSET);
static volatile uint16_t* ulp_adc_buffer = ULP_BUF_PTR(ULP_adc_buffer_OFFSET);
static volatile uint32_t* ulp_missed_samples = ULP_VAR_PTR(ULP_missed_samples_OFFSET);

// Shared memory for CPU-ULP communication
static volatile uint16_t* shared_adc_value_ptr = NULL;
static volatile uint32_t* data_ready_flag_ptr = NULL;

// ULP binary
extern const uint8_t ulp_adc_sampler_bin_start[] __asm__("_binary_ulp_adc_sampler_bin_start");
extern const uint8_t ulp_adc_sampler_bin_end[] __asm__("_binary_ulp_adc_sampler_bin_end");

esp_err_t ulp_adc_sampler_init(void)
{
    LOG_INF("ULP ADC sampler init (CONFIG_ULP_ENABLED is set)");
    
    // Enable ADC peripheral clocks
    uint32_t reg_val = *(volatile uint32_t*)SENS_SAR_PERI_CLK_GATE_CONF_REG;
    reg_val |= SENS_IOMUX_CLK_EN | SENS_SARADC_CLK_EN;
    *(volatile uint32_t*)SENS_SAR_PERI_CLK_GATE_CONF_REG = reg_val;
    LOG_INF("ADC peripheral clocks enabled");
    
    // Enable ADC2 clock (clear SAR2_CLK_GATED)
    reg_val = *(volatile uint32_t*)0x60008900;  // SENS_SAR_PERI_CLK_GATE_CONF_REG
    reg_val &= ~(1 << 6);  // Clear SAR2_CLK_GATED
    *(volatile uint32_t*)0x60008900 = reg_val;
    LOG_INF("ADC2 clock enabled (SAR2_CLK_GATED cleared)");
    
    // Configure ADC2 arbiter for RTC control
    reg_val = *(volatile uint32_t*)0x60008900;
    reg_val |= (1 << 5);  // Set RTC_FORCE
    *(volatile uint32_t*)0x60008900 = reg_val;
    LOG_INF("ADC2 arbiter configured (RTC_FORCE enabled)");
    
    // Enable SAR ADC power domain
    reg_val = *(volatile uint32_t*)0x60008844;  // SENS_SAR_POWER_XPD_SAR_REG
    reg_val |= (1 << 29);  // FORCE_XPD_SAR
    *(volatile uint32_t*)0x60008844 = reg_val;
    LOG_INF("SAR ADC power domain enabled");
    
    // Force ADC2 to RTC control
    reg_val = *(volatile uint32_t*)0x60008830;  // SENS_SAR_MEAS2_CTRL2_REG
    reg_val |= (1 << 31);  // SAR2_RTC_FORCE
    *(volatile uint32_t*)0x60008830 = reg_val;
    LOG_INF("ADC2 forced to RTC control (SAR2_RTC_FORCE set)");
    
    // Disable DAC1 and DAC2 for testing
    reg_val = *(volatile uint32_t*)0x60008838;  // SENS_SAR_ATTEN2_REG
    reg_val |= (1 << 25) | (1 << 24);  // DAC_FORCE bits
    *(volatile uint32_t*)0x60008838 = reg_val;
    LOG_INF("DAC1 and DAC2 disabled for testing");
    
    // Configure GPIO15 as ADC input (remove pullup/pulldown)
    rtc_gpio_init(GPIO_NUM_15);
    rtc_gpio_set_direction(GPIO_NUM_15, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_dis(GPIO_NUM_15);
    rtc_gpio_pulldown_dis(GPIO_NUM_15);
    LOG_INF("GPIO15 configured as ADC input");
    
    // Configure ADC2 CH4 attenuation (11dB for full range)
    reg_val = *(volatile uint32_t*)SENS_SAR_ATTEN2_REG;
    reg_val &= ~(0x3 << ADC2_CH4_ATTEN_SHIFT);
    reg_val |= (ADC_ATTEN_DB_11 << ADC2_CH4_ATTEN_SHIFT);
    *(volatile uint32_t*)SENS_SAR_ATTEN2_REG = reg_val;
    LOG_INF("ADC2 CH4 configured with 11dB attenuation");
    
    // Load ULP binary
    size_t ulp_size = ulp_adc_sampler_bin_end - ulp_adc_sampler_bin_start;
    LOG_INF("Loading ULP binary...");
    LOG_INF("ULP binary present: %zu bytes", ulp_size);
    
    int ret = ulp_riscv_load_binary(ulp_adc_sampler_bin_start, ulp_size);
    if (ret != 0) {
        LOG_ERR("Failed to load ULP binary: %d", ret);
        return ESP_FAIL;
    }
    LOG_INF("ULP binary loaded successfully");
    
    // Initialize pointers to ULP shared variables
    LOG_INF("ULP variables initialized at RTC memory base 0x50000000");
    
    // Initialize shared memory pointers for CPU-ULP communication
    shared_adc_value_ptr = (volatile uint16_t*)(0x50000000 + ULP_shared_adc_value_OFFSET);
    data_ready_flag_ptr = (volatile uint32_t*)(0x50000000 + ULP_data_ready_flag_OFFSET);
    
    return ESP_OK;
}

esp_err_t ulp_adc_sampler_start(uint32_t sample_rate_hz)
{
    if (sample_rate_hz == 0 || sample_rate_hz > 100000) {
        return ESP_ERR_INVALID_ARG;
    }
    
    LOG_INF("Starting ULP sampler at %u Hz", sample_rate_hz);
    
    // Calculate wakeup period in microseconds
    uint32_t period_us = 1000000 / sample_rate_hz;
    LOG_INF("ULP timer period: %u us", period_us);
    
    // Set ULP wakeup period (period_index 0 for RISC-V)
    int ret = ulp_set_wakeup_period(0, period_us);
    if (ret != 0) {
        LOG_ERR("Failed to set wakeup period: %d", ret);
        return ESP_FAIL;
    }
    
    // Start ULP program
    ret = ulp_riscv_run();
    if (ret != 0) {
        LOG_ERR("Failed to start ULP: %d", ret);
        return ESP_FAIL;
    }
    
    LOG_INF("Enabling ULP sampler...");
    
    // Initialize and start hardware timer for precise ADC2 triggering
    int timer_ret = adc_timer_init(shared_adc_value_ptr, data_ready_flag_ptr, sample_rate_hz);
    if (timer_ret != 0) {
        LOG_ERR("Failed to initialize ADC timer: %d", timer_ret);
        return ESP_FAIL;
    }
    
    timer_ret = adc_timer_start();
    if (timer_ret != 0) {
        LOG_ERR("Failed to start ADC timer: %d", timer_ret);
        return ESP_FAIL;
    }
    LOG_INF("ADC2 hardware timer started");
    
    // Enable sampling by setting the enabled flag
    *ulp_enabled = 1;
    
    LOG_INF("ULP sampler started successfully");
    return ESP_OK;
}

void ulp_adc_sampler_stop(void)
{
    LOG_INF("ULP sampler stop");
    
    // Stop hardware timer
    adc_timer_stop();
    LOG_INF("ADC2 timer stopped");
    
    // Stop sampling by clearing the enabled flag
    *ulp_enabled = 0;
}

size_t ulp_adc_read_samples(int16_t *dest, size_t max_count)
{
    if (!dest || max_count == 0) {
        return 0;
    }
    
    // Read current pointers
    uint32_t write = *ulp_write_ptr;
    uint32_t read = *ulp_read_ptr;
    
    // Calculate available samples
    size_t available = (write - read) & BUFFER_MASK;
    
    // Limit to requested count
    size_t to_read = (available < max_count) ? available : max_count;
    
    if (to_read == 0) {
        return 0;
    }
    
    // Read samples from ring buffer
    for (size_t i = 0; i < to_read; i++) {
        dest[i] = (int16_t)ulp_adc_buffer[read];
        read = (read + 1) & BUFFER_MASK;
    }
    
    // Update read pointer
    *ulp_read_ptr = read;
    
    return to_read;
}

size_t ulp_adc_get_available_count(void)
{
    uint32_t write = *ulp_write_ptr;
    uint32_t read = *ulp_read_ptr;
    
    return (write - read) & BUFFER_MASK;
}

uint32_t ulp_adc_get_overflow_count(void)
{
    return *ulp_overflow_count;
}

void ulp_adc_reset_overflow_count(void)
{
    *ulp_overflow_count = 0;
}

uint32_t ulp_adc_get_wakeup_counter(void)
{
    return *ulp_wakeup_counter;
}

uint32_t ulp_adc_get_sample_counter(void)
{
    return *ulp_sample_counter;
}

uint32_t ulp_adc_get_write_ptr(void)
{
    return *ulp_write_ptr;
}

uint32_t ulp_adc_get_read_ptr(void)
{
    return *ulp_read_ptr;
}

uint32_t ulp_adc_get_missed_samples(void)
{
    return *ulp_missed_samples;
}

#else  // CONFIG_ULP_ENABLED not defined

esp_err_t ulp_adc_sampler_init(void)
{
    LOG_WRN("ULP ADC sampler not enabled (CONFIG_ULP_ENABLED not set)");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ulp_adc_sampler_start(uint32_t sample_rate_hz)
{
    (void)sample_rate_hz;
    return ESP_ERR_NOT_SUPPORTED;
}

void ulp_adc_sampler_stop(void)
{
}

size_t ulp_adc_read_samples(int16_t *dest, size_t max_count)
{
    (void)dest; (void)max_count;
    return 0;
}

size_t ulp_adc_get_available_count(void)
{
    return 0;
}

uint32_t ulp_adc_get_overflow_count(void)
{
    return 0;
}

void ulp_adc_reset_overflow_count(void)
{
}

uint32_t ulp_adc_get_wakeup_counter(void)
{
    return 0;
}

uint32_t ulp_adc_get_sample_counter(void)
{
    return 0;
}

uint32_t ulp_adc_get_write_ptr(void)
{
    return 0;
}

uint32_t ulp_adc_get_read_ptr(void)
{
    return 0;
}

uint32_t ulp_adc_get_missed_samples(void)
{
    return 0;
}

#endif  // CONFIG_ULP_ENABLED
