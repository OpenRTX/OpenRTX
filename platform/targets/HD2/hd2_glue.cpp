/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Glue between OpenRTX and the vendor Miosix kernel for the HD2.  The
 * interfaces/delays.h implementation lives in mcu/HR_C7000/drivers/delays.cpp;
 * this file only carries the raw bring-up UART trace helper.
 */

extern "C" {

// Raw UART0 trace for bring-up (UART0 @ 0x14030000, 57600 8N1 set by the bsp).
// Bounded spin so a stuck UART can't hang a thread.
void hd2_trace(const char *s)
{
    volatile unsigned int *thr = (volatile unsigned int *)0x14030000u;
    volatile unsigned int *lsr = (volatile unsigned int *)0x14030014u;
    while (*s) {
        for (unsigned int g = 0; g < 200000u && (*lsr & 0x20u) == 0u; ++g) {
        }
        *thr = (unsigned char)*s++;
    }
}

} // extern "C"
