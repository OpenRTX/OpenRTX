/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Embeds the IMA-ADPCM voice-prompt container (voiceprompts.vpa, built from the
 * stock voiceprompts.vpc by scripts/vpc_to_adpcm.py) into .rodata, for targets
 * that link the ADPCM player (core/voicePrompts_adpcm.c) instead of the
 * codec2-based core/voicePrompts.c.  The player reads it via
 * _vpadata_start / _vpadata_end.  Mirrors voicePromptData.S (the codec2 .vpc
 * embed); the .incbin path resolves the same way (assembler -I -> repo root).
 */
#ifndef VP_USE_FILESYSTEM
.section .note.GNU-stack,""
.section .rodata

.global _vpadata_start
.global _vpadata_end

.balign 4
_vpadata_start:
    .incbin "../voiceprompts.vpa"

.set _vpadata_end, .
#endif
