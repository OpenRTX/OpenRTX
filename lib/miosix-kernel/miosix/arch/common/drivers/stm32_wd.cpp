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

#include "stm32_wd.h"
#include "miosix.h"
#include <algorithm>

using namespace std;

namespace miosix {

IWatchDog& IWatchDog::instance()
{
    static IWatchDog singleton;
    return singleton;
}

void IWatchDog::enable(int ms)
{
    /* disable WD configuration protection */
    IWDG->KR = 0x5555;

    /* set prescaler divider to /32 */
    IWDG->PR = 0x3; 

    /* set reload register */
    IWDG->RLR = max(1,min(4096,ms))-1;

    /* start the watchdog */
    IWDG->KR = 0xCCCC;
}

void IWatchDog::refresh()
{
    IWDG->KR=0xAAAA;
}

} //namespace miosix
