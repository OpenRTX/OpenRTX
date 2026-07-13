/*
 * Embedded M17 baseband test frame for the packet-mode physical-layer bring-up
 * (diag op 'f', gated by HD2_M17_TXTEST).  Raw int16 LE PCM @ 8 kHz: a complete
 * M17 transmission (preamble + LSF + frames + EOT) generated on host by m17-mod
 * and decimated 48k->8k (validated to decode through the 8 kHz bottleneck via
 * the m17-cxx-demod chain).  Played on loop through the FM-TX-RAM pump so an
 * external m17-demod can lock on our over-the-air LSF.  Lives in .rodata (flash),
 * costs no RAM.  m17_frame.raw sits beside this file; GAS resolves .incbin via -I.
 */
    .section .rodata
    .global _m17_blob_start
    .global _m17_blob_end
    .balign 2
_m17_blob_start:
    .incbin "m17_frame.raw"
_m17_blob_end:
