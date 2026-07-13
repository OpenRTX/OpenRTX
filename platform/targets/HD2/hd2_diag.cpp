/*
 * hd2_diag.cpp -- minimal UART0 peek/poke diagnostic thread for the HD2
 * threaded (Miosix + OpenRTX) build.
 *
 * Speaks the EXACT wire protocol the flashing/debug bridge already implements
 * (scripts/rtx_tui.py  POST /cmd -> serial), so `rtx.py dbg r|w|ww` and the TUI
 * side panel work against the threaded firmware UNCHANGED:
 *
 *     'P'                              -> reply: "RTX1 ...version...\n"  (probe)
 *     'R' <u32 addr LE> <u8 size>      -> reply: <size> raw bytes   (byte read)
 *     'r' <u32 addr LE>                -> reply: <u32 LE>           (word read)
 *     'W' <u32 addr LE> <u32 val LE>   -> reply: 'k'                (word write)
 *
 * Unknown command bytes are ignored (resync), so the bridge's probe/keepalive
 * traffic does not wedge the parser. Intended for live MMIO/RAM inspection
 * during bring-up -- e.g. reading scheduler / os-timer state when a thread is
 * hung. Use word ops ('r'/'W') for MMIO registers; 'R' for RAM dumps.
 *
 * UART0 is the same 57600 8N1 console the bsp drives (hd2_dbg_puts); we only
 * add the RX side. Replies are short; concurrent console output during an
 * interactive poke is rare -- if it ever interleaves a reply, just re-issue.
 */

#include <miosix.h>
#include <pthread.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include "hd2_router.h"   /* audio-routing matrix + named device-routing twiddlers */
#include "hd2_wdt.h"      /* watchdog: 'X' reboot / auto-WDT control */
#include "hd2_trace.h"    /* hard-lock trace: 'H' dump + idle-loop stamp */
#include "hd2_cps_settings.h" /* 'D' op: vendor settings/records accessors */
#include "hd2_cps_records.h"
#include "drivers/NVM/flash_w25q_HD2.h" /* 'd' op + CPS server: raw W25Q access */
#include "drivers/baseband/dmr_HD2.h"   /* 'L'/'j' ops: shared DMR-TX bring-up */
#if defined(HD2_DMR_VOICE)
#include "hd2_ambe.h"                   /* 'K' op: AMBE codec bring-up self-test */
#endif
#if defined(HD2_M17)
extern "C" {
#include "codec2_mod.h"                 /* 'i' op: Codec2-mod (M17 3200) self-test */
}
#ifdef C2_PROFILE
/* Per-stage encode timing storage + ns clock, consumed by codec2_encode. */
extern "C" long long c2_prof_t[4] = {0, 0, 0, 0};
extern "C" long long c2_prof_now(void) { return miosix::getTime(); }
#endif
#endif
#include "interfaces/platform.h"        /* platform_ledOn/Off: CPS rx/tx LED feedback */

/* Speaker self-test tone.  platform_beepStart() is the HD2's minimal speaker
 * output pathway: it warms the codec (codec DAC -> lineout), unmutes DIPLEX0 +
 * the GPIOB amp/route, and drives PWM ch1 through the codec to the speaker --
 * with NO AT1846S (RF), NO modem, NO FM demod in the loop.  Exactly the path to
 * check in isolation. */
extern "C" void platform_beepStart(uint16_t freq);
extern "C" void platform_beepStop(void);

/* AT1846S register access (radio_HD2.cpp).  CAUTION: i2c0_lockDeviceBlocking
 * is a NO-OP on HD2 (the AT1846S bit-bang bus was assumed single-user), and the
 * GPIOA RMW is non-atomic -- so q/Q is ONLY safe with the FM worker OFF
 * (g_fm_active=0).  Poking concurrently with the FM worker's 250ms RSSI bit-bang
 * corrupts BOTH transactions, including RF-register *writes*.  (If concurrent
 * access is ever needed, make i2c0_lockDeviceBlocking a real FastMutex per the
 * note in i2c_csky.c.)  Exposed here to inspect/tune the AT1846S and chase the
 * FM demod->codec path without a reflash per experiment. */
extern "C" uint16_t hd2_at1846s_read(uint8_t reg);
extern "C" void     hd2_at1846s_write(uint8_t reg, uint16_t val);

#if defined(HD2_M17_TXTEST)
/* Embedded M17 baseband frame (m17_blob_HD2.S, .incbin raw int16 LE @ 8 kHz).
 * File-scope extern "C" so the linker name stays unmangled (an anonymous-
 * namespace declaration would mangle it and miss the .S symbols). */
extern "C" const unsigned char _m17_blob_start[];
extern "C" const unsigned char _m17_blob_end[];
extern "C" void hd2_m17_fm_tune(uint32_t hz);   /* radio_HD2.cpp: analog-FM tune */
#endif

#if defined(HD2_M17_VOICE)
/* Full on-device M17 voice TX (diag op 'g'): our fixed-point codec2 -> the
 * OpenRTX M17 FrameEncoder + Modulator -> a decimated 8 kHz baseband buffer ->
 * the FM-TX-RAM pump.  These C++ classes live in the global M17 namespace, so
 * they are included at file scope (before the anonymous namespace below). */
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/Modulator.hpp"
#include "protocols/M17/LinkSetupFrame.hpp"
#include "protocols/M17/Callsign.hpp"
#include "protocols/M17/Datatypes.hpp"
/* Modulator.cpp PLATFORM_HD2 sink: arm with the accumulation buffer, then read
 * back how many 8 kHz samples the generated transmission produced. */
extern "C" void   m17mod_hd2_set_sink(int16_t *buf, size_t cap);
extern "C" size_t m17mod_hd2_sink_len(void);
extern "C" void   hd2_m17_fm_tune(uint32_t hz);   /* radio_HD2.cpp: analog-FM tune */
/* Shared canonical FM-TX key/unkey (radio_HD2.cpp) -- the SAME register recipe
 * radio_enableTx / OpMode keys.  The pump op calls these for keying (under
 * g_rf_freeze, like the DMR bench ops) instead of hand-rolling the sequence. */
extern "C" void   hd2_fm_tx_key(uint32_t txFreq, bool narrow, uint8_t txToneEn,
                                uint8_t txToneType, uint16_t txTone, uint8_t tailElim);
extern "C" void   hd2_fm_tx_unkey(void);
#endif

/* TX-power mapping (radio_HD2.cpp): current codeplug txPower -> APC DAC-B drive
 * + AT1846S reg 0x0a padrv.  Used by the 'Y' M17-baseband pump (flag 0x40) to
 * key at real power -- the plain FM-tone test omits this, which is why early
 * 'Y' runs read as a dead carrier on a deaf bench RX. */
extern "C" void hd2_txpower_levels(uint16_t *apc, uint16_t *padrv);

/* Radio-path test helpers (radio_HD2.cpp / hd2_rtx.c).  g_rf_freeze is
 * the global that suspends ALL firmware-initiated AT1846S traffic and
 * audio-GPIO rewrites (rtx RSSI poll, squelch amp/route gating, FM worker,
 * beeps) so a host can run live chip experiments unopposed -- see the gated
 * call-site list at its definition in radio_HD2.cpp. */
extern "C" volatile uint32_t g_rf_freeze;

/* PC-Mode (codeplug programming) coordination flag, OWNED by the default UI.
 * The "PC Mode" UI screen sets it to 1 on entry (after quiescing the radio via
 * g_rf_freeze=1) and clears it on exit.  When set, the diag thread hands UART0
 * straight to the CPS server (cps::session) instead of the per-byte GetVer/0x68
 * gate, and cps::session returns as soon as it is cleared -- so the screen's
 * exit key ends the session.  Defined here (the diag TU); the UI declares it
 * extern.  Live RX/TX frame counters for the screen's on-screen feedback. */
extern "C" volatile uint32_t g_cps_pc_mode    = 0u;
extern "C" volatile uint32_t g_cps_rx_frames  = 0u;   /* host->radio frames served */
extern "C" volatile uint32_t g_cps_tx_replies = 0u;   /* radio->host replies sent  */

/* Radio boot-inhibit (hd2_rtx.c): 0 = radio bring-up deferred at boot; the
 * 'F' op sets it to 1 to bring the radio up on command (HW-I2C wedge debug). */
extern "C" volatile int g_radio_enabled;

/* Watchdog auto-heartbeat (hd2_rtx.c / hd2_wdt.h): rtx_task arms ~10 s and
 * feeds every pass while this is set; a hard lock then self-resets the chip.
 * Diag op 'X' controls it / forces an immediate WDT reboot. */
extern "C" volatile uint32_t g_wdt_auto;

/* IAP-handoff clock restore (platform.c) -- required before any deliberate
 * jump/reset into the IAP, or its UART comes up at the wrong baud. */
extern "C" void clk_restore_prepll(void);
extern "C" void     hd2_at1846s_reinit(uint32_t freq_hz);  /* BLOCKS ~700 ms (VCO cal) */
extern "C" void     hd2_at1846s_profile(uint32_t profile); /* 0=vendor 1=GD77 gains   */
extern "C" uint16_t hd2_at1846s_afmute(uint32_t mute);     /* reg0x30 bit7 RMW        */
/* RX freq + FM-extras live-override now go through the rtx API (rtx/rtx.h):
 * rtx_getCurrentStatus().rxFrequency and rtx_setFmExtras() -- the rtx state is
 * private to rtx.cpp after the OpMode_FM convergence. */
#include "rtx/rtx.h"

/* CPU->codec-DAC PCM stream experiment (hd2_pcm_stream.cpp).  Streams a sine
 * through the SAHB PCM playback window via the vec-0x3b frame IRQ and reports
 * the IRQ count -- the live test for docs/pcm_stream_playback.md.  BLOCKS this
 * thread for the tone duration (<= 10 s). */
extern "C" void hd2_pcm_tone(uint16_t freq_hz, uint16_t ms, uint8_t arm,
                             char *out, unsigned outsz);

/* Full-stack stream test (hd2_pcm_stream.cpp): same tone but via
 * audioPath_request -> audioStream_start -> outputStream_HD2 driver.
 * BLOCKS this thread for the tone duration (<= 10 s). */
extern "C" void hd2_stream_tone(uint16_t freq_hz, uint16_t ms,
                                char *out, unsigned outsz);

/* RX audio-capture probe (hd2_pcm_stream.cpp): arm the codec ADC -> SAHB PCM
 * capture path and report peak-to-peak amplitude of the captured window.
 * BLOCKS this thread for <ms> (<= 10 s). */
extern "C" void hd2_pcm_capture(uint16_t ms, uint8_t arm, char *out, unsigned outsz);

/* APRS RX (hd2_pcm_stream.cpp): carrier-triggered, stream-decodes 1200-baud
 * AFSK/AX.25 from the demod audio.  BLOCKS up to ~10 s. */
extern "C" void hd2_aprs_rx(uint16_t frames, char *out, unsigned outsz);

/* APRS TX (hd2_pcm_stream.cpp): keys analog-FM TX and streams a 1200-baud
 * AFSK/AX.25 beacon via the codec playback path.  TRANSMITS RF.  BLOCKS ~<3 s. */
extern "C" void hd2_aprs_tx(uint8_t preamble, uint8_t flags, char *out, unsigned outsz);

/* Voice-prompt test (hd2_diag.cpp below): forces vpLevel high, queues a
 * sequence, and triggers playback.  The actual codec2 decode + streaming runs
 * in the UI main_thread's vp_tick(), so this returns immediately. */
extern "C" void hd2_vp_say(uint8_t kind, uint8_t arg);

/* IMA-ADPCM decode test (hd2_pcm_stream.cpp): decode the embedded "zero" clip
 * to PCM and stream it.  BLOCKS this thread for the clip duration (~540 ms). */
extern "C" void hd2_adpcm_sample_play(char *out, unsigned outsz);

using namespace miosix;

