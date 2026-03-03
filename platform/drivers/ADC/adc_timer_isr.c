// Hardware timer ISR implementation for ADC2 triggering

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "driver/gptimer.h"

LOG_MODULE_DECLARE(ulp_adc_sampler, LOG_LEVEL_INF);

// ADC2 registers
#define SENS_SAR_MEAS2_CTRL2_REG_ADDR 0x60008830
#define SENS_MEAS2_START_SAR_BIT 17
#define SENS_MEAS2_DONE_SAR_BIT 16

// Shared memory pointers (initialized by caller)
static volatile uint16_t* timer_shared_adc_value = NULL;
static volatile uint32_t* timer_data_ready_flag = NULL;

// Timer handle
static gptimer_handle_t gptimer = NULL;

// Timer ISR callback - runs at 8 kHz
static bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer, 
                                         const gptimer_alarm_event_data_t *edata, 
                                         void *user_data)
{
    (void)timer;
    (void)edata;
    (void)user_data;
    
    // Trigger ADC2 conversion by toggling START bit
    volatile uint32_t* reg = (volatile uint32_t*)SENS_SAR_MEAS2_CTRL2_REG_ADDR;
    uint32_t reg_val = *reg;
    
    // Clear start bit
    reg_val &= ~(1 << SENS_MEAS2_START_SAR_BIT);
    *reg = reg_val;
    
    // Set start bit to trigger conversion
    reg_val |= (1 << SENS_MEAS2_START_SAR_BIT);
    *reg = reg_val;
    
    // Wait for conversion to complete (DONE bit set)
    // ADC conversion takes ~10us typically
    int timeout = 100;  // Short timeout in ISR
    while (timeout-- > 0) {
        reg_val = *reg;
        if (reg_val & (1 << SENS_MEAS2_DONE_SAR_BIT)) {
            break;
        }
    }
    
    if (timeout > 0) {
        // Conversion completed - extract result (lower 13 bits)
        uint16_t adc_value = (uint16_t)(reg_val & 0x1FFF);
        
        // Write to shared memory for ULP
        *timer_shared_adc_value = adc_value;
        *timer_data_ready_flag = 1;
    }
    
    return false;  // Don't yield from ISR
}

int adc_timer_init(volatile uint16_t* shared_adc_ptr, volatile uint32_t* data_flag_ptr, uint32_t sample_rate_hz)
{
    if (!shared_adc_ptr || !data_flag_ptr || sample_rate_hz == 0) {
        return -EINVAL;
    }
    
    timer_shared_adc_value = shared_adc_ptr;
    timer_data_ready_flag = data_flag_ptr;
    
    // Calculate timer period in microseconds
    uint32_t period_us = 1000000 / sample_rate_hz;
    
    LOG_INF("Initializing GPTimer for ADC2 at %u Hz (%u us period)", sample_rate_hz, period_us);
    
    // Create timer
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,  // 1 MHz resolution (1 us per tick)
    };
    
    esp_err_t ret = gptimer_new_timer(&timer_config, &gptimer);
    if (ret != ESP_OK) {
        LOG_ERR("Failed to create GPTimer: %d", ret);
        return -EIO;
    }
    
    // Set alarm action for periodic triggering
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = period_us,  // Fire every period_us microseconds
        .flags.auto_reload_on_alarm = true,  // Auto-reload for periodic operation
    };
    
    ret = gptimer_set_alarm_action(gptimer, &alarm_config);
    if (ret != ESP_OK) {
        LOG_ERR("Failed to set alarm action: %d", ret);
        gptimer_del_timer(gptimer);
        return -EIO;
    }
    
    // Register ISR callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = timer_on_alarm_cb,
    };
    
    ret = gptimer_register_event_callbacks(gptimer, &cbs, NULL);
    if (ret != ESP_OK) {
        LOG_ERR("Failed to register callbacks: %d", ret);
        gptimer_del_timer(gptimer);
        return -EIO;
    }
    
    // Enable timer
    ret = gptimer_enable(gptimer);
    if (ret != ESP_OK) {
        LOG_ERR("Failed to enable timer: %d", ret);
        gptimer_del_timer(gptimer);
        return -EIO;
    }
    
    LOG_INF("GPTimer initialized successfully");
    return 0;
}

int adc_timer_start(void)
{
    if (!gptimer) {
        LOG_ERR("Timer not initialized");
        return -EINVAL;
    }
    
    esp_err_t ret = gptimer_start(gptimer);
    if (ret != ESP_OK) {
        LOG_ERR("Failed to start timer: %d", ret);
        return -EIO;
    }
    
    LOG_INF("ADC2 timer started");
    return 0;
}

int adc_timer_stop(void)
{
    if (!gptimer) {
        return 0;  // Already stopped or not initialized
    }
    
    esp_err_t ret = gptimer_stop(gptimer);
    if (ret != ESP_OK) {
        LOG_ERR("Failed to stop timer: %d", ret);
        return -EIO;
    }
    
    LOG_INF("ADC2 timer stopped");
    return 0;
}

void adc_timer_deinit(void)
{
    if (gptimer) {
        gptimer_disable(gptimer);
        gptimer_del_timer(gptimer);
        gptimer = NULL;
    }
}
