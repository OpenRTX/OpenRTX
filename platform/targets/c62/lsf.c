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

static volatile bool inited = false;

int lsf_controller_init(void)
{
    int ret;

    if (inited) {
        return 0;
    }

    LOG_DBG("Initializing LSF service controller");

    /* Initialize LSF */
    ret = lsf_init();
    if (ret != 0)
        return ret;
    ret = lsf_connect();
    if (ret != 0)
        return ret;

    /* Wait for ready signal */
    ICFenceHandle fence = IC_Proxy_getRemoteFence(0);

    if (fence == NULL)
        return -EIO;
    ret = ICFence_syncWithRemote(fence);
    if (ret != IC_OK)
        return ret;
    LOG_DBG("DSP synced");

    uint32_t val;
    ret = ICFence_wait(fence, &val);
    if (ret != IC_OK)
        return ret;
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

    inited = true;

    return 0;
}
