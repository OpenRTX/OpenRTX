/**
 * @file bk4819.c
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

#include "bk4819.h"
#include "interfaces/delays.h"



static void spi_write_byte(uint8_t data)
{
    BK4819_SCK_LOW;
    BK4819_SDA_DIR_OUT;
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80)
            BK4819_SDA_HIGH;
        else
            BK4819_SDA_LOW;
        // bk4819_delay(1);
        BK4819_SCK_HIGH;
        bk4819_delay(1);
        BK4819_SCK_LOW;
        bk4819_delay(1);
        data <<= 1;
    }
}

static void spi_write_half_word(uint16_t data)
{
    spi_write_byte((data >> 8) & 0xFF);
    spi_write_byte(data & 0xFF);
}

static uint16_t spi_read_half_word(void)
{
    uint16_t data = 0;
    BK4819_SDA_DIR_IN;
    BK4819_SCK_LOW;
    for (uint8_t i = 0; i < 16; i++)
    {
        data <<= 1;
        BK4819_SCK_LOW;
        bk4819_delay(1);
        BK4819_SCK_HIGH;
        bk4819_delay(1);
        data |= BK4819_SDA_READ;
    }
    return data;
}

uint16_t bk4819_read_reg(bk4819_reg_t reg)
{
    uint16_t data;
    BK4819_SCN_LOW;
    bk4819_delay(1);

    spi_write_byte(reg | BK4819_REG_READ);
    data = spi_read_half_word();

    bk4819_delay(1);
    BK4819_SCN_HIGH;
    delay_1ms(10);
    return data;
}

void bk4819_write_reg(bk4819_reg_t reg, uint16_t data)
{
    BK4819_SCN_LOW;
    bk4819_delay(1);

    spi_write_byte(reg | BK4819_REG_WRITE);
    spi_write_half_word(data);

    bk4819_delay(1);
    BK4819_SCN_HIGH;
}

void bk4819_init(void)
{
    uint8_t data;
    bk4819_write_reg(BK4819_REG_00, 0x8000); // reset
    bk4819_delay(1000);
    bk4819_write_reg(BK4819_REG_00, 0x00);

    bk4819_write_reg(BK4819_REG_37, 0x1d0f); //
    bk4819_write_reg(BK4819_REG_13, 0x3be);
    bk4819_write_reg(BK4819_REG_12, 0x37b);
    bk4819_write_reg(BK4819_REG_53, 59000);
    bk4819_write_reg(BK4819_REG_09, 0x603a);
    bk4819_write_reg(BK4819_REG_11, 0x27b);
    bk4819_write_reg(BK4819_REG_10, 0x7a);
    bk4819_write_reg(BK4819_REG_14, 0x19);
    bk4819_write_reg(BK4819_REG_49, 0x2a38);
    bk4819_write_reg(BK4819_REG_7B, 0x8420);
    bk4819_write_reg(BK4819_REG_48, 0xb3ff);
    bk4819_write_reg(BK4819_REG_1E, 0x4c58);
    bk4819_write_reg(BK4819_REG_1F, 0xa656);
    bk4819_write_reg(BK4819_REG_3E, 0xa037);
    bk4819_write_reg(BK4819_REG_3F, 0x7fe);
    bk4819_write_reg(BK4819_REG_2A, 0x7fff);
    bk4819_write_reg(BK4819_REG_28, 0x6b00);
    bk4819_write_reg(BK4819_REG_7D, 0xe952);
    bk4819_write_reg(BK4819_REG_2C, 0x5705);
    bk4819_write_reg(BK4819_REG_4B, 0x7102);
    bk4819_write_reg(BK4819_REG_77, 0x88ef);
    bk4819_write_reg(BK4819_REG_26, 0x13a0);
    bk4819_write_reg(BK4819_REG_4E, 0x6f15);
    bk4819_write_reg(BK4819_REG_4F, 0x3f3e);
    bk4819_write_reg(BK4819_REG_09, 0x6f);
    bk4819_write_reg(BK4819_REG_09, 0x106b);
    bk4819_write_reg(BK4819_REG_09, 0x2067);
    bk4819_write_reg(BK4819_REG_09, 0x3062);
    bk4819_write_reg(BK4819_REG_09, 0x4050);
    bk4819_write_reg(BK4819_REG_09, 0x5047);
    bk4819_write_reg(BK4819_REG_09, 0x702c);
    bk4819_write_reg(BK4819_REG_09, 0x8041);
    bk4819_write_reg(BK4819_REG_09, 0x9037);
    bk4819_write_reg(BK4819_REG_28, 0x6b38);
    bk4819_write_reg(BK4819_REG_09, 0xa025);
    bk4819_write_reg(BK4819_REG_09, 0xb017);
    bk4819_write_reg(BK4819_REG_09, 0xc0e4);
    bk4819_write_reg(BK4819_REG_09, 0xd0cb);
    bk4819_write_reg(BK4819_REG_09, 0xe0b5);
    bk4819_write_reg(BK4819_REG_09, 0xf09f);
    bk4819_write_reg(BK4819_REG_74, 0xfa02);
    bk4819_write_reg(BK4819_REG_44, 0x8f88);
    bk4819_write_reg(BK4819_REG_45, 0x3201);
    bk4819_write_reg(BK4819_REG_29, 0xb4cb);
    bk4819_write_reg(BK4819_REG_40, bk4819_read_reg(BK4819_REG_40) & 0xf000 | 0x4d2);
    bk4819_write_reg(BK4819_REG_31, bk4819_read_reg(BK4819_REG_31) & 0xfffffff7);
    bk4819_set_freq(43949500);
    bk4819_CTDCSS_enable(1);
    bk4819_CTDCSS_set(0, 1485);
    // bk4819_rx_on();
    bk4819_tx_on();
}

static void bk4819_delay(uint32_t count)
{
    delayMs(count);
}

/**
 * @brief Get interrupt
 *
 * @param interrupt Interrupt type
 * @return uint8_t 0:SET 1:RESET
 */
