/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
 * HR_C7000 (Ailunce HD2) MMIO peripheral register map -- bring-up subset.
 *
 * Each peripheral is described as a struct laid out at its register offsets
 * (gaps for registers this port does not use are RESERVED padding), and cast onto
 * the peripheral base address via the accessor macros at the bottom -- the CMSIS
 * convention. The named registers and their offsets are taken from the HR_C7000
 * vendor user guide (system-control chapter 4, peripherals chapter 5). The
 * analog-FM subset SOCSYS needs (audio routing, codec gate, CPU DAC, FM
 * modulator / PTT) is included for the radio driver; the DMR / M17 modem
 * register set is not. Base
 * addresses, pin-mux masks and "magic" configuration words are hardware facts
 * recovered by on-device reverse engineering of the vendor V2.1.3 firmware; they
 * are not guessed.
 */

#ifndef HRC7000_REGISTERS_H
#define HRC7000_REGISTERS_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 *  SOCSYS -- chip system control / clock / pin-mux (base 0x11000000)
 *  Manual 4.1 (reset), 4.2 (clock), 4.4 (system control).
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t SYS_SOFT_RSTN; /* 0x00 per-block soft reset, active-low:
                                      * [8]cpu [7]sys [6]adc_ctrl [5]adc
                                      * [4]codec [3]audio [2]fm [1]phy [0]protocol */
    volatile uint32_t
        CLK_MGR_REG04; /* 0x04 PLL reconfig ctrl (b31 pll_ld, b30 clk_rdy) */
    volatile uint32_t CLK_MGR_REG08; /* 0x08 APLL divider configuration */
    volatile uint32_t CLK_MGR_REG0C; /* 0x0c APLL divider configuration */
    volatile uint32_t CLK_MGR_REG10; /* 0x10 BPLL divider configuration */
    volatile uint32_t CLK_MGR_REG14; /* 0x14 BPLL divider configuration */
    volatile uint32_t CLK_MGR_REG18; /* 0x18 baseband clock divider coefficient */
    volatile uint32_t CLK_MGR_REG1C; /* 0x1c FM 32K/192K divider coefficient */
    volatile uint32_t CLK_MGR_REG20; /* 0x20 codec I2S interface clock divider */
    volatile uint32_t CLK_MGR_REG24; /* 0x24 bus-related clock divider */
    volatile uint32_t CLK_MGR_REG28; /* 0x28 SDIO/USB clock divider */
    volatile uint32_t CLK_MGR_REG2C; /* 0x2c gated-clock control enable */
    volatile uint32_t LCSFC_BAUDR;   /* 0x30 LCSFC SPI SCLK clock divider */
    volatile uint32_t IO_DIPLEX0;    /* 0x34 PTA pad mux (i2c1/uart2/audio) */
    volatile uint32_t IO_DIPLEX1;    /* 0x38 PTA pad mux (spi0 pins) */
    volatile uint32_t IO_DIPLEX2;    /* 0x3c PTC pad mux */
    /* --- analog-FM audio / codec / modulator subset (manual 4.6, ch.9/11) --- */
    uint32_t RESERVED1[12]; /* 0x40 - 0x6c */
    volatile uint32_t
        BB_DAC_CTRL; /* 0x70 baseband RF-mod / RX-bias DAC (datasheet §8.1) -- NOT the codec DAC */
    volatile uint32_t
        BB_ADC_CTRL; /* 0x74 baseband demod (I/Q) ADC front-end (datasheet §8.2) -- NOT the codec ADC */
    uint32_t RESERVED2[2]; /* 0x78 - 0x7c */
    volatile uint32_t
        AUDIO_CONTROL; /* 0x80 baseband audio source mux (§4.6.5.3: tx_voice_source[0], fm_play_ctrl[6], ring_play_ctrl[7]) */
    volatile uint32_t
        AUDIO_BUFFER_CLR; /* 0x84 vocoder decode/encode buffer clear (§4.6.5.4, write-1-self-clear) */
    volatile uint32_t
        LINEOUT_CTRL; /* 0x88 [0]line2out_en [1]line1out_en [31]standby(RO) */
    uint32_t RESERVED3[29]; /* 0x8c - 0xfc */
    volatile uint32_t
        WORK_MODE; /* 0x100 work_mode (FM-analog 0x6e; bit7 = FM modulator) */
    volatile uint32_t
        RF_MODE;   /* 0x104 baseband RF-interface mode (FM 0x034c9060) */
    uint32_t RESERVED4[2];        /* 0x108 - 0x10c */
    volatile uint32_t RF_CONTROL; /* 0x110 tx_pre_on / rf_pre_on_tx */
    volatile uint32_t
        RF_MOD_BIAS;       /* 0x114 two-point MOD1/MOD2 amplitude (deviation) */
    uint32_t RESERVED5[2]; /* 0x118 - 0x11c */
    volatile uint32_t
        THRESHOLD_VALUE;   /* 0x120 RX sync-detection thresholds (§9.2.2.9) */
    uint32_t RESERVED6[17];       /* 0x124 - 0x164 */
    volatile uint32_t SLOT_GUARD; /* 0x168 TDMA slot guard */
    uint32_t RESERVED7[17];       /* 0x16c - 0x1ac */
    volatile uint32_t RX_IF_FREQ; /* 0x1b0 RX IF frequency word */
    volatile uint32_t RX_AGC;     /* 0x1b4 RX AGC configuration */
    uint32_t RESERVED8[120];      /* 0x1b8 - 0x394 */
    volatile uint32_t MODEM_IRQ;  /* 0x398 modem RX IRQ latch (idle 0x4000);
                                     unused in this FM-only build (RX-capture/future) */
    volatile uint32_t
        SYS_INTERP_MASK; /* 0x39c modem/Layer-2 interrupt mask (§4.6.5.10, FM vendor 0x1007f) */
    volatile uint32_t MODEM_IRQ_ACK; /* 0x3a0 write the latch value back to ACK;
                                        unused in this FM-only build (RX-capture/future) */
    uint32_t RESERVED9a[3];          /* 0x3a4 - 0x3ac */
    volatile uint32_t INT_STATUS;    /* 0x3b0 modem int-status / PCM frame ack
                                      * latch: bit4 capture, bit5 playback */
    uint32_t RESERVED9b[19];         /* 0x3b4 - 0x3fc */
    volatile uint32_t
        LAYER2_CONTROL;  /* 0x400 DMR L2 [7]txen [6]rxen TDMA slot-sync;
                            unused in this FM-only build (map completeness) */
    uint32_t RESERVED10; /* 0x404 */
    volatile uint32_t
        LAYER2_TXRX_CTRL; /* 0x408 DMR L2 tx/rx next-slot enable (TDMA slot-sync);
                             unused in this FM-only build (map completeness) */
    uint32_t RESERVED11[85]; /* 0x40c - 0x55c */
    volatile uint32_t
        FM_PTT; /* 0x560 bit0=1 keys the modem FM-TX engine (ch.11) */
} SOCSYS_TypeDef;

