/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Morgan Diepart ON4MOD                    *
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

#include "I2C2.h"
#include <pthread.h>
#include <stm32f4xx.h>
#include <stdlib.h>
#include <interfaces/delays.h>
#include <peripherals/gpio.h>
#include <hwconfig.h>
#include <errno.h>
#include <string.h>

#include <hwconfig.h>

#include <stdio.h>

pthread_mutex_t i2c2_mutex;

typedef struct {
    uint8_t addr;
    void (*callback)(void);
} smb2_ara_resp_t;

typedef struct {
    bool smb_mode;
    volatile bool timeout;
    smb2_ara_resp_t ara[SMB2_MAX_ARA_DEVICES];
} i2c2_smb_t;

static volatile i2c2_smb_t i2c2_smb_state = {
    .smb_mode       = false,
    .timeout        = false,
};

void i2c2_init()
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C2EN; // Enable peripheral
    __DSB();
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C2RST;  // Reset peripheral
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C2RST;    

    I2C2->CR2 = 42;         /* No interrupts, APB1 clock is 42MHz   */
    I2C2->CCR = 210;        /* Freq = 100kHz                        */
    I2C2->TRISE = 43;       /* Conforms to max rise time of 1000 ns */
    I2C2->CR1 = I2C_CR1_PE; /* Enable peripheral */
    i2c2_smb_state.smb_mode = false;
    i2c2_smb_state.timeout  = false;
    
    pthread_mutex_init(&i2c2_mutex, NULL);
}

void smb2_init()
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C2EN; // Enable peripheral
    __DSB();
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C2RST;  // Reset peripheral
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C2RST;   
    
    /* Enable ERR interrupt to handle timeouts and alerts (if needed) */
    I2C2->CR2 = I2C_CR2_ITERREN | 42;   /* ERR interrupt, APB1 clock is 42MHz   */
    NVIC_EnableIRQ(I2C2_ER_IRQn);
    NVIC_SetPriority(I2C2_ER_IRQn, 10);

    I2C2->CCR = 210;                /* Freq = 100kHz                        */
    I2C2->TRISE = 43;               /* Conforms to max rise time of 1000 ns */
    I2C2->CR1 = I2C_CR1_SMBUS | I2C_CR1_PE;        /* Enable peripheral */
    i2c2_smb_state.smb_mode = true;
    i2c2_smb_state.timeout = false;
    
    for(size_t i = 0; i < SMB2_MAX_ARA_DEVICES; i++)
    {
        i2c2_smb_state.ara[i].addr = 0;
        i2c2_smb_state.ara[i].callback = NULL;
    }
    pthread_mutex_init(&i2c2_mutex, NULL);
}

error_t smb2_add_alert_response(uint8_t addr, void (*callback)())
{
    for(size_t i = 0; i < SMB2_MAX_ARA_DEVICES; i++)
    {
        if(i2c2_smb_state.ara[i].addr == 0)
        {
            i2c2_smb_state.ara[i].addr = addr;
            i2c2_smb_state.ara[i].callback = callback;
            return 0;
        }
    }
    return ENOMEM;
}

void i2c2_terminate()
{
    i2c2_lockDeviceBlocking();

    I2C2->CR1 &= ~I2C_CR1_PE;
    RCC->APB1ENR &= ~RCC_APB1ENR_I2C2EN;
    __DSB();

    pthread_mutex_destroy(&i2c2_mutex);
}

bool i2c2_lockDevice()
{
    if(pthread_mutex_trylock(&i2c2_mutex) == 0)
        return true;

    return false;
}

void i2c2_lockDeviceBlocking()
{
    pthread_mutex_lock(&i2c2_mutex);
}

void i2c2_releaseDevice()
{
    pthread_mutex_unlock(&i2c2_mutex);
}

error_t i2c2_write_bytes(uint8_t addr, uint8_t *bytes, uint16_t length, bool stop)
{
    if(bytes == NULL)
        return EINVAL;

    // Send start
    I2C2->CR1 |= I2C_CR1_START;
    // Wait for SB bit to be set
    while(!(I2C2->SR1 & I2C_SR1_SB_Msk));

    // Send address (w)
    I2C2->DR = addr << 1;

    // Wait for ADDR bit to be set then read SR1 and SR2
    while(!(I2C2->SR1 & I2C_SR1_ADDR_Msk))
    {
        if(i2c2_smb_state.timeout)
            return ETIMEDOUT;
        else if(I2C2->SR1 & I2C_SR1_AF_Msk)
        {
            I2C2->CR1 |= I2C_CR1_STOP;
            return ENODEV;
        }
    }

    // Read SR2 by checking that we are transmitter
    if(!(I2C2->SR2 & I2C_SR2_TRA_Msk))
    {   
        I2C2->CR1 |= I2C_CR1_STOP;
        return EPROTO; // We are not transmitter
    }

    // Send data
    for(size_t i = 0; i < length; i++)
    {
        I2C2->DR = bytes[i];    // Send data

        // Wait for data to be sent
        while(!(I2C2->SR1 & I2C_SR1_TXE_Msk))
        {
            if(i2c2_smb_state.timeout)
                return ETIMEDOUT;
        }
    }

    if(stop)
    {
        I2C2->CR1 |= I2C_CR1_STOP;
    }
    
    return 0;
}

