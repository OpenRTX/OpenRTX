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

#include <stdio.h>
#include <state.h>

const curTime_t epoch = {0, 0, 0, 1, 1, 1970};

state_t state = {
    epoch,  //time
    0.0,    //v_bat
    0.0,    //rx_freq
    0.0     //tx_freq
};

modified_t state_flags = {
    false,  //ui_modified
    false,  //rtx_modified
    false   //self_modified
};

void state_init()
{
    /*TODO: Read current state parameters from hardware, 
     * or initialize them to sane defaults */
    state.time = rtc_getTime();
    state.v_bat = platform_getVbat();
    state.rx_freq = 0.0;
    state.tx_freq = 0.0;
}
