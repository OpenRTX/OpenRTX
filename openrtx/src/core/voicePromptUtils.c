/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
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

/**
 * This file contains functions for announcing radio functions using the
 * building blocks in voicePrompts.h/c.
 */

#include "core/voicePromptUtils.h"

#include <state.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interfaces/cps_io.h"

static void vp_clearCurrPromptIfNeeded(VoicePromptQueueFlags_T flags)
{
    if (flags & vpqInit) vp_clearCurrPrompt();
}

static void vp_playIfNeeded(VoicePromptQueueFlags_T flags)
{
    uint8_t vpLevel = state.settings.vpLevel;

    if ((flags & vpqPlayImmediately) ||
        ((flags & vpqPlayImmediatelyAtMediumOrHigher) && (vpLevel >= vpMedium)))
        vp_play();
}

static void addSilenceIfNeeded(VoicePromptQueueFlags_T flags)
{
    if ((flags & vpqAddSeparatingSilence) == 0) return;

    vp_queuePrompt(PROMPT_SILENCE);
    vp_queuePrompt(PROMPT_SILENCE);
}

static void removeUnnecessaryZerosFromVoicePrompts(char* str)
{
    const int NUM_DECIMAL_PLACES = 1;
    int len                      = strlen(str);
    for (int i = len; i > 2; i--)
    {
        if ((str[i - 1] != '0') || (str[i - (NUM_DECIMAL_PLACES + 1)] == '.'))
        {
            str[i] = 0;
            return;
        }
    }
}

void announceVFO()
{
    vp_clearCurrPrompt();

    vp_queuePrompt(PROMPT_VFO);

    vp_play();
}

void announceChannelName(channel_t* channel, uint16_t channelIndex,
                         VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_CHANNEL);
    }
    vp_queueInteger(channelIndex);

    // Only queue the name if it is not the same as the raw number.
    // Otherwise the radio will say channel 1 1 for channel 1.
    char numAsStr[16] = "\0";
    snprintf(numAsStr, 16, "%d", channelIndex);
    if (strcmp(numAsStr, channel->name) != 0)
        vp_queueString(channel->name, vpAnnounceCommonSymbols);

    vp_playIfNeeded(flags);
}

void vpQueueFrequency(freq_t freq)
{
    char buffer[16];
    int mhz = (freq / 1000000);
    int khz = ((freq % 1000000) / 10);

    snprintf(buffer, 16, "%d.%05d", mhz, khz);
    removeUnnecessaryZerosFromVoicePrompts(buffer);

    vp_queueString(buffer, vpAnnounceCommonSymbols);

    vp_queuePrompt(PROMPT_MEGAHERTZ);
}

void announceFrequencies(freq_t rx, freq_t tx, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);
    // If rx and tx frequencies differ, announce both, otherwise just one
    if (rx == tx)
        vpQueueFrequency(rx);
    else
    {
        vp_queuePrompt(PROMPT_RECEIVE);
        vpQueueFrequency(rx);
        vp_queuePrompt(PROMPT_TRANSMIT);
        vpQueueFrequency(tx);
    }
    vp_playIfNeeded(flags);
}

void announceRadioMode(uint8_t mode, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions) vp_queuePrompt(PROMPT_MODE);

    switch (mode)
    {
        case OPMODE_DMR:
            vp_queueStringTableEntry(&currentLanguage->dmr);
            break;
        case OPMODE_FM:
            vp_queueStringTableEntry(&currentLanguage->fm);
            break;
        case OPMODE_M17:
            vp_queueStringTableEntry(&currentLanguage->m17);
            break;
    }

    vp_playIfNeeded(flags);
}

void announceBandwidth(uint8_t bandwidth, VoicePromptQueueFlags_T flags)
{
    if (bandwidth > BW_25) bandwidth = BW_25;  // Should probably never happen!

    vp_clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions) vp_queuePrompt(PROMPT_BANDWIDTH);

    char* bandwidths[] = {"12.5", "20", "25"};
    vp_queueString(bandwidths[bandwidth], vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_KILOHERTZ);

    vp_playIfNeeded(flags);
}

void anouncePower(float power, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    char buffer[16] = "\0";

    if (flags & vpqIncludeDescriptions) vp_queuePrompt(PROMPT_POWER);

    snprintf(buffer, 16, "%1.1f", power);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_WATTS);

    vp_playIfNeeded(flags);
}

