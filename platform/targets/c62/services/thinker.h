/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LSF_SERVICES_THINKER_H__
#define __LSF_SERVICES_THINKER_H__

#include <stdint.h>

int lsf_thinker_set_model(const void *ptr, uint32_t size);

int lsf_thinker_set_input(const void *ptr, uint32_t size);

int lsf_thinker_get_output(void **ptrs, uint32_t *sizes);

int lsf_thinker_get_output_count(void);

#endif /* __LSF_SERVICES_THINKER_H__ */