/* Offset guards: a wrong offset here is a silent hardware bug, so pin the
 * FM-subset registers to their documented addresses at compile time. */
static_assert(offsetof(SOCSYS_TypeDef, IO_DIPLEX2) == 0x3c,
              "SOCSYS IO_DIPLEX2");
static_assert(offsetof(SOCSYS_TypeDef, BB_DAC_CTRL) == 0x70,
              "SOCSYS BB_DAC_CTRL");
static_assert(offsetof(SOCSYS_TypeDef, AUDIO_CONTROL) == 0x80,
              "SOCSYS AUDIO_CONTROL");
static_assert(offsetof(SOCSYS_TypeDef, LINEOUT_CTRL) == 0x88,
              "SOCSYS LINEOUT_CTRL");
static_assert(offsetof(SOCSYS_TypeDef, WORK_MODE) == 0x100, "SOCSYS WORK_MODE");
static_assert(offsetof(SOCSYS_TypeDef, RF_MODE) == 0x104, "SOCSYS RF_MODE");
static_assert(offsetof(SOCSYS_TypeDef, THRESHOLD_VALUE) == 0x120,
              "SOCSYS THRESHOLD_VALUE");
static_assert(offsetof(SOCSYS_TypeDef, SLOT_GUARD) == 0x168,
              "SOCSYS SLOT_GUARD");
static_assert(offsetof(SOCSYS_TypeDef, RX_IF_FREQ) == 0x1b0,
              "SOCSYS RX_IF_FREQ");
