/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
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
 *   (2025) Modified by KD0OSS for use in Module17/OpenRTX
 */

#include <cstdint>
#if !defined(DSTARRX_H)
#define  DSTARRX_H

#include "DStarDefines.h"
#include "arm_math.h"

enum DSRX_STATE {
  DSRXS_NONE,
  DSRXS_HEADER,
  DSRXS_DATA
};

class CDStarRX {
public:
  CDStarRX();

  void samples(const q15_t* samples, uint8_t length);
  void getMyCall(char*);
  void getUrCall(char*);
  void getRpt1Call(char*);
  void getRpt2Call(char*);
  void getSuffix(char*);
  void getText(char*);
  void getData(uint8_t*);
  void reset();

  bool m_synced;

private:
  uint32_t     m_pll;
  bool         m_prev;
  DSRX_STATE   m_rxState;
  uint32_t     m_patternBuffer;
  uint8_t      m_rxBuffer[100U];
  unsigned int m_rxBufferBits;
  unsigned int m_dataBits;
  unsigned int m_mar;
  int          m_pathMetric[4U];
  unsigned int m_pathMemory0[42U];
  unsigned int m_pathMemory1[42U];
  unsigned int m_pathMemory2[42U];
  unsigned int m_pathMemory3[42U];
  uint8_t      m_fecOutput[42U];
  q15_t        m_samples[DSTAR_DATA_SYNC_LENGTH_BITS];
  uint8_t      m_samplesPtr;
  char         m_myCall[9];
  char         m_urCall[9];
  char         m_rpt1Call[9];
  char         m_rpt2Call[9];
  uint8_t      m_message[12];
  char         m_suffix[5];
  char         m_text[21];
  bool         m_slowSpeedUpdate;

  void    processNone(bool bit);
  void    processHeader(bool bit);
  void    processData(bool bit);
  bool    rxHeader(uint8_t* in, uint8_t* out);
  void    acs(int* metric);
  void    viterbiDecode(int* data);
  void    traceBack();
  bool    checksum(const uint8_t* header) const;
  int     slowSpeedDataDecode(unsigned char a, unsigned char b, unsigned char c);
};

#endif

