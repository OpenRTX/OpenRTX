/***************************************************************************
 *   Copyright (C) 2017 by Terraneo Federico                               *
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
* bsp_impl.h Part of the Miosix Embedded OS.
* Board support package, this file initializes hardware.
************************************************************************/

#ifndef BSP_IMPL_H
#define BSP_IMPL_H

#include "interfaces/gpio.h"
#include "hwmapping.h"

namespace miosix {

inline void ledOn()  { redLed::high(); }
inline void ledOff() { redLed::low();  }

/**
 * \return true if the supply voltege is high enough
 */
bool lowVoltageCheck();

/**
 * This class allows to store non volatile data into the last FLASH page
 * of the microcontroller.
 */
class NonVolatileStorage
{
public:
    /**
     * \return an instance of this class
     */
    static NonVolatileStorage& instance();
    
    /**
     * \return the maximum size of the available storage
     */
    int capacity() const { return 1024; }
    
    /**
     * Erase the non voltaile storage, resetting it to all 0xff
     * \return true on success, false on failure
     */
    bool erase();
    
    /**
     * Program data into the non volatile storage
     * \param data data to write to the non-volatile storage
     * \param size size of data to write
     * \return true on success, false on failure
     */
    bool program(const void *data, int size, int offset=0);
    
    /**
     * Read back data from the non volatile storage
     * \param data data to read to the non-volatile storage
     * \param size size of data to read
     */
    void read(void *data, int size);

private:
    NonVolatileStorage() {}
    NonVolatileStorage(const NonVolatileStorage&);
    NonVolatileStorage& operator= (const NonVolatileStorage&);
    
    /**
     * Perform the unlock sequence
     * \return true on success, false on failure
     */
    bool IRQunlock();
    
    static const unsigned int baseAddress=0x0800fc00;
};

} //namespace miosix

#endif //BSP_IMPL_H
