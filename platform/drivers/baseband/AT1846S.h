/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef AT1846S_H
#define AT1846S_H

#include <stdint.h>
#include <stdbool.h>
#include <datatypes.h>

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
     * @param freq: VCO frequency.
     */
    void setFrequency(const freq_t freq)
    {
        // The value to be written in registers is given by: 0.0016*freqency
        uint32_t val = (freq/1000)*16;
        uint16_t fHi = (val >> 16) & 0xFFFF;
        uint16_t fLo = val & 0xFFFF;

        i2c_writeReg16(0x29, fHi);
        i2c_writeReg16(0x2A, fLo);

        reloadConfig();
    }

    /**
     * Set the transmission and reception bandwidth.
     * @param band: bandwidth.
     */
    void setBandwidth(const AT1846S_BW band);

    /**
     * Set the operating mode.
     * @param mode: operating mode.
     */
    void setOpMode(const AT1846S_OpMode mode);

    /**
     * Set the functional mode.
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
     * Enable the CTCSS tone for transmission.
     * @param freq: CTCSS tone frequency.
     */
    void enableTxCtcss(const tone_t freq)
    {
        i2c_writeReg16(0x4A, freq*10);
        i2c_writeReg16(0x4B, 0x0000);
        i2c_writeReg16(0x4C, 0x0000);
        maskSetRegister(0x4E, 0x0600, 0x0600);
    }

    /**
     * Turn off both transmission CTCSS tone and reception CTCSS tone decoding.
     */
    inline void disableCtcss()
    {
        i2c_writeReg16(0x4A, 0x0000);
        maskSetRegister(0x4E, 0x0600, 0x0000); // Disable TX CTCSS
    }

    /**
     * Get current RSSI value.
     * @return current RSSI in dBm.
     */
    inline int16_t readRSSI()
    {
        // RSSI value is contained in the upper 8 bits of register 0x1B.
        return -137 + static_cast< int16_t >(i2c_readReg16(0x1B) >> 8);
    }

    /**
     * Set the gain of internal programmable gain amplifier.
     * @param gain: PGA gain.
     */
    inline void setPgaGain(const uint8_t gain)
    {
        uint16_t pga = (gain & 0x1F) << 6;
        maskSetRegister(0x0A, 0x07C0, pga);
    }

    /**
     * Set microphone gain for transmission.
     * @param gain: microphone gain.
     */
    inline void setMicGain(const uint8_t gain)
    {
        maskSetRegister(0x41, 0x007F, static_cast< uint16_t >(gain));
    }

    /**
     * Set maximum FM transmission deviation.
     * @param dev: maximum allowed deviation.
     */
    inline void setTxDeviation(const uint16_t dev)
    {
        uint16_t value = (dev & 0x03FF) << 6;
        maskSetRegister(0x59, 0xFFC0, value);
    }

    /**
     * Set the gain for internal automatic gain control system.
     * @param gain: AGC gain.
     */
    inline void setAgcGain(const uint8_t gain)
    {
        uint16_t agc = (gain & 0x0F) << 8;
        maskSetRegister(0x44, 0x0F00, agc);
    }

    /**
     * Set audio gain for recepion.
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
     * @param value: PA drive value.
     */
    inline void setPaDrive(const uint8_t value)
    {
        uint16_t pa = value << 11;
        maskSetRegister(0x0A, 0x7800, pa);
    }

    /**
     * Set threshold for analog FM squelch opening.
     * @param thresh: squelch threshold.
     */
    inline void setAnalogSqlThresh(const uint8_t thresh)
    {
        i2c_writeReg16(0x49, static_cast< uint16_t >(thresh));
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
};

#endif /* AT1846S_H */
