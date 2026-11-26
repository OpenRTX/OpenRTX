/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SPI_CUSTOM_H
#define SPI_CUSTOM_H

#include "peripherals/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This module allows to create an SPI device with a custom implementation of
 * the bus transfer function. An example usage is to create an SPI device using
 * an high-performance bitbanging routine.
 */

#define to_custom_spi(x) ((const struct spiBusCustom *)x)

/**
 * Custom SPI bus descriptor.
 */
struct spiBusCustom {
    const void *priv;
    const struct spiOps *ops;
    pthread_mutex_t *mutex;
    uint8_t (*spi_func)(const void *priv, uint8_t tx);
};

/**
 *  Instantiate a custom SPI bus.
 *
 * @param name: bus name.
 * @param impl: user-provided implementation of the single byte transfer function.
 * @param args: pointer to user-defined arguments.
 * @param mutx: pointer to mutex, or NULL.
 */
#define CUSTOM_SPI_BUS_DEFINE(name, impl, args, mutx) \
    extern const struct spiOps custom_spi_ops;        \
    const struct spiBusCustom name = {                \
        .priv = args,                                 \
        .ops = &custom_spi_ops,                       \
        .mutex = mutx,                                \
        .spi_func = impl,                             \
    }

#ifdef __cplusplus
}
#endif

#endif /* SPI_CUSTOM_H */
