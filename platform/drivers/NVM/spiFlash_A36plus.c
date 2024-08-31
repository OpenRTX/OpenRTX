/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <peripherals/gpio.h>
#include <hwconfig.h>
#include <stdint.h>

/*
 * Implementation of external flash SPI interface for the Talkpod A36plus.
 */

uint8_t spiFlash_SendRecv(uint8_t val)
{
    // Based on the above comment:
      while (spi_i2s_flag_get(SPI0, SPI_FLAG_TBE) == RESET)
        ;
    spi_i2s_data_transmit(SPI0, val);
    while (spi_i2s_flag_get(SPI0, SPI_FLAG_TRANS) != RESET)
        ;
    while (spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE) == RESET)
        ;
    return spi_i2s_data_receive(SPI0);
}

void spiFlash_init()
{
    #define FLASH_GPIO_PORT GPIOA
    #define FLASH_GPIO_SCK_PIN GPIO_PIN_5
    #define FLASH_GPIO_DIN_PIN GPIO_PIN_6
    #define FLASH_GPIO_DOUT_PIN GPIO_PIN_7
    #define FLASH_GPIO_CS_PIN GPIO_PIN_4
    
    gpio_af_set(FLASH_GPIO_PORT, GPIO_AF_0, FLASH_GPIO_SCK_PIN | FLASH_GPIO_DIN_PIN | FLASH_GPIO_DOUT_PIN);
    gpio_mode_set(FLASH_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, FLASH_GPIO_SCK_PIN | FLASH_GPIO_DIN_PIN | FLASH_GPIO_DOUT_PIN);
    gpio_output_options_set(FLASH_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_GPIO_SCK_PIN | FLASH_GPIO_DIN_PIN | FLASH_GPIO_DOUT_PIN);

    gpio_mode_set(FLASH_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, FLASH_GPIO_CS_PIN);
    gpio_output_options_set(FLASH_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, FLASH_GPIO_CS_PIN);
    spi_parameter_struct spi_init_struct;
    /* deinitialize SPI and the parameters */
    spi_i2s_deinit(SPI0);

    spi_struct_para_init(&spi_init_struct);

    /* configure SPI1 parameter */
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_4;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init(SPI0, &spi_init_struct);
    spi_enable(SPI0);
    return;
}

void spiFlash_terminate()
{
    gpio_setMode(FLASH_CLK, INPUT);
    gpio_setMode(FLASH_SDO, INPUT);
    gpio_setMode(FLASH_SDI, INPUT);
}