void announceChannelSummary(channel_t* channel, uint16_t channelIndex,
                            uint16_t bank)
{
    if (!channel) return;

    vp_clearCurrPrompt();

    VoicePromptQueueFlags_T localFlags = vpqAddSeparatingSilence;
    // Force on the descriptions for level 3.
    if (state.settings.vpLevel == vpHigh) localFlags |= vpqIncludeDescriptions;
    // If VFO mode, announce VFO.
    // channelIndex will be 0 if called from VFO mode.
    if (channelIndex == 0)
        vp_queuePrompt(PROMPT_VFO);
    else
        announceChannelName(channel, channelIndex, localFlags);
    announceFrequencies(channel->rx_frequency, channel->tx_frequency,
                        localFlags);
    announceRadioMode(channel->mode, localFlags);
    if (channel->mode == OPMODE_FM)
    {
        announceBandwidth(channel->bandwidth, localFlags);
        addSilenceIfNeeded(localFlags);

        if (channel->fm.rxToneEn || channel->fm.txToneEn)
        {
            announceCTCSS(channel->fm.rxToneEn, channel->fm.rxTone,
                          channel->fm.txToneEn, channel->fm.txTone, localFlags);
        }
    }
    else if (channel->mode == OPMODE_M17)
    {
        addSilenceIfNeeded(localFlags);
        announceM17Info(channel, localFlags);
    }
    else if (channel->mode == OPMODE_DMR)
    {
        addSilenceIfNeeded(localFlags);
        announceContactWithIndex(channel->dmr.contact_index, localFlags);
        // Force announcement of the words timeslot and colorcode to avoid
        // ambiguity.
        announceTimeslot(channel->dmr.dmr_timeslot,
                         (localFlags | vpqIncludeDescriptions));
        announceColorCode(channel->dmr.rxColorCode, channel->dmr.txColorCode,
                          (localFlags | vpqIncludeDescriptions));
    }
    addSilenceIfNeeded(localFlags);

    anouncePower(channel->power, localFlags);
    addSilenceIfNeeded(localFlags);

    if (channelIndex > 0)  // i.e. not called from VFO.
        announceBank(bank, localFlags);

    vp_play();
}
 
void AnnounceInputChar(char ch)
{
    char buf[2] = "\0";
    buf[0]      = ch;

    vp_clearCurrPrompt();

    uint8_t flags = vpAnnounceCaps | vpAnnounceSpace | vpAnnounceCommonSymbols |
                    vpAnnounceLessCommonSymbols;

    vp_queueString(buf, flags);

    vp_play();
}

void announceInputReceiveOrTransmit(bool tx, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    if (tx)
        vp_queuePrompt(PROMPT_TRANSMIT);
    else
        vp_queuePrompt(PROMPT_RECEIVE);

    vp_playIfNeeded(flags);
}

void ReplayLastPrompt()
{
    if (vp_isPlaying())
        vp_terminate();
    else
        vp_play();
}

void announceError(VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    vp_queueStringTableEntry(&currentLanguage->error);

    vp_playIfNeeded(flags);
}

void announceText(char* text, VoicePromptQueueFlags_T flags)
{
    if (!text || !*text) return;

    vp_clearCurrPromptIfNeeded(flags);
    // See if we have a prompt for this string.
    int offset = GetEnglishStringTableOffset(text);

    if (offset != -1)
        vp_queueStringTableEntry(
            (const char* const*)(&currentLanguage->languageName + offset));
    else  // Just spell it out
        vp_queueString(text, vpAnnounceCommonSymbols);

    vp_playIfNeeded(flags);
}

void announceCTCSS(bool rxToneEnabled, uint8_t rxTone, bool txToneEnabled,
                   uint8_t txTone, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    if (!rxToneEnabled && !txToneEnabled)
    {
        vp_queuePrompt(PROMPT_TONE);
        vp_queueStringTableEntry(&currentLanguage->off);
        vp_playIfNeeded(flags);
        return;
    }

    char buffer[16] = "\0";

    // If the rx and tx tones are the same and both are enabled, just say Tone.
    if ((rxToneEnabled && txToneEnabled) && (rxTone == txTone))
    {
        vp_queuePrompt(PROMPT_TONE);
        snprintf(buffer, 16, "%3.1f", ctcss_tone[rxTone] / 10.0f);
        vp_queueString(buffer, vpqDefault);
        vp_queuePrompt(PROMPT_HERTZ);
        vp_playIfNeeded(flags);
        return;
    }
    // Speak the individual rx and tx tones.
    if (rxToneEnabled)
    {
        vp_queuePrompt(PROMPT_RECEIVE);
        vp_queuePrompt(PROMPT_TONE);
        snprintf(buffer, 16, "%3.1f", ctcss_tone[rxTone] / 10.0f);
        vp_queueString(buffer, vpqDefault);
        vp_queuePrompt(PROMPT_HERTZ);
    }
    if (txToneEnabled)
    {
        vp_queuePrompt(PROMPT_TRANSMIT);
        vp_queuePrompt(PROMPT_TONE);
        snprintf(buffer, 16, "%3.1f", ctcss_tone[txTone] / 10.0f);
        vp_queueString(buffer, vpqDefault);
        vp_queuePrompt(PROMPT_HERTZ);
    }

    vp_playIfNeeded(flags);
}

