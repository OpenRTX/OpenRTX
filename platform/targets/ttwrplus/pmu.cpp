/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include "interfaces/delays.h"
#include "interfaces/keyboard.h"
#include "hwconfig.h"
#include "pmu.h"

// PMU is controlled through the XPowersLib external library
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"


#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_0), okay)
#define I2C_DEV_NODE    DT_ALIAS(i2c_0)
#else
#error "Please set the correct I2C device"
#endif

#define PMU_IRQ_NODE DT_ALIAS(pmu_irq)

static const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
static const struct gpio_dt_spec pmu_irq  = GPIO_DT_SPEC_GET(PMU_IRQ_NODE, gpios);
static XPowersPMU PMU;
static uint8_t pwrOnPressed = 0;


static int pmu_registerReadByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data,
                                uint8_t len)
{
    // Only single-byte reads are supported
    if (len != 1)
        return -1;

    return i2c_reg_read_byte(i2c_dev, devAddr, regAddr, data);
}

static int pmu_registerWriteByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data,
                                 uint8_t len)
{
    // Only single-byte writes are supported
    if (len != 1)
        return -1;

    return i2c_reg_write_byte(i2c_dev, devAddr, regAddr, *data);
}


void pmu_init()
{
    // Configure I2C connection with PMU
    if (device_is_ready(i2c_dev) == false)
    {
        printk("I2C device is not ready\n");
    }

    const uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER;
    if (i2c_configure(i2c_dev, i2c_cfg) != 0)
    {
        printk("I2C config failed\n");
    }

    // Configure IRQ gpio
    if(gpio_is_ready_dt(&pmu_irq) == false)
    {
        printk("PMU IRQ gpio is not ready\n");
    }

    int ret = gpio_pin_configure_dt(&pmu_irq, GPIO_INPUT);
    if (ret != 0)
    {
        printk("Failed to configure PMU IRQ gpio\n");
    }

    bool result = PMU.begin(AXP2101_SLAVE_ADDRESS, pmu_registerReadByte,
                            pmu_registerWriteByte);
    if (result == false)
    {
        while (1)
        {
            printk("PMU is not online...");
            delayMs(500);
        }
    }

    // Set the minimum common working voltage of the PMU VBUS input,
    // below this value will turn off the PMU
    PMU.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_3V88);

    // Set the maximum current of the PMU VBUS input,
    // higher than this value will turn off the PMU
    PMU.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_2000MA);

    // Get the VSYS shutdown voltage
    uint16_t vol = PMU.getSysPowerDownVoltage();
    printk("->  getSysPowerDownVoltage:%u\n", vol);

    // Set VSY off voltage as 2600mV , Adjustment range 2600mV ~ 3300mV
    PMU.setSysPowerDownVoltage(2600);

    // High transmit power may cause voltage dropoff, disable PMU protection
    PMU.disableDC3LowVoltageTurnOff();

    //! DC1 ESP32S3 Core VDD , Don't change
    // PMU.setDC1Voltage(3300);

    //! DC3 Radio & Pixels VDD , Don't change
    PMU.setDC3Voltage(3400);

    //! ALDO2 MICRO TF Card VDD, Don't change
    PMU.setALDO2Voltage(3300);

    //! ALDO4 GNSS VDD, Don't change
    PMU.setALDO4Voltage(3300);

    //! BLDO1 MIC VDD, Don't change
    PMU.setBLDO1Voltage(3300);

    // DC5 IMAX=2A
    // 1200mV
    // 1400~3700mV,100mV/step,24steps
    PMU.setDC5Voltage(3300);

    //ALDO1 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    PMU.setALDO1Voltage(3300);

    //ALDO3 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    PMU.setALDO3Voltage(3300);

    //BLDO2 IMAX=300mA
    //500~3500mV, 100mV/step,31steps
    PMU.setBLDO2Voltage(3300);

    // Turn on the power that needs to be used
    //! DC1 ESP32S3 Core VDD , Don't change
    // PMU.enableDC3();

    //! External pin power supply
    PMU.enableDC5();
    PMU.enableALDO1();
    PMU.enableALDO3();
    PMU.enableBLDO2();

    //! ALDO2 MICRO TF Card VDD
    PMU.enableALDO2();

    //! ALDO4 GNSS VDD
    PMU.disableALDO4();

    //! BLDO1 MIC VDD
    PMU.enableBLDO1();

    //! DC3 Radio & Pixels VDD
    PMU.disableDC3();

    // power off when not in use
    PMU.disableDC2();
    PMU.disableDC4();
    PMU.disableCPUSLDO();
    PMU.disableDLDO1();
    PMU.disableDLDO2();


    printk("DCDC=======================================================================\n");
    printk("DC1  : %s   Voltage:%u mV \n",  PMU.isEnableDC1()  ? "+" : "-", PMU.getDC1Voltage());
    printk("DC2  : %s   Voltage:%u mV \n",  PMU.isEnableDC2()  ? "+" : "-", PMU.getDC2Voltage());
    printk("DC3  : %s   Voltage:%u mV \n",  PMU.isEnableDC3()  ? "+" : "-", PMU.getDC3Voltage());
    printk("DC4  : %s   Voltage:%u mV \n",  PMU.isEnableDC4()  ? "+" : "-", PMU.getDC4Voltage());
    printk("DC5  : %s   Voltage:%u mV \n",  PMU.isEnableDC5()  ? "+" : "-", PMU.getDC5Voltage());
    printk("ALDO=======================================================================\n");
    printk("ALDO1: %s   Voltage:%u mV\n",  PMU.isEnableALDO1()  ? "+" : "-", PMU.getALDO1Voltage());
    printk("ALDO2: %s   Voltage:%u mV\n",  PMU.isEnableALDO2()  ? "+" : "-", PMU.getALDO2Voltage());
    printk("ALDO3: %s   Voltage:%u mV\n",  PMU.isEnableALDO3()  ? "+" : "-", PMU.getALDO3Voltage());
    printk("ALDO4: %s   Voltage:%u mV\n",  PMU.isEnableALDO4()  ? "+" : "-", PMU.getALDO4Voltage());
    printk("BLDO=======================================================================\n");
    printk("BLDO1: %s   Voltage:%u mV\n",  PMU.isEnableBLDO1()  ? "+" : "-", PMU.getBLDO1Voltage());
    printk("BLDO2: %s   Voltage:%u mV\n",  PMU.isEnableBLDO2()  ? "+" : "-", PMU.getBLDO2Voltage());
    printk("===========================================================================\n");

    // Set the time of pressing the button to turn off
    PMU.setPowerKeyPressOffTime(XPOWERS_POWEROFF_10S);
    uint8_t opt = PMU.getPowerKeyPressOffTime();
    printk("PowerKeyPressOffTime:");
    switch (opt)
    {
        case XPOWERS_POWEROFF_4S: printk("4 Second");
            break;
        case XPOWERS_POWEROFF_6S: printk("6 Second");
            break;
        case XPOWERS_POWEROFF_8S: printk("8 Second");
            break;
        case XPOWERS_POWEROFF_10S: printk("10 Second");
            break;
        default:
            break;
    }
    printk("\n");

    // Set the button power-on press time
    PMU.setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    opt = PMU.getPowerKeyPressOnTime();
    printk("PowerKeyPressOnTime:");
    switch (opt)
    {
        case XPOWERS_POWERON_128MS: printk("128 Ms");
            break;
        case XPOWERS_POWERON_512MS: printk("512 Ms");
            break;
        case XPOWERS_POWERON_1S: printk("1 Second");
            break;
        case XPOWERS_POWERON_2S: printk("2 Second");
            break;
        default:
            break;
    }
    printk("\n");

    printk("===========================================================================\n");
    // It is necessary to disable the detection function of the TS pin on the board
    // without the battery temperature detection function, otherwise it will cause abnormal charging
    PMU.disableTSPinMeasure();

    // Enable internal ADC detection
    PMU.enableBattDetection();
    PMU.enableVbusVoltageMeasure();
    PMU.enableBattVoltageMeasure();
    PMU.enableSystemVoltageMeasure();

    /*
      The default setting is CHGLED is automatically controlled by the PMU.
    - XPOWERS_CHG_LED_OFF,
    - XPOWERS_CHG_LED_BLINK_1HZ,
    - XPOWERS_CHG_LED_BLINK_4HZ,
    - XPOWERS_CHG_LED_ON,
    - XPOWERS_CHG_LED_CTRL_CHG,
    * */
    PMU.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

    // TODO: Implement IRQ
    // pinMode(PMU_IRQ, INPUT_PULLUP);
    // attachInterrupt(PMU_IRQ, setFlag, FALLING);

    // Disable all interrupts
    PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    // Clear all interrupt flags
    PMU.clearIrqStatus();
    // Enable the required interrupt function
    PMU.enableIRQ(
        XPOWERS_AXP2101_BAT_INSERT_IRQ    | XPOWERS_AXP2101_BAT_REMOVE_IRQ    |   // BATTERY
        XPOWERS_AXP2101_VBUS_INSERT_IRQ   | XPOWERS_AXP2101_VBUS_REMOVE_IRQ   |   // VBUS
        XPOWERS_AXP2101_PKEY_POSITIVE_IRQ | XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ |   // POWER KEY ON/OFF
        XPOWERS_AXP2101_PKEY_LONG_IRQ     |                                       // POWER KEY LONG PRESS
        XPOWERS_AXP2101_BAT_CHG_DONE_IRQ  | XPOWERS_AXP2101_BAT_CHG_START_IRQ     // CHARGE
    );

    // Set the precharge charging current
    PMU.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_150MA);

    // Set constant current charge current limit
    //! Using inferior USB cables and adapters will not reach the maximum charging current.
    //! Please pay attention to add a suitable heat sink above the PMU when setting the charging current to 1A
    // NOTE: Charging current set to 500mAh to remove the need for a heat sink
    PMU.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);

    // Set stop charging termination current
    PMU.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_150MA);

    // Set charge cut-off voltage
    // NOTE: Target voltage set to 4.00V (80% charge) to extend battery lifespan of 2.5x-3x
    PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V);

    // Enable the PMU long press shutdown function (emergency shutdown)
    PMU.enableLongPressShutdown();


    // Get charging target current
    static const uint16_t currTable[] =
    {
        0, 0, 0, 0, 100, 125, 150, 175, 200, 300, 400, 500, 600, 700, 800, 900, 1000
    };

    uint8_t val = PMU.getChargerConstantCurr();
    printk("Val = %d\n", val);
    printk("Setting Charge Target Current : %d\n", currTable[val]);

    // Get charging target voltage
    static const uint16_t tableVoltage[] =
    {
        0, 4000, 4100, 4200, 4350, 4400, 255
    };

    val = PMU.getChargeTargetVoltage();
    printk("Setting Charge Target Voltage : %d\n", tableVoltage[val]);
}

