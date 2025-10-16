/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "drivers/baseband/AT1846S.h"

void AT1846S::init()
{
    i2c_writeReg16(0x30, 0x0001);   // Soft reset
    delayMs(160);

    i2c_writeReg16(0x30, 0x0004);   // Set pdn_reg (power down pin)

    i2c_writeReg16(0x04, 0x0FD0);   // Set clk_mode to 25.6MHz/26MHz
    i2c_writeReg16(0x0A, 0x7C20);   // Set 0x0A to its default value
    i2c_writeReg16(0x13, 0xA100);
    i2c_writeReg16(0x1F, 0x1001);   // Set gpio0 to ctcss_out/css_int/css_cmp
                                    // and gpio6 to sq, sq&ctcss/cdcss when sq_out_set=1
    i2c_writeReg16(0x31, 0x0031);
    i2c_writeReg16(0x33, 0x44A5);
    i2c_writeReg16(0x34, 0x2B89);
    i2c_writeReg16(0x41, 0x4122);   // Set voice_gain_tx (voice digital gain) to 0x22
    i2c_writeReg16(0x42, 0x1052);
    i2c_writeReg16(0x43, 0x0100);
    i2c_writeReg16(0x44, 0x07FF);   // Set gain_tx (voice digital gain after tx ADC downsample) to 0x7
    i2c_writeReg16(0x59, 0x0B90);   // Set c_dev (CTCSS/CDCSS TX FM deviation) to 0x10
                                    // and xmitter_dev (voice/subaudio TX FM deviation) to 0x2E
    i2c_writeReg16(0x47, 0x7F2F);
    i2c_writeReg16(0x4F, 0x2C62);
    i2c_writeReg16(0x53, 0x0094);
    i2c_writeReg16(0x54, 0x2A3C);
    i2c_writeReg16(0x55, 0x0081);
    i2c_writeReg16(0x56, 0x0B02);
    i2c_writeReg16(0x57, 0x1C00);
    i2c_writeReg16(0x58, 0x9CDD);   // Bit 0  = 1: CTCSS LPF bandwidth to 250Hz
                                    // Bit 3  = 1: bypass CTCSS HPF
                                    // Bit 4  = 1: bypass CTCSS LPF
                                    // Bit 5  = 0: enable voice LPF
                                    // Bit 6  = 1: bypass voice HPF
                                    // Bit 7  = 1: bypass pre/de-emphasis
                                    // Bit 11 = 1: bypass VOX HPF
                                    // Bit 12 = 1: bypass VOX LPF
                                    // Bit 13 = 0: normal RSSI LPF bandwidth
    i2c_writeReg16(0x5A, 0x06DB);
    i2c_writeReg16(0x63, 0x16AD);
    i2c_writeReg16(0x67, 0x0628);   // Set DTMF C0 697Hz to ???
    i2c_writeReg16(0x68, 0x05E5);   // Set DTMF C1 770Hz to 13MHz and 26MHz
    i2c_writeReg16(0x69, 0x0555);   // Set DTMF C2 852Hz to ???
    i2c_writeReg16(0x6A, 0x04B8);   // Set DTMF C3 941Hz to ???
    i2c_writeReg16(0x6B, 0x02FE);   // Set DTMF C4 1209Hz to 13MHz and 26MHz
    i2c_writeReg16(0x6C, 0x01DD);   // Set DTMF C5 1336Hz
    i2c_writeReg16(0x6D, 0x00B1);   // Set DTMF C6 1477Hz
    i2c_writeReg16(0x6E, 0x0F82);   // Set DTMF C7 1633Hz
    i2c_writeReg16(0x6F, 0x017A);   // Set DTMF C0 2nd harmonic
    i2c_writeReg16(0x70, 0x004C);   // Set DTMF C1 2nd harmonic
    i2c_writeReg16(0x71, 0x0F1D);   // Set DTMF C2 2nd harmonic
    i2c_writeReg16(0x72, 0x0D91);   // Set DTMF C3 2nd harmonic
    i2c_writeReg16(0x73, 0x0A3E);   // Set DTMF C4 2nd harmonic
    i2c_writeReg16(0x74, 0x090F);   // Set DTMF C5 2nd harmonic
    i2c_writeReg16(0x75, 0x0833);   // Set DTMF C6 2nd harmonic
    i2c_writeReg16(0x76, 0x0806);   // Set DTMF C7 2nd harmonic

    i2c_writeReg16(0x30, 0x40A4);   // Set pdn_pin (power down enable)
                                    // and set rx_on
                                    // and set mute when rxno
                                    // and set xtal_mode to 26MHz/13MHz
    delayMs(160);

    i2c_writeReg16(0x30, 0x40A6);   // Start calibration
    delayMs(160);
    i2c_writeReg16(0x30, 0x4006);   // Stop calibration
    delayMs(160);

    i2c_writeReg16(0x40, 0x0031);
}

