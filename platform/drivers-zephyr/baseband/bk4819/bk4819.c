#define DT_DRV_COMPAT beken_bk4819
/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *   Copyright (C) 2024 by Jamiexu                                         *
 *   Copyright (C) 2025 by Andrej A, K8TUN                                 *
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

#include "bk4819.h"
#include <interfaces/delays.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(bk4819, LOG_LEVEL_DBG);

#define BK4819_NODE DT_PATH(bk4819)

/* get pin definition from DTS */
static const struct gpio_dt_spec clk_gpio = GPIO_DT_SPEC_GET(BK4819_NODE, sclk_gpios);
static const struct gpio_dt_spec data_gpio = GPIO_DT_SPEC_GET(BK4819_NODE, sdata_gpios);
static const struct gpio_dt_spec scn_gpio = GPIO_DT_SPEC_GET(BK4819_NODE, scn_gpios);

// GPIO control macros for SCK (Clock)
#define BK4819_SCK_DIR_OUT gpio_pin_configure_dt(&clk_gpio, GPIO_OUTPUT)
#define BK4819_SCK_HIGH    gpio_pin_set_dt(&clk_gpio, 1)
#define BK4819_SCK_LOW     gpio_pin_set_dt(&clk_gpio, 0)

// GPIO control macros for SDA (Serial Data)
#define BK4819_SDA_DIR_OUT gpio_pin_configure_dt(&data_gpio, GPIO_OUTPUT)
#define BK4819_SDA_DIR_IN  gpio_pin_configure_dt(&data_gpio, GPIO_INPUT)
#define BK4819_SDA_HIGH    gpio_pin_set_dt(&data_gpio, 1)
#define BK4819_SDA_LOW     gpio_pin_set_dt(&data_gpio, 0)
#define BK4819_SDA_READ    gpio_pin_get_dt(&data_gpio)

// GPIO control macros for FM POWER
#define BK4819_SCN_DIR_OUT gpio_pin_configure_dt(&scn_gpio, GPIO_OUTPUT)
#define BK4819_SCN_SELECT  gpio_pin_set_dt(&scn_gpio, 1)
#define BK4819_SCN_DESELECT  gpio_pin_set_dt(&scn_gpio, 0)


// Forward declaration for device initialization
static int bk4819_init_device(const struct device *dev);
static void spi_write_byte(uint8_t data);
static uint16_t spi_read_half_word(void);

// Device driver structure
static const struct bk4819_config {
    // Configuration data would go here
} bk4819_cfg = {
    // Configuration initialization
};

// Device data structure
static struct bk4819_data {
    bool initialized;
} bk4819_device_data = {
    .initialized = false
};



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
    //return 0x00;
    uint16_t data;
    BK4819_SCN_SELECT;
    delayUs(1);

    spi_write_byte(reg | BK4819_REG_READ);
    data = spi_read_half_word();

    delayUs(1);
    BK4819_SCN_DESELECT;
    return data;
}

void WriteRegister(bk4819_reg_t reg, uint16_t data)
{
    BK4819_SCN_SELECT;
    delayUs(1);

    spi_write_byte(reg | BK4819_REG_WRITE);
    spi_write_half_word(data);

    delayUs(1);
    BK4819_SCN_DESELECT;
}

void bk4819_init(void)
{
    BK4819_SDA_DIR_OUT;
    BK4819_SDA_HIGH;
    // Configure CS and CLK as outputs
    BK4819_SCN_DIR_OUT;
    BK4819_SCK_DIR_OUT;

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
    WriteRegister(0x48, 0xb3c1);
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
    WriteRegister(0x40, (uVar1 & 0xf000) | 0x4d2);
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
    freq = freq / 10; // Convert to 10 Hz units
    WriteRegister(BK4819_REG_39, (freq >> 16) & 0xFFFF);
    WriteRegister(BK4819_REG_38, freq & 0xFFFF);
    bk4819_rx_on();
}

void bk4819_rx_on(void)
{
    WriteRegister(0x37, 0x1F0F);
    delayUs(1);
    WriteRegister(0x30, 0x0200);
    WriteRegister(0x30, 0xBFF1);
}

void bk4819_set_modulation(bool is_FM)
{
    BK4819_SetAF(is_FM ? 1 : 7);
}

void bk4819_tx_on(void)
{
    //gpio_clearPin(MIC_SPK_EN);
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
    WriteRegister(BK4819_REG_30, BK4819_REG_30_ENABLE_AF_DAC);
}

void bk4819_SetFilterBandwidth(uint8_t bandwidth)
{
    uint16_t Value = ReadRegister(0x43); 
    if (bandwidth) { // 25kHz
        WriteRegister(0x43, (Value & ~0x30) | 32);
    } else { //12.5kHz
        WriteRegister(0x43, (Value & ~0x30) | 0);
    }
}

// Consolidated function to get the calibration value based on frequency
// uint8_t getPaBiasCalValue(uint32_t freq, PowerCalibration calTable) {
//     if (freq < 130000000) {
//         return calTable.power_below_130mhz;
//     } else if (freq >= 455000000 && freq <= 470000000) {
//         return calTable.power_455_470mhz;
//     } else if (freq >= 420000000 && freq < 455000000) {
//         return calTable.power_420_455mhz;
//     } else if (freq >= 300000000 && freq < 420000000) {
//         return calTable.power_300_420mhz;
//     } else if (freq >= 200000000 && freq < 300000000) {
//         return calTable.power_200_300mhz;
//     } else if (freq >= 166000000 && freq < 200000000) {
//         return calTable.power_166_200mhz;
//     } else if (freq >= 145000000 && freq < 166000000) {
//         return calTable.power_145_166mhz;
//     } else if (freq >= 130000000 && freq < 145000000) {
//         return calTable.power_130_145mhz;
//     }
//     // Default case, should not happen if freq is within valid range
//     return calTable.power_below_130mhz;
//     return 0;
// }

