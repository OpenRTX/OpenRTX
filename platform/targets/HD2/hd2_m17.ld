/*
 * Linker script for the Ailunce HD2 (HR_C7000 / CK803S) -- LOW-RAM-ORIGIN
 * variant for the M17 / Codec2 build (select with -DMIOSIX_LINKER_SCRIPT=hd2_m17.ld).
 *
 * Identical to hd2.ld EXCEPT the RAM region starts at 0x20000 instead of
 * 0x33000, reclaiming the 0x20000..0x33000 window (76 KiB) for the heap.  That
 * window is the lower part of the AMBE+2 codec's native code window
 * (0x1bfe0..0x327e8), which hd2.ld reserves for the DMR-voice driver -- so this
 * script is ONLY valid when the AMBE codec is NOT linked (i.e. HD2_DMR_VOICE=OFF).
 * Building HD2_DMR_VOICE with this script would let .data/.bss/heap collide with
 * the runtime-copied codec blob.  Codec2 (M17 voice) needs the bigger heap:
 * codec2_create mallocs ~15-20 KiB of FFT cfgs + analysis buffers, which does not
 * fit the ~39 KiB heap the 0x33000 origin leaves (measured: 10.6 KiB free under a
 * 16 KiB worker stack -> codec2_create faulted).  Origin 0x20000 grows RAM
 * 112 KiB -> 192 KiB (heap ~39 KiB -> ~119 KiB).
 *
 * The IAP/BOOTROM low IRAM (0x10000..0x20000) stays untouched -- the region
 * starts AT 0x20000, above it.  Everything else (flash slot, trace block at
 * 0x4f000, section/symbol definitions) is identical to unikernel.ld.
 */

_irq_stack_size = 1024;

MEMORY
{
    flash(rx) : ORIGIN = 0x0300d000, LENGTH = 614400
    /* 192 KiB: 0x20000..0x4f000.  Top 4 KiB (0x4f000..0x50000) excluded for the
     * hard-lock forensics breadcrumb block (see hd2.ld). */
    ram(wx)   : ORIGIN = 0x00020000, LENGTH = 0x2F000
}

/* No AMBE reservation here: this variant is for non-DMR-voice builds, so the
 * 0x1bfe0..0x327e8 window is not reserved.  These symbols are kept defined (some
 * shared code references them) but point at their nominal addresses. */
_ambe_code_vma = 0x0001bfe0;
_ambe_code_end = 0x000327e8;

INCLUDE ldscripts/miosix-flash-ram.ld