error_t i2c2_read_bytes(uint8_t addr, uint8_t *bytes, uint16_t length, bool stop)
{
    if(length == 0 && stop == false)
        return EINVAL;

    // Send start
    I2C2->CR1 |= I2C_CR1_START;
    // Wait for SB bit to be set
    while(!(I2C2->SR1 & I2C_SR1_SB_Msk));
    
    // Send address (R)
    I2C2->DR = (addr << 1) + 1;  

    if(length == 0)
        I2C2->CR1 |= I2C_CR1_STOP; // Send stop bit
    else if(length == 1)
        I2C2->CR1 &= ~I2C_CR1_ACK; // Nack
    else
        I2C2->CR1 |= I2C_CR1_ACK; // Ack

    // Wait for ADDR bit to be set then read SR1 and SR2
    while(!(I2C2->SR1 & I2C_SR1_ADDR_Msk))
    {
        if(i2c2_smb_state.timeout)
            return ETIMEDOUT;
        else if(I2C2->SR1 & I2C_SR1_AF_Msk)
        {
            I2C2->CR1 |= I2C_CR1_STOP;
            return ENODEV;
        }

    }

    // Read SR2 by checking that we are receiver
    if( (length != 0) && (I2C2->SR2 & I2C_SR2_TRA_Msk))
    {
        I2C2->CR1 |= I2C_CR1_STOP;
        return EPROTO; // We are not receiver
    }
    else if(length == 0)
    {
        return 0;
    }

    for(size_t i = 0; i < length; i++ )
    {   
        // Wait for data to be available
        while(!(I2C2->SR1 & I2C_SR1_RXNE_Msk))
        {
            if(i2c2_smb_state.timeout)
                return ETIMEDOUT;
        }
        if(i+2 >= length)
            I2C2->CR1 &= ~I2C_CR1_ACK; // Nack
        else
            I2C2->CR1 |= I2C_CR1_ACK; // Ack
        
        bytes[i] = I2C2->DR;
    }

    if(stop)
        I2C2->CR1 |= I2C_CR1_STOP;
    
    return 0;
}

error_t smb2_quick_command(uint8_t addr, bool rw)
{
    if(rw)
        return i2c2_write_bytes(addr, NULL, 0, true);
    else
        return i2c2_read_bytes(addr, NULL, 0, true);
}

error_t smb2_send_byte(uint8_t addr, uint8_t byte)
{
    return i2c2_write_bytes(addr, &byte, 1, true);
}

error_t smb2_receive_byte(uint8_t addr, uint8_t *byte)
{
    return i2c2_read_bytes(addr, byte, 1, true);
}

error_t smb2_write_byte(uint8_t addr, uint8_t command, uint8_t byte)
{
    uint8_t buffer[] = {command, byte};
    return i2c2_write_bytes(addr, buffer, 2, true);
}

error_t smb2_write_word(uint8_t addr, uint8_t command, uint16_t word)
{
    uint8_t buffer[] = {command, (uint8_t)word, (uint8_t)(word>>8)};
    return i2c2_write_bytes(addr, buffer, 3, true);
}

error_t smb2_read_byte(uint8_t addr, uint8_t command, uint8_t *byte)
{
    error_t err = i2c2_write_bytes(addr, &command, 1, false);

    if(err == 0)
        err =  i2c2_read_bytes(addr, byte, 1, true);
    
    return err;
    
}

error_t smb2_read_word(uint8_t addr, uint8_t command, uint16_t *word)
{
    error_t err = i2c2_write_bytes(addr, &command, 1, false);

    if(err == 0)
        err = i2c2_read_bytes(addr, (uint8_t*)word, 2, true); // TODO check byte order
    
    return err;
}

error_t smb2_process_call(uint8_t addr, uint8_t command, uint16_t *word)
{
    if(word == NULL)
        return EINVAL;

    uint8_t buffer[] = {command, (uint8_t)(*word), (uint8_t)((*word)>>8)};

    error_t err = i2c2_write_bytes(addr, buffer, 3, false);

    if(err == 0)
        err = i2c2_read_bytes(addr, (uint8_t *)word, 2, true);

    return err;
}

error_t smb2_block_write(uint8_t addr, uint8_t command, uint8_t *bytes, uint8_t length)
{
    if(length == 0 || bytes == NULL)
        return EINVAL;

    uint8_t *buffer = malloc(length+1);
    if(!buffer)
        return ENOMEM;
    buffer[0] = command;
    memcpy(buffer + 1, bytes, length);

    error_t err = i2c2_write_bytes(addr, buffer, length+1, true);
    free(bytes);
    
    return err;
}