// void bk4819_setTxPower(uint32_t power, uint32_t freq, PowerCalibrationTables calData)
// {
//     uint16_t reg = 0;
//     uint8_t PaBias = 0;
//     uint8_t PaGainValues = 0;
    
//     // NOTE: PaGainValues taken straight from disassembly.

//     // Determine the PaGainValues based on power
//     switch (power)
//     {
//     case 1000:
//         PaGainValues = 0xD7;
//         // Retrieve the top byte from calData based on power and frequency
//         PaBias = getPaBiasCalValue(freq, calData.low);
//         break;
//     case 5000:
//         PaGainValues = 0xD7;
//         PaBias = getPaBiasCalValue(freq, calData.med);
//         break;
//     case 10000:
//         PaGainValues = 0xFF;
//         PaBias = getPaBiasCalValue(freq, calData.high);
//         break;
//     default:
//         PaGainValues = 0x13;
//         PaBias = getPaBiasCalValue(freq, calData.low);
//         break;
//     }

//     // Combine the top byte with the least significant byte
//     reg = (PaBias << 8) | PaGainValues;

//     WriteRegister(BK4819_REG_36, reg);
// }

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
    // frequency is in .1 Hz units
    uint32_t ctcss_reg_value = frequency * 2064888 / 100000 / 10; // Convert to register value for 26MHz XTAL at BK4918 in C62

    uint16_t reg = ReadRegister(BK4819_REG_51);
    reg |= BK4819_REG51_TX_CTCDSS_ENABLE | BK4819_REG51_CTCSCSS_MODE_SEL;
    WriteRegister(BK4819_REG_51, reg);
    WriteRegister(BK4819_REG_07, (uint16_t) ctcss_reg_value);
}

void bk4819_enable_rx_ctcss(uint16_t frequency)
{
     // frequency is in .1 Hz units
    uint32_t ctcss_reg_value = frequency * 2064888 / 100000 / 10; // Convert to register value for 26MHz XTAL at BK4918 in C62

    uint16_t reg = ReadRegister(BK4819_REG_51);
    reg |= BK4819_REG51_CTCSCSS_MODE_SEL;
    WriteRegister(BK4819_REG_51, reg);
    WriteRegister(BK4819_REG_07, (uint16_t)ctcss_reg_value);
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
    return ((ReadRegister(BK4819_REG_67) & 0x01FF) / 2) - 160;
}

uint16_t bk4819_get_mic_level(void)
{
    // bits 6:0 of AF TX/RX Input Amplitude
    return (ReadRegister(0x6f) & 0x7f) * 2;
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
//	WriteRegister(BK4819_REG_47, 0x6040 | (AF << 8));
	WriteRegister(BK4819_REG_47, (6u << 12) | (AF << 8) | (1u << 6));
}

__inline uint16_t scale_freq(const uint16_t freq)
{
	return (((uint32_t)freq * 1048576u) + 50000u) / 100000u;   // with rounding
}

// Play tone
void BK4819_BeepStart(uint16_t Frequency, bool bTuningGainSwitch)
{
    //gpio_setPin(MIC_SPK_EN);              
	WriteRegister(BK4819_REG_50, 0x3B20);
    BK4819_SetAF(3); // AF Beep
	uint16_t ToneConfig = BK4819_REG_70_ENABLE_TONE1;
    ToneConfig |=  96u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN;
	WriteRegister(BK4819_REG_70, ToneConfig);  

	WriteRegister(BK4819_REG_71, scale_freq(Frequency));
    WriteRegister(BK4819_REG_30, 0);
	WriteRegister(BK4819_REG_30, BK4819_REG_30_ENABLE_AF_DAC | BK4819_REG_30_ENABLE_DISC_MODE | BK4819_REG_30_ENABLE_TX_DSP);
    //usart0_IRQwrite("BK4819_PlayTone\r\n");
    WriteRegister(BK4819_REG_50, 0x3B20);
    // for(int i = 0; i < 1000; i++)
    // {
    //     WriteRegister(BK4819_REG_71,scale_freq((i % 2) ? 1200 : 2200));
    //     delayUs(833);
    // }
}


void BK4819_BeepStop(void)
{
    WriteRegister(BK4819_REG_50, 0xBB20);
	// WriteRegister(BK4819_REG_30, 0xC1FE);
    //BK4819_SetAF(1);
}

// Zephyr device initialization function
static int bk4819_init_device(const struct device *dev)
{
    LOG_INF("Initializing BK4819 device");
    
    // Initialize the BK4819 hardware
    bk4819_init();
    
    // Mark as initialized
    struct bk4819_data *data = dev->data;
    data->initialized = true;
    
    LOG_INF("BK4819 device initialized successfully");
    return 0;
}

// Define the device using Zephyr's device tree macros
#define DT_DRV_COMPAT beken_bk4819

// Device driver API structure (empty for now, but required)
static const struct bk4819_driver_api {
    // API functions would go here
} bk4819_api = {
    // API initialization
};

// Register the device with Zephyr
DEVICE_DT_INST_DEFINE(0,                    /* Instance 0 */
                      bk4819_init_device,   /* Init function */
                      NULL,                 /* PM device */
                      &bk4819_device_data,  /* Device data */
                      &bk4819_cfg,          /* Device config */
                      POST_KERNEL,          /* Init level */
                      CONFIG_BK4819_INIT_PRIORITY, /* Init priority */
                      &bk4819_api);         /* API */
