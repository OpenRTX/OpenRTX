/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef HRC6000_H
#define HRC6000_H

#include "HR_Cx000.h"

enum class C6000_SpiOpModes : uint8_t
{
    AUX    = 1,     ///< Auxiliary configuration registers.
    DATA   = 2,     ///< Write TX data register and read RX data register.
    SOUND  = 3,     ///< Voice prompt sample register.
    CONFIG = 4,     ///< Main configuration registers.
    AMBE3K = 5,     ///< AMBE3000 configuration register.
    DATA_R = 6,     ///< Write RX data register and read TX data register.
    AMBE1K = 7      ///< AMBE1000 configuration register.
};

class HR_C6000 : public HR_Cx000 < C6000_SpiOpModes >
{
public:

    /**
     * Constructor.
     *
     * @param uSpi: pointer to SPI device for "user" SPI interface.
     * @param uCs: gpioPin object for "user" SPI chip select.
     */
    HR_C6000(const struct spiDevice *uSpi, const struct gpioPin uCs) :
        HR_Cx000< C6000_SpiOpModes >(uSpi, uCs) { }
};

#endif /* HRC6000_H */
