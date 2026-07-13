/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 GPS bring-up probe (RTOS-driven) + pokeable diagnostics.
 *
 * Task 2026-06-05-gps-bringup. The GPS module (u-blox on UART2 @ 0x14050000,
 * 9600 8N1) was silent in the superloop; this is the RTOS-native attempt:
 * a dedicated Miosix thread continuously drains UART2 RX and (optionally) TXes
 * the vendor UBX sentence-trim keepalive — the thing the cooperative superloop
 * couldn't sustain. It also applies a board-init divergence found in the V2.1.3
 * decomp: FUN_03041fa8 sets SOCSYS_IO_DIPLEX1 |= 0x38000000 (bits 27-29) before
 * driving the GPS-area GPIOs; our platform.c writes diplex1=0x000000ff and never
 * sets those bits. (GPIOB.10/20/22 are already driven high by platform_init.)
 *
 * All state is exported as plain extern-"C" globals so it can be read LIVE via
 * the loader word-read poke ('r' @ symbol addr from `nm` per build) while the
 * radio runs headless. NO UART trace logging (it destabilises HD2 boot) — the
 * g_gps_ring[] byte log + counters ARE the log. g_gps_cfg_mode is a runtime
 * knob (poke with 'W'): 1 = send UBX keepalive, 0 = stay silent (test whether
 * the module free-runs NMEA without config).
 */

#include <miosix.h>
#include <pthread.h>
#include <cstdint>
#include "hd2_regs.h"

using namespace miosix;

extern "C" {
/* ---- pokeable diagnostics (read via loader 'r') ---------------------- */
volatile uint32_t g_gps_magic       = 0x67505331u; /* "gPS1" sanity marker */
volatile uint32_t g_gps_rx_bytes    = 0;   /* total bytes pulled from UART2 RBR   */
volatile uint32_t g_gps_loops       = 0;   /* probe-thread iterations (heartbeat) */
volatile uint32_t g_gps_lsr_last    = 0;   /* last UART2 LSR value seen           */
volatile uint32_t g_gps_lsr_dr_loops= 0;   /* loops where LSR data-ready was set  */
volatile uint32_t g_gps_lsr_err     = 0;   /* OR of LSR error bits (FE/PE/BI/OE)  */
volatile uint32_t g_gps_sentences   = 0;   /* count of '\n' bytes (≈ NMEA lines)  */
volatile uint32_t g_gps_dollar      = 0;   /* count of '$' bytes (NMEA starts)    */
volatile uint32_t g_gps_keepalives  = 0;   /* UBX config TX rounds sent           */
volatile uint32_t g_gps_diplex0     = 0;   /* diplex0 after setup                 */
volatile uint32_t g_gps_diplex1     = 0;   /* diplex1 after setup                 */
volatile uint32_t g_gps_gpiob_dr    = 0;   /* GPIOB DR after setup (B.10/20/22?)  */
volatile uint32_t g_gps_ring_head   = 0;   /* write index into g_gps_ring         */
volatile uint8_t  g_gps_ring[64]    = {0}; /* last 64 raw RX bytes (NMEA text)    */

/* ---- runtime knobs (poke with 'W') ----------------------------------- */
volatile uint32_t g_gps_cfg_mode    = 1;   /* 1=send UBX keepalive, 0=silent      */
volatile uint32_t g_gps_diplex1_or  = 0x38000000u; /* bits to OR into diplex1 (0=off) */
}

/* UART2 (DesignWare 16550, word-spaced regs); offsets from hd2_regs.h. */
#define U2(off)  (*(volatile uint32_t *)(UART2_BASE + (off)))
#define LSR_DR        0x01u
#define LSR_THR_EMPTY 0x20u
#define LSR_ERRBITS   0x1Eu          /* OE|PE|FE|BI */

/* u-blox UBX CFG-MSG sentence-trim strings (same as gps_HD2.c, from V2.1.3). */
static const uint8_t UBX_CFG[5][13] = {
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x02,0x00,0xfc,0x13,0x0d,0x0a},
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x03,0x00,0xfd,0x15,0x0d,0x0a},
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x01,0x00,0xfb,0x11,0x0d,0x0a},
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x04,0x00,0xfe,0x17,0x0d,0x0a},
    {0xb5,0x62,0x06,0x01,0x03,0x00,0xf0,0x05,0x00,0xff,0x19,0x0d,0x0a},
};

