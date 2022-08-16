/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Silvano Seva IU2KWO                             *
 *                         Joseph Stephen VK7JS                            *
 *                         Roger Clark  VK3KYY                             *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/keyboard.h>
#include <interfaces/audio.h>
#include <ui/ui_strings.h>
#include <voicePrompts.h>
#include <audio_codec.h>
#include <ctype.h>
#include <state.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t VOICE_PROMPTS_DATA_MAGIC   = 0x5056;  //'VP'
static const uint32_t VOICE_PROMPTS_DATA_VERSION = 0x1000;  // v1000 OpenRTX

#define VOICE_PROMPTS_TOC_SIZE 350
#define CODEC2_HEADER_SIZE     7
#define VP_SEQUENCE_BUF_SIZE   128
#define CODEC2_DATA_BUF_SIZE   2048


typedef struct
{
    uint32_t magic;
    uint32_t version;
}
vpHeader_t;

typedef struct
{
    const char* userWord;
    const voicePrompt_t vp;
}
userDictEntry_t;

typedef struct
{
    uint16_t buffer[VP_SEQUENCE_BUF_SIZE];  // Buffer of individual prompt indices.
    uint16_t pos;                           // Index into above buffer.
    uint16_t length;                        // Number of entries in above buffer.
    uint32_t c2DataIndex;                   // Index into current codec2 data
    uint32_t c2DataLength;                  // Length of codec2 data for current prompt.
}
vpSequence_t;


static const userDictEntry_t userDictionary[] =
{
    {"hotspot",   PROMPT_CUSTOM1},  // Hotspot
    {"clearnode", PROMPT_CUSTOM2},  // ClearNode
    {"sharinode", PROMPT_CUSTOM3},  // ShariNode
    {"microhub",  PROMPT_CUSTOM4},  // MicroHub
    {"openspot",  PROMPT_CUSTOM5},  // Openspot
    {"repeater",  PROMPT_CUSTOM6},  // repeater
    {"blindhams", PROMPT_CUSTOM7},  // BlindHams
    {"allstar",   PROMPT_CUSTOM8},  // Allstar
    {"parrot",    PROMPT_CUSTOM9},  // Parrot
    {"channel",   PROMPT_CHANNEL},  // Channel
    {0, 0}
};

static vpSequence_t vpCurrentSequence =
{
    .pos          = 0,
    .length       = 0,
    .c2DataIndex  = 0,
    .c2DataLength = 0
};

static uint8_t  codec2Data[CODEC2_DATA_BUF_SIZE];
static uint32_t tableOfContents[VOICE_PROMPTS_TOC_SIZE];
static uint32_t vpDataOffset       = 0;
static bool     vpDataLoaded       = false;
static bool     voicePromptActive  = false;
static FILE     *vpFile            = NULL;

/**
 * \internal
 * Load Codec2 data for a voice prompt.
 *
 * @param offset: offset relative to the start of the voice prompt data.
 * @param length: data length in bytes.
 */
static void loadCodec2Data(const int offset, const int length)
{
    const uint32_t minOffset = sizeof(vpHeader_t) + sizeof(tableOfContents);

    if ((vpFile == NULL) || (vpDataOffset < minOffset))
        return;

    if ((offset < 0) || (length > CODEC2_DATA_BUF_SIZE))
        return;

    // Skip codec2 header
    fseek(vpFile, vpDataOffset + offset + CODEC2_HEADER_SIZE, SEEK_SET);
    fread((void*)&codec2Data, length, 1, vpFile);

    // zero buffer from length to the next multiple of 8 to avoid garbage
    // being played back, since codec2 frames are pushed in lots of 8 bytes.
    if ((length % 8) != 0)
    {
        int bytesToZero = length % 8;
        memset(codec2Data + length, 0, bytesToZero);
    }
}

/**
 * \internal
 * Check validity of voice prompt header.
 *
 * @param header: voice prompt header to be checked.
 * @return true if header is valid
 */
static inline bool checkVpHeader(const vpHeader_t* header)
{
    return ((header->magic == VOICE_PROMPTS_DATA_MAGIC) &&
            (header->version == VOICE_PROMPTS_DATA_VERSION));
}

/**
 * \internal
 * Perform a string lookup inside user dictionary.
 *
 * @param ptr: string to be searched.
 * @param advanceBy: final offset with respect of dictionary beginning.
 * @return index of user dictionary's voice prompt.
 */
