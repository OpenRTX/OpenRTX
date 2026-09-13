/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"

#ifdef CONFIG_MESSAGES

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "ui/ui_default.h"
#include "ui/ui_strings.h"
#include "core/messages.h"

/* Generic menu list renderer, implementation is in "ui_menu.c". Reused here
 * so the message list gets the same scrolling/selection/voice-announcement
 * behaviour as every other list screen (banks, channels, contacts, ...). */
extern void _ui_drawMenuList(uint8_t selected,
                             int (*getCurrentEntry)(char *buf, uint8_t max_len,
                                                    uint8_t index));

/**
 * \internal Callback for _ui_drawMenuList(): formats snapshot entry @p index
 * as "<marker><sender> <body preview>", truncated to fit MAX_ENTRY_LEN. The
 * marker is '!' for a message that failed to send and '*' for an unread one.
 * When compose is available, a virtual "New Message" entry is appended after
 * the last real message (at index == messages_count()).
 *
 * @return 0 on success, -1 if index is out of range (stops the list).
 */
static int _ui_getMessagesEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    size_t count = messages_count();

    if (index < count) {
        const struct message *msg = messages_get(index);
        if (msg == NULL)
            return -1;

        const char *other = (msg->direction == MSG_DIR_TX) ? msg->recipient :
                                                             msg->sender;
        const char *marker = " ";
        if (msg->status == MSG_STATUS_FAILED)
            marker = "!";
        else if (msg->unread)
            marker = "*";

        sniprintf(buf, max_len, "%s%-6.6s %.8s", marker, other, msg->body);
        return 0;
    }

    /* Virtual "New Message" item at the end of the list */
    if (index == count && messages_canCompose(last_state.channel.mode)) {
        sniprintf(buf, max_len, "  %s", currentLanguage->newMessage);
        return 0;
    }

    return -1;
}

size_t _ui_messagesListSelection(const ui_state_t *ui_state)
{
    uint32_t pinned = ui_state->messages_list_seq;

    if (pinned == UINT32_MAX)
        return messages_count(); /* the virtual "New Message" row */

    if (pinned != 0) {
        size_t idx = messages_findBySequence(pinned);
        if (idx != SIZE_MAX)
            return idx;
    }

    /* Not pinned yet (screen just entered) or the pinned message is gone:
     * land on the newest entry. */
    return 0;
}

void _ui_drawMessagesList(ui_state_t *ui_state)
{
    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->messages);

    size_t count = messages_count();
    bool can_compose = messages_canCompose(last_state.channel.mode);

    if (count == 0 && !can_compose) {
        gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noMessages);
        return;
    }

    /* Re-derive the highlighted row on every draw, the same way the FSM does,
     * so a message arriving or being evicted in the background cannot leave
     * the highlight and the next key press on different rows. */
    size_t selected = _ui_messagesListSelection(ui_state);
    _ui_drawMenuList((uint8_t)selected, _ui_getMessagesEntryName);
}

