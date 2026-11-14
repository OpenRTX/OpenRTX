/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LSF_CONTROLLER_H__
#define __LSF_CONTROLLER_H__

/**
 * Connect to the DSP and start its services. Called once at startup.
 *
 * @return 0 on success.
 */
int lsf_controller_init(void);

#endif /* __LSF_CONTROLLER_H__ */
