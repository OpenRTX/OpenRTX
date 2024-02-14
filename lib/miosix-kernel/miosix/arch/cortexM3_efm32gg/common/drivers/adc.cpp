/***************************************************************************
 *   Copyright (C) 2023 by Terraneo Federico                               *
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

#include "adc.h"
#include "miosix.h"

Adc& Adc::instance()
{
    static Adc singleton;
    return singleton;
}

void Adc::powerMode(AdcPowerMode mode)
{
    ADC0->CTRL &= ~ 0b11; //Clear WARMUPMODE field
    //NOTE: from an implementation perspective Off is the same as OnDemand
    //but from a logical perspective Off is used to turn the ADC off after it
    //has been turned on, while OnDemand is used to pay the startup cost at
    //every conversion
    if(mode==On)
    {
        ADC0->CTRL |= ADC_CTRL_WARMUPMODE_KEEPADCWARM;
        while((ADC0->STATUS & ADC_STATUS_WARM)==0) ;
    }
}

unsigned short Adc::readChannel(int channel)
{
    auto temp = ADC0->SINGLECTRL;
    temp &= ~(0xf<<8); //Clear INPUTSEL field
    temp |= (channel & 0xf)<<8;
    ADC0->SINGLECTRL = temp;
    ADC0->CMD=ADC_CMD_SINGLESTART;
    while((ADC0->STATUS & ADC_STATUS_SINGLEDV)==0) ;
    return ADC0->SINGLEDATA;
}

Adc::Adc()
{
    {
        miosix::FastInterruptDisableLock dLock;
        CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_ADC0;
    }
    ADC0->CTRL = 0b11111<<16             //TIMEBASE ??
               | (4-1)<<8                //PRESC 48/4=12MHz < 13MHz OK
               | ADC_CTRL_LPFMODE_RCFILT
               | ADC_CTRL_WARMUPMODE_NORMAL;
    //TODO: expose more options
    ADC0->SINGLECTRL = ADC_SINGLECTRL_AT_256CYCLES
                     | ADC_SINGLECTRL_REF_VDD
                     | ADC_SINGLECTRL_RES_12BIT
                     | ADC_SINGLECTRL_ADJ_RIGHT;
}
