/***************************************************************************
 *   Copyright (C) 2023 by Silvano Seva IU2KWO                             *
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

#include <gd32f3x0.h>
#include <peripherals/gpio.h>
#include <gpio-native.h>

void gpio_af_set(uint32_t gpio_periph, uint32_t alt_func_num, uint32_t pin)
{
    uint16_t i;
    uint32_t afrl, afrh;

    afrl = GPIO_AFSEL0(gpio_periph);
    afrh = GPIO_AFSEL1(gpio_periph);

    for(i = 0U; i < 8U; i++) {
        if((1U << i) & pin) {
            /* clear the specified pin alternate function bits */
            afrl &= ~GPIO_AFR_MASK(i);
            afrl |= GPIO_AFR_SET(i, alt_func_num);
        }
    }

    for(i = 8U; i < 16U; i++) {
        if((1U << i) & pin) {
            /* clear the specified pin alternate function bits */
            afrh &= ~GPIO_AFR_MASK(i - 8U);
            afrh |= GPIO_AFR_SET(i - 8U, alt_func_num);
        }
    }

    GPIO_AFSEL0(gpio_periph) = afrl;
    GPIO_AFSEL1(gpio_periph) = afrh;
}

void gpio_output_options_set(uint32_t gpio_periph, uint8_t otype, uint32_t speed, uint32_t pin)
{
    uint16_t i;
    uint32_t ospeed0, ospeed1;

    if(GPIO_OTYPE_OD == otype) {
        GPIO_OMODE(gpio_periph) |= (uint32_t)pin;
    } else {
        GPIO_OMODE(gpio_periph) &= (uint32_t)(~pin);
    }

    /* get the specified pin output speed bits value */
    ospeed0 = GPIO_OSPD0(gpio_periph);

    if(GPIO_OSPEED_MAX == speed) {
        ospeed1 = GPIO_OSPD1(gpio_periph);

        for(i = 0U; i < 16U; i++) {
            if((1U << i) & pin) {
                /* enable very high output speed function of the pin when the corresponding OSPDy(y=0..15)
                   is "11" (output max speed 50MHz) */
                ospeed0 |= GPIO_OSPEED_SET(i, 0x03);
                ospeed1 |= (1U << i);
            }
        }
        GPIO_OSPD0(gpio_periph) = ospeed0;
        GPIO_OSPD1(gpio_periph) = ospeed1;
    } else {
        for(i = 0U; i < 16U; i++) {
            if((1U << i) & pin) {
                /* clear the specified pin output speed bits */
                ospeed0 &= ~GPIO_OSPEED_MASK(i);
                /* set the specified pin output speed bits */
                ospeed0 |= GPIO_OSPEED_SET(i, speed);
            }
        }
        GPIO_OSPD0(gpio_periph) = ospeed0;
    }
}

void gpio_mode_set(uint32_t gpio_periph, uint32_t mode, uint32_t pull_up_down, uint32_t pin)
{
    uint16_t i;
    uint32_t ctl, pupd;

    ctl = GPIO_CTL(gpio_periph);
    pupd = GPIO_PUD(gpio_periph);

    for(i = 0U; i < 16U; i++) {
        if((1U << i) & pin) {
            /* clear the specified pin mode bits */
            ctl &= ~GPIO_MODE_MASK(i);
            /* set the specified pin mode bits */
            ctl |= GPIO_MODE_SET(i, mode);

            /* clear the specified pin pupd bits */
            pupd &= ~GPIO_PUPD_MASK(i);
            /* set the specified pin pupd bits */
            pupd |= GPIO_PUPD_SET(i, pull_up_down);
        }
    }

    GPIO_CTL(gpio_periph) = ctl;
    GPIO_PUD(gpio_periph) = pupd;
}

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

void gpio_setAlternateFunction(void *port, uint8_t pin, uint8_t afNum)
{
    gpio_type *p = (gpio_type *)(port);
    afNum &= 0x0F;
    if(pin < 8)
    {
        p->muxl &= ~(0x0F << (pin*4));
        p->muxl |= (afNum << (pin*4));
    }
    else
    {
        pin -= 8;
        p->muxh &= ~(0x0F << (pin*4));
        p->muxh |= (afNum << (pin*4));
    }
}

void gpio_setOutputSpeed(const void *port, const uint8_t pin, const enum Speed spd)
{
    ((gpio_type *)(port))->odrvr &= ~(3 << (pin*2));   // Clear old value
    ((gpio_type *)(port))->odrvr |= spd << (pin*2);    // Set new value
}