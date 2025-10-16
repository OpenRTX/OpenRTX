/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stm32f4xx.h"
#include <pthread.h>
#include <errno.h>
#include "i2c_stm32.h"

int i2c_init(const struct i2cDevice *dev, const uint8_t speed)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *) dev->periph;
    uint32_t apbMask = 0;

    switch((uint32_t) i2c)
    {
        case I2C1_BASE:
            apbMask = RCC_APB1ENR_I2C1EN;
            break;

        case I2C2_BASE:
            apbMask = RCC_APB1ENR_I2C2EN;
            break;

        case I2C3_BASE:
            apbMask = RCC_APB1ENR_I2C3EN;
            break;

        default:
            return -ENODEV;
            break;
    }

    RCC->APB1ENR |= apbMask;
    __DSB();

    RCC->APB1RSTR |=  apbMask;
    RCC->APB1RSTR &= ~apbMask;

    switch(speed)
    {
        case I2C_SPEED_LOW:
            i2c->CR2 = 42;         /* No interrupts, APB1 clock is 42MHz   */
            i2c->CCR = 1050;       /* Freq = 20kHz                        */
            i2c->TRISE = 63;       /* Max possible rise time */
            break;

        case I2C_SPEED_100kHz:
            i2c->CR2 = 42;         /* No interrupts, APB1 clock is 42MHz   */
            i2c->CCR = 210;        /* Freq = 100kHz                        */
            i2c->TRISE = 43;       /* Conforms to max rise time of 1000 ns */
            break;

        default:
            return -EINVAL;
            break;
    }

    i2c->CR1 = I2C_CR1_PE; /* Enable peripheral */

    if(dev->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) dev->mutex, NULL);

    return 0;
}

void i2c_terminate(const struct i2cDevice *dev)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *) dev->periph;

    i2c_acquire(dev);

    i2c->CR1 &= ~I2C_CR1_PE;
    RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;
    __DSB();

    if(dev->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) dev->mutex);
}


static int i2c_readImpl(const struct i2cDevice *dev, const uint8_t addr,
                        void *data, const size_t length, const bool stop)
{
    I2C_TypeDef *i2c = (I2C_TypeDef *) dev->periph;
    uint8_t   *bytes = (uint8_t *) data;

    if(length == 0 || bytes == NULL)
        return EINVAL;

    // Send start
    i2c->CR1 |= I2C_CR1_START;
    while((i2c->SR1 & I2C_SR1_SB_Msk) == 0) ;

    // Send address (read)
    i2c->DR = (addr << 1) | 1;

    if(length == 1)
        i2c->CR1 &= ~I2C_CR1_ACK; // Nack
    else
        i2c->CR1 |= I2C_CR1_ACK;  // Ack

    // Wait for ADDR bit to be set then read SR1 and SR2
    while((i2c->SR1 & I2C_SR1_ADDR_Msk) == 0)
    {
        // Check if the address was NACKed
        if((i2c->SR1 & I2C_SR1_AF_Msk) != 0)
        {
            i2c->CR1 |= I2C_CR1_STOP;
            return ENODEV;
        }
    }

    // Read SR2 by checking that we are receiver
    if((i2c->SR2 & I2C_SR2_TRA_Msk) != 0)
    {
        i2c->CR1 |= I2C_CR1_STOP;
        return EPROTO;
    }

    for(size_t i = 0; i < length; i++)
    {
        // Wait for data to be available
        while((i2c->SR1 & I2C_SR1_RXNE_Msk) == 0) ;

        if((i + 2) >= length)
            i2c->CR1 &= ~I2C_CR1_ACK; // Nack
        else
            i2c->CR1 |= I2C_CR1_ACK;  // Ack

        bytes[i] = i2c->DR;
    }

    if(stop)
        i2c->CR1 |= I2C_CR1_STOP;

    return 0;
}

static int i2c_writeImpl(const struct i2cDevice *dev, const uint8_t addr,
                         const void *data, const size_t length, const bool stop)
{
    I2C_TypeDef   *i2c   = (I2C_TypeDef *) dev->periph;
    const uint8_t *bytes = (const uint8_t *) data;

    if(length == 0 || bytes == NULL)
        return EINVAL;

    // Send start
    i2c->CR1 |= I2C_CR1_START;
    while((i2c->SR1 & I2C_SR1_SB_Msk) == 0) ;

    // Send address (write)
    i2c->DR = addr << 1;

    // Wait for ADDR bit to be set then read SR1 and SR2
    while((i2c->SR1 & I2C_SR1_ADDR_Msk) == 0)
    {
        // Check if the address was NACKed
        if((i2c->SR1 & I2C_SR1_AF_Msk) != 0)
        {
            i2c->CR1 |= I2C_CR1_STOP;
            return ENODEV;
        }
    }

    // Read SR2 by checking that we are transmitter
    if((i2c->SR2 & I2C_SR2_TRA_Msk) == 0)
    {
        i2c->CR1 |= I2C_CR1_STOP;
        return EPROTO; // We are not transmitter
    }

    // Send data
    for(size_t i = 0; i < length; i++)
    {
        i2c->DR = bytes[i];    // Send data
        // Wait for data to be sent
        while((i2c->SR1 & I2C_SR1_TXE_Msk) == 0) ;
    }

    if(stop)
        i2c->CR1 |= I2C_CR1_STOP;

    return 0;
}

const struct i2cApi i2cStm32_driver =
{
    .read  = i2c_readImpl,
    .write = i2c_writeImpl
};
