/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cap1206.h"
#include "cap1206_regs.h"

int cap1206_init(const struct i2cDevice *i2c)
{
    uint8_t data[2];
    int ret;

    i2c_acquire(i2c);

    // Config 1 register, enable SMB timeout
    data[0] = CAP1206_CONFIG_1;
    data[1] = /*CAP1206_CONFIG_1_TIMEOUT |*/ CAP1206_CONFIG_1_DIS_DIG_NOISE;
    ret = i2c_write(i2c, CAP1206_ADDR, data, 2, true);
    if(ret < 0)
    {
        i2c_release(i2c);
        return ret;
    }

    // Disable touch repetition
    data[0] = CAP1206_RPT_EN;
    data[1] = 0;
    ret = i2c_write(i2c, CAP1206_ADDR, data, 2, true);
    if(ret < 0)
    {
        i2c_release(i2c);
        return ret;
    }

    // Reduce the sensitivity from x32 (default) to x8
    data[0] = CAP1206_SENS_CTRL;
    data[1] = CAP1206_SENS_CTRL_dSENSE_x8 | CAP1206_SENS_CTRL_BASE_SHIFT_x256;
    ret = i2c_write(i2c, CAP1206_ADDR, data, 2, true);

    i2c_release(i2c);
    return ret;
}

int cap1206_readkeys(const struct i2cDevice *i2c)
{
    uint8_t cmd[2];
    uint8_t keyStatus;
    int ret;

    i2c_acquire(i2c);

    cmd[0] = CAP1206_SENSOR_IN_STATUS;
    ret = i2c_write(i2c, CAP1206_ADDR, cmd, 1, false);
    if(ret < 0)
    {
        i2c_release(i2c);
        return ret;
    }

    ret = i2c_read(i2c, CAP1206_ADDR, &keyStatus, 1, true);
    if(ret < 0)
    {
        i2c_release(i2c);
        return ret;
    }

    cmd[0] = CAP1206_MAIN_CTRL;
    cmd[1] = 0x00;
    ret = i2c_write(i2c, CAP1206_ADDR, cmd, 2, false);
    i2c_release(i2c);

    if(ret < 0)
        return ret;

    return keyStatus;
}
