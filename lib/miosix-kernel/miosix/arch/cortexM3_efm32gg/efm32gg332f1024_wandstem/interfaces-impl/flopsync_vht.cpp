/***************************************************************************
 *   Copyright (C)  2013 by Terraneo Federico                              *
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

#include "flopsync_vht.h"

using namespace std;

#define SWITCHED

//
// class Flopsync1
//

FlopsyncVHT::FlopsyncVHT() { reset(); }

int FlopsyncVHT::computeCorrection(int e)
{
    //u(k)=u(k-1)+1.375*e(k)-e(k-1)
    int u=uo+11*e-8*eo;

    //The controller output needs to be quantized, but instead of simply
    //doing u/8 which rounds towards the lowest number use a slightly more
    //advanced algorithm to round towards the closest one, as when the error
    //is close to +/-1 timer tick this makes a significant difference.
    int sign=u>=0 ? 1 : -1;
    int uquant=(u+4*sign)/8;

    #ifdef SWITCHED
    //Adaptive state quantization, while the output always needs to be
    //quantized, the state is only quantized if the error is zero
    uo= e==0 ? 8*uquant : u;
    #else //SWITCHED
    uo=u;
    #endif //SWITCHED
    eo=e;
    
    return uquant;
}

int FlopsyncVHT::getClockCorrection() const
{
    int sign=uo>=0 ? 1 : -1;
    return (uo+4*sign)/8;
}
