/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "ui/ui_default.h"
#include "ui/ui_strings.h"
#include "core/input.h"
#include "core/messages.h"
#include "core/voicePromptUtils.h"
#include "rtx/rtx.h"

/* Generic menu list renderer, implementation is in "ui_menu.c". Reused here
 * so the message list gets the same scrolling/selection/voice-announcement
 * behaviour as every other list screen (banks, channels, contacts, ...). */
extern void _ui_drawMenuList(uint8_t selected,
                             int (*getCurrentEntry)(char *buf, uint8_t max_len,
                                                    uint8_t index));

/* Keypad text entry, implementation is in "ui.c". These edit the buffer
 * passed in, keeping their cursor in the UI state. */
extern void _ui_textInputReset(char *buf, size_t bufSize);
extern void _ui_textInputKeypad(char *buf, uint16_t max_len, kbd_msg_t msg,
                                bool callsign);
extern void _ui_textInputConfirm(char *buf);
extern void _ui_textInputDel(char *buf);

/* Rows of the compose form, top to bottom, as held in compose_focus. */
enum composeRow {
    COMPOSE_TO = 0,
    COMPOSE_BODY,
    COMPOSE_SEND,
    COMPOSE_ROWS,
};

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

        /* Full-length callsign, so that e.g. "N0CALL-1" and "N0CALL-2"
         * stay distinguishable; the preview takes what is left. */
        sniprintf(buf, max_len, "%s%-*.*s %s", marker, MSG_ADDR_MAX_LEN - 1,
                  MSG_ADDR_MAX_LEN - 1, other, msg->body);
        return 0;
    }

    /* Virtual "New Message" item at the end of the list */
    if (index == count && messages_canCompose(last_state.channel.mode)) {
        sniprintf(buf, max_len, "  %s", currentLanguage->newMessage);
        return 0;
    }

    return -1;
}

static size_t _ui_messagesListSelection(const ui_state_t *ui_state)
{
    uint32_t pinned = ui_state->messages_list_seq;

    /* The virtual "New Message" row only exists while compose is possible;
     * if the mode changed since it was pinned, fall back to the newest. */
    if (pinned == UINT32_MAX)
        return messages_canCompose(last_state.channel.mode) ? messages_count() :
                                                              0;

    if (pinned != 0) {
        size_t idx = messages_findBySequence(pinned);
        if (idx != SIZE_MAX)
            return idx;
    }

    /* Not pinned yet (screen just entered) or the pinned message is gone:
     * land on the newest entry. */
    return 0;
}

/**
 * \internal Read a message out: the other party's address, then the body.
 * The voice prompt sequence is bounded, so a long body is cut short.
 */
static void announceMessage(const struct message *msg)
{
    const char *other = (msg->direction == MSG_DIR_TX) ? msg->recipient :
                                                         msg->sender;

    vp_flush();
    vp_queueString(other, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_SILENCE);
    vp_queueString(msg->body, vpAnnounceCommonSymbols);
    vp_play();
}

/**
 * \internal Open entry @p idx in the detail view: pin it, mark it as seen and
 * read it out.
 *
 * @param scroll: initial body scroll offset; INT16_MAX lands on the tail,
 *                since the draw code clamps it to that message's maximum.
 */
static void openMessage(ui_state_t *ui_state, size_t idx, int16_t scroll)
{
    const struct message *msg = messages_get(idx);
    if (msg == NULL)
        return;

    /* Pin by sequence; see messages_detail_seq docs. */
    ui_state->messages_detail_seq = msg->sequence;
    ui_state->messages_list_seq = msg->sequence;
    ui_state->detail_scroll = scroll;
    ui_state->detail_scroll_max = 0;
    announceMessage(msg);
    messages_markRead(idx, true);
}

/**
 * \internal Recipient as resolved for display and transmission: the one typed
 * (or prefilled) in the To field, or @p broadcast when it is left empty.
 */
static const char *composeRecipient(const ui_state_t *ui_state,
                                    const char *broadcast)
{
    if (ui_state->compose_recipient[0] != '\0')
        return ui_state->compose_recipient;

    return broadcast;
}

/**
 * \internal Announce the compose row that has the focus, with its value.
 */
