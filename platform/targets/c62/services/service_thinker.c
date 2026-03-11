/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lsf_thinker_service

#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lsf_thinker, CONFIG_LSF_LOG_LEVEL);

#include <lsf.h>
#include <lsf/service.h>
#include <lsf/services/thinker.h>

#include "ic_allocator.h"
extern IC_Allocator *g_pFreeer;

#define THINKER_SERVICE_ID DT_INST_REG_ADDR(0)

enum {
	THINKERSERVICE_PROPERTY_INPUT,
	THINKERSERVICE_PROPERTY_OUTPUT,
	THINKERSERVICE_PROPERTY_MODEL,
	THINKERSERVICE_PROPERTY_OUTPUT_COUNT,
};

static LsfPropertyConfig thinkerservice_property_defs[] = {
	{
		.property_id = THINKERSERVICE_PROPERTY_INPUT,
		.flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
		.type = LSF_PROPERTY_TYPE_PTR,
	},
	{
		.property_id = THINKERSERVICE_PROPERTY_OUTPUT,
		.flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_GET_ONLY,
		.type = LSF_PROPERTY_TYPE_PTR,
	},
	{
		.property_id = THINKERSERVICE_PROPERTY_MODEL,
		.flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
		.type = LSF_PROPERTY_TYPE_PTR,
	},
	{
		.property_id = THINKERSERVICE_PROPERTY_OUTPUT_COUNT,
		.flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_GET_ONLY,
		.type = LSF_PROPERTY_TYPE_UINT32,
	},
};

int lsf_thinker_set_model(const void *ptr, uint32_t size)
{
	LsfVariant var;
	int ret;

	LOG_DBG("Set model: addr: %p, size: %u", ptr, size);
	LOG_HEXDUMP_DBG(ptr, MIN(8, size), "Model data (first 8 bytes):");

	LsfVariant_init(&var);
	var.variant_type = type_pointer;
	var.u.pointer.pAddr = (void *)ptr;
	var.u.pointer.size = size;

	ret = lsf_set(THINKER_SERVICE_ID, THINKERSERVICE_PROPERTY_MODEL, &var);
	if (ret != 0) {
		LOG_ERR("Failed to set model: %d", ret);
		ret = -EIO;
	}

	LsfVariant_clear(&var);
	return ret;
}

int lsf_thinker_set_input(const void *ptr, uint32_t size)
{
	LsfVariant var;
	int ret;

	LOG_DBG("Set input: addr: %p, size: %u", ptr, size);
	LOG_HEXDUMP_DBG(ptr, MIN(8, size), "Input data (first 8 bytes):");

	LsfVariant_init(&var);
	var.variant_type = type_pointer;
	var.u.pointer.pAddr = (void *)ptr;
	var.u.pointer.size = size;

	ret = lsf_set(THINKER_SERVICE_ID, THINKERSERVICE_PROPERTY_INPUT, &var);
	if (ret != 0) {
		LOG_ERR("Failed to set input: %d", ret);
		ret = -EIO;
	}

	LsfVariant_clear(&var);
	return ret;
}

int lsf_thinker_get_output(void **ptr, uint32_t *size)
{
	LsfVariant *var;
	int ret;

	ret = lsf_get(THINKER_SERVICE_ID, THINKERSERVICE_PROPERTY_OUTPUT, &var);
	if (ret != 0) {
		LOG_ERR("Failed to get output: %d", ret);
		return -EIO;
	}

	LsfVariant output;
	LsfVariant_init(&output);
	output.variant_type = type_pointer;

	size_t output_size = LsfVariant_getMarshalSize(&output);

	size_t buf_size = var->u.pointer.size;
	size_t item_size;
	for (int i = 0; buf_size > 0; i++) {
		LsfVariantClass_unmarshal(&output,
					  (uint8_t *)var->u.pointer.pAddr + i * output_size,
					  output_size, &item_size);
		buf_size -= item_size;

		ptr[i] = output.u.pointer.pAddr;
		size[i] = output.u.pointer.size;

		LOG_DBG("Got output[%d]: ptr: %p, size: %u", i, ptr[i], size[i]);
		LOG_HEXDUMP_DBG(ptr[i], MIN(8, *size), "Output data (first 8 bytes):");
	}

	IC_Sharemem_detach(var->u.pointer.pAddr, var->u.pointer.size);
	IC_Allocator_free(g_pFreeer, var->u.pointer.pAddr);

	LsfVariant_unref(var);

	return 0;
}

int lsf_thinker_get_output_count(void)
{
	LsfVariant *var;
	uint32_t count;
	int ret;

	ret = lsf_get(THINKER_SERVICE_ID, THINKERSERVICE_PROPERTY_OUTPUT_COUNT, &var);
	if (ret < 0) {
		LOG_ERR("Failed to get output count: %d", ret);
		return -EIO;
	}

	count = var->u.uiVal;
	LsfVariant_unref(var);

	LOG_DBG("Got output count: %d", count);

	return count;
}

static int lsf_thinker_init(void)
{
	LOG_DBG("Initialize ThinkerService");

	lsf_import_properties(THINKER_SERVICE_ID, thinkerservice_property_defs,
			      ARRAY_SIZE(thinkerservice_property_defs));

	return 0;
}

LSF_SERVICE_DEFINE(lsf_thinker, lsf_thinker_init);
