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
#include "core/voicePrompts.h"

#include <ctype.h>
#include <state.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <interfaces/audio.h>
#include <audio_codec.h>


#include "interfaces/keyboard.h"
#include "ui/ui_strings.h"

const uint32_t VOICE_PROMPTS_DATA_MAGIC   = 0x5056;  //'VP'
const uint32_t VOICE_PROMPTS_DATA_VERSION = 0x1000;  // v1000 OpenRTX
// Must match the number of voice prompts allowed by the generator script.
#define VOICE_PROMPTS_TOC_SIZE 350
// This gets the data for a voice prompt to be demodulated using Codec2.
// The offset is relative to the start of the voice prompt data.
// The length is the length in bytes of the data.
static void GetCodec2Data(int offset, int length);

#define CODEC2_HEADER_SIZE 7

static FILE *voice_prompt_file = NULL;

typedef struct
{
    const char* userWord;
    const voicePrompt_t vp;
} userDictEntry;

typedef struct
{
    uint32_t magic;
    uint32_t version;
} voicePromptsDataHeader_t;
// offset into voice prompt vpc file where actual codec2 data starts.
static uint32_t vpDataOffset = 0;
// Each codec2 frame is 8 bytes.
// 256 x 8 bytes
#define Codec2DataBufferSize 2048

bool vpDataIsLoaded = false;

static bool voicePromptIsActive = false;
// Uninitialized is -1.
static int promptDataPosition  = -1;
static int currentPromptLength = -1;
// Number of ms from end of playing prompt to disabling amp.

static uint8_t Codec2Data[Codec2DataBufferSize];

#define VOICE_PROMPTS_SEQUENCE_BUFFER_SIZE 128

typedef struct
{ // buffer of individual prompt indices.
    uint16_t buffer[VOICE_PROMPTS_SEQUENCE_BUFFER_SIZE];
    int pos; // index into above buffer.
    int length; // number of entries in above buffer.
    int codec2DataIndex; // index into current codec2 data
    //(buffer content sent in lots of 8 byte frames.)
    int codec2DataLength; // length of codec2 data for current prompt.
} vpSequence_t;

static vpSequence_t vpCurrentSequence = {.pos = 0, .length = 0, .codec2DataIndex = 0, .codec2DataLength = 0};

uint32_t tableOfContents[VOICE_PROMPTS_TOC_SIZE];

const userDictEntry userDictionary[] = {
    {"hotspot", PROMPT_CUSTOM1},    // Hotspot
    {"clearnode", PROMPT_CUSTOM2},  // ClearNode
    {"sharinode", PROMPT_CUSTOM3},  // ShariNode
    {"microhub", PROMPT_CUSTOM4},   // MicroHub
    {"openspot", PROMPT_CUSTOM5},   // Openspot
    {"repeater", PROMPT_CUSTOM6},   // repeater
    {"blindhams", PROMPT_CUSTOM7},  // BlindHams
    {"allstar", PROMPT_CUSTOM8},    // Allstar
    {"parrot", PROMPT_CUSTOM9},     // Parrot
    {"channel", PROMPT_CHANNEL},   {0, 0}};

int vp_open(char *vp_name)
{
    if (!vp_name)
        vp_name = "voiceprompts.vpc";
    voice_prompt_file = fopen(vp_name, "r");
    if (!voice_prompt_file)
        return -1;
    return 0;
}

void vp_close()
{
    fclose(voice_prompt_file);
}

bool vpCheckHeader(uint32_t* bufferAddress)
{
    voicePromptsDataHeader_t* header = (voicePromptsDataHeader_t*)bufferAddress;

    return ((header->magic == VOICE_PROMPTS_DATA_MAGIC) &&
            (header->version == VOICE_PROMPTS_DATA_VERSION));
}

