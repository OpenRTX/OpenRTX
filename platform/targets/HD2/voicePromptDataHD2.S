/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Embeds the HD2 IMA-ADPCM voice-prompt container (voiceprompts.vpa, built by
 * scripts/vpc_to_adpcm.py) into .rodata.  voicePrompts_HD2.c reads it via
 * _vpadata_start / _vpadata_end.  The .incbin path is resolved with an
 * assembler -I (see the HD2 openrtx_hd2 CMakeLists).
 */
.section .note.GNU-stack,""
.section .rodata
.global _vpadata_start
.global _vpadata_end

.balign 4
_vpadata_start:
    .incbin "voiceprompts.vpa"
.set _vpadata_end, .
