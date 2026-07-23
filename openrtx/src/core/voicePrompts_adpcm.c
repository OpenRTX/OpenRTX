/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * IMA-ADPCM voice-prompt player -- a drop-in alternative to the codec2-based
 * core/voicePrompts.c.  A target's build links EITHER this file OR
 * voicePrompts.c: both expose the same vp_* API (so voicePromptUtils.c and the
 * UI announce paths work unchanged) and share the queue / user-dictionary /
 * symbol / beep logic; only the decode core differs -- IMA-ADPCM here vs codec2
 * there.
 *
 * Why: codec2 3200 software decode can't make realtime on a soft-float core
 * (proven on the HD2's CK803S, 2026-06-10 -- slow, jittery, and the long decode
 * pegged the CPU and froze the UI).  IMA-ADPCM is integer-only and decodes far
 * faster than realtime (proven: 27/27 stream halves, no underrun), so slow /
 * soft-float targets can opt into it.
 *
 * Data: an ADPCM container ('VPA1') embedded by core/voicePromptData_adpcm.S
 * (provides _vpadata_start / _vpadata_end), built from the stock
 * voiceprompts.vpc by scripts/vpc_to_adpcm.py.  Any clip absent from the
 * container plays as silence (graceful degradation).
 *
 * Architecture: single-threaded, like the stock player.  vp_play() opens the
 * stream; vp_tick() (the UI main_thread) decodes one 160-sample half per call,
 * paced by the output stream's syncpoint (blocks <= 20 ms).  ADPCM decode is
 * microseconds, so blocking vp_tick on the syncpoint just paces the UI loop to
 * ~50 Hz during a prompt -- no separate thread, no detach/join races.
 */

#include "interfaces/platform.h"
#include "interfaces/keyboard.h"
#include "core/voicePromptUtils.h"
#include "ui/ui_strings.h"
#include "core/voicePrompts.h"
#include "core/audio_path.h"
#include "core/audio_stream.h"
#include "core/state.h"
#include "core/beeps.h"
#include <strings.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codec/adpcm_ima.h"

#define VP_SEQUENCE_BUF_SIZE 128
#define BEEP_SEQ_BUF_SIZE 256
#define VPA_MAGIC 0x31415056u /* 'VPA1' little-endian */
#define STREAM_HALF 160u      /* samples per circular half (20 ms) */
#define STREAM_SAMPLES (STREAM_HALF * 2u)
#define LEADIN_SILENCE_MS 60u /* mask the amp turn-on transient */

/* --- embedded ADPCM container (core/voicePromptData_adpcm.S) --- */
extern const uint8_t _vpadata_start;
extern const uint8_t _vpadata_end;

typedef struct {
    const char *userWord;
    const enum voicePrompt vp;
} userDictEntry_t;

typedef struct {
    uint16_t freq;
    uint16_t duration;
} beepData_t;

static const userDictEntry_t userDictionary[] = {
    { "hotspot", PROMPT_CUSTOM1 },
    { "clearnode", PROMPT_CUSTOM2 },
    { "sharinode", PROMPT_CUSTOM3 },
    { "microhub", PROMPT_CUSTOM4 },
    { "openspot", PROMPT_CUSTOM5 },
    { "repeater", PROMPT_CUSTOM6 },
    { "blindhams", PROMPT_CUSTOM7 },
    { "allstar", PROMPT_CUSTOM8 },
    { "parrot", PROMPT_CUSTOM9 },
    { "channel", PROMPT_CHANNEL },
    { 0, 0 }
};

/* Queue of prompt indices to play. */
static uint16_t vpSequence[VP_SEQUENCE_BUF_SIZE];
static uint16_t vpSeqLength = 0;

/* Container view. */
static const uint8_t *vpData = NULL;
static const uint32_t *vpToc = NULL; /* count+1 byte offsets into vpClips */
static const uint8_t *vpClips = NULL;
static uint32_t vpCount = 0;
static bool vpDataLoaded = false;

static bool voicePromptActive = false;
static pathId vpAudioPath;