void AT1846S::setBandwidth(const AT1846S_BW band)
{
    if(band == AT1846S_BW::_25)
    {
        // 25kHz bandwidth
        i2c_writeReg16(0x15, 0x1F00);
        i2c_writeReg16(0x32, 0x7564);
        i2c_writeReg16(0x3A, 0x04C3);
        i2c_writeReg16(0x3C, 0x1B34);
        i2c_writeReg16(0x3F, 0x29D1);
        i2c_writeReg16(0x48, 0x1F3C);
        i2c_writeReg16(0x60, 0x0F17);
        i2c_writeReg16(0x62, 0x3263);
        i2c_writeReg16(0x65, 0x248A);
        i2c_writeReg16(0x66, 0xFFAE);
        i2c_writeReg16(0x7F, 0x0001);
        i2c_writeReg16(0x06, 0x0024);
        i2c_writeReg16(0x07, 0x0214);
        i2c_writeReg16(0x08, 0x0224);
        i2c_writeReg16(0x09, 0x0314);
        i2c_writeReg16(0x0A, 0x0324);
        i2c_writeReg16(0x0B, 0x0344);
        i2c_writeReg16(0x0C, 0x0384);
        i2c_writeReg16(0x0D, 0x1384);
        i2c_writeReg16(0x0E, 0x1B84);
        i2c_writeReg16(0x0F, 0x3F84);
        i2c_writeReg16(0x12, 0xE0EB);
        i2c_writeReg16(0x7F, 0x0000);
        maskSetRegister(0x30, 0x3000, 0x3000);
    }
    else
    {
        // 12.5kHz bandwidth
        i2c_writeReg16(0x15, 0x1100);
        i2c_writeReg16(0x32, 0x4495);
        i2c_writeReg16(0x3A, 0x00C3);
        i2c_writeReg16(0x3F, 0x29D1);
        i2c_writeReg16(0x3C, 0x1B34);
        i2c_writeReg16(0x48, 0x19B1);
        i2c_writeReg16(0x60, 0x0F17);
        i2c_writeReg16(0x62, 0x1425);
        i2c_writeReg16(0x65, 0x2494);
        i2c_writeReg16(0x66, 0xEB2E);
        i2c_writeReg16(0x7F, 0x0001);
        i2c_writeReg16(0x06, 0x0014);
        i2c_writeReg16(0x07, 0x020C);
        i2c_writeReg16(0x08, 0x0214);
        i2c_writeReg16(0x09, 0x030C);
        i2c_writeReg16(0x0A, 0x0314);
        i2c_writeReg16(0x0B, 0x0324);
        i2c_writeReg16(0x0C, 0x0344);
        i2c_writeReg16(0x0D, 0x1344);
        i2c_writeReg16(0x0E, 0x1B44);
        i2c_writeReg16(0x0F, 0x3F44);
        i2c_writeReg16(0x12, 0xE0EB);
        i2c_writeReg16(0x7F, 0x0000);
        maskSetRegister(0x30, 0x3000, 0x0000);
    }

    reloadConfig();
}

void AT1846S::setOpMode(const AT1846S_OpMode mode)
{
    if(mode == AT1846S_OpMode::DMR)
    {
        //
        // TODO: values copy-pasted from GD77 driver, they seems to work well
        // at least with M17
        //
        i2c_writeReg16(0x3A, 0x00C2);
        i2c_writeReg16(0x33, 0x45F5);
        i2c_writeReg16(0x41, 0x4731);
        i2c_writeReg16(0x42, 0x1036);
        i2c_writeReg16(0x43, 0x00BB);
        i2c_writeReg16(0x58, 0xBCFD);   // Bit 0  = 1: CTCSS LPF bandwidth to 250Hz
                                        // Bit 3  = 1: bypass CTCSS HPF
                                        // Bit 4  = 1: bypass CTCSS LPF
                                        // Bit 5  = 1: bypass voice LPF
                                        // Bit 6  = 1: bypass voice HPF
                                        // Bit 7  = 1: bypass pre/de-emphasis
                                        // Bit 11 = 1: bypass VOX HPF
                                        // Bit 12 = 1: bypass VOX LPF
                                        // Bit 13 = 1: bypass RSSI LPF
        i2c_writeReg16(0x44, 0x06CC);
        i2c_writeReg16(0x40, 0x0031);
    }
    else
    {
        // FM mode
        i2c_writeReg16(0x58, 0x9C05);   // Bit 0  = 1: CTCSS LPF badwidth to 250Hz
                                        // Bit 3  = 0: enable CTCSS HPF
                                        // Bit 4  = 0: enable CTCSS LPF
                                        // Bit 5  = 0: enable voice LPF
                                        // Bit 6  = 0: enable voice HPF
                                        // Bit 7  = 0: enable pre/de-emphasis
                                        // Bit 11 = 1: bypass VOX HPF
                                        // Bit 12 = 1: bypass VOX LPF
                                        // Bit 13 = 0: normal RSSI LPF bandwidth
        i2c_writeReg16(0x40, 0x0030);
    }

    reloadConfig();
}

