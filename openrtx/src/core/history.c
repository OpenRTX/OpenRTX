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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <history.h>

history_list_t *history_list;

history_list_t *history_init()
{
    history_list_t *list = (history_list_t *)malloc(sizeof(history_list_t));
    if(list==NULL) return NULL;
    list->head = NULL;
    list->length = 0;
    return list;
}

history_t *history_find(history_list_t *list, const char* callsign)
{
        history_t *current = list->head;
        while(current)
        {
            if(strncmp(current->callsign, callsign, 9) == 0)
                return current;
            current = current->next;
        }
        return NULL;
}

history_t *history_create(const char* callsign, const char* module, datetime_t state_time)
{
    history_t *node = (history_t *) malloc(sizeof(history_t));
    if(node==NULL)
        return NULL;
    strncpy(node->callsign, callsign, 9);
    if((module!=NULL) && (module!="\0"))
        strncpy(node->module, module, 9);
    node->time = state_time;
    node->next = NULL;
    return node;
}

void history_push(history_list_t *list, const char* callsign, const char* module, datetime_t state_time)
{
    history_t *node = history_create(callsign, module, state_time);
    if(node==NULL)
        return;
    node->next = list->head;
    list->head = node;
    list->length++;
}

void history_update(history_list_t *list, const char* callsign, const char* module, datetime_t state_time)
{
    history_t *node = history_find(list, callsign);
    if(node!=NULL) {
        if((module!=NULL) && (module!="\0"))
            strncpy(node->module, module, 9);
        node->time = state_time;
    }

}

void history_add(history_list_t *list, const char* callsign, const char* module, datetime_t state_time)
{
    if((strncmp(module, "INFO", 4)==0) || (strncmp(module, "ECHO", 4)==0)) return;
    if(history_find(list, callsign)) 
        history_update(list, callsign, module, state_time);
    else 
        history_push(list, callsign, module, state_time); 
}

uint8_t history_size(history_list_t *list)
{
    return list->length;
}