static_assert(offsetof(SOCSYS_TypeDef, MODEM_IRQ) == 0x398, "SOCSYS MODEM_IRQ");
static_assert(offsetof(SOCSYS_TypeDef, MODEM_IRQ_ACK) == 0x3a0,
              "SOCSYS MODEM_IRQ_ACK");
static_assert(offsetof(SOCSYS_TypeDef, INT_STATUS) == 0x3b0,
              "SOCSYS INT_STATUS");
static_assert(offsetof(SOCSYS_TypeDef, LAYER2_CONTROL) == 0x400,
              "SOCSYS LAYER2_CONTROL");
static_assert(offsetof(SOCSYS_TypeDef, LAYER2_TXRX_CTRL) == 0x408,
              "SOCSYS LAYER2_TXRX_CTRL");
static_assert(offsetof(SOCSYS_TypeDef, FM_PTT) == 0x560, "SOCSYS FM_PTT");

/* work_mode / interrupt-mask value macros (analog FM). */
#define WORK_MODE_FM_MOD 0x80u /* work_mode bit7: FM analog modulator mode */
#define SYS_INTERP_MASK_FM_VENDOR \
    0x0001007fu /* vendor FM modem/Layer-2 interrupt-mask value */

/*
 * CPU -> codec-DAC PCM playback bridge (SAHB shared-SRAM mailbox + frame IRQ).
 * Once armed (AUDIO_BUFFER_CLR=3, AUDIO_CONTROL bit0 + playback bit) the codec requests
 * one 80-sample frame every 10 ms via the PCM-play PIC source; the ISR acks the
 * handshake (INT_STATUS bit5) and refills the playback window.  8 kHz mono s16.
 * Used by outputStream_HD2.cpp.
 */
#define SOFT_RSTN_PCM_BITS \
    0x18u /* SYS_SOFT_RSTN [4:3]: codec/audio PCM blocks */
#define AUDIO_CONTROL_PCM_EN \
    0x01u /* AUDIO_CONTROL bit0: 1 = PCM bridge, 0 = direct DAC */
#define AUDIO_CONTROL_PLAY 0x20u /* AUDIO_CONTROL playback-side enable */
#define AUDIO_CONTROL_CAP 0x10u  /* AUDIO_CONTROL mic-capture-side enable */
#define INT_STATUS_PCM_PLAY_ACK \
    0x20u                        /* INT_STATUS: a playback frame was supplied */
#define INT_STATUS_PCM_CAP_ACK \
    0x10u                        /* INT_STATUS: a capture frame was consumed */
#define PCM_FRAME_SAMPLES 80u    /* s16 samples per 10 ms PCM frame @ 8 kHz */
/* SAHB shared-SRAM PCM mailbox windows (absolute addresses). */
#define SAHB_PCM_PLAY \
    ((volatile uint16_t *)0x180000a0u) /* CPU writes, codec DACs */
#define SAHB_PCM_CAP \
    ((volatile uint16_t *)0x18000000u) /* mic ADC frames appear  */
/* PIC source numbers for the PCM frame IRQs (CK803S exception vec = src + 0x20). */
#define HD2_IRQ_PCM_PLAY \
    0x1bu /* codec requests a playback frame (vendor isr_pcm_rd) */
#define HD2_IRQ_PCM_CAP \
    0x1cu /* mic capture frame ready       (vendor isr_pcm_wr) */

/* HD2 board PTC pad-mux values (HD2_DIPLEX2_*) live in targets/HD2/pinmap.h. */

/* -------------------------------------------------------------------------
 *  GPIO banks -- DesignWare DW_apb_gpio (one struct, three instances)
 *  Manual 5.7, Register Tables 20-22.
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t DR;      /* 0x00 output data (SWPORTA_DR) */
    volatile uint32_t DDR;     /* 0x04 direction: 1 = output (SWPORTA_DDR) */
    uint32_t RESERVED0[10];    /* 0x08 - 0x2c */
    volatile uint32_t INTEN;   /* 0x30 interrupt enable */
    volatile uint32_t INTMASK; /* 0x34 interrupt mask */
    volatile uint32_t INTTYPE_LEVEL; /* 0x38 interrupt type (edge/level) */
    volatile uint32_t INT_POLARITY;  /* 0x3c interrupt polarity */
    volatile uint32_t INTSTATUS;     /* 0x40 masked interrupt status */
    volatile uint32_t RAW_INTSTATUS; /* 0x44 raw (pre-mask) interrupt status */
    volatile uint32_t DEBOUNCE;      /* 0x48 debounce enable */
    volatile uint32_t PORTA_EOI;  /* 0x4c interrupt clear (end-of-interrupt) */
    volatile uint32_t EXT_PORT;   /* 0x50 input read-back (EXT_PORTA) */
    uint32_t RESERVED1[9];        /* 0x54 - 0x74 */
    volatile uint32_t ALT_FUNC_B; /* 0x78 vendor per-pin alt-function-B select;
                                   * not part of DW_apb_gpio and distinct from
                                   * the SOCSYS pad-mux (see gpio_setAltFuncB) */
} GPIO_TypeDef;

