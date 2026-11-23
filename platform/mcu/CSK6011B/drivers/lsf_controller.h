/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LSF_CONTROLLER_H
#define LSF_CONTROLLER_H

/**
 * Connect to the DSP, wait until it is ready and start its audio service.
 * Called once at startup, after lsfDsp_init().
 *
 * @return 0 on success.
 */
int lsfController_init(void);

#endif /* LSF_CONTROLLER_H */