static uint16_t UserDictLookup(const char* ptr, int* advanceBy)
{
    if ((ptr == NULL) || (*ptr == '\0'))
        return 0;

    for(uint32_t index = 0; userDictionary[index].userWord != 0; index++)
    {
        int len = strlen(userDictionary[index].userWord);
        if (strncasecmp(userDictionary[index].userWord, ptr, len) == 0)
        {
            *advanceBy = len;
            return userDictionary[index].vp;
        }
    }

    return 0;
}

static bool GetSymbolVPIfItShouldBeAnnounced(char symbol,
                                             VoicePromptFlags_T flags,
                                             voicePrompt_t* vp)
{
    *vp = PROMPT_SILENCE;

    const char indexedSymbols[] =
        "%.+-*#!,@:?()~/[]<>=$'`&|_^{}";  // Must match order of symbols in
                                          // voicePrompt_t enum.
    const char commonSymbols[] = "%.+-*#";

    bool announceCommonSymbols =
        (flags & vpAnnounceCommonSymbols) ? true : false;
    bool announceLessCommonSymbols =
        (flags & vpAnnounceLessCommonSymbols) ? true : false;

    char* symbolPtr = strchr(indexedSymbols, symbol);

    if (symbolPtr == NULL)
    {  // we don't have a prompt for this character.
        return (flags & vpAnnounceASCIIValueForUnknownChars) ? true : false;
    }

    bool commonSymbol = strchr(commonSymbols, symbol) != NULL;

    *vp = PROMPT_PERCENT + (symbolPtr - indexedSymbols);

    return ((commonSymbol && announceCommonSymbols) ||
            (!commonSymbol && announceLessCommonSymbols));
}


void vp_init()
{
    vpDataOffset = 0;

    if(vpFile == NULL)
        vpFile = fopen("voiceprompts.vpc", "r");

    if(vpFile == NULL)
        return;

    // Read header
    vpHeader_t header;
    fseek(vpFile, 0L, SEEK_SET);
    fread((void*)&header, sizeof(header), 1, vpFile);

    if(checkVpHeader(&header) == true)
    {
        // Read in the TOC.
        fread((void*)&tableOfContents, sizeof(tableOfContents), 1, vpFile);
        vpDataOffset = ftell(vpFile);

        if(vpDataOffset == (sizeof(vpHeader_t) + sizeof(tableOfContents)))
            vpDataLoaded = true;
    }

    if (vpDataLoaded)
    {
        // If the hash key is down, set vpLevel to high, if beep or less.
        if ((kbd_getKeys() & KEY_HASH) && (state.settings.vpLevel <= vpBeep))
            state.settings.vpLevel = vpHigh;
    }
    else
    {
        // ensure we at least have beeps in the event no voice prompts are
        // loaded.
        if (state.settings.vpLevel > vpBeep)
            state.settings.vpLevel = vpBeep;
    }

    // TODO: Move this somewhere else for compatibility with M17
    codec_init();
}

void vp_terminate()
{
    if (voicePromptActive)
    {
        audio_disableAmp();
        codec_stop();

        vpCurrentSequence.pos = 0;
        voicePromptActive     = false;
    }

    fclose(vpFile);
}

void vp_clearCurrPrompt()
{
    vpCurrentSequence.length       = 0;
    vpCurrentSequence.pos          = 0;
    vpCurrentSequence.c2DataIndex  = 0;
    vpCurrentSequence.c2DataLength = 0;
}

void vp_queuePrompt(const uint16_t prompt)
{
    if (state.settings.vpLevel < vpLow)
        return;

    if (voicePromptActive)
        vp_clearCurrPrompt();

    if (vpCurrentSequence.length < VP_SEQUENCE_BUF_SIZE)
    {
        vpCurrentSequence.buffer[vpCurrentSequence.length] = prompt;
        vpCurrentSequence.length++;
    }
}