namespace {

// DesignWare 16550 UART0 @ 0x14030000 -- the SAME register file the bsp console
// uses (board/hd2 bsp.cpp). DLAB is 0 in run state (bsp set LCR=0x03 at init),
// so +0x00 reads RBR / writes THR.
#define HD2_UART0(off)   (*(volatile uint32_t*)(0x14030000u + (off)))
#define UART0_RBR        HD2_UART0(0x00u)   // RX buffer (read)
#define UART0_THR        HD2_UART0(0x00u)   // TX holding (write)
#define UART0_FCR        HD2_UART0(0x08u)   // FIFO control (write-only; aliases IIR on read)
#define UART0_LSR        HD2_UART0(0x14u)   // line status
static constexpr uint32_t UART_LSR_DR   = 0x01u;   // RX data ready
static constexpr uint32_t UART_LSR_THRE = 0x20u;   // TX holding empty

// Version returned by the 'P' probe. Keeps the legacy "RTX1" token first (so the
// bridge recognises a healthy link) but appends a REAL build stamp compiled into
// this TU -- a fresh build changes it, so a stale flash is immediately visible
// over the probe (cf. the 2026-05-30 stale-build bug). Newline-terminated; the
// bridge reads a variable-length line.
#define HD2_DIAG_VERSION  "RTX1 hd2-miosix " __DATE__ " " __TIME__

// Bounded byte read: wait up to ~250 ms for RX-ready, then give up so a
// desynced/partial command aborts (parser resyncs) instead of wedging.
//
// 2026-06-11: the old version busy-spun 2,000,000 iterations with NO yield.
// On a partial/stalled command (the host bridge sends an opcode then a byte
// goes missing) that pegged the CPU for hundreds of ms PER missing byte --
// a 'W' awaiting 8 bytes could starve the rtx thread (which shares the
// AT1846S bus) for seconds, presenting as "serial stops answering / system
// frozen then self-recovers".  Now: poll briefly, then Thread::sleep(1) so
// other threads run while we wait, with a wall-clock-ish bound (~250 polls
// of 1 ms).  Returns fast on the common case (byte already in the FIFO).
bool rxByte(uint8_t &out)
{
    for(int ms = 0; ms < 250; ++ms)
    {
        for(int p = 0; p < 256; ++p)
            if((UART0_LSR & UART_LSR_DR) != 0u) { out = UART0_RBR & 0xFFu; return true; }
        Thread::sleep(1);
    }
    return false;
}

void tx(uint8_t c)
{
    for(uint32_t g = 0; g < 200000u && (UART0_LSR & UART_LSR_THRE) == 0u; ++g) {}
    UART0_THR = c;
}

void txStr(const char *s) { while(*s) tx(static_cast<uint8_t>(*s++)); }

bool rxU32(uint32_t &out)                          // little-endian
{
    out = 0;
    for(int i = 0; i < 4; ++i)
    {
        uint8_t b;
        if(!rxByte(b)) return false;               // timeout -> abort/resync
        out |= static_cast<uint32_t>(b) << (8 * i);
    }
    return true;
}

void txU32(uint32_t v)                             // little-endian
{
    for(int i = 0; i < 4; ++i) tx(static_cast<uint8_t>((v >> (8 * i)) & 0xFFu));
}

/*
 * FM TX tone test (op 'Y') -- the native version of the 2026-06-11 host-poke
 * experiments, per HR_C7000 manual ch.11 (FM application):
 *
 *   FM TX = CPU writes 256-sample PCM ping-pong buffers into the FM TX RAM
 *   (0x16000000 +0x030/+0x230, 8-bit RAM -> BYTE writes only, big-endian
 *   s16, low addr = high byte), WORK_MODE(0x11000100) bit7 = FM analog
 *   modulator mode, FM_PTT(0x11000560) bit0 = modem TX on.  The modem
 *   FM-modulates the PCM out MOD1/MOD2 into the AT1846S varactor nets;
 *   the AT1846S itself only keys the carrier (reg 0x30 = 0x4046).
 *
 * The tone is 1 kHz at 8 kHz sampling = 8 samples/cycle, so 256-sample
 * buffers hold exactly 32 cycles -- the ping-pong loops seamlessly with NO
 * FM_TX_INTERP servicing.  Host-poke history (why this is firmware-side):
 * the TX RAM ignores the upper 3 bytes of word stores (8-bit RAM) and an
 * unaligned word store to it hangs the SAHB; the AT1846S carrier keying and
 * WORK_MODE/FM_PTT writes were already proven live from the host.
 */
/* 256-entry s16 sine LUT (+/-12000), built once on first use.  Used by the
 * 'Y' M17-baseband pump (flag 0x40) to synthesise a phase-continuous sweep
 * without per-sample float in the refill loop. */
static int16_t s_fmSineLut[256];
static bool    s_fmSineReady = false;
static void fm_sine_lut_init()
{
    if(s_fmSineReady) return;
    for(unsigned i = 0; i < 256u; ++i)
        s_fmSineLut[i] = static_cast<int16_t>(
            12000.0f * sinf((2.0f * 3.14159265f * static_cast<float>(i)) / 256.0f));
    s_fmSineReady = true;
}

/* Fill ONE 256-sample FM TX-RAM ping-pong buffer (base 0x030 or 0x230) with a
 * phase-continuous sine.  8-bit RAM -> BYTE writes, big-endian s16, low address
 * = upper 8 bits (manual 11.4.1).  The NCO phase/freq are carried across refills
 * so the tone sweeps smoothly -- a static buffer would not exercise the
 * refill+FM_ADDR_SW sync that the real M17 modulator sink depends on. */
static void fm_chirp_fill(volatile uint8_t *txram, unsigned base,
                          uint32_t *phase, uint32_t *freq_hz)
{
    for(unsigned i = 0; i < 256u; ++i)
    {
        int16_t v = s_fmSineLut[(*phase >> 24) & 0xffu];
        txram[base + 2u*i]      = static_cast<uint8_t>((v >> 8) & 0xff);
        txram[base + 2u*i + 1u] = static_cast<uint8_t>(v & 0xff);
        /* phase advance = freq * 2^32 / 8000 (FM voice ping-pong runs at 8 kHz) */
        *phase += static_cast<uint32_t>(((uint64_t)(*freq_hz) << 32) / 8000u);
    }
    *freq_hz += 40u;                        /* ramp ~40 Hz per 32 ms buffer */
    if(*freq_hz > 3500u) *freq_hz = 300u;   /* sweep 300 Hz .. 3.5 kHz, repeat */
}

/* flags: bit0=WORK_MODE|0x80, bit1=FM_PTT=1, bit2=fill TX RAM with tone,
 *        bit3=write SIG_CENTER/RF_MOD_BIAS mid-scale guesses, bit4=engine-
 *        liveness poll loop, bit5=imask vendor FM value.
 * bit6 = M17 TX-RAM BASEBAND PUMP  (Phase 0 SINK_RTX validation; see
 *        docs/m17_tx_plan.md).  Proves the FM modulator FM-modulates ARBITRARY
 *        CPU baseband -- the mechanism the OpenRTX M17 modulator needs.  When set
 *        it forces wm|0x80 + FM_PTT + vendor imask, then:
 *          - selects the CPU-RAM audio source: AUDIO_CONTROL 0x11000080[0]=1
 *            ("voice data comes from RAM written by the MCU", manual 7.2.2.1.2);
 *          - keys at REAL TX power (APC DAC-B + padrv) -- the plain tone test
 *            omits this, which is why early 'Y' runs looked dead on a deaf RX;
 *          - linearises the FM path: FM_BANDWIDTH 0x11000500 pre-emphasis OFF
 *            [4]=0 + wide [3]=1 (4FSK must not be pre-emphasised/clipped);
 *          - continuously refills the 0x16000030/0x230 ping-pong, synced to
 *            FM_ADDR_SW, with a 300 Hz..3.5 kHz sine sweep.
 *        EXPECT on an SDR: the carrier tracks the sweep with a flat passband.
 *        Deviation scaling (FM_DEV_COEF 0x11000504 fm_dev_coef_t) is left at reset
 *        here -- tune it live via the word-poke op to land +/-3 symbols at
 *        +/-2.4 kHz without clipping.  Use bit3=0 (do NOT touch MOD cal).
 * 2026-06-11 isolation matrix: bare keying (flags=0) was RECEIVED as carrier
 * in the host experiments; adding wm|0x80 + FM_PTT made the carrier vanish
 * from the receiver -- suspicion: MOD DACs engage with zeroed cal and drag
 * the carrier off-frequency.  This op lets us A/B each ingredient. */
static void fm_tx_tone_test(uint8_t secs, uint8_t flags, char *out, unsigned outsz)
{
    static const int16_t tone[8] = { 0, 8485, 12000, 8485, 0, -8485, -12000, -8485 };
    volatile uint8_t  *txram = reinterpret_cast<volatile uint8_t *>(0x16000000u);
    volatile uint32_t *wm    = reinterpret_cast<volatile uint32_t*>(0x11000100u);
    volatile uint32_t *ptt   = reinterpret_cast<volatile uint32_t*>(0x11000560u);
    volatile uint32_t *sigc  = reinterpret_cast<volatile uint32_t*>(0x11000108u);
    volatile uint32_t *mbias = reinterpret_cast<volatile uint32_t*>(0x11000114u);
    volatile uint32_t *audc  = reinterpret_cast<volatile uint32_t*>(0x11000080u);
    volatile uint32_t *fmbw  = reinterpret_cast<volatile uint32_t*>(0x11000500u);

    const bool pump = (flags & 0x40u) != 0u;
    if(pump) flags |= 0x01u | 0x02u | 0x20u; /* pump needs modulator + PTT + imask */

    uint32_t freeze0 = g_rf_freeze;
    g_rf_freeze = 1;                       /* rtx thread off the bus while we TX */

    uint32_t nco_phase = 0u, nco_freq = 300u;
    if(pump)
    {
        fm_sine_lut_init();
        fm_chirp_fill(txram, 0x030u, &nco_phase, &nco_freq);  /* prime both halves */
        fm_chirp_fill(txram, 0x230u, &nco_phase, &nco_freq);
    }
    else if(flags & 0x04u)
    {
        static const unsigned bufs[2] = { 0x030u, 0x230u };
        for(unsigned b = 0; b < 2; ++b)
            for(unsigned i = 0; i < 256u; ++i)
            {
                int16_t s = tone[i & 7u];
                txram[bufs[b] + 2u*i]      = static_cast<uint8_t>((s >> 8) & 0xff);
                txram[bufs[b] + 2u*i + 1u] = static_cast<uint8_t>(s & 0xff);
            }
    }

    volatile uint32_t *imask  = reinterpret_cast<volatile uint32_t*>(0x1100039cu);
    volatile uint32_t *istat  = reinterpret_cast<volatile uint32_t*>(0x11000398u);
    volatile uint32_t *iclear = reinterpret_cast<volatile uint32_t*>(0x110003b0u);
    volatile uint32_t *addrsw = reinterpret_cast<volatile uint32_t*>(0x1100056cu);

    uint32_t wm0 = *wm, sigc0 = *sigc, mbias0 = *mbias, imask0 = *imask;
    uint32_t audc0 = *audc, fmbw0 = *fmbw;
    if(pump)
    {
        *audc = audc0 | 0x01u;             /* tx_voice_source = TX RAM (CPU PCM) */
        *fmbw = (fmbw0 & ~0x10u) | 0x08u;  /* pre-emphasis OFF, FM bandwidth wide */
        uint16_t apc, padrv;
        hd2_txpower_levels(&apc, &padrv);
        *reinterpret_cast<volatile uint32_t*>(0x140f0004u) &= ~0x2u; /* DAC-B lp off */
        *reinterpret_cast<volatile uint32_t*>(0x140f0000u) &= ~0x2u; /* DAC-B power up */
        *reinterpret_cast<volatile uint32_t*>(0x140f000cu)  = apc;   /* APC drive */
        hd2_at1846s_write(0x0a, static_cast<uint16_t>((padrv << 11) | 0x0420u));
    }
    if(flags & 0x08u)
    {
        *sigc  = 0x80800000u;              /* MOD1/MOD2 offset mid-scale guess  */
        *mbias = 0x00004040u;              /* MOD amplitude mid-scale guess     */
    }
    if(flags & 0x20u) *imask = 0x0001007fu;/* vendor FM SYS_INTERP_MASK value   */
    if(flags & 0x01u) *wm  = wm0 | 0x80u;  /* FM analog modulator mode          */
    if(flags & 0x02u) *ptt = 1u;           /* modem FM TX on                    */

    uint16_t r40 = hd2_at1846s_read(0x40);
    hd2_at1846s_write(0x40, 0x0030u);      /* vendor TX-side AF-DSP ctrl value  */
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);      /* tx_on: key the carrier            */
    if(pump) hd2_at1846s_write(0x30, 0x40c6u); /* + bit7: PA on (real power)     */

    /* Engine liveness / refill loop.  Poll the SYS_INTERP status latch while
     * keyed; ack everything seen via SYS_INTERP_CLEAR so a level-triggered
     * FM_TX_INTERP can't stall the ping-pong.  irqs counts status-nonzero
     * observations; swseen tracks FM_ADDR_SW movement.  In pump mode, each
     * FM_ADDR_SW flip refills the half the engine just released (fm_addr_sw:
     * 1 = CPU may access LOW buffer 0x030, 2 = HIGH buffer 0x230). */
    uint32_t irqs = 0, swseen = 0, swlast = *addrsw & 3u;
    if(pump || (flags & 0x10u))
    {
        for(unsigned t = 0; t < secs * 200u; ++t)      /* 5 ms cadence */
        {
            uint32_t st = *istat;
            if(st != 0u) { irqs++; *iclear = st; }
            uint32_t sw = *addrsw & 3u;
            if(sw != swlast)
            {
                swseen++; swlast = sw;
                if(pump)
                {
                    if(sw == 1u)      fm_chirp_fill(txram, 0x030u, &nco_phase, &nco_freq);
                    else if(sw == 2u) fm_chirp_fill(txram, 0x230u, &nco_phase, &nco_freq);
                }
            }
            Thread::sleep(5);
        }
    }
    else
    {
        for(unsigned t = 0; t < secs; ++t) Thread::sleep(1000);
    }

    hd2_at1846s_write(0x30, 0x4006u);      /* dekey                             */
    *ptt = 0u;
    *wm  = wm0;
    *imask = imask0;
    if(pump) { *audc = audc0; *fmbw = fmbw0; }
    if(flags & 0x08u) { *sigc = sigc0; *mbias = mbias0; }
    hd2_at1846s_write(0x40, (r40 != 0u) ? r40 : 0x0031u);
    hd2_at1846s_write(0x30, 0x4826u);      /* back to RX-on                     */
    g_rf_freeze = freeze0;

    snprintf(out, outsz,
             "FMTX%s secs=%u flags=%02x wm=%08lx sw=%lu irqs=%lu freq=%lu\r\n",
             pump ? "PUMP" : "TONE", secs, flags, (unsigned long)wm0,
             (unsigned long)swseen, (unsigned long)irqs, (unsigned long)nco_freq);
}

#if defined(HD2_M17_TXTEST)
/* Fill one 256-sample FM-TX-RAM ping-pong half from the M17 blob (looping).
 * 8-bit RAM, big-endian s16; >>1 for modulator headroom (keeps the +/-1 vs +/-3
 * symbol ratio intact -- deviation is set by FM_DEV_COEF, swept via the 'f' arg). */
static void fm_blob_fill(volatile uint8_t *txram, unsigned base, unsigned *pos,
                         unsigned shift)
{
    const int16_t *blob = reinterpret_cast<const int16_t *>(_m17_blob_start);
    const unsigned n = static_cast<unsigned>((_m17_blob_end - _m17_blob_start) / 2);
    for(unsigned i = 0; i < 256u; ++i)
    {
        int16_t s = static_cast<int16_t>(blob[*pos] >> shift);
        txram[base + 2u*i]      = static_cast<uint8_t>((s >> 8) & 0xff);
        txram[base + 2u*i + 1u] = static_cast<uint8_t>(s & 0xff);
        if(++(*pos) >= n) *pos = 0u;          /* loop the whole transmission */
    }
}

/* Play the embedded M17 frame on loop through the FM-TX-RAM pump (the proven
 * flag-0x40 keying path) so an external m17-demod can lock on our LSF and
 * confirm the TX physical layer.  `dev` = FM_DEV_COEF.fm_dev_coef_t (deviation
 * calibration knob -- sweep until m17-demod locks).  TRANSMITS RF for `secs`. */
