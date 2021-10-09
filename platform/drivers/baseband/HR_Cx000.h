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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef HRCx000_H
#define HRCx000_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check if "user" SPI bus, on some platforms shared between the baseband and
 * other chips, is in use by the HR_Cx000 driver. This function is callable
 * either from C or C++ sources.
 *
 * WARNING: this function is NOT thread safe! A proper critical section has to
 * be set up to ensure it is accessed by one task at a time.
 *
 * @return true if SPI lines are being used by this driver.
 */
bool Cx000_uSpiBusy();

#ifdef __cplusplus
}

/**
 * Configuration options for analog FM mode.
 * Each option is tied to a particular bit of the Configuration register 0x34.
 */
enum class FmConfig : uint8_t
{
    BPF_EN     = 1 << 7,    ///< Enable band-pass filter.
    COMP_EN    = 1 << 6,    ///< Enable compression.
    PREEMPH_EN = 1 << 5,    ///< Enable preemphasis.
    BW_25kHz   = 1 << 4,    ///< 25kHz TX bandwidth.
    BW_12p5kHz = 0          ///< 12.5kHz TX bandwidth.
};

/**
 * Audio input selection for both DMR and FM operation.
 */
enum class TxAudioSource
{
    MIC,        ///< Audio source is microphone.
    LINE_IN     ///< Audio source is "line in", e.g. tone generator.
};

/**
 * Generic driver for HR_C5000/HR_C6000 "baseband" chip.
 *
 *
 * WARNING: on some MDx devices the PLL and DMR chips share the SPI MOSI line,
 * thus particular care has to be put to avoid them stomping reciprocally.
 * This driver does not make any check if a SPI transfer is already in progress,
 * deferring the correct bus management to higher level modules. However,
 * a function returning true if the bus is currently in use by this driver is
 * provided.
 */

class ScopedChipSelect;

template< class M >
class HR_Cx000
{
public:

    /**
     * \return a reference to the instance of the AT1846S class (singleton).
     */
    static HR_Cx000& instance()
    {
        static HR_Cx000< M > Cx000;
        return Cx000;
    }

    /**
     * Destructor.
     * When called it shuts down the baseband chip.
     */
    ~HR_Cx000()
    {
        terminate();
    }

    /**
     * Initialise the baseband chip.
     */
    void init();

    /**
     * Shutdown the baseband chip.
     */
    void terminate();

    /**
     * Set value for two-point modulation offset adjustment. This value usually
     * is stored in radio calibration data.
     *
     * @param offset: value for modulation offset adjustment.
     */
    void setModOffset(const uint16_t offset);

    /**
     * Set values for two-point modulation amplitude adjustment. These values
     * usually are stored in radio calibration data.
     *
     * @param iAmp: value for modulation offset adjustment.
     * @param qAmp: value for modulation offset adjustment.
     */
    inline void setModAmplitude(const uint8_t iAmp, const uint8_t qAmp)
    {
        writeReg(M::CONFIG, 0x45, iAmp);    // Mod2 magnitude
        writeReg(M::CONFIG, 0x46, qAmp);    // Mod1 magnitude
    }

    /**
     * Set value for FM-mode modulation factor, a value dependent on bandwidth.
     *
     * @param mf: value for FM modulation factor.
     */
    inline void setModFactor(const uint8_t mf)
    {
        writeReg(M::CONFIG, 0x35, mf);      // FM modulation factor
        writeReg(M::CONFIG, 0x3F, 0x04);    // FM Limiting modulation factor (HR_C6000)
    }

    /**
     * Set the gain of the audio DAC stage. This value affects the sound volume
     * in RX mode.
     *
     * @param value: gain value. Allowed range is 1-31.
     */
    void setDacGain(uint8_t value);

    /**
     * Set the gain for the incoming audio from both the microphone and line-in
     * sources. This value affects the modulation level in analog FM mode and
     * it must be correctly tuned to avoid distorsions.
     *
     * HR_C5000 gain ranges from -34.5dB to +12dB
     * HR_C6000 gain ranges from -12dB to +36dB
     *
     * @param value: gain value in dB.
     */
    void setInputGain(int8_t value);

