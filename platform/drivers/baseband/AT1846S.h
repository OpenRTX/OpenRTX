/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AT1846S_H
#define AT1846S_H

#include <stdint.h>
#include <stdbool.h>
#include "core/datatypes.h"

/**
 * Enumeration type defining the bandwidth settings supported by the AT1846S chip.
 */
enum class AT1846S_BW : uint8_t
{
    _12P5 = 0,    ///< 12.5kHz bandwidth.
    _25   = 1     ///< 25kHz bandwidth.
};

/**
 * Enumeration type defining the possible operating mode configurations for the
 * AT1846S chip.
 */
enum class AT1846S_OpMode : uint8_t
{
    FM  = 0,      ///< Analog FM operation.
    DMR = 1       ///< DMR operation.
};

/**
 * Enumeration type defining the AT1846S functional modes.
 */
enum class AT1846S_FuncMode : uint8_t
{
    OFF = 0,      ///< Both TX and RX off.
    RX  = 1,      ///< RX enabled.
    TX  = 2,      ///< TX enabled.
};

/**
 * Low-level driver for AT1846S "radio on a chip" integrated circuit.
 */

class AT1846S
{
public:

    /**
     * \return a reference to the instance of the AT1846S class (singleton).
     */
    static AT1846S& instance()
    {
        static AT1846S instance;
        return instance;
    }

    /**
     * Destructor.
     * When called it implicitly shuts down the AT146S chip.
     */
    ~AT1846S()
    {
        terminate();
    }

    /**
     * Initialise the AT146S chip.
     */
    void init();

    /**
     * Shut down the AT146S chip.
     */
    inline void terminate()
    {
        disableCtcss();
        setFuncMode(AT1846S_FuncMode::OFF);
    }

    /**
     * Set the VCO frequency, either for transmission or reception.
     *
     * @param freq: VCO frequency.
     */
    void setFrequency(const freq_t freq);

    /**
     * Set the transmission and reception bandwidth.
     *
     * @param band: bandwidth.
     */
    void setBandwidth(const AT1846S_BW band);

    /**
     * Set the operating mode.
     *
     * @param mode: operating mode.
     */
    void setOpMode(const AT1846S_OpMode mode);

    /**
     * Set the functional mode.
     *
     * @param mode: functional mode.
     */
    void setFuncMode(const AT1846S_FuncMode mode);

    /**
     * Setup and enable tone output
     * @param freq frequency in 1/10 Hz
     */
    void enableTone(const tone_t freq)
    {
        i2c_writeReg16(0x35, freq); // Set tone 1 freq
        maskSetRegister(0x3A, 0x7000, 0x1000); // Use tone 1
        maskSetRegister(0x79, 0xF000, 0xC000); // Enable tone output
    }

    /**
     * Change output back to microphone
     */
    void disableTone()
    {
        maskSetRegister(0x3A, 0x7000, 0x4000); // Use microphone
        maskSetRegister(0x79, 0xF000, 0x0000); // Tone output off
    }

    /**
     * Emit the 1750 Hz tone burst (European/German repeater access tone).
     * Wraps enableTone(); pair with disableTone() to end the burst.
     */
    void enableToneBurst1750() { enableTone(17500); }   // 1750.0 Hz, 1/10 Hz units

    /**
     * Enable the CTCSS tone for transmission.
     *
     * @param freq: CTCSS tone frequency.
     */
    void enableTxCtcss(const tone_t freq)
    {
        i2c_writeReg16(0x4A, freq*10);          // Set CTCSS1 frequency reg.
        i2c_writeReg16(0x4B, 0x0000);           // Clear CDCSS bits
        i2c_writeReg16(0x4C, 0x0000);
        maskSetRegister(0x4E, 0x0600, 0x0600);  // Enable CTCSS TX
    }

    /**
     * Enable the CTCSS tone detection during reception.
     *
     * @param freq: CTCSS tone frequency.
     */
    void enableRxCtcss(const tone_t freq)
    {
        i2c_writeReg16(0x4D, freq*10);          // Set CTCSS2 frequency reg.
        i2c_writeReg16(0x5B, getCtcssThreshFromTone(freq));
        maskSetRegister(0x3A, 0x001F, 0x0008);  // Enable CTCSS2 freq. detection
    }