static void m17_txtest(uint8_t secs, uint8_t dev, uint8_t opt, char *out, unsigned outsz)
{
    /* opt: bit0 = also clear fmbpfon (FM bandpass, FM_BANDWIDTH[6]) for a flatter
     *      4FSK path; bit1 = full blob amplitude (shift 0); bit2 = skip self-tune. */
    const unsigned shift   = (opt & 0x02u) ? 0u : 1u;
    const uint32_t fmbwclr = (opt & 0x01u) ? 0x50u : 0x10u;  /* +bit6 BPF if bit0 */
    volatile uint8_t  *txram = reinterpret_cast<volatile uint8_t *>(0x16000000u);
    volatile uint32_t *wm    = reinterpret_cast<volatile uint32_t*>(0x11000100u);
    volatile uint32_t *ptt   = reinterpret_cast<volatile uint32_t*>(0x11000560u);
    volatile uint32_t *audc  = reinterpret_cast<volatile uint32_t*>(0x11000080u);
    volatile uint32_t *fmbw  = reinterpret_cast<volatile uint32_t*>(0x11000500u);
    volatile uint32_t *fmdev = reinterpret_cast<volatile uint32_t*>(0x11000504u);
    volatile uint32_t *imask = reinterpret_cast<volatile uint32_t*>(0x1100039cu);
    volatile uint32_t *istat = reinterpret_cast<volatile uint32_t*>(0x11000398u);
    volatile uint32_t *iclear= reinterpret_cast<volatile uint32_t*>(0x110003b0u);
    volatile uint32_t *addrsw= reinterpret_cast<volatile uint32_t*>(0x1100056cu);

    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1;
    uint32_t wm0=*wm, audc0=*audc, fmbw0=*fmbw, fmdev0=*fmdev, imask0=*imask;

    /* Self-tune to analog-FM @ 433.450 MHz so the test is independent of the idle
     * channel.  opt bit2 SKIPS this -- use it when the radio is already on a proper
     * FM channel (let OpMode_FM's AT1846S config stand; a partial override regresses). */
    if((opt & 0x04u) == 0u) hd2_m17_fm_tune(433450000u);

    unsigned pos = 0;
    fm_blob_fill(txram, 0x030u, &pos, shift);      /* prime both halves */
    fm_blob_fill(txram, 0x230u, &pos, shift);

    *audc  = audc0 | 0x01u;                         /* tx_voice_source = TX RAM     */
    *fmbw  = (fmbw0 & ~fmbwclr) | 0x08u;            /* pre-emph OFF (+BPF off if opt&1), wide */
    *fmdev = (0x0fu << 16) | (0x10u << 8) | dev;    /* fm_dev_coef_t = dev (sweep)  */
    uint16_t apc, padrv; hd2_txpower_levels(&apc, &padrv);
    *reinterpret_cast<volatile uint32_t*>(0x140f0004u) &= ~0x2u;
    *reinterpret_cast<volatile uint32_t*>(0x140f0000u) &= ~0x2u;
    *reinterpret_cast<volatile uint32_t*>(0x140f000cu)  = apc;
    hd2_at1846s_write(0x0a, static_cast<uint16_t>((padrv << 11) | 0x0420u));
    *imask = 0x0001007fu;
    *wm    = wm0 | 0x80u;
    *ptt   = 1u;
    uint16_t r40 = hd2_at1846s_read(0x40);
    hd2_at1846s_write(0x40, 0x0030u);
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);
    hd2_at1846s_write(0x30, 0x40c6u);               /* PA on, real power            */

    uint32_t swseen=0, irqs=0, swlast = *addrsw & 3u;
    for(unsigned t = 0; t < secs * 200u; ++t)       /* 5 ms cadence */
    {
        uint32_t st = *istat; if(st != 0u) { irqs++; *iclear = st; }
        uint32_t sw = *addrsw & 3u;
        if(sw != swlast)
        {
            swseen++; swlast = sw;
            if(sw == 1u)      fm_blob_fill(txram, 0x030u, &pos, shift);
            else if(sw == 2u) fm_blob_fill(txram, 0x230u, &pos, shift);
        }
        Thread::sleep(5);
    }

    hd2_at1846s_write(0x30, 0x4006u);               /* dekey */
    *ptt=0u; *wm=wm0; *imask=imask0; *audc=audc0; *fmbw=fmbw0; *fmdev=fmdev0;
    hd2_at1846s_write(0x40, (r40 != 0u) ? r40 : 0x0031u);
    hd2_at1846s_write(0x30, 0x4826u);               /* back to RX-on */
    g_rf_freeze = freeze0;

    snprintf(out, outsz, "M17TX secs=%u dev=%u opt=%u sw=%lu irqs=%lu blob=%uS\r\n",
             secs, dev, opt, (unsigned long)swseen, (unsigned long)irqs,
             static_cast<unsigned>((_m17_blob_end - _m17_blob_start) / 2));
}
#endif

#if defined(HD2_M17_VOICE)
/* Fill one 256-sample FM-TX-RAM ping-pong half from an arbitrary 8 kHz s16
 * baseband buffer (looping).  Same byte packing + >>shift headroom as the blob
 * filler (fm_blob_fill); here the buffer is the live codec2->M17->Modulator
 * output rather than an embedded frame. */
static void m17_buf_fill(volatile uint8_t *txram, unsigned base, const int16_t *buf,
                         unsigned n, unsigned *pos, unsigned shift)
{
    for(unsigned i = 0; i < 256u; ++i)
    {
        int16_t s = static_cast<int16_t>(buf[*pos] >> shift);
        txram[base + 2u*i]      = static_cast<uint8_t>((s >> 8) & 0xff);
        txram[base + 2u*i + 1u] = static_cast<uint8_t>(s & 0xff);
        if(++(*pos) >= n) *pos = 0u;            /* loop the whole transmission */
    }
}

/* Stream a generated M17 8 kHz baseband buffer through the FM-TX-RAM ping-pong
 * pump.  The carrier KEYING is the shared canonical recipe (hd2_fm_tx_key, the
 * exact code radio_enableTx / OpMode_M17 keys) -- this op no longer hand-rolls
 * the FM_PTT/WORK_MODE/power/0x30 sequence.  What stays here is the HD2 SINK_RTX
 * layer: route the FM modulator off the mic ADC and onto CPU TX-RAM, flatten the
 * FM path for 4FSK, set the deviation sweep knob, and refill the ping-pong.
 * `dev` = FM_DEV_COEF deviation override (sweep knob); `opt` bit0 = also clear
 * the FM bandpass, bit1 = full amplitude (shift 0), bit2 = skip the 433.450
 * self-tune.  TRANSMITS RF for `secs` seconds. */
static void m17_pump_buf(const int16_t *buf, unsigned n, uint8_t secs, uint8_t dev,
                         uint8_t opt, uint32_t *swseen_o, uint32_t *irqs_o)
{
    const unsigned shift   = (opt & 0x02u) ? 0u : 1u;
    const uint32_t fmbwclr = (opt & 0x01u) ? 0x50u : 0x10u;
    volatile uint8_t  *txram = reinterpret_cast<volatile uint8_t *>(0x16000000u);
    /* SINK_RTX layer regs -- the ONLY radio writes this op owns (keying is the
     * shared hd2_fm_tx_key recipe). */
    volatile uint32_t *audc  = reinterpret_cast<volatile uint32_t*>(0x11000080u); /* AUDIO_CONTROL */
    volatile uint32_t *fmbw  = reinterpret_cast<volatile uint32_t*>(0x11000500u); /* FM_BANDWIDTH  */
    volatile uint32_t *fmdev = reinterpret_cast<volatile uint32_t*>(0x11000504u); /* FM_DEV_COEF   */
    volatile uint32_t *istat = reinterpret_cast<volatile uint32_t*>(0x11000398u);
    volatile uint32_t *iclear= reinterpret_cast<volatile uint32_t*>(0x110003b0u);
    volatile uint32_t *addrsw= reinterpret_cast<volatile uint32_t*>(0x1100056cu);

    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1;     /* rtx thread off the bus */
    uint32_t audc0=*audc, fmbw0=*fmbw, fmdev0=*fmdev;

    /* AT1846S FM-mode + narrow bandwidth + tune @ 433.450 (independent of the idle
     * channel).  opt bit2 SKIPS this -- use when already on a proper FM channel. */
    if((opt & 0x04u) == 0u) hd2_m17_fm_tune(433450000u);

    unsigned pos = 0;
    m17_buf_fill(txram, 0x030u, buf, n, &pos, shift);   /* prime both halves */
    m17_buf_fill(txram, 0x230u, buf, n, &pos, shift);

    /* SINK_RTX: select the TX-RAM source + linearize the FM path BEFORE keying so
     * the engine reads CPU PCM from the first FM_PTT edge. */
    *audc  = audc0 | 0x01u;                              /* tx_voice_source = TX RAM */
    *fmbw  = (fmbw0 & ~fmbwclr) | 0x08u;                 /* pre-emph OFF (+BPF off if opt&1), wide */
    *fmdev = (0x0fu << 16) | (0x10u << 8) | dev;         /* fm_dev_coef_t = dev (sweep) */

    /* Key the carrier via the shared canonical recipe -- narrow FM, no sub-audio
     * tone.  Same tune/engine-arm/band/power/0x59/0x40-0x30 sequence as the OpMode. */
    hd2_fm_tx_key(433450000u, /*narrow*/true, /*txToneEn*/0u, 0u, 0u, /*tailElim*/0u);

    uint32_t swseen=0, irqs=0, swlast = *addrsw & 3u;
    for(unsigned t = 0; t < secs * 200u; ++t)            /* 5 ms cadence */
    {
        uint32_t st = *istat; if(st != 0u) { irqs++; *iclear = st; }
        uint32_t sw = *addrsw & 3u;
        if(sw != swlast)
        {
            swseen++; swlast = sw;
            if(sw == 1u)      m17_buf_fill(txram, 0x030u, buf, n, &pos, shift);
            else if(sw == 2u) m17_buf_fill(txram, 0x230u, buf, n, &pos, shift);
        }
        Thread::sleep(5);
    }

    hd2_fm_tx_unkey();                                   /* shared canonical dekey/teardown */
    *audc=audc0; *fmbw=fmbw0; *fmdev=fmdev0;             /* restore SINK_RTX regs */
    hd2_at1846s_write(0x30, 0x4826u);                    /* RX-on (rtx thread re-confirms) */
    g_rf_freeze = freeze0;

    *swseen_o = swseen; *irqs_o = irqs;
}
#endif

/* DMR-TX bench bring-up (diag 'L'): static carrier.  Thin wrapper over the
 * SHARED canonical DMR-TX bring-up in dmr_HD2.cpp (hd2_dmr_tx_start) -- the
 * exact same code the OpMode keys -- so the register recipe lives in ONE
 * place (no more duplicated/divergent sequences).  Stages a fixed bench call
 * (CC1/TS1, group TG9, src 1234567), keys, holds, dekeys.  No per-slot pump
 * (see 'j').  `flags` = MOD1/MOD2 deviation amplitude (0 = vendor default
 * 0x19); `secs` = TX duration.  TRANSMITS RF at vendor-Low power. */
static void dmr_tx_carrier_test(uint8_t secs, uint8_t flags, char *out, unsigned outsz)
{
    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1u;   /* rtx off the bus */

    hd2_dmr_call_t call = { /*cc*/1u, /*ts*/0u, /*priv*/0u, /*src*/1234567u, /*dst*/9u };
    hd2_dmr_tx_call_start(&call);
    hd2_dmr_tx_start(flags);                            /* flags = MOD deviation */

    for(unsigned t = 0; t < secs; ++t) Thread::sleep(1000);

    hd2_dmr_tx_stop();
    g_rf_freeze = freeze0;

    snprintf(out, outsz, "DMRTX secs=%u amp=%02x (shared bring-up)\r\n",
             secs, flags ? flags : 0x19u);
}

/* DMR TS_TX burst PUMP + slot-engine DIAGNOSTIC (diag 'j').  Brings up the
 * SHARED canonical DMR-TX path (hd2_dmr_tx_start), drives the pump via the real
 * vec-0x3e ISR (production path), and -- crucially -- reads the modem's ACTUAL
 * slot-engine status registers (not the dead 0x3b0 ack-poll, which is write-1-
 * to-ack and can't observe TS_TX).  Reports:
 *   b    = vec-0x3e ISR bursts committed (does the TS_TX interrupt fire?)
 *   cnt  = LAYER2_STATUS(0x41c) tx_bit_cnt[8:0] min..max (counter ADVANCING?
 *          span>0 => the slot engine is clocking)
 *   b22  = RX_TYPE_INFO(0x390) bit22 TOGGLE vs static (per-slot bit)
 *   on404= LAYER2_SLOTON(0x404) readback; bit31 o_dll_tx_slot_on = engine
 *          accepted the start
 *   400  = LAYER2_CONTROL low byte (did 0xdb / tx_master_mode stick?)
 *   398  = SYS_INTERP_LIST accumulated (any modem interrupts asserting?)
 * `flags` = MOD deviation amplitude.  TRANSMITS RF. */
