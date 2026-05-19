/***************************************************************************
 *   Copyright (C) 2012, 2013, 2014, 2015, 2016 by Terraneo Federico       *
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

#include "e20.h"

using namespace std;

namespace miosix {

//
// Class EventQueue
//

void EventQueue::post(function<void ()> event)
{
    Lock<FastMutex> l(m);
    events.push_back(event);
    cv.signal();
}

void EventQueue::run()
{
    Lock<FastMutex> l(m);
    for(;;)
    {
        while(events.empty()) cv.wait(l);
        function<void ()> f=events.front();
        events.pop_front();
        {
            Unlock<FastMutex> u(l);
            f();
        }
    }
}

void EventQueue::runOne()
{
    function<void ()> f;
    {
        Lock<FastMutex> l(m);
        if(events.empty()) return;
        f=events.front();
        events.pop_front();
    }
    f();
}

} //namespace miosix
