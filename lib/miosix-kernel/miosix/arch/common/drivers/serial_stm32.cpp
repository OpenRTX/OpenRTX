/***************************************************************************
 *   Copyright (C) 2010-2018 by Terraneo Federico                          *
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

#include <cstring>
#include <errno.h>
#include <termios.h>
#include "serial_stm32.h"
#include "kernel/sync.h"
#include "kernel/scheduler/scheduler.h"
#include "interfaces/portability.h"
#include "filesystem/ioctl.h"
#include "core/cache_cortexMx.h"

using namespace std;
using namespace miosix;

static const int numPorts=3; //Supporting only USART1, USART2, USART3

//A nice feature of the stm32 is that the USART are connected to the same
//GPIOS in all families, stm32f1, f2, f4 and l1. Additionally, USART1 is
//always connected to the APB2, while USART2 and USART3 are always on APB1
//Unfortunately, this does not hold with DMA.
typedef Gpio<GPIOA_BASE,9>  u1tx;
typedef Gpio<GPIOA_BASE,10> u1rx;
typedef Gpio<GPIOA_BASE,11> u1cts;
typedef Gpio<GPIOA_BASE,12> u1rts;

typedef Gpio<GPIOA_BASE,2>  u2tx;
typedef Gpio<GPIOA_BASE,3>  u2rx;
typedef Gpio<GPIOA_BASE,0>  u2cts;
typedef Gpio<GPIOA_BASE,1>  u2rts;

typedef Gpio<GPIOB_BASE,10> u3tx;
typedef Gpio<GPIOB_BASE,11> u3rx;
typedef Gpio<GPIOB_BASE,13> u3cts;
typedef Gpio<GPIOB_BASE,14> u3rts;

/// Pointer to serial port classes to let interrupts access the classes
static STM32Serial *ports[numPorts]={0};

/**
 * \internal interrupt routine for usart1 actual implementation
 */
void __attribute__((noinline)) usart1irqImpl()
{
   if(ports[0]) ports[0]->IRQhandleInterrupt();
}

/**
 * \internal interrupt routine for usart1
 */
void __attribute__((naked)) USART1_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart1irqImplv");
    restoreContext();
}

#if !defined(STM32_NO_SERIAL_2_3)

/**
 * \internal interrupt routine for usart2 actual implementation
 */
void __attribute__((noinline)) usart2irqImpl()
{
   if(ports[1]) ports[1]->IRQhandleInterrupt();
}

/**
 * \internal interrupt routine for usart2
 */
void __attribute__((naked)) USART2_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart2irqImplv");
    restoreContext();
}

#if !defined(STM32F411xE) && !defined(STM32F401xE) && !defined(STM32F401xC)
/**
 * \internal interrupt routine for usart3 actual implementation
 */
void __attribute__((noinline)) usart3irqImpl()
{
   if(ports[2]) ports[2]->IRQhandleInterrupt();
}

/**
 * \internal interrupt routine for usart3
 */
#if !defined(STM32F072xB)
void __attribute__((naked)) USART3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart3irqImplv");
    restoreContext();
}
#else  //!defined(STM32F072xB)
void __attribute__((naked)) USART3_4_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z13usart3irqImplv");
    restoreContext();
}
#endif //!defined(STM32F072xB)
#endif //!defined(STM32F411xE) && !defined(STM32F401xE) && !defined(STM32F401xC)
#endif //!defined(STM32_NO_SERIAL_2_3)

#ifdef SERIAL_1_DMA

/**
 * \internal USART1 DMA tx actual implementation
 */
void __attribute__((noinline)) usart1txDmaImpl()
{
    #if defined(_ARCH_CORTEXM3_STM32F1) || defined (_ARCH_CORTEXM4_STM32F3) \
     || defined(_ARCH_CORTEXM4_STM32L4)
    DMA1->IFCR=DMA_IFCR_CGIF4;
    DMA1_Channel4->CCR=0; //Disable DMA
    #else //stm32f2 and f4
    DMA2->HIFCR=DMA_HIFCR_CTCIF7
              | DMA_HIFCR_CTEIF7
              | DMA_HIFCR_CDMEIF7
              | DMA_HIFCR_CFEIF7;
    #endif
    if(ports[0]) ports[0]->IRQhandleDMAtx();
}

/**
 * \internal USART1 DMA rx actual implementation
 */
void __attribute__((noinline)) usart1rxDmaImpl()
{
    if(ports[0]) ports[0]->IRQhandleDMArx();
}

#if defined(_ARCH_CORTEXM3_STM32F1) || defined (_ARCH_CORTEXM4_STM32F3) \
 || defined(_ARCH_CORTEXM4_STM32L4)
/**
 * \internal DMA1 Channel 4 IRQ (configured as USART1 TX)
 */
void __attribute__((naked)) DMA1_Channel4_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart1txDmaImplv");
    restoreContext();
}

/**
 * \internal DMA1 Channel 5 IRQ (configured as USART1 RX)
 */
void __attribute__((naked)) DMA1_Channel5_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart1rxDmaImplv");
    restoreContext();
}

#else //stm32f2 and stm32f4

/**
 * \internal DMA2 stream 7 IRQ (configured as USART1 TX)
 */
void __attribute__((naked)) DMA2_Stream7_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart1txDmaImplv");
    restoreContext();
}

/**
 * \internal DMA2 stream 5 IRQ (configured as USART1 RX)
 */
void __attribute__((naked)) DMA2_Stream5_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart1rxDmaImplv");
    restoreContext();
}
#endif
#endif //SERIAL_1_DMA

#if defined(SERIAL_2_DMA) && !defined(STM32_NO_SERIAL_2_3)

/**
 * \internal USART2 DMA tx actual implementation
 */
void __attribute__((noinline)) usart2txDmaImpl()
{
    #if defined(_ARCH_CORTEXM3_STM32F1) || defined (_ARCH_CORTEXM4_STM32F3) \
     || defined(_ARCH_CORTEXM4_STM32L4)
    DMA1->IFCR=DMA_IFCR_CGIF7;
    DMA1_Channel7->CCR=0; //Disable DMA
    #else //stm32f2 and f4
    DMA1->HIFCR=DMA_HIFCR_CTCIF6
              | DMA_HIFCR_CTEIF6
              | DMA_HIFCR_CDMEIF6
              | DMA_HIFCR_CFEIF6;
    #endif
    if(ports[1]) ports[1]->IRQhandleDMAtx();
}

/**
 * \internal USART2 DMA rx actual implementation
 */
void __attribute__((noinline)) usart2rxDmaImpl()
{
    if(ports[1]) ports[1]->IRQhandleDMArx();
}