static void announceComposeFocus(const ui_state_t *ui_state)
{
    vp_flush();

    switch (ui_state->compose_focus) {
        case COMPOSE_TO:
            vp_queueStringTableEntry(&currentLanguage->to);
            if (ui_state->compose_recipient[0] != '\0')
                vp_queueString(ui_state->compose_recipient,
                               vpAnnounceCommonSymbols);
            else
                vp_queueStringTableEntry(&currentLanguage->broadcast);
            break;

        case COMPOSE_BODY:
            vp_queueStringTableEntry(&currentLanguage->message);
            vp_queueString(ui_state->compose_body, vpAnnounceCommonSymbols);
            break;

        default:
            vp_queueStringTableEntry(&currentLanguage->send);
            break;
    }

    vp_play();
}

/**
 * \internal Reset MESSAGES_COMPOSE state and switch to it, prefilling the
 * recipient. Shared by the "New Message" and "Reply" entry points so a
 * future compose-state field only needs to be initialised in one place.
 *
 * @param recipient: initial recipient string, or NULL to leave it empty.
 * @param is_reply:  whether this compose session was opened via Reply.
 */
static void startCompose(ui_state_t *ui_state, const char *recipient,
                         bool is_reply)
{
    /* The draft buffer exists only while the compose screen is up; the
     * draw and input code rely on it, so stay on the current screen if
     * the allocation fails. */
    if (ui_state->compose_body == NULL)
        ui_state->compose_body = malloc(MSG_BODY_MAX_LEN);
    if (ui_state->compose_body == NULL)
        return;

    memset(ui_state->compose_recipient, 0, sizeof(ui_state->compose_recipient));
    if (recipient != NULL)
        strncpy(ui_state->compose_recipient, recipient,
                sizeof(ui_state->compose_recipient) - 1);
    memset(ui_state->compose_body, 0, MSG_BODY_MAX_LEN);
    ui_state->input_position = 0;
    ui_state->input_number = 0;
    ui_state->input_set = 0;
    ui_state->last_keypress = 0;
    ui_state->compose_focus = COMPOSE_BODY;
    ui_state->compose_editing = false;
    /* Land directly in body text-entry so the user can type immediately;
     * the empty-draft cursor state matches the COMPOSE_BODY ENTER branch. */
    ui_state->compose_body_editing = true;
    ui_state->compose_is_reply = is_reply;
    state.ui_screen = MESSAGES_COMPOSE;

    vp_flush();
    vp_queueStringTableEntry(is_reply ? &currentLanguage->reply :
                                        &currentLanguage->newMessage);
    vp_queueStringTableEntry(&currentLanguage->message);
    vp_queuePrompt(PROMPT_EDIT);
    vp_play();
}

void _ui_endCompose(ui_state_t *ui_state)
{
    free(ui_state->compose_body);
    ui_state->compose_body = NULL;
    state.ui_screen = MESSAGES_LIST;
}

void _ui_fsmMessagesList(ui_state_t *ui_state, kbd_msg_t msg)
{
    uint8_t mode = last_state.channel.mode;
    bool can_compose = messages_canCompose(mode);
    size_t count = messages_count();
    size_t total = count + (can_compose ? 1 : 0);

    // Resolve the highlighted row from the pinned sequence number; the
    // messages_list_seq field docs explain why an index cannot be trusted
    // across ticks.
    size_t idx = _ui_messagesListSelection(ui_state);

    if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT) {
        if (idx > 0)
            idx--;
        else if (total > 0)
            idx = total - 1;
    } else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT) {
        if (idx + 1 < total)
            idx++;
        else
            idx = 0;
    } else if (msg.keys & KEY_ENTER) {
        if (idx < count) {
            openMessage(ui_state, idx, 0);
            state.ui_screen = MESSAGES_DETAIL;
            return;
        }

        if (idx == count && can_compose) {
            /* Virtual "New Message" item selected. On M17 the channel's
             * destination is the natural default recipient. */
            const char *dest = (mode == OPMODE_M17) ?
                                   last_state.settings.m17_dest :
                                   NULL;
            startCompose(ui_state, dest, false);
            return;
        }
    } else if (msg.keys & KEY_ESC) {
        // Unpin, so re-entry starts at the newest message.
        ui_state->messages_list_seq = 0;
        ui_state->menu_selected = 0;
        state.ui_screen = MENU_TOP;
        return;
    }

    // Re-pin the (possibly updated) selection.
    if (idx < count) {
        const struct message *sel = messages_get(idx);
        ui_state->messages_list_seq = (sel != NULL) ? sel->sequence : 0;
    } else if (idx == count && can_compose) {
        ui_state->messages_list_seq = UINT32_MAX;
    } else {
        ui_state->messages_list_seq = 0;
    }
}

