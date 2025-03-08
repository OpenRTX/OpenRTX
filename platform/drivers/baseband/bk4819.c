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
        // delayUs(1);
        BK4819_SCK_HIGH;
        delayUs(1);
        BK4819_SCK_LOW;
        delayUs(1);
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
        delayUs(1);
        BK4819_SCK_HIGH;
        delayUs(1);
        data |= BK4819_SDA_READ;
    }
    return data;
}

uint16_t ReadRegister(unsigned char reg)
{
    uint16_t data;
    BK4819_SCN_LOW;
    delayUs(1);

    spi_write_byte(reg | BK4819_REG_READ);
    data = spi_read_half_word();

    delayUs(1);
    BK4819_SCN_HIGH;
    delayUs(1);
    return data;
}

static void WriteRegister(bk4819_reg_t reg, uint16_t data)
{
    BK4819_SCN_LOW;
    delayUs(1);

    spi_write_byte(reg | BK4819_REG_WRITE);
    spi_write_half_word(data);

    delayUs(1);
    BK4819_SCN_HIGH;
}

void bk4819_init(void)
{
    uint16_t uVar1;
    WriteRegister(0, 0x8000);
    WriteRegister(0, 0);
    WriteRegister(0x37, 0x1d0f);
    WriteRegister(0x13, 0x3be);
    WriteRegister(0x12, 0x37b);
    WriteRegister(0x11, 0x27b);
    WriteRegister(0x10, 0x7a);
    WriteRegister(0x14, 0x19);
    WriteRegister(0x49, 0x2a38);
    WriteRegister(0x7b, 0x8420);
    WriteRegister(0x7d, 0xe959);
    WriteRegister(0x48, 0xb3ff);
    WriteRegister(0x1e, 0x4c58);
    WriteRegister(0x1f, 0xa656);
    WriteRegister(0x3e, 0xa037);
    WriteRegister(0x3f, 0x7fe);
    WriteRegister(0x2a, 0x7fff);
    WriteRegister(0x28, 0x6b00);
    WriteRegister(0x53, 59000);
    WriteRegister(0x2c, 0x5705);
    WriteRegister(0x4b, 0x7102);
    uVar1 = ReadRegister(0x40);
    WriteRegister(0x40, uVar1 & 0xf000 | 0x4d2);
    WriteRegister(0x77, 0x88ef);
    WriteRegister(0x26, 0x13a0);
    WriteRegister(0x4e, 0x6f15);
    WriteRegister(0x4f, 0x3f3e);
    WriteRegister(9, 0x6f);
    WriteRegister(9, 0x106b);
    WriteRegister(9, 0x2067);
    WriteRegister(9, 0x3062);
    WriteRegister(9, 0x4050);
    WriteRegister(9, 0x5047);
    WriteRegister(9, 0x603a);
    WriteRegister(9, 0x702c);
    WriteRegister(9, 0x8041);
    WriteRegister(9, 0x9037);
    WriteRegister(9, 0xa025);
    WriteRegister(9, 0xb017);
    WriteRegister(9, 0xc0e4);
    WriteRegister(9, 0xd0cb);
    WriteRegister(9, 0xe0b5);
    WriteRegister(9, 0xf09f);
    WriteRegister(0x74, 0xfa02);
    WriteRegister(0x44, 0x8f88);
    WriteRegister(0x45, 0x3201);
    uVar1 = ReadRegister(0x31);
    WriteRegister(0x31, uVar1 & 0xfffffff7);
    WriteRegister(0x28, 0x6b38);
    WriteRegister(0x29, 0xb4cb);
    WriteRegister(BK4819_REG_36, 0xdfbf);
}

uint8_t bk4819_int_get(bk4819_int_t interrupt)
{
    if ((ReadRegister(BK4819_REG_0C) & BIT(0x01)) == 0) return 0;
    return ReadRegister(BK4819_REG_02 & interrupt);
}

void bk4819_int_enable(bk4819_int_t interrupt)
{
    WriteRegister(BK4819_REG_3F, ReadRegister(BK4819_REG_3F) | interrupt);
}

void bk4819_int_disable(bk4819_int_t interrupt)
{
    WriteRegister(BK4819_REG_3F, ReadRegister(BK4819_REG_3F) & (~interrupt));
}

