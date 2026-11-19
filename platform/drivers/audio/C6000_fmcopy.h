/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef C6000_FMCOPY_H
#define C6000_FMCOPY_H

#include "drivers/baseband/HR_C6000.h"
#include "interfaces/audio.h"

/**
 * Audio input driver for the receive baseband taken from the HR_C6000 FM
 * receive copy: the chip demodulates the IF itself and keeps its last
 * 2 x 256 samples (8 kHz, 0-3.4 kHz) in its data RAM. The driver reads each
 * half as the chip fills it and interpolates it to 24 kHz.
 *
 * Only for boards that feed the HR_C6000 the IF in receive. The target calls
 * C6000fmcopy_systemIrq() from the interrupt handler of the chip's system
 * interrupt line. 24 kHz, circular
 * double buffer mode only. The receive I/Q order follows the side of the
 * first local oscillator, so the copy can come out inverted: that is for the
 * caller to handle.
 */

/**
 * Initialise the driver.
 *
 * @param device: the HR_C6000 that demodulates the IF.
 */
void C6000fmcopy_init(HR_C6000 *device);

/**
 * Count one HR_C6000 system interrupt: call from its interrupt handler.
 */
void C6000fmcopy_systemIrq();

extern "C" const struct audioDriver C6000_fmcopy_audio_driver;

#endif /* C6000_FMCOPY_H */
