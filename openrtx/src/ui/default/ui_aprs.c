/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui/ui_default.h"
#include "core/input.h"
#include "hwconfig.h"
#include "protocols/APRS/packet.h"

#ifdef CONFIG_APRS
/**
 * @brief Creates a text callsign with ssid from an APRS address
 *
 * @param outStr A buffer of at least 10 chars to hold the output
 * @param address The aprsAddress_t to be converted to text
 */
void aprsAddrStr(char *outStr, aprsAddress_t *address)
{
    if (!address || !outStr)
        return;

    if (address->ssid)
        sniprintf(outStr, 10, "%s-%d", address->addr, address->ssid);
    else
        sniprintf(outStr, 10, "%s", address->addr);
    return;
}

/**
 * @brief Handles input while in the APRS_PKT menu
 *
 * KEY_UP or KNOB_LEFT goes to the previous packet.
 * KEY_DOWN or KNOB_RIGHT goes to the next packet.
 * KEY_HASH deletes the packet being viewed.
 * KEY_ESC returns to the last main state.
 * Packets are shown in reverse chronological order and KEY_DOWN and
 * KEY_UP do not wrap
 *
 * @param ui_state The current state of the user interface
 * @param state The current state of the overall system
 * @param msg The keys that were pressed
 * @param sync_rtx Whether or not to resync with RTX after handling input 
 */
void ui_aprsPktInput(ui_state_t *ui_state, state_t *state, kbd_msg_t msg)
{
    aprsPacket_t *pkt = ui_state->pkt;

    if (msg.keys & KEY_ESC) {
        state->ui_screen = ui_state->last_main_state;
        return;
    }

    // all other commands require a valid packet
    if (!pkt)
        return;

    if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT) {
        if (pkt->prev)
            ui_state->pkt = pkt->prev;
    } else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT) {
        if (pkt->next)
            ui_state->pkt = pkt->next;
    } else if (msg.keys & KEY_HASH) {
        // switch display (ui_state) to a different packet
        if (pkt->next)
            // if there is a next packet show that
            ui_state->pkt = pkt->next;
        else if (pkt->prev)
            // if not, show the previous packet
            ui_state->pkt = pkt->prev;
        else
            // otherwise we're out of packets to show
            ui_state->pkt = NULL;

        // remove the packet from radio state (state) and free the memory
        state->aprsStoredPkts = aprsPktDelete(state->aprsStoredPkts, pkt);

        // update the aprsSaved counter and set flag to sync RTX
        state->aprsStoredPktsSize--;
    }
}

/**
 * @brief Draws the screen to view an APRS packet
 *
 * @param ui_state The current state of the user interface
 */
void ui_aprsPktDraw(ui_state_t *ui_state)
{
    char addressStr[10] = "";
    aprsPacket_t *pkt = ui_state->pkt;

    gfx_clearScreen();

    if (!pkt) {
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "No Packets");
        return;
    }

    // print the source and destination in TNC2 format
    // the first address is the dst, pull out the device ID
    char *devId = pkt->addresses->addr;
    // the second address is the src, make a address string from it
    aprsAddrStr(addressStr, pkt->addresses->next);
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              "%s>%s", addressStr, devId);

    // print the date and time
    gfx_print(layout.line1_pos, layout.line1_font, TEXT_ALIGN_CENTER,
              color_white, "%d/%d/%02d %d:%02d:%02d", pkt->ts.day,
              pkt->ts.month, pkt->ts.year, pkt->ts.hour, pkt->ts.minute,
              pkt->ts.second);

    // print digipeater addresses
    char digipeaters[32] = "";
    // first address is dst, second is src, third is the first digipeater
    for (aprsAddress_t *address = pkt->addresses->next->next; address;
         address = address->next) {
        aprsAddrStr(addressStr, address);
        // make sure we can fit with a '\0' and possibly a ',' and '*'
        if ((strlen(digipeaters) + strlen(addressStr))
            >= (sizeof(digipeaters) - 3))
            break;
        strcat(digipeaters, addressStr);
        if (address->commandHeard)
            strcat(digipeaters, "*");
        if (address->next)
            strcat(digipeaters, ",");
    }
    gfx_printBuffer(layout.line2_pos, FONT_SIZE_6PT, TEXT_ALIGN_LEFT,
                    color_white, digipeaters);

    // print packet info
    gfx_printBuffer(layout.line3_pos, FONT_SIZE_6PT, TEXT_ALIGN_CENTER,
                    color_white, pkt->info);
}
#endif