    /**
     * Check if CTCSS tone is detected when in RX mode.
     *
     * @return true if the RX CTCSS tone is being detected.
     */
    inline bool rxCtcssDetected()
    {
        // Check if CTCSS detection is enabled: if not, return false.
        if((i2c_readReg16(0x3A) & 0x0008) == 0) return false;

        // Check CTCSS2 compare flag
        uint16_t reg  = i2c_readReg16(0x1C);
        return ((reg & 0x100) != 0);
    }

    /**
     * Turn off both transmission CTCSS tone and reception CTCSS tone decoding.
     */
    inline void disableCtcss()
    {
        maskSetRegister(0x4E, 0x0600, 0x0000);  // Disable TX CTCSS
        maskSetRegister(0x3A, 0x001F, 0x0000);  // Disable CTCSS freq. detection
        i2c_writeReg16(0x4A, 0x0000);           // Clear CTCSS1 frequency reg.
        i2c_writeReg16(0x4D, 0x0000);           // Clear CTCSS2 frequency reg.
    }

    /**
     * Build the 23-bit AT1846S CDCSS codeword (regs 0x4B/0x4C) for a DCS code.
     *
     * The chip wants the standard 23-bit Golay(23,12) DCS word, NOT the raw
     * octal code: the low 9 bits carry the code value, the next 3 bits are the
     * fixed "100" pattern, and the high 11 bits are the Golay parity computed
     * with the standard generator g(x)=0xC75.  Verified against the AT1846S
     * programming guide example (DCS023 -> 0x4B=0x0076, 0x4C=0x3813).
     *
     * @param octCode: the DCS code as a 9-bit octal value (e.g. 0023 -> 19).
     * @return the 24-bit codeword (bits 23:0); reg 0x4B = bits 23:16, 0x4C = 15:0.
     */
    static inline uint32_t dcsCodeword(const uint16_t octCode)
    {
        uint16_t data = 0x800u | (octCode & 0x1FFu);   // "100" + 9-bit code
        uint32_t reg  = (uint32_t)data << 11;
        for(int i = 22; i >= 11; --i)
            if(reg & (1u << i)) reg ^= (uint32_t)0xC75u << (i - 11);
        uint16_t parity = (uint16_t)(reg & 0x7FFu);
        return ((uint32_t)parity << 12) | data;
    }

    /**
     * Enable DCS (CDCSS) sub-audio code for transmission, generated by the
     * AT1846S itself (0x4E[10:9]=10).  Standard 23-bit code; 0x4A must be
     * 134.4 Hz in CDCSS mode (programming guide ch.7).
     *
     * @param octCode: 9-bit octal DCS code value.
     * @param invert:  true for inverted (negative) DCS (0x4E[8]=1).
     */
    void enableTxCdcss(const uint16_t octCode, const bool invert)
    {
        uint32_t cw = dcsCodeword(octCode);
        i2c_writeReg16(0x4A, 13440);                       // 134.4 Hz * 100
        i2c_writeReg16(0x4B, (uint16_t)((cw >> 16) & 0xFF));
        i2c_writeReg16(0x4C, (uint16_t)(cw & 0xFFFF));
        // 0x4E[10:9]=10 (CDCSS by 1846S), [8]=invert, [7]=1 (req'd), [6]=0 (23-bit)
        maskSetRegister(0x4E, 0x07C0, (uint16_t)(0x0480u | (invert ? 0x0100u : 0u)));
    }

    /**
     * Enable DCS (CDCSS) code detection during reception.
     *
     * @param octCode: 9-bit octal DCS code value.
     * @param invert:  true to detect inverted (negative) DCS.
     */
    void enableRxCdcss(const uint16_t octCode, const bool invert)
    {
        uint32_t cw = dcsCodeword(octCode);
        i2c_writeReg16(0x4A, 13440);                       // 134.4 Hz * 100
        i2c_writeReg16(0x4B, (uint16_t)((cw >> 16) & 0xFF));
        i2c_writeReg16(0x4C, (uint16_t)(cw & 0xFFFF));
        maskSetRegister(0x4E, 0x0040, 0x0000);             // [6]=0 -> 23-bit CDCSS
        maskSetRegister(0x58, 0x0008, 0x0000);             // [3]=0 -> don't bypass HPF
        // 0x3A[5]=0 output compared result; [2]=detect invert / [1]=detect normal
        maskSetRegister(0x3A, 0x003F, (uint16_t)(invert ? 0x0004u : 0x0002u));
    }

