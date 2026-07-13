/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Active HD2 bring-up; will fill in as drivers come online.
 *
 * Definitions of the const driver structs declared in hwconfig.h.
 * As drivers come online (LCD, AT1846S, HR_C7000) their const config
 * blobs land here.
 */

#include "hwconfig.h"
#include "drivers/SPI/spi_hd2.h"
#include "drivers/gpio_csky.h"
#include "drivers/NVM/W25Qx.h"

/* External SPI-NOR flash (Winbond W25Q512, 4-byte address mode).
 * CS sits on GPIOA bit EXT_FLASH_CS_BIT (=18) and is driven via the
 * portable gpioPin vtable so W25Qx.c stays target-agnostic.  The SPI
 * bus itself is bitbanged inside drivers/SPI/spi_hd2.c on GPIOA bits
 * 20 (SCK) / 21 (MOSI) / 22 (MISO).  Confirmed via vendor V2.1.3
 * disasm of flash_probe_jedec_id @ 0x03057790. */
static const struct W25QxCfg eflashCfg =
{
    .spi = &nvm_spi,
    .cs  = { .port = &hd2_gpioa_dev, .pin = EXT_FLASH_CS_BIT },
};

W25Qx_DEVICE_DEFINE(eflash, eflashCfg)