uint8_t bk4819_int_get(bk4819_int_t interrupt)
{
    return bk4819_read_reg(BK4819_REG_02 & interrupt);
}

/**
 * @brief Set frequency
 *
 * @param freq
 */
void bk4819_set_freq(uint32_t freq)
{
    bk4819_write_reg(BK4819_REG_39, (freq >> 16) & 0xFFFF);
    bk4819_write_reg(BK4819_REG_38, freq & 0xFFFF);
}

/**
 * @brief Rx ON
 *
 */
void bk4819_rx_on(void)
{
    bk4819_write_reg(BK4819_REG_30, 0x00); // reset

    bk4819_write_reg(BK4819_REG_30,
                     BK4819_REG30_REVERSE2_ENABLE |
                         BK4819_REG30_REVERSE1_ENABLE |
                         BK4819_REG30_VCO_CALIBRATION |
                         BK4819_REG30_RX_LINK_ENABLE |
                         BK4819_REG30_AF_DAC_ENABLE |
                         BK4819_REG30_PLL_VCO_ENABLE |
                         BK4819_REG30_MIC_ADC_ENABLE |
                         BK4819_REG30_PA_GAIN_ENABLE |
                         BK4819_REG30_RX_DSP_ENABLE);
}

/**
 * @brief Tx ON
 *
 */
void bk4819_tx_on(void)
{
    gpio_bit_reset(MIC_EN_GPIO_PORT, MIC_EN_GPIO_PIN); // open microphone

    bk4819_write_reg(BK4819_REG_30, 0x00); // reset

    bk4819_write_reg(BK4819_REG_30,
                     BK4819_REG30_REVERSE1_ENABLE |
                         BK4819_REG30_REVERSE2_ENABLE |
                         BK4819_REG30_VCO_CALIBRATION |
                         BK4819_REG30_MIC_ADC_ENABLE |
                         BK4819_REG30_TX_DSP_ENABLE |
                         BK4819_REG30_PLL_VCO_ENABLE |
                         BK4819_REG30_PA_GAIN_ENABLE);
}

/**
 * @brief Set CTCSS/CDCSS
 *
 * @param sel 0:CTC1 1:CTC2 2:CSCSS
 * @param frequency frquency control word
 */
void bk4819_CTDCSS_set(uint8_t sel, uint16_t frequency)
{
    bk4819_write_reg(BK4819_REG_07, (sel << 13) | frequency);
}

/**
 * @brief Set squelch threshold
 *
 * @param RTSO RSSI threshold for Squelch=1, 0.5dB/step
 * @param RTSC RSSI threshold for Squelch =0, 0.5dB/step
 * @param ETSO Ex-noise threshold for Squelch =1
 * @param ETSC Ex-noise threshold for Squelch =0
 * @param GTSO Glitch threshold for Squelch =1
 * @param GTSC Glitch threshold for Squelch =0
 */
void bk4819_set_Squelch(uint8_t RTSO, uint8_t RTSC, uint8_t ETSO, uint8_t ETSC, uint8_t GTSO, uint8_t GTSC)
{
    bk1080_write_reg(BK4819_REG_78, (RTSO << 8) | RTSC);
    bk1080_write_reg(BK4819_REG_4F, (ETSC << 8) | ETSO);
    bk1080_write_reg(BK4819_REG_4D, GTSC);
    bk1080_write_reg(BK4819_REG_4E, GTSO);
}

/**
 * @brief Enable CTCSS/CDCSS
 *
 * @param sel 0:CDCSS   1:CTCSS
 */
void bk4819_CTDCSS_enable(uint8_t sel)
{
    uint16_t reg = bk4819_read_reg(BK4819_REG_51);
    reg |= BK4819_REG51_TX_CTCDSS_ENABLE;
    if (sel)
        reg |= BK4819_REG51_CTCSCSS_MODE_SEL;
    else
        reg &= ~BK4819_REG51_CTCSCSS_MODE_SEL;
}

void bk4819_CTDCSS_disable(void)
{
    uint16_t reg = bk4819_read_reg(BK4819_REG_51);
    reg &= ~BK4819_REG51_TX_CTCDSS_ENABLE;
    bk4819_write_reg(BK4819_REG_51, reg);
}

/**
 * @brief Set CDCSS code
 *
 * @param sel  0:CDCSS high 12 bits     1:CDCSS low 12 bits
 * @param code CDCSS code
 */
void bk4819_CDCSS_set(uint8_t sel, uint16_t code)
{
    bk1080_write_reg(BK4819_REG_08, (BK4819_REG_08, sel << 15) | code);
}

/**
 * @brief Set Conefficient for detection
 *
 * @param number Symbol number
 * @param coeff Coefficient
 */
void bk4819_DTMF_SELCall_set(uint8_t number, uint8_t coeff)
{
    bk1080_write_reg(BK4819_REG_09, (number << 12) | (coeff));
}

