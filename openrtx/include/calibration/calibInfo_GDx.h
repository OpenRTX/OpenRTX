/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALIBINFO_GDX_H
#define CALIBINFO_GDX_H

#include "core/datatypes.h"
#include <stdint.h>

/**
 * \brief Calibration data for GDx platforms.
 */
typedef struct
{
    uint16_t _DigitalRxGainNarrowband; // 0x000 IF Gain, RX Fine
    uint16_t _DigitalTxGainNarrowband; // 0x002 IF Gain, TX Fine
    uint16_t _DigitalRxGainWideband;   // 0x004 IF Gain, RX Coarse
    uint16_t _DigitalTxGainWideband;   // 0x006 IF Gain, TX Coarse
    uint16_t modBias;                  // 0x008
    uint8_t mod2Offset;                // 0x00A
    uint8_t txLowPower[16];            // 0x00B - 0x02A
    uint8_t txHighPower[16];           // 0x00B - 0x02A
    uint8_t analogSqlThresh[8];        // 0x03F

    uint8_t noise1_HighTsh_Wb;         // 0x047
    uint8_t noise1_LowTsh_Wb;          // 0x048
    uint8_t noise2_HighTsh_Wb;         // 0x049
    uint8_t noise2_LowTsh_Wb;          // 0x04A
    uint8_t rssi_HighTsh_Wb;           // 0x04B
    uint8_t rssi_LowTsh_Wb;            // 0x04C

    uint8_t noise1_HighTsh_Nb;         // 0x04D
    uint8_t noise1_LowTsh_Nb;          // 0x04E
    uint8_t noise2_HighTsh_Nb;         // 0x04F
    uint8_t noise2_LowTsh_Nb;          // 0x050
    uint8_t rssi_HighTsh_Nb;           // 0x051
    uint8_t rssi_LowTsh_Nb;            // 0x052

    uint8_t RSSILowerThreshold;        // 0x053
    uint8_t RSSIUpperThreshold;        // 0x054

    uint8_t mod1Amplitude[8];          // 0x055

    uint8_t digAudioGain;              // 0x05D

    uint8_t txDev_DTMF;                // 0x05E
    uint8_t txDev_tone;                // 0x05F
    uint8_t txDev_CTCSS_wb;            // 0x060
    uint8_t txDev_CTCSS_nb;            // 0x061
    uint8_t txDev_DCS_wb;              // 0x062
    uint8_t txDev_DCS_nb;              // 0x063

    uint8_t PA_drv;                    // 0x064
    uint8_t PGA_gain;                  // 0x065
    uint8_t analogMicGain;             // 0x066
    uint8_t rxAGCgain;                 // 0x067

    uint16_t mixGainWideband;          // 0x068
    uint16_t mixGainNarrowband;        // 0x06A
    uint8_t rxDacGain;                 // 0x06C
    uint8_t rxVoiceGain;               // 0x06D
}
bandCalData_t;

typedef struct
{
    bandCalData_t data[2];      // Calibration data for VHF (index 0) and UHF (index 1) bands
    freq_t vhfCalPoints[8];     // VHF calibration points for both TX power and mod1Amplitude
    freq_t uhfPwrCalPoints[16]; // UHF calibration points for TX power
    freq_t uhfCalPoints[8];     // UHF calibration points for mod1Amplitude
}
gdxCalibration_t;

#endif /* CALIBINFO_GDX_H */