    /**
     * Check if the programmed DCS code is detected in RX mode.
     *
     * @param invert: true if an inverted code was programmed.
     * @return true if the RX DCS code matches.
     */
    inline bool rxCdcssDetected(const bool invert)
    {
        // Bit set in 0x3A[2:1] only when DCS detection is enabled.
        if((i2c_readReg16(0x3A) & 0x0006) == 0) return false;
        uint16_t reg = i2c_readReg16(0x1C);
        // 0x1C[7]=cdcss positive(normal) cmp, [6]=cdcss negative(invert) cmp.
        return ((reg & (invert ? 0x0040u : 0x0080u)) != 0);
    }

    /**
     * Turn off both DCS transmission and DCS detection.
     */
    inline void disableCdcss()
    {
        maskSetRegister(0x4E, 0x07C0, 0x0000);  // Disable TX CDCSS + sel bits
        maskSetRegister(0x3A, 0x001F, 0x0000);  // Disable CDCSS detection
        i2c_writeReg16(0x4B, 0x0000);
        i2c_writeReg16(0x4C, 0x0000);
    }

    /**
     * Set the RSSI squelch open/close thresholds (reg 0x49: th_h_sq[13:7]
     * open, th_l_sq[6:0] close).  Field = 137 + dBm, 7 bits each, matching the
     * readRSSI() scale (-137 + reg).  0x49 is NOT in the per-bandwidth bank
     * list, so it survives setBandwidth(); apply after a band change.
     */
    inline void setSqlThresholds(const int16_t openDbm, const int16_t closeDbm)
    {
        uint16_t hi = (uint16_t)((137 + openDbm)  & 0x7F);
        uint16_t lo = (uint16_t)((137 + closeDbm) & 0x7F);
        i2c_writeReg16(0x49, (uint16_t)((hi << 7) | lo));
    }

    /**
     * Apply the vendor 0..9 squelch level to the AT1846S RSSI squelch.
     * close = -127 + 6*level dBm, open = close + 2 dB (2 dB HW hysteresis).
     * Level clamped to 0..9.
     */
    inline void setSquelchLevel(uint8_t level)
    {
        if(level > 9) level = 9;
        int16_t close = (int16_t)(-127 + 6 * (int)level);
        setSqlThresholds((int16_t)(close + 2), close);
    }

    /**
     * Set the full TX FM deviation word (reg 0x59): xmitterDev[15:6] = voice +
     * sub-audio deviation, cDev[5:0] = CTCSS/CDCSS-only deviation.  Note 0x59
     * is shared with the RX mixer gain, so set on TX entry and restore for RX.
     */
    inline void setTxFmDeviation(const uint16_t xmitterDev, const uint8_t cDev)
    {
        i2c_writeReg16(0x59, (uint16_t)(((xmitterDev & 0x03FF) << 6) | (cDev & 0x3F)));
    }

    /**
     * Select the CTCSS/DCS tail-elimination reverse-burst phase shift
     * (reg 0x4E[15:14]).  Call right after enableTxCtcss/enableTxCdcss; engage
     * the burst on dekey by setting reg 0x30[11] (tail_elim_en).
     */
    enum TailShift : uint16_t
    { TAIL_NONE = 0x0000, TAIL_120 = 0x4000, TAIL_180 = 0x8000, TAIL_240 = 0xC000 };
    inline void setTxTailShift(const TailShift shift)
    {
        maskSetRegister(0x4E, 0xC000, (uint16_t)shift);
    }

