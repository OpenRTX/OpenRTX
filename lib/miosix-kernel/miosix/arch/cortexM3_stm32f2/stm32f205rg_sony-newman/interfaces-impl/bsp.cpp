/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/ 

/***********************************************************************
* bsp.cpp Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#include <cstdlib>
#include "interfaces/bsp.h"
#include "kernel/kernel.h"
#include "interfaces/delays.h"
#include "interfaces/portability.h"
#include "interfaces/arch_registers.h"
#include "interfaces/os_timer.h"
#include "config/miosix_settings.h"
#include <algorithm>

using namespace std;

namespace miosix {

//
// Initialization
//

void IRQbspInit()
{
    //Disable all interrupts that the bootloader 
    NVIC->ICER[0]=0xffffffff;
    NVIC->ICER[1]=0xffffffff;
    NVIC->ICER[2]=0xffffffff;
    NVIC->ICER[3]=0xffffffff;
    //Enable all gpios
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN
                  | RCC_AHB1ENR_GPIOBEN
                  | RCC_AHB1ENR_GPIOCEN;
    RCC_SYNC();
    using namespace oled;
    OLED_nSS_Pin::mode(Mode::OUTPUT);
    OLED_nSS_Pin::high();
    OLED_nSS_Pin::speed(Speed::_50MHz); //Without changing the default speed
    OLED_SCK_Pin::mode(Mode::ALTERNATE); //OLED does not work!
    OLED_SCK_Pin::alternateFunction(5);
    OLED_SCK_Pin::speed(Speed::_50MHz);
    OLED_MOSI_Pin::mode(Mode::ALTERNATE);
    OLED_MOSI_Pin::alternateFunction(5);
    OLED_MOSI_Pin::speed(Speed::_50MHz);
    OLED_A0_Pin::mode(Mode::OUTPUT);
    OLED_A0_Pin::low();
    OLED_A0_Pin::speed(Speed::_50MHz);
    OLED_Reset_Pin::mode(Mode::OUTPUT);
    OLED_Reset_Pin::low();
    OLED_Reset_Pin::speed(Speed::_50MHz);
    OLED_V_ENABLE_Pin::mode(Mode::OUTPUT);
    OLED_V_ENABLE_Pin::low();
    OLED_V_ENABLE_Pin::speed(Speed::_50MHz);
    
    using namespace touch;
    Touch_Reset_Pin::mode(Mode::OUTPUT);
    Touch_Reset_Pin::low();
    Touch_Reset_Pin::speed(Speed::_50MHz);
    TOUCH_WKUP_INT_Pin::mode(Mode::INPUT);
    
    using namespace power;
    BATT_V_ON_Pin::mode(Mode::OUTPUT);
    BATT_V_ON_Pin::low();
    BATT_V_ON_Pin::speed(Speed::_50MHz);
    BAT_V_Pin::mode(Mode::INPUT_ANALOG);
    ENABLE_LIGHT_SENSOR_Pin::mode(Mode::OUTPUT);
    ENABLE_LIGHT_SENSOR_Pin::low();
    ENABLE_LIGHT_SENSOR_Pin::speed(Speed::_50MHz);
    LIGHT_SENSOR_ANALOG_OUT_Pin::mode(Mode::INPUT_ANALOG);
    ENABLE_2V8_Pin::mode(Mode::OUTPUT);
    ENABLE_2V8_Pin::low();
    ENABLE_2V8_Pin::speed(Speed::_50MHz);
    HoldPower_Pin::mode(Mode::OPEN_DRAIN);
    HoldPower_Pin::high();
    HoldPower_Pin::speed(Speed::_50MHz);
    
    ACCELEROMETER_INT_Pin::mode(Mode::INPUT_PULL_DOWN);
    
    using namespace i2c;
    I2C_SCL_Pin::speed(Speed::_50MHz);
    I2C_SDA_Pin::speed(Speed::_50MHz);
    
    BUZER_PWM_Pin::mode(Mode::OUTPUT);
    BUZER_PWM_Pin::low();
    BUZER_PWM_Pin::speed(Speed::_50MHz);
    
    POWER_BTN_PRESS_Pin::mode(Mode::INPUT);
    
    using namespace usb;
    USB5V_Detected_Pin::mode(Mode::INPUT_PULL_DOWN);
    USB_DM_Pin::mode(Mode::INPUT);
    USB_DP_Pin::mode(Mode::INPUT);
    
    using namespace bluetooth;
    Reset_BT_Pin::mode(Mode::OPEN_DRAIN);
    Reset_BT_Pin::low();
    Reset_BT_Pin::speed(Speed::_50MHz);
    BT_CLK_REQ_Pin::mode(Mode::INPUT);
    HOST_WAKE_UP_Pin::mode(Mode::INPUT);
    Enable_1V8_BT_Power_Pin::mode(Mode::OPEN_DRAIN);
    Enable_1V8_BT_Power_Pin::high();
    Enable_1V8_BT_Power_Pin::speed(Speed::_50MHz);
    BT_nSS_Pin::mode(Mode::OUTPUT);
    BT_nSS_Pin::low();
    BT_nSS_Pin::speed(Speed::_50MHz);
    BT_SCK_Pin::mode(Mode::OUTPUT);
    BT_SCK_Pin::low();
    BT_SCK_Pin::speed(Speed::_50MHz);
    BT_MISO_Pin::mode(Mode::INPUT_PULL_DOWN);
    BT_MOSI_Pin::mode(Mode::OUTPUT);
    BT_MOSI_Pin::low();
    BT_MOSI_Pin::speed(Speed::_50MHz);
    
    using namespace unknown;
    WKUP_Pin::mode(Mode::INPUT);
    MCO1_Pin::mode(Mode::ALTERNATE);
    MCO1_Pin::alternateFunction(0);
    MCO1_Pin::speed(Speed::_100MHz);
    Connect_USB_Pin::mode(Mode::OPEN_DRAIN);
    Connect_USB_Pin::low();
    Connect_USB_Pin::speed(Speed::_50MHz);
    POWER_3V3_ON_1V8_OFF_Pin::mode(Mode::OUTPUT);
    POWER_3V3_ON_1V8_OFF_Pin::low();
    POWER_3V3_ON_1V8_OFF_Pin::speed(Speed::_50MHz);
    SPI2_nSS_Pin::mode(Mode::OUTPUT);
    SPI2_nSS_Pin::high();
    SPI2_nSS_Pin::speed(Speed::_50MHz);
    SPI2_SCK_Pin::mode(Mode::OUTPUT);
    SPI2_SCK_Pin::low();
    SPI2_SCK_Pin::speed(Speed::_50MHz);
    SPI2_MISO_Pin::mode(Mode::INPUT_PULL_DOWN);
    SPI2_MOSI_Pin::mode(Mode::OUTPUT);
    SPI2_MOSI_Pin::low();
    SPI2_MOSI_Pin::speed(Speed::_50MHz);
    
    // Taken from underverk's SmartWatch_Toolchain/src/system.c:
    // Prevents hard-faults when booting from USB
    delayMs(50);

    USB_DP_Pin::mode(Mode::INPUT_PULL_UP); //Never leave GPIOs floating
    USB_DM_Pin::mode(Mode::INPUT_PULL_DOWN);
}

void bspInit2()
{
    PowerManagement::instance(); //This initializes the PMU
    BUZER_PWM_Pin::high();
    Thread::sleep(200);
    BUZER_PWM_Pin::low();
    //Wait for user to release the button
    while(POWER_BTN_PRESS_Pin::value()) Thread::sleep(20);
}

//
// Shutdown and reboot
//

void shutdown()
{
    // Taken from underverk's SmartWatch_Toolchain/src/Arduino/Arduino.cpp
    disableInterrupts();
    BUZER_PWM_Pin::high();
    delayMs(200);
    BUZER_PWM_Pin::low();
    while(POWER_BTN_PRESS_Pin::value()) ;
    //This is likely wired to the PMU. If the USB cable is not connected, this
    //cuts off the power to the microcontroller. But if USB is connected, this
    //does nothing. In this case we can only spin waiting for the user to turn
    //the device on again
    power::HoldPower_Pin::low();
    delayMs(500);
    while(POWER_BTN_PRESS_Pin::value()==0) ;
    reboot();
}

void reboot()
{
    disableInterrupts();
    miosix_private::IRQsystemReboot();
}

//
// Other board specific stuff
//

FastMutex& i2cMutex()
{
    static FastMutex mutex;
    return mutex;
}

bool i2cWriteReg(miosix::I2C1Master *i2c, unsigned char dev, unsigned char reg,
        unsigned char data)
{
    const unsigned char buffer[]={reg,data};
    return i2c->send(dev,buffer,sizeof(buffer));
}

bool i2cReadReg(miosix::I2C1Master *i2c, unsigned char dev, unsigned char reg,
        unsigned char& data)
{
    if(i2c->send(dev,&reg,1)==false) return false;
    unsigned char temp;
    if(i2c->recv(dev,&temp,1)==false) return false;
    data=temp;
    return true;
}

void errorMarker(int x)
{
    Thread::sleep(400);
    for(int i=0;i<x;i++)
    {
        BUZER_PWM_Pin::high();
        Thread::sleep(100);
        BUZER_PWM_Pin::low();
        Thread::sleep(400);
    }
}

void IRQerrorMarker(int x)
{
    delayMs(400);
    for(int i=0;i<x;i++)
    {
        BUZER_PWM_Pin::high();
        delayMs(100);
        BUZER_PWM_Pin::low();
        delayMs(400);
    }
}

//As usual, since the PMU datasheet is unavailable (we don't even know what
//chip it is), these are taken from underverk's code
#define CHGSTATUS               0x01

#define CH_ACTIVE_MSK           0x08

#define CHGCONFIG0              0x02

#define VSYS_4_4V               0x40
#define VSYS_5V                 0x80
#define ACIC_100mA_DPPM_ENABLE  0x00
#define ACIC_500mA_DPPM_ENABLE  0x10
#define ACIC_500mA_DPPM_DISABLE 0x20
#define ACIC_USB_SUSPEND        0x20
#define TH_LOOP                 0x08
#define DYN_TMR                 0x04
#define TERM_EN                 0x02
#define CH_EN                   0x01

#define CHGCONFIG1              0x03
#define I_PRE_05                0x00
#define I_PRE_10                0x40
#define I_PRE_15                0x80
#define I_PRE_20                0xC0

#define DEFDCDC                 0x07
#define DCDC1_DEFAULT           0x29
#define DCDC_DISCH              0x40
#define HOLD_DCDC1              0x80

#define ISET_25                 0x00
#define ISET_50                 0x10
#define ISET_75                 0x20
#define ISET_100                0x30

#define I_TERM_05               0x00
#define I_TERM_10               0x04
#define I_TERM_15               0x08
#define I_TERM_20               0x0C

#define CHGCONFIG2              0x04
#define SFTY_TMR_4h             0x0
#define SFTY_TMR_5h             0x40
#define SFTY_TMR_6h             0x80
#define SFTY_TMR_8h             0xC0

#define PRE_TMR_30m             0x0
#define PRE_TMR_60m             0x20

#define NTC_100k                0x0
#define NTC_10k                 0x8

#define V_DPPM_VBAT_100mV       0x0
#define V_DPPM_4_3_V            0x04

#define VBAT_COMP_ENABLE        0x02
#define VBAT_COMP_DISABLE       0x00

//
// class PowerManagement
//

PowerManagement& PowerManagement::instance()
{
    static PowerManagement singleton;
    return singleton;
}

bool PowerManagement::isUsbConnected() const
{
    return usb::USB5V_Detected_Pin::value();
}

bool PowerManagement::isCharging()
{
    if(isUsbConnected()==false) return false;
    Lock<FastMutex> l(i2cMutex());
    unsigned char chgstatus;
    //During testing the i2c command never failed. If it does, we lie and say
    //we're not charging
    if(i2cReadReg(i2c,PMU_I2C_ADDRESS,CHGSTATUS,chgstatus)==false) return false;
    return (chgstatus & CH_ACTIVE_MSK)!=0;
}

int PowerManagement::getBatteryStatus()
{
    const int battCharged=4000; //4.0V
    const int battDead=3000; //3V
    return max(0,min(100,(getBatteryVoltage()-battDead)*100/(battCharged-battDead)));
}

int PowerManagement::getBatteryVoltage()
{
    Lock<FastMutex> l(powerManagementMutex);
    power::BATT_V_ON_Pin::high(); //Enable battry measure circuitry
    ADC1->CR2=ADC_CR2_ADON; //Turn ADC ON
    Thread::sleep(5); //Wait for voltage to stabilize
    ADC1->CR2 |= ADC_CR2_SWSTART; //Start conversion
    while((ADC1->SR & ADC_SR_EOC)==0) ; //Wait for conversion
    int result=ADC1->DR; //Read result
    ADC1->CR2=0; //Turn ADC OFF
    power::BATT_V_ON_Pin::low(); //Disable battery measurement circuitry
    return result*4440/4095;
}

void PowerManagement::setCoreFrequency(CoreFrequency cf)
{
    if(cf==coreFreq) return;
    
    Lock<FastMutex> l(powerManagementMutex);
    //We need to reconfigure I2C for the new frequency 
    Lock<FastMutex> l2(i2cMutex());
    
    {
        FastInterruptDisableLock dLock;
        CoreFrequency oldCoreFreq=coreFreq;
        coreFreq=cf; //Need to change this *before* setting prescalers/core freq
        if(coreFreq>oldCoreFreq)
        {
            //We're increasing the frequency, so change prescalers first
            IRQsetPrescalers();
            IRQsetCoreFreq();
        } else {
            //We're decreasing the frequency, so change frequency first
            IRQsetCoreFreq();
            IRQsetPrescalers();
        }
        
        //Changing frequency requires to change many things that depend on
        //said frequency:
        
        //Miosix's os timer
        SystemCoreClockUpdate();
        internal::IRQosTimerInit();
    }
    
    //And also reconfigure the I2C (can't change this with IRQ disabled)
    //Reinitialize after the frequency change
    delete i2c;
    i2c=new I2C1Master(i2c::I2C_SDA_Pin::getPin(),i2c::I2C_SCL_Pin::getPin(),100);
}

void PowerManagement::goDeepSleep(int ms)
{
    ms=min(30000,ms);
    /*
     * Going in deep sleep would interfere with USB communication. Also,
     * there's no need for such an aggressive power optimization while we are
     * connected to USB.
     */
    if(isUsbConnected())
    {
        if(wakeOnButton)
        {
            bool oldState=POWER_BTN_PRESS_Pin::value();
            for(int i=0;i<ms;i++)
            {
                Thread::sleep(1);
                bool newState=POWER_BTN_PRESS_Pin::value();
                //Imitate edge detection, as the EXTI line would do
                if(newState && !oldState) return;
                oldState=newState;
            }
        } else Thread::sleep(ms);
        return;
    }
    
    Lock<FastMutex> l(powerManagementMutex);
    //We don't use I2C, but we don't want other thread to mess with
    //the hardware while the microcontroller is going in deep sleep
    Lock<FastMutex> l2(i2cMutex());
    
    {
        FastInterruptDisableLock dLock;
        //Enable event 22 (RTC WKUP)
        if(wakeOnButton)
        {
            EXTI->EMR |= 1<<11;
            EXTI->RTSR |= 1<<11;
        } else {
            EXTI->EMR &= ~(1<<11);
            EXTI->RTSR &= ~(1<<11);
        }
        EXTI->EMR |= 1<<22;
        EXTI->RTSR |= 1<<22;
        
        //These two values enable RTC write access
        RTC->WPR=0xca;
        RTC->WPR=0x53;
        //Set wakeup time
        RTC->CR &= ~RTC_CR_WUTE;
        while((RTC->ISR & RTC_ISR_WUTWF)==0) ;
        RTC->CR &= ~ RTC_CR_WUCKSEL; //timebase=32768/16=1024
        RTC->WUTR=ms*2048/1000;
        RTC->CR |= RTC_CR_WUTE | RTC_CR_WUTIE;
        
        //Enter stop mode by issuing a WFE
        PWR->CR |= PWR_CR_FPDS  //Flash in power down while in stop
                 | PWR_CR_LPDS; //Regulator in low power mode while in stop
        SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; //Select stop mode
        __WFE();
        SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; //Unselect stop mode
        
        //Disable wakeup timer
        RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);
        RTC->ISR &= ~RTC_ISR_WUTF; //~because these flags are cleared writing 0
        
        //After stop mode the microcontroller clock settings are lost, we are
        //running with the HSI oscillator, so restart the PLL
        IRQsetSystemClock();
    }
}

