/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "I2C0.h"
#include <pthread.h>
#include "MK22F51212.h"

pthread_mutex_t mutex;

void i2c0_init()
{
    SIM->SCGC4 |= SIM_SCGC4_I2C0(1);

    I2C0->A1   = 0;                 /* Module address when in slave mode */
    I2C0->F    = 0x2C;              /* Divide bus clock by 576           */
    I2C0->C1  |= I2C_C1_IICEN(1);   /* Enable I2C module                 */

    pthread_mutex_init(&mutex, NULL);
}

void i2c0_terminate()
{
    while(I2C0->S & I2C_S_BUSY(1)) ;    /* Wait for bus free */

    I2C0->C1   &= ~I2C_C1_IICEN(1);
    SIM->SCGC4 &= ~SIM_SCGC4_I2C0(1);

    pthread_mutex_destroy(&mutex);
}

void i2c0_write(uint8_t addr, void* buf, size_t len, bool sendStop)
{
    I2C0->C1 |= I2C_C1_TX_MASK      /* Transmit data */
             |  I2C_C1_MST_MASK;    /* Master mode   */
    I2C0->D  = addr & 0xFE;

    for(size_t i = 0; i < len; i++)
    {
        while((I2C0->S & I2C_S_IICIF_MASK) == 0) ;  /* IICIF set on tx completion */
        I2C0->S |= I2C_S_IICIF_MASK;                /* Clear IICIF flag           */
        I2C0->D = ((char *) buf)[i];
    }

    while((I2C0->S & I2C_S_IICIF_MASK) == 0) ;      /* Wait for last byte */
    I2C0->S |= I2C_S_IICIF_MASK;

    if(sendStop)
    {
        I2C0->C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TX_MASK);
        while(I2C0->S & I2C_S_BUSY_MASK) ;
    }
}

void i2c0_read(uint8_t addr, void* buf, size_t len)
{
    /* In case a stop was not sent, send a repeated start instead of a start. */
    if(I2C0->C1 & I2C_C1_MST_MASK)
    {
        I2C0->C1 |= I2C_C1_RSTA_MASK
                 |  I2C_C1_TX_MASK;
    }
    else
    {
        I2C0->C1 |= I2C_C1_TX_MASK
                 |  I2C_C1_MST_MASK;
    }

    I2C0->D = addr | 0x01;

    while((I2C0->S & I2C_S_IICIF_MASK) == 0) ;  /* Wait end of address transfer   */
    I2C0->S |= I2C_S_IICIF_MASK;                /* Clear IICIF flag               */

    I2C0->C1 &= ~I2C_C1_TX_MASK;   /* Configure peripheral for data reception     */
    (void) I2C0->D;                /* Flush RX with a dummy read, also clears TCF */

    for(size_t i = 0; i < len - 1; i++)
    {
        while((I2C0->S & I2C_S_IICIF_MASK) == 0) ;
        I2C0->S |= I2C_S_IICIF_MASK;
        ((char *) buf)[i] = I2C0->D;
    }

    /* Send NACK on last byte read */
    I2C0->C1 |= I2C_C1_TXAK_MASK;

    while((I2C0->S & I2C_S_IICIF_MASK) == 0) ;
    I2C0->S |= I2C_S_IICIF_MASK;

    /* All bytes received, send stop */
    I2C0->C1 &= ~(I2C_C1_MST_MASK | I2C_C1_TXAK_MASK);

    /* Read last byte */
    ((char *) buf)[len - 1] = I2C0->D;

    /* Wait until stop has been sent */
    while(I2C0->S & I2C_S_BUSY_MASK) ;
}

bool i2c0_busy()
{
    return (I2C0->S & I2C_S_BUSY_MASK);
}

bool i2c0_lockDevice()
{
    if(pthread_mutex_trylock(&mutex) == 0)
    {
        return true;
    }

    return false;
}

void i2c0_lockDeviceBlocking()
{
    pthread_mutex_lock(&mutex);
}

void i2c0_releaseDevice()
{
    pthread_mutex_unlock(&mutex);
}
