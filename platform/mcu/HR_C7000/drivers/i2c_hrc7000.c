/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HR_C7000 DesignWare DW_apb_i2c master driver (peripherals/i2c.h).  See the
 * header for the IC_START-trigger quirk and the stop-flag semantics.
 */

#include "drivers/i2c_hrc7000.h"
#include "registers.h"
#include <pthread.h>
#include <stddef.h>

#define I2C_SPIN_LIMIT 0x40000

/* Bounded busy-wait: spin until (*reg & mask) matches `set`.  Used for both the
 * IC_STATUS FIFO/bus bits and the IC_ENABLE_STATUS settle bit. */
static void wait_bits(volatile uint32_t *reg, uint32_t mask, bool set)
{
    for (int spin = I2C_SPIN_LIMIT; spin; --spin) {
        if (((*reg & mask) != 0u) == set)
            return;
    }
}

/* Base controller config (master, 7-bit, fast-mode, restart-enabled).  IC_CON /
 * timing can only change while disabled, so leave the controller OFF here; the
 * first transfer sets IC_TAR and enables it (ensure_target). */
static void configure(I2C_TypeDef *hw)
{
    hw->IC_ENABLE = 0u;
    wait_bits(&hw->IC_ENABLE_STATUS, 1u, false);
    hw->IC_CON = I2C_CON_MASTER_FS;
    hw->IC_FS_SCL_HCNT = I2C_SCL_HCNT;
    hw->IC_FS_SCL_LCNT = I2C_SCL_LCNT;
    hw->IC_INTR_MASK = 0x40u;
    hw->IC_RX_TL = 6u;
    hw->IC_TX_TL = 6u;
}

/* Point the master at `addr`, enabling it if needed.  IC_TAR only changes while
 * disabled, so toggle enable ONLY when the target actually differs -- toggling
 * it around every transfer corrupts the FIFO/command alignment (live-verified).
 * A single-slave bus (e.g. the RTC) thus configures the target exactly once. */
static void ensure_target(I2C_TypeDef *hw, uint8_t addr)
{
    if ((hw->IC_ENABLE_STATUS & 1u) && (hw->IC_TAR == addr))
        return;

    hw->IC_ENABLE = 0u;
    wait_bits(&hw->IC_ENABLE_STATUS, 1u, false);
    hw->IC_TAR = addr;
    hw->IC_ENABLE = 1u;
    wait_bits(&hw->IC_ENABLE_STATUS, 1u, true);
}

/* Post-transfer health check: a clean transfer leaves the TX FIFO empty, the
 * bus idle and no abort latched.  Otherwise clear the abort and reconfigure
 * (the next transfer re-sets IC_TAR and re-enables). */
static void recover(I2C_TypeDef *hw)
{
    if ((hw->IC_STATUS & I2C_STA_TFE) && !(hw->IC_STATUS & I2C_STA_ACTIVITY)
        && (hw->IC_TX_ABRT_SRC == 0u))
        return;
    (void)hw->IC_CLR_TX_ABRT;
    configure(hw);
}

/* Trigger the accumulated transfer and wait for it to drain + the bus to idle. */
static void trigger(I2C_TypeDef *hw)
{
    hw->IC_START = 1u; /* HR_C7000 explicit transfer trigger */
    wait_bits(&hw->IC_STATUS, I2C_STA_TFE, true);
    wait_bits(&hw->IC_STATUS, I2C_STA_ACTIVITY, false);
    hw->IC_START = 0u;
    recover(hw);
}

static int i2c_hrc7000_write(const struct i2cDevice *dev, const uint8_t addr,
                             const void *data, const size_t len,
                             const bool stop)
{
    I2C_TypeDef *hw = (I2C_TypeDef *)dev->periph;
    const uint8_t *bytes = (const uint8_t *)data;

    if ((hw == NULL) || ((len != 0u) && (bytes == NULL)))
        return -1;

    ensure_target(hw, addr);

    for (size_t i = 0; i < len; ++i) {
        wait_bits(&hw->IC_STATUS, I2C_STA_TFNF, true);
        uint32_t cmd = bytes[i];
        if (stop && (i == (len - 1u)))
            cmd |= I2C_CMD_STOP;
        hw->IC_DATA_CMD = cmd;
    }

    /* stop=false -> keep the bytes queued for a following (repeated-START)
     * read/write; the transfer is triggered only when a stop is requested. */
    if (stop)
        trigger(hw);

    return 0;
}

static int i2c_hrc7000_read(const struct i2cDevice *dev, const uint8_t addr,
                            void *data, const size_t len, const bool stop)
{
    I2C_TypeDef *hw = (I2C_TypeDef *)dev->periph;
    uint8_t *bytes = (uint8_t *)data;

    if ((hw == NULL) || (len == 0u) || (bytes == NULL))
        return -1;

    ensure_target(hw, addr);

    /* Queue one read command per byte (flushes any pending write bytes ahead of
     * them as a repeated-START), then trigger and drain the RX FIFO. */
    for (size_t i = 0; i < len; ++i) {
        wait_bits(&hw->IC_STATUS, I2C_STA_TFNF, true);
        uint32_t cmd = I2C_CMD_READ;
        if (stop && (i == (len - 1u)))
            cmd |= I2C_CMD_STOP;
        hw->IC_DATA_CMD = cmd;
    }

    hw->IC_START = 1u;
    for (size_t i = 0; i < len; ++i) {
        wait_bits(&hw->IC_STATUS, I2C_STA_RFNE, true);
        bytes[i] = (uint8_t)hw->IC_DATA_CMD;
    }
    wait_bits(&hw->IC_STATUS, I2C_STA_ACTIVITY, false);
    hw->IC_START = 0u;
    recover(hw);

    return 0;
}

const struct i2cApi i2cHrc7000_driver = {
    .read = i2c_hrc7000_read,
    .write = i2c_hrc7000_write,
};

int i2c_init(const struct i2cDevice *dev, const uint8_t speed)
{
    (void)speed; /* fixed vendor fast-mode timing */
    if ((dev == NULL) || (dev->periph == NULL))
        return -1;

    if (dev->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *)dev->mutex, NULL);

    configure((I2C_TypeDef *)dev->periph);
    return 0;
}

void i2c_terminate(const struct i2cDevice *dev)
{
    if ((dev == NULL) || (dev->periph == NULL))
        return;
    ((I2C_TypeDef *)dev->periph)->IC_ENABLE = 0u;
}
