// ADC2 Timer - Direct adc_read() from ISR (oneshot mode is fast)
#include "adc_esp32.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_timer, LOG_LEVEL_INF);

#define ADC_DEVICE_NODE DT_NODELABEL(adc1)
#define ADC_CHANNEL 4
#define ADC_RESOLUTION 12
#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT

#define BUFFER_SIZE 512
#define BUFFER_MASK (BUFFER_SIZE - 1)

static const struct device *adc_dev = DEVICE_DT_GET(ADC_DEVICE_NODE);

static uint16_t adc_ring_buffer[BUFFER_SIZE];
static volatile uint32_t write_ptr = 0;
static volatile uint32_t read_ptr = 0;
static volatile uint32_t sample_counter = 0;
static volatile uint32_t overflow_count = 0;
static volatile uint32_t isr_call_count = 0;
static volatile uint32_t conversion_timeout_count = 0;

static struct k_timer adc_timer;
static uint32_t timer_period_us = 0;

static int16_t adc_sample_buffer[1];
static struct adc_sequence adc_sequence = {
    .buffer = adc_sample_buffer,
    .buffer_size = sizeof(adc_sample_buffer),
};

// Timer callback - Call adc_read() directly (oneshot mode is fast!)
static void timer_expiry_function(struct k_timer *timer_id)
{
    isr_call_count++;

    // Direct call to adc_read() - in oneshot mode this is just:
    // adc_oneshot_hal_setup() + adc_oneshot_hal_convert()
    // Both are fast register operations
    int ret = adc_read(adc_dev, &adc_sequence);

    if (ret == 0) {
        uint16_t adc_value = (uint16_t)adc_sample_buffer[0];
        adc_ring_buffer[write_ptr] = adc_value;
        uint32_t next_write = (write_ptr + 1) & BUFFER_MASK;

        if (next_write == read_ptr) {
            overflow_count++;
        } else {
            write_ptr = next_write;
            sample_counter++;
        }
    } else {
        conversion_timeout_count++;
    }
}

int adc_timer_init(uint32_t sample_rate_hz)
{
    int err;

    timer_period_us = 1000000 / sample_rate_hz;

    LOG_INF("ADC2 sampler init (Direct ISR adc_read)");

    if (!device_is_ready(adc_dev)) {
        LOG_ERR("ADC device not ready");
        return -ENODEV;
    }

    LOG_INF("ADC device ready");

    struct adc_channel_cfg channel_cfg = {
        .gain = ADC_GAIN,
        .reference = ADC_REFERENCE,
        .acquisition_time = ADC_ACQUISITION_TIME,
        .channel_id = ADC_CHANNEL,
        .differential = 0,
    };

    err = adc_channel_setup(adc_dev, &channel_cfg);
    if (err < 0) {
        LOG_ERR("Failed to configure ADC channel %d: %d", ADC_CHANNEL, err);
        return err;
    }

    LOG_INF("ADC2 CH%d configured with gain=%d (11dB atten)", ADC_CHANNEL, ADC_GAIN);

    adc_sequence.channels = BIT(ADC_CHANNEL);
    adc_sequence.resolution = ADC_RESOLUTION;

    LOG_INF("ADC sequence configured: channel=%d, resolution=%d bits",
            ADC_CHANNEL, ADC_RESOLUTION);

    write_ptr = 0;
    read_ptr = 0;
    sample_counter = 0;
    overflow_count = 0;
    isr_call_count = 0;
    conversion_timeout_count = 0;

    k_timer_init(&adc_timer, timer_expiry_function, NULL);

    LOG_INF("Timer initialized at %u Hz (period %u us), direct ISR adc_read()",
            sample_rate_hz, timer_period_us);
    return 0;
}

int adc_timer_start(void)
{
    if (timer_period_us == 0) {
        LOG_ERR("Timer not initialized");
        return -EINVAL;
    }

    LOG_INF("Starting ADC2 sampler at %u Hz", 1000000 / timer_period_us);
    k_timer_start(&adc_timer, K_USEC(timer_period_us), K_USEC(timer_period_us));
    LOG_INF("ADC2 sampler started successfully");
    return 0;
}

int adc_timer_stop(void)
{
    k_timer_stop(&adc_timer);

    LOG_INF("Stopping ADC2 sampler");
    LOG_INF("=== Statistics ===");
    LOG_INF("Total samples: %u, Overflows: %u", sample_counter, overflow_count);
    LOG_INF("Timer ISR calls: %u, Read failures: %u", isr_call_count, conversion_timeout_count);

    return 0;
}

void adc_timer_deinit(void)
{
    k_timer_stop(&adc_timer);
    timer_period_us = 0;
}

size_t adc_timer_read_samples(int16_t *dest, size_t max_count)
{
    if (!dest || max_count == 0) return 0;

    uint32_t write = write_ptr;
    uint32_t read = read_ptr;
    size_t available = (write - read) & BUFFER_MASK;
    size_t to_read = (available < max_count) ? available : max_count;

    if (to_read == 0) return 0;

    for (size_t i = 0; i < to_read; i++) {
        dest[i] = (int16_t)adc_ring_buffer[read];
        read = (read + 1) & BUFFER_MASK;
    }

    read_ptr = read;
    return to_read;
}

size_t adc_timer_get_available_count(void)
{
    uint32_t write = write_ptr;
    uint32_t read = read_ptr;
    return (write - read) & BUFFER_MASK;
}

uint32_t adc_timer_get_sample_counter(void) { return sample_counter; }
uint32_t adc_timer_get_overflow_count(void) { return overflow_count; }
void adc_timer_reset_overflow_count(void) { overflow_count = 0; }
uint32_t adc_timer_get_isr_call_count(void) { return isr_call_count; }
uint32_t adc_timer_get_timeout_count(void) { return conversion_timeout_count; }
