/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_PACKET_LIST_H
#define APRS_PACKET_LIST_H

#include "protocols/APRS/packet.h"

#ifdef __cplusplus
extern "C" {
#endif

struct aprsPktList {
    struct aprsPacket *head;
    struct aprsPacket *tail;
    size_t len;
};

/**
 * @brief Inserts an APRS packet at the front of a list
 *
 * APRS packets are stored as a double-linked list. This function adds a
 * single packet to the FRONT of a list.
 *
 * @param pkt A pointer to an aprsPacket to add
 * @param list A struct aprsPktList to be added to
 * @return The updated list
 */
struct aprsPktList aprsPktList_insert(struct aprsPktList list,
                                      struct aprsPacket *pkt);

/**
 * @brief Concatenates two APRS packet lists
 *
 * APRS packets are stored as a double-linked list in RTX and in state. This
 * function adds the second list to the first.
 *
 * @param src A struct aprsPktList of the first list
 * @param dst A struct aprsPktList of the second list
 * @return The resulting list first + second list
 */
struct aprsPktList aprsPktList_concat(const struct aprsPktList list1,
                                      const struct aprsPktList list2);

/**
 * @brief Deletes an APRS packet from a list
 *
 * APRS packets are stored as a double-linked list. This function removes a
 * packet from a list, frees the memory allocated, and updates the list
 * length.
 *
 * @param list: A struct aprsPktList to modify
 * @param pkt: A pointer to a struct aprsPacket to delete
 * @return An updated struct aprsPktList
 */
struct aprsPktList aprsPktList_delete(struct aprsPktList list,
                                      struct aprsPacket *pkt);

/**
 * @brief Returns an empty, initialized APRS packet list
 *
 * @return An empty, initialized struct aprsPktList
 */
void aprsPktList_init(struct aprsPktList *list);

/**
 * @brief Frees the APRS packets in a list
 */
void aprsPktList_release(struct aprsPktList list);

#ifdef __cplusplus
}
#endif

#endif /* APRS_PACKET_LIST_H */
