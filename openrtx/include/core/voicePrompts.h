/***************************************************************************
 *   Copyright (C) 2019 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Silvano Seva IU2KWO                             *
 *                         Joseph Stephen VK7JS                            *
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
#ifndef voice_prompts_h_included
#define voice_prompts_h_included
/*
Please note, these prompts represent spoken words or phrases which are not in the UI string table.
For example letters of the alphabet, digits, and descriptive words not displayed in the UI
The voice prompt data file stores these first, then after the data for these prompts, the data for the indexed string table phrases.
*/
/* Please note! this enum must match the order of prompts defined in the wordlist.csv file 
in the voicePrompts generator project. 
*/
typedef enum
{
PROMPT_SILENCE, //  
PROMPT_POINT, // POINT
PROMPT_0, // 0
PROMPT_1, // 1
PROMPT_2, // 2
PROMPT_3, // 3
PROMPT_4, // 4
PROMPT_5, // 5
PROMPT_6, // 6
PROMPT_7, // 7
PROMPT_8, // 8
PROMPT_9, // 9
PROMPT_A, // A
PROMPT_B, // B
PROMPT_C, // C
PROMPT_D, // D
PROMPT_E, // E
PROMPT_F, // F
PROMPT_G, // G
PROMPT_H, // H
PROMPT_I, // I
PROMPT_J, // J
PROMPT_K, // K
PROMPT_L, // L
PROMPT_M, // M
PROMPT_N, // N
PROMPT_O, // O
PROMPT_P, // P
PROMPT_Q, // Q
PROMPT_R, // R
PROMPT_S, // S
PROMPT_T, // T
PROMPT_U, // U
PROMPT_V, // V
PROMPT_W, // W
PROMPT_X, // X
PROMPT_Y, // Y
PROMPT_Z, // Zed
PROMPT_A_PHONETIC, // alpha
PROMPT_B_PHONETIC, // bravo
PROMPT_C_PHONETIC, // charlie
PROMPT_D_PHONETIC, // delta
PROMPT_E_PHONETIC, // echo
PROMPT_F_PHONETIC, // foxtrot
PROMPT_G_PHONETIC, // golf
PROMPT_H_PHONETIC, // hotel
PROMPT_I_PHONETIC, // india
PROMPT_J_PHONETIC, // juliet
PROMPT_K_PHONETIC, // kilo
PROMPT_L_PHONETIC, // lema
PROMPT_M_PHONETIC, // mike
PROMPT_N_PHONETIC, // november
PROMPT_O_PHONETIC, // oscar
PROMPT_P_PHONETIC, // papa
PROMPT_Q_PHONETIC, // quebec
PROMPT_R_PHONETIC, // romeo
PROMPT_S_PHONETIC, // siera
PROMPT_T_PHONETIC, // tango
PROMPT_U_PHONETIC, // uniform
PROMPT_V_PHONETIC, // victor
PROMPT_W_PHONETIC, // whisky
PROMPT_X_PHONETIC, // exray
PROMPT_Y_PHONETIC, // yankie
PROMPT_Z_PHONETIC, // zulu
PROMPT_CAP, // cap 
PROMPT_HERTZ, // hertz
PROMPT_KILOHERTZ, // Kilohertz
PROMPT_MEGAHERTZ, // Megahertz
PROMPT_VFO, // V F O
PROMPT_MILLISECONDS, // Milliseconds
PROMPT_SECONDS, // Seconds
PROMPT_MINUTES, // Minutes
PROMPT_VOLTS, // Volts
PROMPT_MILLIWATTS, // Milliwatts
PROMPT_WATT, // Wattt
PROMPT_WATTS, // Watts
PROMPT_PERCENT, // Percent
PROMPT_RECEIVE, // Receive
PROMPT_TRANSMIT, // Transmit
PROMPT_MODE, // Mode
PROMPT_DMR, // D M R
PROMPT_FM, // F M
PROMPT_M17, // M seventeen
PROMPT_PLUS, // Plus
PROMPT_MINUS, // Minus
PROMPT_STAR, // Star
PROMPT_HASH, // Hash
PROMPT_SPACE, // space
PROMPT_EXCLAIM, // exclaim
PROMPT_COMMA, // comma
PROMPT_AT, // at
PROMPT_COLON, // colon
PROMPT_QUESTION, // question
PROMPT_LEFT_PAREN, // left paren
PROMPT_RIGHT_PAREN, // right paren
PROMPT_TILDE, // tilde
PROMPT_SLASH, // slash
PROMPT_LEFT_BRACKET, // left bracket
PROMPT_RIGHT_BRACKET, // right bracket
PROMPT_LESS, // less
PROMPT_GREATER, // greater
PROMPT_EQUALS, // equals
PROMPT_DOLLAR, // dollar
PROMPT_APOSTROPHE, // apostrophe
PROMPT_GRAVE, // grave
PROMPT_AMPERSAND, // and
PROMPT_BAR, // bar
PROMPT_UNDERLINE, // underline
PROMPT_CARET, // caret
PROMPT_LEFT_BRACE, // left brace
NUM_VOICE_PROMPTS,
	__MAKE_ENUM_16BITS = INT16_MAX
} voicePrompt_t;

#define PROMPT_VOICE_NAME (NUM_VOICE_PROMPTS + (sizeof(stringsTable_t)/sizeof(char*)))
typedef enum
{
	vpAnnounceCaps=0x01,
	vpAnnounceCustomPrompts=0x02,
	vpAnnounceSpaceAndSymbols=0x04,
	vpAnnouncePhoneticRendering=0x08,
} VoicePromptFlags_T;

extern bool voicePromptDataIsLoaded;
extern const uint32_t VOICE_PROMPTS_FLASH_HEADER_ADDRESS;
// Loads just the TOC from Flash and stores in RAM for fast access.
void vpCacheInit(void);
// event driven to play a voice prompt in progress.
void vpTick(void);

void vpInit(void);// Call before building the prompt sequence
void vpAppendPrompt(uint16_t prompt);// Append an individual prompt item. This can be a single letter number or a phrase
void vpQueueString(char *promptString, VoicePromptFlags_T flags);
void vpQueueInteger(int32_t value); // Append a signed integer
void vpQueueStringTableEntry(const char * const *);//Append a text string from the current language e.g. currentLanguage->off
void vpPlay(void);// Starts prompt playback
extern bool vpIsPlaying(void);
bool vpHasDataToPlay(void);
void vpTerminate(void);
bool vpCheckHeader(uint32_t *bufferAddress);
 
#endif