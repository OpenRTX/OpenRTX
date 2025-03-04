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

#if !defined(RINGBUFFER_H)
#define  RINGBUFFER_H
#include "stdint.h"

template <typename TDATATYPE>
class CRingBuffer {
public:
  CRingBuffer(uint16_t length = 370U);
  
  uint16_t getSpace() const;
  
  uint16_t getData() const;

  bool put(TDATATYPE item) volatile;

  bool get(TDATATYPE& item) volatile;

  TDATATYPE peek() const;

  bool hasOverflowed();

  void reset();

private:
  uint16_t              m_length;
  TDATATYPE*            m_buffer;
  volatile uint16_t     m_head;
  volatile uint16_t     m_tail;
  volatile bool         m_full;
  bool                  m_overflow;
};

#include "RingBuffer.impl.h"

#endif
