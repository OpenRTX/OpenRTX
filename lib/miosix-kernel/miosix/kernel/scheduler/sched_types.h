/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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

#include "config/miosix_settings.h"
#include "kernel/scheduler/priority/priority_scheduler_types.h"
#include "kernel/scheduler/control/control_scheduler_types.h"
#include "kernel/scheduler/edf/edf_scheduler_types.h"

namespace miosix {

#ifdef SCHED_TYPE_PRIORITY
typedef PrioritySchedulerPriority Priority;
typedef PrioritySchedulerData SchedulerData;
#elif defined(SCHED_TYPE_CONTROL_BASED)
typedef ControlSchedulerPriority Priority;
typedef ControlSchedulerData SchedulerData;
#elif defined(SCHED_TYPE_EDF)
typedef EDFSchedulerPriority Priority;
typedef EDFSchedulerData SchedulerData;
#else
#error No scheduler selected in config/miosix_settings.h
#endif

} //namespace miosix
