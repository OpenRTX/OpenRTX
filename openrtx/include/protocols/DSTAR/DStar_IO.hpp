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

#include "DStarRX.h"
#include "DStarTX.h"

#define  ARM_MATH_CM4
#include <DSTAR/arm_math.h>

static constexpr size_t  RX_SAMPLE_RATE     = 24000;
static constexpr size_t  TX_SAMPLE_RATE     = 24000;
static constexpr size_t  SAMPLE_BUF_SIZE    = 8;

enum MOD17_STATE {
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

class DStar_IO {
public:
  DStar_IO();


  // Hardware specific routines
  void initInt();
  void startInt();
  void terminate();
  void setMode();
  void start();
  void reset();
  void process();
  void setDStarInt(bool on);
  void startBasebandSampling();
  void stopBasebandSampling();
  bool isLocked();
  bool update(const bool invertPhase);
  void update_tx(const bool invertPhase);
  void process_tx();
  void getMyCall(char*);
  void getUrCall(char*);
  void getRpt1Call(char*);
  void getRpt2Call(char*);
  void getSuffix(char*);
  void getText(char*);
  void getData(uint8_t*);
  void setPTTInt(bool ptt);
  void init_tx();
  void terminate_tx();
  bool start_tx();
  void stop_tx();
  void setTxHeader(const uint8_t *header, uint8_t len);
  void setTxData(uint8_t *data, uint8_t len);
  void writeEOT();
  void setTXDelay(uint8_t delay);
  void slowSpeedDataEncode(char *cMessage, unsigned char *ucBytes, unsigned char ucMode);

private:
  std::unique_ptr< int16_t[] >   baseband_rxbuffer; ///< Buffer for baseband audio handling.
  std::unique_ptr< int16_t[] >   baseband_txbuffer; ///< Buffer for baseband audio handling.
  pathId           basebandPath;
  streamId         basebandId;      ///< Id of the baseband input stream.
  bool             locked;
  stream_sample_t *idleBuffer;      ///< Half baseband buffer, free for processing.
  streamId         outStream;       ///< Baseband output stream ID.
  pathId           outPath;         ///< Baseband output path ID.
  bool             txRunning;       ///< Transmission running.
  bool             invPhase;        ///< Invert signal phase

  MOD17_STATE m_modemState;
  bool        m_dstarEnable;
  bool        m_dcd;
  bool        m_pttInvert;
  uint32_t    m_count;
  uint32_t    m_ledCount;
  bool        m_ledValue;
  uint8_t     m_rxLevel;

  volatile uint32_t m_watchdog;

  CDStarRX     dstarRX;
  CDStarTX     dstarTX;
  bool         m_started;

  arm_fir_instance_q15 m_GMSKFilter;
  q15_t                m_GMSKState[40U];     // NoTaps + BlockSize - 1, 12 + 20 - 1 plus some spare
  bool                 m_detect;
  bool                 m_lockout;
  char                 m_myCall[9];
  char                 m_urCall[9];
  char                 m_rpt1Call[9];
  char                 m_rpt2Call[9];
  char                 m_suffix[5];
  char                 m_text[21];
};
#endif
