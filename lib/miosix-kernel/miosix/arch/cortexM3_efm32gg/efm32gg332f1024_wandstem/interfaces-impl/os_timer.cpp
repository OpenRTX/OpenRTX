/***************************************************************************
 *   Copyright (C) 2016 by Fabiano Riccardi, Sasan                         *
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

#include "interfaces/os_timer.h"
#include "kernel/timeconversion.h"
#include "vht.h"
#include "virtual_clock.h"

using namespace miosix;

namespace miosix {

static HRTB *b=nullptr;
static TimeConversion tc;
static VHT *vht=nullptr;
static VirtualClock *vt=nullptr;

long long getTime() noexcept
{
    return tc.tick2ns(vt->uncorrected2corrected(vht->uncorrected2corrected(b->addBasicCorrection(b->getCurrentTick()))));
}

long long IRQgetTime() noexcept
{
    return tc.tick2ns(vt->uncorrected2corrected(vht->uncorrected2corrected(b->addBasicCorrection(b->IRQgetCurrentTick()))));
}

namespace internal {

void IRQosTimerInit()
{
    b=&HRTB::instance();
    tc=TimeConversion(b->getTimerFrequency());
    vht=&VHT::instance();
    vt=&VirtualClock::instance();
}

void IRQosTimerSetInterrupt(long long ns) noexcept
{
    b->IRQsetNextInterruptCS(b->removeBasicCorrection(vht->corrected2uncorrected(vt->corrected2uncorrected(tc.ns2tick(ns)))));
}

// long long ContextSwitchTimer::getNextInterrupt() const
// {
//     return tc->tick2ns(vt->uncorrected2corrected(vht->uncorrected2corrected(pImpl->b.addBasicCorrection(pImpl->b.IRQgetSetTimeCS()))));
// }

// void IRQosTimerSetTime(long long ns) noexcept
// {
//     //TODO
// }

unsigned int osTimerGetFrequency()
{
    return b->getTimerFrequency();
}

} //namespace internal

} //namespace miosix
