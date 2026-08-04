/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HRC6000_H
#define HRC6000_H

#include "core/datatypes.h"
#include "drivers/baseband/HR_Cx000.h"

enum class C6000_SpiOpModes : uint8_t
{
    AUX    = 1,     ///< Auxiliary configuration registers.
    DATA   = 2,     ///< Write TX data register and read RX data register.
    SOUND  = 3,     ///< Voice prompt sample register.
    CONFIG = 4,     ///< Main configuration registers.
    AMBE3K = 5,     ///< AMBE3000 configuration register.
    DATA_R = 6,     ///< Write RX data register and read TX data register.
    AMBE1K = 7      ///< AMBE1000 configuration register.
};

class HR_C6000 : public HR_Cx000 < C6000_SpiOpModes >
{
public:

    /**
     * Constructor.
     *
     * @param uSpi: pointer to SPI device for "user" SPI interface.
     * @param uCs: gpioPin object for "user" SPI chip select.
     */
    HR_C6000(const struct spiDevice *uSpi, const struct gpioPin uCs) :
        HR_Cx000< C6000_SpiOpModes >(uSpi, uCs) { }

    /**
     * Configure CTCSS tone transmission.
     *
     * @param tone: CTCSS tone frequency.
     * @param deviation: CTCSS tone deviation.
     */
    void setTxCtcss(const tone_t tone, const uint8_t deviation);

    /**
     * Configure CTCSS tone detection.
     *
     * @param tone: CTCSS tone frequency.
     */
    void setRxCtcss(const tone_t tone);

    /**
     * Test if RX CTCSS tone has been detected.
     *
     * @return true if RX CTCSS tone has been detected.
     */
    inline bool ctcssDetected()
    {
        uint8_t reg = readCfgRegister(0x93);
        return ((reg & 0x01) != 0) ? true : false;
    }

    /**
     * Disable all tone encode and decode.
     */
    inline void disableTones()
    {
        writeCfgRegister(0xA1, 0x00);     // Disable all tones
    }

    /**
     * Transmit a tone of a given frequency.
     *
     * @param freq: tone frequency in Hz.
     * @param deviation: tone deviation.
     */
    void sendTone(const uint32_t freq, const uint8_t deviation);

    /**
     * Program the HR_C6000 built-in DTMF generator's eight tone slots with the
     * standard DTMF frequencies, which sendDtmf() then selects digits from.
     * sendDtmfKey() keeps the table programmed on its own, so drivers using it
     * have no need to call this.
     */
    void initDtmf();

    /**
     * Transmit a DTMF digit using the HR_C6000 built-in DTMF generator. While
     * active the tone replaces the microphone audio in the FM modulation, so
     * this both keys the DTMF digit on air and mutes the mic; the mic is
     * restored the next time the analog transmitter is (re)started.
     *
     * @param code: DTMF digit code (0-9 = digits, 0xE = '*', 0xF = '#',
     *              0xA-0xD = 'A'-'D').
     * @param deviation: tone deviation.
     */
    void sendDtmf(const uint8_t code, const uint8_t deviation);

    /**
     * Transmit the DTMF digit corresponding to a keypad key. Takes care of the
     * keypad-to-chip digit mapping, of the tone deviation, which depends on
     * transmit band and channel bandwidth, and of programming the tone table.
     *
     * @param key: keypad code, 0-9 for the digits, 10 for '*' and 11 for '#'.
     *             Any other value is ignored.
     * @param txFrequency: transmit frequency, in Hz.
     * @param narrowBand: true when the channel bandwidth is 12.5kHz.
     */
    void sendDtmfKey(const uint8_t key, const freq_t txFrequency,
                     const bool narrowBand);

private:

    /**
     * True when the DTMF tone table holds the standard frequencies. Cleared by
     * anything that borrows one of the tone slots for another purpose, so that
     * the next DTMF digit knows it has to program the table again.
     */
    bool dtmfTableValid = false;

    /**
     * Write a register with 16-bit address.
     *
     * @param opMode: "operating mode" specifier, see datasheet for details.
     * @param addr: register address.
     * @param value: value to be written.
     */
    void writeReg16(const C6000_SpiOpModes opMode, const uint16_t addr, const uint8_t value)
    {
        uint8_t data[4];

        data[0] = SPI_FLAGS_EXTD | static_cast< uint8_t >(opMode);
        data[2] = (addr >> 8) & 0x07;
        data[1] = addr & 0xFF;
        data[3] = value;

        ScopedChipSelect cs(uSpi, uCs);
        spi_send(uSpi, data, 4);
    }

    /**
     * Read a register with 16-bit address.
     *
     * @param opMode: "operating mode" specifier, see datasheet for details.
     * @param addr: register address.
     * @return: value read from the register.
     */
    uint8_t readReg16(const C6000_SpiOpModes opMode, const uint16_t addr)
    {
        uint8_t data[3];
        uint8_t value[1];

        data[0] = SPI_FLAGS_READ | SPI_FLAGS_EXTD
                | static_cast< uint8_t >(opMode);
        data[2] = (addr >> 8) & 0x07;
        data[1] = addr & 0xFF;

        ScopedChipSelect cs(uSpi, uCs);
        spi_send(uSpi, data, sizeof(data));
        spi_receive(uSpi, value, 1);

        return value[0];
    }
};

#endif /* HRC6000_H */
