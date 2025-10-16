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
    void setFrequency(const freq_t freq)
    {
        // AT1846S datasheet specifies a frequency step of 1/16th of kHz per bit.
        // Computation of register value is done using 64 bit to avoid overflows,
        // result is then truncated to 32 bits to fit it into the registers.
        uint64_t val = ((uint64_t) freq * 16) / 1000;
        val &= 0xFFFFFFFF;

        uint16_t fHi = (val >> 16) & 0xFFFF;
        uint16_t fLo = val & 0xFFFF;

        i2c_writeReg16(0x29, fHi);
        i2c_writeReg16(0x2A, fLo);

        reloadConfig();
    }

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
    void setFuncMode(const AT1846S_FuncMode mode)
    {
        /*
         * Functional mode is controlled by bits 5 (RX on) and 6 (TX on) in
         * register 0x30. With a cast and shift we can set it easily.
         */

        uint16_t value = static_cast< uint16_t >(mode) << 5;
        maskSetRegister(0x30, 0x0060, value);
    }

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
    }

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