static void dmr_tx_pump_test(uint8_t secs, uint8_t flags, char *out, unsigned outsz)
{
    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1u;

    hd2_dmr_call_t call = { /*cc*/1u, /*ts*/0u, /*priv*/0u, /*src*/1234567u, /*dst*/9u };
    hd2_dmr_tx_call_start(&call);
    hd2_dmr_tx_start(flags);

    hd2_dmr_irq_enable();                          /* real vec-0x3e ISR pump */

    volatile uint32_t *r404 = reinterpret_cast<volatile uint32_t*>(0x11000404u);
    volatile uint32_t *r41c = reinterpret_cast<volatile uint32_t*>(0x1100041cu);
    volatile uint32_t *r390 = reinterpret_cast<volatile uint32_t*>(0x11000390u);
    volatile uint32_t *r400 = reinterpret_cast<volatile uint32_t*>(0x11000400u);
    volatile uint32_t *r398 = reinterpret_cast<volatile uint32_t*>(0x11000398u);

    volatile uint32_t *r3a0 = reinterpret_cast<volatile uint32_t*>(0x110003a0u);
    uint32_t ctrl = *r400;
    uint32_t cntMin = 0x1ffu, cntMax = 0u, b22 = 0u, sysint = 0u;
    long long deadline = miosix::getTime() + (long long)secs * 1000000000LL;
    while(miosix::getTime() < deadline)
    {
        uint32_t c = (*r41c) & 0x1ffu;
        if(c < cntMin) cntMin = c;
        if(c > cntMax) cntMax = c;
        b22    |= ((*r390 >> 22) & 1u) ? 2u : 1u;  /* bit0=saw 0, bit1=saw 1 */
        /* SERVICE the modem sys-interrupt (vendor task_sysint): drain
         * SYS_INTERP_LIST(0x398), ack via SYS_INTERP_CLEAR(0x3a0).  The prior
         * run left 398 bit1 (DLL_RDY_RX) asserted+unserviced -> engine wedged
         * (FM AF_GATE lesson).  Does servicing it let the slot engine start? */
        uint32_t p = *r398;
        if(p) { sysint |= p; *r3a0 = p; }
        Thread::sleep(2);
    }
    uint32_t sloton = *r404;        /* AFTER servicing: did o_dll_tx_slot_on engage? */

    uint16_t bursts = hd2_dmr_tx_bursts();
    hd2_dmr_irq_disable();
    hd2_dmr_tx_stop();
    g_rf_freeze = freeze0;

    snprintf(out, outsz,
             "DMRPUMP b=%u cnt=%u..%u b22=%s on404=%08lx 400=%02lx 398=%08lx\r\n",
             bursts, (unsigned)cntMin, (unsigned)cntMax,
             (b22 == 3u) ? "TOGGLE" : "static",
             (unsigned long)sloton, (unsigned long)(ctrl & 0xffu),
             (unsigned long)sysint);
}

/*
 * DTMF transmit (diag 'T'): key a bare FM carrier and send an ASCII digit
 * string.  The AT1846S generates the tone pairs internally (reg 0x35/0x36) and
 * sums them into the FM modulation -- no C7000 voice path needed.  Fixed
 * on/off timing per digit.  TRANSMITS RF.  Mirrors fm_tx_tone_test's key/dekey.
 */
static void dtmf_tx_send(const char *s, unsigned n, uint16_t onMs, uint16_t offMs,
                         char *out, unsigned outsz)
{
    static const uint16_t rows[4] = { 697, 770, 852, 941 };
    static const uint16_t cols[4] = { 1209, 1336, 1477, 1633 };

    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1;
    uint16_t r40 = hd2_at1846s_read(0x40);

    hd2_at1846s_write(0x40, 0x0030u);
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);                                       // tx_on
    hd2_at1846s_write(0x3A, (hd2_at1846s_read(0x3A) & ~0x7000u) | 0x3000u); // tone1+tone2
    hd2_at1846s_write(0x57, hd2_at1846s_read(0x57) | 0x0001u);              // AFOUT=DTMF
    hd2_at1846s_write(0x79, hd2_at1846s_read(0x79) & ~0xC000u);             // dtmf_direct/tx=0

    Thread::sleep(60);   // let the carrier + far-end squelch/AGC settle before the first tone

    unsigned sent = 0;
    for(unsigned i = 0; i < n; ++i)
    {
        int r, c;
        switch(s[i])
        {
            case '1':r=0;c=0;break; case '2':r=0;c=1;break; case '3':r=0;c=2;break; case 'A':case 'a':r=0;c=3;break;
            case '4':r=1;c=0;break; case '5':r=1;c=1;break; case '6':r=1;c=2;break; case 'B':case 'b':r=1;c=3;break;
            case '7':r=2;c=0;break; case '8':r=2;c=1;break; case '9':r=2;c=2;break; case 'C':case 'c':r=2;c=3;break;
            case '*':r=3;c=0;break; case '0':r=3;c=1;break; case '#':r=3;c=2;break; case 'D':case 'd':r=3;c=3;break;
            default: continue;
        }
        hd2_at1846s_write(0x35, (uint16_t)(rows[r] * 10u));
        hd2_at1846s_write(0x36, (uint16_t)(cols[c] * 10u));
        hd2_at1846s_write(0x7A, hd2_at1846s_read(0x7A) | 0x8000u);          // dtmf_en=1
        Thread::sleep(onMs);
        hd2_at1846s_write(0x7A, hd2_at1846s_read(0x7A) & ~0x8000u);         // dtmf_en=0
        Thread::sleep(offMs);
        sent++;
    }

    hd2_at1846s_write(0x57, hd2_at1846s_read(0x57) & ~0x0001u);
    hd2_at1846s_write(0x3A, (hd2_at1846s_read(0x3A) & ~0x7000u) | 0x4000u); // mic
    hd2_at1846s_write(0x30, 0x4006u);                                       // dekey
    hd2_at1846s_write(0x40, (r40 != 0u) ? r40 : 0x0031u);
    hd2_at1846s_write(0x30, 0x4826u);                                       // RX-on
    g_rf_freeze = freeze0;
    snprintf(out, outsz, "DTMF tx sent=%u\r\n", sent);
}

/*
 * DTMF receive (diag 't'): enable the AT1846S decoder and drain up to 'max'
 * digits seen within a ~3 s window.  NOTE: the 0x67..0x76 Goertzel coefficients
 * are left at the chip default (12.8/25.6 MHz reference); the HD2 is 26 MHz, so
 * this op is the HW check of whether decode works at the default coefficients.
 */
static void dtmf_rx_read(uint8_t max, char *out, unsigned outsz)
{
    static const char tbl[16] = { '0','1','2','3','4','5','6','7',
                                  '8','9','A','B','C','D','*','#' };
    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1;
    hd2_at1846s_write(0x7A, 0x8018u);                  // dtmf_en + detect time

    char digits[33]; unsigned k = 0;
    if(max > 32u) max = 32u;
    for(unsigned t = 0; t < 300u && k < max; ++t)      // ~3 s @10 ms
    {
        uint16_t st = hd2_at1846s_read(0x7E);
        if((st & 0x0010u) != 0u)                        // dtmf_sample ready
        {
            digits[k++] = tbl[st & 0x0F];
            for(unsigned w = 0; w < 30u; ++w)           // wait for ready to clear
            {
                if((hd2_at1846s_read(0x7E) & 0x0010u) == 0u) break;
                Thread::sleep(5);
            }
        }
        Thread::sleep(10);
    }

    hd2_at1846s_write(0x7A, hd2_at1846s_read(0x7A) & ~0x8000u);  // disable
    g_rf_freeze = freeze0;
    digits[k] = 0;
    snprintf(out, outsz, "DTMF rx=%s (%u)\r\n", digits, k);
}

/* ==================================================================== *
 *  Vendor CPS (Ailunce / Dahua) protocol SERVER                        *
 *                                                                      *
 *  Serves the SAME wire protocol the vendor firmware speaks, on this   *
 *  SAME UART0 @57600, so a host (dmrconfig / pylunce) can read AND      *
 *  write the W25Q codeplug while OpenRTX runs.  Multiplexed into the    *
 *  diag loop: a CPS session is entered ONLY via the literal "GetVer"   *
 *  (the vendor CPS always opens with it) or a bare 0x68 frame byte, so  *
 *  it never collides per-byte with the single-letter diag ops.         *
 *                                                                      *
 *  Frame/reply formats reconciled from two sources:                    *
 *   1. Vendor fw RE (HD-GPS-HD2PA-C7000-V2.1.3-GPS.decrypted.bin, CK803S*
 *      XIP @0x03000000).  The binary frame handler is at VMA           *
 *      0x03035826..0x03035912 (NOT the earlier-assumed 0x0304xxxx,     *
 *      which is UI code).  It dispatches on byte[1] (0x0d/0x0f/0x31)    *
 *      over reply buffer base 0x00040504, appends a 0x10 terminator,    *
 *      and -- crucially -- rewrites reply byte[2]=0x02 ONLY on the      *
 *      b1=0x31 path (st.b r2(=2),(r4,0x2) @0x030358fe); the b1=0x0f     *
 *      path echoes byte[2] verbatim (no such store).                    *
 *   2. pylunce host side (vendor/pylunce/hd1_dump.py read_chunk,        *
 *      hd1_codeplug_write.py build_cmd_0f/build_cmd_31, docs/protocol.md)*
 *      which ACCEPTS both echo forms on 0x0f reads and REQUIRES         *
 *      byte[2]=0x02 on 0x31 write-acks -- consistent with the fw.       *
 *                                                                      *
 *  READ reply  (0x0f & 0x31): [10 header bytes] [size payload] [0x10]   *
 *      header = request bytes[0..9]; byte[2] -> 0x02 for 0x31 only.     *
 *  WRITE ack   (0x0f): 68 0f 01 01 [pct] 00 80 00 [alo][ahi] 10  (11B)  *
 *  WRITE ack   (0x31): 68 31 02 01 [pct] 31 00 10 [alo][ahi] 10  (11B)  *
 *                                                                      *
 *  Address mapping (cps_io_HD2.c / nvmem_vendor_records_HD2.c):         *
 *   b1=0x0f  physical = radio_addr + 0x790000  (flat; vfo_config        *
 *            0x2000->0x792000, settings 0x2900->0x792900).              *
 *   b1=0x31  physical = radio_addr << 10        (block idx; channels    *
 *            0x1BCC->0x6F3000, contacts 0x1B84->0x6E1000, zones         *
 *            0x21D8->0x886000).                                         *
 * ==================================================================== */

