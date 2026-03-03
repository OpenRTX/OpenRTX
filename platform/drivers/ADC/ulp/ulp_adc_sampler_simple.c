/***************************************************************************
 *   Copyright (C) 2026 by OpenRTX                                         *
 *   ULP RISC-V ADC Sampler - CPU writes, ULP buffers                      *
 ***************************************************************************/

#include <stdint.h>

#define BUFFER_SIZE 512
#define BUFFER_MASK (BUFFER_SIZE - 1)

// ESP32-S3 ULP control registers
#define RTC_CNTL_COCPU_CTRL_REG 0x60008104
#define RTC_CNTL_COCPU_DONE (1 << 25)
#define RTC_CNTL_COCPU_SHUT_RESET_EN (1 << 22)
#define RTC_CNTL_COCPU_SHUT_2_CLK_DIS_SHIFT 14
#define RTC_CNTL_COCPU_SHUT_2_CLK_DIS_MASK (0xFF << 14)

// Shared variables in RTC memory
volatile uint16_t adc_buffer[BUFFER_SIZE] = {0};
volatile uint32_t write_ptr = 0;
volatile uint32_t read_ptr = 0;
volatile uint32_t sample_counter = 0;
volatile uint32_t overflow_count = 0;
volatile uint32_t enabled = 0;
volatile uint32_t wakeup_counter = 0;

// CPU-to-ULP communication
volatile uint16_t shared_adc_value = 0;
volatile uint32_t data_ready_flag = 0;
volatile uint32_t missed_samples = 0;

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

int main(void) {
    ulp_rescue_from_monitor();
    wakeup_counter++;
    
    if (!enabled) {
        ulp_halt();
        return 0;
    }

    // Check if CPU has written new data
    if (data_ready_flag) {
        // Read the value the CPU prepared for us
        uint16_t sample = shared_adc_value;
        
        // Clear the flag to signal we consumed it
        data_ready_flag = 0;
        
        // Store in ring buffer
        adc_buffer[write_ptr] = sample;
        uint32_t next_write = (write_ptr + 1) & BUFFER_MASK;

        if (next_write == read_ptr) {
            overflow_count++;
        } else {
            write_ptr = next_write;
            sample_counter++;
        }
    } else {
        // CPU didn't have data ready - missed sample
        missed_samples++;
    }

    ulp_halt();
    return 0;
}