/* HD2 board GPIOB pin bits (LED/PTT/PWR/GPS + analog-FM audio-path pins) live
 * in targets/HD2/pinmap.h. */

/* -------------------------------------------------------------------------
 *  LCD -- HR_C7000 hardware i8080 controller (base 0x12000000, manual 5.3)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t INDEX; /* 0x00 command cycle */
    volatile uint32_t DATA;  /* 0x04 8-bit data cycle */
    uint32_t RESERVED0[2];   /* 0x08 - 0x0c */
    volatile uint32_t WCFG;  /* 0x10 write strobe timing */
} LCD_TypeDef;
/* LCD_WCFG_DEFAULT / LCD_RESET_BIT (panel-specific) live in the ST7735S driver. */

/* -------------------------------------------------------------------------
 *  PWM block (base 0x140c0000, manual 5.8, Register Table 23). Per-channel
 *  registers CFG/RPT/PER/FP/STATUS at 0x00/0x04/0x08/0x0c/0x10; channel stride
 *  0x20; ch0 = LCD backlight.
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CFG;    /* 0x00 config: start/mode/soft_rst/inten */
    volatile uint32_t RPT;    /* 0x04 ONE_SHOT repeated-period count */
    volatile uint32_t PER;    /* 0x08 system-clock cycles per period */
    volatile uint32_t FP;     /* 0x0c first-phase clock cycles (duty) */
    volatile uint32_t STATUS; /* 0x10 status register */
    volatile uint32_t RESERVED_HW[3]; /* 0x14-0x1c manual marks reserved; vendor
                                       * V2.1.3 writes 0x300 here -- required or
                                       * the backlight stays dark. */
} PWM_Channel_TypeDef;

#define PWM_TIMER_HZ 42000000u /* post-PLL source feeding the PWM block */
/* PWM ch0 duty-gating: the V2.1.3 boot leaves STATUS (0x10) and the three
 * reserved words 0x14-0x1c at 0x300.  Left at 0 they silently squash the duty
 * cycle -> backlight LED stays dark. */
#define PWM_CH0_GATE_ON 0x00000300u