#if defined(_ARCH_CORTEXM3_STM32F1) || defined (_ARCH_CORTEXM4_STM32F3) \
 || defined(_ARCH_CORTEXM4_STM32L4)
/**
 * \internal DMA1 Channel 7 IRQ (configured as USART2 TX)
 */
void __attribute__((naked)) DMA1_Channel7_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart2txDmaImplv");
    restoreContext();
}

/**
 * \internal DMA1 Channel 6 IRQ (configured as USART2 RX)
 */
void __attribute__((naked)) DMA1_Channel6_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart2rxDmaImplv");
    restoreContext();
}

#else //stm32f2 and stm32f4

/**
 * \internal DMA1 stream 6 IRQ (configured as USART2 TX)
 */
void __attribute__((naked)) DMA1_Stream6_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart2txDmaImplv");
    restoreContext();
}

/**
 * \internal DMA1 stream 5 IRQ (configured as USART2 RX)
 */
void __attribute__((naked)) DMA1_Stream5_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart2rxDmaImplv");
    restoreContext();
}
#endif
#endif //SERIAL_2_DMA

#if defined(SERIAL_3_DMA) && !defined(STM32_NO_SERIAL_2_3)

/**
 * \internal USART3 DMA tx actual implementation
 */
void __attribute__((noinline)) usart3txDmaImpl()
{
    #if defined(_ARCH_CORTEXM3_STM32F1) || defined (_ARCH_CORTEXM4_STM32F3) \
     || defined(_ARCH_CORTEXM4_STM32L4)
    DMA1->IFCR=DMA_IFCR_CGIF2;
    DMA1_Channel2->CCR=0; //Disable DMA
    #else //stm32f2 and f4
    DMA1->LIFCR=DMA_LIFCR_CTCIF3
              | DMA_LIFCR_CTEIF3
              | DMA_LIFCR_CDMEIF3
              | DMA_LIFCR_CFEIF3;
    #endif
    if(ports[2]) ports[2]->IRQhandleDMAtx();
}

/**
 * \internal USART3 DMA rx actual implementation
 */
void __attribute__((noinline)) usart3rxDmaImpl()
{
    if(ports[2]) ports[2]->IRQhandleDMArx();
}

#if defined(_ARCH_CORTEXM3_STM32F1) || defined (_ARCH_CORTEXM4_STM32F3) \
 || defined(_ARCH_CORTEXM4_STM32L4)
/**
 * \internal DMA1 Channel 2 IRQ (configured as USART3 TX)
 */
void __attribute__((naked)) DMA1_Channel2_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart3txDmaImplv");
    restoreContext();
}

/**
 * \internal DMA1 Channel 3 IRQ (configured as USART3 RX)
 */
void __attribute__((naked)) DMA1_Channel3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart3rxDmaImplv");
    restoreContext();
}

#else //stm32f2 and stm32f4

/**
 * \internal DMA1 stream 3 IRQ (configured as USART3 TX)
 */
void __attribute__((naked)) DMA1_Stream3_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart3txDmaImplv");
    restoreContext();
}

/**
 * \internal DMA1 stream 1 IRQ (configured as USART3 RX)
 */
void __attribute__((naked)) DMA1_Stream1_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15usart3rxDmaImplv");
    restoreContext();
}
#endif
#endif //SERIAL_3_DMA

