/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"

#ifdef CONFIG_MESSAGES

#include <stdio.h>
#include <stddef.h>
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
 * as "<unread-marker><sender> <body preview>", truncated to fit MAX_ENTRY_LEN.
 *
 * @return 0 on success, -1 if index is out of range (stops the list).
 */
static int _ui_getMessagesEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    size_t count = messages_count();

    if (index < count) {
        struct message_header *hdr = messages_get(index);
        if (hdr == NULL)
            return -1;

        const char *body = (hdr->body != NULL) ? (const char *)hdr->body : "";
        const char *other = (hdr->direction == MSG_DIR_TX) ? hdr->recipient :
                                                             hdr->sender;
        sniprintf(buf, max_len, "%s%-6.6s %.8s", hdr->unread ? "*" : " ", other,
                  body);
        return 0;
    }

    /* Virtual "New Message" item at the end of the list */
    if (index == count && messages_can_compose(last_state.channel.mode)) {
        sniprintf(buf, max_len, "  %s", currentLanguage->newMessage);
        return 0;
    }

    return -1;
}

void _ui_drawMessagesList(ui_state_t *ui_state)
{
    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->messages);

    if (messages_count() == 0) {
        gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noMessages);
        return;
    }

    _ui_drawMenuList(ui_state->menu_selected, _ui_getMessagesEntryName);
}

void _ui_drawMessagesDetail(ui_state_t *ui_state)
{
    size_t idx = messages_find_by_sequence(ui_state->messages_detail_seq);
    struct message_header *hdr = (idx != SIZE_MAX) ? messages_get(idx) : NULL;

    gfx_clearScreen();

    if (hdr == NULL) {
        /* The pinned message vanished (evicted) between the last FSM check
         * and this draw call; the FSM will bounce back to the list on the
         * next key event. Render a harmless placeholder meanwhile. */
        gfx_print(layout.line2_pos, layout.line2_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->noMessages);
        return;
    }

    /* Clamp scroll each frame before drawing. */
    if (ui_state->detail_scroll > ui_state->detail_scroll_max)
        ui_state->detail_scroll = ui_state->detail_scroll_max;
    if (ui_state->detail_scroll < 0)
        ui_state->detail_scroll = 0;

    /* ---- Inverted header bar: "SENDER > RECIPIENT" ---- */
    point_t hdr_rect = { 0, 0 };
    gfx_drawRect(hdr_rect, CONFIG_SCREEN_WIDTH, layout.top_h, color_white,
                 true);

    const char *sender = hdr->sender;
    const char *recipient = (hdr->recipient[0] != '\0') ? hdr->recipient :
                                                          "ALL";
    char title[32] = { 0 };
    sniprintf(title, sizeof(title), "%.9s > %.9s", sender, recipient);
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_black,
              title);

    /* ---- Scrollable body ---- */
    int16_t clip_top = (int16_t)layout.top_h;
    int16_t clip_bot = (int16_t)(CONFIG_SCREEN_HEIGHT - layout.bottom_h - 1);
    uint8_t font_h = gfx_getFontHeight(layout.menu_font);

    const char *body_text = (hdr->body != NULL) ? (const char *)hdr->body : "";

    uint16_t text_h =
        gfx_measureText(layout.menu_font, body_text, layout.horizontal_pad,
                        CONFIG_SCREEN_WIDTH - layout.horizontal_pad, SIZE_MAX);
    int16_t visible_h = clip_bot - clip_top;
    ui_state->detail_scroll_max = (text_h > (uint16_t)visible_h) ?
                                      (int16_t)(text_h - visible_h) :
                                      0;

    if (ui_state->detail_scroll > ui_state->detail_scroll_max)
        ui_state->detail_scroll = ui_state->detail_scroll_max;

    point_t text_start = { (int16_t)layout.horizontal_pad,
                           (int16_t)(clip_top + font_h
                                     - ui_state->detail_scroll) };
    gfx_printBufferClipped(text_start, layout.menu_font, TEXT_ALIGN_LEFT,
                           color_white, body_text,
                           CONFIG_SCREEN_WIDTH - layout.horizontal_pad,
                           clip_top, clip_bot);

    /* ---- Inverted bottom bar: "Reply" left, "N/M" centre ---- */
    point_t bot_rect = { 0, (int16_t)(CONFIG_SCREEN_HEIGHT - layout.bottom_h) };
    gfx_drawRect(bot_rect, CONFIG_SCREEN_WIDTH, layout.bottom_h, color_white,
                 true);

    point_t bot_pos = { layout.bottom_pos.x,
                        CONFIG_SCREEN_HEIGHT - layout.bottom_h / 2 };
    if ((messages_source_mode(idx) == last_state.channel.mode)
        && (messages_supported_actions(idx) & MSG_ACTION_REPLY))
        gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_LEFT, color_black,
                  currentLanguage->reply);

    char counter[12] = { 0 };
    sniprintf(counter, sizeof(counter), "%u/%u", (unsigned)(idx + 1),
              (unsigned)messages_count());
    gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_CENTER, color_black,
              counter);
}

#endif /* CONFIG_MESSAGES */