void bk4819_set_freq(uint32_t freq)
{
    WriteRegister(BK4819_REG_39, (freq >> 16) & 0xFFFF);
    WriteRegister(BK4819_REG_38, freq & 0xFFFF);
    bk4819_rx_on();
}

void bk4819_rx_on(void)
{
    WriteRegister(0x37, 0x1F0F);
    delayUs(5);
    WriteRegister(0x30, 0x0200);
    WriteRegister(0x30, 0xBFF1);
}

void bk4819_tx_on(void)
{
    WriteRegister(BK4819_REG_30, 0x00);  // reset
    WriteRegister(BK4819_REG_30,
                  BK4819_REG30_REVERSE1_ENABLE | BK4819_REG30_REVERSE2_ENABLE |
                      BK4819_REG30_VCO_CALIBRATION |
                      BK4819_REG30_MIC_ADC_ENABLE | BK4819_REG30_TX_DSP_ENABLE |
                      BK4819_REG30_PLL_VCO_ENABLE |
                      BK4819_REG30_PA_GAIN_ENABLE);
}

void bk4819_rtx_off(void){
    WriteRegister(BK4819_REG_30, 0x00);  // reset
}

// set BK4819 power
void bk4819_setTxPower(uint32_t power)
{
    // Power setting is done by setting the PA bias voltage
    // in REG_36<15:8> and the gain tuning values in <5:0>.
    // I'm eyeballing what 1W, 5W and 10W should be,
    // but I'm not at all sure if the values are correct.
    uint16_t reg = 0;
    switch (power)
    {
    case 1000:
        reg = 0x70AA;
        break;
    case 5000:
        reg = 0xA2AD;
        break;
    case 10000:
        reg = 0xFFBF;
        break;
    default:
        reg = 0x2092;
        break;
    }
    WriteRegister(BK4819_REG_36, reg);
}

// toggle BK4819 GPIO pins
void bk4819_gpio_pin_set(uint8_t Pin, bool bSet)
{
    Pin += 1;
    uint16_t BK4819_GpioOutState = ReadRegister(BK4819_REG_33);
    // Enable GPIO output (set REG_33<15:8> to 0x00)
    // HACK: this enables all GPIO pins as output
    BK4819_GpioOutState &= 0x00FF;
    if (bSet)
    {
        BK4819_GpioOutState |= (0x0080 >> Pin);
    }
    else
    {
        BK4819_GpioOutState &= ~(0x0080 >> Pin);
    }
    WriteRegister(BK4819_REG_33, BK4819_GpioOutState);
}

void bk4819_enable_tx_ctcss(uint16_t frequency)
{
    uint16_t reg = ReadRegister(BK4819_REG_51);
    reg |= BK4819_REG51_TX_CTCDSS_ENABLE | BK4819_REG51_CTCSCSS_MODE_SEL;
    WriteRegister(BK4819_REG_51, reg);
    WriteRegister(BK4819_REG_07, frequency * 20.64888);
}

void bk4819_enable_rx_ctcss(uint16_t frequency)
{
    uint16_t reg = ReadRegister(BK4819_REG_51);
    reg |= BK4819_REG51_CTCSCSS_MODE_SEL;
    WriteRegister(BK4819_REG_51, reg);
    WriteRegister(BK4819_REG_07, frequency * 20.64888);
}

void bk4819_enable_ctcss2(uint16_t frequency)
{
    uint16_t reg = ReadRegister(BK4819_REG_51);
    reg |= BK4819_REG51_TX_CTCDSS_ENABLE | BK4819_REG51_CTCSCSS_MODE_SEL;
    WriteRegister(BK4819_REG_07, frequency | BIT(13));
}

void bk4819_enable_tx_cdcss(uint8_t code_type, uint8_t bit_sel, uint32_t cdcss_code)
{
    WriteRegister(BK4819_REG_51, BK4819_REG51_TX_CTCDSS_ENABLE | BITV(code_type, 13) | BITV(bit_sel, 11));
    WriteRegister(BK4819_REG_07, BITV(2, 13) | 0x0AD7);
    WriteRegister(BK4819_REG_08, BIT(15) | ((cdcss_code >> 12) & 0XFFF));
    WriteRegister(BK4819_REG_08, cdcss_code  & 0XFFF);
}

