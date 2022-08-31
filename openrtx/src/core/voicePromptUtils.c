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

static void clearCurrPromptIfNeeded(const vpQueueFlags_t flags)
{
    if (flags & vpqInit)
        vp_flush();
}

static void playIfNeeded(const vpQueueFlags_t flags)
{
    uint8_t vpLevel = state.settings.vpLevel;

    if ((flags & vpqPlayImmediately) ||
        ((flags & vpqPlayImmediatelyAtMediumOrHigher) && (vpLevel >= vpMedium)))
        vp_play();
}

static void addSilenceIfNeeded(const vpQueueFlags_t flags)
{
    if ((flags & vpqAddSeparatingSilence) == 0)
        return;

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
            str[i] = '\0';
            return;
        }
    }
}



void vp_announceVFO()
{
    vp_flush();
    vp_queuePrompt(PROMPT_VFO);
    vp_play();
}

void vp_announceChannelName(const channel_t* channel,
                            const uint16_t channelIndex,
                            const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

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
    {
        vp_queueString(channel->name, vpAnnounceCommonSymbols);
    }

    playIfNeeded(flags);
}

void vp_queueFrequency(const freq_t freq)
{
    char buffer[16];
    int MHz = (freq / 1000000);
    int kHz = ((freq % 1000000) / 10);

    snprintf(buffer, 16, "%d.%05d", MHz, kHz);

    removeUnnecessaryZerosFromVoicePrompts(buffer);

    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_MEGAHERTZ);
}

void vp_announceFrequencies(const freq_t rx, const freq_t tx,
                            const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    // If rx and tx frequencies differ, announce both, otherwise just one
    if (rx == tx)
    {
        vp_queueFrequency(rx);
    }
    else
    {
        vp_queuePrompt(PROMPT_RECEIVE);
        vp_queueFrequency(rx);
        vp_queuePrompt(PROMPT_TRANSMIT);
        vp_queueFrequency(tx);
    }

    playIfNeeded(flags);
}

void vp_announceRadioMode(const uint8_t mode, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_MODE);
    }

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

    playIfNeeded(flags);
}

void vp_announceBandwidth(const uint8_t bandwidth, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_BANDWIDTH);
    }

    char* bandwidths[] = {"12.5", "20", "25"};
    vp_queueString(bandwidths[bandwidth], vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_KILOHERTZ);
    playIfNeeded(flags);
}

void vp_anouncePower(const float power, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_POWER);
    }

    char buffer[16] = "\0";
    snprintf(buffer, 16, "%1.1f", power);

    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_WATTS);
    playIfNeeded(flags);
}

void vp_announceChannelSummary(const channel_t* channel,
                               const uint16_t channelIndex, const uint16_t bank)
{
    if (channel == NULL)
        return;

    vp_flush();

    vpQueueFlags_t localFlags = vpqAddSeparatingSilence;

    // Force on the descriptions for level 3.
    if (state.settings.vpLevel == vpHigh)
    {
        localFlags |= vpqIncludeDescriptions;
    }

    // If VFO mode, announce VFO.
    // channelIndex will be 0 if called from VFO mode.
    if (channelIndex == 0)
    {
        vp_queuePrompt(PROMPT_VFO);
    }
    else
    {
        vp_announceChannelName(channel, channelIndex, localFlags);
    }

    vp_announceFrequencies(channel->rx_frequency, channel->tx_frequency,
                        localFlags);
    vp_announceRadioMode(channel->mode, localFlags);

    if (channel->mode == OPMODE_FM)
    {
        vp_announceBandwidth(channel->bandwidth, localFlags);
        addSilenceIfNeeded(localFlags);

        if (channel->fm.rxToneEn || channel->fm.txToneEn)
        {
            vp_announceCTCSS(channel->fm.rxToneEn, channel->fm.rxTone,
                             channel->fm.txToneEn, channel->fm.txTone, localFlags);
        }
    }
    else if (channel->mode == OPMODE_M17)
    {
        addSilenceIfNeeded(localFlags);
        vp_announceM17Info(channel, localFlags);
    }
    else if (channel->mode == OPMODE_DMR)
    {
        addSilenceIfNeeded(localFlags);
        vp_announceContactWithIndex(channel->dmr.contact_index, localFlags);

        // Force announcement of the words timeslot and colorcode to avoid
        // ambiguity.
        vp_announceTimeslot(channel->dmr.dmr_timeslot,
                         (localFlags | vpqIncludeDescriptions));
        vp_announceColorCode(channel->dmr.rxColorCode, channel->dmr.txColorCode,
                          (localFlags | vpqIncludeDescriptions));
    }

    addSilenceIfNeeded(localFlags);
    vp_anouncePower(channel->power, localFlags);
    addSilenceIfNeeded(localFlags);

    if (channelIndex > 0)  // i.e. not called from VFO.
    {
        vp_announceBank(bank, localFlags);
    }

    vp_play();
}