void vp_queueString(const char* string, VoicePromptFlags_T flags)
{
    if (state.settings.vpLevel < vpLow)
        return;

    if (voicePromptActive)
        vp_clearCurrPrompt();

    if (state.settings.vpPhoneticSpell)
        flags |= vpAnnouncePhoneticRendering;

    while (*string != '\0')
    {
        int advanceBy    = 0;
        voicePrompt_t vp = UserDictLookup(string, &advanceBy);

        if (vp != 0)
        {
            vp_queuePrompt(vp);
            string += advanceBy;
            continue;
        }
        else if ((*string >= '0') && (*string <= '9'))
        {
            vp_queuePrompt(*string - '0' + PROMPT_0);
        }
        else if ((*string >= 'A') && (*string <= 'Z'))
        {
            if (flags & vpAnnounceCaps)
                vp_queuePrompt(PROMPT_CAP);
            if (flags & vpAnnouncePhoneticRendering)
                vp_queuePrompt((*string - 'A') + PROMPT_A_PHONETIC);
            else
                vp_queuePrompt(*string - 'A' + PROMPT_A);
        }
        else if ((*string >= 'a') && (*string <= 'z'))
        {
            if (flags & vpAnnouncePhoneticRendering)
                vp_queuePrompt((*string - 'a') + PROMPT_A_PHONETIC);
            else
                vp_queuePrompt(*string - 'a' + PROMPT_A);
        }
        else if ((*string == ' ') && (flags & vpAnnounceSpace))
        {
            vp_queuePrompt(PROMPT_SPACE);
        }
        else if (GetSymbolVPIfItShouldBeAnnounced(*string, flags, &vp))
        {
            if (vp != PROMPT_SILENCE)
                vp_queuePrompt(vp);
            else
            {
                // announce ASCII
                int32_t val = *string;
                vp_queuePrompt(PROMPT_CHARACTER);
                vp_queueInteger(val);
            }
        }
        else
        {
            // otherwise just add silence
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

    char buf[12] = {0};  // min: -2147483648, max: 2147483647
    snprintf(buf, 12, "%d", value);
    vp_queueString(buf, 0);
}

void vp_queueStringTableEntry(const char* const* stringTableStringPtr)
{
    /*
     * This function looks up a voice prompt corresponding to a string table
     * entry. These are stored in the voice data after the voice prompts with no
     * corresponding string table entry, hence the offset calculation:
     * NUM_VOICE_PROMPTS + (stringTableStringPtr - currentLanguage->languageName)
     */

    if (state.settings.vpLevel < vpLow)
        return;

    if (stringTableStringPtr == NULL)
        return;

    uint16_t pos = NUM_VOICE_PROMPTS
                 + 1
                 + (stringTableStringPtr - &currentLanguage->languageName) /
                    sizeof(const char *);

    vp_queuePrompt(pos);
}

void vp_play()
{
    if (state.settings.vpLevel < vpLow)
        return;

    if (voicePromptActive)
        return;

    if (vpCurrentSequence.length <= 0)
        return;

    voicePromptActive = true;

    codec_startDecode(SINK_SPK);
    audio_enableAmp();
}

void vp_tick()
{
    if (!voicePromptActive)
        return;

    while(vpCurrentSequence.pos < vpCurrentSequence.length)
    {
        // get the codec2 data for the current prompt if needed.
        if (vpCurrentSequence.c2DataLength == 0)
        {
            // obtain the data for the prompt.
            int promptNumber = vpCurrentSequence.buffer[vpCurrentSequence.pos];

            vpCurrentSequence.c2DataLength = tableOfContents[promptNumber + 1]
                                           - tableOfContents[promptNumber];

            loadCodec2Data(tableOfContents[promptNumber],
                           vpCurrentSequence.c2DataLength);

            vpCurrentSequence.c2DataIndex = 0;
        }

        while (vpCurrentSequence.c2DataIndex < vpCurrentSequence.c2DataLength)
        {
            // push the codec2 data in lots of 8 byte frames.
            if (!codec_pushFrame(codec2Data+vpCurrentSequence.c2DataIndex, false))
                return;

            vpCurrentSequence.c2DataIndex += 8;
        }

        vpCurrentSequence.pos++;            // ready for next prompt in sequence.
        vpCurrentSequence.c2DataLength = 0; // flag that we need to get more data.
        vpCurrentSequence.c2DataIndex  = 0;
    }

    // see if we've finished.
    if(vpCurrentSequence.pos == vpCurrentSequence.length)
        voicePromptActive = false;
}

bool vp_isPlaying()
{
    return voicePromptActive;
}

bool vp_sequenceNotEmpty()
{
    return (vpCurrentSequence.length > 0);
}