/* Inline decode state (single-threaded, advanced one half per vp_tick). */
static streamId vpStream = -1;
static uint16_t vpPos = 0;           /* index into vpSequence              */
static const uint8_t *vpClip = NULL; /* current clip ADPCM bytes           */
static uint32_t vpNibbles = 0;       /* samples in current clip            */
static uint32_t vpNib = 0;           /* next sample within current clip    */
static adpcm_ima_state vpIma;
static unsigned vpLeadin = 0;        /* remaining lead-in silence halves   */

/* Output ring -- one owner (the UI thread, via vp_tick), so a single static
 * buffer is safe.  Two 160-sample halves (20 ms each).  Shared by voice-prompt
 * clips and generated beep tones (they never play simultaneously). */
static int16_t vpStreamBuf[STREAM_SAMPLES];

/* Beep management.  Beeps play a generated square-wave tone through vpStream
 * (see beep_tick), NOT a hardware PWM: on this target the PWM tone only sounds
 * while the codec DAC is actively clocked by a PCM stream, so a bare PWM beep
 * was silent -- a PCM tone through the proven playback path always sounds. */
static beepData_t beepSeriesBuffer[BEEP_SEQ_BUF_SIZE];
static uint16_t currentBeepDuration = 0; /* remaining halves of current beep */
static uint8_t beepSeriesIndex = 0;
static uint16_t beepFreq = 0;            /* current beep tone frequency (Hz) */
static uint32_t beepPhase = 0; /* square-wave phase accum (mod 8 kHz) */

/* ---- audio path helpers ---- */

static inline void enableSpkOutput()
{
    if (audioPath_getStatus(vpAudioPath) == PATH_CLOSED)
        vpAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_PROMPT);
}

static inline void disableSpkOutput()
{
    if ((currentBeepDuration != 0) || (voicePromptActive == true))
        return;
    audioPath_release(vpAudioPath);
}

/* Fill one 160-sample half with a square wave at beepFreq (8 kHz sample rate).
 * A phase accumulator (mod the 8 kHz rate) keeps successive halves and series
 * steps phase-continuous.  freq 0 -> silence. */
static void vpFillBeepHalf(stream_sample_t *dst)
{
    if (beepFreq == 0) {
        memset(dst, 0, STREAM_HALF * sizeof(stream_sample_t));
        return;
    }
    for (unsigned i = 0; i < STREAM_HALF; i++) {
        beepPhase += beepFreq;
        if (beepPhase >= 8000u)
            beepPhase -= 8000u;
        dst[i] = (beepPhase < 4000u) ? 8000 : -8000;
    }
}

/* Open the MCU->SPK path + output stream for a beep (mirrors vp_play's open).
 * Reused across a beep series; a no-op once the stream is already running. */
static bool beep_stream_open()
{
    enableSpkOutput();
    if (audioPath_getStatus(vpAudioPath) != PATH_OPEN)
        return false;
    if (vpStream < 0) {
        /* Pre-fill BOTH halves with the tone so it sounds from frame 0 -- a
         * silent prime would truncate a short keytone (~60 ms) to just a click
         * before the tone-filled halves cycle in. */
        vpFillBeepHalf(&vpStreamBuf[0]);
        vpFillBeepHalf(&vpStreamBuf[STREAM_HALF]);
        vpStream = audioStream_start(vpAudioPath, vpStreamBuf, STREAM_SAMPLES,
                                     8000, STREAM_OUTPUT | BUF_CIRC_DOUBLE);
        if (vpStream < 0) {
            vpStream = -1;
            return false;
        }
    }
    return true;
}

static void beep_flush()
{
    if (vpStream >= 0) {
        audioStream_stop(vpStream);
        vpStream = -1;
    }
    memset(beepSeriesBuffer, 0, sizeof(beepSeriesBuffer));
    currentBeepDuration = 0;
    beepSeriesIndex = 0;
    beepFreq = 0;
    disableSpkOutput();
}

/* One 160-sample tone half per tick, paced by the stream syncpoint; duration
 * counts halves (20 ms each).  Returns true while a beep is in progress so
 * vp_tick skips the voice-prompt pump (beeps and clips share vpStream). */