void vp_announceInputChar(const char ch)
{
    char buf[2] = "\0";
    buf[0]      = ch;

    vp_flush();

    uint8_t flags = vpAnnounceCaps
                  | vpAnnounceSpace
                  | vpAnnounceCommonSymbols
                  | vpAnnounceLessCommonSymbols;

    vp_queueString(buf, flags);
    vp_play();
}

void vp_announceInputReceiveOrTransmit(const bool tx, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (tx)
        vp_queuePrompt(PROMPT_TRANSMIT);
    else
        vp_queuePrompt(PROMPT_RECEIVE);

    playIfNeeded(flags);
}

void vp_replayLastPrompt()
{
    if (vp_isPlaying())
        vp_stop();
    else
        vp_play();
}

void vp_announceError(const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);
    vp_queueStringTableEntry(&currentLanguage->error);
    playIfNeeded(flags);
}

void vp_announceText(const char* text, const vpQueueFlags_t flags)
{
    if ((text == NULL) || (*text == '\0'))
        return;

    clearCurrPromptIfNeeded(flags);

    // See if we have a prompt for this string.
    int offset = GetEnglishStringTableOffset(text);

    if (offset != -1)
        vp_queueStringTableEntry(
            (const char* const*)(&currentLanguage->languageName + offset));
    else  // Just spell it out
        vp_queueString(text, vpAnnounceCommonSymbols);

    playIfNeeded(flags);
}

void vp_announceCTCSS(const bool rxToneEnabled, const uint8_t rxTone,
                      const bool txToneEnabled, const uint8_t txTone,
                      const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if ((rxToneEnabled == false) && (txToneEnabled == false))
    {
        vp_queuePrompt(PROMPT_TONE);
        vp_queueStringTableEntry(&currentLanguage->off);
        playIfNeeded(flags);
        return;
    }

    char buffer[16] = "\0";

    // If the rx and tx tones are the same and both are enabled, just say Tone.
    if ((rxToneEnabled && txToneEnabled) && (rxTone == txTone))
    {
        vp_queuePrompt(PROMPT_TONE);

        snprintf(buffer, 16, "%3.1f", ctcss_tone[rxTone] / 10.0f);
        vp_queueString(buffer, vpAnnounceCommonSymbols);
        vp_queuePrompt(PROMPT_HERTZ);
        playIfNeeded(flags);

        return;
    }

    // Speak the individual rx and tx tones.
    if (rxToneEnabled)
    {
        vp_queuePrompt(PROMPT_RECEIVE);
        vp_queuePrompt(PROMPT_TONE);

        snprintf(buffer, 16, "%3.1f", ctcss_tone[rxTone] / 10.0f);
        vp_queueString(buffer, vpAnnounceCommonSymbols);
        vp_queuePrompt(PROMPT_HERTZ);
    }
    if (txToneEnabled)
    {
        vp_queuePrompt(PROMPT_TRANSMIT);
        vp_queuePrompt(PROMPT_TONE);

        snprintf(buffer, 16, "%3.1f", ctcss_tone[txTone] / 10.0f);
        vp_queueString(buffer, vpAnnounceCommonSymbols);
        vp_queuePrompt(PROMPT_HERTZ);
    }

    playIfNeeded(flags);
}

