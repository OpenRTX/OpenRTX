/***************************************************************************
 *   Copyright (C) 2020, 2021 by Terraneo Federico                         *
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

#include "clock.h"
#include "interfaces/arch_registers.h"
#include "board_settings.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


unsigned int SystemCoreClock = miosix::bootClock;

static void configureRcfast(int frange)
{
    auto tempRcFastCfg = SCIF->SCIF_RCFASTCFG;
    tempRcFastCfg &= ~SCIF_RCFASTCFG_FRANGE_Msk;
    tempRcFastCfg |= SCIF_RCFASTCFG_FRANGE(frange) | SCIF_RCFASTCFG_EN;
    SCIF->SCIF_UNLOCK = SCIF_UNLOCK_KEY(0xaa) | SCIF_UNLOCK_ADDR(SCIF_RCFASTCFG_OFFSET);
    SCIF->SCIF_RCFASTCFG = tempRcFastCfg; //Can't do read-modify-write, confuses the lock
    while((SCIF->SCIF_RCFASTCFG & SCIF_RCFASTCFG_EN)==0) ;
    PM->PM_UNLOCK = PM_UNLOCK_KEY(0xaa) | PM_UNLOCK_ADDR(PM_MCCTRL_OFFSET);
    PM->PM_MCCTRL=PM_MCCTRL_MCSEL_RCFAST;
}

void SystemInit()
{
    //TODO: support more clock options in SystemInit() and getSelectedOscillator()
    switch(miosix::bootClock)
    {
        case 4000000:
            configureRcfast(0);
            break;
        case 8000000:
            configureRcfast(1);
            break;
        case 12000000:
            configureRcfast(2);  
            break;
    }
}

#ifdef __cplusplus
}
#endif //__cplusplus

namespace miosix {

int getSelectedOscillator()
{
    //see Table 13-8 Generic Clock Sources
    switch(SystemCoreClock)
    {
        case 4000000:  return 5; //RCFAST
        case 8000000:  return 5; //RCFAST
        case 12000000: return 5; //RCFAST
        default: return 0; //RCSYS
    }
}

void start32kHzOscillator()
{
#ifndef USE_RC_32K_OSCILLATOR
    //NOTE: at least with the 32kHz crystal I've tested (CL=12.5pF), this
    //oscillator has a very noticeable jitter. Triggering with a scope on the
    //rising edge, you can see it by zooming on the falling edge. Using the
    //maximum current of 425nA reduced the jitter, but it is still ~200ns!
    //Amplitude controlled mode is worse, don't use it.
    BSCIF->BSCIF_OSCCTRL32 = BSCIF_OSCCTRL32_STARTUP(4)  //64K cycles startup
                           | BSCIF_OSCCTRL32_SELCURR(15) //425nA (max)
                           | BSCIF_OSCCTRL32_MODE(1)     //Crystal mode
                           | BSCIF_OSCCTRL32_EN1K
                           | BSCIF_OSCCTRL32_EN32K
                           | BSCIF_OSCCTRL32_OSC32EN;
    while((BSCIF->BSCIF_PCLKSR & BSCIF_PCLKSR_OSC32RDY) == 0) ;
//     //Output OSC32K on PA2/GCLK0 for measurement purpose
//     SCIF->SCIF_GCCTRL[0].SCIF_GCCTRL = SCIF_GCCTRL_OSCSEL(1) //Output OSC32K
//                                      | SCIF_GCCTRL_CEN;
//     using gclk0 = Gpio<GPIOA_BASE,2>;
//     gclk0::mode(Mode::ALTERNATE);
//     gclk0::alternateFunction('A');
#else //USE_RC_32K_OSCILLATOR
    //Enable RC 32kHz oscillator
    BSCIF->BSCIF_UNLOCK = BSCIF_UNLOCK_KEY(0xaa)
                        | BSCIF_UNLOCK_ADDR(BSCIF_RC32KCR_OFFSET);
    BSCIF->BSCIF_RC32KCR = BSCIF_RC32KCR_EN1K
                         | BSCIF_RC32KCR_EN32K
                         | BSCIF_RC32KCR_TCEN
                         | BSCIF_RC32KCR_EN;
    while((BSCIF->BSCIF_PCLKSR & BSCIF_PCLKSR_RC32KRDY) == 0) ;
    //Select RC 32kHz oscillator
    auto tempPmcon = BPM->BPM_PMCON | BPM_PMCON_CK32S;
    BPM->BPM_UNLOCK = BPM_UNLOCK_KEY(0xaa)
                    | BPM_UNLOCK_ADDR(BPM_PMCON_OFFSET);
    BPM->BPM_PMCON = tempPmcon; //Can't do read-modify-write, confuses the lock
#endif
}

} //namespace miosix
