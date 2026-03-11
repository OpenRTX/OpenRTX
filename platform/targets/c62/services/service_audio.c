/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lsf_audio_service

#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lsf_audio, LOG_LEVEL_DBG);

#include "service.h"

#include <AudioSystem.h>

static int lsf_audio_init(void)
{
	LOG_DBG("Initialize AudioService");

	AudioSystem_initialize();

	return 0;
}

LSF_SERVICE_DEFINE(lsf_audio, lsf_audio_init);
