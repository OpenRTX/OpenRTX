/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

typedef int16_t adc_data_size_t;
#define INVALID_ADC_VALUE INT_MIN

#if CONFIG_NOCACHE_MEMORY
#define __NOCACHE	__attribute__((__section__(".nocache")))
#else /* CONFIG_NOCACHE_MEMORY */
#define __NOCACHE
#endif /* CONFIG_NOCACHE_MEMORY */

#define BUFFER_SIZE  8
#ifdef CONFIG_TEST_USERSPACE
static ZTEST_BMEM adc_data_size_t m_sample_buffer[BUFFER_SIZE];
#else
static __aligned(32) adc_data_size_t m_sample_buffer[BUFFER_SIZE] __NOCACHE;
#endif

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};
static const int adc_channels_count = ARRAY_SIZE(adc_channels);

const struct device *get_adc_device(void)
{
	if (!adc_is_ready_dt(&adc_channels[0])) {
		printk("ADC device is not ready\n");
		return NULL;
	}

	return adc_channels[0].dev;
}

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(test_counter)) && \
	defined(CONFIG_COUNTER)
static void init_counter(void)
{
	int err;
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(test_counter));
	struct counter_top_cfg top_cfg = { .callback = NULL,
					   .user_data = NULL,
					   .flags = 0 };

	__ASSERT(device_is_ready(dev), "Counter device is not ready");

	counter_start(dev);
	top_cfg.ticks = counter_us_to_ticks(dev, CONFIG_ADC_API_SAMPLE_INTERVAL_US);
	err = counter_set_top_value(dev, &top_cfg);
	__ASSERT(err == 0, "%s: Counter failed to set top value (err: %d)", dev->name, err);
}
#endif

static void init_adc(void)
{
	int i, ret;

	__ASSERT(adc_is_ready_dt(&adc_channels[0]), "ADC device is not ready");

	for (i = 0; i < adc_channels_count; i++) {
		ret = adc_channel_setup_dt(&adc_channels[i]);
		__ASSERT(ret == 0, "Setting up of channel %d failed with code %d", i, ret);
	}

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(test_counter)) && \
	defined(CONFIG_COUNTER)
	init_counter();
#endif
}

static void check_samples(int expected_count)
{
	int i;

	printk("Samples read: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		adc_data_size_t sample_value = m_sample_buffer[i];

#if CONFIG_ADC_32_BITS_DATA
		printk("0x%08x ", sample_value);
#else
		printk("0x%04hx ", sample_value);
#endif
		if (i < expected_count) {
			__ASSERT(INVALID_ADC_VALUE != sample_value,
				"[%u] should be filled", i);
		}
		// else {
		// 	__ASSERT(INVALID_ADC_VALUE == sample_value,
		// 		"[%u] should be empty", i);
		// }
	}
	printk("\n");
}

/*
 * test_adc_sample_one_channel
 */
static int test_task_one_channel(void)
{
	int ret;
	struct adc_sequence sequence = {
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	printk("Initializing adc...");
	init_adc();
	printk("done!\n");
	printk("Initializing sequence adc...");
	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);
	printk("done!\n");

	printk("Reading from adc...");
	ret = adc_read_dt(&adc_channels[0], &sequence);
	__ASSERT(ret == 0, "adc_read() failed with code %d", ret);
	printk("done!\n");

	printk("Checking samples...");
	check_samples(1);
	printk("done!\n");

	return 0;
}

/*
 * test_adc_sample_with_interval
 */
static uint32_t my_sequence_identifier = 0x12345678;
static void *user_data = &my_sequence_identifier;

static enum adc_action sample_with_interval_callback(const struct device *dev,
						     const struct adc_sequence *sequence,
						     uint16_t sampling_index)
{
	if (sequence->options->user_data != &my_sequence_identifier) {
		user_data = sequence->options->user_data;
		return ADC_ACTION_FINISH;
	}

	printk("%s: sampling %d\n", __func__, sampling_index);
	return ADC_ACTION_CONTINUE;
}

static int test_task_with_interval(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.interval_us     = 52UL, // 4 x M17 symbol rate
		.callback        = NULL,
		.user_data       = user_data,
		.extra_samplings = 0,
	};
	struct adc_sequence sequence = {
		.options     = &options,
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	printk("Initializing adc...");
	init_adc();
	printk("done!\n");

	printk("Initializing sequence adc...");
	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);
	printk("done!\n");

	printk("Reading from adc...");
	ret = adc_read_dt(&adc_channels[0], &sequence);
	__ASSERT(ret == 0, "adc_read() failed with code %d", ret);
	printk("done!\n");

	printk("Checking samples...");
	__ASSERT(user_data == sequence.options->user_data,
		"Invalid user data: %p, expected: %p",
		user_data, sequence.options->user_data);

	check_samples(1 + options.extra_samplings);
	printk("done!\n");

	return 0;
}

int start_adc_test(void)
{
	// int res = test_task_one_channel();
	// printk("test_task_one_channel(): %d\n", res);
	int res = test_task_with_interval();
	printk("test_task_with_interval(): %d\n", res);

	return 0;
}
