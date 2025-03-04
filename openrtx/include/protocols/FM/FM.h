/*
 *   Copyright (C) 2020,2021 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   (2025) Modified by KD0OSS for FM mode on Module17/OpenRTX
 */

//#if defined(MODE_FM)

#if !defined(FM_H)
#define  FM_H

#include "FMCTCSSRX.h"
#include <DSTAR/RingBuffer.h>
#include "FMDirectForm1.h"
#include "FMNoiseSquelch.h"
#include "stdint.h"
#define  ARM_MATH_CM4
#include <DSTAR/arm_math.h>

class CFM {
public:
  CFM();

  void samples(bool cos, q15_t* samples, uint8_t length);
  void reset();
  uint8_t setMisc(uint8_t ctcssFrequency, uint8_t ctcssHighThreshold, uint8_t ctcssLowThreshold, uint8_t accessMode, bool cosInvert, bool noiseSquelch, uint8_t squelchHighThreshold, uint8_t squelchLowThreshold, uint8_t maxDev, uint8_t rxLevel);

  void DEBUG1(const char *text);

  bool                 m_openSq;
  bool                 m_rfSignal;

private:
  CFMCTCSSRX           m_ctcssRX;
  CFMNoiseSquelch      m_squelch;
  CFMDirectFormI       m_filterStage1;
  CFMDirectFormI       m_filterStage2;
  CFMDirectFormI       m_filterStage3;
  uint8_t              m_accessMode;
  bool                 m_cosInvert;
  bool                 m_noiseSquelch;
  q15_t                m_rxLevel;
  CRingBuffer<q15_t>   m_inputRFRB;
};

#endif

//#endif