namespace cps {

constexpr uint8_t  CPS_SYNC      = 0x68u;
constexpr uint8_t  CPS_TERM      = 0x10u;
constexpr uint32_t CPS_0F_BASE   = 0x00790000u;   /* flat base for b1=0x0F */
/* Idle timeout: number of consecutive rxByte() failures (each ~250 ms) before
 * we abandon the session and fall back to the diag loop. */
constexpr int      CPS_IDLE_GIVEUP = 8;            /* ~2 s of silence */

/* ---- intra-frame byte I/O (the 57600-WRITE hang fix) -------------------- *
 *
 * The shared sleepy rxByte() (poll ~256 spins, then Thread::sleep(1) per ms,
 * up to 250 ms) is fine to WAIT for a host between frames, but it must NOT be
 * used INSIDE a frame: at 57600 a 0x31 WRITE streams 4096 payload bytes back
 * to back (~0.71 s of continuous traffic).  Yielding for a whole millisecond
 * mid-frame lets the UART0 RX FIFO (16-deep DesignWare) overrun -- bytes are
 * silently dropped -- and a single >250 ms stall trips the timeout, aborting
 * the frame and desyncing the parser.  So every read WITHIN a frame uses the
 * busy-poll helper below: it drains the FIFO at full speed and only tolerates
 * small inter-byte gaps, bounded by a real wall-clock deadline so a genuinely
 * dead link still returns instead of spinning forever.
 *
 * Reads do NOT correspond per-byte to the download (TX) path, which is why
 * downloads never hung: there the server is TRANSMITTING the bulk payload. */

/* Per-byte inter-arrival budget: how long we busy-wait for the NEXT byte of a
 * frame already in progress.  ~1.5 s comfortably exceeds any sane gap between
 * consecutive bytes at 57600 (~174 us/byte) yet still releases a stalled host.
 * The whole 4 KB frame thus tolerates pauses without ever sleeping mid-stream. */
constexpr long long CPS_BYTE_DEADLINE_NS = 1500000000LL;   /* 1.5 s, in ns */

/* Read one byte by busy-polling LSR_DR with NO Thread::sleep, bounded by a
 * wall-clock deadline (miosix::getTime() is monotonic ns, already linked via
 * <miosix.h>).  Returns false only if the link truly stalls past the deadline.
 * Use for ALL reads inside serve_frame so the RX FIFO is drained at line rate. */
static bool rxByteFast(uint8_t &out)
{
    const long long deadline = miosix::getTime() + CPS_BYTE_DEADLINE_NS;
    do {
        if((UART0_LSR & UART_LSR_DR) != 0u) { out = UART0_RBR & 0xFFu; return true; }
    } while(miosix::getTime() < deadline);
    return false;
}

/* Resync after a malformed/timed-out frame: busy-read and DISCARD bytes until
 * the link has been quiet for ~a few ms, so the next genuine 0x68 SYNC starts a
 * clean frame instead of the loop misreading leftover payload as opcodes.  No
 * Thread::sleep -- a partial frame's tail arrives within microseconds at 57600,
 * so a short no-byte window means the host has stopped. */
static void drain(void)
{
    constexpr long long QUIET_NS = 3000000LL;   /* 3 ms of silence == link idle */
    long long quietUntil = miosix::getTime() + QUIET_NS;
    for(;;)
    {
        if((UART0_LSR & UART_LSR_DR) != 0u)
        {
            (void) (UART0_RBR & 0xFFu);                       /* drop the byte */
            quietUntil = miosix::getTime() + QUIET_NS;        /* restart the gap */
        }
        else if(miosix::getTime() >= quietUntil)
        {
            return;
        }
    }
}

/* Ensure the UART0 RX FIFO is on at session entry, with a sane RX trigger.
 * The bsp already enables it (FCR=0x07: FIFO-en + flush both) at boot, so this
 * is normally redundant -- but re-asserting it once per session is cheap
 * insurance and lets us raise the RX trigger to 8 bytes for extra slack.  We do
 * NOT set the reset bits (1<<1 / 1<<2) here so no byte already queued by the
 * host is flushed at entry.  FCR is write-only (reads alias IIR), so we just
 * write the desired control word.
 *   bit0=1   FIFO enable
 *   bits6:7  RX trigger: 0b10 = 8 bytes (DesignWare 16-deep FIFO) */
static void fifo_enable(void)
{
    UART0_FCR = 0x01u | (0x2u << 6);   /* 0x81: FIFO-en, RX trigger = 8 bytes */
}

/* GetVer reply: 35-byte vendor-shape filename id (mimics the vendor
 * "HD-GPS-HD2PA-C7000-V*.bin" so the host's `b"HD" in ver` check passes).
 * Vendor string @VMA 0x0306df00 is "HD-GPS-HD2PA-C7000-V2.1.3-GPS.bin".  We
 * stamp an OpenRTX marker in so a host operator can tell the firmwares apart,
 * while keeping "HD" present and the field 35 bytes, NUL-padded. */
static const char CPS_GETVER[35] = "HD2-OPENRTX-C7000-V0.1.bin";

/* Shared 4 kB sector scratch for all CPS flash writes.  Keeps the big buffer
 * off the small diag stack AND out of the static-RAM budget that a per-write
 * buffer would add (the >~12-16 kB static hangs-boot constraint).  Single-user:
 * a CPS session is single-threaded on the diag thread, with the rtx/FM workers
 * frozen via g_rf_freeze. */
static uint8_t g_cps_sec[W25Q_HD2_SECTOR_SIZE];

/* LED rx/tx feedback for the PC-Mode codeplug session.  GREEN = a frame was
 * received from the host, RED = a reply/ACK was transmitted back.  Only active
 * while g_cps_pc_mode is set (the LEDs are otherwise owned by the rtx RX/PTT
 * path); when not in PC mode these are no-ops so the existing diag-gated CPS
 * entry does not stomp the radio's LEDs.  Both are bare GPIOB register writes
 * (platform_ledOn/Off) with no blocking -- safe to call inline in serve_frame.
 * Counters drive the on-screen frame tally. */
static inline void cps_led_rx(void)
{
    if(g_cps_pc_mode) { platform_ledOff(RED); platform_ledOn(GREEN); }
    g_cps_rx_frames++;
}
static inline void cps_led_tx(void)
{
    if(g_cps_pc_mode) { platform_ledOff(GREEN); platform_ledOn(RED); }
    g_cps_tx_replies++;
}

/* RMW-write `len` bytes of `data` to absolute flash `addr`, erasing every 4 kB
 * sector the range touches (mirrors nvmem_vendor_records_HD2.c::
 * hd2_vendor_channel_write and cps_io_HD2.c::bitmap_write).  Returns 0 on
 * success. */
static int flash_rmw(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if(!w25q_hd2_probe()) return -1;
    const uint32_t end = addr + len;
    for(uint32_t base = addr & ~(W25Q_HD2_SECTOR_SIZE - 1u);
        base < end; base += W25Q_HD2_SECTOR_SIZE)
    {
        w25q_hd2_read(base, g_cps_sec, sizeof(g_cps_sec));
        uint32_t lo = (addr > base) ? addr : base;
        uint32_t hi = (end < base + W25Q_HD2_SECTOR_SIZE) ? end
                                                          : base + W25Q_HD2_SECTOR_SIZE;
        memcpy(g_cps_sec + (lo - base), data + (lo - addr), hi - lo);
        if(w25q_hd2_eraseSector(base) < 0)                       return -1;
        if(w25q_hd2_program(base, g_cps_sec, sizeof(g_cps_sec)) < 0) return -1;
    }
    return 0;
}

/* radio addr -> physical W25Q byte address, per b1 family. */
static inline uint32_t phys_addr(uint8_t b1, uint16_t radio_addr)
{
    return (b1 == 0x31u) ? ((uint32_t) radio_addr << 10)
                         : ((uint32_t) radio_addr + CPS_0F_BASE);
}

/*
 * Serve a single 0x68 frame whose SYNC byte has ALREADY been consumed.  `b1`
 * (the command-family byte) is read here; we then pull the rest of the 11-byte
 * header, act on it, and emit the vendor reply.  Returns true if a recognised
 * frame was handled (session continues), false on a malformed/timed-out frame
 * (caller may give up).  Large payloads stream straight from / to the flash via
 * a small static window, so the diag stack and RAM stay tiny (memory-constraints
 * memory: never buffer megabytes).
 */
static bool serve_frame(char first /* unused: SYNC already eaten */)
{
    (void) first;
    uint8_t hdr[10];
    hdr[0] = CPS_SYNC;
    /* Read header bytes [1..9] (we already consumed [0]=0x68).  Intra-frame:
     * busy-poll, never Thread::sleep -- see rxByteFast rationale above. */
    for(int i = 1; i < 10; ++i)
        if(!rxByteFast(hdr[i])) return false;

    cps_led_rx();   /* GREEN: a full frame header arrived from the host */

    const uint8_t  b1   = hdr[1];
    const uint8_t  dir  = hdr[2];                       /* 0x00 read, 0x01 write */
    const uint16_t size = (uint16_t)(hdr[6] | (hdr[7] << 8));
    const uint16_t raddr= (uint16_t)(hdr[8] | (hdr[9] << 8));

    /* The b1=0x31 read frame carries size=0x0400 in [6:7]; the b1=0x0F read
     * frame carries the byte size (0x80 etc).  Writes carry size in the same
     * field (0x80 for 0x0F, 0x1000 for 0x31). */

    if(b1 == 0x0Fu || b1 == 0x0Du)                      /* byte-addressed family */
    {
        if(dir == 0x00u)                                /* READ */
        {
            uint8_t trail;
            if(!rxByteFast(trail)) return false;        /* consume terminator */
            uint32_t phys = phys_addr(0x0Fu, raddr);
            /* Reply header: echo bytes[0..9] verbatim (byte[2] stays 0x00 on
             * the 0x0F path -- fw 0x03035870 path has no st.b to (r4,2)). */
            cps_led_tx();   /* RED: replying to the host */
            for(int i = 0; i < 10; ++i) tx(hdr[i]);
            /* Stream the payload from flash through a small window. */
            uint8_t win[64];
            uint32_t left = size, off = 0;
            while(left)
            {
                uint32_t n = (left < sizeof(win)) ? left : (uint32_t)sizeof(win);
                if(w25q_hd2_probe()) w25q_hd2_read(phys + off, win, n);
                else memset(win, 0xFF, n);
                for(uint32_t i = 0; i < n; ++i) tx(win[i]);
                off += n; left -= n;
            }
            tx(CPS_TERM);
            return true;
        }
        else                                            /* WRITE (dir 0x01) */
        {
            /* Pull `size` payload bytes (sector RMW after) + terminator. */
            uint32_t phys = phys_addr(0x0Fu, raddr);
            static uint8_t pay[256];                    /* 0x0F size is 0x80 */
            if(size > sizeof(pay)) return false;
            for(uint16_t i = 0; i < size; ++i)
                if(!rxByteFast(pay[i])) return false;   /* bulk: full-rate poll */
            uint8_t trail;
            if(!rxByteFast(trail)) return false;        /* terminator */
            (void) flash_rmw(phys, pay, size);
            /* ACK = echo of header bytes[0..9] verbatim + 0x10 (byte[2]=0x01). */
            cps_led_tx();   /* RED: write-ack to the host */
            for(int i = 0; i < 10; ++i) tx(hdr[i]);
            tx(CPS_TERM);
            return true;
        }
    }
    else if(b1 == 0x31u)                                /* block-addressed family */
    {
        if(dir == 0x00u)                                /* READ (1024 B) */
        {
            uint8_t trail;
            if(!rxByteFast(trail)) return false;        /* terminator */
            uint32_t phys = phys_addr(0x31u, raddr);
            /* Reply header: bytes[0..9] with byte[2] forced 0x02 (fw
             * 0x030358fe: st.b r2(=2),(r4,2) on the 0x31 path). */
            uint8_t rep[10];
            memcpy(rep, hdr, 10); rep[2] = 0x02u;
            cps_led_tx();   /* RED: replying to the host */
            for(int i = 0; i < 10; ++i) tx(rep[i]);
            uint16_t rdsize = (size != 0u) ? size : 0x0400u;
            uint8_t win[64];
            uint32_t left = rdsize, off = 0;
            while(left)
            {
                uint32_t n = (left < sizeof(win)) ? left : (uint32_t)sizeof(win);
                if(w25q_hd2_probe()) w25q_hd2_read(phys + off, win, n);
                else memset(win, 0xFF, n);
                for(uint32_t i = 0; i < n; ++i) tx(win[i]);
                off += n; left -= n;
            }
            tx(CPS_TERM);
            return true;
        }
        else                                            /* WRITE (4096 B) */
        {
            /* size field is 0x1000 = one full erase sector.  Stream straight
             * into the shared sector scratch and erase+program it: a 4 kB 0x31
             * write maps to exactly one 4 kB sector (addr<<10 is sector-aligned
             * for any block index multiple of 4 -- the host steps +4), and the
             * host sends the WHOLE block, so no read-modify is needed. */
            uint32_t phys  = phys_addr(0x31u, raddr);
            uint32_t total = (size != 0u) ? size : 0x1000u;
            if(total > sizeof(g_cps_sec)) return false;
            for(uint32_t i = 0; i < total; ++i)
                if(!rxByteFast(g_cps_sec[i])) return false;  /* 4 KB at line rate */
            uint8_t trail;
            if(!rxByteFast(trail)) return false;        /* terminator */
            if(w25q_hd2_probe())
            {
                /* Sector-aligned full overwrite. */
                if(w25q_hd2_eraseSector(phys) == 0)
                    (void) w25q_hd2_program(phys, g_cps_sec, total);
            }
            /* ACK: byte[2] forced 0x02 + 0x10 (matches pylunce 68 31 02 01..). */
            uint8_t ack[10];
            memcpy(ack, hdr, 10); ack[2] = 0x02u;
            cps_led_tx();   /* RED: write-ack to the host */
            for(int i = 0; i < 10; ++i) tx(ack[i]);
            tx(CPS_TERM);
            return true;
        }
    }

    /* Unknown family: ignore the frame body we can't size; resync on the next
     * SYNC.  (Setup probes the vendor CPS sends -- e.g. clock-set 0x68 0x0F
     * 0x05 -- fall here harmlessly; our dmrconfig client does not send them.) */
    return true;
}

/*
 * CPS session loop.  Entered after the gate matched "GetVer" (then `primed`
 * carries the GetVer reply) or after a bare 0x68 frame byte (then `pending68`
 * is true and `serve_frame` runs immediately).  Serves CPS frames until "END",
 * an idle timeout, or repeated malformed frames, then returns to the diag loop.
 *
 * PC-Mode entry: the diag loop calls session(false,false) when g_cps_pc_mode is
 * set (the UI "PC Mode" screen).  In that case there is no per-byte gate to fall
 * back to, the UI has ALREADY set g_rf_freeze=1, and the session must persist
 * across long idle gaps (the operator may sit on the screen with no host
 * connected) -- so the idle timeout is disabled while g_cps_pc_mode is set, and
 * the loop returns the instant the UI clears the flag (the screen's exit key).
 */
static void session(bool primed_getver, bool pending68)
{
    const bool pc_mode = (g_cps_pc_mode != 0u);

    g_rf_freeze = 1;                 /* hold the rtx/FM workers off the W25Q+I2C
                                      * bus for the whole codeplug session */

    fifo_enable();                   /* (re)assert UART0 RX FIFO + 8-byte trigger
                                      * for slack against the 4 KB WRITE stream */

    if(primed_getver)
        for(unsigned i = 0; i < sizeof(CPS_GETVER); ++i)
            tx((uint8_t) CPS_GETVER[i]);

    int idle = 0;
    if(pending68) { if(!serve_frame(0x68)) drain(); }   /* resync on a bad frame */

    for(;;)
    {
        /* PC mode owns the session lifetime: exit the moment the UI clears the
         * flag (Back/ESC on the screen).  Never time out on idle in PC mode. */
        if(pc_mode && g_cps_pc_mode == 0u) break;

        uint8_t c;
        if(!rxByte(c))               /* ~250 ms with no byte */
        {
            if(pc_mode) continue;    /* stay resident until the UI exits */
            if(++idle >= CPS_IDLE_GIVEUP) break;
            continue;
        }
        idle = 0;

        switch(c)
        {
            case CPS_SYNC:                       /* 0x68 binary frame */
                /* On a real read failure (dropped/late byte mid-frame), drain
                 * leftover payload bytes so the link goes quiet before we look
                 * for the next genuine 0x68 -- otherwise the loop would
                 * misinterpret payload tail bytes as opcodes and desync. */
                if(!serve_frame(0x68)) drain();
                break;

            case 'G':                            /* GetVer again mid-session */
            {
                uint8_t g[5];
                bool ok = true;
                for(int i = 0; i < 5 && ok; ++i) ok = rxByte(g[i]);
                if(ok && g[0]=='e'&&g[1]=='t'&&g[2]=='V'&&g[3]=='e'&&g[4]=='r')
                    for(unsigned i = 0; i < sizeof(CPS_GETVER); ++i)
                        tx((uint8_t) CPS_GETVER[i]);
                break;
            }

            case 'S':                            /* SLC7000 -> model id */
            {
                uint8_t s[6];
                bool ok = true;
                for(int i = 0; i < 6 && ok; ++i) ok = rxByte(s[i]);
                /* "SLC7000": reply a vendor-shape model id (must contain "HD"
                 * so the host write handshake proceeds; the vendor answers
                 * "BJDR380" but the host only checks for a sane non-empty
                 * reply, not that literal). */
                if(ok && s[0]=='L'&&s[1]=='C'&&s[2]=='7'&&s[3]=='0'&&s[4]=='0'&&s[5]=='0')
                    txStr("BJDR380");
                break;
            }

            case 'E':                            /* "END" -> finalize + exit */
            {
                uint8_t e[2];
                bool ok = rxByte(e[0]) && rxByte(e[1]);
                if(ok && e[0]=='N' && e[1]=='D')
                {
                    /* Writes are already persisted to flash (RMW per frame), so
                     * END just closes the session.  The vendor fw reboots here;
                     * we do NOT -- OpenRTX picks up the edited codeplug on its
                     * next cps_read*, and a reboot would drop the diag link the
                     * operator is sharing.  In PC mode the operator is still on
                     * the screen, so END does NOT leave -- we stay resident and
                     * keep the bus frozen until the UI exit key clears
                     * g_cps_pc_mode (avoids a freeze/unfreeze flap between back-
                     * to-back host sessions).  Outside PC mode, exit to the diag
                     * loop as before. */
                    if(!pc_mode) goto done;
                }
                break;
            }

            default:
                /* stray byte (host drain noise) -> ignore, resync */
                break;
        }
    }

done:
    /* In PC mode the UI owns g_rf_freeze (set on screen entry, cleared on exit)
     * and the LEDs; leaving them as-is avoids a contention window if the diag
     * loop re-enters the session while the screen is still up.  Outside PC mode
     * (the legacy GetVer/0x68-gated entry) we unfreeze the radio here as before;
     * the LEDs were only touched if g_cps_pc_mode happened to be set, so no
     * restore is needed on the non-PC-mode path. */
    if(!pc_mode)
        g_rf_freeze = 0;
}

} // namespace cps

