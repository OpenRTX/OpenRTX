/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   Adapted from STM32 USART driver for miosix kernel written by Federico *
 *   Terraneo.                                                             *
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
 *                                                                         *
 *   (2025) Added and modified by KD0OSS                                   *
 ***************************************************************************/

#include <peripherals/gpio.h>
#include <pthread.h>
#include "USART3_MOD17.h"
#include "stm32f4xx.h"

#define U3TXD GPIOC,10
#define U3RXD GPIOC,11

/*!< USART CR1 register clear Mask ((~(uint16_t)0xE9F3)) */
#define CR1_CLEAR_MASK            ((uint16_t)(USART_CR1_M | USART_CR1_PCE | \
                                              USART_CR1_PS | USART_CR1_TE | \
                                              USART_CR1_RE))

/*!< USART CR2 register clock bits clear Mask ((~(uint16_t)0xF0FF)) */
#define CR2_CLOCK_CLEAR_MASK      ((uint16_t)(USART_CR2_CLKEN | USART_CR2_CPOL | \
                                              USART_CR2_CPHA | USART_CR2_LBCL))

/*!< USART CR3 register clear Mask ((~(uint16_t)0xFCFF)) */
#define CR3_CLEAR_MASK            ((uint16_t)(USART_CR3_RTSE | USART_CR3_CTSE | USART_CR3_ONEBIT))

/**
 * \internal
 * Wait until all characters have been written to the serial port.
 */
inline void waitSerialTxFifoEmpty()
{
    while((USART3->SR & USART_SR_TC) == 0) ;
}

void usart3_mod17_init(unsigned int baudrate)
{
    gpio_setMode(U3RXD, ALTERNATE | ALTERNATE_FUNC(7));
    gpio_setMode(U3TXD, ALTERNATE | ALTERNATE_FUNC(7));

    uint32_t tmpreg = 0x00;

    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    __DSB();

    /* Clear STOP[13:12] bits */
    USART3->CR2 &= (uint32_t)~((uint32_t)USART_CR2_STOP);

    tmpreg = USART3->CR1;

    /* Clear M, PCE, PS, TE and RE bits */
    tmpreg &= (uint32_t)~((uint32_t)CR1_CLEAR_MASK);
    tmpreg |= (uint32_t)(4 | 8);
    USART3->CR1 = (uint16_t)tmpreg | USART_CR1_UE;

    /* Clear CTSE and RTSE bits */
    USART3->CR3 &= (uint32_t)~((uint32_t)CR3_CLEAR_MASK);

    unsigned int freq = SystemCoreClock;
    if(RCC->CFGR & RCC_CFGR_PPRE1_2) freq/= 1<<(((RCC->CFGR >> 10) & 0x3)+1);

    const unsigned int quot = 2*freq/baudrate; // 2*freq for round to nearest
    USART3->BRR  = quot/2 + (quot & 1);        // Round to nearest
}

void usart3_mod17_terminate()
{
    waitSerialTxFifoEmpty();

    USART3->CR1  &= ~USART_CR1_UE;
    RCC->APB1ENR &= ~RCC_APB1ENR_USART3EN;
    __DSB();
}

ssize_t usart3_mod17_readBlock(void *buffer, size_t size)
{
    char *buf = reinterpret_cast< char* >(buffer);
    size_t result = 0;

    for(;;)
    {
        //for(; result < size; result++)
        {
            unsigned int status = USART3->SR;
            char c;

            if (status & USART_SR_RXNE)
            {
                //Always read data, since this clears interrupt flags
                c = USART3->DR;

                //If no error put data in buffer
                if ((status & USART_SR_FE) == 0)
                {
//                    if (c == 10) break;
                    *buf++ = c & (uint16_t)0x01FF;
                    result++;
//                    if (result == size) break;
                }

//                rxIdle1 = false;
            }
            else
                break;
        }

      //  if(!rxIdle1 && result == size) break;
      //  if(result == size)
      //  {
      //      result = -1;
      //      break;
      //  }
    }
    return result;
}

ssize_t usart3_mod17_writeBlock(void *buffer, size_t size)
{
    const char *buf = reinterpret_cast< const char* >(buffer);
    for(size_t i = 0; i < size; i++)
    {
        while((USART3->SR & USART_SR_TXE) == 0) ;
        USART3->DR = *buf++ & (uint16_t)0x01FF;
    }

    return size;
}