PowerManagement::PowerManagement() : i2c(new I2C1Master(i2c::I2C_SDA_Pin::getPin(),
        i2c::I2C_SCL_Pin::getPin(),100)), chargingAllowed(true), wakeOnButton(false),
        coreFreq(FREQ_120MHz), powerManagementMutex(FastMutex::RECURSIVE)
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_SYSCFGEN;
        RCC_SYNC();
        //Configure PB1 (POWER_BUTTON) as EXTI input
        SYSCFG->EXTICR[2] &= ~(0xf<<12);
        SYSCFG->EXTICR[2] |= 1<<12;
        //Then disable SYSCFG access, as we don't need it anymore
        RCC->APB2ENR &= ~RCC_APB2ENR_SYSCFGEN;
    }
    ADC1->CR1=0;
    ADC1->CR2=0; //Keep the ADC OFF to save power
    ADC1->SMPR2=ADC_SMPR2_SMP2_0; //Sample for 15 cycles channel 2 (battery)
    ADC1->SQR1=0; //Do only one conversion
    ADC1->SQR2=0;
    ADC1->SQR3=2; //Convert channel 2 (battery voltage)
    
    unsigned char config0=VSYS_4_4V
                        | ACIC_100mA_DPPM_ENABLE
                        | TH_LOOP
                        | DYN_TMR
                        | TERM_EN
                        | CH_EN;
    unsigned char config1=I_TERM_10
                        | ISET_100
                        | I_PRE_10;
    unsigned char config2=SFTY_TMR_5h
                        | PRE_TMR_30m
                        | NTC_10k
                        | V_DPPM_4_3_V
                        | VBAT_COMP_ENABLE;
    unsigned char defdcdc=DCDC_DISCH
                        | DCDC1_DEFAULT;
    Lock<FastMutex> l(i2cMutex());
    bool error=false;
    if(i2cWriteReg(i2c,PMU_I2C_ADDRESS,CHGCONFIG0,config0)==false) error=true;
    if(i2cWriteReg(i2c,PMU_I2C_ADDRESS,CHGCONFIG1,config1)==false) error=true;
    if(i2cWriteReg(i2c,PMU_I2C_ADDRESS,CHGCONFIG2,config2)==false) error=true;
    if(i2cWriteReg(i2c,PMU_I2C_ADDRESS,DEFDCDC,defdcdc)==false) error=true;
    if(error) errorMarker(10); //Should never happen
    Rtc::instance(); //Sleep stuff depends on RTC, so it must be initialized
}

