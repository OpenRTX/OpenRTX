/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdio.h>
#include <string.h>
#include "ui/ui_default.h"
#include "ui/ui_strings.h"
#include "core/messages.h"
#include "core/graphics.h"
#include "core/input.h"

/* Forward declarations of layout globals defined in ui.c */
extern layout_t layout;
extern state_t last_state;

/**
 * @brief Draw the message inbox list view.
 *
 * Shows up to one screen's worth of inbox entries (newest first).
 * The highlighted entry is indicated by a filled white rectangle.
 * An unread indicator "*" is shown to the left of unread entries.
 * If the inbox is empty, "No messages" is displayed.
 *
 * The "New" affordance appears in the bottom bar when at least one
 * compose-capable source is registered.
 *
 * @param ui_state: pointer to current UI state; uses menu_selected as the
 *                  highlighted row index.
 */
void _ui_drawMessagesList(ui_state_t *ui_state)
{
    gfx_clearScreen();

    /* Title bar */
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER, color_white,
              currentLanguage->messages);

    size_t total = messages_count();

    if (total == 0) {
        point_t center = { CONFIG_SCREEN_WIDTH / 2, CONFIG_SCREEN_HEIGHT / 2 };
        gfx_print(center, layout.menu_font, TEXT_ALIGN_CENTER, color_white,
                  currentLanguage->noMessages);

        /* Show "New" hint in bottom bar if a compose source matches current mode. */
        if (messages_can_compose(last_state.channel.mode)) {
            point_t bot = { layout.bottom_pos.x,
                            CONFIG_SCREEN_HEIGHT - layout.bottom_h / 2 };
            gfx_print(bot, layout.top_font, TEXT_ALIGN_RIGHT, color_white,
                      currentLanguage->newMessage);
        }
        return;
    }

    point_t pos = layout.line1_pos;
    uint8_t entries_in_screen =
        (CONFIG_SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t selected = ui_state->menu_selected;
    uint8_t scroll = 0;

    if (selected >= entries_in_screen)
        scroll = selected - entries_in_screen + 1;

    for (uint8_t row = 0; pos.y < CONFIG_SCREEN_HEIGHT; row++) {
        size_t idx = row + scroll;
        if (idx >= total)
            break;

        message_header_t *hdr = messages_get(idx);
        if (hdr == NULL)
            break;

        color_t text_color = color_white;
        if (idx == selected) {
            text_color = color_black;
            point_t rect_pos = { 0, pos.y - layout.menu_h + 3 };
            gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h,
                         color_white, true);
        }

        {
            char line[MAX_ENTRY_LEN] = { 0 };
            /* Unread indicator + other party + truncated body.
             * For TX show recipient; for RX show sender.
             * Format: "U CCCCCC BBBBBBBB"
             * 1 + 6 + 1 + 8 = 16 chars; pixel width at 8pt fits 160px. */
            const char *other = (hdr->direction == MSG_DIR_TX) ?
                                    hdr->recipient :
                                    hdr->sender;
            const char *body_text = (hdr->body != NULL) ?
                                        (const char *)hdr->body :
                                        "";
            sniprintf(line, sizeof(line), "%s%-6.6s %.8s",
                      hdr->unread ? "*" : " ", other, body_text);
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, text_color, line);
        }
        pos.y += layout.menu_h;
    }

    /* Virtual "New Message" item at the end of the list */
    if (can_compose && pos.y < CONFIG_SCREEN_HEIGHT) {
        color_t text_color = color_white;
        if ((size_t)selected >= total) {
            text_color = color_black;
            point_t rect_pos = { 0, pos.y - layout.menu_h + 3 };
            gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h,
                         color_white, true);
        }
        gfx_print(pos, layout.menu_font, TEXT_ALIGN_CENTER, text_color,
                  currentLanguage->newMessage);
        pos.y += layout.menu_h;
    }
}

/**
 * @brief Draw the message detail view.
 *
 * Renders the header (sender + status line) then dispatches the body
 * rendering to the source's vtable->render_detail callback.
 *
 * "Reply" is shown in the bottom bar if MSG_ACTION_REPLY is supported.
 *
 * @param ui_state: pointer to current UI state; menu_selected is the
 *                  snapshot index of the message being displayed.
 */
void _ui_drawMessagesDetail(ui_state_t *ui_state)
{
    gfx_clearScreen();

    size_t idx = ui_state->menu_selected;
    message_header_t *hdr = messages_get(idx);
    if (hdr == NULL) {
        /* Entry disappeared (evicted between ticks). */
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, currentLanguage->messages);
        return;
    }

    /* ---- Inverted header bar: "SENDER > RECIPIENT" ---- */
    /* Clamp scroll each frame before drawing. */
    if (ui_state->detail_scroll > ui_state->detail_scroll_max)
        ui_state->detail_scroll = ui_state->detail_scroll_max;
    if (ui_state->detail_scroll < 0)
        ui_state->detail_scroll = 0;

    /* Full-width filled bar at the top, same height as the standard top
     * bar.  Baseline sits at layout.top_pos.y so the font aligns with
     * every other screen's title. */
    point_t hdr_rect = { 0, 0 };
    gfx_drawRect(hdr_rect, CONFIG_SCREEN_WIDTH, layout.top_h, color_white,
                 true);

    /* "sender > recipient" (or "ALL" when recipient is empty broadcast) */
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

    /* Measure total text height to compute scroll max. */
    uint16_t text_h =
        gfx_measureText(layout.menu_font, body_text, layout.horizontal_pad,
                        CONFIG_SCREEN_WIDTH - layout.horizontal_pad, SIZE_MAX);
    int16_t visible_h = clip_bot - clip_top;
    ui_state->detail_scroll_max = (text_h > (uint16_t)visible_h) ?
                                      (int16_t)(text_h - visible_h) :
                                      0;

    /* Re-clamp after update */
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
    if (messages_source_mode(idx) == last_state.channel.mode)
        gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_LEFT, color_black,
                  currentLanguage->reply);

    char counter[12] = { 0 };
    sniprintf(counter, sizeof(counter), "%u/%u", (unsigned)(idx + 1),
              (unsigned)messages_count());
    gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_CENTER, color_black,
              counter);
}

/**
 * @brief Handle a compose-source picker for the "New" affordance.
 *
 * If exactly one compose-capable source is registered, open it directly.
 * If more than one, display a simple numbered list and route key presses.
 * This function is called from the FSM when the user triggers "New".
 *
 * @param ui_state: pointer to current UI state.
 * @param msg: the key event that triggered the compose flow.
 * @return true if a compose source was launched.
 */
bool _ui_messagesStartCompose(ui_state_t *ui_state, kbd_msg_t msg)
{
    (void)ui_state;
    (void)msg;
    return messages_start_compose(last_state.channel.mode) == 0;
}
