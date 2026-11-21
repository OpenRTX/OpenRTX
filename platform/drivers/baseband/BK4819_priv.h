/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * SPDX-FileCopyrightText: Copyright 2024 Jamiexu (Jamie793)
 * SPDX-FileCopyrightText: Copyright 2024 Andrej (Tunas1337)
 *
 * SPDX-License-Identifier: GPL-3.0-or-later AND MIT
 *
 * Based on Jamie793's MIT-licensed BK4819 driver for the Talkpod A36Plus,
 * https://github.com/Jamie793/A36Plus_Radio, as extended by Andrej in
 * https://github.com/OpenRTX/OpenRTX/pull/278.
 */

#ifndef BK4819_PRIV_H
#define BK4819_PRIV_H

/*
 * Register fields used by the BK4819 driver. From the BK4819 (V3) register
 * list and application note:
 * https://github.com/amnemonic/Quansheng_UV-K5_Firmware/tree/main/hardware
 * and the UV-K5 firmware's register header:
 * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/driver/bk4819-regs.h
 */

#ifndef BIT
#define BIT(n) (1U << (n))
#endif

/* Serial bus command */
enum {
    BK4819_REG_READ = BIT(7),
    BK4819_REG_WRITE = 0,
};

/* REG_07: CTCSS/CDCSS frequency */
enum {
    BK4819_REG07_MODE_CDCSS = 0x4000,
    /* CDCSS bit rate, 134.4 Hz * 20.64888 for a 13/26 MHz crystal */
    BK4819_REG07_CDCSS_BAUD_134_4HZ = 0x0ad7,
};

/* REG_08: CDCSS code */
enum {
    BK4819_REG08_CDCSS_HIGH = BIT(15), ///< High 12 bits; clear for the low.
    BK4819_REG08_CDCSS_CODE_MASK = 0x0fff,
};

/* REG_0C: status (read only) */
enum {
    BK4819_REG0C_CTC1_RECEIVED = BIT(10),
    BK4819_REG0C_VOX = BIT(2),
};

/* REG_2B: AF filters */
enum {
    /* RX HPF, LPF and de-emphasis, TX HPF, LPF1 and pre-emphasis */
    BK4819_REG2B_AF_FILTERS_DISABLE = 0x0707,
};

/* REG_30: power and signal path enables */
enum {
    BK4819_REG30_VCO_CALIB_ENABLE = BIT(15),
    BK4819_REG30_UNDOC14 = BIT(14),
    BK4819_REG30_RX_LINK_ENABLE = 0x3c00,
    BK4819_REG30_AF_DAC_ENABLE = BIT(9),
    BK4819_REG30_DISC_MODE_ENABLE = BIT(8),
    BK4819_REG30_PLL_VCO_ENABLE = 0x00f0,
    BK4819_REG30_PA_GAIN_ENABLE = BIT(3),
    BK4819_REG30_MIC_ADC_ENABLE = BIT(2),
    BK4819_REG30_TX_DSP_ENABLE = BIT(1),
    BK4819_REG30_RX_DSP_ENABLE = BIT(0),
};

/* REG_31: audio processing */
enum {
    BK4819_REG31_COMPANDER = BIT(3),
    BK4819_REG31_VOX = BIT(2),
};

/* REG_37: power and LDOs */
enum {
    /*
     * DSP voltage 1, ANA, VCO, RF and PLL LDOs at 2.7 V, reserved bit 3 set,
     * DSP, crystal and band gap on, as the UV-K5 firmware's RX_TurnOn():
     * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/driver/bk4819.c#L346-L361
     */
    BK4819_REG37_ACTIVE = 0x1f0f,
};

/* REG_40: TX deviation */
enum {
    BK4819_REG40_TX_DEVIATION_ENABLE = BIT(12),
    BK4819_REG40_TX_DEVIATION_MASK = 0x0fff,
};

#define BK4819_REG40_TX_DEVIATION(x) ((x) & BK4819_REG40_TX_DEVIATION_MASK)

/* REG_43: RF and AF TX filters, bandwidth mode */
enum {
    BK4819_REG43_RF_BW_MASK = 0x7000,
    BK4819_REG43_RF_BW_WEAK_MASK = 0x0e00,
    BK4819_REG43_AF_TX_LPF2_BW_MASK = 0x01c0,
    BK4819_REG43_BW_MODE_MASK = 0x0030,
    BK4819_REG43_UNDOC3 = BIT(3),
};

#define BK4819_REG43_RF_BW(x) (((x) << 12) & BK4819_REG43_RF_BW_MASK)
#define BK4819_REG43_RF_BW_WEAK(x) (((x) << 9) & BK4819_REG43_RF_BW_WEAK_MASK)
#define BK4819_REG43_AF_TX_LPF2_BW(x) \
    (((x) << 6) & BK4819_REG43_AF_TX_LPF2_BW_MASK)
#define BK4819_REG43_BW_MODE(x) (((x) << 4) & BK4819_REG43_BW_MODE_MASK)

/* REG_47: AF output */
enum {
    BK4819_REG47_AF_TX_FILTER_BYPASS = BIT(0),
    /* Undocumented, "for gain" in the application note's RF_EnterBypass() */
    BK4819_REG47_UNDOC1 = BIT(1),
    BK4819_REG47_BYPASS = BK4819_REG47_AF_TX_FILTER_BYPASS
                        | BK4819_REG47_UNDOC1,
    BK4819_REG47_AF_OUTPUT_MASK = 0x0f00,
};