void bk4819_disable_ctdcss(void)
{
    uint16_t reg = ReadRegister(BK4819_REG_51);
    reg &= ~BK4819_REG51_TX_CTCDSS_ENABLE;
    WriteRegister(BK4819_REG_51, reg);
}

uint16_t bk4819_get_ctcss(void)
{
    return ReadRegister(BK4819_REG_0C) & BIT(10);
}

void bk4819_enable_vox(uint8_t delay_time,
                       uint8_t interval_time,
                       uint16_t threshold_on,
                       uint16_t threshold_off)
{
    WriteRegister(BK4819_REG_31, ReadRegister(BK4819_REG_31) | BIT(2));
    WriteRegister(BK4819_REG_79, BITV(interval_time, 10) | threshold_off);
    WriteRegister(BK4819_REG_46, threshold_on);
}

uint8_t bk4819_get_vox(void){
    return ReadRegister(BK4819_REG_0C) & BIT(2);
}

void bk4819_set_Squelch(uint8_t RTSO,
                        uint8_t RTSC,
                        uint8_t ETSO,
                        uint8_t ETSC,
                        uint8_t GTSO,
                        uint8_t GTSC)
{
    WriteRegister(BK4819_REG_78, (RTSO << 8) | RTSC);
    WriteRegister(BK4819_REG_4F, (ETSC << 8) | ETSO);
    WriteRegister(BK4819_REG_4D, GTSC);
    WriteRegister(BK4819_REG_4E, GTSO);
}

int16_t bk4819_get_rssi(void)
{
    return ((ReadRegister(BK4819_REG_67)) / 2) - 160;
}

void bk4819_enable_freq_scan(uint8_t scna_time){
    WriteRegister(BK4819_REG_32, ReadRegister(BK4819_REG_32) | BITV(scna_time, 14));
    WriteRegister(BK4819_REG_32, ReadRegister(BK4819_REG_32) | 0x01);
}

void bk4819_disable_freq_scan(void){
     WriteRegister(BK4819_REG_32, ReadRegister(BK4819_REG_32) & (~0x01));
}

uint8_t bk4819_get_scan_freq_flag(void){
    return ReadRegister(BK4819_REG_0D) & BIT(15);
}

uint32_t bk4819_get_scan_freq(void){
    return ((ReadRegister(BK4819_REG_0D) << 16) | ReadRegister(BK4819_REG_0E)) / 10;
}

void BK4819_SetAF(uint8_t AF)
{
	// AF Output Inverse Mode = Inverse
	// Undocumented bits 0x2040
	//
//	BK4819_WriteRegister(BK4819_REG_47, 0x6040 | (AF << 8));
	WriteRegister(BK4819_REG_47, (6u << 12) | (AF << 8) | (1u << 6));
}

__inline uint16_t scale_freq(const uint16_t freq)
{
//	return (((uint32_t)freq * 1032444u) + 50000u) / 100000u;   // with rounding
	return (((uint32_t)freq * 1353245u) + (1u << 16)) >> 17;   // with rounding
}

// Play tone
void BK4819_PlayTone(uint16_t Frequency, bool bTuningGainSwitch)
{
	uint16_t ToneConfig = BK4819_REG_70_ENABLE_TONE1;
    //gpio_setPin(MIC_SPK_EN);              
	WriteRegister(BK4819_REG_50, 0xBB20);
	BK4819_SetAF(3); // AF Beep

	if (bTuningGainSwitch == 0)
		ToneConfig |=  96u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN;
	else
		ToneConfig |= 28u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN;
	WriteRegister(BK4819_REG_70, ToneConfig);   

	WriteRegister(BK4819_REG_30, 0);
	WriteRegister(BK4819_REG_30, BK4819_REG_30_ENABLE_AF_DAC | BK4819_REG_30_ENABLE_DISC_MODE | BK4819_REG_30_ENABLE_TX_DSP);

	WriteRegister(BK4819_REG_71, scale_freq(Frequency));
    //usart0_IRQwrite("BK4819_PlayTone\r\n");
}