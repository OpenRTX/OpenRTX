/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include "MK22F51212.h"
#include "drivers/GPIO/gpio_mk22.h"

/*
 * MK22 GPIO management is a bit convoluted: instead of having all the registers
 * needed for GPIO configuration in one single peripheral (like ST does), we have
 * GPIO peripheral for input/output direction selection, output control and
 * input reading, while alternate function, pull up/down and other stuff is
 * managed by the PORT peripheral. WHY?!
 * To overcome this while keeping the GPIO driver interface standard, we have to
 * create a function that, given the GPIO register, gives back the corresponding
 * PORT one. Ah, and, of course, this cannot be made using simple pointer
 * addition/subtraction/multiplication...
 */
static PORT_Type *getPortFromGPio(const GPIO_Type *gpio)
{
    PORT_Type *port;

    switch(((unsigned int) gpio))
    {
        case GPIOA_BASE:
            port = PORTA;
            break;

        case GPIOB_BASE:
            port = PORTB;
            break;

        case GPIOC_BASE:
            port = PORTC;
            break;

        case GPIOD_BASE:
            port = PORTD;
            break;

        case GPIOE_BASE:
            port = PORTE;
            break;

        default:
            port = 0;
            break;
    }

    return port;
}

void gpio_setMode(const void *port, const uint8_t pin, const uint16_t mode)
{
    GPIO_Type *g = (GPIO_Type *)(port);
    PORT_Type *p = getPortFromGPio(g);

    switch(mode & 0xFF)
    {
        case INPUT:
            p->PCR[pin] = PORT_PCR_MUX(1);   /* Enable pin in GPIO mode */
            g->PDDR    &= ~(1 << pin);       /* Input mode              */
            break;

        case INPUT_PULL_UP:
            p->PCR[pin] = PORT_PCR_MUX(1)    /* Enable pin in GPIO mode */
                        | PORT_PCR_PS(1)     /* Pull up mode            */
                        | PORT_PCR_PE(1);    /* Pull up/down enable     */
            g->PDDR    &= ~(1 << pin);       /* Input mode              */
            break;

        case INPUT_PULL_DOWN:
            p->PCR[pin] = PORT_PCR_MUX(1)    /* Enable pin in GPIO mode */
                        | PORT_PCR_PE(1);    /* Pull up/down enable     */
            g->PDDR    &= ~(1 << pin);       /* Input mode              */

            break;

        case ANALOG:
            p->PCR[pin] = PORT_PCR_MUX(0);   /* Enable pin in AF0 mode  */
            g->PDDR    &= ~(1 << pin);       /* Input mode              */
            break;

        case OUTPUT:
            p->PCR[pin] = PORT_PCR_MUX(1);   /* Enable pin in GPIO mode  */
            g->PDDR    |= (1 << pin);        /* Output mode              */
            break;

        case OPEN_DRAIN:
            p->PCR[pin] = PORT_PCR_MUX(1)    /* Enable pin in GPIO mode  */
                        | PORT_PCR_ODE(1);   /* Enable open drain mode   */
            g->PDDR    |= (1 << pin);        /* Output mode              */
            break;

        case OPEN_DRAIN_PU:
            p->PCR[pin] = PORT_PCR_MUX(1)    /* Enable pin in GPIO mode  */
                        | PORT_PCR_ODE(1)    /* Enable open drain mode   */
                        | PORT_PCR_PS(1)     /* Pull up mode            */
                        | PORT_PCR_PE(1);    /* Pull up/down enable     */
            g->PDDR    |= (1 << pin);        /* Output mode              */
            break;

        /*
         * case ALTERNATE:
         * NOTE: alternate mode unimplemented here for hardware structure
         * reasons.
         */

        /*
         * ALTERNATE_OD:
         * NOTE: alternate open drain mode unimplemented here for hardware
         * structure reasons.
         */

        default:
            p->PCR[pin] = PORT_PCR_MUX(1);   /* Enable pin in GPIO mode */
            g->PDDR    &= ~(1 << pin);       /* Input mode              */
            break;
    }

    uint8_t af = (mode >> 8) & 0xFF;
    if(af > 1)
    {
        p->PCR[pin] &= ~PORT_PCR_MUX_MASK;   /* Clear old configuration */
        p->PCR[pin] |= PORT_PCR_MUX(af);     /* Set new AF, range 0 - 7 */
    }
}

void gpio_setOutputSpeed(const void *port, uint8_t pin, enum Speed spd)
{
    GPIO_Type *g = (GPIO_Type *)(port);
    PORT_Type *p = getPortFromGPio(g);

    if(spd == FAST)
    {
        p->PCR[pin] |= PORT_PCR_SRE(1);
    }
    else
    {
        p->PCR[pin] &= ~PORT_PCR_SRE(1);
    }
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
