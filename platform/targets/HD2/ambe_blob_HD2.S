/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Embeds the device-resident software AMBE+2 voice codec, carved from the
 * user's own HD2 firmware by scripts/ambe_carve.py (build-ambe/, gitignored --
 * AMBE+2 is DVSI-patented; the bytes are NEVER committed, only extracted at
 * build time from firmware the user already owns).
 *
 * Three blobs, stored here in flash .rodata; hd2_ambe.c copies them at runtime
 * to their native VMAs (the codec is not position-independent):
 *   ambe_lo.bin   -> 0x0001bfe0   (driver + encode + analysis + decode + tables)
 *   ambe_hi.bin   -> 0x0003227c   (49-bit bit serializers)
 *   ambe_sahb.bin -> 0x18000000   (.sahb_sram DSP coefficient tables the
 *                                  analysis path dereferences; separate SRAM,
 *                                  no overlap with the OpenRTX image)
 *
 * The .incbin paths are resolved with an assembler -I pointing at build-ambe/
 * (see the HD2 openrtx_hd2 CMakeLists, gated on HD2_DMR_VOICE).
 */
.section .note.GNU-stack,""
.section .rodata
.global _ambe_lo_start
.global _ambe_lo_end
.global _ambe_hi_start
.global _ambe_hi_end
.global _ambe_sahb_start
.global _ambe_sahb_end

.balign 4
_ambe_lo_start:
    .incbin "ambe_lo.bin"
.set _ambe_lo_end, .

.balign 4
_ambe_hi_start:
    .incbin "ambe_hi.bin"
.set _ambe_hi_end, .

.balign 4
_ambe_sahb_start:
    .incbin "ambe_sahb.bin"
.set _ambe_sahb_end, .
