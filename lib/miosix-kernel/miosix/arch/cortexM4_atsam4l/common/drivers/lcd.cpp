/***************************************************************************
 *   Copyright (C) 2020 by Terraneo Federico                               *
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
 *   by the GNU General Public License. However the suorce code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "lcd.h"
#include "kernel/kernel.h"
#include <algorithm>

using namespace std;

namespace miosix {

void initLcd(unsigned int numSegments, unsigned int contrast)
{
    //Although the hardware supports up to 40 segments, this driver is currently
    //limited to 32, as we don't handle LCDCA_DRH registers.
    numSegments = min(numSegments, 32u);
    //Don't set higher than 31 (FCST field in peripheral register is signed and
    //going negative causes peripheral startup issues)
    contrast = min(contrast, 31u);
    
    {
        FastInterruptDisableLock dLock;
        PM->PM_UNLOCK = PM_UNLOCK_KEY(0xaa)
                    | PM_UNLOCK_ADDR(PM_PBAMASK_OFFSET);
        PM->PM_PBAMASK |= PM_PBAMASK_LCDCA;
    }

    //Make sure LCD is disabled before changing settings
    LCDCA->LCDCA_CR = LCDCA_CR_DIS
                    | LCDCA_CR_FC2DIS
                    | LCDCA_CR_BSTOP;
    
    LCDCA->LCDCA_BCFG = LCDCA_BCFG_FCS(2);  //Using FC2 for blinking
    
    LCDCA->LCDCA_TIM = LCDCA_TIM_CLKDIV(3)
                     | LCDCA_TIM_PRESC;
    
    LCDCA->LCDCA_CFG = LCDCA_CFG_NSU(numSegments) //LCD number of segments
                     | LCDCA_CFG_FCST(contrast)
                     | LCDCA_CFG_DUTY(0);         //1/4 duty, 1/3 bias, 4 COM

    LCDCA->LCDCA_CR = LCDCA_CR_EN;
    LCDCA->LCDCA_CR = LCDCA_CR_CDM;
    while((LCDCA->LCDCA_SR & LCDCA_SR_EN) == 0) ;
}

void setSegment(unsigned int com, unsigned int seg, int value)
{
    com = min(com, 3u);
    seg = min(seg, 31u);

    auto reg = &LCDCA->LCDCA_DRL0;
    if(value) reg[2 * com] |=   1 << seg;
    else      reg[2 * com] &= ~(1 << seg);
}

void setDigit(unsigned int digit, unsigned char segments)
{
    digit = min(digit, 15u);

    unsigned int base;
    switch(digit)
    {
        case 0:  base = 2; break; //Digit 0 and 1 are swapped
        case 1:  base = 0; break; //to simplify PCB routing
        default: base = 2 * digit;
    }
    //NOTE: the way segments are wired does not allow to use the character
    //generator, otherwise the code would be like this
    //LCDCA->LCDCA_CMCFG = LCDCA_CMCFG_STSEG(base) | LCDCA_CMCFG_TDG(1);
    //LCDCA->LCDCA_CMDR = segments;
    
    // Mapping:
    //  0    1    2    3    4    5    6    7     bits of the segments variable
    //  a    b    c    d    e    f    g   dp
    //               base               base+1   bits to change in DRL0
    //         base+1     base                   bits to change in DRL1
    //                         base base+1       bits to change in DRL2
    //base base+1                                bits to change in DRL3
    unsigned int s = segments;
    LCDCA->LCDCA_DRL0 = (LCDCA->LCDCA_DRL0 & ~(0b11 << base))
                      | ((((s >> 3) & 0b01) | ((s >> 6) & 0b10)) << base);
    LCDCA->LCDCA_DRL1 = (LCDCA->LCDCA_DRL1 & ~(0b11 << base))
                      | ((((s >> 4) & 0b01) | ((s >> 1) & 0b10)) << base);
    LCDCA->LCDCA_DRL2 = (LCDCA->LCDCA_DRL2 & ~(0b11 << base))
                      | (((segments >> 5) & 0b11) << base);
    LCDCA->LCDCA_DRL3 = (LCDCA->LCDCA_DRL3 & ~(0b11 << base))
                      | ((segments & 0b11) << base);
}

const unsigned char digitTbl[] =
{
    // 0    1    2    3    4    5    6    7    8    9
    0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
};

void enableBlink(bool fast)
{
    LCDCA->LCDCA_CR = LCDCA_CR_FC2DIS
                    | LCDCA_CR_BSTOP;
    
    LCDCA->LCDCA_TIM = (LCDCA->LCDCA_TIM & ~LCDCA_TIM_FC2_Msk)
                     | LCDCA_TIM_FC2(fast ? 1 : 2); //Select ~500ms / ~1s rate
                    
    LCDCA->LCDCA_CR = LCDCA_CR_FC2EN
                    | LCDCA_CR_BSTART;
}

} //namespace miosix
