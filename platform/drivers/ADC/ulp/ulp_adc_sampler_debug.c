/***************************************************************************
 *   Copyright (C) 2026 by OpenRTX                                         *
 *   ULP RISC-V ADC Sampler - Debug version with register dumps            *
 ***************************************************************************/

#include <stdint.h>

#define BUFFER_SIZE 512
#define BUFFER_MASK (BUFFER_SIZE - 1)

// ESP32-S3 Register addresses
#define RTC_CNTL_COCPU_CTRL_REG 0x60008104
#define RTC_CNTL_COCPU_DONE (1 << 25)
#define RTC_CNTL_COCPU_SHUT_RESET_EN (1 << 22)
#define RTC_CNTL_COCPU_SHUT_2_CLK_DIS_SHIFT 14
#define RTC_CNTL_COCPU_SHUT_2_CLK_DIS_MASK (0xFF << 14)

// ADC2 registers
#define SENS_BASE 0x60008800
#define SENS_SAR_MEAS2_CTRL2_REG (SENS_BASE + 0x30)
#define SENS_MEAS2_START_SAR_BIT 17
#define SENS_MEAS2_DONE_SAR_BIT 16

// Shared variables
volatile uint16_t adc_buffer[BUFFER_SIZE] = {0};
volatile uint32_t write_ptr = 0;
volatile uint32_t read_ptr = 0;
volatile uint32_t sample_counter = 0;
volatile uint32_t overflow_count = 0;
volatile uint32_t enabled = 0;
volatile uint32_t wakeup_counter = 0;
volatile uint32_t last_raw_value = 0;  // Debug: store raw register value
volatile uint32_t conversion_failures = 0;  // Debug: count failures

static inline void write_reg(uint32_t addr, uint32_t val) {
    *(volatile uint32_t*)addr = val;
}

static inline uint32_t read_reg(uint32_t addr) {
    return *(volatile uint32_t*)addr;
}

static void ulp_rescue_from_monitor(void) {
    uint32_t reg_val = read_reg(RTC_CNTL_COCPU_CTRL_REG);
    reg_val &= ~(RTC_CNTL_COCPU_DONE | RTC_CNTL_COCPU_SHUT_RESET_EN);
    write_reg(RTC_CNTL_COCPU_CTRL_REG, reg_val);
}

static void ulp_halt(void) {
    uint32_t reg_val;
    
    reg_val = read_reg(RTC_CNTL_COCPU_CTRL_REG);
    reg_val &= ~RTC_CNTL_COCPU_SHUT_2_CLK_DIS_MASK;
    reg_val |= (0x3F << RTC_CNTL_COCPU_SHUT_2_CLK_DIS_SHIFT);
    write_reg(RTC_CNTL_COCPU_CTRL_REG, reg_val);
    
    reg_val = read_reg(RTC_CNTL_COCPU_CTRL_REG);
    reg_val |= (RTC_CNTL_COCPU_DONE | RTC_CNTL_COCPU_SHUT_RESET_EN);
    write_reg(RTC_CNTL_COCPU_CTRL_REG, reg_val);
    
    while (1) {
        __asm__ volatile ("nop");
    }
}

// Simplified ADC read - just read the register value directly
static int32_t read_adc2_ch4(void) {
    uint32_t reg_val;
    
    // Read current register value
    reg_val = read_reg(SENS_SAR_MEAS2_CTRL2_REG);
    last_raw_value = reg_val;  // Store for debugging
    
    // Clear previous channel selection (bits 30:19)
    reg_val &= ~(0xFFF << 19);
    // Select channel 4 (bit 23 for channel 4)
    reg_val |= (1 << (19 + 4));
    write_reg(SENS_SAR_MEAS2_CTRL2_REG, reg_val);
    
    // Start conversion (toggle bit 17)
    reg_val &= ~(1 << SENS_MEAS2_START_SAR_BIT);
    write_reg(SENS_SAR_MEAS2_CTRL2_REG, reg_val);
    reg_val |= (1 << SENS_MEAS2_START_SAR_BIT);
    write_reg(SENS_SAR_MEAS2_CTRL2_REG, reg_val);
    
    // Wait for done (bit 16), with timeout
    int timeout = 10000;
    while (timeout-- > 0) {
        reg_val = read_reg(SENS_SAR_MEAS2_CTRL2_REG);
        if (reg_val & (1 << SENS_MEAS2_DONE_SAR_BIT)) {
            break;
        }
    }
    
    if (timeout <= 0) {
        conversion_failures++;
        return -1;
    }
    
    // Read result - data is in bits [12:0] (13 bits for 12-bit ADC + status)
    reg_val = read_reg(SENS_SAR_MEAS2_CTRL2_REG);
    last_raw_value = reg_val;  // Store raw value
    
    // Extract ADC data (bits 12:0)
    return (int32_t)(reg_val & 0x1FFF);
}

int main(void) {
    ulp_rescue_from_monitor();
    wakeup_counter++;
    
    if (!enabled) {
        ulp_halt();
        return 0;
    }

    int32_t sample = read_adc2_ch4();

    if (sample >= 0) {
        adc_buffer[write_ptr] = (uint16_t)sample;
        uint32_t next_write = (write_ptr + 1) & BUFFER_MASK;

        if (next_write == read_ptr) {
            overflow_count++;
        } else {
            write_ptr = next_write;
            sample_counter++;
        }
    }

    ulp_halt();
    return 0;
}
