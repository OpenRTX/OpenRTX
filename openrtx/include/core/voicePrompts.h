/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VOICEPROMPTS_H
#define VOICEPROMPTS_H

#include "core/datatypes.h"
#include <stdbool.h>

/**
 * List of voice prompts for spoken words or phrases which are not in the UI
 * string table. The voice prompt data file stores these first, then after the
 * data for these prompts, the data for the indexed string table phrases.
 *
 * WARNING: this enum must match the order of prompts defined in the
 * wordlist.csv file in the voicePrompts generator project.
 */
typedef enum
{
    PROMPT_SILENCE,     //
    PROMPT_0,           // 0
    PROMPT_1,           // 1
    PROMPT_2,           // 2
    PROMPT_3,           // 3
    PROMPT_4,           // 4
    PROMPT_5,           // 5
    PROMPT_6,           // 6
    PROMPT_7,           // 7
    PROMPT_8,           // 8
    PROMPT_9,           // 9
    PROMPT_A,           // A
    PROMPT_B,           // B
    PROMPT_C,           // C
    PROMPT_D,           // D
    PROMPT_E,           // E
    PROMPT_F,           // F
    PROMPT_G,           // G
    PROMPT_H,           // H
    PROMPT_I,           // I
    PROMPT_J,           // J
    PROMPT_K,           // K
    PROMPT_L,           // L
    PROMPT_M,           // M
    PROMPT_N,           // N
    PROMPT_O,           // O
    PROMPT_P,           // P
    PROMPT_Q,           // Q
    PROMPT_R,           // R
    PROMPT_S,           // S
    PROMPT_T,           // T
    PROMPT_U,           // U
    PROMPT_V,           // V
    PROMPT_W,           // W
    PROMPT_X,           // X
    PROMPT_Y,           // Y
    PROMPT_Z,           // Zed
    PROMPT_A_PHONETIC,  // alpha
    PROMPT_B_PHONETIC,  // bravo
    PROMPT_C_PHONETIC,  // charlie
    PROMPT_D_PHONETIC,  // delta
    PROMPT_E_PHONETIC,  // echo
    PROMPT_F_PHONETIC,  // foxtrot
    PROMPT_G_PHONETIC,  // golf
    PROMPT_H_PHONETIC,  // hotel
    PROMPT_I_PHONETIC,  // india
    PROMPT_J_PHONETIC,  // juliet
    PROMPT_K_PHONETIC,  // kilo
    PROMPT_L_PHONETIC,  // lema
    PROMPT_M_PHONETIC,  // mike
    PROMPT_N_PHONETIC,  // november
    PROMPT_O_PHONETIC,  // oscar
    PROMPT_P_PHONETIC,  // papa
    PROMPT_Q_PHONETIC,  // quebec
    PROMPT_R_PHONETIC,  // romeo
    PROMPT_S_PHONETIC,  // siera
    PROMPT_T_PHONETIC,  // tango
    PROMPT_U_PHONETIC,  // uniform
    PROMPT_V_PHONETIC,  // victor
    PROMPT_W_PHONETIC,  // whisky
    PROMPT_X_PHONETIC,  // exray
    PROMPT_Y_PHONETIC,  // yankie
    PROMPT_Z_PHONETIC,  // zulu
    PROMPT_CAP,         // cap
    PROMPT_HERTZ,       // hertz
    PROMPT_KILOHERTZ,   // Kilohertz
    PROMPT_MEGAHERTZ,   // Megahertz
    PROMPT_CHANNEL,
    PROMPT_VFO,             // V F O
    PROMPT_MILLISECONDS,    // Milliseconds
    PROMPT_SECONDS,         // Seconds
    PROMPT_MINUTES,         // Minutes
    PROMPT_VOLTS,           // Volts
    PROMPT_MILLIWATTS,      // Milliwatts
    PROMPT_WATT,            // Wattt
    PROMPT_WATTS,           // Watts
    PROMPT_RECEIVE,         // Receive
    PROMPT_TRANSMIT,        // Transmit
    PROMPT_MODE,            // Mode
    PROMPT_BANDWIDTH,       // bandwidth
    PROMPT_POWER,           // power
    PROMPT_SQUELCH,         // squelch
    PROMPT_SOURCE_ID,       // Source ID
    PROMPT_DEST_ID,         // Destination ID
    PROMPT_DMR_ID,          // DMR ID
    PROMPT_TALKGROUP,       // Talk group
    PROMPT_TIMESLOT,        // timeslot
    PROMPT_COLORCODE,       // color code
    PROMPT_TONE,            // tone
    PROMPT_CONTACT,         // contact
    PROMPT_NORTH,           // north
    PROMPT_SOUTH,           // south
    PROMPT_EAST,            // east
    PROMPT_WEST,            // west
    PROMPT_LATITUDE,        // latitude
    PROMPT_LONGITUDE,       // longitude
    PROMPT_SPEED,           // speed
    PROMPT_ALTITUDE,        // altitude
    PROMPT_SATELLITES,      // satellites
    PROMPT_COMPASS,         // compass
    PROMPT_DEGREES,         // degrees
    PROMPT_VP_UNAVAILABLE,  // Voice prompts will be unavailable during this
                            // operation.
    PROMPT_CHARACTER,       // character
    PROMPT_SPACE,           // space
    PROMPT_PERCENT,         // Percent
    PROMPT_POINT,           // POINT
    PROMPT_PLUS,            // Plus
    PROMPT_MINUS,           // Minus
    PROMPT_STAR,            // Star
    PROMPT_HASH,            // Hash
    PROMPT_EXCLAIM,         // exclaim
    PROMPT_COMMA,           // comma
    PROMPT_AT,              // at
    PROMPT_COLON,           // colon
    PROMPT_QUESTION,        // question
    PROMPT_LEFT_PAREN,      // left paren
    PROMPT_RIGHT_PAREN,     // right paren
    PROMPT_TILDE,           // tilde
    PROMPT_SLASH,           // slash
    PROMPT_LEFT_BRACKET,    // left bracket
    PROMPT_RIGHT_BRACKET,   // right bracket
    PROMPT_LESS,            // less
    PROMPT_GREATER,         // greater
    PROMPT_EQUALS,          // equals
    PROMPT_DOLLAR,          // dollar
    PROMPT_APOSTROPHE,      // apostrophe
    PROMPT_GRAVE,           // grave
    PROMPT_AMPERSAND,       // and
    PROMPT_BAR,             // bar
    PROMPT_UNDERLINE,       // underline
    PROMPT_CARET,           // caret
    PROMPT_LEFT_BRACE,      // left brace
    PROMPT_RIGHT_BRACE,     // right brace
    PROMPT_EDIT,            // edit
    PROMPT_CUSTOM1,         // Hotspot
    PROMPT_CUSTOM2,         // ClearNode
    PROMPT_CUSTOM3,         // ShariNode
    PROMPT_CUSTOM4,         // MicroHub
    PROMPT_CUSTOM5,         // Openspot
    PROMPT_CUSTOM6,         // repeater
    PROMPT_CUSTOM7,         // BlindHams
    PROMPT_CUSTOM8,         // Allstar
    PROMPT_CUSTOM9,         // parrot
    PROMPT_CUSTOM10,        // unused
    NUM_VOICE_PROMPTS,
}
voicePrompt_t;

