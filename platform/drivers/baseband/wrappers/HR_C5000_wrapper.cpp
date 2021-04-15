/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "HR_C5000_wrapper.h"
#include "HR_C5000.h"

void C5000_init()
{
    HR_C5000::instance().init();
}

void C5000_terminate()
{
    HR_C5000::instance().terminate();
}

void C5000_setModOffset(uint8_t offset)
{
    HR_C5000::instance().setModOffset(offset);
}

void C5000_setModAmplitude(uint8_t iAmp, uint8_t qAmp)
{
    HR_C5000::instance().setModAmplitude(iAmp, qAmp);
}

void C5000_setModFactor(uint8_t mf)
{
    HR_C5000::instance().setModFactor(mf);
}

void C5000_dmrMode()
{
    HR_C5000::instance().dmrMode();
}

void C5000_fmMode()
{
    HR_C5000::instance().fmMode();
}

void C5000_startAnalogTx()
{
    HR_C5000::instance().startAnalogTx(TxAudioSource::MIC, FmConfig::PREEMPH_EN |
                                                           FmConfig::BW_25kHz);
}

void C5000_stopAnalogTx()
{
    HR_C5000::instance().stopAnalogTx();
}