void pmu_terminate()
{
    PMU.disableDC3();   // Turn off baseband power
    PMU.disableALDO4(); // Turn off GPS power
    PMU.shutdown();     // General shutdown
}

uint16_t pmu_getVbat()
{
    return PMU.isBatteryConnect() ? PMU.getBattVoltage() : 0;
}

void pmu_setBasebandPower(bool enable)
{
    if (enable)
        PMU.enableDC3();
    else
        PMU.disableDC3();
}

void pmu_setGPSPower(bool enable)
{
    if (enable)
        PMU.enableALDO4();
    else
        PMU.disableALDO4();
}

void pmu_handleIRQ()
{
    // Check if we got some interrupts
    if(gpio_pin_get_dt(&pmu_irq) == 0)
        return;

    uint32_t irqStatus = PMU.getIrqStatus();
    PMU.clearIrqStatus();

    // Power on key rising edge
    if((irqStatus & XPOWERS_AXP2101_PKEY_POSITIVE_IRQ) != 0)
        pwrOnPressed = 0;

    // Power on key falling edge
    if((irqStatus & XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ) != 0)
        pwrOnPressed = 1;

    // Power key long press
    if ((irqStatus & XPOWERS_AXP2101_PKEY_LONG_IRQ) != 0)
        pwrOnPressed = 2;

    // Charger start IRQ
    if((irqStatus & XPOWERS_AXP2101_BAT_CHG_START_IRQ) != 0)
    {
        if(PMU.isBatteryConnect())
            PMU.setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
    }

    // Battery remove IRQ
    if((irqStatus & XPOWERS_AXP2101_BAT_REMOVE_IRQ) != 0)
        PMU.setChargingLedMode(XPOWERS_CHG_LED_BLINK_1HZ);
}

uint8_t pmu_pwrBtnStatus()
{
    return pwrOnPressed;
}