// The name of the voice prompt file is always encoded as the last prompt.
#define PROMPT_VOICE_NAME (NUM_VOICE_PROMPTS + (sizeof(stringsTable_t)/sizeof(char*)))

/**
 * Flags controlling how vp_queueString operates.
 */
typedef enum
{
    vpDefault                           = 0x00,
    vpAnnounceCaps                      = 0x01,
    vpAnnounceCustomPrompts             = 0x02,
    vpAnnounceSpace                     = 0x04,
    vpAnnounceCommonSymbols             = 0x08,
    vpAnnounceLessCommonSymbols         = 0x10,
    vpAnnounceASCIIValueForUnknownChars = 0x20,
    vpAnnouncePhoneticRendering         = 0x40,
}
vpFlags_t;

/**
 * Queuing flags determining if speech is interrupted, played immediately,
 * whether prompts are queued for values, etc.
 */
typedef enum
{
    vpqDefault                         = 0,
    vpqInit                            = 0x01,  // stop any voice prompts already in progress.
    vpqPlayImmediately                 = 0x02,  // call play after queue at all levels.
    vpqPlayImmediatelyAtMediumOrHigher = 0x04,
    vpqIncludeDescriptions             = 0x08,
    vpqAddSeparatingSilence            = 0x10
}
vpQueueFlags_t;

