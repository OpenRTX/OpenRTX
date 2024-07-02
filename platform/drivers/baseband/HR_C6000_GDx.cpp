/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <peripherals/gpio.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include "HR_C6000.h"

static const uint8_t initSeq1[] = { 0x01, 0x04, 0xD5, 0xD7, 0xF7, 0x7F, 0xD7, 0x57 };
static const uint8_t initSeq2[] =
{
    0x01, 0x10, 0x69, 0x69, 0x96, 0x96, 0x96, 0x99, 0x99, 0x99, 0xA5, 0xA5, 0xAA,
    0xAA, 0xCC, 0xCC, 0x00, 0xF0, 0x01, 0xFF, 0x01, 0x0F, 0x00, 0x00, 0x00, 0x00,
    0x10, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t initSeq3[] =
{
    0x01, 0x30, 0x00, 0x00, 0x14, 0x1E, 0x1A, 0xFF, 0x3D, 0x50, 0x07, 0x60, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t initSeq4[] = { 0x01, 0x40, 0x00, 0x03, 0x01, 0x02, 0x05, 0x1E, 0xF0 };
static const uint8_t initSeq5[] = { 0x01, 0x51, 0x00, 0x00, 0xEB, 0x78, 0x67 };
static const uint8_t initSeq6[] =
{
    0x01, 0x60, 0x32, 0xEF, 0x00, 0x31, 0xEF, 0x00, 0x12, 0xEF, 0x00, 0x13, 0xEF,
    0x00, 0x14, 0xEF, 0x00, 0x15, 0xEF, 0x00, 0x16, 0xEF, 0x00, 0x17, 0xEF, 0x00,
    0x18, 0xEF, 0x00, 0x19, 0xEF, 0x00, 0x1A, 0xEF, 0x00, 0x1B, 0xEF, 0x00, 0x1C,
    0xEF, 0x00, 0x1D, 0xEF, 0x00, 0x1E, 0xEF, 0x00, 0x1F, 0xEF, 0x00, 0x20, 0xEF,
    0x00, 0x21, 0xEF, 0x00, 0x22, 0xEF, 0x00, 0x23, 0xEF, 0x00, 0x24, 0xEF, 0x00,
    0x25, 0xEF, 0x00, 0x26, 0xEF, 0x00, 0x27, 0xEF, 0x00, 0x28, 0xEF, 0x00, 0x29,
    0xEF, 0x00, 0x2A, 0xEF, 0x00, 0x2B, 0xEF, 0x00, 0x2C, 0xEF, 0x00, 0x2D, 0xEF,
    0x00, 0x2E, 0xEF, 0x00, 0x2F, 0xEF, 0x00
};

template class HR_Cx000 < C6000_SpiOpModes >;

template< class M >
void HR_Cx000< M >::init()
{
    gpio_setMode(DMR_SLEEP, OUTPUT);
    gpio_setMode(DMR_RESET, OUTPUT);

    gpio_setPin(DMR_RESET);
    gpio_setPin(DMR_SLEEP);

    delayMs(10);
    gpio_clearPin(DMR_SLEEP);           // Exit from sleep pulling down DMR_SLEEP
    delayMs(10);

    writeReg(M::CONFIG, 0x0A, 0x81);    // Clock connected to crystal
    writeReg(M::CONFIG, 0x0B, 0x40);    // Set PLL M Register
    writeReg(M::CONFIG, 0x0C, 0x32);    // Set PLL Dividers
    writeReg(M::CONFIG, 0xB9, 0x05);
    writeReg(M::CONFIG, 0x0A, 0x01);    // Clock connected to PLL, set Clock Source Enable CLKOUT Pin

    sendSequence(initSeq1, sizeof(initSeq1));
    sendSequence(initSeq2, sizeof(initSeq2));
    sendSequence(initSeq3, sizeof(initSeq3));
    sendSequence(initSeq4, sizeof(initSeq4));
    sendSequence(initSeq5, sizeof(initSeq5));
    sendSequence(initSeq6, sizeof(initSeq6));

    writeReg(M::CONFIG, 0x00, 0x00);   // Reset of all internal systems
    writeReg(M::CONFIG, 0x10, 0x6E);   // Set DMR, Tier2, Timeslot Mode, Layer 2, Repeater, Aligned, Slot1
    writeReg(M::CONFIG, 0x11, 0x80);   // Set LocalChanMode to Default Value
    writeReg(M::CONFIG, 0x13, 0x00);   // Zero Cend_Band Timing advance
    writeReg(M::CONFIG, 0x1F, 0x10);   // Set LocalEMB  DMR Colour code in upper 4 bits - defaulted to 1, and is updated elsewhere in the code
    writeReg(M::CONFIG, 0x20, 0x00);   // Set LocalAccessPolicy to Impolite
    writeReg(M::CONFIG, 0x21, 0xA0);   // Set LocalAccessPolicy1 to Polite to Color Code  (unsure why there are two registers for this)
    writeReg(M::CONFIG, 0x22, 0x26);   // Start Vocoder Decode, I2S mode
    writeReg(M::CONFIG, 0x22, 0x86);   // Start Vocoder Encode, I2S mode
    writeReg(M::CONFIG, 0x25, 0x0E);   // Undocumented Register
    writeReg(M::CONFIG, 0x26, 0x7D);   // Undocumented Register
    writeReg(M::CONFIG, 0x27, 0x40);   // Undocumented Register
    writeReg(M::CONFIG, 0x28, 0x7D);   // Undocumented Register
    writeReg(M::CONFIG, 0x29, 0x40);   // Undocumented Register
    writeReg(M::CONFIG, 0x2A, 0x0B);   // Set spi_clk_cnt to default value
    writeReg(M::CONFIG, 0x2B, 0x0B);   // According to Datasheet this is a Read only register For FM Squelch
    writeReg(M::CONFIG, 0x2C, 0x17);   // According to Datasheet this is a Read only register For FM Squelch
    writeReg(M::CONFIG, 0x2D, 0x05);   // Set FM Compression and Decompression points (?)
    writeReg(M::CONFIG, 0x2E, 0x04);   // Set tx_pre_on (DMR Transmission advance) to 400us
    writeReg(M::CONFIG, 0x2F, 0x0B);   // Set I2S Clock Frequency
    writeReg(M::CONFIG, 0x32, 0x02);   // Set LRCK_CNT_H CODEC Operating Frequency to default value
    writeReg(M::CONFIG, 0x33, 0xFF);   // Set LRCK_CNT_L CODEC Operating Frequency to default value
    writeReg(M::CONFIG, 0x34, 0xF0);   // Set FM Filters on and bandwidth to 12.5Khz
    writeReg(M::CONFIG, 0x35, 0x28);   // Set FM Modulation Coefficient
    writeReg(M::CONFIG, 0x3E, 0x28);   // Set FM Modulation Offset
    writeReg(M::CONFIG, 0x3F, 0x10);   // Set FM Modulation Limiter
    writeReg(M::CONFIG, 0x36, 0x00);   // Enable all clocks
    writeReg(M::CONFIG, 0x37, 0x00);   // Set mcu_control_shift to default. (codec under HRC-6000 control)
    writeReg(M::CONFIG, 0x4B, 0x1B);   // Set Data packet types to defaults
    writeReg(M::CONFIG, 0x4C, 0x00);   // Set Data packet types to defaults
    writeReg(M::CONFIG, 0x56, 0x00);   // Undocumented Register
    writeReg(M::CONFIG, 0x5F, 0xC0);   // Enable Sync detection for MS or BS orignated signals
    writeReg(M::CONFIG, 0x81, 0xFF);   // Enable all Interrupts
    writeReg(M::CONFIG, 0xD1, 0xC4);   // According to Datasheet this register is for FM DTMF

    writeReg(M::CONFIG, 0x01, 0x70);   // set 2 point Mod, swap receive I and Q, receive mode IF
    writeReg(M::CONFIG, 0x03, 0x00);   // zero Receive I Offset
    writeReg(M::CONFIG, 0x05, 0x00);   // Zero Receive Q Offset
    writeReg(M::CONFIG, 0x12, 0x15);   // Set rf_pre_on Receive to transmit switching advance
    writeReg(M::CONFIG, 0xA1, 0x80);   // According to Datasheet this register is for FM Modulation Setting (?)
    writeReg(M::CONFIG, 0xC0, 0x0A);   // Set RF Signal Advance to 1ms (10x100us)
    writeReg(M::CONFIG, 0x06, 0x21);   // Use SPI vocoder under MCU control
    writeReg(M::CONFIG, 0x07, 0x0B);   // Set IF Frequency H to default 450KHz
    writeReg(M::CONFIG, 0x08, 0xB8);   // Set IF Frequency M to default 450KHz
    writeReg(M::CONFIG, 0x09, 0x00);   // Set IF Frequency L to default 450KHz
    writeReg(M::CONFIG, 0x0D, 0x10);   // Set Voice Superframe timeout value
    writeReg(M::CONFIG, 0x0E, 0x8E);   // Register Documented as Reserved
    writeReg(M::CONFIG, 0x0F, 0xB8);   // FSK Error Count
    writeReg(M::CONFIG, 0xC2, 0x00);   // Disable Mic Gain AGC
    writeReg(M::CONFIG, 0xE0, 0x8B);   // CODEC under MCU Control, LineOut2 Enabled, Mic_p Enabled, I2S Slave Mode
    writeReg(M::CONFIG, 0xE1, 0x0F);   // Undocumented Register (Probably associated with CODEC)
    writeReg(M::CONFIG, 0xE2, 0x06);   // CODEC  Anti Pop Enabled, DAC Output Enabled
    writeReg(M::CONFIG, 0xE3, 0x52);   // CODEC Default Settings
    writeReg(M::CONFIG, 0xE4, 0x4A);   // CODEC   LineOut Gain 2dB, Mic Stage 1 Gain 0dB, Mic Stage 2 Gain 30dB
    writeReg(M::CONFIG, 0xE5, 0x1A);   // CODEC Default Setting
    writeReg(M::CONFIG, 0x40, 0xC3);   // Enable DMR Tx, DMR Rx, Passive Timing, Normal mode
    writeReg(M::CONFIG, 0x41, 0x40);   // Receive during next timeslot
}

template< class M >
void HR_Cx000< M >::terminate()
{
    gpio_setPin(DMR_SLEEP);
    gpio_setMode(DMR_CS, INPUT);
}

template< class M >
void HR_Cx000< M >::setModOffset(const uint16_t offset)
{
    uint8_t offUpper = (offset >> 8) & 0x03;
    uint8_t offLower = offset & 0xFF;

    writeReg(M::CONFIG, 0x48, offUpper);    // Two-point bias, upper value
    writeReg(M::CONFIG, 0x47, offLower);    // Two-point bias, lower value
}

// Unused functionalities on GDx
template< class M > void HR_Cx000< M >::dmrMode() { }
template< class M > void HR_Cx000< M >::fmMode() { }
template< class M >
void HR_Cx000< M >::startAnalogTx(const TxAudioSource source, const FmConfig cfg)
{
    (void) source;
    (void) cfg;
}
template< class M > void HR_Cx000< M >::stopAnalogTx() { }

/*
 * SPI interface driver
 */
template< class M >
void HR_Cx000< M >::uSpi_init()
{
    gpio_setMode(DMR_CS,   OUTPUT);
    gpio_setMode(DMR_CLK,  OUTPUT | ALTERNATE_FUNC(2));
    gpio_setMode(DMR_MOSI, OUTPUT | ALTERNATE_FUNC(2));
    gpio_setMode(DMR_MISO, INPUT  | ALTERNATE_FUNC(2));

    SIM->SCGC6 |= SIM_SCGC6_SPI0_MASK;

    SPI0->MCR &= ~SPI_MCR_MDIS_MASK;     // Enable the SPI0 module
    SPI0->MCR |= SPI_MCR_MSTR_MASK       // Master mode
              |  SPI_MCR_PCSIS_MASK      // CS high when inactive
              |  SPI_MCR_DIS_RXF_MASK    // Disable RX FIFO
              |  SPI_MCR_DIS_TXF_MASK    // Disable TX FIFO
              |  SPI_MCR_HALT_MASK;      // Stop transfers

    SPI0->CTAR[0] = SPI_CTAR_FMSZ(7)     // 8bit frame size
                  |  SPI_CTAR_CPHA_MASK  // CPHA = 1
                  |  SPI_CTAR_PBR(2)     // CLK prescaler divide by 5
                  |  SPI_CTAR_BR(3)      // CLK scaler divide by 8
                  |  SPI_CTAR_PCSSCK(1)
                  |  SPI_CTAR_PASC(1)
                  |  SPI_CTAR_CSSCK(4)
                  |  SPI_CTAR_ASC(4);
}

template< class M >
uint8_t HR_Cx000< M >::uSpi_sendRecv(const uint8_t value)
{
    SPI0->MCR  &= ~SPI_MCR_HALT_MASK; // Start transfer

    SPI0->MCR |= SPI_MCR_CLR_TXF_MASK | SPI_MCR_CLR_RXF_MASK;

    while((SPI0->SR & SPI_SR_TFFF_MASK) == 0) ;

    SPI0->PUSHR = SPI_PUSHR_EOQ_MASK | value;

    SPI0->SR |= SPI_SR_TFFF_MASK;

    while((SPI0->SR & SPI_SR_RFDF_MASK) == 0) ;
    SPI0->SR |= SPI_SR_RFDF_MASK;

    SPI0->MCR  |= SPI_MCR_HALT_MASK; // Start transfer

    return SPI0->POPR;
}
