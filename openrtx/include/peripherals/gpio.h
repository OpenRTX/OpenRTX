/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "drivers/GPIO/gpio-native.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This file provides the common interface for gpio management. Two interfaces
 * are available:
 *
 * 1) gpioDev interface, designed to be common to all the architectures and
 * devices but, due to this, slow.
 * 2) gpio interface, available only for MCUs, dependent on the underlying
 * architecture and device and faster than gpioDev.
 *
 * MCU gpio interface consists of the following functions, which have to be
 * defined in gpio-native.h:
 *
 * void gpio_setMode(void *port, const uint8_t pin, const uint16_t mode);
 * void gpio_setPin(void *port, const uint8_t pin);
 * void gpio_clearPin(void *port, const uint8_t pin);
 * uint8_t gpio_readPin(const void *port, const uint8_t pin);
 *
 * Usage and parameters are identical to their gpioDev counterparts.
 */

/**
 * gpio functional modes.
 * For more details see the documentation of the gpio peripheral.
 */
enum Mode
{
    INPUT           = 0, ///< Input, floating
    INPUT_PULL_UP   = 1, ///< Input, with pull-up
    INPUT_PULL_DOWN = 2, ///< Input, with pull-down
    ANALOG          = 3, ///< Analog
    OUTPUT          = 4, ///< Output, push pull
    OPEN_DRAIN      = 5, ///< Output, open drain
    OPEN_DRAIN_PU   = 6, ///< Output, open drain with pull-up
    ALTERNATE       = 7, ///< Alternate function
    ALTERNATE_OD    = 8, ///< Alternate function, open drain
    ALTERNATE_OD_PU = 9  ///< Alternate function, open drain with pull-up
};

/**
 * Helper macro to set gpio alternate function number.
 */
#define ALTERNATE_FUNC(x) (x << 8)

struct gpioDev;

/**
 * Gpio device driver API.
 */
struct gpioApi
{
    /**
     * Configure gpio pin functional mode.
     *
     * @param port: device handling the gpio port.
     * @param pin: gpio pin number.
     * @param mode: bit 7:0 set gpio functional mode, bit 15:8 manage gpio alternate
     * function mapping.
     * @return zero on success, a negative error code otherwise.
     */
    int (*mode)(const struct gpioDev *dev, const uint8_t pin, const uint16_t mode);

    /**
     * Set gpio pin to logic high level.
     * NOTE: this function does not guarantee that the operation is performed atomically.
     *
     * @param port: device handling the gpio port.
     * @param pin: gpio pin number.
     */
    void (*set)(const struct gpioDev *dev, const uint8_t pin);

    /**
     * Set gpio pin to logic low level.
     * NOTE: this function does not guarantee that the operation is performed atomically.
     *
     * @param port: device handling the gpio port.
     * @param pin: gpio pin number.
     */
    void (*clear)(const struct gpioDev *dev, const uint8_t pin);

    /**
     * Read gpio pin's logic level.
     *
     * @param port: device handling the gpio port.
     * @param pin: gpio pin number.
     * @return true if pin is at high logic level, 0 if pin is at low logic level.
     */
    bool (*read)(const struct gpioDev *dev, const uint8_t pin);
};

/**
 * Gpio device descriptor.
 */
struct gpioDev
{
    const struct gpioApi *api;     ///< Pointer to device driver API
    const void           *priv;    ///< Pointer to device data
};

/**
 * Gpio pin descriptor, general form.
 */
struct gpioPin
{
    const struct gpioDev *port;    ///< Pointer to the gpio device
    const uint8_t         pin;     ///< Gpio pin number
};

/**
 * Gpio pin descriptor, for native MCU gpios.
 */
struct gpio
{
    const void   *port;    ///< Pointer to gpio port
    const uint8_t pin;     ///< Gpio pin number
};

/**
 * Configure gpio pin functional mode.
 *
 * @param port: device handling the gpio port.
 * @param pin: gpio pin number.
 * @param mode: bit 7:0 set gpio functional mode, bit 15:8 manage gpio alternate
 * function mapping.
 * @return zero on success, a negative error code otherwise.
 */
static inline int gpioDev_setMode(const struct gpioDev *port, const uint8_t pin,
                                  const uint16_t mode)
{
    return port->api->mode(port, pin, mode);
}

/**
 * Set gpio pin to logic high level.
 * NOTE: this function does not guarantee that the operation is performed atomically.
 *
 * @param port: device handling the gpio port.
 * @param pin: gpio pin number.
 */
static inline void gpioDev_set(const struct gpioDev *port, const uint8_t pin)
{
    port->api->set(port, pin);
}

/**
 * Set gpio pin to logic low level.
 * NOTE: this function does not guarantee that the operation is performed atomically.
 *
 * @param port: device handling the gpio port.
 * @param pin: gpio pin number.
 */
static inline void gpioDev_clear(const struct gpioDev *port, const uint8_t pin)
{
    port->api->clear(port, pin);
}

/**
 * Read gpio pin's logic level.
 *
 * @param port: device handling the gpio port.
 * @param pin: gpio pin number.
 * @return true if pin is at high logic level, 0 if pin is at low logic level.
 */
static inline bool gpioDev_read(const struct gpioDev *port, const uint8_t pin)
{
    return port->api->read(port, pin);
}

/**
 * Configure gpio pin functional mode.
 *
 * @param gpio: pointer to GPIO pin descriptor.
 * @param mode: bit 7:0 set gpio functional mode, bit 15:8 manage gpio alternate
 * function mapping.
 * @return zero on success, a negative error code otherwise.
 */
static inline int gpioPin_setMode(const struct gpioPin *gpio, const uint16_t mode)
{
    return gpio->port->api->mode(gpio->port, gpio->pin, mode);
}

/**
 * Set gpio pin to logic high level.
 * NOTE: this function does not guarantee that the operation is performed atomically.
 *
 * @param gpio: pointer to GPIO pin descriptor.
 */
static inline void gpioPin_set(const struct gpioPin *gpio)
{
    gpio->port->api->set(gpio->port, gpio->pin);
}

/**
 * Set gpio pin to logic low level.
 * NOTE: this function does not guarantee that the operation is performed atomically.
 *
 * @param gpio: pointer to GPIO pin descriptor.
 */
static inline void gpioPin_clear(const struct gpioPin *gpio)
{
    gpio->port->api->clear(gpio->port, gpio->pin);
}

/**
 * Read gpio pin's logic level.
 *
 * @param gpio: pointer to GPIO pin descriptor.
 * @return true if pin is at high logic level, 0 if pin is at low logic level.
 */
static inline bool gpioPin_read(const struct gpioPin *gpio)
{
    return gpio->port->api->read(gpio->port, gpio->pin);
}

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
