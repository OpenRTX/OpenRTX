/***************************************************************************
 *   Copyright (C) 2019 by Federico Amedeo Izzo IU2NUO,                    *
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
#include <ctype.h>
#include "core/voicePrompts.h"
#include "ui/UIStrings.h"
const uint32_t VOICE_PROMPTS_DATA_MAGIC = 0x5056;//'VP'
const uint32_t VOICE_PROMPTS_DATA_VERSION = 0x1000; // v1000 OpenRTX
// Must match the number of voice prompts allowed by the generator script.
#define VOICE_PROMPTS_TOC_SIZE 350
// This gets the data for a voice prompt to be demodulated using Codec2.
// The offset is relative to the start of the voice prompt data.
// The length is the length in bytes of the data. 
static void GetCodec2Data(int offset,int length);

typedef struct
{
	uint32_t magic;
	uint32_t version;
} voicePromptsDataHeader_t;
// ToDo: may be a file on flashdisk.
// ToDo figure this out for OpenRTX
// Address of voice prompt header for checking version etc.
const uint32_t VOICE_PROMPTS_FLASH_HEADER_ADDRESS 		= 0x8F400;
// Start of actual voice prompt data.
static uint32_t vpFlashDataAddress;// = VOICE_PROMPTS_FLASH_HEADER_ADDRESS + sizeof(voicePromptsDataHeader_t) + sizeof(uint32_t)*VOICE_PROMPTS_TOC_SIZE ;
// TODO figure out Codec2 frame equivalent.
// 76 x 27 byte Codec2 frames
#define Codec2DataBufferSize  2052

bool voicePromptDataIsLoaded = false;
static bool voicePromptIsActive = false;
// Uninitialized is -1.
static int promptDataPosition = -1;
static int currentPromptLength = -1;
// Number of ms from end of playing prompt to disabling amp.
#define PROMPT_TAIL  30
static int promptTail = 0;

static uint8_t Codec2Data[Codec2DataBufferSize];

#define VOICE_PROMPTS_SEQUENCE_BUFFER_SIZE 128

typedef struct
{
	uint16_t  Buffer[VOICE_PROMPTS_SEQUENCE_BUFFER_SIZE];
	int  Pos;
	int  Length;
} vpSequence_t;

static vpSequence_t vpCurrentSequence =
{
	.Pos = 0,
	.Length = 0
};

uint32_t tableOfContents[VOICE_PROMPTS_TOC_SIZE];


void vpCacheInit(void)
{
	voicePromptsDataHeader_t header;
// ToDo not sure where this is coming from yet.
	//SPI_Flash_read(VOICE_PROMPTS_FLASH_HEADER_ADDRESS,(uint8_t *)&header,sizeof(voicePromptsDataHeader_t));

	if (vpCheckHeader((uint32_t *)&header))
	{// ToDo see above 
		voicePromptDataIsLoaded = SPI_Flash_read(VOICE_PROMPTS_FLASH_HEADER_ADDRESS + sizeof(voicePromptsDataHeader_t), (uint8_t *)&tableOfContents, sizeof(uint32_t) * VOICE_PROMPTS_TOC_SIZE);
		vpFlashDataAddress =  VOICE_PROMPTS_FLASH_HEADER_ADDRESS + sizeof(voicePromptsDataHeader_t) + sizeof(uint32_t)*VOICE_PROMPTS_TOC_SIZE ;
	}

}

bool vpCheckHeader(uint32_t *bufferAddress)
{
	voicePromptsDataHeader_t *header = (voicePromptsDataHeader_t *)bufferAddress;

	return ((header->magic == VOICE_PROMPTS_DATA_MAGIC) && (header->version == VOICE_PROMPTS_DATA_VERSION));
}

static void GetCodec2Data(int offset,int length)
{
	if (length <= Codec2DataBufferSize)
	{// ToDo where are we reading this from?
		SPI_Flash_read(vpFlashDataAddress + offset, (uint8_t *)&Codec2Data, length);
	}
}

void vpTick(void)
{
	if (voicePromptIsActive)
	{
		if (promptDataPosition < currentPromptLength)
		{// ToDo figure out buffering.
			//if (wavbuffer_count <= (WAV_BUFFER_COUNT / 2))
			{
//				codecDecode((uint8_t *)&Codec2Data[promptDataPosition], 3);
				promptDataPosition += 27;
			}

			//soundTickRXBuffer();
		}
		else
		{
			if ( vpCurrentSequence.Pos < (vpCurrentSequence.Length - 1))
			{
				vpCurrentSequence.Pos++;
				promptDataPosition = 0;

				int promptNumber = vpCurrentSequence.Buffer[vpCurrentSequence.Pos];
				currentPromptLength = tableOfContents[promptNumber + 1] - tableOfContents[promptNumber];
				GetCodec2Data(tableOfContents[promptNumber], currentPromptLength);
			}
			else
			{
				// wait for wave buffer to empty when prompt has finished playing

//				if (wavbuffer_count == 0)
				{
					vpTerminate();
				}
			}
		}
	}
	else
	{
		if (promptTail > 0)
		{
			promptTail--;

			if ((promptTail == 0) && trxCarrierDetected() && (trxGetMode() == RADIO_MODE_ANALOG))
			{// ToDo disable amp.
				//GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 1); // Set the audio path to AT1846 -> audio amp.
			}
		}
	}
}

void vpTerminate(void)
{
	if (voicePromptIsActive)
	{
		//disableAudioAmp(AUDIO_AMP_MODE_PROMPT);

		vpCurrentSequence.Pos = 0;
		//soundTerminateSound();
		//soundInit();
		promptTail = PROMPT_TAIL;

		voicePromptIsActive = false;
	}
}

void vpInit(void)
{
	if (voicePromptIsActive)
	{
		vpTerminate();
	}
	
	vpCurrentSequence.Length = 0;
	vpCurrentSequence.Pos = 0;
}

void vpQueuePrompt(uint16_t prompt)
{
	if (voicePromptIsActive)
	{
		vpInit();
	}
	if (vpCurrentSequence.Length < VOICE_PROMPTS_SEQUENCE_BUFFER_SIZE)
	{
		vpCurrentSequence.Buffer[vpCurrentSequence.Length] = prompt;
		vpCurrentSequence.Length++;
	}
}

static bool GetSymbolVPIfItShouldBeAnnounced(char symbol, 
VoicePromptFlags_T flags, voicePrompt_t* vp)
{
	*vp=PROMPT_SILENCE;
	
	const char indexedSymbols[] = "%.+-*#!,@:?()~/[]<>=$'`&|_^{}"; // Must match order of symbols in voicePrompt_t enum.
	const char commonSymbols[] = "%.+-*#";
	
	bool announceCommonSymbols = (flags & vpAnnounceCommonSymbols) ? true : false;
	bool announceLessCommonSymbols=(flags & vpAnnounceLessCommonSymbols) ? true : false;
	
	char* symbolPtr = strchr(indexedSymbols, symbol);
	
	if (symbolPtr == NULL)
	{// we don't have a prompt for this character.
		return (flags&vpAnnounceASCIIValueForUnknownChars) ? true : false;
	}
	
	bool commonSymbol= strchr(commonSymbols, symbol) != NULL;
	
	*vp = PROMPT_PERCENT+(symbolPtr-indexedSymbols);
	
	return ((commonSymbol && announceCommonSymbols) || (!commonSymbol && announceLessCommonSymbols));
}

// This function spells out a string letter by letter.
void vpQueueString(char *promptString, VoicePromptFlags_T flags)
{
	if (voicePromptIsActive)
	{
		vpInit();
	}
	
	while (*promptString != 0)
	{
		voicePrompt_t vp = PROMPT_SILENCE;
		
		if ((*promptString >= '0') && (*promptString <= '9'))
		{
			vpQueuePrompt(*promptString - '0' + PROMPT_0);
		}
		else if ((*promptString >= 'A') && (*promptString <= 'Z'))
		{
			if (flags&vpAnnounceCaps)
				vpQueuePrompt(PROMPT_CAP);
			if (flags&vpAnnouncePhoneticRendering)
				vpQueuePrompt((*promptString - 'A') + PROMPT_A_PHONETIC);
			else
				vpQueuePrompt(*promptString - 'A' + PROMPT_A);
		}
		else if ((*promptString >= 'a') && (*promptString <= 'z'))
		{
			if (flags&vpAnnouncePhoneticRendering)
				vpQueuePrompt((*promptString - 'a') + PROMPT_A_PHONETIC);
			else
				vpQueuePrompt(*promptString - 'a' + PROMPT_A);
		}
		else if ((*promptString==' ') && (flags&vpAnnounceSpace))
		{
			vpQueuePrompt(PROMPT_SPACE);
		}
		else if (GetSymbolVPIfItShouldBeAnnounced(*promptString, flags, &vp))
		{
			if (vp != PROMPT_SILENCE)
				vpQueuePrompt(vp);
			else // announce ASCII
			{
				int32_t val = *promptString;
				vpQueueLanguageString(&currentLanguage->dtmf_code); // just the word "code" as we don't have character.
				vpQueueInteger(val);
			}
		}
		else
		{
			// otherwise just add silence
			vpQueuePrompt(PROMPT_SILENCE);
		}
		
		promptString++;
	}
}

void vpQueueInteger(int32_t value)
{
	char buf[12] = {0}; // min: -2147483648, max: 2147483647
	itoa(value, buf, 10);
	vpQueueString(buf, 0);
}

// This function looks up a voice prompt corresponding to a string table entry.
// These are stored in the voice data after the voice prompts with no 
// corresponding string table entry, hence the offset calculation:
// NUM_VOICE_PROMPTS + (stringTableStringPtr - currentLanguage->languageName)
void vpQueueStringTableEntry(const char * const *stringTableStringPtr)
{
	if (stringTableStringPtr == NULL)
	{
		return;
	}
	vpQueuePrompt(NUM_VOICE_PROMPTS + (stringTableStringPtr - currentLanguage->languageName));
}

void vpPlay(void)
{
	if ((voicePromptIsActive == false) && (vpCurrentSequence.Length > 0))
	{
		voicePromptIsActive = true;// Start the playback
		int promptNumber = vpCurrentSequence.Buffer[0];

		vpCurrentSequence.Pos = 0;
		
		currentPromptLength = tableOfContents[promptNumber + 1] - tableOfContents[promptNumber];
		GetCodec2Data(tableOfContents[promptNumber], currentPromptLength);
		
//		GPIO_PinWrite(GPIO_RX_audio_mux, Pin_RX_audio_mux, 0);// set the audio mux HR-C6000 -> audio amp
		//enableAudioAmp(AUDIO_AMP_MODE_PROMPT);

		//codecInit(true);
		promptDataPosition = 0;

	}
}

inline bool vpIsPlaying(void)
{
	return (voicePromptIsActive || (promptTail > 0));
}

bool vpHasDataToPlay(void)
{
	return (vpCurrentSequence.Length > 0);
}