/* -------------------------------------------------------------------------
 *  I2C2 -- DesignWare DW_apb_i2c, the RTC's dedicated internal bus.
 *  (base 0x14080000; RTC slave address 0x70). The HR_C7000 adds a non-standard
 *  IC_START (+0xa0) trigger that must be written to launch a transfer.
 *  Manual 5.1, Register Tables 9-10.
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t IC_CON;           /* 0x00 master cfg = 0x65 */
    volatile uint32_t IC_TAR;           /* 0x04 target slave 7-bit address */
    uint32_t RESERVED0[2];              /* 0x08 - 0x0c */
    volatile uint32_t IC_DATA_CMD;      /* 0x10 tx data byte / rx command */
    uint32_t RESERVED1[2];              /* 0x14 - 0x18 */
    volatile uint32_t IC_FS_SCL_HCNT;   /* 0x1c */
    volatile uint32_t IC_FS_SCL_LCNT;   /* 0x20 */
    uint32_t RESERVED2[2];              /* 0x24 - 0x28 */
    volatile uint32_t IC_INTR_STAT;     /* 0x2c masked interrupt status */
    volatile uint32_t IC_INTR_MASK;     /* 0x30 */
    volatile uint32_t IC_RAW_INTR_STAT; /* 0x34 raw interrupt status */
    volatile uint32_t IC_RX_TL;         /* 0x38 */
    volatile uint32_t IC_TX_TL;         /* 0x3c */
    volatile uint32_t IC_CLR_INTR;      /* 0x40 clear all combined interrupts */
    volatile uint32_t IC_CLR_RX_UNDER;  /* 0x44 */
    volatile uint32_t IC_CLR_RX_OVER;   /* 0x48 */
    volatile uint32_t IC_CLR_TX_OVER;   /* 0x4c */
    volatile uint32_t IC_CLR_RD_REQ;    /* 0x50 */
    volatile uint32_t IC_CLR_TX_ABRT;   /* 0x54 read clears TX_ABRT latch */
    volatile uint32_t IC_CLR_RX_DONE;   /* 0x58 */
    volatile uint32_t IC_CLR_ACTIVITY;  /* 0x5c */
    volatile uint32_t IC_CLR_STOP_DET;  /* 0x60 */
    volatile uint32_t IC_CLR_START_DET; /* 0x64 */
    uint32_t RESERVED3;                 /* 0x68 */
    volatile uint32_t IC_ENABLE;        /* 0x6c */
    volatile uint32_t IC_STATUS;        /* 0x70 */
    volatile uint32_t IC_TXFLR;         /* 0x74 tx FIFO level */
    volatile uint32_t IC_RXFLR;         /* 0x78 rx FIFO level */
    uint32_t RESERVED4;                 /* 0x7c */
    volatile uint32_t IC_TX_ABRT_SRC;   /* 0x80 */
    uint32_t RESERVED5[6];              /* 0x84 - 0x98 */
    volatile uint32_t IC_ENABLE_STATUS; /* 0x9c bit0: IC_EN settled */
    volatile uint32_t IC_START;         /* 0xa0 HR_C7000-specific trigger */
} I2C_TypeDef;

#define I2C_CMD_READ 0x100u        /* IC_DATA_CMD bit8: read request */
#define I2C_CMD_STOP 0x200u        /* IC_DATA_CMD bit9: STOP after byte */
#define I2C_STA_ACTIVITY (1u << 0) /* bus busy */
#define I2C_STA_TFNF (1u << 1)     /* tx FIFO not full */
#define I2C_STA_TFE (1u << 2)      /* tx FIFO empty */
#define I2C_STA_RFNE (1u << 3)     /* rx FIFO not empty */
#define I2C_CON_MASTER_FS 0x65u    /* master, 7-bit, fast-mode, restart-en */
/* 400 kHz off the 42 MHz APB clock (clk/400000 = 105, split hi/lo). */
#define I2C_SCL_HCNT 0x2cu
#define I2C_SCL_LCNT 0x34u
#define RTC_I2C_SLAVE 0x70u /* RTC device address (7'b1110000) */

/* -------------------------------------------------------------------------
 *  ADC -- HR_C7000 on-chip ADC (base 0x140d0000, manual 5.9, Register Table 24).
 *  The battery pack sits on channel 2 -> DATA_CD bits[9:0].
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t CTRL;       /* 0x00 */
    volatile uint32_t INTR_DELTA; /* 0x04 interrupt error range */
    volatile uint32_t INTR;       /* 0x08 interrupt control (bit16 INTR_EN) */
    volatile uint32_t INTR_STA;   /* 0x0c interrupt status */
    volatile uint32_t SCAN_TIME;  /* 0x10 scan time interval */
    volatile uint32_t START;      /* 0x14 write 1 to start a conversion */
    volatile uint32_t CTRL_STATE; /* 0x18 bit0 BUSY, bits[5:1] SAMP_FSM */
    volatile uint32_t INTR_THRESHOLE; /* 0x1c interrupt initial value/threshold */
    volatile uint32_t CTRL_STOP;      /* 0x20 */
    volatile uint32_t CH_VLD;         /* 0x24 channel-enable bitmask (1<<ch) */
    volatile uint32_t SEOC_TIME;      /* 0x28 */
    volatile uint32_t P2S_EN;         /* 0x2c */
    volatile uint32_t DATA_AB;        /* 0x30 ch0 [9:0] / ch1 [25:16] */
    volatile uint32_t DATA_CD;        /* 0x34 ch2 (battery) / ch3 */
    volatile uint32_t DATA_EF;        /* 0x38 ch4 / ch5 */
    volatile uint32_t DATA_GH;        /* 0x3c ch6 / ch7 */
} ADC_TypeDef;

