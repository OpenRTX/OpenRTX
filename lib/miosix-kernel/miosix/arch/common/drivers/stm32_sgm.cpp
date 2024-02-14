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

#include "stm32_sgm.h"
#include <string.h>
#include "miosix.h"

namespace miosix {

extern unsigned char _preserve_start asm("_preserve_start");
extern unsigned char _preserve_end asm("_preserve_end");

static unsigned char *preserve_start=&_preserve_start;
static unsigned char *preserve_end=&_preserve_end;

SGM& SGM::instance()
{
    static SGM singleton;
    return singleton;
}

SGM::SGM()
{
    /* Enable PWR clock */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    
    /* Enable backup SRAM Clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_BKPSRAMEN;

    enableWrite();

    /* Enable Backup regulator */
    PWR->CSR |= PWR_CSR_BRE;  

    /* Enable the Backup SRAM low power Regulator */
    PWR->CSR |= PWR_CSR_BRE;

    /* Wait for backup regulator */
    while (!(PWR->CSR & (PWR_CSR_BRR)));

    /* Retrive last reset reason and clear the pending flag */
    readResetRegister();

    /* 
     * If the reset wasn't caused by software failure we cannot trust
     * the content of the backup memory and we need to clear it.
     */
    if(lastReset != RST_SW)
    {
        memset(preserve_start, 0, preserve_end-preserve_start);
    }
}

void SGM::disableWrite()
{
    /* Enable Backup Domain write protection */
    PWR->CR &= ~PWR_CR_DBP;
}

void SGM::enableWrite()
{
    /* Disable Backup Domain write protection */
     PWR->CR |= PWR_CR_DBP; 
}

void SGM::clearResetFlag()
{
    RCC->CSR |= RCC_CSR_RMVF;
}

void SGM::readResetRegister()
{
    uint32_t resetReg = RCC->CSR;
    clearResetFlag();
    if(resetReg & RCC_CSR_LPWRRSTF)
    {
        lastReset = RST_LOW_PWR;
    }
    else if( resetReg & RCC_CSR_WWDGRSTF)
    {
        lastReset = RST_WINDOW_WDG;
    }
    else if( resetReg & RCC_CSR_WDGRSTF)
    {
        lastReset = RST_INDEPENDENT_WDG;
    }
    else if( resetReg & RCC_CSR_SFTRSTF)
    {
        lastReset = RST_SW;
    }
    else if( resetReg & RCC_CSR_PORRSTF)
    {
        lastReset = RST_POWER_ON;
    }
    else if( resetReg & RCC_CSR_PADRSTF)
    {
        lastReset = RST_PIN;
    }
    else
    {
        lastReset = RST_UNKNOWN;
    }
}

} // namespace miosix
