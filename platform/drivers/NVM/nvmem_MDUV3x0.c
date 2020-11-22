/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <delays.h>
#include "nvmTypes_MDUV3x0.h"
#include "nvmem.h"
#include "extFlash_MDx.h"

/**
 * \internal Utility function to convert 4 byte BCD values into a 32-bit
 * unsigned integer ones.
 */
uint32_t _bcd2bin(uint32_t bcd)
{
    return ((bcd >> 28) & 0x0F) * 10000000 +
           ((bcd >> 24) & 0x0F) * 1000000 +
           ((bcd >> 20) & 0x0F) * 100000 +
           ((bcd >> 16) & 0x0F) * 10000 +
           ((bcd >> 12) & 0x0F) * 1000 +
           ((bcd >> 8) & 0x0F)  * 100 +
           ((bcd >> 4) & 0x0F)  * 10 +
           (bcd & 0x0F);
}

void nvm_init()
{
    extFlash_init();
}

void nvm_terminate()
{
    extFlash_terminate();
}

void nvm_readCalibData(void *buf)
{
    extFlash_wakeup();
    delayUs(5);

    mduv3x0Calib_t *calib = ((mduv3x0Calib_t *) buf);

    /* Common calibration data */
    (void) extFlash_readSecurityRegister(0x1000, (&calib->vox1), 6);

    /* UHF-band calibration data */
    (void) extFlash_readSecurityRegister(0x1009, (&calib->uhfCal.freqAdjustMid), 1);
    (void) extFlash_readSecurityRegister(0x1010, calib->uhfCal.txHighPower, 9);
    (void) extFlash_readSecurityRegister(0x2090, calib->uhfCal.txMidPower, 9);
    (void) extFlash_readSecurityRegister(0x1020, calib->uhfCal.txLowPower, 9);
    (void) extFlash_readSecurityRegister(0x1030, calib->uhfCal.rxSensitivity, 9);
    (void) extFlash_readSecurityRegister(0x1040, calib->uhfCal.openSql9, 9);
    (void) extFlash_readSecurityRegister(0x1050, calib->uhfCal.closeSql9, 9);
    (void) extFlash_readSecurityRegister(0x1070, calib->uhfCal.closeSql1, 9);
    (void) extFlash_readSecurityRegister(0x1060, calib->uhfCal.openSql1, 9);
    (void) extFlash_readSecurityRegister(0x1090, calib->uhfCal.ctcss67Hz, 9);
    (void) extFlash_readSecurityRegister(0x10a0, calib->uhfCal.ctcss151Hz, 9);
    (void) extFlash_readSecurityRegister(0x10b0, calib->uhfCal.ctcss254Hz, 9);
    (void) extFlash_readSecurityRegister(0x10d0, calib->uhfCal.dcsMod1, 9);
    (void) extFlash_readSecurityRegister(0x2030, calib->uhfCal.sendIrange, 9);
    (void) extFlash_readSecurityRegister(0x2040, calib->uhfCal.sendQrange, 9);
    (void) extFlash_readSecurityRegister(0x2070, calib->uhfCal.analogSendIrange, 9);
    (void) extFlash_readSecurityRegister(0x2080, calib->uhfCal.analogSendQrange, 9);

    uint32_t freqs[18];
    (void) extFlash_readSecurityRegister(0x20b0, ((uint8_t *) &freqs), 72);

    for(uint8_t i = 0; i < 9; i++)
    {
        calib->uhfCal.rxFreq[i] = ((freq_t) _bcd2bin(freqs[2*i]));
        calib->uhfCal.txFreq[i] = ((freq_t) _bcd2bin(freqs[2*i+1]));
    }

    /* VHF-band calibration data */
    (void) extFlash_readSecurityRegister(0x100c, (&calib->vhfCal.freqAdjustMid), 1);
    (void) extFlash_readSecurityRegister(0x1019, calib->vhfCal.txHighPower, 5);
    (void) extFlash_readSecurityRegister(0x2099, calib->vhfCal.txMidPower, 5);
    (void) extFlash_readSecurityRegister(0x1029, calib->vhfCal.txLowPower, 5);
    (void) extFlash_readSecurityRegister(0x1039, calib->vhfCal.rxSensitivity, 5);
    (void) extFlash_readSecurityRegister(0x109b, calib->vhfCal.ctcss67Hz, 5);
    (void) extFlash_readSecurityRegister(0x10ab, calib->vhfCal.ctcss151Hz, 5);
    (void) extFlash_readSecurityRegister(0x10bb, calib->vhfCal.ctcss254Hz, 5);
    (void) extFlash_readSecurityRegister(0x10e0, calib->vhfCal.openSql9, 5);
    (void) extFlash_readSecurityRegister(0x10e5, calib->vhfCal.closeSql9, 5);
    (void) extFlash_readSecurityRegister(0x10ea, calib->vhfCal.closeSql1, 5);
    (void) extFlash_readSecurityRegister(0x10ef, calib->vhfCal.openSql1, 5);
    (void) extFlash_readSecurityRegister(0x10db, calib->vhfCal.dcsMod1, 5);
    (void) extFlash_readSecurityRegister(0x2039, calib->vhfCal.sendIrange, 5);
    (void) extFlash_readSecurityRegister(0x2049, calib->vhfCal.sendQrange, 5);
    (void) extFlash_readSecurityRegister(0x2079, calib->uhfCal.analogSendIrange, 5);
    (void) extFlash_readSecurityRegister(0x2089, calib->vhfCal.analogSendQrange, 5);

    (void) extFlash_readSecurityRegister(0x2000, ((uint8_t *) &freqs), 40);
    extFlash_sleep();

    for(uint8_t i = 0; i < 5; i++)
    {
        calib->vhfCal.rxFreq[i] = ((freq_t) _bcd2bin(freqs[2*i]));
        calib->vhfCal.txFreq[i] = ((freq_t) _bcd2bin(freqs[2*i+1]));
    }
}
