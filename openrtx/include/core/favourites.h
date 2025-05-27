/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef FAVOURITES_H
#define FAVOURITES_H

#include <datatypes.h>
#include <settings.h>
#include <pthread.h>
#include <stdbool.h>
#include <gps.h>

#define FAVOURITES_MAX 20

typedef struct favourite 
{
    char callsign[10];
    char module[10];
    struct favourite * next;
    struct favourite * prev;
}
favourite_t;

typedef struct favourite_list
{
    favourite_t *head;
    favourite_t *tail;
    uint8_t length;
}
favourite_list_t;

// extern pthread_mutex_t history_mutex;
extern favourite_list_t *favourite_list;

/**
 * Initialise radio state mutex and radio state variable, reading the
 * informations from device drivers.
 */
favourite_list_t *favourite_init();
void favourite_add(favourite_list_t *list, const char* callsign, const char* module);
int read_favourite(favourite_list_t *list, favourite_t *item, uint8_t pos);
uint8_t favourite_size(favourite_list_t *list);
void format_favourite_value(char *buf, int max_len, favourite_t item);

#endif /* HISTORY_H */
