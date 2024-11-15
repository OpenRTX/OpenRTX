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
bool new_history;

history_list_t *history_init()
{
    history_list_t *list = (history_list_t *)malloc(sizeof(history_list_t));
    if(list==NULL) return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    new_history = false;
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
    int i;
    for (i = 0; callsign[i] != ' ' && callsign[i] != '\0' && i<9; i++);
    strncpy(node->callsign, callsign, i);
    // if((module!=NULL) && (module[0]!='\0'))
    //     strncpy(node->module, module, 9);
    node->time = state_time;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void history_prune(history_list_t *list, history_t *this)
{
    history_t *current = this;
    if(current->prev!=NULL)
        current->prev->next =  current->next;
    if (current->next!=NULL)
        current->next->prev = current->prev;
    if (list->tail == current)
        list->tail = current->prev;
    list->length--;
    free(current);
}

void history_push(history_list_t *list, const char* callsign, const char* module, datetime_t state_time)
{
    if(list->length > HISTORY_MAX)
        history_prune(list, list->tail);
    history_t *node = history_create(callsign, module, state_time);
    if(node==NULL)
        return;
    node->next = list->head;
    node->next->prev = node;
    node->prev = NULL;
    if(list->tail == NULL)
        list->tail = node;
    list->head = node;
    list->length++;
    new_history = true;
}

void history_update(history_list_t *list, history_t* node, const char* module, datetime_t state_time)
{
    if(node!=NULL) {
        if((module!=NULL) && (module[0]!='\0'))
            strncpy(node->module, module, 9);
        node->time = state_time;
        node->next->prev = node->prev;
        node->prev->next = node->next;
        if(list->tail == node)
            list->tail = node->next;
        node->next = list->head;
	list->head = node;
        node->prev = NULL;
    }

}

void history_test(history_list_t *list, const char* src, const char* dest, const char* refl, const char *link , datetime_t state_time)
{
    char message[10];
    snprintf(message,9,"%2s%2s%2s%2s",src,dest,refl,link);
    history_push(list, message, NULL, state_time); 
}

void history_add(history_list_t *list, const char* callsign, const char* module, datetime_t state_time)
{
    history_t *node = history_find(list, callsign);
    if(node) 
        history_update(list, node, module, state_time);
    else 
        history_push(list, callsign, module, state_time); 
}

int read_history(history_list_t *list,  history_t *history, uint8_t pos)
{
    history_t *current = list->head;
    uint8_t index = 0;
    if (pos==history_size(list)) return -1;
    while(pos>index)
    {
        current = current->next;
        index++;
    }
    memcpy(history, current, sizeof(history_t));
    return index;
}

uint8_t history_size(history_list_t *list)
{
    return list->length;
}

void format_history_value(char *buf, int max_len, history_t history) {
    sniprintf(buf, max_len, "%10s  %02d:%02d:%02d", history.callsign, history.time.hour, history.time.minute, history.time.second);
}

bool is_new_history() {
    return new_history;
}

void reset_new_history() {
    new_history = false;
}
