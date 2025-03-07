/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
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
 */

//#if defined(MODE_FM)

#if !defined(FMNOISESQUELCH_H)
#define  FMNOISESQUELCH_H
#include "stdint.h"
#define  ARM_MATH_CM4
#include <DSTAR/arm_math.h>

class CFMNoiseSquelch {
public:
  CFMNoiseSquelch();

  void DEBUG3(const char* text, int16_t n1, int16_t n2);
  void DEBUG4(const char *text, q31_t value, q31_t thresh, bool state);
  void setParams(uint8_t highThreshold, uint8_t lowThreshold);
  
  bool process(q15_t sample);

  void reset();

private:
  q31_t    m_highThreshold;
  q31_t    m_lowThreshold;
  uint16_t m_count;
  q31_t    m_q0;
  q31_t    m_q1;
  bool     m_state;
  uint8_t  m_validCount;
  uint8_t  m_invalidCount;
};

#endif

//#endif

