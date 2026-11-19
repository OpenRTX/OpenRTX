/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef HRC6000_H
#define HRC6000_H

#include <array>
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
     * Start sending symbols in the physical layer mode (RadioFoundry,
     * docs/spec/modem-hrc6000.md 10.3, https://github.com/radiofoundry/radiofoundry):
     * both slot buffers hold idle symbols and the chip's slot clock starts,
     * raising the timeslot interrupt every 30 ms. Set the modulation
     * amplitudes first: the symbols use the two-point pair (setModAmplitude()).
     */
    void startSymbolTx();

    /**
     * Give the chip the symbols of one slot, sent in the slot after a
     * timeslot interrupt.
     *
     * @param interrupt: timeslot interrupts since startSymbolTx(), from 1.
     * @param symbols: SYMBOL_SLOT_BYTES bytes of dibits, first symbol in the
     * high bits: +1 is 00, +3 01, -1 10, -3 11.
     */
    void writeSymbolSlot(const uint32_t interrupt, const uint8_t *symbols);

    /**
     * Stop sending symbols and put back the registers startSymbolTx() changed.
     */
    void stopSymbolTx();

    /**
     * Start the FM receive copy (RadioFoundry, docs/spec/modem-hrc6000.md
     * 10.6): the chip's demodulated FM, at 8 kHz, with every receive audio
     * filter off. The system interrupt falls each time FM_COPY_SAMPLES new
     * samples are ready; no other system interrupt is enabled meanwhile.
     */
    void startFmCopy();

    /**
     * Read the FM copy's latest FM_COPY_SAMPLES samples, after a system
     * interrupt.
     *
     * @param samples: destination, FM_COPY_SAMPLES samples.
     */
    void readFmCopy(int16_t *samples);

    /**
     * Stop the FM receive copy and put back the registers startFmCopy()
     * changed.
     */
    void stopFmCopy();

    static constexpr size_t  SYMBOL_SLOT_BYTES = 36;    ///< 144 symbols, 30 ms
    static constexpr uint8_t SYMBOL_IDLE       = 0x77;  ///< +3 -3 +3 -3
    static constexpr size_t  FM_COPY_SAMPLES   = 256;   ///< 32 ms at 8 kHz

private:

    /**
     * Write a block of RAM with 8-bit addressing.
     *
     * @param opMode: register page.
     * @param addr: start address.
     * @param data: bytes to write.
     * @param len: number of bytes.
     */
    void writeRam(const C6000_SpiOpModes opMode, const uint8_t addr,
                  const uint8_t *data, const size_t len)
    {
        const uint8_t cmd[2] = { static_cast< uint8_t >(opMode), addr };

        ScopedChipSelect cs(uSpi, uCs);
        spi_send(uSpi, cmd, sizeof(cmd));
        spi_send(uSpi, data, len);
    }

    /**
     * Read a block of RAM with 16-bit addressing. The first byte of a read
     * comes back corrupt, so the read starts one word early and drops it.
     *
     * @param opMode: register page.
     * @param addr: start address.
     * @param data: destination.
     * @param len: number of bytes.
     */
    void readRam16(const C6000_SpiOpModes opMode, const uint16_t addr,
                   uint8_t *data, const size_t len)
    {
        const uint16_t start  = addr - 2;
        const uint8_t cmd[3] = {
            static_cast< uint8_t >(SPI_FLAGS_READ | SPI_FLAGS_EXTD
                                   | static_cast< uint8_t >(opMode)),
            static_cast< uint8_t >(start & 0xFF),
            static_cast< uint8_t >((start >> 8) & 0x07)
        };
        uint8_t lead[2];

        ScopedChipSelect cs(uSpi, uCs);
        spi_send(uSpi, cmd, sizeof(cmd));
        spi_receive(uSpi, lead, sizeof(lead));
        spi_receive(uSpi, data, len);
    }

    /**
     * Last sample of one of the FM copy's two halves.
     *
     * @param half: 0 or 1.
     * @return the sample.
     */
    int16_t fmCopyLastSample(const uint8_t half);

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

    static constexpr uint16_t FM_COPY_RAM = 0x030;  ///< Two halves, big endian

    std::array< uint8_t, 7 > savedRegs; ///< Registers a mode changed
    int16_t fmCopyLast[2] = { 0, 0 };   ///< Last sample of each copy half
    uint8_t fmCopyHalf    = 1;          ///< Copy half read last
};

#endif /* HRC6000_H */
