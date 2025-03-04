/*
 *   Copyright (C) 2016,2017,2020 by Jonathan Naylor G4KLX
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
 *   (2025) Modified by KD0OSS for P25 on Module17/OpenRTX
 */

#if !defined(P25TX_H)
#define  P25TX_H

#define  ARM_MATH_CM4
#include <DSTAR/arm_math.h>
#include <DSTAR/RingBuffer.h>

class CP25TX {
public:
  CP25TX();

  uint8_t writeData(const uint8_t* data, uint16_t length);

  void setTxLevel(uint8_t level);

  void process();

  void writeSyncFrames();

  void setTXDelay(uint8_t delay);

  uint8_t getSpace() const;

  void setParams(uint8_t txHang);

private:
  CRingBuffer<uint8_t>             m_buffer;
  arm_fir_interpolate_instance_q15 m_modFilter;
  arm_fir_instance_q15             m_lpFilter;
  q15_t                            m_modState[16U];    // blockSize + phaseLength - 1, 4 + 9 - 1 plus some spare
  q15_t                            m_lpState[60U];     // NoTaps + BlockSize - 1, 32 + 20 - 1 plus some spare
  uint8_t                          m_poBuffer[1200U];
  uint8_t                          m_rxLevel;
  uint8_t                          m_txLevel;
  uint16_t                         m_poLen;
  uint16_t                         m_poPtr;
  uint16_t                         m_txDelay;
  uint32_t                         m_txHang;
  uint32_t                         m_txCount;

  void writeByte(uint8_t c);
  void writeSilence();
};

#endif

