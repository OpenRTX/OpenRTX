/***************************************************************************
 *   Copyright (C) 2014 by Terraneo Federico                               *
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

#ifndef SD_STM32L4_H
#define	SD_STM32L4_H

#include "kernel/sync.h"
#include "filesystem/devfs/devfs.h"
#include "filesystem/ioctl.h"

namespace miosix {

/**
 * Driver for the SDIO peripheral in STM32F2 and F4 microcontrollers
 */
class SDIODriver : public Device
{
public:
    /**
     * \return an instance to this class, singleton
     */
    static intrusive_ref_ptr<SDIODriver> instance();
    
    virtual ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    virtual ssize_t writeBlock(const void *buffer, size_t size, off_t where);
    
    virtual int ioctl(int cmd, void *arg);
private:
    /**
     * Constructor
     */
    SDIODriver();
    
    FastMutex mutex;
};

} //namespace miosix

#endif //SD_STM32L4_H