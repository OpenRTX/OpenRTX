/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include "drivers/GPIO/gpio_stm32.h"

static inline void setGpioAf(GPIO_TypeDef *port, uint8_t pin, const uint8_t af)
{
    if(pin < 8)
    {
        port->AFR[0] &= ~(0x0F << (pin*4));
        port->AFR[0] |=  (af   << (pin*4));
    }
    else
    {
        pin -= 8;
        port->AFR[1] &= ~(0x0F << (pin*4));
        port->AFR[1] |=  (af   << (pin*4));
    }
}

void gpio_setMode(const void *port, const uint8_t pin, const uint16_t mode)
{
    GPIO_TypeDef *p = (GPIO_TypeDef *)(port);
    uint8_t      af = (mode >> 8) & 0x0F;

    p->MODER  &= ~(3 << (pin*2));
    p->OTYPER &= ~(1 << pin);
    p->PUPDR  &= ~(3 << (pin*2));

    switch(mode & 0xFF)
    {
        case INPUT:
            // (MODE=00 TYPE=0 PUP=00)
            p->MODER  |= 0x00 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case INPUT_PULL_UP:
            // (MODE=00 TYPE=0 PUP=01)
            p->MODER  |= 0x00 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x01 << (pin*2);
            break;

        case INPUT_PULL_DOWN:
            // (MODE=00 TYPE=0 PUP=10)
            p->MODER  |= 0x00 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x02 << (pin*2);
            break;

        case ANALOG:
            // (MODE=11 TYPE=0 PUP=00)
            p->MODER  |= 0x03 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case OUTPUT:
            // (MODE=01 TYPE=0 PUP=00)
            p->MODER  |= 0x01 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case OPEN_DRAIN:
            // (MODE=01 TYPE=1 PUP=00)
            p->MODER  |= 0x01 << (pin*2);
            p->OTYPER |= 0x01 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case OPEN_DRAIN_PU:
            // (MODE=01 TYPE=1 PUP=01)
            p->MODER  |= 0x01 << (pin*2);
            p->OTYPER |= 0x01 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;

        case ALTERNATE:
            // (MODE=10 TYPE=0 PUP=00)
            p->MODER  |= 0x02 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            setGpioAf(p, pin, af);
            break;

        case ALTERNATE_OD:
            // (MODE=10 TYPE=1 PUP=00)
            p->MODER  |= 0x02 << (pin*2);
            p->OTYPER |= 0x01 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            setGpioAf(p, pin, af);
            break;

        case ALTERNATE_OD_PU:
            // (MODE=10 TYPE=1 PUP=01)
            p->MODER  |= 0x02 << (pin*2);
            p->OTYPER |= 0x01 << pin;
            p->PUPDR  |= 0x01 << (pin*2);
            setGpioAf(p, pin, af);
            break;

        default:
            // Default to INPUT mode
            p->MODER  |= 0x00 << (pin*2);
            p->OTYPER |= 0x00 << pin;
            p->PUPDR  |= 0x00 << (pin*2);
            break;
    }
}

void gpio_setOutputSpeed(const void *port, const uint8_t pin, const enum Speed spd)
{
    ((GPIO_TypeDef *)(port))->OSPEEDR &= ~(3 << (pin*2));   // Clear old value
    ((GPIO_TypeDef *)(port))->OSPEEDR |= spd << (pin*2);    // Set new value
}

static int gpioApi_mode(const struct gpioDev *dev, const uint8_t pin,
                        const uint16_t mode)
{
    if(pin > 15)
        return -EINVAL;

    gpio_setMode((void *) dev->priv, pin, mode);
    return 0;
}

static void gpioApi_set(const struct gpioDev *dev, const uint8_t pin)
{
    gpio_setPin((void *) dev->priv, pin);
}

static void gpioApi_clear(const struct gpioDev *dev, const uint8_t pin)
{
    gpio_clearPin((void *) dev->priv, pin);
}

static bool gpioApi_read(const struct gpioDev *dev, const uint8_t pin)
{
    uint8_t val = gpio_readPin(dev->priv, pin);
    return (val != 0) ? true : false;
}

static const struct gpioApi gpioApi =
{
    .mode  = gpioApi_mode,
    .set   = gpioApi_set,
    .clear = gpioApi_clear,
    .read  = gpioApi_read
};

const struct gpioDev GpioA = { .api = &gpioApi, .priv = GPIOA };
const struct gpioDev GpioB = { .api = &gpioApi, .priv = GPIOB };
const struct gpioDev GpioC = { .api = &gpioApi, .priv = GPIOC };
const struct gpioDev GpioD = { .api = &gpioApi, .priv = GPIOD };
const struct gpioDev GpioE = { .api = &gpioApi, .priv = GPIOE };
