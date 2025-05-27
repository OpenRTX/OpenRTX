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

// history_list_t *history_init()
void history_init()
{
    history_list = (history_list_t *)malloc(sizeof(history_list_t));
<<<<<<< Updated upstream
    if(history_list==NULL) return NULL;
=======
>>>>>>> Stashed changes
    history_list->head = NULL;
    history_list->tail = NULL;
    history_list->length = 0;
    history_list->new_history = false;
<<<<<<< Updated upstream
    // return list;
=======
    history_list->status = ' ';
>>>>>>> Stashed changes
}

int findCallsignLength(const char* callsign) {
        int i;
        for (i = 0; callsign[i] != ' ' && callsign[i] != '\0' && i<9; i++);
	return i;
}

// history_t *history_find(history_list_t *list, const char* callsign)
history_t *history_find(const char* callsign)
{
<<<<<<< Updated upstream
=======
	history_list->status='L';
>>>>>>> Stashed changes
        if(history_list == NULL) return NULL;

        history_t *current = history_list->head;
        while(current != NULL)
        {
            int i = findCallsignLength(callsign);
<<<<<<< Updated upstream
            if(strncmp(current->callsign, callsign, i) == 0)
                return current;
            current = current->next;
        }
        return NULL;
}

history_t *history_create(const char* callsign, const char* module, datetime_t state_time)
{
=======
            if(strncmp(current->callsign, callsign, i) == 0) {
		history_list->status='F';
                return current;
            }
            current = current->next;
        }
	history_list->status='f';
        return NULL;
}

history_t *history_create(const char* callsign, datetime_t state_time)
{ 
>>>>>>> Stashed changes
    history_t *node = (history_t *) malloc(sizeof(history_t));
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

// void history_prune(history_list_t *list, history_t *this)
void history_prune(history_t *this)
{
    if (history_list==NULL || this==NULL) return;
    history_t *current = this;
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

<<<<<<< Updated upstream
// void history_push(history_list_t *list, const char* callsign, const char* module, datetime_t state_time)
void history_push(const char* callsign, const char* module, datetime_t state_time)
{
    if(history_list->length > HISTORY_MAX)
        history_prune(history_list->tail);
    history_t *node = history_create(callsign, module, state_time);
    if(node==NULL)
        return;
    node->next = history_list->head;
=======
void history_push(const char* callsign, datetime_t state_time)
{
    if(history_list->length > HISTORY_MAX)
        history_prune(history_list->tail);
    history_t *node = history_create(callsign, state_time);
    if(node==NULL)
        return;
    if(history_list->head != NULL)
        node->next = history_list->head;
>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
// void history_update(history_list_t *list, history_t* node, const char* module, datetime_t state_time)
void history_update(history_t* node, const char* module, datetime_t state_time)
{
    if(node!=NULL) {
        // if((module!=NULL) && (module[0]!='\0'))
        //     strncpy(node->module, module, 9);
=======
void history_update(history_t* node, datetime_t state_time)
{
    if(node!=NULL) {
>>>>>>> Stashed changes
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

#ifdef DEBUG
void history_test(history_list_t *list, const char* src, const char* dest, const char* refl, const char *link , datetime_t state_time)
{
    char message[10];
    snprintf(message,9,"%2s%2s%2s%2s",src,dest,refl,link);
    history_push(list, message, NULL, state_time); 
}
#endif

<<<<<<< Updated upstream
// void history_add(history_list_t *list, const char* callsign, const char* module, datetime_t state_time)
void history_add(const char* callsign, const char* module, datetime_t state_time)
{
    history_t *node = history_find(callsign);
    // if(node != NULL) 
    //     history_update(node, module, state_time);
    // else 
        history_push(callsign, module, state_time); 
=======
void history_add(const char* callsign, datetime_t state_time)
{
    if(callsign == NULL) return;
    history_t *node = history_find(callsign);
    if(node != NULL) 
        history_update(node, state_time);
    else 
        history_push(callsign, state_time); 
>>>>>>> Stashed changes
}

// int read_history(history_list_t *list,  history_t *history, uint8_t pos)
int read_history(history_t *history, uint8_t pos)
{
    history_t *current = history_list->head;
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
    memcpy(history, current, sizeof(history_t));
<<<<<<< Updated upstream
    return index;
=======
    return 0;
>>>>>>> Stashed changes
}

// uint8_t history_size(history_list_t *list)
uint8_t history_size()
{
    return history_list->length;
}

<<<<<<< Updated upstream
void format_history_value(char *buf, int max_len, history_t history) {
    int gap = max_len - 10;
    gap -= strlen(history.callsign);
    char spaces[gap + 1]; // Create a buffer for the spaces
    memset(spaces, ' ', gap); // Fill the buffer with spaces
    spaces[gap] = '\0'; // Null-terminate the string
    sniprintf(buf, max_len, "%s%s%02d:%02d:%02d", history.callsign, spaces, history.time.hour, history.time.minute, history.time.second);
=======
char history_status()
{
   return history_list->status;
}

void format_history_value(char *buf, int max_len, history_t history) {
//    int gap = max_len - 10;
//    gap -= strlen(history.callsign);
//    char spaces[gap + 1]; // Create a buffer for the spaces
//    memset(spaces, ' ', gap); // Fill the buffer with spaces
//    spaces[gap] = '\0'; // Null-terminate the string
//    sniprintf(buf, max_len, "%s%s%02d:%02d:%02d", history.callsign, spaces, history.time.hour, history.time.minute, history.time.second);
    sniprintf(buf, max_len, "%s %02d:%02d:%02d", history.callsign, history.time.hour, history.time.minute, history.time.second);
>>>>>>> Stashed changes
}

bool is_new_history() {
    return history_list->new_history;
}

void reset_new_history() {
    history_list->new_history = false;
}
<<<<<<< Updated upstream
=======

>>>>>>> Stashed changes