void PowerManagement::IRQsetSystemClock()
{
    //Turn on HSE and wait for it to stabilize
    RCC->CR |= RCC_CR_HSEON;
    while((RCC->CR & RCC_CR_HSERDY)==0) ;

    //Configure PLL and turn it on
    const int m=HSE_VALUE/1000000;
    const int n=240;
    const int p=2;
    const int q=5;
    RCC->PLLCFGR=m | (n<<6) | (((p/2)-1)<<16) | RCC_PLLCFGR_PLLSRC_HSE | (q<<24);
    RCC->CR |= RCC_CR_PLLON;
    while((RCC->CR & RCC_CR_PLLRDY)==0) ;
    
    IRQsetPrescalers();
    IRQsetCoreFreq();
}

void PowerManagement::IRQsetPrescalers()
{
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    switch(coreFreq)
    {
        case FREQ_120MHz:
            RCC->CFGR |= RCC_CFGR_HPRE_DIV1;  //HCLK=SYSCLK
            RCC->CFGR |= RCC_CFGR_PPRE2_DIV2; //PCLK2=HCLK/2
            RCC->CFGR |= RCC_CFGR_PPRE1_DIV4; //PCLK1=HCLK/4
            //Configure flash wait states
            //Three wait states seem to make it unstable (crashing) when CPU load is high
            FLASH->ACR=FLASH_ACR_PRFTEN
                     | FLASH_ACR_ICEN
                     | FLASH_ACR_DCEN
                     | FLASH_ACR_LATENCY_7WS;
            break;
        case FREQ_26MHz:
            RCC->CFGR |= RCC_CFGR_HPRE_DIV1;  //HCLK=SYSCLK
            RCC->CFGR |= RCC_CFGR_PPRE2_DIV1; //PCLK2=HCLK
            RCC->CFGR |= RCC_CFGR_PPRE1_DIV1; //PCLK1=HCLK
            //Configure flash wait states
            FLASH->ACR=FLASH_ACR_PRFTEN
                     | FLASH_ACR_ICEN
                     | FLASH_ACR_DCEN
                     | FLASH_ACR_LATENCY_1WS;
            break;
    }
}

