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

#include <nvmem.h>
#include <delays.h>
#include "extFlash_MDx.h"
#include "calibInfo_MDx.h"

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

    md3x0Calib_t *calib = ((md3x0Calib_t *) buf);

    (void) extFlash_readSecurityRegister(0x1000, &(calib->vox1), 11);
    (void) extFlash_readSecurityRegister(0x1010, calib->txHighPower, 9);
    (void) extFlash_readSecurityRegister(0x1020, calib->txLowPower, 9);
    (void) extFlash_readSecurityRegister(0x1030, calib->rxSensitivity, 9);
    (void) extFlash_readSecurityRegister(0x1040, calib->openSql9, 9);
    (void) extFlash_readSecurityRegister(0x1050, calib->closeSql9, 9);
    (void) extFlash_readSecurityRegister(0x1060, calib->openSql1, 9);
    (void) extFlash_readSecurityRegister(0x1070, calib->closeSql1, 9);
    (void) extFlash_readSecurityRegister(0x1080, calib->maxVolume, 9);
    (void) extFlash_readSecurityRegister(0x1090, calib->ctcss67Hz, 9);
    (void) extFlash_readSecurityRegister(0x10a0, calib->ctcss151Hz, 9);
    (void) extFlash_readSecurityRegister(0x10b0, calib->ctcss254Hz, 9);
    (void) extFlash_readSecurityRegister(0x10c0, calib->dcsMod2, 9);
    (void) extFlash_readSecurityRegister(0x10d0, calib->dcsMod1, 9);
    (void) extFlash_readSecurityRegister(0x10e0, calib->mod1Partial, 9);
    (void) extFlash_readSecurityRegister(0x10f0, calib->analogVoiceAdjust, 9);

    (void) extFlash_readSecurityRegister(0x2000, calib->lockVoltagePartial, 9);
    (void) extFlash_readSecurityRegister(0x2010, calib->sendIpartial, 9);
    (void) extFlash_readSecurityRegister(0x2020, calib->sendQpartial, 9);
    (void) extFlash_readSecurityRegister(0x2030, calib->sendIrange, 9);
    (void) extFlash_readSecurityRegister(0x2040, calib->sendQrange, 9);
    (void) extFlash_readSecurityRegister(0x2050, calib->rxIpartial, 9);
    (void) extFlash_readSecurityRegister(0x2060, calib->rxQpartial, 9);
    (void) extFlash_readSecurityRegister(0x2070, calib->analogSendIrange, 9);
    (void) extFlash_readSecurityRegister(0x2080, calib->analogSendQrange, 9);

    uint32_t freqs[18];
    (void) extFlash_readSecurityRegister(0x20b0, ((uint8_t *) &freqs), 72);
    extFlash_sleep();

    for(uint8_t i = 0; i < 9; i++)
    {
        calib->rxFreq[i] = ((freq_t) _bcd2bin(freqs[2*i]));
        calib->txFreq[i] = ((freq_t) _bcd2bin(freqs[2*i+1]));
    }
}