void _ui_fsmMessagesDetail(ui_state_t *ui_state, kbd_msg_t msg)
{
    size_t idx = messages_findBySequence(ui_state->messages_detail_seq);
    if (idx == SIZE_MAX) {
        // The message was deleted or evicted while being viewed: bounce back
        // to the list rather than showing stale/empty content.
        ui_state->messages_list_seq = 0;
        state.ui_screen = MESSAGES_LIST;
        return;
    }

    /* Scroll one rendered body line per step. */
    const int16_t scrollStep = (int16_t)gfx_getFontHeight(layout.message_font);

    if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT) {
        if (ui_state->detail_scroll > 0) {
            ui_state->detail_scroll -= scrollStep;
            if (ui_state->detail_scroll < 0)
                ui_state->detail_scroll = 0;
        } else if (idx > 0) {
            /* Edge: go to the previous message, landing on its tail.
             * Navigate from the freshly resolved idx, so a rebuild while the
             * message is open cannot skip or repeat one. */
            openMessage(ui_state, idx - 1, INT16_MAX);
        }
    } else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT) {
        if (ui_state->detail_scroll < ui_state->detail_scroll_max) {
            ui_state->detail_scroll += scrollStep;
            if (ui_state->detail_scroll > ui_state->detail_scroll_max)
                ui_state->detail_scroll = ui_state->detail_scroll_max;
        } else if (idx + 1 < messages_count()) {
            /* Edge: go to the next message, from its top. */
            openMessage(ui_state, idx + 1, 0);
        }
    } else if (msg.keys & KEY_ESC) {
        state.ui_screen = MESSAGES_LIST;
    } else if (msg.keys & KEY_ENTER) {
        /* Only received messages of the current mode can be replied to. */
        const struct message *entry = messages_get(idx);
        if ((entry != NULL) && (entry->mode == last_state.channel.mode)
            && (entry->direction == MSG_DIR_RX)
            && messages_canCompose(entry->mode)) {
            startCompose(ui_state, entry->sender, true);
        }
    }
}

/**
 * \internal Input handling for the To-field editing overlay.
 */
static void composeRecipientInput(ui_state_t *ui_state, kbd_msg_t msg)
{
    if (msg.keys & KEY_ENTER) {
        _ui_textInputConfirm(ui_state->new_callsign);
        /* An untouched field is just the "_" reset placeholder; store it as
         * empty, which means broadcast, rather than the literal "_". */
        const char *entered = ui_state->new_callsign;
        if (entered[0] == '_' && entered[1] == '\0')
            entered = "";
        memset(ui_state->compose_recipient, 0,
               sizeof(ui_state->compose_recipient));
        strncpy(ui_state->compose_recipient, entered,
                sizeof(ui_state->compose_recipient) - 1);
        ui_state->compose_editing = false;
        announceComposeFocus(ui_state);
    } else if (msg.keys & KEY_ESC) {
        /* Discard: compose_recipient is left unchanged */
        ui_state->compose_editing = false;
        announceComposeFocus(ui_state);
    } else if (msg.keys & KEY_UP || msg.keys & KEY_DOWN || msg.keys & KEY_LEFT
               || msg.keys & KEY_RIGHT) {
        _ui_textInputDel(ui_state->new_callsign);
    } else if (input_isCharPressed(msg)) {
        _ui_textInputKeypad(ui_state->new_callsign,
                            sizeof(ui_state->new_callsign) - 1, msg, true);
    }
}

/**
 * \internal Input handling while typing the message body.
 */
static void composeBodyInput(ui_state_t *ui_state, kbd_msg_t msg)
{
    /* Any arrow key or knob deletes the last character, same as M17
     * destination entry on the VFO screen (universally available). */
    if (msg.keys & KEY_UP || msg.keys & KEY_DOWN || msg.keys & KEY_LEFT
        || msg.keys & KEY_RIGHT || msg.keys & KNOB_LEFT
        || msg.keys & KNOB_RIGHT) {
        _ui_textInputDel(ui_state->compose_body);
    } else if (input_isCharPressed(msg)) {
        _ui_textInputKeypad(ui_state->compose_body, MSG_BODY_MAX_LEN - 1, msg,
                            false);
    } else if (msg.keys & KEY_ENTER || msg.keys & KEY_ESC) {
        /* Confirm the pending multi-tap char, leave body editing but stay
         * on the body row. */
        _ui_textInputConfirm(ui_state->compose_body);
        ui_state->compose_body_editing = false;
        announceComposeFocus(ui_state);
    }
}