static void gps_uart2_setup()
{
    /* Board-init divergence: apply the vendor's diplex1 high bits (poke
     * g_gps_diplex1_or to 0 to disable for an A/B test). */
    if(g_gps_diplex1_or != 0u)
        SOCSYS_IO_DIPLEX1 |= g_gps_diplex1_or;

    /* GPS module enable: drive GPIOB.15 (vendor pin 0x2f) HIGH. This is the
     * HD2's GPS power/enable (GPS3V3_EN equivalent); the vendor asserts it and
     * we didn't -> module held off, UART2 RX silent. Isolated live: B.15 high
     * -> NMEA flows, low -> stops. B.15 is already an output (GPIOB_DDR). */
    GPIOB_DR |= (1u << 15);

    /* Mux PTA11/PTA12 to the UART2 function (clear rxd/txd sel bits). */
    SOCSYS_IO_DIPLEX0 &= ~(DIPLEX0_UART2_RXD | DIPLEX0_UART2_TXD);

    /* UART2: FIFO enable + reset, polled (no IER), 9600 8N1 @ 42 MHz. */
    U2(UART_FCR_OFF) = 0x67u;
    uint32_t lcr = U2(UART_LCR_OFF);
    U2(UART_LCR_OFF) = lcr | LCR_DLAB;
    uint32_t div = 42000000u / (16u * 9600u);     /* = 273 */
    U2(UART_DLL_OFF) = div & 0xFFu;
    U2(UART_DLH_OFF) = (div >> 8) & 0xFFu;
    U2(UART_LCR_OFF) = LCR_8N1;
    U2(UART_IER_OFF) = 0u;

    g_gps_diplex0  = SOCSYS_IO_DIPLEX0;
    g_gps_diplex1  = SOCSYS_IO_DIPLEX1;
    g_gps_gpiob_dr = GPIOB_DR;
}

static void gps_txByte(uint8_t b)
{
    uint32_t guard = 200000u;
    while(((U2(UART_LSR_OFF) & LSR_THR_EMPTY) == 0u) && (--guard != 0u)) { }
    U2(UART_THR_OFF) = b;
}

static void gps_sendConfig()
{
    for(unsigned i = 0; i < 5u; ++i)
        for(unsigned j = 0; j < sizeof(UBX_CFG[0]); ++j)
            gps_txByte(UBX_CFG[i][j]);
    g_gps_keepalives++;
}

static void *gpsProbeThread(void *)
{
    gps_uart2_setup();
    if(g_gps_cfg_mode != 0u)
        gps_sendConfig();

    for(;;)
    {
        uint32_t lsr = U2(UART_LSR_OFF);
        g_gps_lsr_last = lsr;
        if(lsr & LSR_ERRBITS) g_gps_lsr_err |= (lsr & LSR_ERRBITS);

        if(lsr & LSR_DR)
        {
            g_gps_lsr_dr_loops++;
            while(U2(UART_LSR_OFF) & LSR_DR)
            {
                uint8_t c = (uint8_t)(U2(UART_RBR_OFF) & 0xFFu);
                g_gps_rx_bytes++;
                g_gps_ring[g_gps_ring_head & 63u] = c;
                g_gps_ring_head++;
                if(c == (uint8_t)'\n') g_gps_sentences++;
                if(c == (uint8_t)'$')  g_gps_dollar++;
            }
        }

        g_gps_loops++;
        /* periodic keepalive (~every 2 s) while enabled */
        if(g_gps_cfg_mode != 0u && (g_gps_loops % 400u) == 0u)
            gps_sendConfig();

        Thread::sleep(5);
    }
    return nullptr;
}

/* Start the GPS probe thread. Call AFTER platform/clock init (openrtx_init),
 * from the HD2 entry (main.cpp). HD2-only. */
extern "C" void hd2_gps_start()
{
    static pthread_t gps_thread;
    pthread_create(&gps_thread, nullptr, gpsProbeThread, nullptr);
}