#if defined(HD2_M17)
/* The codec2_t state is ~31 KiB -- large against this RAM.  The 'i' self-test
 * and the 'g' voice op never run concurrently (both are spawned from the diag
 * dispatch and pthread_join'd), so they SHARE this one static instance; a second
 * copy would starve the boot heap (memory hd2-memory-constraints) even under the
 * low-RAM hd2_m17.ld script. */
static codec2_t g_hd2_codec2;

/* Codec2-3200 encode self-test (M17 Phase 1), run on a 16 KiB-stack worker --
 * codec2's float analysis chain + ~KB of create-time mallocs overflow the diag
 * thread's 2 KiB stack.  Confirms codec2 links + runs on this soft-float CK803,
 * MEASURES the steady-state per-frame encode cost against the 20 ms M17 budget,
 * and checks encode is non-trivial (tone vs silence outputs differ).  Diag is
 * blocked in pthread_join while this runs -> single console writer. */
void *codec2_selftest_worker(void *)
{
    char line[128];
    codec2_t &c2 = g_hd2_codec2;       /* shared ~31 KiB static (Codec2-mod is malloc-free) */

    txStr("C2 A: start\r\n");
    /* Heap is no longer the gate (static state), but log it for comparison. */
    snprintf(line, sizeof line, "C2 heap free=%u total=%u\r\n",
             miosix::MemoryProfiling::getCurrentFreeHeap(),
             miosix::MemoryProfiling::getHeapSize());
    txStr(line);

    codec2_init(&c2);
    const int nsam  = CODEC2_SAMPLES_PER_FRAME;       /* 160 @ 3200 (20 ms) */
    const int nbyte = CODEC2_BYTES_PER_FRAME;         /* 8                  */
    snprintf(line, sizeof line, "C2 B: init nsam=%d nbyte=%d\r\n", nsam, nbyte);
    txStr(line);

    static int16_t pcm_tone[160];                     /* static: keep worker stack small */
    static int16_t pcm_sil[160];
    for(int i = 0; i < nsam; ++i)
    {
        pcm_tone[i] = (int16_t)(8000.0f * sinf(2.0f * 3.14159265f * (float)i / 8.0f));
        pcm_sil[i]  = 0;                               /* 1 kHz @ 8 kHz = 8 samp/cycle */
    }

    uint8_t bt[16] = {0}, bs[16] = {0};
    long long a0 = miosix::getTime();
    codec2_encode(&c2, bt, pcm_tone);                 /* first encode (timed alone) */
    long long a1 = miosix::getTime();
    snprintf(line, sizeof line, "C2 C: enc0=%lldus tone=%02x%02x%02x\r\n",
             (a1 - a0) / 1000, bt[0], bt[1], bt[2]);
    txStr(line);

    const int N = 8;
    long long t0 = miosix::getTime();
    for(int k = 0; k < N; ++k) codec2_encode(&c2, bt, pcm_tone);
    long long t1 = miosix::getTime();
    codec2_encode(&c2, bs, pcm_sil);

    long long us = (t1 - t0) / 1000 / N;
    bool differ  = (memcmp(bt, bs, nbyte) != 0);
    snprintf(line, sizeof line,
             "C2 nsam=%d nbyte=%d enc=%lldus/frame (budget=20000) "
             "tone=%02x%02x%02x sil=%02x%02x%02x %s\r\n",
             nsam, nbyte, us, bt[0], bt[1], bt[2], bs[0], bs[1], bs[2],
             differ ? "DIFFER-ok" : "SAME-noop!");
    txStr(line);

#ifdef C2_PROFILE
    snprintf(line, sizeof line,
             "C2 PROF analyse2x=%lldus lsp=%lldus quant=%lldus\r\n",
             (c2_prof_t[1] - c2_prof_t[0]) / 1000,
             (c2_prof_t[2] - c2_prof_t[1]) / 1000,
             (c2_prof_t[3] - c2_prof_t[2]) / 1000);
    txStr(line);
#endif

    return nullptr;
}

#endif

#if defined(HD2_M17_VOICE)
/* Diag op 's' worker: M17 voice TX via the HR_C7000 NATIVE 4FSK physical-layer
 * (Layer-1) path -- the architecturally-correct lever past the 8 kHz FM-pump wall
 * (which only ever LICH-synced, never locked the LSF).  codec2 (C2_FIXED) + the
 * OpenRTX M17 FrameEncoder build preamble + LSF + stream + EOT as a continuous
 * DIBIT BYTE stream; M17 frame_t bytes are already the PHY's 2-bit symbol format
 * (byteToSymbols LUT {+1,+3,-1,-3} MSB-first == manual §9.5.2), so there is NO
 * Modulator / 48 kHz baseband -- the bytes feed the PHY ping-pong verbatim and the
 * hardware emits 4FSK at the native 38.4 kHz / 8 samples-per-symbol.  PHY bring-up
 * lives in dmr_HD2.cpp (hd2_m17_phy_*), reusing the proven DMR two-point RF
 * backend.  16 KiB worker; diag blocked in pthread_join (single console writer).
 * TRANSMITS RF for `secs`.  args: dev = RF_MOD_BIAS reduce (LOWER = more deviation;
 * sweep for M17 ±2.4/±0.8 kHz); opt b2 = skip the 433.450 self-tune. */
static volatile uint8_t g_m17v_secs = 6u, g_m17v_dev = 0x10u, g_m17v_opt = 0u;

void *m17_voice_worker(void *)
{
    char line[128];
    const uint8_t secs = g_m17v_secs, dev = g_m17v_dev, opt = g_m17v_opt;

    /* Build the whole transmission as a continuous dibit byte stream:
     *   preamble (0x77 = +3,-3,+3,-3) + LSF(48 B) + nStream*48 B + EOT(48 B).
     * frame_t includes the M17 sync word, so this is the complete on-air symbol
     * sequence.  ~1 s; the pump loops it for `secs`.  Heap is tiny (~1.4 KiB) --
     * no Modulator baseband buffer here (baseband generation is skipped). */
    codec2_t &c2 = g_hd2_codec2;              /* shared ~31 KiB static (see 'i' self-test) */
    static int16_t pcm[CODEC2_SAMPLES_PER_FRAME];
    for(int i = 0; i < CODEC2_SAMPLES_PER_FRAME; ++i)
        pcm[i] = static_cast<int16_t>(8000.0f * sinf(2.0f * 3.14159265f * (float)i / 8.0f));

    const unsigned nStream = 25u;             /* 25 * 40 ms = 1.0 s of voice */
    const unsigned PRE     = 96u;             /* 96 B = 384 sym = 80 ms preamble (0x77) */
    const unsigned slen    = PRE + 48u * (1u + nStream + 1u);   /* pre + LSF + stream + EOT */
    uint8_t *bs = static_cast<uint8_t *>(malloc(slen));
    if(bs == nullptr) { txStr("M17PHY malloc fail\r\n"); return nullptr; }

    codec2_init(&c2);
    M17::LinkSetupFrame lsf;
    lsf.clear();
    lsf.setSource(M17::Callsign("AB1CDE"));
    lsf.setDestination(M17::Callsign("N0CALL"));
    M17::streamType_t type;
    type.value           = 0;
    type.fields.dataMode = M17::DATAMODE_STREAM;
    type.fields.dataType = M17::DATATYPE_VOICE;
    type.fields.CAN      = 0;
    lsf.setType(type);

    static M17::FrameEncoder encoder;         /* static: keep worker stack small */
    M17::frame_t   fr;
    M17::payload_t payload;
    uint8_t        bits[2][CODEC2_BYTES_PER_FRAME];
    unsigned w = 0;

    for(unsigned i = 0; i < PRE; ++i) bs[w++] = 0x77u;       /* preamble symbols */
    encoder.reset();
    encoder.encodeLsf(lsf, fr);
    memcpy(bs + w, fr.data(), 48); w += 48u;                 /* LSF (incl. sync word) */
    for(unsigned f = 0; f < nStream; ++f)
    {
        codec2_encode(&c2, bits[0], pcm);     /* 2x 20 ms codec frames per M17 frame */
        codec2_encode(&c2, bits[1], pcm);
        memcpy(payload.data(),     bits[0], CODEC2_BYTES_PER_FRAME);
        memcpy(payload.data() + 8, bits[1], CODEC2_BYTES_PER_FRAME);
        encoder.encodeStreamFrame(payload, fr, (f == nStream - 1u));
        memcpy(bs + w, fr.data(), 48); w += 48u;
    }
    encoder.encodeEotFrame(fr);
    memcpy(bs + w, fr.data(), 48); w += 48u;

    snprintf(line, sizeof line, "M17PHY gen bytes=%u heapfree=%u\r\n",
             w, miosix::MemoryProfiling::getCurrentFreeHeap());
    txStr(line);

    /* ---- TX: arm the PHY-L1 modulator + poll-refill the symbol ping-pong ----
     * The refill cadence is the one piece with no firmware precedent (every
     * vendor L1-TX site stages once for a static BER tone).  First cut: poll
     * LAYER2_STATUS -- [31] tx_slot_choose is the bank toggle (the L1 analog of
     * the FM pump's FM_ADDR_SW); [8:0] tx_bit_cnt is the engine-liveness counter.
     * On each toggle, refill the just-released bank with the next 36 bytes.  The
     * strobes/refills/bit_cnt readout is the bring-up instrumentation: strobes==0
     * => the slot engine isn't clocking the banks in this mode (next: try
     * WORK_MODE=0x02, or add the slot-engine start); bit_cnt advancing confirms
     * the modulator is consuming symbols.  TEMP: this instrumentation can be
     * trimmed once the on-air decode is confirmed. */
    uint32_t freeze0 = g_rf_freeze; g_rf_freeze = 1;
    hd2_m17_fm_tune(433450000u);                             /* freq/band (always) */

    hd2_m17_phy_stage(0, bs + 0);             /* prime both banks */
    hd2_m17_phy_stage(1, bs + M17_PHY_FRAME_BYTES);
    unsigned pos = 2u * M17_PHY_FRAME_BYTES;  /* next stream byte to emit */
    hd2_m17_phy_tx_start(dev);

    /* ABSOLUTE-TIME refill.  Continuous L1 mode exposes NO bank-consumption
     * strobe (TS_TX/MODEM_IRQ/SLOT_PRE/LAYER2_STATUS all stayed static on
     * hardware), so refill is time-paced -- but with ABSOLUTE deadlines
     * (t0 + N*30 ms), not cumulative sleep(30) (which drifts slow and tears
     * frames -> LICH only).  The modem symbol clock (38.4 kHz) and the CPU timer
     * both divide the 24 MHz crystal, so they're coherent: a fixed-phase
     * absolute schedule stays aligned.  Each 144-symbol bank = 30 ms; refill the
     * bank the engine just finished (bank0 at t0+30 while it reads bank1, ...).
     * opt = the refill PHASE within the 30 ms bank window, in MILLISECONDS
     * (0..29) -- sweep it to find the phase that doesn't tear (the per-bank
     * striping seen on the SDR) and locks the LSF. */
    const long long BANK_NS = 30000000LL;
    uint32_t refills = 0, strobes = 0;
    unsigned bank = 0u;
    uint8_t  chunk[M17_PHY_FRAME_BYTES];
    long long t0   = miosix::getTime();
    long long ph   = (long long)(opt % 30u) * 1000000LL;   /* opt ms phase, 0..29 */
    unsigned  n    = 1u;
    long long next = t0 + BANK_NS + ph;
    long long end  = t0 + (long long)secs * 1000000000LL;
    while(miosix::getTime() < end)
    {
        if(miosix::getTime() >= next)
        {
            for(unsigned k = 0; k < M17_PHY_FRAME_BYTES; ++k)
            { chunk[k] = bs[pos]; if(++pos >= slen) pos = 0u; }
            hd2_m17_phy_stage(bank, chunk);
            bank ^= 1u; ++refills; ++n;
            next = t0 + (long long)n * BANK_NS + ph;    /* absolute -> no drift */
        }
        Thread::sleep(1);
    }
    uint32_t bcMin = 0, bcMax = 0;                 /* (no L1 strobe to report) */

    hd2_m17_phy_tx_stop();
    hd2_at1846s_write(0x30, 0x4826u);         /* RX-on (rtx thread re-confirms) */
    g_rf_freeze = freeze0;
    free(bs);

    snprintf(line, sizeof line,
             "M17PHY secs=%u dev=%u opt=%u bytes=%u strobes=%lu refills=%lu bc=%lu..%lu st=%08lx\r\n",
             secs, dev, opt, w, (unsigned long)strobes, (unsigned long)refills,
             (unsigned long)bcMin, (unsigned long)bcMax, (unsigned long)hd2_m17_phy_status());
    txStr(line);
    return nullptr;
}
#endif

#if defined(HD2_DMR_VOICE)
/* AMBE self-test body, run on a dedicated 16 KiB-stack thread: the codec's
 * encode/analysis call chain has deep, large stack frames that overflow the
 * diag thread's 2 KiB default stack (delayed corruption -> WDT).  Staged prints
 * localise a fault; diag is blocked in pthread_join while this runs, so the
 * shared console UART has a single writer. */
