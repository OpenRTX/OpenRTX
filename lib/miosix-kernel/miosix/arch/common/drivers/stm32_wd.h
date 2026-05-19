/***************************************************************************
 *   Copyright (C) 2017 by Matteo Michele Piazzolla                        *
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

namespace miosix {

/**
 * Driver for the STM32 independent watchdog
 */
class IWatchDog 
{
public:
    /**
     * \return an instance of this class (singleton)
     */
    static IWatchDog& instance();
    
    /**
     * Enable the watchdog
     * \param ms reload period, from 1 (1 millisecond) to 4096 (4.096 seconds)
     */
    void enable(int ms);

    /**
     * If the watchdog is not periodically reloaded at least within the
     * period selected by enable, the microcontroller is reset.
     * The datsheet says that the oscillator clocking the watchdog can be
     * up to twice as fast, so it is recomended to reload the watchdog three
     * to four times faster to prevent spurious resets.
     */
    void refresh();

private:
    IWatchDog(const IWatchDog&)=delete;
    IWatchDog& operator=(const IWatchDog&)=delete;
    IWatchDog() {}
};

} //namespace miosix