void announceBrightness(uint8_t brightness, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
        vp_queueStringTableEntry(&currentLanguage->brightness);

    vp_queueInteger(brightness);

    vp_playIfNeeded(flags);
}

void announceSquelch(uint8_t squelch, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions) vp_queuePrompt(PROMPT_SQUELCH);

    vp_queueInteger(squelch);

    vp_playIfNeeded(flags);
}

void announceContact(contact_t* contact, VoicePromptQueueFlags_T flags)
{
    if (!contact) return;

    vp_clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions) vp_queuePrompt(PROMPT_CONTACT);

    if (contact->name[0]) vp_queueString(contact->name, vpAnnounceCommonSymbols);

    vp_playIfNeeded(flags);
}

void announceContactWithIndex(uint16_t index, VoicePromptQueueFlags_T flags)
{
    if (index == 0) return;

    contact_t contact;

    if (cps_readContact(&contact, index) == -1) return;

    announceContact(&contact, flags);
}

void announceTimeslot(uint8_t timeslot, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions) vp_queuePrompt(PROMPT_TIMESLOT);

    vp_queueInteger(timeslot);

    vp_playIfNeeded(flags);
}

void announceColorCode(uint8_t rxColorCode, uint8_t txColorCode,
                       VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions) vp_queuePrompt(PROMPT_COLORCODE);

    if (rxColorCode == txColorCode)
    {
        vp_queueInteger(rxColorCode);
    }
    else
    {
        vp_queuePrompt(PROMPT_RECEIVE);
        vp_queueInteger(rxColorCode);
        vp_queuePrompt(PROMPT_TRANSMIT);
        vp_queueInteger(txColorCode);
    }

    vp_playIfNeeded(flags);
}

void announceBank(uint16_t bank, VoicePromptQueueFlags_T flags)
{
    vp_clearCurrPromptIfNeeded(flags);
    if (flags & vpqIncludeDescriptions)
        vp_queueStringTableEntry(&currentLanguage->banks);

    if (state.bank_enabled)
    {
        bankHdr_t bank_hdr = {0};
        cps_readBankHeader(&bank_hdr, bank);
        vp_queueString(bank_hdr.name, vpAnnounceCommonSymbols);
    }
    else
        vp_queueStringTableEntry(&currentLanguage->allChannels);

    vp_playIfNeeded(flags);
}

void announceM17Info(channel_t* channel, VoicePromptQueueFlags_T flags)
{
    if (!channel) return;

    vp_clearCurrPromptIfNeeded(flags);
    if (state.m17_data.dst_addr[0])
    {
        if (flags & vpqIncludeDescriptions) vp_queuePrompt(PROMPT_DEST_ID);
        vp_queueString(state.m17_data.dst_addr, vpAnnounceCommonSymbols);
    }
    else if (channel->m17.contact_index)
        announceContactWithIndex(channel->m17.contact_index, flags);

    vp_playIfNeeded(flags);
}

#ifdef GPS_PRESENT
void announceGPSInfo()
{
    if (!state.settings.gps_enabled) return;

    vp_clearCurrPrompt();
    VoicePromptQueueFlags_T flags =
        vpqIncludeDescriptions | vpqAddSeparatingSilence;

    vp_queueStringTableEntry(&currentLanguage->gps);

    switch (state.gps_data.fix_quality)
    {
        case 0:
            vp_queueStringTableEntry(&currentLanguage->noFix);
            break;
        case 1:
            vp_queueString("SPS", vpAnnounceCommonSymbols);
            break;
        case 2:
            vp_queueString("DGPS", vpAnnounceCommonSymbols);
            break;
        case 3:
            vp_queueString("PPS", vpAnnounceCommonSymbols);
            break;
        case 6:
            vp_queueStringTableEntry(&currentLanguage->fixLost);
            break;
        default:
            vp_queueStringTableEntry(&currentLanguage->error);

            vp_play();

            return;
    }
    addSilenceIfNeeded(flags);

    switch (state.gps_data.fix_type)
    {
        case 2:
            vp_queueString("2D", vpAnnounceCommonSymbols);
            break;
        case 3:
            vp_queueString("3D", vpAnnounceCommonSymbols);
            break;
    }
    addSilenceIfNeeded(flags);
    // lat/long
    char buffer[16] = "\0";
    vp_queuePrompt(PROMPT_LATITUDE);
    snprintf(buffer, 16, "%8.6f", state.gps_data.latitude);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_NORTH);
    float longitude         = state.gps_data.longitude;
    voicePrompt_t direction = (longitude < 0) ? PROMPT_WEST : PROMPT_EAST;
    longitude               = (longitude < 0) ? -longitude : longitude;
    snprintf(buffer, 16, "%8.6f", longitude);
    vp_queuePrompt(PROMPT_LONGITUDE);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(direction);
    addSilenceIfNeeded(flags);
    // speed/altitude:
    vp_queuePrompt(PROMPT_SPEED);
    snprintf(buffer, 16, "%4.1fkm/h", state.gps_data.speed);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_ALTITUDE);
    snprintf(buffer, 16, "%4.1fm", state.gps_data.altitude);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    addSilenceIfNeeded(flags);

    vp_queuePrompt(PROMPT_COMPASS);
    snprintf(buffer, 16, "%3.1f", state.gps_data.tmg_true);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_DEGREES);
    addSilenceIfNeeded(flags);

    vp_queuePrompt(PROMPT_SATELLITES);
    vp_queueInteger(__builtin_popcount(state.gps_data.active_sats));

    vp_play();
}
#endif // GPS_PRESENT

