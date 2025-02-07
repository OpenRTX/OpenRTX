/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Silvano Seva IU2KWO                      *
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

#include <errno.h>
#include "gd32f3x0.h"
#include "gpio_gd32.h"

void gpio_setMode(const void *port, const uint8_t pin, const uint16_t mode)
{
    gpio_type *p = (gpio_type *)(port);
    p->cfgr  &= ~(3 << (pin*2));
    p->omode &= ~(1 << pin);
    p->pull  &= ~(3 << (pin*2));

    switch(mode)
    {
        case INPUT:
            // (CFGR=00 OMODE=0 PULL=00)
            p->cfgr  |= 0x00 << (pin*2);
            p->omode |= 0x00 << pin;
            p->pull  |= 0x00 << (pin*2);
            break;

        case INPUT_PULL_UP:
            // (MODE=00 TYPE=0 PUP=01)
            p->cfgr  |= 0x00 << (pin*2);
            p->omode |= 0x00 << pin;
            p->pull  |= 0x01 << (pin*2);
            break;

        case INPUT_PULL_DOWN:
            // (MODE=00 TYPE=0 PUP=10)
            p->cfgr  |= 0x00 << (pin*2);
            p->omode |= 0x00 << pin;
            p->pull  |= 0x02 << (pin*2);
            break;

        case ANALOG:
            // (MODE=11 TYPE=0 PUP=00)
            p->cfgr  |= 0x03 << (pin*2);
            p->omode |= 0x00 << pin;
            p->pull  |= 0x00 << (pin*2);
            break;

        case OUTPUT:
            // (MODE=01 TYPE=0 PUP=00)
            p->cfgr  |= 0x01 << (pin*2);
            p->omode |= 0x00 << pin;
            p->pull  |= 0x00 << (pin*2);
            break;

        case OPEN_DRAIN:
            // (MODE=01 TYPE=1 PUP=00)
            p->cfgr  |= 0x01 << (pin*2);
            p->omode |= 0x01 << pin;
            p->pull  |= 0x00 << (pin*2);
            break;

        case ALTERNATE:
            // (MODE=10 TYPE=0 PUP=00)
            p->cfgr  |= 0x02 << (pin*2);
            p->omode |= 0x00 << pin;
            p->pull  |= 0x00 << (pin*2);
            break;

        case ALTERNATE_OD:
            // (MODE=10 TYPE=1 PUP=00)
            p->cfgr  |= 0x02 << (pin*2);
            p->omode |= 0x01 << pin;
            p->pull  |= 0x00 << (pin*2);
            break;

        default:
            // Default to INPUT mode
            p->cfgr  |= 0x00 << (pin*2);
            p->omode |= 0x00 << pin;
            p->pull  |= 0x00 << (pin*2);
            break;
    }
}

void gpio_setOutputSpeed(const void *port, const uint8_t pin, const enum Speed spd)
{
    ((gpio_type *)(port))->odrvr &= ~(3 << (pin*2));   // Clear old value
    ((gpio_type *)(port))->odrvr |= spd << (pin*2);    // Set new value
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
const struct gpioDev GpioF = { .api = &gpioApi, .priv = GPIOF };