static bool beep_tick()
{
    if (currentBeepDuration == 0)
        return false;

    if ((vpStream < 0) || (audioPath_getStatus(vpAudioPath) != PATH_OPEN)) {
        beep_flush();
        return true;
    }
    if (outputStream_sync(vpStream, false) == false) {
        beep_flush();
        return true;
    }
    stream_sample_t *idle = outputStream_getIdleBuffer(vpStream);
    if (idle == NULL) {
        beep_flush();
        return true;
    }
    vpFillBeepHalf(idle);

    currentBeepDuration--;
    if (currentBeepDuration == 0) {
        if ((beepSeriesBuffer[beepSeriesIndex + 1].freq != 0)
            && (beepSeriesBuffer[beepSeriesIndex + 1].duration != 0)) {
            beepSeriesIndex++;
            currentBeepDuration = beepSeriesBuffer[beepSeriesIndex].duration;
            beepFreq = beepSeriesBuffer[beepSeriesIndex].freq;
        } else {
            /* Drain the last tone half (let it reach the DAC) before stopping,
             * else the tail is cut and a short beep sounds like a click. */
            outputStream_sync(vpStream, false);
            beep_flush();
        }
    }
    return true;
}

/* ---- dictionary / symbol lookup (identical to stock) ---- */

static uint16_t userDictLookup(const char *ptr, int *advanceBy)
{
    if ((ptr == NULL) || (*ptr == '\0'))
        return 0;
    for (uint32_t i = 0; userDictionary[i].userWord != 0; i++) {
        int len = strlen(userDictionary[i].userWord);
        if (strncasecmp(userDictionary[i].userWord, ptr, len) == 0) {
            *advanceBy = len;
            return userDictionary[i].vp;
        }
    }
    return 0;
}

static bool GetSymbolVPIfItShouldBeAnnounced(char symbol, enum vpFlags flags,
                                             enum voicePrompt *vp)
{
    *vp = PROMPT_SILENCE;
    const char indexedSymbols[] = "%.+-*#!,@:?()~/[]<>=$'`&|_^{}";
    const char commonSymbols[] = "%.+-*#";
    bool announceCommon = (flags & vpAnnounceCommonSymbols) ? true : false;
    bool announceLess = (flags & vpAnnounceLessCommonSymbols) ? true : false;
    char *p = strchr(indexedSymbols, symbol);
    if (p == NULL)
        return (flags & vpAnnounceASCIIValueForUnknownChars) ? true : false;
    bool common = strchr(commonSymbols, symbol) != NULL;
    *vp = PROMPT_PERCENT + (p - indexedSymbols);
    return ((common && announceCommon) || (!common && announceLess));
}

/* ---- inline ADPCM decode (one half per vp_tick) ---- */

/* Load the clip for the prompt at vpSequence[vpPos] into the decode cursor.
 * Empty / missing clips (not in the embedded subset) load as zero-length =
 * silence.  Returns false when the sequence is exhausted. */
static bool vpLoadCurrentClip()
{
    if (vpPos >= vpSeqLength)
        return false;
    uint16_t prompt = vpSequence[vpPos];
    vpClip = NULL;
    vpNibbles = 0;
    vpNib = 0;
    adpcm_ima_reset(&vpIma);
    if (vpDataLoaded && (prompt + 1u <= vpCount)) {
        uint32_t start = vpToc[prompt];
        uint32_t len = vpToc[prompt + 1] - start; /* bytes */
        if (len > 0) {
            vpClip = vpClips + start;
            vpNibbles = len * 2u;
        }
    }
    return true;
}

/* Fill one 160-sample half, drawing seamlessly across clips in the sequence.
 * Returns false when the whole sequence has been emitted. */
static bool vpFillHalf(stream_sample_t *dst)
{
    for (unsigned i = 0; i < STREAM_HALF; i++) {
        while (vpNib >= vpNibbles) {    /* current clip exhausted */
            vpPos++;
            if (!vpLoadCurrentClip()) { /* sequence done -> pad rest */
                for (; i < STREAM_HALF; i++)
                    dst[i] = 0;
                return false;
            }
        }
        if (vpClip == NULL) { /* silence clip */
            dst[i] = 0;
            vpNib++;
            continue;
        }
        uint8_t byte = vpClip[vpNib >> 1];
        uint8_t code = (vpNib & 1) ? (byte >> 4) : (byte & 0xf);
        dst[i] = adpcm_ima_decode(&vpIma, code);
        vpNib++;
    }
    return true;
}

