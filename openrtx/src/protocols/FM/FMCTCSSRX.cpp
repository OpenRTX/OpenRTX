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

#include <FMCTCSSRX.h>
//#include <drivers/USART3_MOD17.h> // for debugging
#include "string.h"
#include "stdio.h"

const struct RX_CTCSS_TABLE {
  uint8_t frequency;
  q63_t   coeffDivTwo;
} RX_CTCSS_TABLE_DATA[] = {
  { 67U, 2147153298},
  { 69U, 2147130228},
  { 71U, 2147103212},
  { 74U, 2147076297},
  { 77U, 2147047330},
  { 79U, 2147016195},
  { 82U, 2146982775},
  { 85U, 2146946945},
  { 88U, 2146907275},
  { 91U, 2146867538},
  { 94U, 2146822298},
  { 97U, 2146785526},
  {100U, 2146747759},
  {103U, 2146695349},
  {107U, 2146637984},
  {110U, 2146578604},
  {114U, 2146513835},
  {118U, 2146445080},
  {123U, 2146370355},
  {127U, 2146291161},
  {131U, 2146205372},
  {136U, 2146112589},
  {141U, 2146014479},
  {146U, 2145910829},
  {151U, 2145796971},
  {156U, 2145676831},
  {159U, 2145604646},
  {162U, 2145547790},
  {165U, 2145468230},
  {167U, 2145409363},
  {171U, 2145324517},
  {173U, 2145261046},
  {177U, 2145170643},
  {179U, 2145102321},
  {183U, 2145006080},
  {186U, 2144932648},
  {189U, 2144830280},
  {192U, 2144748638},
  {196U, 2144639788},
  {199U, 2144555290},
  {203U, 2144436713},
  {206U, 2144346237},
  {210U, 2144217348},
  {218U, 2143983951},
  {225U, 2143735870},
  {229U, 2143622139},
  {233U, 2143469001},
  {241U, 2143182299},
  {250U, 2142874683},
  {254U, 2142733729}};

const uint8_t CTCSS_TABLE_DATA_LEN = 50U;

// 4Hz bandwidth
const uint16_t N = 24000U / 4U;

CFMCTCSSRX::CFMCTCSSRX() :
m_coeffDivTwo(0),
m_highThreshold(0),
m_lowThreshold(0),
m_count(0U),
m_q0(0),
m_q1(0),
m_state(false)
{
}

// Added by KD0OSS
void CFMCTCSSRX::DEBUG4(const char *text, q31_t value, q31_t thresh, bool state)
{
	char buf[80];
	sprintf(buf, "%s:  %d  %d\n", text, (int)value, (int)state);
//	usart3_mod17_writeBlock((void*)buf, strlen(buf));
}

uint8_t CFMCTCSSRX::setParams(uint8_t frequency, uint8_t highThreshold, uint8_t lowThreshold)
{
  m_coeffDivTwo = 0;

  for (uint8_t i = 0U; i < CTCSS_TABLE_DATA_LEN; i++) {
    if (RX_CTCSS_TABLE_DATA[i].frequency == frequency) {
      m_coeffDivTwo = RX_CTCSS_TABLE_DATA[i].coeffDivTwo;
      break;
    }
  }

  if (m_coeffDivTwo == 0)
    return 4U;

  m_highThreshold = q31_t(highThreshold * 1000); // Modified by KD0OSS
  m_lowThreshold  = q31_t(lowThreshold * 1000);  // Modified by KD0OSS

  return 0U;
}

bool CFMCTCSSRX::process(q15_t sample)
{
  //get more dynamic into the decoder by multiplying the sample by 1.5
  q31_t sample31 = q31_t(sample) +  (q31_t(sample) >> 1);

  q31_t q2 = m_q1;
  m_q1 = m_q0;

  // Q31 multiplication, t3 = m_coeffDivTwo * 2 * m_q1
  q63_t t1 = m_coeffDivTwo * m_q1;
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
    q63_t t7 = t6 * m_coeffDivTwo;
    q31_t t8 = __SSAT((t7 >> 31), 31);
    q31_t t9  = t8 * 2;

    // value = m_q0 * m_q0 + m_q1 * m_q1 - m_q0 * m_q1 * m_coeffDivTwo * 2
    q31_t value = t2 + t4 - t9;

    bool previousState = m_state;

    q31_t threshold = m_highThreshold;
    if (previousState)
      threshold = m_lowThreshold;

    m_state = value >= threshold;

//    if (previousState != m_state)
 //     DEBUG4("CTCSS Value / Threshold / Valid", value, threshold, m_state);

    m_count = 0U;
    m_q0 = 0;
    m_q1 = 0;
  }

  return m_state;
}

void CFMCTCSSRX::reset()
{
  m_q0 = 0;
  m_q1 = 0;
  m_state = false;
  m_count = 0U;
}

//#endif

