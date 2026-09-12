/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LSF_SERVICES_WASM_H__
#define __LSF_SERVICES_WASM_H__

#include <stdint.h>

int lsf_wasm_load_app(const void *ptr, uint32_t size);

#endif /* __LSF_SERVICES_WASM_H__ */