    /**
     * Configure chipset for DMR operation.
     */
    void dmrMode();

    /**
     * Configure chipset for analog FM operation.
     */
    void fmMode();

    /**
     * Start analog FM transmission.
     *
     * @param source: audio source for TX.
     * @param cfg: TX configuration parameters, e.g. bandwidth.
     */
    void startAnalogTx(const TxAudioSource source, const FmConfig cfg);

    /**
     * Stop analog FM transmission.
     */
    void stopAnalogTx();

    /**
     * Set the value of a configuration register.
     *
     * @param reg: register number.
     * @param value: new register value.
     */
    inline void writeCfgRegister(const uint8_t reg, const uint8_t value)
    {
        writeReg(M::CONFIG, reg, value);
    }

    /**
     * Get the current value of a configuration register.
     *
     * @param reg: register number.
     * \return current value of the register.
     */
    inline uint8_t readCfgRegister(const uint8_t reg)
    {
        return readReg(M::CONFIG, reg);
    }

private:

    /**
     * Constructor.
     */
    HR_Cx000()
    {
        // Being a singleton class, uSPI is initialised only once.
        uSpi_init();
    }

    /**
     * Helper function for register writing.
     *
     * @param opMode: "operating mode" specifier, see datasheet for details.
     * @param addr: register number.
     * @param value: value to be written.
     */
    void writeReg(const M opMode, const uint8_t addr, const uint8_t value)
    {
        ScopedChipSelect cs;
        (void) uSpi_sendRecv(static_cast< uint8_t >(opMode));
        (void) uSpi_sendRecv(addr);
        (void) uSpi_sendRecv(value);
    }

    /**
     * Helper function for register reading.
     *
     * @param opMode: "operating mode" specifier, see datasheet for details.
     * @param addr: register number.
     * @return current value of the addressed register.
     */
    uint8_t readReg(const M opMode, const uint8_t addr)
    {
        ScopedChipSelect cs;
        (void) uSpi_sendRecv(static_cast< uint8_t >(opMode) | 0x80);
        (void) uSpi_sendRecv(addr);
        return uSpi_sendRecv(0x00);
    }

    /**
     * Send a configuration sequence to the chipset. Configuration sequences are
     * blocks of data sent contiguously.
     *
     * @param seq: pointer to the configuration sequence to be sent.
     * @param len: length of the configuration sequence.
     */
    void sendSequence(const uint8_t *seq, const size_t len)
    {
        ScopedChipSelect cs;
        for(size_t i = 0; i < len; i++)
        {
            (void) uSpi_sendRecv(seq[i]);
        }
    }

    /**
     * Initialise the low-level driver which manages "user" SPI interface, that
     * is the one used to configure the chipset functionalities.
     */
    void uSpi_init();

    /**
     * Transfer one byte across the "user" SPI interface.
     *
     * @param value: value to be sent.
     * @return incoming byte from the baseband chip.
     */
    uint8_t uSpi_sendRecv(const uint8_t value);
};

/**
 * \internal
 * Specialisation of logical OR operator to allow composition of FmConfig fields.
 * This allows to have code like: "FmConfig::BPF_EN | FmConfig::WB_MODE"
 */
FmConfig operator |(FmConfig lhs, FmConfig rhs);

/**
 * \internal
 * RAII class for chip select management.
 */
class ScopedChipSelect
{
public:

    /**
     * Constructor.
     * When called it brings the  HR_C5000/HR_C6000 chip select to logical low,
     * selecting it.
     */
    ScopedChipSelect();

    /**
     * Destructor.
     * When called it brings the  HR_C5000/HR_C6000 chip select to logical high,
     * deselecting it.
     */
    ~ScopedChipSelect();
};

#endif // __cplusplus

#endif // HRCx000_H
