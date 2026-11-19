/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef C6000_SYMBOLS_H
#define C6000_SYMBOLS_H

#include "drivers/baseband/HR_C6000.h"
#include "interfaces/audio.h"

/**
 * Audio output driver that sends 4FSK symbols through the HR_C6000 physical
 * layer mode: the stream carries one symbol per sample, -3, -1, +1 or +3, at
 * 4800 Hz, and the chip shapes and modulates them itself. A zero, as before
 * the stream's first data, sends idle (+3 -3).
 *
 * The chip takes 144 symbols at each timeslot interrupt, every 30 ms: the
 * target calls C6000symbols_timeslotIrq() from the interrupt handler of the
 * line, and C6000symbols_task() serves it. Stopping the stream returns once
 * the last symbol and one idle slot have gone out.
 *
 * Circular double buffer mode only.
 */

/**
 * Initialise the driver.
 *
 * @param device: the HR_C6000 that modulates the symbols.
 */
void C6000symbols_init(HR_C6000 *device);

/**
 * Count one HR_C6000 timeslot interrupt: call from its interrupt handler.
 */
void C6000symbols_timeslotIrq();

/**
 * Give the chip its next slot of symbols after a timeslot interrupt. To be
 * called at least every few milliseconds, from the thread serving the other
 * HR_C6000 audio.
 */
void C6000symbols_task();

extern "C" const struct audioDriver C6000_symbols_audio_driver;

#endif /* C6000_SYMBOLS_H */
