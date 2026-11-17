/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <kernel/scheduler/scheduler.h>
#include "core/cache_cortexMx.h"
#include "interfaces/delays.h"
#include "peripherals/gpio.h"
#include "DmaStream.hpp"
#include "hwconfig.h"
#include <miosix.h>
#include <errno.h>
#include "stm32_adc.h"

#if defined(STM32H743xx)
#include "Lptim.hpp"

typedef Lptim          timebase_type;
#define TimebaseCh1    Lptim(LPTIM3_BASE, 168000000)
#define TimebaseCh2    Lptim(LPTIM3_BASE, 168000000)
#define TimebaseCh3    Lptim(LPTIM3_BASE, 168000000)
#else
#include "Timer.hpp"

typedef Timer          timebase_type;
#define TimebaseCh1    Timer(TIM2_BASE)
#define TimebaseCh2    Timer(TIM2_BASE)
#define TimebaseCh3    Timer(TIM2_BASE)
#endif

struct AdcPeriph
{
    ADC_TypeDef   *adc;
    StreamHandler *stream;
    timebase_type  tim;
};

using Dma2_Stream0 = DmaStream< DMA2_BASE, 0, 0, 2 >; // DMA 2, Stream 2, channel 0, high priority (ADC1)
using Dma2_Stream1 = DmaStream< DMA2_BASE, 1, 2, 2 >; // DMA 2, Stream 2, channel 2, high priority (ADC3)
using Dma2_Stream2 = DmaStream< DMA2_BASE, 2, 1, 2 >; // DMA 2, Stream 2, channel 1, high priority (ADC2)

StreamHandler Dma2_Stream0_hdl = Dma2_Stream0::init(10, DataSize::_16BIT, false);
StreamHandler Dma2_Stream1_hdl = Dma2_Stream1::init(10, DataSize::_16BIT, false);
StreamHandler Dma2_Stream2_hdl = Dma2_Stream2::init(10, DataSize::_16BIT, false);

static struct streamCtx *AdcContext[3];

static constexpr AdcPeriph periph[] =
{
    {(ADC_TypeDef *) ADC1_BASE, &Dma2_Stream0_hdl, TimebaseCh1},
    {(ADC_TypeDef *) ADC2_BASE, &Dma2_Stream2_hdl, TimebaseCh2},
    {(ADC_TypeDef *) ADC3_BASE, &Dma2_Stream1_hdl, TimebaseCh3},
};


#ifdef STM32H743xx
/*
 * The H7's ADC converts at 16 bit and oversamples in hardware: 16 conversions
 * per trigger, summed and shifted right by 5, a 15 bit sample that fits the
 * signed 16 bit audio samples. At most 16 x (16.5 + 8.5) = 400 ADC clocks per
 * trigger: 16 us with the 25 MHz ADC clock (HCLK / 4, halved on rev V; 8 us at
 * 50 MHz on rev Y), inside the 20.8 us period at 48 kHz, the fastest stream.
 *
 * RM0433 (STM32H743 reference manual), ADC chapter, registers ADC_CFGR (RES),
 * ADC_CFGR2 (ROVSE, OVSR = ratio - 1, OVSS = right shift) and ADC_SMPRx (SMP),
 * https://www.st.com/resource/en/reference_manual/rm0433-stm32h742-stm32h743753-and-stm32h750-value-line-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
 * Minimum sampling time against source impedance: DS12110 (STM32H743xI
 * datasheet), "ADC characteristics",
 * https://www.st.com/resource/en/datasheet/stm32h743zi.pdf
 * The technique: ST AN5537, "How to use ADC oversampling techniques to
 * improve signal-to-noise ratio on STM32 MCUs".
 */
static constexpr uint32_t OVS_RATIO = 16;
static constexpr uint32_t OVS_SHIFT = 5;
// SMP = 0b011: 16.5 ADC clock cycles
static constexpr uint32_t SMP_16_5 = 3;
static constexpr uint32_t SMPR_FIELDS = 10;

/**
 * \internal
 * Build an SMPRx value with the same sampling time in each of its SMP fields.
 *
 * @param smp: SMP field value.
 * @return the SMPRx register value.
 */
static constexpr uint32_t smprAll(const uint32_t smp)
{
    uint32_t smpr = 0;
    for(uint32_t i = 0; i < SMPR_FIELDS; i++)
        smpr |= smp << (i * ADC_SMPR1_SMP1_Pos);

    return smpr;
}
#endif

/**
 * \internal
 * Stop an ongoing transfer, deactivating timers and DMA stream.
 */
static void stopTransfer(const uint8_t instNum)
{
    ADC_TypeDef *adc = periph[instNum].adc;

    #ifdef STM32H743xx
    adc->CR |= ADC_CR_ADSTP;
    while((adc->CR & ADC_CR_ADSTP) != 0) ;
    #else
    adc->CR2 &= ~ADC_CR2_ADON;
    #endif

    periph[instNum].tim.stop();
    AdcContext[instNum]->running = 0;
}

