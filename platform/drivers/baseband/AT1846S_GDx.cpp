/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
#include "I2C0.h"
#include "drivers/baseband/AT1846S.h"

void AT1846S::init()
{
    i2c_writeReg16(0x30, 0x0001);   // Soft reset
    delayMs(50);

    i2c_writeReg16(0x30, 0x0004);   // Chip enable
    i2c_writeReg16(0x04, 0x0FD0);   // 26MHz crystal frequency
    i2c_writeReg16(0x1F, 0x1000);   // Gpio6 squelch output
    i2c_writeReg16(0x09, 0x03AC);
    i2c_writeReg16(0x24, 0x0001);
    i2c_writeReg16(0x31, 0x0031);
    i2c_writeReg16(0x33, 0x45F5);   // AGC number
    i2c_writeReg16(0x34, 0x2B89);   // RX digital gain
    i2c_writeReg16(0x3F, 0x3263);   // RSSI 3 threshold
    i2c_writeReg16(0x41, 0x470F);   // Tx digital gain
    i2c_writeReg16(0x42, 0x1036);
    i2c_writeReg16(0x43, 0x00BB);
    i2c_writeReg16(0x44, 0x06FF);   // Tx digital gain
    i2c_writeReg16(0x47, 0x7F2F);   // Soft mute
    i2c_writeReg16(0x4E, 0x0082);
    i2c_writeReg16(0x4F, 0x2C62);
    i2c_writeReg16(0x53, 0x0094);
    i2c_writeReg16(0x54, 0x2A3C);
    i2c_writeReg16(0x55, 0x0081);
    i2c_writeReg16(0x56, 0x0B02);
    i2c_writeReg16(0x57, 0x1C00);   // Bypass RSSI low-pass
    i2c_writeReg16(0x5A, 0x4935);   // SQ detection time
    i2c_writeReg16(0x58, 0xBCCD);
    i2c_writeReg16(0x62, 0x3263);   // Modulation detect tresh
    i2c_writeReg16(0x4E, 0x2082);
    i2c_writeReg16(0x63, 0x16AD);
    i2c_writeReg16(0x30, 0x40A4);
    delayMs(50);

    i2c_writeReg16(0x30, 0x40A6);   // Start calibration
    delayMs(100);
    i2c_writeReg16(0x30, 0x4006);   // Stop calibration

    delayMs(100);

    i2c_writeReg16(0x58, 0xBCED);
    i2c_writeReg16(0x0A, 0x7BA0);   // PGA gain
    i2c_writeReg16(0x41, 0x4731);   // Tx digital gain
    i2c_writeReg16(0x44, 0x05FF);   // Tx digital gain
    i2c_writeReg16(0x59, 0x09D2);   // Mixer gain
    i2c_writeReg16(0x44, 0x05CF);   // Tx digital gain
    i2c_writeReg16(0x44, 0x05CC);   // Tx digital gain
    i2c_writeReg16(0x48, 0x1A32);   // Noise 1 threshold
    i2c_writeReg16(0x60, 0x1A32);   // Noise 2 threshold
    i2c_writeReg16(0x3F, 0x29D1);   // RSSI 3 threshold
    i2c_writeReg16(0x0A, 0x7BA0);   // PGA gain
    i2c_writeReg16(0x49, 0x0C96);   // RSSI SQL thresholds
    i2c_writeReg16(0x33, 0x45F5);   // AGC number
    i2c_writeReg16(0x41, 0x470F);   // Tx digital gain
    i2c_writeReg16(0x42, 0x1036);
    i2c_writeReg16(0x43, 0x00BB);
}

