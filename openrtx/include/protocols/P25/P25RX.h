/*
 *   Copyright (C) 2015,2016,2017,2020 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Bryan Biedenkapp <gatekeep@gmail.com>
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


#if !defined(P25RX_H)
#define  P25RX_H

#define  ARM_MATH_CM4
#include <DSTAR/arm_math.h>
#include <P25/P25Defines.h>

enum P25RX_STATE {
  P25RXS_NONE,
  P25RXS_HDR,
  P25RXS_LDU
};

class CP25RX {
public:
  CP25RX();

  void samples(const q15_t* samples, uint16_t* rssi, uint8_t length);
  void getData(uint8_t *data);
  void reset();

  bool m_synced;

private:
  P25RX_STATE m_state;
  uint32_t    m_bitBuffer[P25_RADIO_SYMBOL_LENGTH];
  q15_t       m_buffer[P25_LDU_FRAME_LENGTH_SAMPLES];
  uint16_t    m_bitPtr;
  uint16_t    m_dataPtr;
  uint16_t    m_hdrStartPtr;
  uint16_t    m_lduStartPtr;
  uint16_t    m_lduEndPtr;
  uint16_t    m_minSyncPtr;
  uint16_t    m_maxSyncPtr;
  uint16_t    m_hdrSyncPtr;
  uint16_t    m_lduSyncPtr;
  q31_t       m_maxCorr;
  uint16_t    m_lostCount;
  uint8_t     m_countdown;
  q15_t       m_centre[16U];
  q15_t       m_centreVal;
  q15_t       m_threshold[16U];
  q15_t       m_thresholdVal;
  uint8_t     m_averagePtr;
  uint32_t    m_rssiAccum;
  uint16_t    m_rssiCount;
  uint8_t     m_duid;

  void processNone(q15_t sample);
  void processHdr(q15_t sample);
  void processLdu(q15_t sample);
  bool correlateSync();
  void calculateLevels(uint16_t start, uint16_t count);
  void samplesToBits(uint16_t start, uint16_t count, uint8_t* buffer, uint16_t offset, q15_t centre, q15_t threshold);
  void writeRSSILdu(uint8_t* ldu);
};

#endif