/**
 * \internal
 * Actual implementation of DMA interrupt handler.
 */
void __attribute__((used)) ADC_DMA_Handler(uint32_t instNum)
{
    switch(instNum)
    {
        case 0:
            Dma2_Stream0::IRQhandleInterrupt(periph[instNum].stream);
            break;

        case 1:
            Dma2_Stream2::IRQhandleInterrupt(periph[instNum].stream);
            break;

        case 2:
            Dma2_Stream1::IRQhandleInterrupt(periph[instNum].stream);
            break;
    }
}

// DMA 2, Stream 0: data transfer for ADC1
void __attribute__((used)) DMA2_Stream0_IRQHandler()
{
    saveContext();
    asm volatile("mov r0, #0");
    asm volatile("bl _Z15ADC_DMA_Handlerm");
    restoreContext();
}

// DMA 2, Stream 2: data transfer for ADC2
void __attribute__((used)) DMA2_Stream2_IRQHandler()
{
    saveContext();
    asm volatile("mov r0, #1");
    asm volatile("bl _Z15ADC_DMA_Handlerm");
    restoreContext();
}

// DMA 2, Stream 1: data transfer for ADC3
void __attribute__((used)) DMA2_Stream1_IRQHandler()
{
    saveContext();
    asm volatile("mov r0, #2");
    asm volatile("bl _Z15ADC_DMA_Handlerm");
    restoreContext();
}



void stm32adc_init(const uint8_t instance)
{
    // Enable peripherals
    switch(instance)
    {
        case STM32_ADC_ADC1:
            #ifdef STM32H743xx
            RCC->AHB1ENR     |= RCC_AHB1ENR_ADC12EN;
            ADC12_COMMON->CCR = ADC_CCR_CKMODE_1
                              | ADC_CCR_CKMODE_0;
            DMAMUX1_Channel8->CCR = 9;
            #else
            RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
            #endif
            break;

        case STM32_ADC_ADC2:
            #ifdef STM32H743xx
            RCC->AHB1ENR     |= RCC_AHB1ENR_ADC12EN;
            ADC12_COMMON->CCR = ADC_CCR_CKMODE_1
                              | ADC_CCR_CKMODE_0;
            DMAMUX1_Channel10->CCR = 10;
            #else
            RCC->APB2ENR |= RCC_APB2ENR_ADC2EN;
            #endif
            break;

        case STM32_ADC_ADC3:
            #ifdef STM32H743xx
            RCC->AHB4ENR    |= RCC_AHB4ENR_ADC3EN;
            ADC3_COMMON->CCR = ADC_CCR_CKMODE_1
                             | ADC_CCR_CKMODE_0;
            DMAMUX1_Channel9->CCR = 115;
            #else
            RCC->APB2ENR |= RCC_APB2ENR_ADC3EN;
            #endif
            break;
    }

    /*
     * Use TIM2 as trigger source for all the ADCs.
     * TODO: change this with a dedicated timer for each ADC.
     */
    #ifdef STM32H743xx
    RCC->APB4ENR |= RCC_APB4ENR_LPTIM3EN;
    RCC->D3CCIPR |= RCC_D3CCIPR_LPTIM345SEL_0;
    uint32_t adcTrig = 20 << ADC_CFGR_EXTSEL_Pos;    // lptim3_out as trig. source
    #else
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    ADC->CCR     |= ADC_CCR_ADCPRE_0;
    uint32_t adcTrig = ADC_CR2_EXTSEL_2     // 0b0110 TIM2_TRGO trig. source
                     | ADC_CR2_EXTSEL_1;
    #endif
    __DSB();

    ADC_TypeDef *adc = periph[instance].adc;

    #ifdef STM32H743xx
    adc->CR = ADC_CR_ADVREGEN
            | ADC_CR_BOOST_1
            | ADC_CR_BOOST_0;

    while((adc->ISR & ADC_ISR_LDORDY) == 0) ;

    // Calibrate offset and linearity before enabling, as adc_stm32h7.c does
    adc->CR |= ADC_CR_ADCAL
            |  ADC_CR_ADCALLIN;
    while((adc->CR & ADC_CR_ADCAL) != 0) ;

    /*
     * Enable the ADC and wait for ADRDY. ADEN set less than 4 ADC clock
     * cycles after a calibration ends is cleared again by the calibration
     * logic, so keep setting it until the ADC is ready: ST's workaround in
     * ADC_Enable(), STM32CubeH7 HAL v1.11.7,
     * https://github.com/STMicroelectronics/stm32h7xx-hal-driver/blob/v1.11.7/Src/stm32h7xx_hal_adc.c#L3709-L3724
     */
    adc->ISR   = ADC_ISR_ADRDY;     // Clear the ADRDY flag
    adc->CR   |= ADC_CR_ADEN;
    while((adc->ISR & ADC_ISR_ADRDY) == 0)
    {
        if((adc->CR & ADC_CR_ADEN) == 0)
            adc->CR |= ADC_CR_ADEN;
    }

    adc->SMPR2 = smprAll(SMP_16_5);
    adc->SMPR1 = smprAll(SMP_16_5);
    adc->SQR1  = 0;                 // Convert one channel
    adc->CFGR |= ADC_CFGR_DISCEN    // RES left at 000: 16-bit resolution
              |  ADC_CFGR_EXTEN_0   // Trigger on rising edge
              |  adcTrig            // Trigger source
              |  ADC_CFGR_DMNGT_1   // Continuous DMA requests
              |  ADC_CFGR_DMNGT_0;  // Enable DMA data transfer
    adc->CFGR2 = ADC_CFGR2_ROVSE    // Regular oversampling
               | ((OVS_RATIO - 1) << ADC_CFGR2_OVSR_Pos)
               | (OVS_SHIFT << ADC_CFGR2_OVSS_Pos);
    #else
    adc->SMPR2 = ADC_SMPR2_SMP2_2
               | ADC_SMPR2_SMP1_2;
    adc->SQR1  = 0;                 // Convert one channel
    adc->CR1  |= ADC_CR1_DISCEN;
    adc->CR2  |= ADC_CR2_EXTEN_0    // Trigger on rising edge
              |  adcTrig            // Trigger source
              |  ADC_CR2_DDS        // Continuous DMA requests
              |  ADC_CR2_DMA;       // Enable DMA data transfer
    #endif

    // Register end-of-transfer callbacks
    periph[instance].stream->setEndTransferCallback(std::bind(stopTransfer, instance));
}