void *ambe_selftest_worker(void *)
{
    char buf[96];
    int n = hd2_ambe_load();
    snprintf(buf, sizeof buf, "AMBE load=%d\r\n", n);
    txStr(buf);
    void *enc = hd2_ambe_enc_alloc();
    snprintf(buf, sizeof buf, "AMBE init enc=%p\r\n", enc);
    txStr(buf);
    if(enc)
    {
        /* Locate the encoder OUTPUT across the WHOLE codec region
         * (0x50000..0x56000, both fixed sub-regions) via per-256B-block FNV
         * checksums (memory-light): encode a tone, hash all blocks, re-encode
         * with silence, re-hash, and list the blocks that differ. The input
         * block(s) ~0x55200 will differ trivially; any OTHER differing block is
         * real, input-dependent output. No diff anywhere => encode isn't
         * consuming the staged PCM (needs spawner config / different args). */
        enum { BLK = 256, REGION = 0x6000, NBLK = REGION / BLK };
        const uint8_t *region = (const uint8_t *)0x00050000u;
        static uint32_t sums[NBLK];                  /* static: keep worker stack small */
        int16_t pcm[160];

        hd2_ambe_test_tone(pcm);
        hd2_ambe_encode_half(enc, &pcm[0],  0);
        hd2_ambe_encode_half(enc, &pcm[80], 1);
        for(int b = 0; b < NBLK; b++) {
            uint32_t s = 2166136261u;
            for(int i = 0; i < BLK; i++) s = (s ^ region[b*BLK+i]) * 16777619u;
            sums[b] = s;
        }

        enc = hd2_ambe_enc_alloc();                  /* fresh state */
        memset(pcm, 0, sizeof pcm);                  /* silence */
        hd2_ambe_encode_half(enc, &pcm[0],  0);
        hd2_ambe_encode_half(enc, &pcm[80], 1);

        char diffs[200];
        int dn = 0, nd = 0;
        diffs[0] = '\0';
        for(int b = 0; b < NBLK; b++) {
            uint32_t s = 2166136261u;
            for(int i = 0; i < BLK; i++) s = (s ^ region[b*BLK+i]) * 16777619u;
            if(s != sums[b]) {
                nd++;
                if(dn < (int)sizeof diffs - 12)
                    dn += snprintf(diffs+dn, sizeof diffs - dn, "%x,", 0x50000u + b*BLK);
            }
        }
        snprintf(buf, sizeof buf, "AMBE diffblk n=%d: %s\r\n", nd, nd ? diffs : "none");
        txStr(buf);
        hd2_ambe_free(enc);
    }
    txStr("AMBE done\r\n");
    return nullptr;
}
#endif

