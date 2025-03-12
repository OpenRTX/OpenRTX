/*
 *   Copyright (C) 2009-2016 by Jonathan Naylor G4KLX
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

#if !defined(DSTAR_H)
#define  DSTAR_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <audio_path.h>
#include <audio_stream.h>
#include <cstddef>
#include <memory>
#include <array>

#include <DSTAR/DStarRX.h>
#include <DSTAR/DStarTX.h>

#define  ARM_MATH_CM4
#include <DSTAR/arm_math.h>

static constexpr size_t  RX_SAMPLE_RATE     = 24000;
static constexpr size_t  SAMPLE_BUF_SIZE    =  8;
extern std::unique_ptr< int16_t[] >   baseband_buffer; ///< Buffer for baseband audio handling.

enum MMDVM_STATE {
  STATE_IDLE      = 0,
  STATE_DSTAR     = 1,
  STATE_DMR       = 2,
  STATE_YSF       = 3,
  STATE_P25       = 4,

  // Dummy states start at 90
  STATE_CWID      = 97,
  STATE_DMRCAL    = 98,
  STATE_DSTARCAL  = 99
};

extern const bool  valid_frame;
extern pathId      basebandPath;
extern streamId    basebandId;      ///< Id of the baseband input stream.
extern bool        isLocked;

extern MMDVM_STATE m_modemState;
extern bool        m_dstarEnable;
extern bool        m_tx;
extern bool        m_dcd;
extern uint32_t    m_count;
extern uint32_t    m_ledCount;
extern bool        m_ledValue;
extern q15_t       m_rxLevel;
extern bool        pttInvert;

extern volatile uint32_t m_watchdog;

extern uint32_t          m_sampleCount;
extern bool              m_sampleInsert;

extern CDStarRX dstarRX;
extern CDStarTX dstarTX;

extern bool         m_started;

extern arm_fir_instance_q15 m_GMSKFilter;
extern q15_t                m_GMSKState[40U];     // NoTaps + BlockSize - 1, 12 + 20 - 1 plus some spare
extern bool                 m_detect;
extern bool                 m_lockout;

  // Hardware specific routines
void start_io();
void initInt();
void startInt();
void setDStarInt(bool on);
void startBasebandSampling();
void stopBasebandSampling();
bool update(const bool invertPhase);

#endif