/**
 * Voice prompt verbosity levels.
 */
typedef enum
{
    vpNone = 0,
    vpBeep,
    vpLow,
    vpMedium,
    vpHigh
}
vpVerbosity_t;

typedef enum
{
    vpChannelNameOrVFO      = 0x01,
    vpFrequencies           = 0x02,
    vpRadioMode             = 0x04,
    vpModeSpecificInfo      = 0x08,
    vpPower                 = 0x10,
    vpBankNameOrAllChannels = 0x20,
    vpAllInfo               = 0xff
}
vpSummaryInfoFlags_t;

typedef enum
{
    vpGPSNone       = 0,
    vpGPSIntro      = 0x01,
    vpGPSFixQuality = 0x02,
    vpGPSFixType    = 0x04,
    vpGPSLatitude   = 0x08,
    vpGPSLongitude  = 0x10,
    vpGPSSpeed      = 0x20,
    vpGPSAltitude   = 0x40,
    vpGPSDirection  = 0x80,
    vpGPSSatCount   = 0x100,
    vpGPSAll        = 0x1ff,
}
vpGPSInfoFlags_t;


/**
 * Initialise the voice prompt system and load vp table of contents.
 */
void vp_init();

/**
 * Terminate the currently ongoing prompt and shutdown the voice prompt system.
 */
void vp_terminate();

/**
 * Stop an in-progress prompt and rewind data pointers to the beginning keeping
 * prompt data intact. This allows to replay the prompt.
 */
void vp_stop();

/**
 * Stop an in-progress prompt, if present, and clear the prompt data buffer.
 */
void vp_flush();

/**
 * Append an individual prompt item to the prompt queue.
 *
 * @param prompt: voice prompt ID.
 */
void vp_queuePrompt(const uint16_t prompt);

/**
 * Append the spelling of a complete string to the queue.
 *
 * @param string: string to be spelled.
 * @param flags: control flags.
 */
void vp_queueString(const char* string, vpFlags_t flags);

/**
 * Append a signed integer to the queue.
 *
 * @param value: value to be appended.
 */
void vp_queueInteger(const int value);

/**
 * Append a text string from the current language to the queue.
 */
void vp_queueStringTableEntry(const char* const* stringTableStringPtr);

/**
 * Start prompt playback.
 */
void vp_play();

/**
 * Function handling vp data decoding, to be called periodically.
 */
void vp_tick();

/**
 * Check if a voice prompt is being played.
 *
 * @return true if a voice prompt is being played.
 */
bool vp_isPlaying();

/**
 * Check if the voice prompt sequence is empty.
 *
 * @return true if the voice prompt sequence is empty.
 */
bool vp_sequenceNotEmpty();

/**
 * play a beep at a given frequency for a given duration.
 */
void vp_beep(uint16_t freq, uint16_t duration);

/**
 * Play a series of beeps at a given frequency for a given duration.
 * Array is freq, duration, ... 0, 0 to terminate series.
 */
void vp_beepSeries(const uint16_t* beepSeries);

#endif
