/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hwconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

enum notification_type {
    NOTIFY_NONE = 0,
    NOTIFY_TONE,
    NOTIFY_VIBE,      /* reserved, not yet implemented */
    NOTIFY_TONE_VIBE, /* reserved, not yet implemented */
};

enum msg_notification_tone {
    MSG_TONE_NOKIA = 0,
    MSG_TONE_CQ,
    MSG_TONE_BEEP,  /* simple single-tone beep, no RTTTL */
    MSG_TONE_COUNT, /* number of entries in the tone table */
};

/**
 * @brief Return the display name for a notification type value.
 * @param type: notification type enum value.
 * @return pointer to a static NUL-terminated string.
 */
const char *notification_type_name(enum notification_type type);

/**
 * @brief Return the display name for a message notification tone index.
 * @param idx: tone index (0 .. MSG_TONE_COUNT - 1).
 * @return pointer to a static NUL-terminated string.
 */
const char *msg_notification_tone_name(uint8_t idx);

/**
 * @brief Begin playing the configured message notification tone.
 *
 * If @p type is NOTIFY_TONE or NOTIFY_TONE_VIBE, the RTTTL tone at
 * @p tone_idx is parsed and played asynchronously via the
 * notification_tick() sequencer.  If a tone is already playing this call
 * is a no-op (tones do not preempt each other).
 *
 * @param type: notification type setting (NOTIFY_NONE, NOTIFY_TONE, etc.).
 * @param tone_idx: tone index (0 .. MSG_TONE_COUNT - 1).
 */
void notification_play_message_tone(enum notification_type type,
                                    uint8_t tone_idx);

/**
 * @brief Play a tone preview by index, regardless of notification_type.
 *
 * Bypasses the notification_type check so the tone can be previewed while
 * changing the tone setting.  Otherwise identical to
 * notification_play_message_tone().
 */
void notification_play_tone_preview(uint8_t tone_idx);

/**
 * @brief Check if a notification tone is currently playing.
 *
 * Queried by voicePrompts.c so a keypress/menu beep does not start on top
 * of an in-progress notification tone and steal the beeper hardware.
 *
 * @return true if a tone is currently being played by notification_tick().
 */
bool notification_toneActive(void);

/**
 * @brief Periodic tick for the notification tone sequencer.
 *
 * Must be called each UI cycle (typically from the main loop in threads.c).
 * Drives note timing using getTick() and directly calls
 * platform_beepStart()/platform_beepStop() and audioPath_request/release().
 */
void notification_tick(void);

#ifdef __cplusplus
}
#endif

#endif /* NOTIFICATION_H */
