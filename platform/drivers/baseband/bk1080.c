/**
 * @file bk1080.c
 * @author Jamiexu (doxm@foxmail.com)
 * @brief 
 * @version 0.1
 * @date 2024-05-24
 * 
 * @copyright MIT License

Copyright (c) 2024 (Jamiexu or Jamie793)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 * 
 */
#include "bk1080.h"

void i2c_start(void)
{
    BK1080_SDA_HIGH;
    bk1080_delay(1);
    BK1080_SCK_HIGH;
    bk1080_delay(1);
    BK1080_SDA_LOW;
    bk1080_delay(1);
    BK1080_SCK_LOW;
    bk1080_delay(1);
}

static void i2c_stop(void)
{
    BK1080_SCK_LOW;
    bk1080_delay(1);
    BK1080_SDA_LOW;
    bk1080_delay(1);
    BK1080_SCK_HIGH;
    bk1080_delay(1);
    BK1080_SDA_HIGH;
}

static void i2c_write_byte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80)
            BK1080_SDA_HIGH;
        else
            BK1080_SDA_LOW;
        bk1080_delay(1);
        BK1080_SCK_HIGH;
        bk1080_delay(1);
        BK1080_SCK_LOW;
        bk1080_delay(1);
        data <<= 1;
    }
}

static uint8_t i2c_read_byte(void)
{
    uint8_t data = 0;
    BK1080_SDA_DIR_IN;
    bk1080_delay(5);
    for (uint8_t i = 0; i < 8; i++)
    {
        data <<= 1;
        BK1080_SCK_HIGH;
        bk1080_delay(1);
        data |= BK1080_SDA_READ;
        BK1080_SCK_LOW;
        bk1080_delay(1);
    }
    BK1080_SDA_DIR_OUT;
    bk1080_delay(5);
    return data;
}

static void i2c_send_ack(iic_ack_t ack)
{
    if (ack)
        BK1080_SDA_HIGH;
    else
        BK1080_SDA_LOW;
    bk1080_delay(1);
    BK1080_SCK_HIGH;
    bk1080_delay(1);
    BK1080_SCK_LOW;
    bk1080_delay(1);
}

static iic_ack_t i2c_get_ack(void)
{
    iic_ack_t ack;
    BK1080_SDA_DIR_IN;
    bk1080_delay(5);
    BK1080_SCK_HIGH;
    bk1080_delay(1);
    if (BK1080_SDA_READ)
        ack = I2C_NACK;
    else
        ack = I2C_ACK;
    BK1080_SCK_LOW;
    bk1080_delay(1);
    BK1080_SDA_DIR_OUT;
    bk1080_delay(1);
    return ack;
}

static void bk1080_delay(uint32_t count)
{
    delayUs(count);
}

void bk1080_write_reg(bk1080_reg_t reg, uint16_t data)
{
    i2c_start();
    i2c_write_byte(BK1080_ADDRESS);
    if (i2c_get_ack() == I2C_ACK)
    {
        i2c_write_byte(reg << 1);
        if (i2c_get_ack() == I2C_ACK)
        {
            i2c_write_byte((data >> 8) & 0xFF);
            if (i2c_get_ack() == I2C_ACK)
                i2c_write_byte(data & 0xFF);
            else
                printf("wACK3 Not in...\r\n");
        }
        else
            printf("wACK2 Not in...\r\n");
    }
    else
    {
        printf("wACK1 Not in...\r\n");
    }
    i2c_stop();
}

uint16_t bk1080_read_reg(bk1080_reg_t reg)
{
    uint16_t data = 0;
    i2c_start();
    i2c_write_byte(BK1080_ADDRESS);
    if (i2c_get_ack() == I2C_ACK)
    {
        i2c_write_byte((reg << 1) | 0x01);
        if (i2c_get_ack() == I2C_ACK)
        {
            data |= (i2c_read_byte() << 8);
            i2c_send_ack(I2C_ACK);
            data |= i2c_read_byte();
            i2c_send_ack(I2C_NACK);
        }
        else
            printf("ACK2 Not in...\r\n");
    }
    else
    {
        printf("ACK1 Not in...\r\n");
    }
    i2c_stop();
    return data;
}
