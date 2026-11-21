/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef LSF_DSP_H
#define LSF_DSP_H

/**
 * Load the DSP firmware and start the DSP.
 *
 * @return 0 on success.
 */
int lsf_dsp_init(void);

/**
 * Stop the DSP and hold it in reset.
 *
 * A reset of the application core leaves the DSP running: stop it before
 * rebooting, so it does not raise interrupts while the next boot starts.
 */
void lsf_dsp_terminate(void);

#endif /* LSF_DSP_H */