void PowerManagement::IRQsetCoreFreq()
{
    //Note that we don't turn OFF the PLL when going to 26MHz. It's true, it
    //draws power, but the USB and RNG use it so for now we'll be on the safe
    //side and keep in active
    RCC->CFGR &= ~(RCC_CFGR_SW);
    switch(coreFreq)
    {
        case FREQ_120MHz:
            RCC->CFGR |= RCC_CFGR_SW_PLL;
            while((RCC->CFGR & RCC_CFGR_SWS)!=RCC_CFGR_SWS_PLL) ;
            break;
        case FREQ_26MHz:
            RCC->CFGR |= RCC_CFGR_SW_HSE;
            while((RCC->CFGR & RCC_CFGR_SWS)!=RCC_CFGR_SWS_HSE) ;
            break;
    }
}

//
// class LightSensor
//

LightSensor& LightSensor::instance()
{
    static LightSensor singleton;
    return singleton;
}

int LightSensor::read()
{
    //Prevent frequency changes/entering deep sleep while reading light sensor
    Lock<PowerManagement> l(PowerManagement::instance());
    
    power::ENABLE_LIGHT_SENSOR_Pin::high(); //Enable battry measure circuitry
    ADC2->CR2=ADC_CR2_ADON; //Turn ADC ON
    Thread::sleep(5); //Wait for voltage to stabilize
    ADC2->CR2 |= ADC_CR2_SWSTART; //Start conversion
    while((ADC2->SR & ADC_SR_EOC)==0) ; //Wait for conversion
    int result=ADC2->DR; //Read result
    ADC2->CR2=0; //Turn ADC OFF
    power::ENABLE_LIGHT_SENSOR_Pin::low(); //Disable battery measure circuitry
    return result;
}

