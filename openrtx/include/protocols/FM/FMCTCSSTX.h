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

#if !defined(FMCTCSSTX_H)
#define  FMCTCSSTX_H
#include "stdint.h"
#define  ARM_MATH_CM4
#include <DSTAR/arm_math.h>

class CFMCTCSSTX {
public:
  CFMCTCSSTX();

  uint8_t setParams(uint8_t frequency, uint8_t level);

  q15_t getAudio(bool reverse);

private:
  q15_t*   m_values;
  uint16_t m_length;
  uint16_t m_n;
};

#endif

//#endif

