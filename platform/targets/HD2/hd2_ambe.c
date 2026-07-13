/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Wrapper around the device-resident software AMBE+2 voice codec.  See
 * hd2_ambe.h and scripts/labels/ambe_codec.py for the codec ABI + provenance.
 *
 * The codec is not position-independent: it is copied to its native VMAs and
 * called there through function pointers at the addresses RE'd from the vendor
 * firmware.  The encoder STATE is caller-allocated and passed in explicitly
 * (the param-taking entry points; we do NOT use the vendor's OS-coupled global
 * spawner @0x1bbb8).  Whether the param path also touches the codec's absolute
 * 0x33000+ scratch globals is exactly what the bring-up self-test checks --
 * our RAM image starts at 0x33000, so such a write would corrupt us.
 */
#include "hd2_ambe.h"
#include <stdlib.h>
#include <string.h>

/* ---- carved blob, embedded in flash by ambe_blob_HD2.S ---- */
extern const uint8_t _ambe_lo_start[], _ambe_lo_end[];
extern const uint8_t _ambe_hi_start[], _ambe_hi_end[];
extern const uint8_t _ambe_sahb_start[], _ambe_sahb_end[];

/* Native VMAs the codec must run at (link@ from ambe_carve.py). */
#define AMBE_LO_VMA   ((void *)0x0001bfe0u)
#define AMBE_HI_VMA   ((void *)0x0003227cu)
/* .sahb_sram DSP coefficient tables the encode/analysis path dereferences at
 * fixed addresses (0x18001168..0x180034cc). Separate SRAM block, no overlap
 * with our image. Without these the first analysis call faults (RE 2026-06-19). */
#define AMBE_SAHB_VMA ((void *)0x18000000u)

/* Codec entry points (native VMAs, from scripts/labels/ambe_codec.py).
 * encode ABI (RE 2026-06-19, verified against the in-firmware caller @0x1c028):
 *   r0 = pointer to the 80 input PCM samples = state + 0x1898  (NOT the output)
 *   r1 = pitch, r2 = index, r3 = len (80, clamped 76..84)
 *   stack: [0x800, half(0/1), *(u16*)(state+0x173a), state]
 * The 49-bit output lands inside the state struct (params @ ~state+0xc4c);
 * extract via ambe_49bit_pack -- NOT returned through an out pointer. */
typedef int (*ambe_init_fn)(void *state, int a, int b);
typedef int (*ambe_enc_fn)(void *pcm_in, int pitch, int index, int len,
                           int c800, int half, int field173a, void *state);

#define AMBE_INIT  ((ambe_init_fn)0x0001ca4cu)
#define AMBE_ENC   ((ambe_enc_fn) 0x0001c8e0u)

/* Offsets within the state struct (RE 2026-06-19). */
#define AMBE_PCM_IN_OFF   0x1898u    /* 80 x s16 input window for the half-block */
#define AMBE_FIELD_173A   0x173au    /* u16 field passed as encode stack arg [sp+8] */
#define AMBE_PARAMS_OFF   0x0c4cu    /* 49-bit MBE param vector produced by encode */

/* The codec accesses its working buffers by HARDCODED ABSOLUTE address (PCM in
 * @0x5527c, etc.), NOT relative to the state pointer -- so the state MUST be the
 * codec's native object base 0x539e4 (else those internal accesses miss our
 * buffer and encode reads zeros).  Its two fixed regions (0x50f44 and 0x539e4,
 * spanning ~0x50f44..0x5539c) sit in the unused top SRAM ABOVE our image (ram
 * ends 0x4f000) and below SRAM end 0x58000 -- no heap/image overlap.  Empirically
 * established 2026-06-19 (encode ignored an arbitrary state ptr). */
#define AMBE_OBJ_BASE     ((void *)0x000539e4u)   /* native codec object = state base */
#define AMBE_RESET_BASE   ((void *)0x00050000u)   /* zero both fixed regions before init */
#define AMBE_RESET_LEN    0x6000u                 /* 0x50000..0x56000 (< SRAM end 0x58000) */

static int s_loaded = 0;

int hd2_ambe_load(void)
{
    const unsigned lo = (unsigned)(_ambe_lo_end - _ambe_lo_start);
    const unsigned hi = (unsigned)(_ambe_hi_end - _ambe_hi_start);
    if (lo == 0u)
        return 0;                       /* blob absent (HD2_DMR_VOICE off) */

    const unsigned sahb = (unsigned)(_ambe_sahb_end - _ambe_sahb_start);
    memcpy(AMBE_LO_VMA,   _ambe_lo_start,   lo);
    memcpy(AMBE_HI_VMA,   _ambe_hi_start,   hi);
    memcpy(AMBE_SAHB_VMA, _ambe_sahb_start, sahb);   /* coefficient tables */
    /* Code was just written as data; serialise before we execute it. CK803S
     * here has no SW-managed I-cache (miosix uses a bare `sync` barrier). */
    __asm__ volatile("sync" ::: "memory");
    s_loaded = 1;
    return (int)(lo + hi + sahb);
}

void *hd2_ambe_enc_alloc(void)
{
    if (!s_loaded)
        return NULL;
    memset(AMBE_RESET_BASE, 0, AMBE_RESET_LEN);   /* clear both fixed regions */
    AMBE_INIT(AMBE_OBJ_BASE, 1, 1);               /* seed Q15 coeffs + working area */
    return AMBE_OBJ_BASE;
}

void hd2_ambe_free(void *state)
{
    (void)state;                         /* fixed reserved buffer; nothing to free */
}

int hd2_ambe_encode_half(void *state, const int16_t *pcm80, int half)
{
    if (state == NULL || !s_loaded)
        return -1;
    uint8_t *base = (uint8_t *)state;
    /* Stage the 80 input samples where encode reads them, then call with the
     * RE'd register/stack layout. Output is left inside the state struct. */
    memcpy(base + AMBE_PCM_IN_OFF, pcm80, 80 * sizeof(int16_t));
    unsigned f173a = *(volatile uint16_t *)(base + AMBE_FIELD_173A);
    AMBE_ENC(base + AMBE_PCM_IN_OFF, 0, 0, 80, 0x800, half, (int)f173a, state);
    return 0;
}

int hd2_ambe_encode_frame(void *state, const int16_t *pcm160, uint8_t out[9])
{
    if (state == NULL || !s_loaded)
        return -1;
    /* One 20 ms frame = two 80-sample half-blocks. */
    hd2_ambe_encode_half(state, &pcm160[0],  0);
    hd2_ambe_encode_half(state, &pcm160[80], 1);
    /* Copy the produced 49-bit param vector out of the state struct (raw, not
     * yet bit-packed to the 9-byte DMR payload -- enough to verify encode ran
     * and responded to input). */
    memcpy(out, (uint8_t *)state + AMBE_PARAMS_OFF, 9);
    return 9;
}

void hd2_ambe_test_tone(int16_t *pcm160)
{
    /* 250 Hz: 32-sample period (8000/32 = 250) tiled across 160 samples. */
    static const int16_t sine32[32] = {
            0,   6392,  12539,  18204,  23170,  27245,  30273,  32137,
        32767,  32137,  30273,  27245,  23170,  18204,  12539,   6392,
            0,  -6392, -12539, -18204, -23170, -27245, -30273, -32137,
       -32767, -32137, -30273, -27245, -23170, -18204, -12539,  -6392,
    };
    for (int i = 0; i < 160; i++)
        pcm160[i] = (int16_t)(sine32[i & 31] / 4);   /* -1/4 FS, comfortable */
}
