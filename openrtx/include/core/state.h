/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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
#include <settings.h>
#include <pthread.h>
#include <stdbool.h>
#include <cps.h>
#include <gps.h>

/**
 * Part of this structure has been commented because the corresponding
 * functionality is not yet implemented.
 * Uncomment once the related feature is ready
 */
typedef struct
{
    uint8_t    devStatus ;
    datetime_t time ;
    uint16_t   v_bat ;
    uint8_t    charge ;
    float      rssi ;

    uint8_t    ui_prevScreen ;
    uint8_t    ui_screen ;
    uint8_t    tuner_mode ;

    uint16_t   channel_index ;
    channel_t  channel ;
    channel_t  vfo_channel ;
    bool       bank_enabled ;
    uint16_t   bank ;
    uint8_t    rtxStatus ;
    bool       tone_enabled ;

    uint16_t   contact_index ;
    contact_t  contact ;

    bool       emergency ;
    settings_t settings ;
    gps_t      gps_data ;
    bool       gps_set_time ;
    bool       gpsDetected ;
    bool       backup_eflash ;
    bool       restore_eflash ;
    bool       txDisable ;
    uint8_t    step_index ;
}State_st;

extern uint32_t freq_steps[];
extern size_t n_freq_steps;

typedef enum
{
    VFO = 0,
    CH,
    SCAN,
    CHSCAN
}TunerMode_en;

typedef enum
{
    RTX_OFF = 0,
    RTX_RX,
    RTX_TX
}RTxStatus_en;

typedef enum
{
    PWROFF = 0,
    STARTUP,
    RUNNING,
    DATATRANSFER,
    SHUTDOWN
}DeviceStatus_en;

extern State_st state;
extern pthread_mutex_t state_mutex;

/**
 * Initialise radio state mutex and radio state variable, reading the
 * informations from device drivers.
 */
void state_init( void );

/**
 * Terminate the radio state saving persistent settings to flash and destroy
 * the state mutex.
 */
void state_terminate( void );

/**
 * Update radio state fetching data from device drivers.
 */
void state_task( void );

/**
 * Reset the fields of radio state containing user settings and VFO channel.
 */
void state_resetSettingsAndVfo( void );

#endif /* STATE_H */
