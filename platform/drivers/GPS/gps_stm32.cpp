/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"
#include "rcc.h"
#include "nmea_rbuf.h"
#include "gps_stm32.h"

#if defined(CONFIG_GPS_STM32_USART1)
    #define PORT        USART1
    #define IRQn        USART1_IRQn
    #define IRQHandler  USART1_IRQHandler
    #define CLK_EN()    (RCC->APB2ENR |= RCC_APB2ENR_USART1EN);
    #define CLK_DIS()   (RCC->APB2ENR &= ~RCC_APB2ENR_USART1EN);
#elif defined(CONFIG_GPS_STM32_USART2)
    #define PORT        USART2
    #define IRQn        USART2_IRQn
    #define IRQHandler  USART2_IRQHandler
    #define CLK_EN()    (RCC->APB1ENR |= RCC_APB1ENR_USART2EN);
    #define CLK_DIS()   (RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN);
#elif defined(CONFIG_GPS_STM32_USART3)
    #define PORT        USART3
    #define IRQn        USART3_IRQn
    #define IRQHandler  USART3_IRQHandler
    #define CLK_EN()    (RCC->APB1ENR |= RCC_APB1ENR_USART3EN);
    #define CLK_DIS()   (RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN);
#elif defined(CONFIG_GPS_STM32_UART4)
    #define PORT        UART4
    #define IRQn        UART4_IRQn
    #define IRQHandler  UART4_IRQHandler
    #define CLK_EN()    (RCC->APB1ENR |= RCC_APB1ENR_UART4EN);
    #define CLK_DIS()   (RCC->APB1ENR &= ~RCC_APB1ENR_UART4EN);
#elif defined(CONFIG_GPS_STM32_UART5)
    #define PORT        UART5
    #define IRQn        UART5_IRQn
    #define IRQHandler  UART5_IRQHandler
    #define CLK_EN()    (RCC->APB1ENR |= RCC_APB1ENR_UART5EN);
    #define CLK_DIS()   (RCC->APB1ENR &= ~RCC_APB1ENR_UART5EN);
#elif defined(CONFIG_GPS_STM32_USART6)
    #define PORT        USART6
    #define IRQn        USART6_IRQn
    #define IRQHandler  USART6_IRQHandler
    #define CLK_EN()    (RCC->APB2ENR |= RCC_APB2ENR_USART6EN);
    #define CLK_DIS()   (RCC->APB2ENR &= ~RCC_APB2ENR_USART6EN);
#elif defined(CONFIG_GPS_STM32_UART7)
    #define PORT        UART7
    #define IRQn        UART7_IRQn
    #define IRQHandler  UART7_IRQHandler
    #define CLK_EN()    (RCC->APB1ENR |= RCC_APB1ENR_UART7EN);
    #define CLK_DIS()   (RCC->APB1ENR &= ~RCC_APB1ENR_UART7EN);
#elif defined(CONFIG_GPS_STM32_UART8)
    #define PORT        UART8
    #define IRQn        UART8_IRQn
    #define IRQHandler  UART8_IRQHandler
    #define CLK_EN()    (RCC->APB1ENR |= RCC_APB1ENR_UART8EN);
    #define CLK_DIS()   (RCC->APB1ENR &= ~RCC_APB1ENR_UART8EN);
#else
    #error No USART defined!
#endif

static nmeaRbuf nmea;

void IRQHandler()
{
    #if defined(STM32H743xx)
    if(PORT->ISR & USART_ISR_RXNE_RXFNE) {
        PORT->ISR = 0;
        nmeaRbuf_putChar(&nmea, PORT->RDR);
    }
    #else
    if(PORT->SR & USART_SR_RXNE) {
        PORT->SR = 0;
        nmeaRbuf_putChar(&nmea, PORT->DR);
    }
    #endif
}

void gpsStm32_init(const uint16_t baud)
{
    CLK_EN();
    __DSB();

    uint32_t quot = rcc_getPeriphClock(PORT);
    quot = (2 * quot) / baud;

    PORT->BRR  = (quot / 2) + (quot & 1);
    PORT->CR3 |= USART_CR3_ONEBIT;
    PORT->CR1  = USART_CR1_RE
               | USART_CR1_RXNEIE;

    NVIC_SetPriority(IRQn, 14);

    nmeaRbuf_reset(&nmea);
}

void gpsStm32_terminate()
{
    gpsStm32_disable(NULL);
    CLK_DIS();
}

void gpsStm32_enable(void *priv)
{
    (void) priv;

    PORT->CR1 |= USART_CR1_UE;

    NVIC_ClearPendingIRQ(IRQn);
    NVIC_EnableIRQ(IRQn);
}

void gpsStm32_disable(void *priv)
{
    (void) priv;

    PORT->CR1 &= ~USART_CR1_UE;
    NVIC_DisableIRQ(USART6_IRQn);
}

int gpsStm32_getNmeaSentence(void *priv, char *buf, const size_t maxLength)
{
    (void) priv;

    return nmeaRbuf_getSentence(&nmea, buf, maxLength);
}
