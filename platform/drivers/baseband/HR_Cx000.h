/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HRCx000_H
#define HRCx000_H

#include "peripherals/gpio.h"
#include "peripherals/spi.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

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

class ScopedChipSelect;

/**
 * Generic driver for HR_C5000/HR_C6000 "baseband" chip.
 */
template< class M >
class HR_Cx000
{
public:

    /**
     * Constructor.
     *
     * @param uSpi: pointer to SPI device for "user" SPI interface.
     * @param uCs: gpioPin object for "user" SPI chip select.
     */
    HR_Cx000(const struct spiDevice *uSpi, const struct gpioPin uCs) : uSpi(uSpi), uCs(uCs)
    {
        // Configure chip select pin
        gpioPin_setMode(&uCs, OUTPUT);
        gpioPin_set(&uCs);
    }

    /**
     * Destructor.
     * When called it shuts down the baseband chip.
     */
    ~HR_Cx000()
    {
        terminate();
        gpioPin_setMode(&uCs, INPUT);
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
        writeReg(M::CONFIG, 0x3F, 0x07);    // FM Limiting modulation factor (HR_C6000)
    }

    /**
     * Set the gain of the audio DAC stage, each step corresponds to an 1.5dB
     * variation.
     *
     * @param gain: gain value. Allowed range is from -31 to +31.
     */
    void setDacGain(int8_t gain);

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

    /**
     * Send audio to the DAC FIFO for playback via the "OpenMusic" mode.
     * This function assumes that audio chunk is composed of 64 bytes.
     *
     * @param audio: pointer to a 64 byte audio chunk.
     */
    inline void sendAudio(const uint8_t *audio)
    {
        uint8_t cmd[2];

        cmd[0] = 0x03;
        cmd[1] = 0x00;

        ScopedChipSelect cs(uSpi, uCs);
        spi_send(uSpi, cmd, 2);
        spi_send(uSpi, audio, 64);
    }

private:

    /**
     * Helper function for register writing.
     *
     * @param opMode: "operating mode" specifier, see datasheet for details.
     * @param addr: register number.
     * @param value: value to be written.
     */
    void writeReg(const M opMode, const uint8_t addr, const uint8_t value)
    {
        uint8_t data[3];

        data[0] = static_cast< uint8_t >(opMode);
        data[1] = addr;
        data[2] = value;

        ScopedChipSelect cs(uSpi, uCs);
        spi_send(uSpi, data, 3);
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
        uint8_t cmd[3];
        uint8_t ret[3];

        cmd[0] = SPI_FLAGS_READ | static_cast< uint8_t >(opMode);
        cmd[1] = addr;
        cmd[2] = 0x00;

        ScopedChipSelect cs(uSpi, uCs);
        spi_transfer(uSpi, cmd, ret, 3);

        return ret[2];
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
        ScopedChipSelect cs(uSpi, uCs);
        spi_send(uSpi, seq, len);
    }

protected:

    enum SpiFlags
    {
        SPI_FLAGS_READ = 0x80,    ///< Do a read transaction
        SPI_FLAGS_EXTD = 0x40,    ///< Use 16-bit addressing (HR_C6000 only)
    };

    const struct spiDevice *uSpi;
    const struct gpioPin uCs;
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
     * When called it acquires exclusive ownership on the "user" SPI bus and
     * brings the  HR_C5000/HR_C6000 chip select to logical low.
     *
     * @param dev: pointer to device interface.
     */
    ScopedChipSelect(const struct spiDevice *spi, const struct gpioPin& cs);

    /**
     * Destructor.
     * When called it brings the  HR_C5000/HR_C6000 chip select to logical high,
     * and releases exclusive ownership on the "user" SPI bus.
     */
    ~ScopedChipSelect();

private:
    const struct spiDevice *spi;
    const struct gpioPin& cs;
};

#endif // HRCx000_H
