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

#ifndef HISTORY_H
#define HISTORY_H

#include <datatypes.h>
#include <settings.h>
#include <pthread.h>
#include <stdbool.h>
#include <gps.h>

#define HISTORY_MAX 20

/**
 * Data structure for History.
 */
struct history
{
    char callsign[10];
    datetime_t time;
    struct history * next;
    struct history * prev;
};

/**
 * Data structure History list.
 */
struct history_list
{
    struct history *head;
    struct history *tail;
    uint8_t length;
    bool new_history;
    char status;
};

extern struct history_list *history_list;

/**
 * Initialise radio state mutex and radio state variable, reading the
 * informations from device drivers.
 */
void history_init();

/**
 * Add History entry.
 *
 * @param callsign: callsign of user.
 * @param state_time: time when user is heard.
 */
void history_add(const char* callsign, datetime_t state_time);

/**
 * Read History entry.
 *
 * @param history: history entry.
 * @param pos: position of entry.
 */
int history_read(struct history *history, uint8_t pos);

/**
 * Return size of history list.
 *
 * @return the length of history list
 */
uint8_t history_size();

/**
 * Return boolean if history is empty or not.
 *
 * @return boolean if history is empty or not
 */
bool history_is_empty();

/**
 * Return status of history list
 *
 * @return the status of history list
 */
char history_status();

/**
 * Format the history value
 */
void history_format_value(char *buf, int max_len, struct history history);

/**
 * Check if its a new History value
 *
 * @return boolean value if history is new or old
 */
bool history_is_new();

/**
 * Reset new history
 */
void history_new_reset();

#endif /* HISTORY_H */