void _ui_drawMessagesDetail(ui_state_t *ui_state)
{
    size_t idx = messages_findBySequence(ui_state->messages_detail_seq);
    const struct message *msg = messages_get(idx);

    gfx_clearScreen();

    if (msg == NULL) {
        /* The pinned message vanished (evicted) between the last FSM check
         * and this draw call; the FSM will bounce back to the list on the
         * next key event. Render a harmless placeholder meanwhile. */
        gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noMessages);
        return;
    }

    /* ---- Inverted header bar: "SENDER > RECIPIENT" ---- */
    point_t hdr_rect = { 0, 0 };
    gfx_drawRect(hdr_rect, CONFIG_SCREEN_WIDTH, layout.top_h, color_white,
                 true);

    const char *sender = msg->sender;
    const char *recipient = (msg->recipient[0] != '\0') ?
                                msg->recipient :
                                currentLanguage->broadcast;
    char title[32] = { 0 };
    sniprintf(title, sizeof(title), "%.9s > %.9s", sender, recipient);
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_black,
              "%s", title);

    /* ---- Scrollable body ---- */
    int16_t clip_top = (int16_t)layout.top_h;
    int16_t clip_bot = (int16_t)(CONFIG_SCREEN_HEIGHT - layout.bottom_h - 1);
    uint8_t font_h = gfx_getFontHeight(layout.message_font);

    uint16_t text_h =
        gfx_measureText(layout.message_font, msg->body, layout.horizontal_pad,
                        CONFIG_SCREEN_WIDTH - layout.horizontal_pad, SIZE_MAX);
    int16_t visible_h = clip_bot - clip_top;
    ui_state->detail_scroll_max = (text_h > (uint16_t)visible_h) ?
                                      (int16_t)(text_h - visible_h) :
                                      0;

    /* Clamp against the max just computed for this message (not a previous
     * one), so a jump-to-tail (detail_scroll = INT16_MAX) lands correctly. */
    if (ui_state->detail_scroll > ui_state->detail_scroll_max)
        ui_state->detail_scroll = ui_state->detail_scroll_max;
    if (ui_state->detail_scroll < 0)
        ui_state->detail_scroll = 0;

    point_t text_start = { (int16_t)layout.horizontal_pad,
                           (int16_t)(clip_top + font_h
                                     - ui_state->detail_scroll) };
    gfx_printBufferClipped(text_start, layout.message_font, TEXT_ALIGN_LEFT,
                           color_white, msg->body,
                           CONFIG_SCREEN_WIDTH - layout.horizontal_pad,
                           clip_top, clip_bot);

    /* ---- Inverted bottom bar: "Reply" left, "N/M" centre ---- */
    point_t bot_rect = { 0, (int16_t)(CONFIG_SCREEN_HEIGHT - layout.bottom_h) };
    gfx_drawRect(bot_rect, CONFIG_SCREEN_WIDTH, layout.bottom_h, color_white,
                 true);

    point_t bot_pos = { layout.bottom_pos.x,
                        CONFIG_SCREEN_HEIGHT - layout.bottom_h / 2 };
    if ((msg->mode == last_state.channel.mode)
        && (msg->direction == MSG_DIR_RX))
        gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_LEFT, color_black,
                  currentLanguage->reply);

    char counter[12] = { 0 };
    sniprintf(counter, sizeof(counter), "%u/%u", (unsigned)(idx + 1),
              (unsigned)messages_count());
    gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_CENTER, color_black,
              counter);
}

#ifdef CONFIG_M17_SMS
/**
 * Resolve the effective compose recipient: an explicitly-typed recipient
 * overrides the channel's configured m17_dest, which in turn overrides
 * @p fallback. Shared by the compose draw code (which wants a localized
 * placeholder string) and the send code (which needs the on-air broadcast
 * literal, not a translated one) so the resolution order only has to be
 * maintained in one place.
 */
const char *_ui_resolveComposeRecipient(const char *typed, const char *m17_dest,
                                        const char *fallback)
{
    if (typed[0] != '\0')
        return typed;
    if (m17_dest[0] != '\0')
        return m17_dest;
    return fallback;
}

