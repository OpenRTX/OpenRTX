/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Bring-up GPIO driver for the HR_C7000 CSKY V2 SoC.  Today this is a
 * minimal GPIOC-only port (the bank that carries the W25Q SPI flash
 * and LCD bus); GPIOA/B will join as drivers need them.
 */

#ifndef GPIO_CSKY_H
#define GPIO_CSKY_H

#include "peripherals/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GPIO bank gpioDevs.  Pin numbers are raw bit positions (0..31) within
 * the port's DR/DDR/DIN registers -- no bank-encoding indirection.
 *
 * GPIOA (0x14020000):
 *   pin = 18  -> W25Q SPI CS    (PTA18 = SPI0 CSN0)
 *   pin = 20  -> W25Q SPI SCK   (PTA20, driven by spi_hd2 directly)
 *   pin = 21  -> W25Q SPI MOSI  (PTA21, driven by spi_hd2 directly)
 *   pin = 22  -> W25Q SPI MISO  (PTA22, sampled by spi_hd2 directly)
 *
 * GPIOC (0x14110000):
 *   pins 2..14 -> LCD parallel bus (driven by ST7735S_HD2 directly)
 */
extern const struct gpioDev hd2_gpioa_dev;
extern const struct gpioDev hd2_gpioc_dev;

#ifdef __cplusplus
}
#endif

#endif /* GPIO_CSKY_H */
