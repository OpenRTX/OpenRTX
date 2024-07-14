#ifndef __BK4818_JAMIEXU_H__
#define __BK4818_JAMIEXU_H__


#include "gpio.h"
#include "peripherals/gpio.h"
// Written by Jamiexu
#ifdef __cplusplus
extern "C" {
#endif

#define BK4819_SCK_LOW gpio_clearPin(BK4819_CLK)
#define BK4819_SCK_HIGH gpio_setPin(BK4819_CLK)

#define BK4819_SDA_LOW gpio_clearPin(BK4819_DAT)
#define BK4819_SDA_HIGH gpio_setPin(BK4819_DAT)

#define BK4819_SCN_LOW gpio_clearPin(BK4819_CS)
#define BK4819_SCN_HIGH gpio_setPin(BK4819_CS)

#define BK4819_SDA_READ gpio_readPin(BK4819_DAT)

#define BK4819_SDA_DIR_OUT gpio_setMode(BK4819_DAT, OUTPUT)
#define BK4819_SDA_DIR_IN gpio_setMode(BK4819_DAT, INPUT_PULL_UP)

#define BK4819_REG_READ 0x80
#define BK4819_REG_WRITE 0x00

#define BIT(x) (1u << (x))
#define BITV(x, y) ((x) << (y))

#define BK4819_REG30_VCO_CALIBRATION BIT(15)
#define BK4819_REG30_REVERSE1_ENABLE BIT(14)
#define BK4819_REG30_RX_LINK_ENABLE BITV(0x0F, 10)
#define BK4819_REG30_REVERSE2_ENABLE BIT(8)
#define BK4819_REG30_AF_DAC_ENABLE BIT(9)
#define BK4819_REG30_PLL_VCO_ENABLE BITV(0x0F, 4)
#define BK4819_REG30_PA_GAIN_ENABLE BIT(3)
#define BK4819_REG30_MIC_ADC_ENABLE BIT(2)
#define BK4819_REG30_TX_DSP_ENABLE BIT(1)
#define BK4819_REG30_RX_DSP_ENABLE BIT(0)

#define BK4819_REG51_TX_CTCDSS_ENABLE BIT(15)
#define BK4819_REG51_GPIO6_CDCSS BIT(14)
#define BK4819_REG51_TRANSMIT_NEG_CDCSS_CODE BIT(13)
#define BK4819_REG51_CTCSCSS_MODE_SEL BIT(12)
#define BK4819_REG51_CDCSS_BIT_SEL BIT(11)
#define BK4819_REG51_1050HZ_DET_MOD BIT(10)
#define BK4819_REG51_AUTO_CDCSS_BW_MOD BIT(9)
#define BK4819_REG51_AUTO_CTCSS_BW_MOD BIT(8)
#define BK4819_REG51_CTCDCSS_TX_GAIN1T(x) BITV((x), 0)

typedef enum
{
    BK4819_REG_00 = 0x00,
    BK4819_REG_02 = 0x02,
    BK4819_REG_07 = 0x07,
    BK4819_REG_08 = 0x08,
    BK4819_REG_09 = 0x09,
    BK4819_REG_0A = 0x0A,
    BK4819_REG_0B = 0x0B,
    BK4819_REG_0C = 0x0C,

    BK4819_REG_0D = 0x0D,
    BK4819_REG_0E = 0x0E,
    BK4819_REG_10 = 0x10,
    BK4819_REG_11 = 0x11,
    BK4819_REG_12 = 0x12,
    BK4819_REG_13 = 0x13,
    BK4819_REG_14 = 0x14,
    BK4819_REG_18 = 0x18,

    BK4819_REG_19 = 0x19,
    BK4819_REG_1A = 0x1A,
    BK4819_REG_1F = 0x1F,
    BK4819_REG_1E = 0x1E,
    BK4819_REG_24 = 0x24,
    BK4819_REG_26 = 0x26,
    BK4819_REG_28 = 0x28,
    BK4819_REG_29 = 0x29,
    BK4819_REG_2A = 0X2A,
    BK4819_REG_2B = 0x2B,
    BK4819_REG_2C = 0x2c,
    BK4819_REG_2E = 0x2E,
    BK4819_REG_30 = 0x30,
    BK4819_REG_31 = 0x31,
    BK4819_REG_32 = 0x32,
    BK4819_REG_33 = 0x33,
    BK4819_REG_36 = 0x36,
    BK4819_REG_37 = 0x37,
    BK4819_REG_38 = 0x38,

    BK4819_REG_39 = 0x39,
    BK4819_REG_3B = 0x3B,
    BK4819_REG_3C = 0x3C,
    BK4819_REG_3D = 0x3D,
    BK4819_REG_3E = 0x3E,
    BK4819_REG_3F = 0x3F,
    BK4819_REG_40 = 0x40,
    BK4819_REG_43 = 0x43,

    BK4819_REG_44 = 0x44,
    BK4819_REG_45 = 0x45,
    BK4819_REG_46 = 0x46,
    BK4819_REG_47 = 0x47,
    BK4819_REG_48 = 0x48,
    BK4819_REG_49 = 0x49,

    BK4819_REG_4B = 0x4B,
    BK4819_REG_4D = 0x4D,
    BK4819_REG_4E = 0x4E,
    BK4819_REG_4F = 0x4F,
    BK4819_REG_50 = 0x50,
    BK4819_REG_51 = 0x51,
    BK4819_REG_52 = 0x52,
    BK4819_REG_54 = 0x54,
    BK4819_REG_53 = 0x53,
    BK4819_REG_55 = 0x55,
    BK4819_REG_58 = 0x58,
    BK4819_REG_59 = 0x59,
    BK4819_REG_5A = 0x5A,
    BK4819_REG_5B = 0x5B,
    BK4819_REG_5C = 0x5C,
    BK4819_REG_5D = 0x5D,
    BK4819_REG_5E = 0x5E,

    BK4819_REG_5F = 0x5F,
    BK4819_REG_63 = 0x63,
    BK4819_REG_64 = 0x64,
    BK4819_REG_65 = 0x65,
    BK4819_REG_67 = 0x67,
    BK4819_REG_68 = 0x68,
    BK4819_REG_69 = 0x69,
    BK4819_REG_6A = 0x6A,

    BK4819_REG_6F = 0x6F,
    BK4819_REG_70 = 0x70,
    BK4819_REG_71 = 0x71,
    BK4819_REG_72 = 0x72,
    BK4819_REG_73 = 0x73,
    BK4819_REG_74 = 0x74,
    BK4819_REG_75 = 0x75,
    BK4819_REG_77 = 0x77,

    BK4819_REG_78 = 0x78,
    BK4819_REG_79 = 0x79,
    BK4819_REG_7A = 0x7A,
    BK4819_REG_7B = 0x7B,
    BK4819_REG_7C = 0x7C,
    BK4819_REG_7D = 0x7D,
    BK4819_REG_7E = 0x7E,
} bk4819_reg_t;

typedef enum
{
    BK4819_INT_FSKTF = BIT(15),   // FSK TX Finished Interrupt
    BK4819_INT_FSKFFAE = BIT(14), // FSK FIFO Almost Empty interrupt
    BK4819_INT_FSKRXF = BIT(13),  // FSK RX Finished interrupt
    BK4819_INT_FSKFFAF = BIT(12), // FSK FIFO Almost Full interrupt
    BK4819_INT_DTMFTF = BIT(11),  // DTMF/5 TONE Found interrupt
    BK4819_INT_CTDSTF = BIT(10),  // CTCSS/CDCSS Tail Found interrupt
    BK4819_INT_CDCSF = BIT(9),    // CDCSS Found interrupt
    BK4819_INT_CDCSL = BIT(8),    // CDCSS Lost interrupt
    BK4819_INT_CTSSF = BIT(7),    // CTCSS Found interrupt
    BK4819_INT_CTCSL = BIT(6),    // CTCSS Lost interrupt
    BK4819_INT_VOXF = BIT(5),     // VOX Found interrupt
    BK4819_INT_VOXL = BIT(4),     // VOX Lost interrupt
    BK4819_INT_SECF = BIT(3),     // Squelch Found interrupt
    BK4819_INT_SECL = BIT(2),     // Squelch Lost interrupt
    BK4819_INT_FSKRS = BIT(1)     // FSK RX Sync interrupt
} bk4819_int_t;

typedef enum
{
    BK4819_FLAG_DTMF_REV = BITV(0x8, 11), // DTMF/5 Tone code received
    BK4819_FLAG_FSK_RX_SNF = BIT(7),      // FSK RX Sync Negative has been found
    BK4819_FLAG_FSK_RX_SPF = BIT(6),      // FSK RX Sync Positive has been found
    BK4819_FLAG_FSK_RX_CRC = BIT(4),      // FSK RX CRC indicator
    BK4819_FLAG_CDCSS_PCR = BIT(14),      // CDCSS positive code received
    BK4819_FLAG_CDCSS_PNR = BIT(15),      //  CDCSS negative code received
} bk4819_flag_t;

typedef enum
{
    BK4819_RDATA_
} bk4819_rdata_t;

// typedef enum
// {
//     bk4819_CTCSS_PHASE_120 = 0X01,
//     bk4819_CTCSS_PHASE_180 = 0X02,
//     bk4819_CTCSS_PHASE_240 = 0X03,
// } bk4819_CTCSS_PHASE;

// typedef enum
// {
//     bk4819_RX_ACG_GAIN_PAG =
// } bk4819_RX_ACG_GAIN;

static void spi_write_byte(uint8_t data);
static void spi_write_half_word(uint16_t data);
static uint16_t spi_read_half_word(void);

static uint16_t ReadRegister(unsigned char reg);
static void WriteRegister(bk4819_reg_t reg, uint16_t data);

uint8_t bk4819_int_get(bk4819_int_t interrupt);
uint8_t bk4819_flag_get(void);

void bk4819_init(void);

void bk4819_set_freq(uint32_t frq);

void bk4819_rx_on(void);

void bk4819_tx_on(void);

void bk4819_enable_ctcss(uint16_t frequency);

void bk4819_enable_ctcss2(uint16_t frequency);

void bk4819_enable_cdcss();

void bk4819_disable_ctdcss(void);

uint16_t bk4819_get_ctcss(void);

void bk4819_set_Squelch(uint8_t RTSO, uint8_t RTSC, uint8_t ETSO, uint8_t ETSC, uint8_t GTSO, uint8_t GTSC);

int16_t bk4819_get_rssi(void);


#ifdef __cplusplus
}
#endif

#endif