/* -------------------------------------------------------------------------
 *  UART -- DesignWare 16550-compatible, 32-bit word-spaced registers. UART2
 *  (base 0x14050000) is the GPS module port. RBR/THR/DLL alias at 0x00 and
 *  IER/DLH alias at 0x04, selected by the LCR DLAB bit (bit 7).
 * ------------------------------------------------------------------------- */
typedef struct {
    union {
        volatile uint32_t RBR; /* 0x00 RX buffer (read) */
        volatile uint32_t THR; /* 0x00 TX holding (write) */
        volatile uint32_t DLL; /* 0x00 divisor latch low (LCR DLAB = 1) */
    };
    union {
        volatile uint32_t IER; /* 0x04 interrupt enable */
        volatile uint32_t DLH; /* 0x04 divisor latch high (LCR DLAB = 1) */
    };
    union {
        volatile uint32_t IIR; /* 0x08 interrupt identification (read) */
        volatile uint32_t FCR; /* 0x08 FIFO control (write) */
    };
    volatile uint32_t LCR;     /* 0x0c line control (bit 7 = DLAB) */
    volatile uint32_t MCR;     /* 0x10 modem control */
    volatile uint32_t LSR;     /* 0x14 line status (bit 0 = data ready) */
    volatile uint32_t MSR;     /* 0x18 modem status */
    volatile uint32_t SCR;     /* 0x1c scratch */
} UART_TypeDef;

#define UART_LCR_DLAB 0x80u   /* LCR bit 7: divisor-latch access */
#define UART_LCR_8N1 0x03u    /* 8 data bits, no parity, 1 stop */
#define UART_FCR_ENABLE 0x67u /* FIFO enable + RX/TX reset + trigger level */
#define UART_LSR_DR 0x01u     /* LSR bit 0: RX data ready */

/* -------------------------------------------------------------------------
 *  CPU DAC (base 0x140f0000, manual 5.10). Three 12-bit channels; PD bits are
 *  active-HIGH power-DOWN (reset = all channels down). The analog-FM path uses
 *  DATA_C as the AF-receive bias and DATA_B as the TX APC power-ramp target.
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t PD_CTRL;    /* 0x00 [2:0] C/B/A power (1 = down) */
    volatile uint32_t PD_MODE_EN; /* 0x04 power-mode enable */
    volatile uint32_t DATA_A;     /* 0x08 channel A data */
    volatile uint32_t DATA_B;     /* 0x0c channel B data (TX APC ramp) */
    volatile uint32_t DATA_C;     /* 0x10 channel C data (AF-receive bias) */
} DAC_TypeDef;

/* -------------------------------------------------------------------------
 *  Codec AFE control block (base 0x16000900, BYTE-addressed). The codec config
 *  interface is a byte-wide register file poked at sparse offsets (0xc0-0xef),
 *  not word registers, so it is accessed via a byte accessor rather than a
 *  struct. Offsets/roles verified live against a vendor unit playing FM.
 * ------------------------------------------------------------------------- */
#define CODEC_BASE 0x16000900u
#define CODEC_BYTE(off) (*(volatile uint8_t *)(CODEC_BASE + (off)))

/* -------------------------------------------------------------------------
 *  Peripheral instances -- cast the base address onto the register struct.
 * ------------------------------------------------------------------------- */
#define SOCSYS ((SOCSYS_TypeDef *)0x11000000u)
#define GPIOA ((GPIO_TypeDef *)0x14020000u)
#define GPIOB ((GPIO_TypeDef *)0x14100000u)
#define GPIOC ((GPIO_TypeDef *)0x14110000u)
#define LCD_HW ((LCD_TypeDef *)0x12000000u)
#define PWM_CH0 ((PWM_Channel_TypeDef *)0x140c0000u) /* LCD backlight */
#define I2C1 ((I2C_TypeDef *)0x14070000u) /* radio bus: AT1846S transceiver */
#define I2C2 ((I2C_TypeDef *)0x14080000u) /* internal bus: RTC */
#define ADC_HW ((ADC_TypeDef *)0x140d0000u)
#define DAC_HW ((DAC_TypeDef *)0x140f0000u)
#define UART2 ((UART_TypeDef *)0x14050000u) /* GPS module port */

#endif                                      /* HRC7000_REGISTERS_H */