/**
 * \internal Hand the draft to the inbox for transmission.
 *
 * @return true if it was handed over and compose can be closed.
 */
static bool composeSend(ui_state_t *ui_state)
{
    size_t len = strnlen(ui_state->compose_body, MSG_BODY_MAX_LEN);
    if (len == 0)
        return false;

    /* An empty To field means broadcast, which goes on the air as the
     * source's wire address, never the localized display string. */
    uint8_t mode = last_state.channel.mode;
    const char *rcpt = composeRecipient(ui_state, messages_broadcastAddr(mode));
    if (rcpt == NULL) {
        vp_announceError(vpqInit | vpqPlayImmediately);
        return false;
    }

    struct message txMsg = { 0 };
    txMsg.body = ui_state->compose_body;
    txMsg.bodyLen = len;
    txMsg.mode = mode;
    strncpy(txMsg.sender, last_state.settings.callsign,
            sizeof(txMsg.sender) - 1);
    strncpy(txMsg.recipient, rcpt, sizeof(txMsg.recipient) - 1);

    /* Close compose regardless of the immediate result: the entry records
     * its own SENDING/SENT/FAILED status in the inbox, so leaving compose
     * open on a transient failure (e.g. -EBUSY while a previous send is
     * still in flight) would only strand the user and store a duplicate
     * entry on each retry. */
    messages_send(&txMsg);
    return true;
}

