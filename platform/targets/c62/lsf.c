/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lsf, LOG_LEVEL_DBG);

#include <lsf.h>
#include <ic_proxy.h>
#include <ic_fence.h>

#include <controller.h>
#include <service.h>

/*
 * The DSP firmware's ready fence, as ListenAI's LSF controller requests it:
 * https://github.com/OpenRTX/lsf-zephyr-sdk/blob/2ac605527dcde3e90c814bb4175546cc7fbd36da/src/lsf.c
 */
#define DSP_READY_FENCE_ID 0

int lsf_controller_init(void)
{
    int ret;

    LOG_DBG("Initializing LSF service controller");

    /* Initialize LSF. Its calls return LSF and IC codes, not errno values. */
    ret = lsf_init();
    if (ret != 0) {
        LOG_ERR("LSF init failed: %d", ret);
        return -EIO;
    }
    ret = lsf_connect();
    if (ret != 0) {
        LOG_ERR("Connecting to the DSP failed: %d", ret);
        return -EIO;
    }

    /* Wait for ready signal */
    ICFenceHandle fence = IC_Proxy_getRemoteFence(DSP_READY_FENCE_ID);

    if (fence == NULL) {
        LOG_ERR("DSP ready fence not found");
        return -EIO;
    }
    ret = ICFence_syncWithRemote(fence);
    if (ret != IC_OK) {
        LOG_ERR("DSP sync failed: %d", ret);
        return -EIO;
    }
    LOG_DBG("DSP synced");

    uint32_t val;
    ret = ICFence_wait(fence, &val);
    if (ret != IC_OK) {
        LOG_ERR("Waiting for the DSP failed: %d", ret);
        return -EIO;
    }
    LOG_DBG("DSP ready");

    STRUCT_SECTION_FOREACH(lsf_service, service)
    {
        LOG_DBG("Initializing service %s", service->name);

        ret = service->init();
        if (ret != 0) {
            LOG_ERR("Failed to initialize service %s: %d", service->name, ret);
            return ret;
        }
    }

    LOG_DBG("All services initialized");

    return 0;
}