void announceAboutScreen()
{
    vp_clearCurrPrompt();

    vp_queueStringTableEntry(&currentLanguage->openRTX);

    vp_queueStringTableEntry(&currentLanguage->Niccolo);
    vp_queueStringTableEntry(&currentLanguage->Silvano);
    vp_queueStringTableEntry(&currentLanguage->Federico);
    vp_queueStringTableEntry(&currentLanguage->Fred);
    vp_queueStringTableEntry(&currentLanguage->Joseph);

    vp_play();
}

void announceBackupScreen()
{
    vp_clearCurrPrompt();

    vp_queueStringTableEntry(&currentLanguage->flashBackup);

    vp_queueStringTableEntry(&currentLanguage->connectToRTXTool);
    vp_queueStringTableEntry(&currentLanguage->toBackupFlashAnd);
    vp_queueStringTableEntry(&currentLanguage->pressPTTToStart);
    vp_queuePrompt(PROMPT_VP_UNAVAILABLE);

    vp_play();
}

void announceRestoreScreen()
{
    vp_clearCurrPrompt();

    vp_queueStringTableEntry(&currentLanguage->flashRestore);

    vp_queueStringTableEntry(&currentLanguage->connectToRTXTool);
    vp_queueStringTableEntry(&currentLanguage->toRestoreFlashAnd);
    vp_queueStringTableEntry(&currentLanguage->pressPTTToStart);
    vp_queuePrompt(PROMPT_VP_UNAVAILABLE);

    vp_play();
}

#ifdef RTC_PRESENT
void announceSettingsTimeDate()
{
    vp_clearCurrPrompt();

    vp_queueStringTableEntry(&currentLanguage->timeAndDate);

    datetime_t local_time = utcToLocalTime(state.time, state.settings.utc_timezone);

    char buffer[16] = "\0";
    snprintf(buffer, 16, "%02d/%02d/%02d", local_time.date, local_time.month,
             local_time.year);
    vp_queueString(buffer,
                  (vpAnnounceCommonSymbols | vpAnnounceLessCommonSymbols));

    snprintf(buffer, 16, "%02d:%02d:%02d", local_time.hour, local_time.minute,
             local_time.second);
    vp_queueString(buffer,
                  (vpAnnounceCommonSymbols | vpAnnounceLessCommonSymbols));

    vp_play();
}
#endif // RTC_PRESENT

/*
 * There are 5 levels of verbosity:
 *
 * vpNone: no voice or beeps.
 * vpBeep: beeps only.
 * vpLow: menus talk, but channel and frequency changes are indicated with a
 * beep and only voiced on demand with f1.
 * vpMedium: menus, channel and frequency changes talk but with extra
 * descriptions eliminated unless ambiguity would result. E.g. We'd say "FM"
 * rather than "Mode: FM" in the channel summary.
 * vpHigh: like vpMedium except with extra descriptions: e.g. "Mode fm".
 *
 * Also, if a voice prompt is in progress, e.g. changing a menu item, the
 * descriptions are eliminated, e.g., changing ctcss tones would not repeat
 * "ctcss" when arrowing through the options rapidly.
 */

VoicePromptQueueFlags_T GetQueueFlagsForVoiceLevel()
{
    VoicePromptQueueFlags_T flags = vpqInit | vpqAddSeparatingSilence;

    uint8_t vpLevel = state.settings.vpLevel;
    switch (vpLevel)
    {
        case vpNone:
        case vpBeep:
            return vpqDefault;
        // Play some immediately, other things on demand.
        case vpLow:
            flags |= vpqPlayImmediatelyAtMediumOrHigher;
            break;
        // Play all immediately but without extra descriptions
        case vpMedium:
        {
            flags |= vpqPlayImmediately;
            break;
        }
        // Play immediately with descriptions unless speech is in progress.
        case vpHigh:
            flags |= vpqPlayImmediately;
            if (!vp_isPlaying()) flags |= vpqIncludeDescriptions;
            break;
    }

    return flags;
}