void *diagThreadFunc(void *)
{
    for(;;)
    {
        /* PC Mode (UI-driven codeplug programming): the "PC Mode" screen has
         * quiesced the radio (g_rf_freeze=1) and wants UART0 dedicated to the
         * CPS server.  Hand the link straight to cps::session, which serves the
         * vendor read/write protocol (GetVer/SLC7000/0x68/END) with GREEN-on-rx
         * / RED-on-tx LED feedback and stays resident until the screen clears
         * g_cps_pc_mode (its exit key).  Bypasses the per-byte GetVer/0x68 gate
         * below; the normal diag ops resume automatically when PC mode is off. */
        if(g_cps_pc_mode != 0u)
        {
            cps::session(/*primed_getver=*/false, /*pending68=*/false);
            continue;
        }

        // Idle: sleep rather than busy-burn until a byte arrives. A 5 ms sleeper
        // survives a PARTIAL hang the same way main_thread does, so the poke
        // interface still answers when the UI thread is wedged. (The getTime()
        // os_timer race that once collapsed sleep() is fixed in 812d5fa3, so a
        // sleeper no longer hangs with the timer -- a yield() spin here would just
        // peg the core and defeat tickless idle.)
        if((UART0_LSR & UART_LSR_DR) == 0u)
        {
            HD2_TRACE_STAMP(diag);      // hard-lock trace: diag idle loop alive
            Thread::sleep(5);
            continue;
        }

        uint8_t cmd;
        if(!rxByte(cmd)) continue;

        /* ---- CPS-session gate (vendor codeplug protocol over UART0) -------
         * The vendor CPS opens a session with the literal "GetVer"; our
         * dmrconfig client does too.  When we see 'G', peek the next 5 bytes:
         * if they spell "etVer", hand the link to the CPS server (it replies
         * the version then serves read/write frames until END / idle).  A bare
         * 0x68 SYNC also enters the session (direct frame, no probe).  Both are
         * collision-free with the single-letter diag ops: 'S'/'E'/'G' are only
         * interpreted as CPS tokens INSIDE a session.  ('g'/'G' diag ops were
         * removed, so 'G' is free here.) */
        if(cmd == 'G')
        {
            uint8_t g[5];
            bool ok = true;
            for(int i = 0; i < 5 && ok; ++i) ok = rxByte(g[i]);
            if(ok && g[0]=='e'&&g[1]=='t'&&g[2]=='V'&&g[3]=='e'&&g[4]=='r')
            {
                cps::session(/*primed_getver=*/true, /*pending68=*/false);
                continue;
            }
            /* not "GetVer" -> fall through and ignore (no 'G' diag op) */
            continue;
        }
        if(cmd == cps::CPS_SYNC)                 /* 0x68: direct CPS frame */
        {
            cps::session(/*primed_getver=*/false, /*pending68=*/true);
            continue;
        }

        switch(cmd)
        {
            case 'P':                              // probe -> version line
                txStr(HD2_DIAG_VERSION);
                tx('\n');
                break;
            case 'R':                              // byte read: <addr><size>
            {
                uint32_t addr; uint8_t size;
                if(!rxU32(addr) || !rxByte(size)) break;   // timeout -> resync
                volatile const uint8_t *p =
                    reinterpret_cast<volatile const uint8_t *>(addr);
                for(uint8_t i = 0; i < size; ++i) tx(p[i]);
                break;
            }
            case 'r':                              // word read: <addr>
            {
                uint32_t addr;
                if(!rxU32(addr)) break;
                txU32(*reinterpret_cast<volatile const uint32_t *>(addr));
                break;
            }
            case 'W':                              // word write: <addr><val>
            {
                uint32_t addr, val;
                if(!rxU32(addr) || !rxU32(val)) break;
                *reinterpret_cast<volatile uint32_t *>(addr) = val;
                tx('k');                           // ack
                break;
            }
            case 'N':                              // connect path: <src u8><sink u8> -> u8
            {
                uint8_t src, snk;
                if(!rxByte(src) || !rxByte(snk)) break;
                tx(hd2_router_connect(src, snk) == 0 ? 0x00u : 0xFFu);
                break;
            }
            case 'n':                              // disconnect path: <src u8><sink u8> -> u8
            {
                uint8_t src, snk;
                if(!rxByte(src) || !rxByte(snk)) break;
                tx(hd2_router_disconnect(src, snk) == 0 ? 0x00u : 0xFFu);
                break;
            }
            case 'e':                              // FM extras: <flags u8><vox u8> -> 'k'
            {                                      // flags bit0=1750 burst, bit1=tail elim; vox 0..5
                uint8_t flags, vox;
                if(!rxByte(flags) || !rxByte(vox)) break;
                rtx_setFmExtras(flags, vox);
                tx('k');
                break;
            }
            case 'T':                              // DTMF tx: <n u8><onMs u16LE><offMs u16LE><n ASCII digits>
            {                                      // TRANSMITS RF.  on/off 0 -> defaults (120/80 ms).
                uint8_t n, ol, oh, fl, fh;
                if(!rxByte(n) || !rxByte(ol) || !rxByte(oh) || !rxByte(fl) || !rxByte(fh)) break;
                if(n > 32u) n = 32u;
                uint16_t onMs  = (uint16_t)(ol | (oh << 8));
                uint16_t offMs = (uint16_t)(fl | (fh << 8));
                if(onMs  == 0u) onMs  = 120u;
                if(offMs == 0u) offMs = 80u;
                char s[32];
                bool ok = true;
                for(uint8_t i = 0; i < n; ++i) { uint8_t b; if(!rxByte(b)) { ok = false; break; } s[i] = (char)b; }
                if(!ok) break;
                char buf[48];
                dtmf_tx_send(s, n, onMs, offMs, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 't':                              // DTMF rx: <max u8> -> "DTMF rx=<digits> (N)"
            {
                uint8_t mx;
                if(!rxByte(mx)) break;
                char buf[48];
                dtmf_rx_read(mx, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'S':                              // set routing target: <target u8><val u32 LE> -> u32 LE
            {
                uint8_t target; uint32_t val;
                if(!rxByte(target) || !rxU32(val)) break;
                txU32(hd2_route_set(target, val));
                break;
            }
            case 'E':                              // arm guarded routing ops (one-shot)
                hd2_route_arm_tx();
                txStr("TX armed\r\n");              // \r\n to match main.c's reply
                break;
            case 'U':                              // routing snapshot -> ascii line(s)
            {
                char buf[320];
                hd2_route_dump(buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'b':                              // speaker tone test: <freq u16 LE> -> 'k'
            {                                      // PWM ch1 -> codec DAC -> lineout -> amp -> speaker;
                uint8_t lo, hi;                    // bypasses AT1846S / modem / FM entirely.
                if(!rxByte(lo) || !rxByte(hi)) break;
                uint16_t f = (uint16_t)((uint16_t)lo | (uint16_t)(hi << 8));
                if(f == 0u) f = 1000u;             // 0 -> 1 kHz default
                platform_beepStart(f);
                Thread::sleep(300);                // ~300 ms tone (timer is healthy post-812d5fa3)
                platform_beepStop();
                tx('k');
                break;
            }
            case 'q':                              // read AT1846S register: <reg u8> -> u16 LE
            {
                uint8_t reg;
                if(!rxByte(reg)) break;
                uint16_t v = hd2_at1846s_read(reg);
                tx((uint8_t)(v & 0xffu));
                tx((uint8_t)((v >> 8) & 0xffu));
                break;
            }
            case 'Q':                              // write AT1846S register: <reg u8> <val u16 LE> -> 'k'
            {
                uint8_t reg, lo, hi;
                if(!rxByte(reg) || !rxByte(lo) || !rxByte(hi)) break;
                hd2_at1846s_write(reg, (uint16_t)((uint16_t)lo | (uint16_t)(hi << 8)));
                tx('k');
                break;
            }
            case 'z':                              // rf_freeze: <on u8> -> "RFFREEZE=N\r\n"
            {                                      // suspend/resume ALL firmware AT1846S I2C +
                uint8_t on;                        // audio-GPIO rewrites (live-experiment guard)
                if(!rxByte(on)) break;
                g_rf_freeze = (on != 0u) ? 1u : 0u;
                txStr(g_rf_freeze ? "RFFREEZE=1\r\n" : "RFFREEZE=0\r\n");
                break;
            }
            case 'd':                              // flash-write one sector: <addr u32 LE>
            {                                      // <len u16 LE> <data[len]> -> "FWR=ok/err"
                static uint8_t fwbuf[W25Q_HD2_SECTOR_SIZE];
                uint8_t h[6];
                bool ok = true;
                for(int i = 0; i < 6 && ok; ++i) ok = rxByte(h[i]);
                if(!ok) { txStr("FWR=short\r\n"); break; }
                uint32_t addr = (uint32_t)h[0] | ((uint32_t)h[1] << 8) |
                                ((uint32_t)h[2] << 16) | ((uint32_t)h[3] << 24);
                uint32_t len  = (uint32_t)h[4] | ((uint32_t)h[5] << 8);
                if(len > sizeof(fwbuf)) { txStr("FWR=toolong\r\n"); break; }
                for(uint32_t i = 0; i < len && ok; ++i) ok = rxByte(fwbuf[i]);
                if(!ok) { txStr("FWR=short\r\n"); break; }
                if(!w25q_hd2_probe()) { txStr("FWR=noflash\r\n"); break; }
                int e = w25q_hd2_eraseSector(addr);
                int p = (e == 0) ? w25q_hd2_program(addr, fwbuf, len) : -1;
                txStr((e == 0 && p == 0) ? "FWR=ok\r\n" : "FWR=err\r\n");
                break;
            }
            case 'F':                              // radio_enable: bring the radio up on command
            {                                      // (boot-inhibit debug). -> "RADIO=1\r\n"
                g_radio_enabled = 1;
                txStr("RADIO=1\r\n");
                break;
            }
            case 'D':                              // dump vendor data layer (settings +
            {                                      // channel[0] + contact[0]) -> ascii lines
                char buf[200];
                hd2_cps_settings_t st;
                int sres = hd2_cps_settings_load(&st);
                snprintf(buf, sizeof buf,
                    "VSET(%s) sql=%u bl=%u bri=%u mic=%d keybeep=%u step=%u\r\n",
                    sres == 1 ? "dflt" : (sres == 0 ? "ok" : "err"),
                    st.squelch, st.backlight, st.brightness, (int)st.mic_gain,
                    (st.flags4 & HD2_VSET_F4_KEY_BEEP) ? 1u : 0u, st.step);
                txStr(buf);

                hd2_vendor_channel_t ch;
                if(hd2_vendor_channel_read(0, &ch) == 0 && hd2_vendor_channel_present(&ch))
                {
                    char nm[11]; memcpy(nm, ch.name, 10); nm[10] = 0;
                    snprintf(buf, sizeof buf,
                        "CH0 \"%s\" rx=%lu tx=%lu %s %s\r\n", nm,
                        (unsigned long)hd2_bcd4_to_hz(ch.rx_freq),
                        (unsigned long)hd2_bcd4_to_hz(ch.tx_freq),
                        hd2_channel_is_dmr(&ch) ? "DMR" : "FM",
                        hd2_channel_is_wide(&ch) ? "wide" : "narrow");
                }
                else snprintf(buf, sizeof buf, "CH0 empty\r\n");
                txStr(buf);

                hd2_vendor_contact_t ct;
                if(hd2_vendor_contact_read(0, &ct) == 0 && hd2_vendor_contact_present(&ct))
                {
                    char nm[17]; memcpy(nm, ct.name, 16); nm[16] = 0;
                    snprintf(buf, sizeof buf, "CT0 id=%lu type=%u \"%s\"\r\n",
                        (unsigned long)ct.dmr_id, ct.type, nm);
                }
                else snprintf(buf, sizeof buf, "CT0 empty\r\n");
                txStr(buf);
                break;
            }
            case 'H':                              // trace: previous-life + live stamps
            {                                      // (hard-lock trace; hd2_trace.h)
#ifdef WITH_HD2_TRACE
                char buf[360];
                snprintf(buf, sizeof buf,
                    "PREV boots=%lu rtx=%lu@%08lx wake=%lu@%08lx ovf=%lu@%08lx diag=%lu@%08lx"
                    " wctl=%lx wld=%08lx wcur=%08lx irq=%08lx%08lx"
                    " disp=%lu@%08lx arm=%lu/%08lx pic=%08lx res=%08lx/%08lx/%08lx%s\r\n"
                    "LIVE boots=%lu rtx=%lu@%08lx wake=%lu@%08lx ovf=%lu@%08lx diag=%lu@%08lx now=%08lx\r\n",
                    (unsigned long)hd2_trace_prev.boot_count,
                    (unsigned long)hd2_trace_prev.rtx_hb,  (unsigned long)hd2_trace_prev.rtx_tick,
                    (unsigned long)hd2_trace_prev.wake_hb, (unsigned long)hd2_trace_prev.wake_tick,
                    (unsigned long)hd2_trace_prev.ovf_hb,  (unsigned long)hd2_trace_prev.ovf_tick,
                    (unsigned long)hd2_trace_prev.diag_hb, (unsigned long)hd2_trace_prev.diag_tick,
                    (unsigned long)hd2_trace_prev.wake_ctrl, (unsigned long)hd2_trace_prev.wake_load,
                    (unsigned long)hd2_trace_prev.wake_cur,
                    (unsigned long)hd2_trace_prev.irqns_hi, (unsigned long)hd2_trace_prev.irqns_lo,
                    (unsigned long)hd2_trace_prev.disp_hb, (unsigned long)hd2_trace_prev.disp_tick,
                    (unsigned long)hd2_trace_prev.arm_hb,  (unsigned long)hd2_trace_prev.arm_rel,
                    (unsigned long)hd2_trace_prev.pic_snap,
                    (unsigned long)hd2_trace_prev.res_sp,
                    (unsigned long)hd2_trace_prev.res_epc,
                    (unsigned long)hd2_trace_prev.res_epsr,
                    (hd2_trace_prev.magic == HD2_TRACE_MAGIC) ? "" : " (INVALID)",
                    (unsigned long)HD2_TRACE->boot_count,
                    (unsigned long)HD2_TRACE->rtx_hb,  (unsigned long)HD2_TRACE->rtx_tick,
                    (unsigned long)HD2_TRACE->wake_hb, (unsigned long)HD2_TRACE->wake_tick,
                    (unsigned long)HD2_TRACE->ovf_hb,  (unsigned long)HD2_TRACE->ovf_tick,
                    (unsigned long)HD2_TRACE->diag_hb, (unsigned long)HD2_TRACE->diag_tick,
                    (unsigned long)HD2_TRACE_NOW());
                txStr(buf);
#else
                txStr("trace disabled (build -DWITH_HD2_TRACE)\r\n");
#endif
                break;
            }
            case 'X':                              // wdt: <mode u8> 0=force reboot now,
            {                                      // 1=auto-WDT on, 2=auto-WDT off
                uint8_t m;
                if(!rxByte(m)) break;
                if(m == 0u)
                {
                    txStr("REBOOT\r\n");
                    Thread::sleep(30);             // let the reply drain the UART
                    /* Restore the IAP-handoff (~84 MHz) clocks BEFORE the reset:
                     * the WDT reset does NOT reset the clock tree, and the IAP
                     * computes its UART divisor assuming the pre-PLL clock --
                     * without this it comes up at 28800 and spews junk (and any
                     * host bytes it mis-frames become menu keystrokes, which
                     * can wedge it into YMODEM-wait).  Same recipe as the 'Z'
                     * jump-to-IAP op (main.c). */
                    clk_restore_prepll();
                    hd2_wdt_force_reset();         // ~15 ms WDT -> chip reset; no return
                }
                else if(m == 1u) { g_wdt_auto = 1u; txStr("WDT=1\r\n"); }
                else             { g_wdt_auto = 0u; hd2_wdt_off(); txStr("WDT=0\r\n"); }
                break;
            }
            case 'c':                              // echo: <val u8> -> "ECHO=NN\r\n"
            {                                      // sequence counter so a reply after a lock can
                uint8_t v;                         // be tied to the exact probe that produced it
                if(!rxByte(v)) break;
                char buf[16];
                snprintf(buf, sizeof buf, "ECHO=%02x\r\n", (unsigned)v);
                txStr(buf);
                break;
            }
            case 'I':                              // at_reinit -> "REINIT freq=<hz>\r\n"
            {                                      // one-shot full AT1846S bring-up; BLOCKS this
                                                   // thread ~700 ms (VCO calibration delays).
                uint32_t f = rtx_getCurrentStatus().rxFrequency;
                hd2_at1846s_reinit(f);
                char buf[48];
                snprintf(buf, sizeof buf, "REINIT freq=%lu\r\n", (unsigned long)f);
                txStr(buf);
                break;
            }
            case 'o':                              // at_profile: <profile u8> -> 'k'
            {                                      // 0=vendor / 1=GD77 audio-gain regs, live A/B
                uint8_t prof;
                if(!rxByte(prof)) break;
                hd2_at1846s_profile(prof);
                tx('k');
                break;
            }
            case 'm':                              // at_mute: <mute u8> -> u16 LE (resulting reg 0x30)
            {                                      // RMW AT1846S reg 0x30 bit7 (RX AF mute)
                uint8_t mute;
                if(!rxByte(mute)) break;
                uint16_t v = hd2_at1846s_afmute(mute);
                tx((uint8_t)(v & 0xffu));
                tx((uint8_t)((v >> 8) & 0xffu));
                break;
            }
            case 'u':                              // audio_snap -> ascii block (multi-line)
            {                                      // full audio-path state: AT1846S + GPIO +
                char buf[512];                     // diplex + CLK_MGR + codec (hd2_router.c)
                hd2_audio_snap(buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'O':                              // pcm_capture probe: <ms u16 LE><arm u8> -> "CAP pp=.."
            {
                uint8_t mlo, mhi, arm;
                if(!rxByte(mlo) || !rxByte(mhi) || !rxByte(arm)) break;
                char buf[96];
                hd2_pcm_capture((uint16_t)(mlo | (mhi << 8)), arm, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'w':                              // aprs_rx: <frames u8> -> "APRS: <frame>" or status
            {
                uint8_t fr;
                if(!rxByte(fr)) break;
                static char buf[288];              // static: keep the diag stack small
                hd2_aprs_rx((uint16_t)fr, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'B':                              // aprs_tx: <preamble u8><flags u8> -> "APRSTX .."
            {                                      // keys analog-FM TX, beacons 1200-AFSK via
                uint8_t pre, fl;                   // codec playback.  TRANSMITS RF for <~3 s.
                if(!rxByte(pre) || !rxByte(fl)) break;
                char buf[96];
                hd2_aprs_tx(pre, fl, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'p':                              // pcm_tone: <freq u16 LE> <ms u16 LE> <arm u8> -> ascii line
            {                                      // SAHB PCM-window sine stream via vec-0x3b IRQ;
                uint8_t flo, fhi, mlo, mhi, arm;   // reply reports IRQ count + path regs.  Blocks <= 10 s.
                if(!rxByte(flo) || !rxByte(fhi) || !rxByte(mlo) || !rxByte(mhi)
                   || !rxByte(arm)) break;
                char buf[96];
                hd2_pcm_tone((uint16_t)(flo | (fhi << 8)),
                             (uint16_t)(mlo | (mhi << 8)), arm, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'v':                              // stream_tone: <freq u16 LE> <ms u16 LE> -> ascii line
            {                                      // full audioPath/audioStream/outputStream_HD2
                uint8_t flo, fhi, mlo, mhi;        // stack test.  Blocks <= 10 s.
                if(!rxByte(flo) || !rxByte(fhi) || !rxByte(mlo) || !rxByte(mhi)) break;
                char buf[96];
                hd2_stream_tone((uint16_t)(flo | (fhi << 8)),
                                (uint16_t)(mlo | (mhi << 8)), buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'A':                              // adpcm_test: no args -> ascii line
            {                                      // decode embedded "zero" ADPCM clip -> stream
                char buf[80];
                hd2_adpcm_sample_play(buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'V':                              // vp_say: <kind u8> <arg u8> -> 'k'
            {                                      // software voice-prompt test (codec2 -> stream).
                uint8_t kind, arg;                 // kind 0=integer(arg), 1=prompt(arg), 2="OpenRTX"
                if(!rxByte(kind) || !rxByte(arg)) break;
                hd2_vp_say(kind, arg);
                tx('k');
                break;
            }
            case 'M':                              // byte write (st.b): <addr u32><val u8> -> 'k'
            {                                      // for 8-bit blocks: codec @0x160009xx and the
                uint32_t addr; uint8_t val;        // modem TX/RX RAM @0x1600xxxx (word stores only
                if(!rxU32(addr) || !rxByte(val)) break;   // land their low byte there)
                *reinterpret_cast<volatile uint8_t *>(addr) = val;
                tx('k');
                break;
            }
            case 'Y':                              // fm_tx_tone: <secs u8><flags u8> -> ascii line
            {                                      // FM-TX tone test; flags 0x40 = M17 TX-RAM
                uint8_t secs, flags;               // baseband pump (see fm_tx_tone_test above).
                                                   // TRANSMITS RF for <secs> seconds!
                if(!rxByte(secs) || !rxByte(flags)) break;
                if(secs == 0u || secs > 20u) secs = 6u;
                char buf[96];
                fm_tx_tone_test(secs, flags, buf, sizeof buf);
                txStr(buf);
                break;
            }
#if defined(HD2_M17_TXTEST)
            case 'f':                              // m17_txtest: <secs u8><dev u8><opt u8> -> line
            {                                      // play embedded M17 frame; dev=FM_DEV_COEF,
                uint8_t secs, dev, opt;            // opt b0=BPF off b1=full-amp. TRANSMITS RF!
                if(!rxByte(secs) || !rxByte(dev) || !rxByte(opt)) break;
                if(secs == 0u || secs > 30u) secs = 8u;
                char buf[96];
                m17_txtest(secs, dev, opt, buf, sizeof buf);
                txStr(buf);
                break;
            }
#endif
#if defined(HD2_M17_VOICE)
            case 's':                              // m17_voice: <secs u8><dev u8><opt u8> -> line
            {                                      // generate codec2->M17 voice on-device, pump it
                uint8_t secs, dev, opt;            // on air. dev=FM_DEV_COEF; opt as op 'f'.
                if(!rxByte(secs) || !rxByte(dev) || !rxByte(opt)) break;  // TRANSMITS RF!
                if(secs == 0u || secs > 30u) secs = 6u;
                g_m17v_secs = secs; g_m17v_dev = dev; g_m17v_opt = opt;
                pthread_attr_t a;                  // 16 KiB worker: codec2 + Modulator overflow
                pthread_t t;                       // the diag thread's 2 KiB stack.
                pthread_attr_init(&a);
                pthread_attr_setstacksize(&a, 16384);
                if(pthread_create(&t, &a, m17_voice_worker, nullptr) == 0)
                    pthread_join(t, nullptr);
                else
                    txStr("M17V spawn fail\r\n");
                pthread_attr_destroy(&a);
                break;
            }
#endif
            case 'L':                              // dmr_tx_carrier: <secs u8><flags u8> -> ascii line
            {                                      // DMR-TX bench bring-up (see dmr_tx_carrier_test).
                uint8_t secs, flags;               // TRANSMITS RF for <secs> seconds!
                if(!rxByte(secs) || !rxByte(flags)) break;
                if(secs == 0u || secs > 20u) secs = 6u;
                char buf[96];
                dmr_tx_carrier_test(secs, flags, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'j':                              // dmr_tx_pump: <secs u8><flags u8> -> ascii line
            {                                      // sustained DMR burst pump (see dmr_tx_pump_test).
                uint8_t secs, flags;               // TRANSMITS RF for <secs> seconds!
                if(!rxByte(secs) || !rxByte(flags)) break;
                if(secs == 0u || secs > 30u) secs = 8u;
                char buf[96];
                dmr_tx_pump_test(secs, flags, buf, sizeof buf);
                txStr(buf);
                break;
            }
            case 'x':                              // vp_fire: <msg_id u8> <mode u8> -> 'k'
            {                                      // voice-prompt doorbell experiment
                uint8_t id, mode;                  // (hd2_router.c, docs/voice_prompt_map.md)
                if(!rxByte(id) || !rxByte(mode)) break;
                hd2_vp_fire(id, mode);
                tx('k');
                break;
            }
#if defined(HD2_DMR_VOICE)
            case 'K':                              // AMBE codec self-test (bring-up)
            {                                      // run on a 16 KiB-stack worker -- the codec
                pthread_attr_t a;                  // overflows the diag thread's 2 KiB stack.
                pthread_t t;
                pthread_attr_init(&a);
                pthread_attr_setstacksize(&a, 16384);
                if(pthread_create(&t, &a, ambe_selftest_worker, nullptr) == 0)
                    pthread_join(t, nullptr);
                else
                    txStr("AMBE spawn fail\r\n");
                pthread_attr_destroy(&a);
                break;
            }
#endif
#if defined(HD2_M17)
            case 'i':                              // Codec2-3200 encode self-test (M17 Phase 1)
            {                                      // 16 KiB-stack worker (32 KiB won't spawn --
                pthread_attr_t a;                  // heap too small; 16 KiB is the AMBE-proven max)
                pthread_t t;
                pthread_attr_init(&a);
                pthread_attr_setstacksize(&a, 16384);
                if(pthread_create(&t, &a, codec2_selftest_worker, nullptr) == 0)
                    pthread_join(t, nullptr);
                else
                    txStr("C2 spawn fail\r\n");
                pthread_attr_destroy(&a);
                break;
            }
#endif
            default:
                break;                             // ignore -> resync next cmd
        }
    }
    return nullptr;                                // unreachable
}

} // namespace

// Start the diag thread. Call AFTER platform/UART init (openrtx_init), from the
// HD2 entry (main.cpp). HD2-only -- never linked into OpenRTX core.
extern "C" void hd2_diag_start()
{
    static pthread_t diag_thread;
    pthread_create(&diag_thread, nullptr, diagThreadFunc, nullptr);
}