LightSensor::LightSensor()
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
        RCC_SYNC();
    }
    ADC2->CR1=0;
    ADC2->CR2=0; //Keep the ADC OFF to save power
    ADC2->SMPR1=ADC_SMPR1_SMP14_0; //Sample for 15 cycles channel 14
    ADC2->SQR1=0; //Do only one conversion
    ADC2->SQR2=0;
    ADC2->SQR3=14; //Convert channel 14 (light sensor)
}

//
// class Rtc
//

Rtc& Rtc::instance()
{
    static Rtc singleton;
    return singleton;
}

struct tm Rtc::getTime()
{
    while((RTC->ISR & RTC_ISR_RSF)==0) ; //Wait for registers to sync
    unsigned int t,d;
    for(;;)
    {
        t=RTC->TR;
        d=RTC->DR;
        if(t==RTC->TR) break;
        //Otherwise the registers were updated while reading and may not
        //reflect the same time instant, so retry
    }
    struct tm result;
    #define BCD(x,y,z) (((x)>>(y)) & 0xf) + (((x)>>((y)+4)) & (z))*10
    result.tm_sec=BCD(t,0,0x7);
    result.tm_min=BCD(t,8,0x7);
    result.tm_hour=BCD(t,16,0x7);
    result.tm_mday=BCD(d,0,0x7);
    result.tm_mon=BCD(d,8,0x1)-1; //-1 as tm_mon's range is 0..11
    //RTC has only two digits for year, and struct tm counts year from 1900
    result.tm_year=BCD(d,16,0xf)+100;
    int wdu=(d>>13) & 0x7;
    result.tm_wday= (wdu>6) ? 0 : wdu; //Sunday is 0 for struct tm, 7 for RTC
    result.tm_yday=0; //TODO
    result.tm_isdst=0; //TODO
    #undef BCD
    return result;
}

