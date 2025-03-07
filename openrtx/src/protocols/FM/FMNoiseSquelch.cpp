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
 *
 *   (2025) Modified by KD0OSS for FM mode on Module17/OpenRTX
 */

//#if defined(MODE_FM)

#include <FMNoiseSquelch.h>
#include "string.h"
#include "stdio.h"
//#include <drivers/USART3_MOD17.h> // for debugging

// 4500Hz centre frequency
const q31_t COEFF_DIV_TWO = 821806413;

// 400Hz bandwidth
const uint16_t N = 24000U / 400U;

CFMNoiseSquelch::CFMNoiseSquelch() :
m_highThreshold(0),
m_lowThreshold(0),
m_count(0U),
m_q0(0),
m_q1(0),
m_state(false),
m_validCount(0U)
{

}

// Added by KD0OSS
void CFMNoiseSquelch::DEBUG3(const char* text, int16_t n1, int16_t n2)
{
	char buf[80];
	sprintf(buf, "%s:  %d  %d\n", text, (int)n1, (int)n2);
//	usart3_mod17_writeBlock((void*)buf, strlen(buf));
}

void CFMNoiseSquelch::DEBUG4(const char *text, q31_t value, q31_t thresh, bool state)
{
	char buf[80];
	sprintf(buf, "%s:  %d  %d  %d\n", text, (int)value, (int)thresh, (int)state);
//	usart3_mod17_writeBlock((void*)buf, strlen(buf));
}

void CFMNoiseSquelch::setParams(uint8_t highThreshold, uint8_t lowThreshold)
{
  m_highThreshold = q31_t(highThreshold * 500);
  m_lowThreshold  = q31_t(lowThreshold * 500);
  m_invalidCount  = 0U;
}

bool CFMNoiseSquelch::process(q15_t sample)
{
  //get more dynamic into the decoder by multiplying the sample by 64
  q31_t sample31 = q31_t(sample) << 6; //+  (q31_t(sample) >> 1);

  q31_t q2 = m_q1;
  m_q1 = m_q0;

  // Q31 multiplication, t3 = m_coeffDivTwo * 2 * m_q1
  q63_t t1 = COEFF_DIV_TWO * m_q1;
  q31_t t2 = __SSAT((t1 >> 31), 31);
  q31_t t3 = t2 * 2;

  // m_q0 = m_coeffDivTwo * m_q1 * 2 - q2 + sample
  m_q0 = t3 - q2 + sample31;

  m_count++;
  if (m_count == N) {
    // Q31 multiplication, t2 = m_q0 * m_q0
    q63_t t1 = q63_t(m_q0) * q63_t(m_q0);
    q31_t t2 = __SSAT((t1 >> 31), 31);

    // Q31 multiplication, t4 = m_q0 * m_q0
    q63_t t3 = q63_t(m_q1) * q63_t(m_q1);
    q31_t t4 = __SSAT((t3 >> 31), 31);

    // Q31 multiplication, t9 = m_q0 * m_q1 * m_coeffDivTwo * 2
    q63_t t5 = q63_t(m_q0) * q63_t(m_q1);
    q31_t t6 = __SSAT((t5 >> 31), 31);
    q63_t t7 = t6 * COEFF_DIV_TWO;
    q31_t t8 = __SSAT((t7 >> 31), 31);
    q31_t t9  = t8 * 2;

    // value = m_q0 * m_q0 + m_q1 * m_q1 - m_q0 * m_q1 * m_coeffDivTwo * 2
    q31_t value = t2 + t4 - t9;

    bool previousState = m_state;

    q31_t threshold = m_highThreshold;
    if (previousState)
      threshold = m_lowThreshold;

    if (!m_state) {
      if (value < threshold)
        m_validCount++;
      else
        m_validCount = 0U;
    }

    if (m_state) {
      if (value >= threshold)
        m_invalidCount++;
      else
        m_invalidCount = 0U;
    }

    m_state = m_validCount >= 10U && m_invalidCount < 10U;

    if(previousState && !m_state)
      m_invalidCount = 0U;

//    if (previousState != m_state) {
//      DEBUG4("Noise Squelch Value / Threshold / Valid", value, threshold, m_state);
//      DEBUG3("Valid Count / Invalid Count", m_validCount, m_invalidCount);
//    }

    m_count = 0U;
    m_q0 = 0;
    m_q1 = 0;
  }

  return m_state;
}

void CFMNoiseSquelch::reset()
{
  m_q0 = 0;
  m_q1 = 0;
  m_state = false;
  m_count = 0U;
}

//#endif

