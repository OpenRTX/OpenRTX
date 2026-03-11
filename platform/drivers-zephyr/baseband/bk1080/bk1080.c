#define DT_DRV_COMPAT beken_bk1080


#include "bk1080.h"
#include <interfaces/delays.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(bk1080, LOG_LEVEL_DBG);


#define BK1080_NODE DT_PATH(bk1080)

/* get pin definition from DTS */
static const struct gpio_dt_spec clk_gpio = GPIO_DT_SPEC_GET(BK1080_NODE, i2c_clk_gpios);
static const struct gpio_dt_spec data_gpio = GPIO_DT_SPEC_GET(BK1080_NODE, i2c_data_gpios);
static const struct gpio_dt_spec power_gpio = GPIO_DT_SPEC_GET(BK1080_NODE, power_gpios);

// GPIO control macros for SCK (Clock)
#define BK1080_SCK_DIR_OUT gpio_pin_configure_dt(&clk_gpio, GPIO_OUTPUT)
#define BK1080_SCK_HIGH    gpio_pin_set_dt(&clk_gpio, 1)
#define BK1080_SCK_LOW     gpio_pin_set_dt(&clk_gpio, 0)

// GPIO control macros for SDA (Serial Data)
#define BK1080_SDA_DIR_OUT gpio_pin_configure_dt(&data_gpio, GPIO_OUTPUT)
#define BK1080_SDA_DIR_IN  gpio_pin_configure_dt(&data_gpio, GPIO_INPUT)
#define BK1080_SDA_HIGH    gpio_pin_set_dt(&data_gpio, 1)
#define BK1080_SDA_LOW     gpio_pin_set_dt(&data_gpio, 0)
#define BK1080_SDA_READ    gpio_pin_get_dt(&data_gpio)

// GPIO control macros for FM POWER
#define BK1080_POWER_DIR_OUT gpio_pin_configure_dt(&power_gpio, GPIO_OUTPUT)
#define BK1080_POWER_OFF    gpio_pin_set_dt(&power_gpio, 0)
#define BK1080_POWER_ON     gpio_pin_set_dt(&power_gpio, 1)



// Forward declaration for device initialization
static int bk1080_init_device(const struct device *dev);

// Device driver structure
static const struct bk1080_config {
    // Configuration data would go here
} bk1080_cfg = {
    // Configuration initialization
};

// Device data structure
static struct bk1080_data {
    bool initialized;
} bk1080_device_data = {
    .initialized = false
};


#ifndef ARRAY_SIZE
	#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

static const uint16_t BK1080_RegisterTable[] =
{
	0x0008, 0x1080, 0x0201, 0x0000, 0x40C0, 0x0A1F, 0x002E, 0x02FF,
	0x5B11, 0x0000, 0x411E, 0x0000, 0xCE00, 0x0000, 0x0000, 0x1000,
	0x3197, 0x0000, 0x13FF, 0x9852, 0x0000, 0x0000, 0x0008, 0x0000,
	0x51E1, 0xA8BC, 0x2645, 0x00E4, 0x1CD8, 0x3A50, 0xEAE0, 0x3000,
	0x0200, 0x0000,
};

static bool gIsInitBK1080;

uint16_t BK1080_BaseFrequency;
uint16_t BK1080_FrequencyDeviation;

enum {
	I2C_WRITE = 0U,
	I2C_READ = 1U,
};

static void I2C_Start(void);
static void I2C_Stop(void);

static uint8_t I2C_Read(bool bFinal);
static int I2C_Write(uint8_t Data);

static int I2C_ReadBuffer(void *pBuffer, uint8_t Size);
static int I2C_WriteBuffer(const void *pBuffer, uint8_t Size);

static void I2C_Start(void)
{
	BK1080_SDA_HIGH; delayUs(1);
	BK1080_SCK_HIGH; delayUs(1);
	BK1080_SDA_LOW; delayUs(1);
	BK1080_SCK_LOW; delayUs(1);
}

static void I2C_Stop(void)
{
	BK1080_SDA_LOW; delayUs(1);
	BK1080_SCK_LOW; delayUs(1);
	BK1080_SCK_HIGH; delayUs(1);
	BK1080_SDA_HIGH; delayUs(1);
}

static uint8_t I2C_Read(bool bFinal)
{
	uint8_t i, Data;

	BK1080_SDA_DIR_IN;

	Data = 0;
	for (i = 0; i < 8; i++) {
		BK1080_SCK_LOW;
		delayUs(1);
		BK1080_SCK_HIGH;
		delayUs(1);
		Data <<= 1;
		delayUs(1);
		if (BK1080_SDA_READ) {
			Data |= 1U;
		}
		BK1080_SCK_LOW;
		delayUs(1);
	}

	BK1080_SDA_DIR_OUT;
	BK1080_SCK_LOW;
	delayUs(1);
	if (bFinal) {
		BK1080_SDA_HIGH;
	} else {
		BK1080_SDA_LOW;
	}
	delayUs(1);
	BK1080_SCK_HIGH;
	delayUs(1);
	BK1080_SCK_LOW;
	delayUs(1);

	return Data;
}

static int I2C_Write(uint8_t Data)
{
	uint8_t i;
	int ret = -1;

	BK1080_SCK_LOW;
	delayUs(1);
	for (i = 0; i < 8; i++) {
		if ((Data & 0x80) == 0) {
			BK1080_SDA_LOW;
		} else {
			BK1080_SDA_HIGH;
		}
		Data <<= 1;
		delayUs(1);
		BK1080_SCK_HIGH;
		delayUs(1);
		BK1080_SCK_LOW;
		delayUs(1);
	}

	BK1080_SDA_DIR_IN;
	BK1080_SDA_HIGH;
	delayUs(1);
	BK1080_SCK_HIGH;
	delayUs(1);

	for (i = 0; i < 255; i++) {
		if (BK1080_SDA_READ == 0) {
			ret = 0;
			break;
		}
	}

	BK1080_SCK_LOW;
	delayUs(1);
	BK1080_SDA_DIR_OUT;
	BK1080_SDA_LOW;

	return ret;
}

