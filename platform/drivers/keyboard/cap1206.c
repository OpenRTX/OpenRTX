/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Morgan Diepart ON4MOD                    *
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

#include "cap1206.h"
#include <I2C2.h>
#include <stdint.h>
#include <stddef.h>

void cap1206_init()
{
    // Config 1 register, enable SMB timeout
    smb2_write_byte(CAP1206_ADDR, CAP1206_CONFIG_1, 
                    CAP1206_CONFIG_1_TIMEOUT | CAP1206_CONFIG_1_DIS_DIG_NOISE);
    // Disable touch repetition              
    smb2_write_byte(CAP1206_ADDR, CAP1206_RPT_EN, 0);

    // Reduce the sensitivity from x32 (default) to x8
    smb2_write_byte(CAP1206_ADDR, CAP1206_SENS_CTRL, CAP1206_SENS_CTRL_dSENSE_x8 | CAP1206_SENS_CTRL_BASE_SHIFT_x256);
}

uint8_t cap1206_readkeys()
{
    smb2_lockDeviceBlocking();
    uint8_t byte;
    smb2_read_byte(CAP1206_ADDR, CAP1206_SENSOR_IN_STATUS, &byte);
    smb2_write_byte(CAP1206_ADDR, CAP1206_MAIN_CTRL, 0x00);
    smb2_releaseDevice();
    return byte;
}