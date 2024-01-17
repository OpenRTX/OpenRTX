/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include "I2C1.h"
#include <pthread.h>
#include <stm32f4xx.h>

#include <interfaces/delays.h>
#include <peripherals/gpio.h>
#include <errno.h>

pthread_mutex_t i2c1_mutex;
static uint32_t timeout_ms = 0;

void i2c1_init(bool slow, uint32_t timeout)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    __DSB();
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    if(slow)
    {
        I2C1->CR2 = 42;         /* No interrupts, APB1 clock is 42MHz   */
        I2C1->CCR = 1050;       /* Freq = 20kHz                        */
        I2C1->TRISE = 63;       /* Max possible rise time */
        I2C1->CR1 = I2C_CR1_PE; /* Enable peripheral */
    }
    else
    {
        I2C1->CR2 = 42;         /* No interrupts, APB1 clock is 42MHz   */
        I2C1->CCR = 210;        /* Freq = 100kHz                        */
        I2C1->TRISE = 43;       /* Conforms to max rise time of 1000 ns */
        I2C1->CR1 = I2C_CR1_PE; /* Enable peripheral */
    }
    timeout_ms = timeout;
    
    pthread_mutex_init(&i2c1_mutex, NULL);
}

void i2c1_terminate()
{
    i2c1_lockDeviceBlocking();

    I2C1->CR1 &= ~I2C_CR1_PE;
    RCC->APB1ENR &= ~RCC_APB1ENR_I2C1EN;
    __DSB();

    pthread_mutex_destroy(&i2c1_mutex);
}

bool i2c1_lockDevice()
{
    if(pthread_mutex_trylock(&i2c1_mutex) == 0)
        return true;

    return false;
}

void i2c1_lockDeviceBlocking()
{
    pthread_mutex_lock(&i2c1_mutex);
}

void i2c1_releaseDevice()
{
    pthread_mutex_unlock(&i2c1_mutex);
}

error_t i2c1_write_bytes(uint8_t addr, uint8_t *bytes, uint8_t length, bool stop)
{
    uint32_t start = getTick();

    if(length == 0 || bytes == NULL)
        return EINVAL;

    // Send start
    I2C1->CR1 |= I2C_CR1_START;
    // Wait for SB bit to be set
    while(!(I2C1->SR1 & I2C_SR1_SB_Msk))
    {
        if(getTick()-start > timeout_ms)
            return ETIMEDOUT;
    }

    // Send address (w)
    I2C1->DR = addr << 1;

    // Wait for ADDR bit to be set then read SR1 and SR2
    while(!(I2C1->SR1 & I2C_SR1_ADDR_Msk))
    {   
        // Check if the address was NACKed
        if(I2C1->SR1 & I2C_SR1_AF_Msk)
        {
            I2C1->CR1 |= I2C_CR1_STOP;
            return ENODEV;
        }
        if(getTick()-start > timeout_ms)
            return ETIMEDOUT;
    }

    // Read SR2 by checking that we are transmitter
    if(!(I2C1->SR2 & I2C_SR2_TRA_Msk))
    {
        I2C1->CR1 |= I2C_CR1_STOP;       
        return EPROTO; // We are not transmitter
    }

    // Send data
    for(size_t i = 0; i < length; i++)
    {
        I2C1->DR = bytes[i];    // Send data
        
        // Wait for data to be sent
        while(!(I2C1->SR1 & I2C_SR1_TXE_Msk))
        {
            if(getTick()-start > timeout_ms)
                return ETIMEDOUT;
        }
    }

    if(stop)
    {
        I2C1->CR1 |= I2C_CR1_STOP;
    }        
    
    return 0;
}

error_t i2c1_read_bytes(uint8_t addr, uint8_t *bytes, uint8_t length, bool stop)
{
    uint32_t start = getTick();
    // Send start
    I2C1->CR1 |= I2C_CR1_START;
    // Wait for SB bit to be set
    while(!(I2C1->SR1 & I2C_SR1_SB_Msk))
    {
        if(getTick()-start > timeout_ms)
            return ETIMEDOUT;
    }
    
    // Send address (w)
    I2C1->DR = (addr << 1) + 1;  

    if(length == 1)
        I2C2->CR1 &= ~I2C_CR1_ACK; // Nack
    else
        I2C2->CR1 |= I2C_CR1_ACK; // Ack

    // Wait for ADDR bit to be set then read SR1 and SR2
    while(!(I2C1->SR1 & I2C_SR1_ADDR_Msk))
    {
        // Check if the address was NACKed
        if(I2C1->SR1 & I2C_SR1_AF_Msk)
        {
            I2C1->CR1 |= I2C_CR1_STOP;
            return ENODEV;
        }
        if(getTick()-start > timeout_ms)
            return ETIMEDOUT;
    }

    // Read SR2 by checking that we are receiver
    if(I2C1->SR2 & I2C_SR2_TRA_Msk)
    {
        I2C1->CR1 |= I2C_CR1_STOP;
        return EPROTO; // We are not receiver
    }

    for(size_t i = 0; i < length; i++)
    {   
        // Wait for data to be available
        while(!(I2C2->SR1 & I2C_SR1_RXNE_Msk))
        {
            if(getTick()-start > timeout_ms)
                return ETIMEDOUT;
        }
        if(i+2 >= length)
            I2C2->CR1 &= ~I2C_CR1_ACK; // Nack
        else
            I2C2->CR1 |= I2C_CR1_ACK; // Ack
        
        bytes[i] = I2C2->DR;
    }

    if(stop)
        I2C1->CR1 |= I2C_CR1_STOP;

    return 0;
}