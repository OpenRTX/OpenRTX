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

#include <FMCTCSSTX.h>

const struct TX_CTCSS_TABLE {
  uint8_t  frequency;
  uint16_t length;
  q31_t    increment;
} TX_CTCSS_TABLE_DATA[] = {
  { 67U, 358U, 5995059},
  { 69U, 346U, 6200860},
  { 71U, 334U, 6433504},
  { 74U, 323U, 6657200},
  { 77U, 312U, 6889844},
  { 79U, 301U, 7131436},
  { 82U, 291U, 7381976},
  { 85U, 281U, 7641463},
  { 88U, 271U, 7918846},
  { 91U, 262U, 8187282},
  { 94U, 253U, 8482561},
  { 97U, 246U, 8715205},
  {100U, 240U, 8947849},
  {103U, 232U, 9261024},
  {107U, 224U, 9592094},
  {110U, 216U, 9923165},
  {114U, 209U, 10272131},
  {118U, 202U, 10630045},
  {123U, 195U, 11005854},
  {127U, 189U, 11390612},
  {131U, 182U, 11793265},
  {136U, 176U, 12213814},
  {141U, 170U, 12643310},
  {146U, 164U, 13081755},
  {151U, 159U, 13547043},
  {156U, 153U, 14021279},
  {159U, 150U, 14298662},
  {162U, 148U, 14513411},
  {165U, 145U, 14808690},
  {167U, 143U, 15023438},
  {171U, 140U, 15327665},
  {173U, 138U, 15551361},
  {177U, 135U, 15864536},
  {179U, 133U, 16097180},
  {183U, 131U, 16419303},
  {186U, 129U, 16660894},
  {189U, 126U, 16991965},
  {192U, 124U, 17251452},
  {196U, 122U, 17591471},
  {199U, 120U, 17850958},
  {203U, 118U, 18208872},
  {206U, 116U, 18477308},
  {210U, 114U, 18853117},
  {218U, 110U, 19515258},
  {225U, 106U, 20195295},
  {229U, 105U, 20499521},
  {233U, 103U, 20902175},
  {241U,  99U, 21635898},
  {250U,  96U, 22396465},
  {254U,  94U, 22736484}};

const uint8_t CTCSS_TABLE_DATA_LEN = 50U;

CFMCTCSSTX::CFMCTCSSTX() :
m_values(NULL),
m_length(0U),
m_n(0U)
{
}

uint8_t CFMCTCSSTX::setParams(uint8_t frequency, uint8_t level)
{
  const TX_CTCSS_TABLE* entry = NULL;

  for (uint8_t i = 0U; i < CTCSS_TABLE_DATA_LEN; i++) {
    if (TX_CTCSS_TABLE_DATA[i].frequency == frequency) {
      entry = TX_CTCSS_TABLE_DATA + i;
      break;
    }
  }

  if (entry == NULL)
    return 4U;

  m_length = entry->length;

  delete[] m_values;
  m_values = new q15_t[m_length];

  q31_t arg = 0;
  for (uint16_t i = 0U; i < m_length; i++) {
    q63_t value = ::arm_sin_q31(arg) * q63_t(level * 13);
    m_values[i] = q15_t(__SSAT((value >> 31), 16));

    arg += entry->increment;
  }

  return 0U;
}

q15_t CFMCTCSSTX::getAudio(bool reverse)
{
  q15_t sample = m_values[m_n++];
  if (m_n >= m_length)
    m_n = 0U;
  
  if (reverse)
    return -sample;
  else
    return sample;
}

//#endif