static void vpFinish()
{
    if (vpStream >= 0)
        audioStream_stop(vpStream);
    vpStream = -1;
    voicePromptActive = false;
    disableSpkOutput();
}

/* ---- public vp_* API ---- */

void vp_init()
{
    if (&_vpadata_start == &_vpadata_end)
        return;

    vpData = &_vpadata_start;
    uint32_t magic = ((const uint32_t *)vpData)[0];
    vpCount = ((const uint32_t *)vpData)[1];
    if (magic != VPA_MAGIC || vpCount == 0 || vpCount > 4096) {
        vpDataLoaded = false;
    } else {
        vpToc = (const uint32_t *)(vpData + 8);
        vpClips = vpData + 8 + (vpCount + 1) * 4;
        vpDataLoaded = true;
    }

    if (vpDataLoaded) {
        if ((kbd_getKeys() & KEY_HASH) && (state.settings.vpLevel <= vpBeep))
            state.settings.vpLevel = vpHigh;
    } else if (state.settings.vpLevel > vpBeep) {
        state.settings.vpLevel = vpBeep; /* beeps only if no data */
    }
}

void vp_terminate()
{
    if (voicePromptActive)
        vp_flush();
}

void vp_stop()
{
    if (voicePromptActive)
        vpFinish();
    vpPos = 0;
    vpNib = 0;
    vpNibbles = 0;
    beep_flush();
}

void vp_flush()
{
    vp_stop();
    vpSeqLength = 0;
}

bool vp_isPlaying()
{
    return voicePromptActive;
}

bool vp_sequenceNotEmpty()
{
    return (vpSeqLength > 0);
}

void vp_queuePrompt(const uint16_t prompt)
{
    if (state.settings.vpLevel < vpLow)
        return;
    if (voicePromptActive)
        vp_flush();
    if (vpSeqLength < VP_SEQUENCE_BUF_SIZE)
        vpSequence[vpSeqLength++] = prompt;
}

void vp_queueString(const char *string, enum vpFlags flags)
{
    if (state.settings.vpLevel < vpLow)
        return;
    if (voicePromptActive)
        vp_flush();
    if (state.settings.vpPhoneticSpell)
        flags |= vpAnnouncePhoneticRendering;

    while (*string != '\0') {
        int advanceBy = 0;
        enum voicePrompt vp = userDictLookup(string, &advanceBy);
        if (vp != 0) {
            vp_queuePrompt(vp);
            string += advanceBy;
            continue;
        } else if ((*string >= '0') && (*string <= '9')) {
            vp_queuePrompt(*string - '0' + PROMPT_0);
        } else if ((*string >= 'A') && (*string <= 'Z')) {
            if (flags & vpAnnounceCaps)
                vp_queuePrompt(PROMPT_CAP);
            if (flags & vpAnnouncePhoneticRendering)
                vp_queuePrompt((*string - 'A') + PROMPT_A_PHONETIC);
            else
                vp_queuePrompt(*string - 'A' + PROMPT_A);
        } else if ((*string >= 'a') && (*string <= 'z')) {
            if (flags & vpAnnouncePhoneticRendering)
                vp_queuePrompt((*string - 'a') + PROMPT_A_PHONETIC);
            else
                vp_queuePrompt(*string - 'a' + PROMPT_A);
        } else if ((*string == ' ') && (flags & vpAnnounceSpace)) {
            vp_queuePrompt(PROMPT_SPACE);
        } else if (GetSymbolVPIfItShouldBeAnnounced(*string, flags, &vp)) {
            if (vp != PROMPT_SILENCE)
                vp_queuePrompt(vp);
            else {
                vp_queuePrompt(PROMPT_CHARACTER);
                vp_queueInteger((int)*string);
            }
        } else {
            vp_queuePrompt(PROMPT_SILENCE);
        }
        string++;
    }

    if (flags & vpqAddSeparatingSilence)
        vp_queuePrompt(PROMPT_SILENCE);
}

void vp_queueInteger(const int value)
{
    if (state.settings.vpLevel < vpLow)
        return;
    char buf[12] = { 0 };
    if (value < 0)
        vp_queuePrompt(PROMPT_MINUS);
    sniprintf(buf, 12, "%d", value);
    vp_queueString(buf, 0);
}