    /**
     * Enable the AT1846S VOX detector (detect-during-RX mode): program the
     * open/shut thresholds (reg 0x64: th_h[13:7], th_l[6:0]) and set vox_on
     * (reg 0x30 bit4).  Poll voxDetected() for the result.  No GPIO7 routing
     * or tx_adc/dsp reset (keeps the RX bank intact).
     */
    inline void enableVox(const uint8_t thHigh, const uint8_t thLow)
    {
        i2c_writeReg16(0x64, (uint16_t)(((thHigh & 0x7F) << 7) | (thLow & 0x7F)));
        maskSetRegister(0x30, 0x0010, 0x0010);  // vox_on = 1
    }
    inline void disableVox() { maskSetRegister(0x30, 0x0010, 0x0000); }
    inline bool voxDetected()
    {
        if((i2c_readReg16(0x30) & 0x0010) == 0) return false;   // vox_on off
        return ((i2c_readReg16(0x1C) & 0x0002) != 0);           // vox_cmp
    }

    /* ---- DTMF (AT1846S engine, programming guide ch.10) -------------- */

    /** Map an ASCII DTMF digit to its (row,col) tone-pair freqs in Hz.
     *  Accepts 0-9, A-D (case-insensitive), '*', '#'.  Returns false otherwise. */
    static inline bool dtmfDigitToTones(char d, uint16_t *row, uint16_t *col)
    {
        static const uint16_t rows[4] = { 697, 770, 852, 941 };
        static const uint16_t cols[4] = { 1209, 1336, 1477, 1633 };
        int r, c;
        switch(d)
        {
            case '1': r=0;c=0;break; case '2': r=0;c=1;break; case '3': r=0;c=2;break;
            case '4': r=1;c=0;break; case '5': r=1;c=1;break; case '6': r=1;c=2;break;
            case '7': r=2;c=0;break; case '8': r=2;c=1;break; case '9': r=2;c=2;break;
            case '*': r=3;c=0;break; case '0': r=3;c=1;break; case '#': r=3;c=2;break;
            case 'A': case 'a': r=0;c=3;break; case 'B': case 'b': r=1;c=3;break;
            case 'C': case 'c': r=2;c=3;break; case 'D': case 'd': r=3;c=3;break;
            default: return false;
        }
        *row = rows[r]; *col = cols[c];
        return true;
    }

    /** Map a decoded reg-0x7E[3:0] code to an ASCII digit. */
    static inline char dtmfCodeToChar(uint8_t code)
    {
        static const char tbl[16] = {'0','1','2','3','4','5','6','7',
                                     '8','9','A','B','C','D','*','#'};
        return tbl[code & 0x0F];
    }

    /** Enable DTMF decode on RX (reg 0x7A[15]=1 + detect time 0x18).  NOTE: the
     *  0x67..0x76 Goertzel coefficients are left at their power-on default
     *  (tuned for a 12.8/25.6 MHz reference); the HD2 runs 26 MHz, so decode
     *  reliability must be HW-confirmed (diag 't') -- if poor, reprogram the
     *  coefficient table with verified 26 MHz values. */
    inline void dtmfEnableDecode()  { maskSetRegister(0x7A, 0x80FF, 0x8018); }
    inline bool dtmfDecodeReady()   { return (i2c_readReg16(0x7E) & 0x0010) != 0; }
    inline char dtmfReadDigit()     { return dtmfCodeToChar((uint8_t)(i2c_readReg16(0x7E) & 0x0F)); }
    inline void dtmfDisableDecode() { maskSetRegister(0x7A, 0x8000, 0x0000); }

    /**
     * Get current RSSI value.
     *
     * @return current RSSI in dBm.
     */
    inline int16_t readRSSI()
    {
        // RSSI value is contained in the upper 8 bits of register 0x1B.
        return -137 + static_cast< int16_t >(i2c_readReg16(0x1B) >> 8);
    }

    /**
     * Set the gain of internal programmable gain amplifier.
     *
     * @param gain: PGA gain.
     */
    inline void setPgaGain(const uint8_t gain)
    {
        uint16_t pga = (gain & 0x1F) << 6;
        maskSetRegister(0x0A, 0x07C0, pga);
    }

    /**
     * Set microphone gain for transmission.
     *
     * @param gain: microphone gain.
     */
    inline void setMicGain(const uint8_t gain)
    {
        maskSetRegister(0x41, 0x007F, static_cast< uint16_t >(gain));
    }

