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
 * vendor user guide (system-control chapter 4, peripherals chapter 5); the wider
 * modem / codec / RF register set is intentionally omitted from this port. Base
 * addresses, pin-mux masks and "magic" configuration words are hardware facts
 * recovered by on-device reverse engineering of the vendor V2.1.3 firmware; they
 * are not guessed.
 */

#ifndef HRC7000_REGISTERS_H
#define HRC7000_REGISTERS_H

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
} SOCSYS_TypeDef;

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

/* HD2 board GPIOB pin bits (LED/PTT/PWR/GPS) live in targets/HD2/pinmap.h. */

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
 *  Peripheral instances -- cast the base address onto the register struct.
 * ------------------------------------------------------------------------- */
#define SOCSYS ((SOCSYS_TypeDef *)0x11000000u)
#define GPIOA ((GPIO_TypeDef *)0x14020000u)
#define GPIOB ((GPIO_TypeDef *)0x14100000u)
#define GPIOC ((GPIO_TypeDef *)0x14110000u)
#define LCD_HW ((LCD_TypeDef *)0x12000000u)
#define PWM_CH0 ((PWM_Channel_TypeDef *)0x140c0000u)
#define I2C2 ((I2C_TypeDef *)0x14080000u)
#define ADC_HW ((ADC_TypeDef *)0x140d0000u)
#define UART2 ((UART_TypeDef *)0x14050000u) /* GPS module port */

#endif                                      /* HRC7000_REGISTERS_H */
