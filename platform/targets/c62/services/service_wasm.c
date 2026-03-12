/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT lsf_wasm_service

#include <zephyr/device.h>
#include <zephyr/storage/flash_map.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lsf_wasm, CONFIG_LSF_LOG_LEVEL);

#include <lsf.h>
#include <lsf/service.h>
#include <lsf/services/wasm.h>

#define WASM_SERVICE_ID DT_INST_REG_ADDR(0)

#define Z_FLASH_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_flash))

#if defined(CONFIG_LSF_WASM_SERVICE_LOAD_APP_ON_BOOT)
#define WASM_APP_NODE DT_CHOSEN(lsf_dsp_wasm_app)
#endif /* CONFIG_LSF_WASM_SERVICE_LOAD_APP_ON_BOOT */

enum {
	WASMSERVICE_PROPERTY_APP,
};

static LsfPropertyConfig wasmservice_property_defs[] = {
	{
		.property_id = WASMSERVICE_PROPERTY_APP,
		.flag = LSF_PROPERTY_FLAG_SYNC | LSF_PROPERTY_FLAG_SET_ONLY,
		.type = LSF_PROPERTY_TYPE_PTR,
	},
};

int lsf_wasm_load_app(const void *ptr, uint32_t size)
{
	LsfVariant var;
	int ret;

	uint8_t *app = (uint8_t *)ptr;
	LOG_DBG("Load app from %p", app);

	/* Check for app header */
	const char *hdr = "LSHD";
	if (*((uint32_t *)app) != *((uint32_t *)hdr)) {
		LOG_ERR("Invalid app header");
		return -EINVAL;
	}

	/* Check for app size */
	uint32_t app_size = *((uint32_t *)(app + 4));
	if (app_size > size) {
		LOG_ERR("Invalid app size (expected: %u, actual: %u)", app_size, size);
		return -EINVAL;
	}

	uint8_t *app_addr = app + 8;
	LOG_DBG("App addr: %p, size: %u", app_addr, app_size);

	LsfVariant_init(&var);
	var.variant_type = type_pointer;
	var.u.pointer.pAddr = app_addr;
	var.u.pointer.size = app_size;

	ret = lsf_set(WASM_SERVICE_ID, WASMSERVICE_PROPERTY_APP, &var);
	if (ret != 0) {
		LOG_ERR("Failed to load app: %d", ret);
		return -EIO;
	}

	LsfVariant_clear(&var);

	return 0;
}

static int lsf_wasm_init(void)
{
	LOG_DBG("Initialize WasmService");

	lsf_import_properties(WASM_SERVICE_ID, wasmservice_property_defs,
			      ARRAY_SIZE(wasmservice_property_defs));

#if defined(CONFIG_LSF_WASM_SERVICE_LOAD_APP_ON_BOOT)
	LOG_DBG("Auto loading WASM app from %s (offset: 0x%08x, size: %u)", DT_LABEL(WASM_APP_NODE),
		DT_REG_ADDR(WASM_APP_NODE), DT_REG_SIZE(WASM_APP_NODE));

	lsf_wasm_load_app((void *)(Z_FLASH_BASE + DT_REG_ADDR(WASM_APP_NODE)),
			  DT_REG_SIZE(WASM_APP_NODE));
#endif /* CONFIG_LSF_WASM_SERVICE_LOAD_APP_ON_BOOT */

	return 0;
}

LSF_SERVICE_DEFINE(lsf_wasm, lsf_wasm_init);