    /**
     * Set maximum FM transmission deviation.
     *
     * @param dev: maximum allowed deviation.
     */
    inline void setTxDeviation(const uint16_t dev)
    {
        uint16_t value = (dev & 0x03FF) << 6;
        maskSetRegister(0x59, 0xFFC0, value);
    }

    /**
     * Set the gain for internal automatic gain control system.
     *
     * @param gain: AGC gain.
     */
    inline void setAgcGain(const uint8_t gain)
    {
        uint16_t agc = (gain & 0x0F) << 8;
        maskSetRegister(0x44, 0x0F00, agc);
    }

    /**
     * Set audio gain for recepion.
     *
     * @param analogDacGain: "analog DAC gain" in AT1846S manual.
     * @param digitalGain: "digital voice gain" in AT1846S manual.
     */
    inline void setRxAudioGain(const uint8_t analogDacGain,
                               const uint8_t digitalGain)
    {
        uint16_t value = (analogDacGain & 0x0F) << 4;
        maskSetRegister(0x44, 0x00F0, value);
        maskSetRegister(0x44, 0x000F, static_cast< uint16_t >(digitalGain));
    }

    /**
     * Set noise1 thresholds for squelch opening and closing.
     *
     * @param highTsh: upper threshold.
     * @param lowTsh: lower threshold.
     */
    inline void setNoise1Thresholds(const uint8_t highTsh, const uint8_t lowTsh)
    {
        uint16_t value = ((highTsh & 0x1F) << 8) | (lowTsh & 0x1F);
        i2c_writeReg16(0x48, value);
    }

    /**
     * Set noise2 thresholds for squelch opening and closing.
     *
     * @param highTsh: upper threshold.
     * @param lowTsh: lower threshold.
     */
    inline void setNoise2Thresholds(const uint8_t highTsh, const uint8_t lowTsh)
    {
        uint16_t value = ((highTsh & 0x1F) << 8) | (lowTsh & 0x1F);
        i2c_writeReg16(0x60, value);
    }

    /**
     * Set RSSI thresholds for squelch opening and closing.
     *
     * @param highTsh: upper threshold.
     * @param lowTsh: lower threshold.
     */
    inline void setRssiThresholds(const uint8_t highTsh, const uint8_t lowTsh)
    {
        uint16_t value = ((highTsh & 0x1F) << 8) | (lowTsh & 0x1F);
        i2c_writeReg16(0x3F, value);
    }

    /**
     * Set PA drive control bits.
     *
     * @param value: PA drive value.
     */
    inline void setPaDrive(const uint8_t value)
    {
        uint16_t pa = value << 11;
        maskSetRegister(0x0A, 0x7800, pa);
    }

    /**
     * Set threshold for analog FM squelch opening.
     *
     * @param thresh: squelch threshold.
     */
    inline void setAnalogSqlThresh(const uint8_t thresh)
    {
        i2c_writeReg16(0x49, static_cast< uint16_t >(thresh));
    }

    /**
     * Mute the RX audio output while keeping the chip in RX mode.
     */
    inline void muteRxOutput()
    {
        // Setting bit 7 of register 0x30 mutes the RX audio output
        maskSetRegister(0x30, 0x0080, 0x0080);
    }

    /**
     * Unmute the RX audio output.
     */
    inline void unmuteRxOutput()
    {
        // Clearing bit 7 of register 0x30 unmutes the RX audio output
        maskSetRegister(0x30, 0x0080, 0x0000);
    }

private:

    /**
     * Constructor.
     */
    AT1846S()
    {
        i2c_init();
    }

    /**
     * Helper function to set/clear some specific bits in a register.
     *
     * @param reg: address of the register to be changed.
     * @param mask: bitmask to select which bits to change. To modify the i-th
     * bit in the register, set its value to "1" in the bitmask.
     * @param value: New value for the masked bits.
     */
    inline void maskSetRegister(const uint8_t reg, const uint16_t mask,
                                const uint16_t value)
    {
        uint16_t regVal = i2c_readReg16(reg);
        regVal = (regVal & ~mask) | (value & mask);
        i2c_writeReg16(reg, regVal);
    }