void _ui_drawMessagesCompose(ui_state_t *ui_state)
{
    gfx_clearScreen();

    /* Title bar */
    const char *title = ui_state->compose_is_reply ?
                            currentLanguage->reply :
                            currentLanguage->newMessage;
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              title);

    /* ---- To row (row 0) ---- */
    point_t to_rpos = layout.line1_pos;

    if (!ui_state->compose_editing && ui_state->compose_focus == 0) {
        point_t rect_pos = { 0, (int16_t)(to_rpos.y - layout.menu_h + 3) };
        gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white,
                     true);
    }

    color_t to_col = (ui_state->compose_focus == 0
                      && !ui_state->compose_editing) ?
                         color_black :
                         color_white;
    {
        const char *rcpt = _ui_resolveComposeRecipient(
            ui_state->compose_recipient, last_state.settings.m17_dest,
            currentLanguage->broadcast);
        char val[11] = { 0 };
        sniprintf(val, sizeof(val), "%.10s", rcpt);
        gfx_print(to_rpos, layout.menu_font, TEXT_ALIGN_LEFT, to_col, "To");
        gfx_print(to_rpos, layout.menu_font, TEXT_ALIGN_RIGHT, to_col, "%s",
                  val);
    }

    /* ---- Body area (row 1) ---- */
    static const uint8_t BOX_PAD = 2;
    int16_t to_row_top = (int16_t)(to_rpos.y - layout.menu_h + 3);
    int16_t body_top = (int16_t)(to_row_top + layout.menu_h);
    int16_t send_h_top = (int16_t)(layout.bottom_pos.y - layout.menu_h + 3);
    int16_t body_bot = (int16_t)(send_h_top - 1);
    uint16_t body_w =
        (uint16_t)(CONFIG_SCREEN_WIDTH - 2u * layout.horizontal_pad);
    uint16_t body_h = (uint16_t)(body_bot - body_top + 1);
    point_t body_orig = { (int16_t)layout.horizontal_pad, body_top };

    if (ui_state->compose_focus == 1 && ui_state->compose_body_editing)
        gfx_drawRect(body_orig, body_w, body_h, color_white, true);
    gfx_drawRect(body_orig, body_w, body_h, color_white, false);

    color_t body_col = (ui_state->compose_focus == 1
                        && ui_state->compose_body_editing) ?
                           color_black :
                           color_white;
    uint8_t font_h = gfx_getFontHeight(layout.message_font);
    uint16_t max_x = (uint16_t)(layout.horizontal_pad + body_w - BOX_PAD);
    int16_t clip_top = (int16_t)(body_top + 1);
    int16_t clip_bot = (int16_t)(body_bot - 1);

    int16_t scroll_offset = gfx_scrollOffsetForCursor(
        layout.message_font, ui_state->compose_body, ui_state->input_position,
        (uint16_t)(layout.horizontal_pad + BOX_PAD), max_x,
        clip_bot - clip_top);

    point_t text_start = { (int16_t)(layout.horizontal_pad + BOX_PAD),
                           (int16_t)(clip_top + font_h - scroll_offset) };
    gfx_printBufferClipped(text_start, layout.message_font, TEXT_ALIGN_LEFT,
                           body_col, ui_state->compose_body, max_x, clip_top,
                           clip_bot);

    /* ---- Send row (row 2) ---- */
    point_t send_rpos = { (int16_t)layout.horizontal_pad, layout.bottom_pos.y };
    if (ui_state->compose_focus == 2) {
        point_t rect_pos = { 0, (int16_t)(send_rpos.y - layout.menu_h + 3) };
        gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white,
                     true);
    }
    color_t send_col = (ui_state->compose_focus == 2) ? color_black :
                                                        color_white;
    gfx_print(send_rpos, layout.menu_font, TEXT_ALIGN_CENTER, send_col,
              currentLanguage->send);

    /* ---- To-field editing overlay ---- */
    if (ui_state->compose_editing) {
        uint16_t rect_width = CONFIG_SCREEN_WIDTH - (layout.horizontal_pad * 2);
        uint16_t rect_height =
            (CONFIG_SCREEN_HEIGHT - (layout.top_h + layout.bottom_h)) / 2;
        point_t rect_origin = {
            (int16_t)((CONFIG_SCREEN_WIDTH - rect_width) / 2),
            (int16_t)((CONFIG_SCREEN_HEIGHT - rect_height) / 2)
        };
        gfx_drawRect(rect_origin, rect_width, rect_height, color_black, true);
        gfx_drawRect(rect_origin, rect_width, rect_height, color_white, false);
        gfx_printLine(1, 1, layout.top_h,
                      CONFIG_SCREEN_HEIGHT - layout.bottom_h,
                      layout.horizontal_pad, layout.input_font,
                      TEXT_ALIGN_CENTER, color_white, ui_state->new_callsign);
    }
}
#endif /* CONFIG_M17_SMS */

#endif /* CONFIG_MESSAGES */