void Rtc::setTime(tm time)
{
    time.tm_sec=min(59,time.tm_sec);
    time.tm_min=min(59,time.tm_min);
    time.tm_hour=min(23,time.tm_hour);
    time.tm_mday=max(1,min(31,time.tm_mday));
    time.tm_mon=max(1,min(12,time.tm_mon));
    time.tm_wday=min(6,time.tm_wday);
    int wdu= (time.tm_wday==0) ? 7 : time.tm_wday; //Sunday is 0 for struct tm, 7 for RTC
    unsigned int t,d;
    #define BCD(x,y,v) (x)|=(((v) % 10)<<(y) | ((v)/10)<<((y)+4)) 
    t=0;
    BCD(t,0,time.tm_sec);
    BCD(t,8,time.tm_min);
    BCD(t,16,time.tm_hour);
    d=0;
    BCD(d,0,time.tm_mday);
    BCD(d,8,time.tm_mon+1); //+1 as tm_mon's range is 0..11
    //RTC has only two digits for year, and struct tm counts year from 1900
    BCD(d,16,time.tm_year-100);
    d|=wdu<<13;
    #undef BCD
    
    //Prevent frequency changes/entering deep sleep while setting time
    Lock<PowerManagement> l(PowerManagement::instance());
    
    //These two values enable RTC write access
    RTC->WPR=0xca;
    RTC->WPR=0x53;
    RTC->ISR |= RTC_ISR_INIT;
    while((RTC->ISR & RTC_ISR_INITF)==0) ; //Wait to enter writable mode
    RTC->TR=t;
    RTC->DR=d;
    RTC->ISR &= ~RTC_ISR_INIT;
}

bool Rtc::notSetYet() const
{
    return (RTC->ISR & RTC_ISR_INITS)==0;
}

Rtc::Rtc()
{
    {
        FastInterruptDisableLock dLock;
        RCC->APB1ENR |= RCC_APB1ENR_PWREN;
        RCC_SYNC();
        PWR->CR |= PWR_CR_DBP;         //Enable access to RTC registers
        
        //Without this reset, the LSEON bit is ignored, and code hangs at while
        //However, this resets the RTC time. An alternative is to write many
        //times the LSEON, RTCEN and RTCSEL_0 bits until they're set, but sadly
        //RTC time is still not kept. TODO: fix this is possible
        RCC->BDCR=RCC_BDCR_BDRST;
        RCC->BDCR=0;
        
        RCC->BDCR=RCC_BDCR_LSEON       //External 32KHz oscillator enabled
                | RCC_BDCR_RTCEN       //RTC enabled
                | RCC_BDCR_RTCSEL_0;   //Select LSE as clock source for RTC
    }
    
    while((RCC->BDCR & RCC_BDCR_LSERDY)==0) ; //Wait
}

} //namespace miosix