void vp_queueStringTableEntry(const char *const *stringTableStringPtr)
{
    if (state.settings.vpLevel < vpLow)
        return;
    if (stringTableStringPtr == NULL)
        return;
    uint16_t pos = NUM_VOICE_PROMPTS
                 + (stringTableStringPtr - &currentLanguage->languageName);
    vp_queuePrompt(pos);
}

void vp_play()
{
    if (state.settings.vpLevel < vpLow)
        return;
    if (voicePromptActive)
        return;
    if (vpSeqLength == 0)
        return;
    if (!vpDataLoaded)
        return;

    /* A queued prompt preempts any in-progress beep -- both use vpStream, and
     * at voice-prompt level the spoken prompt replaces the keytone. */
    if (currentBeepDuration != 0)
        beep_flush();

    enableSpkOutput();
    if (audioPath_getStatus(vpAudioPath) != PATH_OPEN)
        return;

    memset(vpStreamBuf, 0, sizeof(vpStreamBuf));
    vpStream = audioStream_start(vpAudioPath, vpStreamBuf, STREAM_SAMPLES, 8000,
                                 STREAM_OUTPUT | BUF_CIRC_DOUBLE);
    if (vpStream < 0) {
        vpStream = -1;
        disableSpkOutput();
        return;
    }

    vpPos = 0;
    vpLeadin = (LEADIN_SILENCE_MS / 20u) + 1u; /* mask amp turn-on pop */
    vpLoadCurrentClip();
    voicePromptActive = true;
}

void vp_tick()
{
    if (platform_getPttStatus()
        && (voicePromptActive || (currentBeepDuration > 0))) {
        vp_stop();
        return;
    }

    if (beep_tick())
        return;

    if (!voicePromptActive)
        return;

    /* Path closed/suspended out from under us -> abandon. */
    if (audioPath_getStatus(vpAudioPath) != PATH_OPEN) {
        vpFinish();
        return;
    }

    /* One half per tick, paced by the stream syncpoint (<= 20 ms block). */
    if (outputStream_sync(vpStream, false) == false) {
        vpFinish();
        return;
    }
    stream_sample_t *idle = outputStream_getIdleBuffer(vpStream);
    if (idle == NULL) {
        vpFinish();
        return;
    }

    if (vpLeadin > 0) {
        memset(idle, 0, STREAM_HALF * sizeof(stream_sample_t));
        vpLeadin--;
        return;
    }

    if (vpFillHalf(idle) == false) {
        /* Last half emitted (with silent tail); let it drain, then finish. */
        outputStream_sync(vpStream, false);
        vpFinish();
    }
}

void vp_beep(uint16_t freq, uint16_t duration)
{
    if (state.settings.vpLevel < vpBeep)
        return;
    if (voicePromptActive) /* a prompt owns vpStream */
        return;
    if (currentBeepDuration != 0)
        return;
    if (duration > 20)
        duration = 20;
    if (duration == 0)
        return;
    beepSeriesBuffer[0].freq = freq;
    beepSeriesBuffer[0].duration = duration;
    beepSeriesBuffer[1].freq = 0;
    beepSeriesBuffer[1].duration = 0;
    beepSeriesIndex = 0;
    beepFreq = freq;
    beepPhase = 0;
    if (!beep_stream_open())        /* open path + tone stream */
        return;
    currentBeepDuration = duration; /* set last: arms beep_tick */
}

void vp_beepSeries(const uint16_t *beepSeries)
{
    if (state.settings.vpLevel < vpBeep)
        return;
    if (voicePromptActive)
        return;
    if (currentBeepDuration != 0)
        return;
    if (beepSeries == NULL)
        return;
    memcpy(beepSeriesBuffer, beepSeries,
           BEEP_SEQ_BUF_SIZE * sizeof(beepData_t));
    beepSeriesBuffer[BEEP_SEQ_BUF_SIZE - 1].freq = 0;
    beepSeriesBuffer[BEEP_SEQ_BUF_SIZE - 1].duration = 0;
    if (beepSeriesBuffer[0].duration == 0)
        return;
    beepSeriesIndex = 0;
    beepFreq = beepSeriesBuffer[0].freq;
    beepPhase = 0;
    if (!beep_stream_open())
        return;
    currentBeepDuration = beepSeriesBuffer[0].duration;
}
