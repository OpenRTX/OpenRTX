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
#include <favourites.h>

favourite_list_t *favourite_list;
bool new_favourite;

favourite_list_t *favourite_init()
{
    favourite_list_t *list = (favourite_list_t *)malloc(sizeof(favourite_list_t));
    if(list==NULL) return NULL;
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    return list;
}

favourite_t *favourite_find(favourite_list_t *list, const char* callsign)
{
        favourite_t *current = list->head;
        while(current)
        {
            if(strncmp(current->callsign, callsign, 9) == 0)
                return current;
            current = current->next;
        }
        return NULL;
}

favourite_t *favourite_create(const char* callsign, const char* module, datetime_t state_time)
{
    favourite_t *node = (favourite_t *) malloc(sizeof(favourite_t));
    if(node==NULL)
        return NULL;
    int i;
    for (i = 0; callsign[i] != ' ' && callsign[i] != '\0' && i<9; i++);
    strncpy(node->callsign, callsign, i);
    if((module!=NULL) && (module[0]!='\0'))
        strncpy(node->module, module, 9);
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void favourite_prune(favourite_list_t *list, favourite_t *this)
{
    favourite_t *current = this;
    if(current->prev!=NULL)
        current->prev->next =  current->next;
    if (current->next!=NULL)
        current->next->prev = current->prev;
    if (list->tail == current)
        list->tail = current->prev;
    list->length--;
    free(current);
}

void favourite_push(favourite_list_t *list, const char* callsign, const char* module)
{
    if(list->length > FAVOURITES_MAX)
        history_prune(list, list->tail);
    favourite_t *node = history_create(callsign, module);
    if(node==NULL)
        return;
    node->next = list->head;
    node->next->prev = node;
    node->prev = NULL;
    if(list->tail == NULL)
        list->tail = node;
    list->head = node;
    list->length++;
}

void favourite_update(favourite_list_t *list, favourite_t* node, const char* module)
{
    if(node!=NULL) {
        if((module!=NULL) && (module[0]!='\0'))
            strncpy(node->module, module, 9);
        node->next->prev = node->prev;
        node->prev->next = node->next;
        if(list->tail == node)
            list->tail = node->next;
        node->next = list->head;
	list->head = node;
        node->prev = NULL;
    }

}

void favourite_test(favourite_list_t *list, const char* src, const char* dest, const char* refl, const char *link , datetime_t state_time)
{
    char message[10];
    snprintf(message,9,"%2s%2s%2s%2s",src,dest,refl,link);
    history_push(list, message, NULL); 
}

void favourite_add(favourite_list_t *list, const char* callsign, const char* module)
{
    favourite_t *node = history_find(list, callsign);
    if(node) 
        history_update(list, node, module);
    else 
        history_push(list, callsign, module); 
}

int read_favourite(favourite_list_t *list,  favourite_t *item, uint8_t pos)
{
    favourite_t *current = list->head;
    uint8_t index = 0;
    if (pos==history_size(list)) return -1;
    while(pos>index)
    {
        current = current->next;
        index++;
    }
    memcpy(item, current, sizeof(favourite_t));
    return index;
}

uint8_t favourite_size(favourite_list_t *list)
{
    return list->length;
}

void format_favourite_value(char *buf, int max_len, favourite_t history) {
    sniprintf(buf, max_len, "%10s %10s", history.callsign, history.module);
}