void vp_announceBrightness(const uint8_t brightness, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queueStringTableEntry(&currentLanguage->brightness);
    }

    vp_queueInteger(brightness);
    playIfNeeded(flags);
}

void vp_announceSquelch(const uint8_t squelch, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_SQUELCH);
    }

    vp_queueInteger(squelch);
    playIfNeeded(flags);
}

void vp_announceContact(const contact_t* contact, const vpQueueFlags_t flags)
{
    if (contact == NULL)
        return;

    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_CONTACT);
    }

    if (contact->name[0] != '\0')
    {
        vp_queueString(contact->name, vpAnnounceCommonSymbols);
    }

    playIfNeeded(flags);
}

void vp_announceContactWithIndex(const uint16_t index, const vpQueueFlags_t flags)
{
    if (index == 0)
        return;

    contact_t contact;
    if (cps_readContact(&contact, index) == -1)
        return;

    vp_announceContact(&contact, flags);
}

void vp_announceTimeslot(const uint8_t timeslot, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_TIMESLOT);
    }

    vp_queueInteger(timeslot);
    playIfNeeded(flags);
}

void vp_announceColorCode(const uint8_t rxColorCode, const uint8_t txColorCode,
                          const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_COLORCODE);
    }

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

    playIfNeeded(flags);
}

void vp_announceBank(const uint16_t bank, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queueStringTableEntry(&currentLanguage->banks);
    }

    if (state.bank_enabled)
    {
        bankHdr_t bank_hdr = {0};
        cps_readBankHeader(&bank_hdr, bank);
        vp_queueString(bank_hdr.name, vpAnnounceCommonSymbols);
    }
    else
    {
        vp_queueStringTableEntry(&currentLanguage->allChannels);
    }

    playIfNeeded(flags);
}

void vp_announceM17Info(const channel_t* channel, const vpQueueFlags_t flags)
{
    if (channel == NULL)
        return;

    clearCurrPromptIfNeeded(flags);

    if (state.m17_data.dst_addr[0] != '\0')
    {
        if (flags & vpqIncludeDescriptions)
        {
            vp_queuePrompt(PROMPT_DEST_ID);
        }

        vp_queueString(state.m17_data.dst_addr, vpAnnounceCommonSymbols);
    }
    else if (channel->m17.contact_index != 0)
    {
        vp_announceContactWithIndex(channel->m17.contact_index, flags);
    }

    playIfNeeded(flags);
}

#ifdef GPS_PRESENT
void vp_announceGPSInfo()
{
    if (!state.settings.gps_enabled)
        return;

    vp_flush();
    vpQueueFlags_t flags = vpqIncludeDescriptions
                         | vpqAddSeparatingSilence;

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
    snprintf(buffer, 16, "%8.6f", state.gps_data.latitude);
    vp_queuePrompt(PROMPT_LATITUDE);
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
    snprintf(buffer, 16, "%4.1fkm/h", state.gps_data.speed);
    vp_queuePrompt(PROMPT_SPEED);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_ALTITUDE);

    snprintf(buffer, 16, "%4.1fm", state.gps_data.altitude);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    addSilenceIfNeeded(flags);

    snprintf(buffer, 16, "%3.1f", state.gps_data.tmg_true);
    vp_queuePrompt(PROMPT_COMPASS);
    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_DEGREES);
    addSilenceIfNeeded(flags);

    vp_queuePrompt(PROMPT_SATELLITES);
    vp_queueInteger(__builtin_popcount(state.gps_data.active_sats));

    vp_play();
}
#endif // GPS_PRESENT

