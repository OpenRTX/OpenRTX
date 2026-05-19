/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VP_USE_FILESYSTEM
.section .note.GNU-stack,""
.section .rodata

.global _vpdata_start
.global _vpdata_end

_vpdata_start:
    .incbin "../voiceprompts.vpc"

.set _vpdata_end, .
#endif
