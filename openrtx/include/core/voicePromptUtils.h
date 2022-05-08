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
 // This file contains functions for announcing radio functions using the building blocks in voicePrompts.h/c.
 #ifndef VOICE_PROMPT_UTILS_H_INCLUDED
  #define VOICE_PROMPT_UTILS_H_INCLUDED
 
 #include "core/voicePrompts.h"
#include "ui/UIStrings.h"
#include "cps.h"
 
 void announceChannelName(channel_t* channel, uint16_t channelIndex, VoicePromptQueueFlags_T flags);
 void announceFrequencies(freq_t rx, freq_t tx, VoicePromptQueueFlags_T flags);
void announceRadioMode(uint8_t mode, VoicePromptQueueFlags_T flags);
void vpAnnounceChannelSummary(channel_t* channel, VoicePromptQueueFlags_T flags);
 
#endif //VOICE_PROMPT_UTILS_H_INCLUDED