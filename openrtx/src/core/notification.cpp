/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"

#include "core/notification.h"
#include "core/rtttl.h"
#include "interfaces/platform.h"
#include "interfaces/audio.h"
#include "interfaces/delays.h"
#include "core/audio_path.h"
#include "core/voicePrompts.h"

/* ------------------------------------------------------------------
 * Tone table
 * ------------------------------------------------------------------ */

struct toneEntry {
    const char *name;
    const char *rtttl;
};

static const struct toneEntry tone_table[] = {
    { "Nokia", "24:d=4,o=5,b=180:8e6,8d6,f#,g#,8c#6,8b,d,e,8b,8a,c#,e,2a" },
    { "CQ", "24:d=16,o=6,b=120:8c,p,c,p,8c,p,c,4p,8c,p,8c,p,c,p,8c,8p" },
};

/* tone_table[] holds every RTTTL-driven tone; MSG_TONE_BEEP is synthesized
 * directly (see parse_tone()) rather than table-driven, so it must sit
 * immediately after the table in the enum. */
static_assert(sizeof(tone_table) / sizeof(tone_table[0]) == MSG_TONE_BEEP,
              "tone_table must have exactly MSG_TONE_BEEP entries");

/* ------------------------------------------------------------------
 * Sequencer state
 * ------------------------------------------------------------------ */

static struct {
    struct rtttl_note notes[RTTTL_MAX_NOTES];
    uint8_t count;
    uint8_t current;
    long long start_tick;
    pathId audio_path;
    bool playing;
} seq;

/* ------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------ */

const char *notification_type_name(enum notification_type type)
{
    switch (type) {
        case NOTIFY_NONE:
            return "None";
        case NOTIFY_TONE:
            return "Tone";
        case NOTIFY_VIBE:
            return "Vibe";
        case NOTIFY_TONE_VIBE:
            return "Tone+Vibe";
        default:
            return "";
    }
}

const char *msg_notification_tone_name(uint8_t idx)
{
    if (idx == MSG_TONE_BEEP)
        return "Beep";
    if (idx >= MSG_TONE_COUNT)
        return "";
    return tone_table[idx].name;
}

/**
 * Parse a tone directly into the sequencer's own note buffer (seq.notes),
 * setting seq.count.  No intermediate stack buffer or copy: the caller
 * already guarantees seq.playing is false before this runs, so seq.notes
 * is not in use.
 */
static bool parse_tone(uint8_t tone_idx)
{
    if (tone_idx == MSG_TONE_BEEP) {
        seq.notes[0].freq_hz = 800;
        seq.notes[0].duration_ms = 500;
        seq.count = 1;
        return true;
    }

    if (!rtttl_parse(tone_table[tone_idx].rtttl, seq.notes, &seq.count))
        return false;

    return seq.count > 0;
}

/**
 * Start playback of the tone already parsed into seq.notes/seq.count.
 * Requires seq.count > 0 and seq.playing == false (checked by callers).
 */
static void start_tone_playback(void)
{
    pathId pid = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_BEEP);
    if (pid < 0)
        return;

    seq.current = 0;
    seq.start_tick = getTick();
    seq.audio_path = pid;
    seq.playing = true;

    /* The first note may itself be a pause (freq_hz == 0): only start the
     * beeper for an audible note, same as notification_tick() does when
     * advancing between notes. */
    if (seq.notes[0].freq_hz > 0)
        platform_beepStart(seq.notes[0].freq_hz);
}

void notification_play_message_tone(enum notification_type type,
                                    uint8_t tone_idx)
{
    if (type != NOTIFY_TONE && type != NOTIFY_TONE_VIBE)
        return;

    notification_play_tone_preview(tone_idx);
}

bool notification_toneActive(void)
{
    return seq.playing;
}

void notification_play_tone_preview(uint8_t tone_idx)
{
    if (seq.playing)
        return;

    // Don't steal the beeper out from under an in-progress keypress/menu
    // beep (a voice prompt already blocks us via audioPath_request's
    // priority arbitration in start_tone_playback()).
    if (vp_beepBusy())
        return;

    if (tone_idx >= MSG_TONE_COUNT)
        return;

    if (!parse_tone(tone_idx))
        return;

    start_tone_playback();
}

void notification_tick(void)
{
    if (!seq.playing)
        return;

    long long now = getTick();
    long long elapsed = now - seq.start_tick;

    if (elapsed >= (long long)seq.notes[seq.current].duration_ms) {
        platform_beepStop();

        seq.current++;
        if (seq.current >= seq.count) {
            /* All notes played — clean up */
            audioPath_release(seq.audio_path);
            seq.playing = false;
            return;
        }

        /* Start next note — use cumulative timing to avoid drift */
        seq.start_tick += seq.notes[seq.current - 1].duration_ms;
        if (seq.notes[seq.current].freq_hz > 0)
            platform_beepStart(seq.notes[seq.current].freq_hz);
    }
}
