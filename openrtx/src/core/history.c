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

struct history_list *history_list;

// struct history_list *history_init()
void history_init()
{
    history_list = (struct history_list *)malloc(sizeof(history_list));
    
    if(history_list==NULL) return;
    
    history_list->head = NULL;
    history_list->tail = NULL;
    history_list->length = 0;
    history_list->new_history = false;
    history_list->status = ' ';
}

int findCallsignLength(const char* callsign) {
        int i;
        for (i = 0; callsign[i] != ' ' && callsign[i] != '\0' && i<9; i++);
	return i;
}

// struct history *history_find(struct history_list *list, const char* callsign)
struct history *history_find(const char* callsign)
{
	history_list->status='L';
        if(history_list == NULL) return NULL;

        struct history *current = history_list->head;
        while(current != NULL)
        {
            int i = findCallsignLength(callsign);
            if(strncmp(current->callsign, callsign, i) == 0) {
		history_list->status='F';
                return current;
            }
            current = current->next;
        }
	history_list->status='f';
        return NULL;
}

struct history *history_create(const char* callsign, datetime_t state_time)
{ 
    struct history *node = (struct history *) malloc(sizeof(struct history));
    if(node==NULL)
        return NULL;
    int i = findCallsignLength(callsign);
    strncpy(node->callsign, callsign, i);
    node->callsign[i]='\0';
    node->time = state_time;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

// void history_prune(struct history_list *list, struct history *this)
void history_prune(struct history *this)
{
    if (history_list==NULL || this==NULL) return;
    struct history *current = this;
    if(current->prev!=NULL) {
        current->prev->next =  current->next;
    }
    if (current->next!=NULL) {
        current->next->prev = current->prev;
    }
    if (history_list->tail == current) {
        history_list->tail = current->prev;
    }
    history_list->length--;
    free(current);
}

void history_push(const char* callsign, datetime_t state_time)
{
    if(history_list->length > HISTORY_MAX)
        history_prune(history_list->tail);
    struct history *node = history_create(callsign, state_time);
    if(node==NULL)
        return;
    if(history_list->head != NULL)
        node->next = history_list->head;
    if(node->next != NULL) {
        node->next->prev = node;
    }
    if(history_list->tail == NULL) {
        history_list->tail = node;
    }
    history_list->head = node;
    history_list->length++;
    history_list->new_history = true;
}

void history_update(struct history* node, datetime_t state_time)
{
    if(node!=NULL) {
        node->time = state_time;

        if (history_list->head == node) {
            return;
        }
        if(node->next != NULL) {
            node->next->prev = node->prev;
        }
        if(node->prev != NULL) {
            node->prev->next = node->next;
        }
        if(history_list->tail == node) {
            history_list->tail = node->prev;
        }
        node->next = history_list->head;
        if(history_list->head != NULL) {
            history_list->head->prev = node;
        }
        history_list->head = node;
        node->prev = NULL;
    }

}

void history_add(const char* callsign, datetime_t state_time)
{
    if(callsign == NULL) return;
    struct history *node = history_find(callsign);
    if(node != NULL) 
        history_update(node, state_time);
    else 
        history_push(callsign, state_time); 
}

// int read_history(struct history_list *list,  struct history *history, uint8_t pos)
int read_history(struct history *history, uint8_t pos)
{
    struct history *current = history_list->head;
    uint8_t index = 0;
    if (pos==history_size(history_list)) return -1;
    while(pos>index)
    {
	if(current == NULL) {
		return -1;
        }
        current = current->next;
        index++;
    }
    memcpy(history, current, sizeof(struct history));
    return 0;
}

// uint8_t history_size(struct history_list *list)
uint8_t history_size()
{
    return history_list->length;
}

char history_status()
{
   return history_list->status;
}

void format_history_value(char *buf, int max_len, struct history history) {
//    int gap = max_len - 10;
//    gap -= strlen(history.callsign);
//    char spaces[gap + 1]; // Create a buffer for the spaces
//    memset(spaces, ' ', gap); // Fill the buffer with spaces
//    spaces[gap] = '\0'; // Null-terminate the string
//    sniprintf(buf, max_len, "%s%s%02d:%02d:%02d", history.callsign, spaces, history.time.hour, history.time.minute, history.time.second);
    sniprintf(buf, max_len, "%s %02d:%02d:%02d", history.callsign, history.time.hour, history.time.minute, history.time.second);
}

bool is_new_history() {
    return history_list->new_history;
}

void reset_new_history() {
    history_list->new_history = false;
}