void vp_init(void)
{
    voicePromptsDataHeader_t header;
    vpDataOffset=0;

    if (!voice_prompt_file)
        vp_open(NULL);

    if (!voice_prompt_file)
        return;

    fseek(voice_prompt_file, 0L, SEEK_SET);
    fread((void*)&header, sizeof(header), 1, voice_prompt_file);

    if (vpCheckHeader((uint32_t*)&header))
    {                            // read in the TOC.
        fread((void*)&tableOfContents, sizeof(tableOfContents), 1, voice_prompt_file);
        vpDataOffset = ftell(voice_prompt_file);
        if(vpDataOffset == (sizeof(voicePromptsDataHeader_t) + sizeof(tableOfContents)))
            vpDataIsLoaded = true;
    }
    if (vpDataIsLoaded)
    {  // if the hash key is down, set vpLevel to high, if beep or less.
        if ((kbd_getKeys() & KEY_HASH) && (state.settings.vpLevel <= vpBeep))
            state.settings.vpLevel = vpHigh;
    }
    else
    {  // ensure we at least have beeps in the event no voice prompts are
       // loaded.
        if (state.settings.vpLevel > vpBeep) state.settings.vpLevel = vpBeep;
    }
    // TODO: Move this somewhere else for compatibility with M17
    codec_init();
}



static void GetCodec2Data(int offset, int length)
{
    if (!voice_prompt_file || (vpDataOffset < (sizeof(voicePromptsDataHeader_t) + sizeof(tableOfContents))))
        return;

    if ((offset < 0) || (length > Codec2DataBufferSize))
        return;

    // Skip codec2 header
    fseek(voice_prompt_file, vpDataOffset+offset+CODEC2_HEADER_SIZE, SEEK_SET);
    fread((void*)&Codec2Data, length, 1, voice_prompt_file);
    // zero buffer from length to the next multiple of 8 to avoid garbage
    // being played back, since codec2 frames are pushed in lots of 8 bytes.
    if ((length % 8) != 0)
	{
        int bytesToZero = length % 8;
        memset(Codec2Data+length, 0, bytesToZero);
    }
}

void vp_terminate(void)
{
    if (voicePromptIsActive)
    {
        audio_disableAmp();
        codec_stop();

        vpCurrentSequence.pos = 0;

        voicePromptIsActive = false;
    }
}

void vp_clearCurrPrompt(void)
{
    vpCurrentSequence.length = 0;
    vpCurrentSequence.pos    = 0;
    vpCurrentSequence.codec2DataIndex = 0;
    vpCurrentSequence.codec2DataLength = 0;
}

void vp_queuePrompt(const uint16_t prompt)
{
    if (state.settings.vpLevel < vpLow) return;

    if (voicePromptIsActive)
    {
        vp_clearCurrPrompt();
    }
    if (vpCurrentSequence.length < VOICE_PROMPTS_SEQUENCE_BUFFER_SIZE)
    {
        vpCurrentSequence.buffer[vpCurrentSequence.length] = prompt;
        vpCurrentSequence.length++;
    }
}