/*
 * Implementation of AT1846S I2C interface.
 *
 * On MD-UV3x0 the I2C interface towars the AT1846S is not connected to any
 * hardware peripheral and has to be implemented in software by bit-banging.
 */

void _i2c_start();
void _i2c_stop();
void _i2c_write(uint8_t val);
uint8_t _i2c_read(bool ack);

static constexpr uint8_t devAddr = 0x5C;

void AT1846S::i2c_init()
{
    gpio_setMode(I2C_SDA, INPUT);
    gpio_setMode(I2C_SCL, OUTPUT);
    gpio_clearPin(I2C_SCL);
}

void AT1846S::i2c_writeReg16(uint8_t reg, uint16_t value)
{
    /*
     * Beware of endianness!
     * When writing an AT1846S register, bits 15:8 must be sent first, followed
     * by bits 7:0.
     */
    uint8_t valHi = (value >> 8) & 0xFF;
    uint8_t valLo = value & 0xFF;

    _i2c_start();
    _i2c_write(devAddr);
    _i2c_write(reg);
    _i2c_write(valHi);
    _i2c_write(valLo);
    _i2c_stop();
}

uint16_t AT1846S::i2c_readReg16(uint8_t reg)
{
    _i2c_start();
    _i2c_write(devAddr);
    _i2c_write(reg);
    _i2c_start();
    _i2c_write(devAddr | 0x01);
    uint8_t valHi = _i2c_read(true);
    uint8_t valLo = _i2c_read(false);
    _i2c_stop();

    return (valHi << 8) | valLo;
}

/*
 * Software I2C routine
 */

void _i2c_start()
{
    gpio_setMode(I2C_SDA, OUTPUT);

    /*
     * Lines commented to keep SCL high when idle
     *
    gpio_clearPin(I2C_SCL);
    delayUs(2);
    */

    gpio_setPin(I2C_SDA);
    delayUs(2);

    gpio_setPin(I2C_SCL);
    delayUs(2);

    gpio_clearPin(I2C_SDA);
    delayUs(2);

    gpio_clearPin(I2C_SCL);
    delayUs(6);
}

void _i2c_stop()
{
    gpio_setMode(I2C_SDA, OUTPUT);

    gpio_clearPin(I2C_SCL);
    delayUs(2);

    gpio_clearPin(I2C_SDA);
    delayUs(2);

    gpio_setPin(I2C_SCL);
    delayUs(2);

    gpio_setPin(I2C_SDA);
    delayUs(2);

    /*
     * Lines commented to keep SCL high when idle
     *
    gpio_clearPin(I2C_SCL);
    delayUs(2);
    */
}

void _i2c_write(uint8_t val)
{
    gpio_setMode(I2C_SDA, OUTPUT);

    for(uint8_t i = 0; i < 8; i++)
    {
        gpio_clearPin(I2C_SCL);
        delayUs(1);

        if(val & 0x80)
        {
            gpio_setPin(I2C_SDA);
        }
        else
        {
            gpio_clearPin(I2C_SDA);
        }

        val <<= 1;
        delayUs(1);
        gpio_setPin(I2C_SCL);
        delayUs(2);
    }

    /* Ensure SCL is low before releasing SDA */
    gpio_clearPin(I2C_SCL);

    /* Clock cycle for slave ACK/NACK */
    gpio_setMode(I2C_SDA, INPUT_PULL_UP);
    delayUs(2);
    gpio_setPin(I2C_SCL);
    delayUs(2);
    gpio_clearPin(I2C_SCL);
    delayUs(1);

    /* Asserting SDA pin allows to fastly bring the line to idle state */
    gpio_setPin(I2C_SDA);
    gpio_setMode(I2C_SDA, OUTPUT);
    delayUs(6);
}

uint8_t _i2c_read(bool ack)
{
    gpio_setMode(I2C_SDA, INPUT_PULL_UP);
    gpio_clearPin(I2C_SCL);

    uint8_t value = 0;
    for(uint8_t i = 0; i < 8; i++)
    {
        delayUs(2);
        gpio_setPin(I2C_SCL);
        delayUs(2);

        value <<= 1;
        value |= gpio_readPin(I2C_SDA);

        gpio_clearPin(I2C_SCL);
    }

    /*
     * Set ACK/NACK state BEFORE putting SDA gpio to output mode.
     * This avoids spurious spikes which can be interpreted as NACKs
     */
    gpio_clearPin(I2C_SDA);
    gpio_setMode(I2C_SDA, OUTPUT);
    delayUs(2);
    if(!ack) gpio_setPin(I2C_SDA);

    /* Clock cycle for ACK/NACK */
    delayUs(2);
    gpio_setPin(I2C_SCL);
    delayUs(2);
    gpio_clearPin(I2C_SCL);
    delayUs(2);

    return value;
}
