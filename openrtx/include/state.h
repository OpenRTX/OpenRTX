/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef STATE_H
#define STATE_H

#include <datatypes.h>
#include <stdbool.h>
#include <rtc.h>

/**
 * Part of this structure has been commented because the corresponding
 * functionality is not yet implemented.
 * Uncomment once the related feature is ready
 */
typedef struct state_t {
    curTime_t time;
    float v_bat;
    //enum ui_screen;
    //enum tuner_mode;
    //enum radio_mode;
    
    //time_t rx_status_tv;
    //bool rx_status;
    
    //time_t tx_status_tv;
    //bool tx_status;
    
    freq_t rx_freq;
    freq_t tx_freq;
    
    //float tx_power;
    
    //uint8_t squelch;
    
    //tone_t rx_tone;
    //tone_t tx_tone;
    
    //ch_t *channel;
    
//#ifdef DMR_ENABLED
    //uint8_t dmr_color;
    //uint8_t dmr_timeslot;
    //dmr_contact_t *dmr_contact;
//#endif
} state_t;

/**
 * This structure is used to mark if the state has been modified
 * and by which thread.
 * The threads that are watching for state updates
 * check the variables of other threads, if they are set,
 * they know that the state have been modified
 */
typedef struct modified_t {
    bool ui_modified;
    bool rtx_modified;
    bool self_modified;
} modified_t;

extern state_t state;
extern modified_t state_flags;


/**
 * This function initializes the Radio state, acquiring the information
 * needed to populate it from device drivers.
 */
void state_init();

#endif /* STATE_H */