void _ui_fsmMessagesCompose(ui_state_t *ui_state, kbd_msg_t msg)
{
    if (ui_state->compose_editing) {
        composeRecipientInput(ui_state, msg);
        return;
    }

    if (ui_state->compose_focus == COMPOSE_BODY
        && ui_state->compose_body_editing) {
        composeBodyInput(ui_state, msg);
        return;
    }

    /* Form navigation. No body edit is pending here, and input_position may
     * hold a stale cursor from the To-field overlay, so nothing is confirmed
     * on the way: that would truncate compose_body at the wrong offset. */
    if (msg.keys & KEY_ESC) {
        _ui_endCompose(ui_state);
    } else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT) {
        ui_state->compose_focus = (ui_state->compose_focus + 1) % COMPOSE_ROWS;
        announceComposeFocus(ui_state);
    } else if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT) {
        ui_state->compose_focus = (ui_state->compose_focus == 0) ?
                                      COMPOSE_ROWS - 1 :
                                      ui_state->compose_focus - 1;
        announceComposeFocus(ui_state);
    } else if (msg.keys & KEY_ENTER) {
        switch (ui_state->compose_focus) {
            case COMPOSE_TO: {
                /* Open the To overlay: seed new_callsign from the current
                 * recipient, cursor at its end. */
                memset(ui_state->new_callsign, 0,
                       sizeof(ui_state->new_callsign));
                strncpy(ui_state->new_callsign, ui_state->compose_recipient,
                        sizeof(ui_state->new_callsign) - 1);
                size_t rlen = strnlen(ui_state->new_callsign,
                                      sizeof(ui_state->new_callsign));
                if (rlen > 0) {
                    ui_state->input_position = rlen - 1;
                    ui_state->input_set = 0;
                    ui_state->last_keypress = 1;
                    ui_state->input_number = 0;
                } else {
                    _ui_textInputReset(ui_state->new_callsign,
                                       sizeof(ui_state->new_callsign));
                }
                ui_state->compose_editing = true;
                vp_announceBuffer(&currentLanguage->to, true, true,
                                  ui_state->compose_recipient);
                break;
            }

            case COMPOSE_BODY: {
                /* Activate body text entry with the cursor at the end of
                 * any existing text, so re-entering to continue typing does
                 * not clobber what is already there. */
                ui_state->compose_body_editing = true;
                size_t blen = strnlen(ui_state->compose_body, MSG_BODY_MAX_LEN);
                if (blen > 0) {
                    ui_state->input_position = blen - 1;
                    ui_state->last_keypress = 1;
                } else {
                    ui_state->input_position = 0;
                    ui_state->last_keypress = 0;
                }
                ui_state->input_number = 0;
                ui_state->input_set = 0;
                vp_announceBuffer(&currentLanguage->message, true, false,
                                  ui_state->compose_body);
                break;
            }

            default:
                if (composeSend(ui_state))
                    _ui_endCompose(ui_state);
                break;
        }
    }
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
    if ((msg->mode == last_state.channel.mode) && (msg->direction == MSG_DIR_RX)
        && messages_canCompose(msg->mode))
        gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_LEFT, color_black,
                  currentLanguage->reply);

    char counter[12] = { 0 };
    sniprintf(counter, sizeof(counter), "%u/%u", (unsigned)(idx + 1),
              (unsigned)messages_count());
    gfx_print(bot_pos, layout.top_font, TEXT_ALIGN_CENTER, color_black,
              counter);
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

    /* ---- To row ---- */
    point_t to_rpos = layout.line1_pos;
    bool to_selected = !ui_state->compose_editing
                    && ui_state->compose_focus == COMPOSE_TO;

    if (to_selected) {
        point_t rect_pos = { 0, (int16_t)(to_rpos.y - layout.menu_h + 3) };
        gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white,
                     true);
    }

    color_t to_col = to_selected ? color_black : color_white;
    {
        const char *rcpt = composeRecipient(ui_state,
                                            currentLanguage->broadcast);
        char val[11] = { 0 };
        sniprintf(val, sizeof(val), "%.10s", rcpt);
        gfx_print(to_rpos, layout.menu_font, TEXT_ALIGN_LEFT, to_col, "%s",
                  currentLanguage->to);
        gfx_print(to_rpos, layout.menu_font, TEXT_ALIGN_RIGHT, to_col, "%s",
                  val);
    }

    /* ---- Body area ---- */
    static const uint8_t BOX_PAD = 2;
    int16_t to_row_top = (int16_t)(to_rpos.y - layout.menu_h + 3);
    int16_t body_top = (int16_t)(to_row_top + layout.menu_h);
    int16_t send_h_top = (int16_t)(layout.bottom_pos.y - layout.menu_h + 3);
    int16_t body_bot = (int16_t)(send_h_top - 1);
    uint16_t body_w =
        (uint16_t)(CONFIG_SCREEN_WIDTH - 2u * layout.horizontal_pad);
    uint16_t body_h = (uint16_t)(body_bot - body_top + 1);
    point_t body_orig = { (int16_t)layout.horizontal_pad, body_top };
    bool body_editing = ui_state->compose_focus == COMPOSE_BODY
                     && ui_state->compose_body_editing;

    if (body_editing)
        gfx_drawRect(body_orig, body_w, body_h, color_white, true);
    gfx_drawRect(body_orig, body_w, body_h, color_white, false);

    color_t body_col = body_editing ? color_black : color_white;
    uint8_t font_h = gfx_getFontHeight(layout.message_font);
    uint16_t max_x = (uint16_t)(layout.horizontal_pad + body_w - BOX_PAD);
    int16_t clip_top = (int16_t)(body_top + 1);
    int16_t clip_bot = (int16_t)(body_bot - 1);

    /* input_position is the body cursor only while the body is being
     * edited; otherwise it may belong to the To-field overlay, so keep the
     * end of the text in view instead. */
    size_t cursor = ui_state->input_position;
    if (!body_editing) {
        size_t blen = strnlen(ui_state->compose_body, MSG_BODY_MAX_LEN);
        cursor = (blen > 0) ? blen - 1 : 0;
    }

    int16_t scroll_offset = gfx_scrollOffsetForCursor(
        layout.message_font, ui_state->compose_body, cursor,
        (uint16_t)(layout.horizontal_pad + BOX_PAD), max_x,
        clip_bot - clip_top);

    point_t text_start = { (int16_t)(layout.horizontal_pad + BOX_PAD),
                           (int16_t)(clip_top + font_h - scroll_offset) };
    gfx_printBufferClipped(text_start, layout.message_font, TEXT_ALIGN_LEFT,
                           body_col, ui_state->compose_body, max_x, clip_top,
                           clip_bot);

    /* ---- Send row ---- */
    point_t send_rpos = { (int16_t)layout.horizontal_pad, layout.bottom_pos.y };
    bool send_selected = ui_state->compose_focus == COMPOSE_SEND;
    if (send_selected) {
        point_t rect_pos = { 0, (int16_t)(send_rpos.y - layout.menu_h + 3) };
        gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h, color_white,
                     true);
    }
    color_t send_col = send_selected ? color_black : color_white;
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