static uint16_t UserDictLookup(char* ptr, int* advanceBy)
{
    if (!ptr || !*ptr) return 0;

    for (int index = 0; userDictionary[index].userWord != 0; ++index)
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

// This function spells out a string letter by letter.
void vp_queueString(char* promptString, VoicePromptFlags_T flags)
{
    if (state.settings.vpLevel < vpLow) return;

    if (voicePromptIsActive)
    {
        vp_clearCurrPrompt();
    }

    if (state.settings.vpPhoneticSpell) flags |= vpAnnouncePhoneticRendering;
    while (*promptString != 0)
    {
        int advanceBy    = 0;
        voicePrompt_t vp = UserDictLookup(promptString, &advanceBy);
        if (vp)
        {
            vp_queuePrompt(vp);
            promptString += advanceBy;
            continue;
        }
        else if ((*promptString >= '0') && (*promptString <= '9'))
        {
            vp_queuePrompt(*promptString - '0' + PROMPT_0);
        }
        else if ((*promptString >= 'A') && (*promptString <= 'Z'))
        {
            if (flags & vpAnnounceCaps) vp_queuePrompt(PROMPT_CAP);
            if (flags & vpAnnouncePhoneticRendering)
                vp_queuePrompt((*promptString - 'A') + PROMPT_A_PHONETIC);
            else
                vp_queuePrompt(*promptString - 'A' + PROMPT_A);
        }
        else if ((*promptString >= 'a') && (*promptString <= 'z'))
        {
            if (flags & vpAnnouncePhoneticRendering)
                vp_queuePrompt((*promptString - 'a') + PROMPT_A_PHONETIC);
            else
                vp_queuePrompt(*promptString - 'a' + PROMPT_A);
        }
        else if ((*promptString == ' ') && (flags & vpAnnounceSpace))
        {
            vp_queuePrompt(PROMPT_SPACE);
        }
        else if (GetSymbolVPIfItShouldBeAnnounced(*promptString, flags, &vp))
        {
            if (vp != PROMPT_SILENCE)
                vp_queuePrompt(vp);
            else  // announce ASCII
            {
                int32_t val = *promptString;
                vp_queuePrompt(PROMPT_CHARACTER);  // just the word "code" as we
                                                  // don't have character.
                vp_queueInteger(val);
            }
        }
        else
        {
            // otherwise just add silence
            vp_queuePrompt(PROMPT_SILENCE);
        }

        promptString++;
    }
    if (flags & vpqAddSeparatingSilence) vp_queuePrompt(PROMPT_SILENCE);
}

void vp_queueInteger(const int32_t value)
{
    if (state.settings.vpLevel < vpLow) return;

    char buf[12] = {0};  // min: -2147483648, max: 2147483647
    snprintf(buf, 12, "%d", value);
    vp_queueString(buf, 0);
}

// This function looks up a voice prompt corresponding to a string table entry.
// These are stored in the voice data after the voice prompts with no
// corresponding string table entry, hence the offset calculation:
// NUM_VOICE_PROMPTS + (stringTableStringPtr - currentLanguage->languageName)
void vp_queueStringTableEntry(const char* const* stringTableStringPtr)
{
    if (state.settings.vpLevel < vpLow) return;

    if (stringTableStringPtr == NULL)
    {
        return;
    }
    vp_queuePrompt(NUM_VOICE_PROMPTS + 1 +
                  (stringTableStringPtr - &currentLanguage->languageName)
                  / sizeof(const char *));
}

void vp_play(void)
{
    if (state.settings.vpLevel < vpLow) return;

    if (voicePromptIsActive) return;

    if (vpCurrentSequence.length <= 0) return;

    voicePromptIsActive = true;  // Start the playback

    codec_startDecode(SINK_SPK);

    audio_enableAmp();
}

// Call this from the main timer thread to continue voice prompt playback.
void vp_tick()
{
    if (!voicePromptIsActive) return;

    while (vpCurrentSequence.pos < vpCurrentSequence.length)
    {// get the codec2 data for the current prompt if needed.
        if (vpCurrentSequence.codec2DataLength == 0)
        { // obtain the data for the prompt.
            int promptNumber    = vpCurrentSequence.buffer[vpCurrentSequence.pos];

            vpCurrentSequence.codec2DataLength =
            tableOfContents[promptNumber + 1] - tableOfContents[promptNumber];

            GetCodec2Data(tableOfContents[promptNumber], vpCurrentSequence.codec2DataLength);

            vpCurrentSequence.codec2DataIndex = 0;
        }
        // push  the codec2 data in lots of 8 byte frames.
        while (vpCurrentSequence.codec2DataIndex < vpCurrentSequence.codec2DataLength)
        {
            if (!codec_pushFrame(Codec2Data+vpCurrentSequence.codec2DataIndex, false))
                return; // wait until there is room, perhaps next vp_tick call.
            vpCurrentSequence.codec2DataIndex += 8;
        }

        vpCurrentSequence.pos++; // ready for next prompt in sequence.
        vpCurrentSequence.codec2DataLength = 0; // flag that we need to get more data.
        vpCurrentSequence.codec2DataIndex = 0;
    }
    // see if we've finished.
    if(vpCurrentSequence.pos == vpCurrentSequence.length)
        voicePromptIsActive=false;
}

bool vp_isPlaying(void)
{
    return voicePromptIsActive;
}

bool vp_sequenceNotEmpty(void)
{
    return (vpCurrentSequence.length > 0);
}