    /**
     * Helper function to be called to make effective some of the AT1846S
     * configuration, when changed with TX or RX active.
     * It has been observed that, to make effective a change in some of the main
     * AT1846S parameters, the chip must be "power cycled" by turning it off and
     * then switching back the previous functionality.
     */
    inline void reloadConfig()
    {
        uint16_t funcMode = i2c_readReg16(0x30) & 0x0060;   // Get current op. status
        maskSetRegister(0x30, 0x0060, 0x0000);              // RX and TX off
        maskSetRegister(0x30, 0x0060, funcMode);            // Restore op. status
    }

    /**
     * Initialise the I2C interface.
     */
    void i2c_init();

    /**
     * Write one register via I2C interface.
     *
     * @param reg: address of the register to be written.
     * @param value: value to be written to the register.
     */
    void i2c_writeReg16(const uint8_t reg, const uint16_t value);

    /**
     * Read one register via I2C interface.
     *
     * @param reg: address of the register to be read.
     */
    uint16_t i2c_readReg16(const uint8_t reg);

    /**
     * This function returns the value to be written into the AT1846S CTCSS
     * threshold register when enabling the detection in RX mode.
     * Values were obtained from the function contained in TYT firmware for
     * MD-UV380 version S18.16 at address 0x0806ba2c.
     *
     * @param tone: tone_t variable specifying the CTCSS tone.
     * @return an uint16_t value to be written directly into AT1846S
     *         CTCSS threshold register.
     */
    uint16_t getCtcssThreshFromTone(const tone_t tone)
    {
        switch(tone)
        {
            case 670:  return 0x0C0D; break;    // 67.0 Hz
            case 693:  return 0x0C0C; break;    // 69.3 Hz
            case 719:  return 0x0B0B; break;    // 71.9 Hz
            case 744:                           // 74.4 Hz
            case 770:  return 0x0A0A; break;    // 77.0 Hz
            case 797:                           // 79.7 Hz
            case 825:  return 0x0909; break;    // 82.5 Hz
            case 854:                           // 85.4 Hz
            case 885:  return 0x0808; break;    // 88.5 Hz
            case 915:                           // 91.5 Hz
            case 948:  return 0x0707; break;    // 94.8 Hz
            case 974:  return 0x0706; break;    // 97.4 Hz
            case 1000:                          // 100.0Hz
            case 1034: return 0x0606; break;    // 103.4Hz
            case 1072:                          // 107.2Hz
            case 1109: return 0x0605; break;    // 110.9Hz
            case 1148: return 0x0505; break;    // 114.8Hz
            case 1188:                          // 118.8Hz
            case 1230: return 0x0504; break;    // 123.0Hz
            case 1273:                          // 127.3Hz
            case 1318: return 0x0404; break;    // 131.8Hz
            case 1365:                          // 136.5Hz
            case 1413:                          // 141.3Hz
            case 1462: return 0x0403; break;    // 146.2Hz
            case 1514: return 0x0504; break;    // 151.4Hz
            case 1567:                          // 156.7Hz
            case 1622:                          // 162.2Hz
            case 1679:                          // 167.9Hz
            case 1713:                          // 171.3Hz
            case 1799: return 0x0403; break;    // 179.9Hz
            case 1862: return 0x0400; break;    // 186.2Hz
            case 1928: return 0x0302; break;    // 192.8Hz
            case 2035:                          // 203.5Hz
            case 2107:                          // 210.7Hz
            case 2181:                          // 218.1Hz
            case 2257: return 0x0302; break;    // 225.7Hz
            case 2336:                          // 233.6Hz
            case 2418:                          // 241.8Hz
            case 2503: return 0x0300; break;    // 250.3Hz

                                                // 159.8Hz, 165.5Hz, 173.8Hz,
                                                // 177.3Hz, 183.5Hz, 189.9Hz,
                                                // 196.6Hz, 199.5Hz, 206.5Hz,
            default:   return 0x0505; break;    // 229.1Hz, 254.1Hz
        }
    }
};

#endif /* AT1846S_H */