error_t smb2_block_read(uint8_t addr, uint8_t command, uint8_t *bytes, uint8_t *length)
{
    i2c2_write_bytes(addr, &command, 1, false);

    // Send start
    I2C2->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    // Wait for SB bit to be set
    while(!(I2C2->SR1 & I2C_SR1_SB_Msk));
    
    // Send address (R)
    I2C2->DR = (addr << 1) + 1;  

    // Wait for ADDR bit to be set then read SR1 and SR2
    while(!(I2C2->SR1 & I2C_SR1_ADDR_Msk))
    {
        if(i2c2_smb_state.timeout)
            return ETIMEDOUT;
        else if(I2C2->SR1 & I2C_SR1_AF_Msk)
        {
            I2C2->CR1 |= I2C_CR1_STOP;
            return ENODEV;
        }
    }

    // Read SR2 by checking that we are receiver
    if(I2C2->SR2 & I2C_SR2_TRA_Msk)
        return EPROTO; // We are not receiver

    *length = I2C2->DR;
    if(*length == 1)
        I2C2->CR1 &= ~I2C_CR1_ACK;
    
    bytes = malloc(*length);
    
    if(!bytes)
    {
        // Memory allocation error, reset I2C2 then return
        I2C2->CR1 |= I2C_CR1_SWRST;
        I2C2->CR1 &= ~I2C_CR1_SWRST;
        return ENOMEM;
    }
    
    for(size_t i = 0; i < *length; i++ )
    {   
        // Wait for data to be available
        while(!(I2C2->SR1 & I2C_SR1_RXNE_Msk))
        {
            if(i2c2_smb_state.timeout)
            {
                free(bytes);
                return ETIMEDOUT;
            }
        }
        if(i+2 >= *length)
            I2C2->CR1 &= ~I2C_CR1_ACK; // Nack
        else
            I2C2->CR1 |= I2C_CR1_ACK; // Ack
        
        bytes[i] = I2C2->DR;
    }

    I2C2->CR1 |= I2C_CR1_STOP;
    
    return 0;
}

error_t smb2_block_write_block_read_process_call(uint8_t addr, uint8_t command, uint8_t *bytesW, uint8_t *bytesR, uint8_t *length)
{
    if((*length == 0) || (bytesW == NULL))
        return EINVAL;
    
    uint8_t *buffer = malloc(*length+2);
    if(buffer == NULL)
        return ENOMEM;

    buffer[0] = command;
    buffer[1] = *length;
    memcpy(buffer+2, bytesW, *length);
    i2c2_write_bytes(addr, buffer, *length, false);
    free(buffer);
    // Send start
    I2C2->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    // Wait for SB bit to be set
    while(!(I2C2->SR1 & I2C_SR1_SB_Msk));
    
    // Send address (R)
    I2C2->DR = (addr << 1) + 1;  

    // Wait for ADDR bit to be set then read SR1 and SR2
    while(!(I2C2->SR1 & I2C_SR1_ADDR_Msk))
    {
        if(i2c2_smb_state.timeout)
            return ETIMEDOUT;
        else if(I2C2->SR1 & I2C_SR1_AF_Msk)
        {
            I2C2->CR1 |= I2C_CR1_STOP;
            return ENODEV;
        }
    }

    // Read SR2 by checking that we are receiver
    if(I2C2->SR2 & I2C_SR2_TRA_Msk)
        return EPROTO; // We are not receiver
        

    *length = I2C2->DR;
    if(*length == 1)
        I2C2->CR1 &= ~I2C_CR1_ACK;
    
    bytesR = malloc(*length);
    
    if(!bytesR)
    {
        // Memory allocation error, reset I2C2 then return
        I2C2->CR1 |= I2C_CR1_SWRST;
        I2C2->CR1 &= ~I2C_CR1_SWRST;
        return ENOMEM;
    }
    
    for(size_t i = 0; i < *length; i++ )
    {   
        // Wait for data to be available
        while(!(I2C2->SR1 & I2C_SR1_RXNE_Msk))
        {
            if(i2c2_smb_state.timeout)
            {
                free(bytesR);
                return ETIMEDOUT;
            }
        }
        if(i+2 >= *length)
            I2C2->CR1 &= ~I2C_CR1_ACK; // Nack
        else
            I2C2->CR1 |= I2C_CR1_ACK; // Ack
        
        bytesR[i] = I2C2->DR;
    }

    I2C2->CR1 |= I2C_CR1_STOP;
    
    return 0;

}

void I2C2_ER_IRQHandler()
{    
    if(I2C2->SR1 & I2C_SR1_TIMEOUT_Msk)
    {
        if(i2c2_smb_state.smb_mode)
            i2c2_smb_state.timeout = true;
        I2C2->SR1 &= ~I2C_SR1_TIMEOUT; // Clear bit
    }

    // These interrupts are not used by this driver. So we just clear the flags.
    if(I2C2->SR1 & (I2C_SR1_SMBALERT_Msk | I2C_SR1_PECERR_Msk | I2C_SR1_OVR_Msk | I2C_SR1_AF_Msk | I2C_SR1_ARLO_Msk | I2C_SR1_BERR_Msk))
    {
        I2C2->SR1 &= ~(I2C_SR1_SMBALERT | I2C_SR1_PECERR | I2C_SR1_OVR | I2C_SR1_AF | I2C_SR1_ARLO | I2C_SR1_BERR);
    }
}

