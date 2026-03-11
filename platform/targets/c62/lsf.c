/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lsf_service_controller

#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lsf, LOG_LEVEL_DBG);

#include <lsf.h>
#include <ic_proxy.h>
#include <ic_fence.h>

#include <controller.h>
#include <service.h>

static volatile bool inited = false;

int lsf_controller_init(void)
{
	int ret;

	if (inited) {
		return 0;
	}

	LOG_DBG("Initializing LSF service controller");

	/* Initialize LSF */
	lsf_init();
	lsf_connect();

	/* Wait for ready signal */
	ICFenceHandle fence = IC_Proxy_getRemoteFence(0);

	ICFence_syncWithRemote(fence);
	LOG_DBG("DSP synced");

	uint32_t val;
	ICFence_wait(fence, &val);
	LOG_DBG("DSP ready");

	STRUCT_SECTION_FOREACH(lsf_service, service) {
		LOG_DBG("Initializing service %s", service->name);

		ret = service->init();
		if (ret != 0) {
			LOG_ERR("Failed to initialize service %s: %d", service->name, ret);
			return ret;
		}
	}

	LOG_DBG("All services initialized");

	inited = true;

	return 0;
}

static int lsf_controller_init_internal(const struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_HAS_CHOSEN(lsf_dsp_firmware)
	return lsf_controller_init();
#else  /* DT_HAS_CHOSEN(lsf_dsp_firmware) */
	return 0;
#endif /* DT_HAS_CHOSEN(lsf_dsp_firmware) */
}

DEVICE_DT_INST_DEFINE(0, lsf_controller_init_internal, NULL, NULL, NULL, APPLICATION,
		      CONFIG_APPLICATION_INIT_PRIORITY, NULL);