void vp_announceAboutScreen()
{
    vp_flush();

    vp_queueStringTableEntry(&currentLanguage->openRTX);

    vp_queueStringTableEntry(&currentLanguage->Niccolo);
    vp_queueStringTableEntry(&currentLanguage->Silvano);
    vp_queueStringTableEntry(&currentLanguage->Federico);
    vp_queueStringTableEntry(&currentLanguage->Fred);
    vp_queueStringTableEntry(&currentLanguage->Joseph);

    vp_play();
}

void vp_announceBackupScreen()
{
    vp_flush();

    vp_queueStringTableEntry(&currentLanguage->flashBackup);

    vp_queueStringTableEntry(&currentLanguage->connectToRTXTool);
    vp_queueStringTableEntry(&currentLanguage->toBackupFlashAnd);
    vp_queueStringTableEntry(&currentLanguage->pressPTTToStart);
    vp_queuePrompt(PROMPT_VP_UNAVAILABLE);

    vp_play();
}

void vp_announceRestoreScreen()
{
    vp_flush();

    vp_queueStringTableEntry(&currentLanguage->flashRestore);

    vp_queueStringTableEntry(&currentLanguage->connectToRTXTool);
    vp_queueStringTableEntry(&currentLanguage->toRestoreFlashAnd);
    vp_queueStringTableEntry(&currentLanguage->pressPTTToStart);
    vp_queuePrompt(PROMPT_VP_UNAVAILABLE);

    vp_play();
}

#ifdef RTC_PRESENT
void vp_announceSettingsTimeDate()
{
    vp_flush();

    vp_queueStringTableEntry(&currentLanguage->timeAndDate);

    datetime_t local_time = utcToLocalTime(state.time,
                                           state.settings.utc_timezone);

    char buffer[16] = "\0";
    snprintf(buffer, 16, "%02d/%02d/%02d", local_time.date, local_time.month,
                                           local_time.year);
    vp_queueString(buffer, vpAnnounceCommonSymbols |
                           vpAnnounceLessCommonSymbols);

    snprintf(buffer, 16, "%02d:%02d:%02d", local_time.hour, local_time.minute,
                                           local_time.second);
    vp_queueString(buffer, vpAnnounceCommonSymbols |
                           vpAnnounceLessCommonSymbols);

    vp_play();
}
#endif // RTC_PRESENT

void vp_announceSettingsVoiceLevel(const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);
    switch (state.settings.vpLevel)
    {
        case vpNone:
            vp_queueStringTableEntry(&currentLanguage->off);
            break;

        case vpBeep:
            vp_queueStringTableEntry(&currentLanguage->beep);
            break;

        default:
            if (flags & vpqIncludeDescriptions)
            {
                vp_queuePrompt(PROMPT_VOICE_NAME);
                vp_queueStringTableEntry(&currentLanguage->level);
            }
            vp_queueInteger(state.settings.vpLevel-vpBeep);
            break;
    }

    playIfNeeded(flags);
}

void vp_announceSettingsOnOffToggle(const char* const* stringTableStringPtr,
                                    const vpQueueFlags_t flags, bool val)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
        vp_queueStringTableEntry(stringTableStringPtr);

    vp_queueStringTableEntry(val ? &currentLanguage->on : &currentLanguage->off);

     playIfNeeded(flags);
}

vpQueueFlags_t vp_getVoiceLevelQueueFlags()
{
    uint8_t vpLevel = state.settings.vpLevel;
    vpQueueFlags_t flags = vpqInit
                         | vpqAddSeparatingSilence;

    switch (vpLevel)
    {
        case vpNone:
        case vpBeep:
            return vpqDefault;

        case vpLow:
            // Play some immediately, other things on demand.
            flags |= vpqPlayImmediatelyAtMediumOrHigher;
            break;

        case vpMedium:
            // Play all immediately but without extra descriptions
            flags |= vpqPlayImmediately;
            break;

        case vpHigh:
            // Play immediately with descriptions unless speech is in progress.
            flags |= vpqPlayImmediately;
            if (!vp_isPlaying())
                flags |= vpqIncludeDescriptions;
            break;
    }

    return flags;
}

