/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LSF_SERVICE_H__
#define __LSF_SERVICE_H__

#include <zephyr/toolchain/common.h>

typedef int (*lsf_service_init_t)(void);

struct lsf_service {
	const char *name;
	lsf_service_init_t init;
};

#define LSF_SERVICE_DEFINE(_name, _init_fn)                                                        \
	static STRUCT_SECTION_ITERABLE(lsf_service, _name) = {                                     \
		.name = STRINGIFY(_name), .init = _init_fn,                                        \
	};

#endif /* __LSF_SERVICE_H__ */
