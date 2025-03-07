/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2020 by Geoffrey Merck F4FXL - KC3FRA
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

#include "RingBuffer.h"

template <typename TDATATYPE> CRingBuffer<TDATATYPE>::CRingBuffer(uint16_t length) :
m_length(length),
m_head(0U),
m_tail(0U),
m_full(false),
m_overflow(false)
{
  m_buffer = new TDATATYPE[length];
}

template <typename TDATATYPE> uint16_t CRingBuffer<TDATATYPE>::getSpace() const
{
  uint16_t n = 0U;

  if (m_tail == m_head)
    n = m_full ? 0U : m_length;
  else if (m_tail < m_head)
    n = m_length - m_head + m_tail;
  else
    n = m_tail - m_head;

  if (n > m_length)
    n = 0U;

  return n;
}

template <typename TDATATYPE> uint16_t CRingBuffer<TDATATYPE>::getData() const
{
  if (m_tail == m_head)
    return m_full ? m_length : 0U;
  else if (m_tail < m_head)
    return m_head - m_tail;
  else
    return m_length - m_tail + m_head;
}

template <typename TDATATYPE> bool CRingBuffer<TDATATYPE>::put(TDATATYPE item) volatile
{
  if (m_full) {
    m_overflow = true;
    return false;
  }

  m_buffer[m_head] = item;

  m_head++;
  if (m_head >= m_length)
    m_head = 0U;

  if (m_head == m_tail)
    m_full = true;

  return true;
}

template <typename TDATATYPE> TDATATYPE CRingBuffer<TDATATYPE>::peek() const
{
  return m_buffer[m_tail];
}

template <typename TDATATYPE> bool CRingBuffer<TDATATYPE>::get(TDATATYPE& item) volatile
{
  if (m_head == m_tail && !m_full)
    return false;

  item = m_buffer[m_tail];

  m_full = false;

  m_tail++;
  if (m_tail >= m_length)
    m_tail = 0U;

  return true;
}

template <typename TDATATYPE> bool CRingBuffer<TDATATYPE>::hasOverflowed()
{
  bool overflow = m_overflow;

  m_overflow = false;

  return overflow;
}

template <typename TDATATYPE> void CRingBuffer<TDATATYPE>::reset()
{
  m_head = 0U;
  m_tail = 0U;
  m_full     = false;
  m_overflow = false;
}
