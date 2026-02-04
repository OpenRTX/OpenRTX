/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/packet_list.h"

struct aprsPktList aprsPktList_insert(struct aprsPktList list,
                                      struct aprsPacket *pkt)
{
    if (!list.head) {
        list.head = pkt;
        list.tail = pkt;
    } else {
        pkt->next = list.head;
        list.head = pkt;
        pkt->next->prev = pkt;
    }
    list.len++;

    return list;
}

struct aprsPktList aprsPktList_concat(const struct aprsPktList list1,
                                      const struct aprsPktList list2)
{
    struct aprsPktList result;

    list1.tail->next = list2.head;
    list2.head->prev = list1.tail;

    result.head = list1.head;
    result.tail = list2.tail;
    result.len = list1.len + list2.len;

    return result;
}

struct aprsPktList aprsPktList_delete(struct aprsPktList list,
                                      struct aprsPacket *pkt)
{
    if (list.head == pkt)
        list.head = pkt->next;
    if (list.tail == pkt)
        list.tail = pkt->prev;
    if (pkt->prev)
        pkt->prev->next = pkt->next;
    if (pkt->next)
        pkt->next->prev = pkt->prev;
    list.len--;
    free(pkt);

    return list;
}

void aprsPktList_init(struct aprsPktList *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->len = 0;

    return;
}

void aprsPktList_release(struct aprsPktList list)
{
    struct aprsPacket *pkt = list.head;
    while (pkt) {
        struct aprsPacket *next = pkt->next;
        free(pkt);
        pkt = next;
    }
}
