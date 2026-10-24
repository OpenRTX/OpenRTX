/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __LSF_CONTROLLER_H__
#define __LSF_CONTROLLER_H__

/**
 * Load the DSP firmware and start the DSP.
 *
 * @return 0 on success.
 */
int lsf_dsp_init(void);

int lsf_controller_init(void);

#endif /* __LSF_CONTROLLER_H__ */