#define BK4819_REG47_AF_OUTPUT(x) (((x) << 8) & BK4819_REG47_AF_OUTPUT_MASK)

/* REG_48: AF RX gains */
enum {
    BK4819_REG48_AF_RX_GAIN1_MASK = 0x0c00,
    BK4819_REG48_AF_RX_GAIN2_MASK = 0x03f0,
};

#define BK4819_REG48_AF_RX_GAIN1(x) \
    (((x) << 10) & BK4819_REG48_AF_RX_GAIN1_MASK)
#define BK4819_REG48_AF_RX_GAIN2(x) (((x) << 4) & BK4819_REG48_AF_RX_GAIN2_MASK)

/* REG_4B: ALC */
enum {
    BK4819_REG4B_ALC_DISABLE = BIT(5),
};

/*
 * REG_4D, REG_4E, REG_4F: squelch glitch and noise thresholds. "Close" is the
 * register list's "squelch = 0" threshold, "open" its "squelch = 1".
 */
enum {
    BK4819_REG4D_GLITCH_CLOSE_MASK = 0x00ff,
    BK4819_REG4E_GLITCH_OPEN_MASK = 0x00ff,
    BK4819_REG4F_NOISE_CLOSE_MASK = 0x7f00,
    BK4819_REG4F_NOISE_OPEN_MASK = 0x007f,
};

#define BK4819_REG4D_GLITCH_CLOSE(x) ((x) & BK4819_REG4D_GLITCH_CLOSE_MASK)
#define BK4819_REG4E_GLITCH_OPEN(x) ((x) & BK4819_REG4E_GLITCH_OPEN_MASK)
#define BK4819_REG4F_NOISE_CLOSE(x) (((x) << 8) & BK4819_REG4F_NOISE_CLOSE_MASK)
#define BK4819_REG4F_NOISE_OPEN(x) ((x) & BK4819_REG4F_NOISE_OPEN_MASK)

/* REG_51: CTCSS/CDCSS */
enum {
    BK4819_REG51_CXCSS_TX_ENABLE = BIT(15),
    BK4819_REG51_TX_CDCSS_NEGATIVE = BIT(13),
    BK4819_REG51_CTCSS_MODE = BIT(12), ///< Set for CTCSS, clear for CDCSS.
    BK4819_REG51_CDCSS_24BIT = BIT(11),
    BK4819_REG51_CXCSS_TX_GAIN_MASK = 0x007f,
};

#define BK4819_REG51_CXCSS_TX_GAIN(x) ((x) & BK4819_REG51_CXCSS_TX_GAIN_MASK)

/* REG_67: RSSI (read only) */
enum {
    BK4819_REG67_RSSI_MASK = 0x01ff, ///< 0.5 dB/step.
};

/* REG_6F: AF TX/RX input amplitude (read only) */
enum {
    BK4819_REG6F_AF_AMPLITUDE_MASK = 0x007f,
};

/* REG_70: tone generator */
enum {
    BK4819_REG70_TONE1_ENABLE = BIT(15),
    BK4819_REG70_TONE1_GAIN_MASK = 0x7f00,
    BK4819_REG70_TONE2_ENABLE = BIT(7),
};

#define BK4819_REG70_TONE1_GAIN(x) (((x) << 8) & BK4819_REG70_TONE1_GAIN_MASK)

/* REG_78: squelch RSSI thresholds, 0.5 dB/step */
enum {
    BK4819_REG78_RSSI_OPEN_MASK = 0xff00,
    BK4819_REG78_RSSI_CLOSE_MASK = 0x00ff,
};

#define BK4819_REG78_RSSI_OPEN(x) (((x) << 8) & BK4819_REG78_RSSI_OPEN_MASK)
#define BK4819_REG78_RSSI_CLOSE(x) ((x) & BK4819_REG78_RSSI_CLOSE_MASK)

/* REG_79: VOX timing */
#define BK4819_REG79_VOX_INTERVAL(x) ((x) << 10)

/* REG_7D: microphone sensitivity */
enum {
    BK4819_REG7D_MIC_GAIN_MASK = 0x001f,
};

#define BK4819_REG7D_MIC_GAIN(x) ((x) & BK4819_REG7D_MIC_GAIN_MASK)

/* REG_7E: DC filters */
enum {
    BK4819_REG7E_TX_DCF_BW_MASK = 0x0038,
    BK4819_REG7E_RX_DCF_BW_MASK = 0x0007,
};

#define BK4819_REG7E_TX_DCF_BW(x) (((x) << 3) & BK4819_REG7E_TX_DCF_BW_MASK)
#define BK4819_REG7E_RX_DCF_BW(x) ((x) & BK4819_REG7E_RX_DCF_BW_MASK)

/* Field values set by bk4819_init() */
enum {
    BK4819_DEFAULT_TX_DEVIATION = 0x4d2,
    BK4819_DEFAULT_RX_AUDIO_GAIN1 = 0,
    BK4819_DEFAULT_RX_AUDIO_GAIN2 = 0x3c,
    BK4819_DEFAULT_MIC_GAIN = 0x19,
    BK4819_DEFAULT_TX_DCF_BW = 5,
    BK4819_DEFAULT_RX_DCF_BW = 6,
};

#endif /* BK4819_PRIV_H */