static int I2C_ReadBuffer(void *pBuffer, uint8_t Size)
{
	uint8_t *pData = (uint8_t *)pBuffer;
	uint8_t i;

	for (i = 0; i < Size - 1; i++) {
		delayUs(1);
		pData[i] = I2C_Read(false);
	}

    delayUs(1);
	pData[i] = I2C_Read(true);

	return Size;
}

static int I2C_WriteBuffer(const void *pBuffer, uint8_t Size)
{
	const uint8_t *pData = (const uint8_t *)pBuffer;
	uint8_t i;

	for (i = 0; i < Size; i++) {
		if (I2C_Write(*pData++) < 0) {
			return -1;
		}
	}

	return 0;
}




uint16_t BK1080_ReadRegister(BK1080_Register_t Register)
{
	uint8_t Value[2];

	I2C_Start();
	I2C_Write(0x80);
	I2C_Write((Register << 1) | I2C_READ);
	I2C_ReadBuffer(Value, sizeof(Value));
	I2C_Stop();

	return (Value[0] << 8) | Value[1];
}

void BK1080_WriteRegister(BK1080_Register_t Register, uint16_t Value)
{
	I2C_Start();
	I2C_Write(0x80);
	I2C_Write((Register << 1) | I2C_WRITE);
	Value = ((Value >> 8) & 0xFF) | ((Value & 0xFF) << 8);
	I2C_WriteBuffer(&Value, sizeof(Value));
	I2C_Stop();
}

void BK1080_Init(uint32_t freq, uint8_t band)
{
	unsigned int i;

	if (freq) {
		BK1080_POWER_ON;

		if (!gIsInitBK1080) {
			for (i = 0; i < ARRAY_SIZE(BK1080_RegisterTable); i++)
				BK1080_WriteRegister(i, BK1080_RegisterTable[i]);

			delayUs(250000);

			BK1080_WriteRegister(BK1080_REG_25_INTERNAL, 0xA83C);
			BK1080_WriteRegister(BK1080_REG_25_INTERNAL, 0xA8BC);

			delayUs(60000);

			gIsInitBK1080 = true;
		}
		else {
			BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, 0x0201);
		}

		BK1080_WriteRegister(BK1080_REG_05_SYSTEM_CONFIGURATION2, 0x0A1F);
		BK1080_SetFrequency(freq, band/*, space*/);
	}
	else {
		BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, 0x0241);
		BK1080_POWER_OFF;
	}
}

void BK1080_Mute(bool Mute)
{
	BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, Mute ? 0x4201 : 0x0201);
}

void BK1080_SetFrequency(uint32_t frequency, uint8_t band)
{
	uint16_t channel = (frequency - BK1080_GetFreqLoLimit(band)) / 100000;

	uint16_t regval = BK1080_ReadRegister(BK1080_REG_05_SYSTEM_CONFIGURATION2);
	regval = (regval & ~(0b11 << 6)) | ((band & 0b11) << 6);

	BK1080_WriteRegister(BK1080_REG_05_SYSTEM_CONFIGURATION2, regval);

	BK1080_WriteRegister(BK1080_REG_03_CHANNEL, channel);
	delayMs(10);
	BK1080_WriteRegister(BK1080_REG_03_CHANNEL, channel | 0x8000);
}

void BK1080_GetFrequencyDeviation(uint32_t Frequency)
{
	BK1080_BaseFrequency      = Frequency;
	BK1080_FrequencyDeviation = BK1080_ReadRegister(BK1080_REG_07) / 16;
}

uint32_t BK1080_GetFreqLoLimit(uint8_t band)
{
	uint32_t lim[] = {87500000, 76000000, 76000000, 64000000}; //Hz
	return lim[band % 4];
}

uint32_t BK1080_GetFreqHiLimit(uint8_t band)
{
	band %= 4;
	uint32_t lim[] = {108000000, 108000000, 90000000, 76000000}; //Hz
	return lim[band % 4];
}



// Zephyr device initialization function
static int bk1080_init_device(const struct device *dev)
{
    LOG_INF("Initializing BK1080 device");
    
    // Initialize the BK1080 hardware
    BK1080_POWER_DIR_OUT;
    BK1080_POWER_OFF;

    // Configure CS and CLK as outputs
    BK1080_SCK_DIR_OUT;
    BK1080_SDA_DIR_OUT;

    BK1080_Init(0,0);
    
    // Mark as initialized
    struct bk1080_data *data = dev->data;
    data->initialized = true;
    
    LOG_INF("BK1080 device initialized successfully");
    return 0;
}

// Define the device using Zephyr's device tree macros
#define DT_DRV_COMPAT beken_bk1080

// Device driver API structure (empty for now, but required)
static const struct bk1080_driver_api {
    // API functions would go here
} bk1080_api = {
    // API initialization
};

// Register the device with Zephyr
DEVICE_DT_INST_DEFINE(0,                    /* Instance 0 */
                      bk1080_init_device,   /* Init function */
                      NULL,                 /* PM device */
                      &bk1080_device_data,  /* Device data */
                      &bk1080_cfg,          /* Device config */
                      POST_KERNEL,          /* Init level */
                      CONFIG_BK1080_INIT_PRIORITY, /* Init priority */
                      &bk1080_api);         /* API */
