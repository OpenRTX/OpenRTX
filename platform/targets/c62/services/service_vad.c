/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lsf_vad_service

#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lsf_vad, CONFIG_LSF_LOG_LEVEL);

#include <lsf.h>
#include <lsf/service.h>
#include <lsf/services/vad.h>

#define VAD_SERVICE_ID DT_INST_REG_ADDR(0)

enum {
	VADSERVICE_PROPERTY_CONFIG,
	VADSERVICE_PROPERTY_ENABLE,
};

static LsfPropertyConfig vadservice_property_defs[] = {
	{
		.property_id = VADSERVICE_PROPERTY_CONFIG,
		.flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
		.type = LSF_PROPERTY_TYPE_BUF,
	},
	{
		.property_id = VADSERVICE_PROPERTY_ENABLE,
		.flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
		.type = LSF_PROPERTY_TYPE_UINT32,
	},
};

struct lsf_vad_config {
	uint8_t channel;
	uint16_t amp_l;
	uint16_t amp_h;
	uint8_t minlen_trans_neg;
	uint8_t minlen;
};

int lsf_vad_enable(void)
{
	LsfVariant var;
	int ret;

	LOG_DBG("Enable VAD");

	LsfVariant_init(&var);
	var.variant_type = type_uint;
	var.u.uiVal = 1;

	ret = lsf_set(VAD_SERVICE_ID, VADSERVICE_PROPERTY_ENABLE, &var);
	if (ret != 0) {
		LOG_ERR("Failed to enable VAD: %d", ret);
		return -EIO;
	}

	LsfVariant_clear(&var);

	return 0;
}

static int lsf_vad_init(void)
{
	LsfVariant var;
	int ret;

	LOG_DBG("Initialize VadService");

	lsf_import_properties(VAD_SERVICE_ID, vadservice_property_defs,
			      ARRAY_SIZE(vadservice_property_defs));

	struct lsf_vad_config cfg = {
		.channel = DT_INST_PROP(0, channel),
		.amp_l = DT_INST_PROP(0, amp_l),
		.amp_h = DT_INST_PROP(0, amp_h),
		.minlen_trans_neg = DT_INST_PROP(0, minlen_trans_neg),
		.minlen = DT_INST_PROP(0, minlen),
	};

	LsfVariant_init(&var);
	var.variant_type = type_membuf;
	var.u.memBuf.pData = &cfg;
	var.u.memBuf.size = sizeof(cfg);

	ret = lsf_set(VAD_SERVICE_ID, VADSERVICE_PROPERTY_CONFIG, &var);
	if (ret != 0) {
		LOG_ERR("Failed to set config: %d", ret);
		return -EIO;
	}

	LsfVariant_clear(&var);

	return 0;
}

LSF_SERVICE_DEFINE(lsf_vad, lsf_vad_init);