namespace miosix {

#ifdef SERIAL_DMA
#if defined(_ARCH_CORTEXM4_STM32F4) || defined(_ARCH_CORTEXM4_STM32F3) \
 || defined(_ARCH_CORTEXM4_STM32L4)
/**
 * The STM3F3, STM32F4 and STM32L4 have an ugly quirk of having 64KB RAM area
 * called CCM that can only be accessed by the processor and not be the DMA.
 * \param x pointer to check
 * \return true if the pointer is inside the CCM, and thus it isn't possible
 * to use it for DMA transfers
 */
static bool isInCCMarea(const void *x)
{
    unsigned int ptr=reinterpret_cast<const unsigned int>(x);
    return (ptr>=0x10000000) && (ptr<(0x10000000+64*1024));
}
#else //_ARCH_CORTEXM4_STM32F4 and _ARCH_CORTEXM4_STM32F3
static inline bool isInCCMarea(const void *x) { return false; }
#endif // _ARCH_CORTEXM4_STM32F4 and _ARCH_CORTEXM4_STM32F3
#endif //SERIAL_DMA

//
// class STM32Serial
//

// A note on the baudrate/500: the buffer is selected so as to withstand
// 20ms of full data rate. In the 8N1 format one char is made of 10 bits.
// So (baudrate/10)*0.02=baudrate/500
STM32Serial::STM32Serial(int id, int baudrate, FlowCtrl flowControl)
        : Device(Device::TTY), rxQueue(rxQueueMin+baudrate/500),
          flowControl(flowControl==RTSCTS), portId(id)
{
    #if !defined(_ARCH_CORTEXM3_STM32F1)
    //stm32f2, f4, l4, l1, f7, h7 require alternate function mapping
    //stm32f0 family has different alternate function mapping
    //with respect to the other families
    switch(id)
    {
        case 1:
            #if !defined(_ARCH_CORTEXM0_STM32F0)
            u1tx::alternateFunction(7);
            u1rx::alternateFunction(7);
            if(flowControl)
            {
                u1rts::alternateFunction(7);
                u1cts::alternateFunction(7);
            }
            #else  //!defined(_ARCH_CORTEXM0_STM32F0)
            u1tx::alternateFunction(1);
            u1rx::alternateFunction(1);
            if(flowControl)
            {
                u1rts::alternateFunction(1);
                u1cts::alternateFunction(1);
            }
            #endif //!defined(_ARCH_CORTEXM0_STM32F0)
            break;
        case 2:
            #if !defined(_ARCH_CORTEXM0_STM32F0)
            u2tx::alternateFunction(7);
            u2rx::alternateFunction(7);
            if(flowControl)
            {
                u2rts::alternateFunction(7);
                u2cts::alternateFunction(7);
            }
            #else  //!defined(_ARCH_CORTEXM0_STM32F0)
            u2tx::alternateFunction(1);
            u2rx::alternateFunction(1);
            if(flowControl)
            {
                u2rts::alternateFunction(1);
                u2cts::alternateFunction(1);
            }
            #endif //!defined(_ARCH_CORTEXM0_STM32F0)            
            break;
        case 3:
            #if !defined(_ARCH_CORTEXM0_STM32F0)
            u3tx::alternateFunction(7);
            u3rx::alternateFunction(7);
            if(flowControl)
            {
                u3rts::alternateFunction(7);
                u3cts::alternateFunction(7);
            }
            #else  //!defined(_ARCH_CORTEXM0_STM32F0)
            u3tx::alternateFunction(4);
            u3rx::alternateFunction(4);
            if(flowControl)
            {
                u3rts::alternateFunction(4);
                u3cts::alternateFunction(4);
            }
            #endif //!defined(_ARCH_CORTEXM0_STM32F0)            
            break;
    }
    #endif //_ARCH_CORTEXM3_STM32F1
    
    switch(id)
    {
        case 1:
            commonInit(id,baudrate,u1tx::getPin(),u1rx::getPin(),
                       u1rts::getPin(),u1cts::getPin());
            break;
        case 2:
            commonInit(id,baudrate,u2tx::getPin(),u2rx::getPin(),
                       u2rts::getPin(),u2cts::getPin());
            break;
        case 3:
            commonInit(id,baudrate,u3tx::getPin(),u3rx::getPin(),
                       u3rts::getPin(),u3cts::getPin());
            break;
    }
}

STM32Serial::STM32Serial(int id, int baudrate, GpioPin tx, GpioPin rx)
    : Device(Device::TTY), rxQueue(rxQueueMin+baudrate/500),
      flowControl(false), portId(id)
{
    commonInit(id,baudrate,tx,rx,tx,rx); //The last two args will be ignored
}

STM32Serial::STM32Serial(int id, int baudrate, GpioPin tx, GpioPin rx,
    miosix::GpioPin rts, miosix::GpioPin cts)
    : Device(Device::TTY), rxQueue(rxQueueMin+baudrate/500),
      flowControl(true), portId(id)
{
    commonInit(id,baudrate,tx,rx,rts,cts);
}

void STM32Serial::commonInit(int id, int baudrate, GpioPin tx, GpioPin rx,
                             GpioPin rts, GpioPin cts)
{
    #ifdef SERIAL_DMA
    dmaTx=0;
    dmaRx=0;
    txWaiting=0;
    dmaTxInProgress=false;
    #endif //SERIAL_DMA
    InterruptDisableLock dLock;
    if(id<1|| id>numPorts || ports[id-1]!=0) errorHandler(UNEXPECTED);
    ports[id-1]=this;
    unsigned int freq=SystemCoreClock;
    //Quirk the position of the PPRE1 and PPRE2 bitfields in RCC->CFGR changes
    //STM32F0 does not have ppre1 and ppre2, in this case the variables are not
    //defined in order to avoid "unused variable" warning
    #if defined(_ARCH_CORTEXM3_STM32F1)   || defined(_ARCH_CORTEXM3_STM32L1) \
     || defined(_ARCH_CORTEXM4_STM32F3) || defined(_ARCH_CORTEXM4_STM32L4)
    const unsigned int ppre1=8;
    const unsigned int ppre2=11;
    #elif !defined(_ARCH_CORTEXM7_STM32H7) && !defined(_ARCH_CORTEXM0_STM32F0)
    const unsigned int ppre1=10;
    const unsigned int ppre2=13;
    #endif
    switch(id)
    {
        case 1:
            port=USART1;
            RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
            RCC_SYNC();
            #ifdef SERIAL_1_DMA
            #if defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM4_STM32F3) \
             || defined(_ARCH_CORTEXM4_STM32L4)
            #ifdef _ARCH_CORTEXM4_STM32L4
            RCC->AHB1ENR |= RCC_AHBENR_DMA1EN;
            DMA1_CSELR->CSELR  |= (2 << DMA_CSELR_C4S_Pos) // Assign DMA1_CH4 to USART1_TX
                               |  (2 << DMA_CSELR_C5S_Pos);// Assign DMA1_CH5 to USART1_RX
            #else 
            RCC->AHBENR |= RCC_AHBENR_DMA1EN;
            #endif
            RCC_SYNC();
            NVIC_SetPriority(DMA1_Channel4_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(DMA1_Channel4_IRQn);
            dmaTx=DMA1_Channel4;
            //Higher priority to ensure IRQhandleDMArx() is called before
            //IRQhandleInterrupt(), so that idle is set correctly
            NVIC_SetPriority(DMA1_Channel5_IRQn,14);
            NVIC_EnableIRQ(DMA1_Channel5_IRQn);
            dmaRx=DMA1_Channel5;
            #else //stm32f2, stm32f4
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
            RCC_SYNC();
            NVIC_SetPriority(DMA2_Stream7_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(DMA2_Stream7_IRQn);
            dmaTx=DMA2_Stream7;
            //Higher priority to ensure IRQhandleDMArx() is called before
            //IRQhandleInterrupt(), so that idle is set correctly
            NVIC_SetPriority(DMA2_Stream5_IRQn,14);
            NVIC_EnableIRQ(DMA2_Stream5_IRQn);
            dmaRx=DMA2_Stream5;
            #endif
            port->CR3=USART_CR3_DMAT | USART_CR3_DMAR;
            #endif //SERIAL_1_DMA
            NVIC_SetPriority(USART1_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(USART1_IRQn);
            #if !defined(_ARCH_CORTEXM7_STM32H7) && !defined(_ARCH_CORTEXM0_STM32F0)
            if(RCC->CFGR & RCC_CFGR_PPRE2_2) freq/=1<<(((RCC->CFGR>>ppre2) & 0x3)+1);
            #elif defined(_ARCH_CORTEXM0_STM32F0)
            // STM32F0 family has only PPRE2 register
            if(RCC->CFGR & RCC_CFGR_PPRE_2) freq/=1<<(((RCC->CFGR>>8) & 0x3)+1);
            #else
            //rcc_hclk3 = SystemCoreClock / HPRE
            //rcc_pclk2 = rcc_hclk1 / D2PPRE2
            //NOTE: are rcc_hclk3 and rcc_hclk1 the same signal?
            //usart1 clock is rcc_pclk2
            if(RCC->D1CFGR & RCC_D1CFGR_HPRE_3)
                freq/=1<<(((RCC->D1CFGR>>RCC_D1CFGR_HPRE_Pos) & 0x7)+1);
            if(RCC->D2CFGR & RCC_D2CFGR_D2PPRE2_2)
                freq/=1<<(((RCC->D2CFGR>>RCC_D2CFGR_D2PPRE2_Pos) & 0x3)+1);
            #endif //_ARCH_CORTEXM7_STM32H7
            break;
        
        #if !defined(STM32_NO_SERIAL_2_3)
        case 2:
            port=USART2;
            #ifndef _ARCH_CORTEXM7_STM32H7
            #ifndef _ARCH_CORTEXM4_STM32L4
            RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
            #else  //_ARCH_CORTEXM4_STM32L4
            RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
            #endif //_ARCH_CORTEXM4_STM32L4
            #else  //_ARCH_CORTEXM7_STM32H7
            RCC->APB1LENR |= RCC_APB1LENR_USART2EN;
            #endif //_ARCH_CORTEXM7_STM32H7
            RCC_SYNC();
            #ifdef SERIAL_2_DMA
            #if defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM4_STM32F3) \
             || defined(_ARCH_CORTEXM4_STM32L4)
            #ifdef _ARCH_CORTEXM4_STM32L4
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
            DMA1_CSELR->CSELR  |= (2 << DMA_CSELR_C7S_Pos) // Assign DMA1_CH7 to USART2_TX
                               |  (2 << DMA_CSELR_C6S_Pos);// Assign DMA1_CH6 to USART2_RX
            #else 
            RCC->AHBENR |= RCC_AHBENR_DMA1EN;
            #endif
            RCC_SYNC();
            NVIC_SetPriority(DMA1_Channel7_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(DMA1_Channel7_IRQn);
            dmaTx=DMA1_Channel7;
            //Higher priority to ensure IRQhandleDMArx() is called before
            //IRQhandleInterrupt(), so that idle is set correctly
            NVIC_SetPriority(DMA1_Channel6_IRQn,14);
            NVIC_EnableIRQ(DMA1_Channel6_IRQn);
            dmaRx=DMA1_Channel6;
            #else //stm32f2, stm32f4
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
            RCC_SYNC();
            NVIC_SetPriority(DMA1_Stream6_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(DMA1_Stream6_IRQn);
            dmaTx=DMA1_Stream6;
            //Higher priority to ensure IRQhandleDMArx() is called before
            //IRQhandleInterrupt(), so that idle is set correctly
            NVIC_SetPriority(DMA1_Stream5_IRQn,14);
            NVIC_EnableIRQ(DMA1_Stream5_IRQn);
            dmaRx=DMA1_Stream5;
            #endif
            port->CR3=USART_CR3_DMAT | USART_CR3_DMAR;
            #endif //SERIAL_2_DMA
            NVIC_SetPriority(USART2_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(USART2_IRQn);
            #if !defined(_ARCH_CORTEXM7_STM32H7) && !defined(_ARCH_CORTEXM0_STM32F0)
            if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/=1<<(((RCC->CFGR>>ppre1) & 0x3)+1);
            #elif defined(_ARCH_CORTEXM0_STM32F0)
            // STM32F0 family has only PPRE2 register
            if(RCC->CFGR & RCC_CFGR_PPRE_2) freq/=1<<(((RCC->CFGR>>8) & 0x3)+1);
            #else //_ARCH_CORTEXM7_STM32H7
            //rcc_hclk3 = SystemCoreClock / HPRE
            //rcc_pclk1 = rcc_hclk1 / D2PPRE1
            //NOTE: are rcc_hclk3 and rcc_hclk1 the same signal?
            //usart2 clock is rcc_pclk1
            if(RCC->D1CFGR & RCC_D1CFGR_HPRE_3)
                freq/=1<<(((RCC->D1CFGR>>RCC_D1CFGR_HPRE_Pos) & 0x7)+1);
            if(RCC->D2CFGR & RCC_D2CFGR_D2PPRE1_2)
                freq/=1<<(((RCC->D2CFGR>>RCC_D2CFGR_D2PPRE1_Pos) & 0x3)+1);
            #endif //_ARCH_CORTEXM7_STM32H7
            break;
        #if !defined(STM32F411xE) && !defined(STM32F401xE) && !defined(STM32F401xC)
        case 3:
            port=USART3;
            #ifndef _ARCH_CORTEXM7_STM32H7
            #ifndef _ARCH_CORTEXM4_STM32L4
            RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
            #else  //_ARCH_CORTEXM4_STM32L4
            RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN;
            #endif //_ARCH_CORTEXM4_STM32L4
            #else
            RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
            #endif //_ARCH_CORTEXM7_STM32H7
            RCC_SYNC();
            #ifdef SERIAL_3_DMA
            #if defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM4_STM32F3) \
             || defined(_ARCH_CORTEXM4_STM32L4)
            #ifdef _ARCH_CORTEXM4_STM32L4
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
            DMA1_CSELR->CSELR  |= (2 << DMA_CSELR_C2S_Pos) // Assign DMA1_CH2 to USART2_TX
                               |  (2 << DMA_CSELR_C3S_Pos);// Assign DMA1_CH3 to USART2_RX
            #else 
            RCC->AHBENR |= RCC_AHBENR_DMA1EN;
            #endif
            RCC_SYNC();
            NVIC_SetPriority(DMA1_Channel2_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(DMA1_Channel2_IRQn);
            dmaTx=DMA1_Channel2;
            //Higher priority to ensure IRQhandleDMArx() is called before
            //IRQhandleInterrupt(), so that idle is set correctly
            NVIC_SetPriority(DMA1_Channel3_IRQn,14);
            NVIC_EnableIRQ(DMA1_Channel3_IRQn);
            dmaRx=DMA1_Channel3;
            #else //stm32f2, stm32f4
            RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
            RCC_SYNC();
            NVIC_SetPriority(DMA1_Stream3_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(DMA1_Stream3_IRQn);
            dmaTx=DMA1_Stream3;
            //Higher priority to ensure IRQhandleDMArx() is called before
            //IRQhandleInterrupt(), so that idle is set correctly
            NVIC_SetPriority(DMA1_Stream1_IRQn,14);
            NVIC_EnableIRQ(DMA1_Stream1_IRQn);
            dmaRx=DMA1_Stream1;
            #endif
            port->CR3=USART_CR3_DMAT | USART_CR3_DMAR;
            #endif //SERIAL_3_DMA
            #if !defined(STM32F072xB)
            NVIC_SetPriority(USART3_IRQn,15);//Lowest priority for serial
            NVIC_EnableIRQ(USART3_IRQn);
            #else  //STM32F072xB
            NVIC_SetPriority(USART3_4_IRQn,15);
            NVIC_EnableIRQ(USART3_4_IRQn);
            #endif //STM32F072xB
            #if !defined(_ARCH_CORTEXM7_STM32H7) && !defined(_ARCH_CORTEXM0_STM32F0)
            if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/=1<<(((RCC->CFGR>>ppre1) & 0x3)+1);
            #elif defined(_ARCH_CORTEXM0_STM32F0)
            // STM32F0 family has only PPRE2 register
            if(RCC->CFGR & RCC_CFGR_PPRE_2) freq/=1<<(((RCC->CFGR>>8) & 0x3)+1);
            #else //_ARCH_CORTEXM7_STM32H7
            //rcc_hclk3 = SystemCoreClock / HPRE
            //rcc_pclk1 = rcc_hclk1 / D2PPRE1
            //NOTE: are rcc_hclk3 and rcc_hclk1 the same signal?
            //usart2 clock is rcc_pclk1
            if(RCC->D1CFGR & RCC_D1CFGR_HPRE_3)
                freq/=1<<(((RCC->D1CFGR>>RCC_D1CFGR_HPRE_Pos) & 0x7)+1);
            if(RCC->D2CFGR & RCC_D2CFGR_D2PPRE1_2)
                freq/=1<<(((RCC->D2CFGR>>RCC_D2CFGR_D2PPRE1_Pos) & 0x3)+1);
            #endif //_ARCH_CORTEXM7_STM32H7
            break;
        #endif //!defined(STM32F411xE) && !defined(STM32F401xE) && !defined(STM32F401xC)
        #endif //!defined(STM32_NO_SERIAL_2_3)
    }
    //Quirk: stm32f1 rx pin has to be in input mode, while stm32f2 and up want
    //it in ALTERNATE mode. Go figure...
    #ifdef _ARCH_CORTEXM3_STM32F1
    Mode::Mode_ rxPinMode=Mode::INPUT;
    #else //_ARCH_CORTEXM3_STM32F1
    Mode::Mode_ rxPinMode=Mode::ALTERNATE;
    #endif //_ARCH_CORTEXM3_STM32F1
    tx.mode(Mode::ALTERNATE);
    rx.mode(rxPinMode);
    if(flowControl)
    {
        rts.mode(Mode::ALTERNATE);
        cts.mode(rxPinMode);
    }
    const unsigned int quot=2*freq/baudrate; //2*freq for round to nearest
    port->BRR=quot/2 + (quot & 1);           //Round to nearest
    if(flowControl==false) port->CR3 |= USART_CR3_ONEBIT;
    else port->CR3 |= USART_CR3_ONEBIT | USART_CR3_RTSE | USART_CR3_CTSE;
    //Enabled, 8 data bit, no parity, interrupt on character rx
    #ifdef SERIAL_DMA
    if(dmaTx)
    {
        port->CR1 = USART_CR1_UE     //Enable port
                  | USART_CR1_IDLEIE //Interrupt on idle line
                  | USART_CR1_TE     //Transmission enbled
                  | USART_CR1_RE;    //Reception enabled
        IRQdmaReadStart();
        return;
    }
    #endif //SERIAL_DMA
    port->CR1 = USART_CR1_UE     //Enable port
              | USART_CR1_RXNEIE //Interrupt on data received
              | USART_CR1_IDLEIE //Interrupt on idle line
              | USART_CR1_TE     //Transmission enbled
              | USART_CR1_RE;    //Reception enabled
}

ssize_t STM32Serial::readBlock(void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> l(rxMutex);
    char *buf=reinterpret_cast<char*>(buffer);
    size_t result=0;
    FastInterruptDisableLock dLock;
    DeepSleepLock dpLock;
    for(;;)
    {
        //Try to get data from the queue
        for(;result<size;result++)
        {
            if(rxQueue.tryGet(buf[result])==false) break;
            //This is here just not to keep IRQ disabled for the whole loop
            FastInterruptEnableLock eLock(dLock);
        }
        if(idle && result>0) break;
        if(result==size) break;
        //Wait for data in the queue
        do {
            rxWaiting=Thread::IRQgetCurrentThread();
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        } while(rxWaiting);
    }
    return result;
}

ssize_t STM32Serial::writeBlock(const void *buffer, size_t size, off_t where)
{
    Lock<FastMutex> l(txMutex);
    DeepSleepLock dpLock;
    const char *buf=reinterpret_cast<const char*>(buffer);
    #ifdef SERIAL_DMA
    if(dmaTx)
    {
        size_t remaining=size;
        if(isInCCMarea(buf)==false)
        {
            //Use zero copy for all but the last txBufferSize bytes, if possible
            while(remaining>txBufferSize)
            {
                //DMA is limited to 64K
                size_t transferSize=min<size_t>(remaining-txBufferSize,65535);
                waitDmaTxCompletion();
                writeDma(buf,transferSize);
                buf+=transferSize;
                remaining-=transferSize;
            }
        }
        while(remaining>0)
        {
            size_t transferSize=min(remaining,static_cast<size_t>(txBufferSize));
            waitDmaTxCompletion();
            //Copy to txBuffer only after DMA xfer completed, as the previous
            //xfer may be using the same buffer
            memcpy(txBuffer,buf,transferSize);
            writeDma(txBuffer,transferSize);
            buf+=transferSize;
            remaining-=transferSize;
        }
        return size;
    }
    #endif //SERIAL_DMA
    for(size_t i=0;i<size;i++)
    {
        #if !defined(_ARCH_CORTEXM7_STM32F7) && !defined(_ARCH_CORTEXM7_STM32H7) \
         && !defined(_ARCH_CORTEXM0_STM32F0)   && !defined(_ARCH_CORTEXM4_STM32F3) \
         && !defined(_ARCH_CORTEXM4_STM32L4)
        while((port->SR & USART_SR_TXE)==0) ;
        port->DR=*buf++;
        #else //_ARCH_CORTEXM7_STM32F7/H7
        while((port->ISR & USART_ISR_TXE)==0) ;
        port->TDR=*buf++;
        #endif //_ARCH_CORTEXM7_STM32F7/H7
    }
    return size;
}

void STM32Serial::IRQwrite(const char *str)
{
    // We can reach here also with only kernel paused, so make sure
    // interrupts are disabled. This is important for the DMA case
    bool interrupts=areInterruptsEnabled();
    if(interrupts) fastDisableInterrupts();
    #ifdef SERIAL_DMA
    if(dmaTx)
    {
        #if defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM4_STM32F3) \
         || defined(_ARCH_CORTEXM4_STM32L4)
        //If no DMA transfer is in progress bit EN is zero. Otherwise wait until
        //DMA xfer ends, by waiting for the TC (or TE) interrupt flag
        static const unsigned int irqMask[]=
        {
            (DMA_ISR_TCIF4 | DMA_ISR_TEIF4),
            (DMA_ISR_TCIF7 | DMA_ISR_TEIF7),
            (DMA_ISR_TCIF2 | DMA_ISR_TEIF2)
        };
        #if defined(_ARCH_CORTEXM4_STM32F3) || defined(_ARCH_CORTEXM4_STM32L4)
        // Workaround for ST messing up with flag definitions...
        constexpr unsigned int DMA_CCR4_EN = DMA_CCR_EN;
        #endif
        while((dmaTx->CCR & DMA_CCR4_EN) && !(DMA1->ISR & irqMask[getId()-1])) ;
        #else //_ARCH_CORTEXM3_STM32F1
        //Wait until DMA xfer ends. EN bit is cleared by hardware on transfer end
        while(dmaTx->CR & DMA_SxCR_EN) ;
        #endif //_ARCH_CORTEXM3_STM32F1
    }
    #endif //SERIAL_DMA
    while(*str)
    {
        #if !defined(_ARCH_CORTEXM7_STM32F7) && !defined(_ARCH_CORTEXM7_STM32H7) \
         && !defined(_ARCH_CORTEXM0_STM32F0)   && !defined(_ARCH_CORTEXM4_STM32F3) \
         && !defined(_ARCH_CORTEXM4_STM32L4)
        while((port->SR & USART_SR_TXE)==0) ;
        port->DR=*str++;
        #else //_ARCH_CORTEXM7_STM32F7/H7
        while((port->ISR & USART_ISR_TXE)==0) ;
        port->TDR=*str++;
        #endif //_ARCH_CORTEXM7_STM32F7/H7
    }
    waitSerialTxFifoEmpty();
    if(interrupts) fastEnableInterrupts();
}

int STM32Serial::ioctl(int cmd, void* arg)
{
    if(reinterpret_cast<unsigned>(arg) & 0b11) return -EFAULT; //Unaligned
    termios *t=reinterpret_cast<termios*>(arg);
    switch(cmd)
    {
        case IOCTL_SYNC:
            waitSerialTxFifoEmpty();
            return 0;
        case IOCTL_TCGETATTR:
            t->c_iflag=IGNBRK | IGNPAR;
            t->c_oflag=0;
            t->c_cflag=CS8 | (flowControl ? CRTSCTS : 0);
            t->c_lflag=0;
            return 0;
        case IOCTL_TCSETATTR_NOW:
        case IOCTL_TCSETATTR_DRAIN:
        case IOCTL_TCSETATTR_FLUSH:
            //Changing things at runtime unsupported, so do nothing, but don't
            //return error as console_device.h implements some attribute changes
            return 0;
        default:
            return -ENOTTY; //Means the operation does not apply to this descriptor
    }
}

void STM32Serial::IRQhandleInterrupt()
{
    #if !defined(_ARCH_CORTEXM7_STM32F7) && !defined(_ARCH_CORTEXM7_STM32H7) \
     && !defined(_ARCH_CORTEXM0_STM32F0)   && !defined(_ARCH_CORTEXM4_STM32F3) \
     && !defined(_ARCH_CORTEXM4_STM32L4)
    unsigned int status=port->SR;
    #else //_ARCH_CORTEXM7_STM32F7/H7
    unsigned int status=port->ISR;
    constexpr unsigned int USART_SR_RXNE=USART_ISR_RXNE;
    constexpr unsigned int USART_SR_IDLE=USART_ISR_IDLE;
    constexpr unsigned int USART_SR_FE  =USART_ISR_FE;
    #endif //_ARCH_CORTEXM7_STM32F7/H7
    char c;
    #ifdef SERIAL_DMA
    if(dmaRx==0 && (status & USART_SR_RXNE))
    #else //SERIAL_DMA
    if(status & USART_SR_RXNE)
    #endif //SERIAL_DMA
    {
        //Always read data, since this clears interrupt flags
        #if !defined(_ARCH_CORTEXM7_STM32F7) && !defined(_ARCH_CORTEXM7_STM32H7) \
         && !defined(_ARCH_CORTEXM0_STM32F0)   && !defined(_ARCH_CORTEXM4_STM32F3) \
         && !defined(_ARCH_CORTEXM4_STM32L4)
        c=port->DR;
        #else //_ARCH_CORTEXM7_STM32F7/H7
        c=port->RDR;
        #endif //_ARCH_CORTEXM7_STM32F7/H7
        //If no error put data in buffer
        if((status & USART_SR_FE)==0)
            if(rxQueue.tryPut(c)==false) /*fifo overflow*/;
        idle=false;
    }
    if(status & USART_SR_IDLE)
    {
        #if !defined(_ARCH_CORTEXM7_STM32F7) && !defined(_ARCH_CORTEXM7_STM32H7) \
         && !defined(_ARCH_CORTEXM0_STM32F0)   && !defined(_ARCH_CORTEXM4_STM32F3) \
         && !defined(_ARCH_CORTEXM4_STM32L4)
        c=port->DR; //clears interrupt flags
        #else //_ARCH_CORTEXM7_STM32F7/H7
        port->ICR=USART_ICR_IDLECF; //clears interrupt flags
        #endif //_ARCH_CORTEXM7_STM32F7/H7
        #ifdef SERIAL_DMA
        if(dmaRx) IRQreadDma();
        #endif //SERIAL_DMA
        idle=true;
    }
    if((status & USART_SR_IDLE) || rxQueue.size()>=rxQueueMin)
    {
        //Enough data in buffer or idle line, awake thread
        if(rxWaiting)
        {
            rxWaiting->IRQwakeup();
            if(rxWaiting->IRQgetPriority()>
                Thread::IRQgetCurrentThread()->IRQgetPriority())
                    Scheduler::IRQfindNextThread();
            rxWaiting=0;
        }
    }
}

#ifdef SERIAL_DMA
void STM32Serial::IRQhandleDMAtx()
{
    dmaTxInProgress=false;
    if(txWaiting==0) return;
    txWaiting->IRQwakeup();
    if(txWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    txWaiting=0;
}

void STM32Serial::IRQhandleDMArx()
{
    IRQreadDma();
    idle=false;
    if(rxWaiting==0) return;
    rxWaiting->IRQwakeup();
    if(rxWaiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    rxWaiting=0;
}
#endif //SERIAL_DMA

STM32Serial::~STM32Serial()
{
    waitSerialTxFifoEmpty();
    {
        InterruptDisableLock dLock;
        port->CR1=0;
        int id=getId();
        ports[id-1]=0;
        switch(id)
        {
            case 1:
                #ifdef SERIAL_1_DMA
                IRQdmaReadStop();
                #if defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM4_STM32F3) \
                 || defined(_ARCH_CORTEXM4_STM32L4)
                NVIC_DisableIRQ(DMA1_Channel4_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Channel4_IRQn);
                NVIC_DisableIRQ(DMA1_Channel5_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Channel5_IRQn);
                #else //stm32f2, stm32f4
                NVIC_DisableIRQ(DMA2_Stream7_IRQn);
                NVIC_ClearPendingIRQ(DMA2_Stream7_IRQn);
                NVIC_DisableIRQ(DMA2_Stream5_IRQn);
                NVIC_ClearPendingIRQ(DMA2_Stream5_IRQn);
                #endif
                #endif //SERIAL_1_DMA
                NVIC_DisableIRQ(USART1_IRQn);
                NVIC_ClearPendingIRQ(USART1_IRQn);
                RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN;
                break;
                
            #if !defined(STM32_NO_SERIAL_2_3)    
            case 2:
                #ifdef SERIAL_2_DMA
                IRQdmaReadStop();
                #if defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM4_STM32F3) \
                 || defined(_ARCH_CORTEXM4_STM32L4)
                NVIC_DisableIRQ(DMA1_Channel7_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Channel7_IRQn);
                NVIC_DisableIRQ(DMA1_Channel6_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Channel6_IRQn);
                #else //stm32f2, stm32f4
                NVIC_DisableIRQ(DMA1_Stream6_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Stream6_IRQn);
                NVIC_DisableIRQ(DMA1_Stream5_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Stream5_IRQn);
                #endif
                #endif //SERIAL_2_DMA
                NVIC_DisableIRQ(USART2_IRQn);
                NVIC_ClearPendingIRQ(USART2_IRQn);
                #ifndef _ARCH_CORTEXM7_STM32H7
                #ifdef _ARCH_CORTEXM4_STM32L4
                RCC->APB1ENR1 &= ~RCC_APB1ENR1_USART2EN;
                #else
                RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;
                #endif
                #else //_ARCH_CORTEXM7_STM32H7
                RCC->APB1LENR &= ~RCC_APB1LENR_USART2EN;
                #endif //_ARCH_CORTEXM7_STM32H7
                break;
            #if !defined(STM32F411xE) && !defined(STM32F401xE) && !defined(STM32F401xC)
            case 3:
                #ifdef SERIAL_3_DMA
                IRQdmaReadStop();
                #if defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM4_STM32F3) \
                 || defined(_ARCH_CORTEXM4_STM32L4)
                NVIC_DisableIRQ(DMA1_Channel2_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Channel2_IRQn);
                NVIC_DisableIRQ(DMA1_Channel3_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Channel3_IRQn);
                #else //stm32f2, stm32f4
                NVIC_DisableIRQ(DMA1_Stream3_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Stream3_IRQn);
                NVIC_DisableIRQ(DMA1_Stream1_IRQn);
                NVIC_ClearPendingIRQ(DMA1_Stream1_IRQn);
                #endif
                #endif //SERIAL_3_DMA
                #if !defined(STM32F072xB)
                NVIC_SetPriority(USART3_IRQn,15);//Lowest priority for serial
                NVIC_EnableIRQ(USART3_IRQn);
                #else  //STM32F072xB
                NVIC_SetPriority(USART3_4_IRQn,15);
                NVIC_EnableIRQ(USART3_4_IRQn);
                #endif //STM32F072xB
                #ifndef _ARCH_CORTEXM7_STM32H7
                #ifdef _ARCH_CORTEXM4_STM32L4
                RCC->APB1ENR1 &= ~RCC_APB1ENR1_USART3EN;
                #else
                RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN;
                #endif
                #else //_ARCH_CORTEXM7_STM32H7
                RCC->APB1LENR &= ~RCC_APB1LENR_USART3EN;
                #endif //_ARCH_CORTEXM7_STM32H7
                break;
            #endif //!defined(STM32F411xE) && !defined(STM32F401xE) && !defined(STM32F401xC)
            #endif //!defined(STM32_NO_SERIAL_2_3)
        }
    }
}

#ifdef SERIAL_DMA
void STM32Serial::waitDmaTxCompletion()
{
    FastInterruptDisableLock dLock;
    // If a previous DMA xfer is in progress, wait
    if(dmaTxInProgress)
    {
        txWaiting=Thread::IRQgetCurrentThread();
        do {
            Thread::IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        } while(txWaiting);
    }
}

void STM32Serial::writeDma(const char *buffer, size_t size)
{
    markBufferBeforeDmaWrite(buffer,size);
    //Quirk: DMA messes up the TC bit, and causes waitSerialTxFifoEmpty() to
    //return prematurely, causing characters to be missed when rebooting
    //immediatley a write. You can just clear the bit manually, but doing that
    //is dangerous, as if you clear the bit but for any reason the serial
    //write doesn't start (think an invalid buffer, or another thread crashing),
    //then TC will never be set and waitSerialTxFifoEmpty() deadlocks!
    //The only way to clear it safely is to first read SR and then write to
    //DR (thus the bit is cleared at the same time a transmission is started,
    //and the race condition is eliminated). This is the purpose of this
    //instruction, it reads SR. When we start the DMA, the DMA controller
    //writes to DR and completes the TC clear sequence.
    DeepSleepLock dpLock;
    #if !defined(_ARCH_CORTEXM7_STM32F7) && !defined(_ARCH_CORTEXM7_STM32H7) \
     && !defined(_ARCH_CORTEXM0_STM32F0)   && !defined(_ARCH_CORTEXM4_STM32F3) \
     && !defined(_ARCH_CORTEXM4_STM32L4)
    while((port->SR & USART_SR_TXE)==0) ;
    #else //_ARCH_CORTEXM7_STM32F7/H7
    while((port->ISR & USART_ISR_TXE)==0) ;
    #endif //_ARCH_CORTEXM7_STM32F7/H7
    
    dmaTxInProgress=true;
    #if defined(_ARCH_CORTEXM3_STM32F1)
    dmaTx->CPAR=reinterpret_cast<unsigned int>(&port->DR);
    dmaTx->CMAR=reinterpret_cast<unsigned int>(buffer);
    dmaTx->CNDTR=size;
    dmaTx->CCR=DMA_CCR4_MINC  //Increment RAM pointer
             | DMA_CCR4_DIR   //Memory to peripheral
             | DMA_CCR4_TEIE  //Interrupt on transfer error
             | DMA_CCR4_TCIE  //Interrupt on transfer complete
             | DMA_CCR4_EN;   //Start DMA
    #else
    #if defined(_ARCH_CORTEXM4_STM32F3) || defined(_ARCH_CORTEXM4_STM32L4)
    dmaTx->CPAR=reinterpret_cast<unsigned int>(&port->TDR);
    dmaTx->CMAR=reinterpret_cast<unsigned int>(buffer);
    dmaTx->CNDTR=size;
    dmaTx->CCR=DMA_CCR_MINC  //Increment RAM pointer
             | DMA_CCR_DIR   //Memory to peripheral
             | DMA_CCR_TEIE  //Interrupt on transfer error
             | DMA_CCR_TCIE  //Interrupt on transfer complete
             | DMA_CCR_EN;   //Start DMA
    #else  //_ARCH_CORTEXM4_STM32F3
    #if !defined(_ARCH_CORTEXM7_STM32F7) && !defined(_ARCH_CORTEXM7_STM32H7) \
     && !defined(_ARCH_CORTEXM0_STM32F0)
    dmaTx->PAR=reinterpret_cast<unsigned int>(&port->DR);
    #else //_ARCH_CORTEXM7_STM32F7/H7
    dmaTx->PAR=reinterpret_cast<unsigned int>(&port->TDR);
    #endif //_ARCH_CORTEXM7_STM32F7/H7
    dmaTx->M0AR=reinterpret_cast<unsigned int>(buffer);
    dmaTx->NDTR=size;
    //Quirk: not enabling DMA_SxFCR_FEIE because the USART seems to
    //generate a spurious fifo error. The code was tested and the
    //transfer completes successfully even in the presence of this fifo
    //error
    dmaTx->FCR=DMA_SxFCR_DMDIS;//Enable fifo
    dmaTx->CR=DMA_SxCR_CHSEL_2 //Select channel 4 (USART_TX)
                   | DMA_SxCR_MINC    //Increment RAM pointer
                   | DMA_SxCR_DIR_0   //Memory to peripheral
                   | DMA_SxCR_TCIE    //Interrupt on completion
                   | DMA_SxCR_TEIE    //Interrupt on transfer error
                   | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                   | DMA_SxCR_EN;     //Start the DMA
    #endif //_ARCH_CORTEXM4_STM32F3
    #endif //_ARCH_CORTEXM3_STM32F1
}

void STM32Serial::IRQreadDma()
{
    int elem=IRQdmaReadStop();
    markBufferAfterDmaRead(rxBuffer,rxQueueMin);
    for(int i=0;i<elem;i++)
        if(rxQueue.tryPut(rxBuffer[i])==false) /*fifo overflow*/;
    IRQdmaReadStart();
}

void STM32Serial::IRQdmaReadStart()
{
    #if defined(_ARCH_CORTEXM3_STM32F1)
    dmaRx->CPAR=reinterpret_cast<unsigned int>(&port->DR);
    dmaRx->CMAR=reinterpret_cast<unsigned int>(rxBuffer);
    dmaRx->CNDTR=rxQueueMin;
    dmaRx->CCR=DMA_CCR4_MINC  //Increment RAM pointer
             | 0              //Peripheral to memory
             | DMA_CCR4_TEIE  //Interrupt on transfer error
             | DMA_CCR4_TCIE  //Interrupt on transfer complete
             | DMA_CCR4_EN;   //Start DMA
    #else
    #if defined(_ARCH_CORTEXM4_STM32F3) || defined(_ARCH_CORTEXM4_STM32L4)
    dmaRx->CPAR=reinterpret_cast<unsigned int>(&port->RDR);
    dmaRx->CMAR=reinterpret_cast<unsigned int>(rxBuffer);
    dmaRx->CNDTR=rxQueueMin;
    dmaRx->CCR=DMA_CCR_MINC  //Increment RAM pointer
             | 0              //Peripheral to memory
             | DMA_CCR_TEIE  //Interrupt on transfer error
             | DMA_CCR_TCIE  //Interrupt on transfer complete
             | DMA_CCR_EN;   //Start DMA
    #else //_ARCH_CORTEXM4_STM32F3
    #if !defined(_ARCH_CORTEXM7_STM32F7) && !defined(_ARCH_CORTEXM7_STM32H7) \
     && !defined(_ARCH_CORTEXM0_STM32F0)
    dmaRx->PAR=reinterpret_cast<unsigned int>(&port->DR);
    #else //_ARCH_CORTEXM7_STM32F7/H7
    dmaRx->PAR=reinterpret_cast<unsigned int>(&port->RDR);
    #endif //_ARCH_CORTEXM7_STM32F7/H7
    dmaRx->M0AR=reinterpret_cast<unsigned int>(rxBuffer);
    dmaRx->NDTR=rxQueueMin;
    dmaRx->CR=DMA_SxCR_CHSEL_2 //Select channel 4 (USART_RX)
                   | DMA_SxCR_MINC    //Increment RAM pointer
                   | 0                //Peripheral to memory
                   | DMA_SxCR_HTIE    //Interrupt on half transfer
                   | DMA_SxCR_TEIE    //Interrupt on transfer error
                   | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                   | DMA_SxCR_EN;     //Start the DMA
    #endif //_ARCH_CORTEXM4_STM32F3
    #endif //_ARCH_CORTEXM3_STM32F1
}

int STM32Serial::IRQdmaReadStop()
{
    #if defined(_ARCH_CORTEXM3_STM32F1) || defined(_ARCH_CORTEXM4_STM32F3) \
     || defined(_ARCH_CORTEXM4_STM32L4)
    dmaRx->CCR=0;
    static const unsigned int irqMask[]=
    {
        DMA_IFCR_CGIF5,
        DMA_IFCR_CGIF6,
        DMA_IFCR_CGIF3   
    };
    DMA1->IFCR=irqMask[getId()-1];
    return rxQueueMin-dmaRx->CNDTR;
    #else //_ARCH_CORTEXM3_STM32F1
    //Stop DMA and wait for it to actually stop
    dmaRx->CR &= ~DMA_SxCR_EN;
    while(dmaRx->CR & DMA_SxCR_EN) ;
    static const unsigned int irqMask[]=
    {
        (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5),
        (DMA_HIFCR_CTCIF5 | DMA_HIFCR_CHTIF5 | DMA_HIFCR_CTEIF5 | DMA_HIFCR_CDMEIF5 | DMA_HIFCR_CFEIF5),
        (DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1)
    };
    static volatile unsigned long * const irqRegs[]=
    {
        &DMA2->HIFCR,
        &DMA1->HIFCR,
        &DMA1->LIFCR
    };
    *irqRegs[getId()-1]=irqMask[getId()-1];
    return rxQueueMin-dmaRx->NDTR;
    #endif //_ARCH_CORTEXM3_STM32F1
}
#endif //SERIAL_DMA

} //namespace miosix
