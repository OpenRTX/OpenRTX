/***************************************************************************
 *   Copyright (C) 2010, 2011 by Terraneo Federico                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#pragma once

namespace miosix {

//
// Parameters for the control based scheduler
//

///Enabe feedforward in the control scheduler. Feedforward means that every
///thread blocking (due to a sleep, wait, or termination) cause the update of
///the set point (alfa). If it is disabled the scheduler reacts to blocking
///using feedback only in the external PI regulator.
#define ENABLE_FEEDFORWARD

///Enable regulator reinitialization. This works in combination with
///ENABLE_FEEDFORWARD to avoid spikes when thread blocking/unblocking occurs.
///Essentially, every time the regulator set point changes, the state of the
///integral regulators is reset to its default value.
#define ENABLE_REGULATOR_REINIT

///Enables support for Real-time priorities in the control scheduler, that
///are otherwise ignored.
//#define SCHED_CONTROL_MULTIBURST

///Run the scheduler using fixed point math only. Faster but less precise.
///Note that the inner integral regulators are always fixed point, this affects
///round partitioning and the external PI regulator.
///Also note this imposes a number of limits:
///- the number of threads has a maximum of 64 (this constrain is enforced in
///  PKaddThread()
///- the max "priority" is limited to 63 (this constraint is enforced by
///  priority valdation, as usual)
///- both krr and zrr must be less than 1.99f (this constraint is not enforced,
///  if a wrong value is set strange things may happen)
///- the maximum average burst must be less than 8192. Individual bursts may
///  exceed this, but the su of all bursts in the Tr variable can't exceed
///  64 (max # threads) * 8191 = ~524287 (this constraint is enforced by
///  clamping Tr to that value)
//#define SCHED_CONTROL_FIXED_POINT

#if defined(ENABLE_REGULATOR_REINIT) && !defined(ENABLE_FEEDFORWARD)
#error "ENABLE_REGULATOR_REINIT requires ENABLE_FEEDFORWARD"
#endif

const float kpi=0.5;
const float krr=0.9;//1.4f;
const float zrr=0.88f;

///Implementation detail resulting from a fixed point implementation of the
///inner integral regulators. Never change this, change kpi instead.
const int multFactor=static_cast<int>(1.0f/kpi);

///Instead of fixing a round time the current policy is to have
///roundTime=bNominal * numThreads, where bNominal is the nominal thread burst
static const int bNominal=static_cast<int>(4000000);// 4ms

///minimum burst time (to avoid inefficiency caused by context switch
///time longer than burst time)
static const int bMin=static_cast<int>(200000);// 200us

///maximum burst time (to avoid losing responsiveness/timer overflow)
static const int bMax=static_cast<int>(20000000);// 20ms

}