void AT1846S::setBandwidth(const AT1846S_BW band)
{
    if(band == AT1846S_BW::_25)
    {
        // 25kHz bandwidth
        i2c_writeReg16(0x15, 0x1F00);   // Tuning bit
        i2c_writeReg16(0x32, 0x7564);   // AGC target power
        i2c_writeReg16(0x3A, 0x44C3);   // Modulation detect sel
        i2c_writeReg16(0x3F, 0x29D2);   // RSSI 3 threshold
        i2c_writeReg16(0x3C, 0x0E1C);   // Peak detect threshold
        i2c_writeReg16(0x48, 0x1E38);   // Noise 1 threshold
        i2c_writeReg16(0x62, 0x3767);   // Modulation detect tresh
        i2c_writeReg16(0x65, 0x248A);
        i2c_writeReg16(0x66, 0xFF2E);   // RSSI comp and AFC range
        i2c_writeReg16(0x7F, 0x0001);   // Switch to page 1
        i2c_writeReg16(0x06, 0x0024);   // AGC gain table
        i2c_writeReg16(0x07, 0x0214);
        i2c_writeReg16(0x08, 0x0224);
        i2c_writeReg16(0x09, 0x0314);
        i2c_writeReg16(0x0A, 0x0324);
        i2c_writeReg16(0x0B, 0x0344);
        i2c_writeReg16(0x0D, 0x1384);
        i2c_writeReg16(0x0E, 0x1B84);
        i2c_writeReg16(0x0F, 0x3F84);
        i2c_writeReg16(0x12, 0xE0EB);
        i2c_writeReg16(0x7F, 0x0000);   // Back to page 0
        maskSetRegister(0x30, 0x3000, 0x3000);
    }
    else
    {
        // 12.5kHz bandwidth
        i2c_writeReg16(0x15, 0x1100);   // Tuning bit
        i2c_writeReg16(0x32, 0x4495);   // AGC target power
        i2c_writeReg16(0x3A, 0x40C3);   // Modulation detect sel
        i2c_writeReg16(0x3F, 0x28D0);   // RSSI 3 threshold
        i2c_writeReg16(0x3C, 0x0F1E);   // Peak detect threshold
        i2c_writeReg16(0x48, 0x1DB6);   // Noise 1 threshold
        i2c_writeReg16(0x62, 0x1425);   // Modulation detect tresh
        i2c_writeReg16(0x65, 0x2494);
        i2c_writeReg16(0x66, 0xEB2E);   // RSSI comp and AFC range
        i2c_writeReg16(0x7F, 0x0001);   // Switch to page 1
        i2c_writeReg16(0x06, 0x0014);   // AGC gain table
        i2c_writeReg16(0x07, 0x020C);
        i2c_writeReg16(0x08, 0x0214);
        i2c_writeReg16(0x09, 0x030C);
        i2c_writeReg16(0x0A, 0x0314);
        i2c_writeReg16(0x0B, 0x0324);
        i2c_writeReg16(0x0C, 0x0344);
        i2c_writeReg16(0x0D, 0x1344);
        i2c_writeReg16(0x0E, 0x1B44);
        i2c_writeReg16(0x0F, 0x3F44);
        i2c_writeReg16(0x12, 0xE0EB);   // Back to page 0
        i2c_writeReg16(0x7F, 0x0000);
        maskSetRegister(0x30, 0x3000, 0x0000);
    }

    reloadConfig();
}

void AT1846S::setOpMode(const AT1846S_OpMode mode)
{
    if(mode == AT1846S_OpMode::DMR)
    {
        // DMR mode
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
        i2c_writeReg16(0x33, 0x44A5);
        i2c_writeReg16(0x41, 0x4431);
        i2c_writeReg16(0x42, 0x10F0);
        i2c_writeReg16(0x43, 0x00A9);
        i2c_writeReg16(0x58, 0xBC05);   // Bit 0  = 1: CTCSS LPF badwidth to 250Hz
                                        // Bit 3  = 0: enable CTCSS HPF
                                        // Bit 4  = 0: enable CTCSS LPF
                                        // Bit 5  = 0: enable voice LPF
                                        // Bit 6  = 0: enable voice HPF
                                        // Bit 7  = 0: enable pre/de-emphasis
                                        // Bit 11 = 1: bypass VOX HPF
                                        // Bit 12 = 1: bypass VOX LPF
                                        // Bit 13 = 1: bypass RSSI LPF
        i2c_writeReg16(0x44, 0x06FF);
        i2c_writeReg16(0x40, 0x0030);

        maskSetRegister(0x57, 0x0001, 0x00);     // Audio feedback off
        maskSetRegister(0x3A, 0x7000, 0x4000);   // Select voice channel
    }

    reloadConfig();
}

/*
 * Implementation of AT1846S I2C interface.
 *
 * NOTE: on GDx devices the I2C bus is shared between the EEPROM and the AT1846S,
 * so we have to acquire exclusive ownership before exchanging data
*/

static constexpr uint8_t devAddr = 0xE2;

void AT1846S::i2c_init()
{
    // I2C already init'd by platform support package
}

void AT1846S::i2c_writeReg16(uint8_t reg, uint16_t value)
{
    /*
     * Beware of endianness!
     * When writing an AT1846S register, bits 15:8 must be sent first, followed
     * by bits 7:0.
     */

    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = value & 0xFF;

    i2c0_lockDeviceBlocking();
    i2c0_write(devAddr, buf, 3, true);
    i2c0_releaseDevice();
}

uint16_t AT1846S::i2c_readReg16(uint8_t reg)
{
    uint16_t value = 0;

    i2c0_lockDeviceBlocking();
    i2c0_write(devAddr, &reg, 1, false);
    i2c0_read(devAddr, &value, 2);
    i2c0_releaseDevice();

    // Correction for AT1846S sending register data in big endian format
    return __builtin_bswap16(value);
}