void stm32adc_terminate()
{
    // Terminate streams before shutting of the peripherals
    for(int i = 0; i < 2; i++)
    {
        if(AdcContext[i] != NULL)
        {
            if(AdcContext[i]->running != 0)
                periph[i].stream->halt();
        }
    }

    // TODO: turn off peripherals
    #ifdef STM32H743xx
    RCC->APB1LENR &= ~RCC_APB1LENR_TIM2EN;
    #else
    RCC->APB1ENR &= ~RCC_APB1ENR_TIM2EN;
    #endif
    __DSB();
}

static int stm32adc_start(const uint8_t instance, const void *config, struct streamCtx *ctx)
{
    if(ctx == NULL)
        return -EINVAL;

    if((ctx->running != 0) || (periph[instance].stream->running()))
        return -EBUSY;

    __disable_irq();
    ctx->running = 1;
    __enable_irq();

    ctx->priv = (void *) &periph[instance];
    AdcContext[instance] = ctx;

    // Set conversion channel and enable the ADC
    ADC_TypeDef *adc = periph[instance].adc;

    #ifdef STM32H743xx
    uint32_t channel = (uint32_t) config;
    adc->SQR1  = channel << ADC_SQR1_SQ1_Pos;
    adc->PCSEL = 1 << channel;
    adc->CR   |= ADC_CR_ADSTART;
    #else
    adc->SQR3 = (uint32_t) config;
    adc->CR2 |= ADC_CR2_ADON;
    #endif

    // Start DMA stream
    bool circ = false;
    if(ctx->bufMode == BUF_CIRC_DOUBLE)
        circ = true;

    periph[instance].stream->start(&(periph[instance].adc->DR), ctx->buffer,
                                   ctx->bufSize, circ);

    // Configure ADC trigger
    periph[instance].tim.setUpdateFrequency(ctx->sampleRate);
    periph[instance].tim.start();

    return 0;
}

static int stm32adc_data(struct streamCtx *ctx, stream_sample_t **buf)
{
    AdcPeriph *p = reinterpret_cast< AdcPeriph * >(ctx->priv);

    // Default case: linear mode
    *buf = ctx->buffer;
    int size = ctx->bufSize;

    if(ctx->bufMode == BUF_CIRC_DOUBLE)
    {
        *buf = reinterpret_cast< stream_sample_t *>(p->stream->idleBuf());
        size = ctx->bufSize/2;
    }

    miosix::markBufferAfterDmaRead(*buf, size*sizeof(int16_t));
    return size;
}

static int stm32adc_sync(struct streamCtx *ctx, uint8_t dirty)
{
    (void) dirty;

    AdcPeriph *p = reinterpret_cast< AdcPeriph * >(ctx->priv);
    bool ok = p->stream->sync();
    if(ok)
        return 0;

   return -1;
}

static void stm32adc_stop(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    reinterpret_cast< AdcPeriph * >(ctx->priv)->stream->stop();
}

static void stm32adc_halt(struct streamCtx *ctx)
{
    if(ctx->running == 0)
        return;

    reinterpret_cast< AdcPeriph * >(ctx->priv)->stream->halt();
}

#pragma GCC diagnostic ignored "-Wpedantic"
const struct audioDriver stm32_adc_audio_driver =
{
    .start     = stm32adc_start,
    .data      = stm32adc_data,
    .sync      = stm32adc_sync,
    .stop      = stm32adc_stop,
    .terminate = stm32adc_halt
};
#pragma GCC diagnostic pop
