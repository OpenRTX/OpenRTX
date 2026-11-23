/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lsf_controller, LOG_LEVEL_INF);

#include <lsf.h>
#include <ic_proxy.h>
#include <ic_fence.h>

#include <AudioSystem.h>

#include "lsf_controller.h"

/*
 * The DSP firmware's ready fence, as ListenAI's LSF controller requests it:
 * https://github.com/OpenRTX/lsf-zephyr-sdk/blob/2ac605527dcde3e90c814bb4175546cc7fbd36da/src/lsf.c
 */
#define DSP_READY_FENCE_ID 0

int lsfController_init(void)
{
    int ret;

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

    uint32_t val;
    ret = ICFence_wait(fence, &val);
    if (ret != IC_OK) {
        LOG_ERR("Waiting for the DSP failed: %d", ret);
        return -EIO;
    }

    /* The DSP's audio service, the only one OpenRTX uses */
    AudioSystem_initialize();

    return 0;
}
