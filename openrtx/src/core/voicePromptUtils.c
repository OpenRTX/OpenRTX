/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Silvano Seva IU2KWO                      *
 *                                Joseph Stephen VK7JS                     *
 *                                                                         *
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
#include <utils.h>
#include <ui/ui_default.h>
#include <beeps.h>
#include "interfaces/cps_io.h"

const uint16_t BOOT_MELODY[] = {400, 3, 600, 3, 800, 3, 0, 0};

static void clearCurrPromptIfNeeded(const VPQueueFlags_en flags)
{
    if (flags & VPQ_INIT)
        vp_flush();
}

static void playIfNeeded(const VPQueueFlags_en flags)
{
    uint8_t vpLevel = state.settings.vpLevel;

    if ((flags & VPQ_PLAY_IMMEDIATELY) ||
        ((flags & VPQ_PLAY_IMMEDIATELY_AT_MEDIUM_OR_HIGHER) && (vpLevel >= VPP_MEDIUM)))
        vp_play();
}

static void addSilenceIfNeeded(const VPQueueFlags_en flags)
{
    if ((flags & VPQ_ADD_SEPARATING_SILENCE) == 0)
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



void vp_announceChannelName(const channel_t* channel,
                            const uint16_t channelNumber,
                            const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
    {
        vp_queuePrompt(PROMPT_CHANNEL);
    }

    vp_queueInteger(channelNumber);

    // Only queue the name if it is not the same as the raw number.
    // Otherwise the radio will repeat  channel 1 channel 1 for channel 1.
    char numAsStr[16] = "\0";
    snprintf(numAsStr, 16, "Channel%d", channelNumber);

    if (strcmp(numAsStr, channel->name) != 0)
    {
        vp_queueString(channel->name, VP_ANNOUNCE_COMMON_SYMBOLS);
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

    vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
    vp_queuePrompt(PROMPT_MEGAHERTZ);
}

void vp_announceFrequencies(const freq_t rx, const freq_t tx,
                            const VPQueueFlags_en flags)
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

void vp_announceRadioMode(const uint8_t mode, const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
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

void vp_announceBandwidth(const uint8_t bandwidth, const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
    {
        vp_queuePrompt(PROMPT_BANDWIDTH);
    }

    char* bandwidths[] = {"12.5", "20", "25"};
    vp_queueString(bandwidths[bandwidth], VP_ANNOUNCE_COMMON_SYMBOLS);
    vp_queuePrompt(PROMPT_KILOHERTZ);
    playIfNeeded(flags);
}

void vp_anouncePower(const float power, const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
    {
        vp_queuePrompt(PROMPT_POWER);
    }

    char buffer[16] = "\0";
    snprintf(buffer, 16, "%1.1f", power);

    vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
    vp_queuePrompt(PROMPT_WATTS);
    playIfNeeded(flags);
}

void vp_announceChannelSummary(const channel_t* channel,
                               const uint16_t channelNumber, const uint16_t bank,
                               const VPSummaryInfoFlags_en infoFlags)
{
    if (channel == NULL)
        return;

    vp_flush();

    VPQueueFlags_en localFlags = VPQ_ADD_SEPARATING_SILENCE;

    // Force on the descriptions for level 3.
    if (state.settings.vpLevel == VPP_HIGH)
    {
        localFlags |= VPQ_INCLUDE_DESCRIPTIONS;
    }

    // If VFO mode, announce VFO.
    // channelNumber will be 0 if called from VFO mode.
    if ((infoFlags & VPSI_CHANNEL_NAME_OR_VFO) != 0)
    {
        if (channelNumber == 0)
        {
            vp_queuePrompt(PROMPT_VFO);
        }
        else
        {
            vp_announceChannelName(channel, channelNumber, localFlags);
        }
        addSilenceIfNeeded(localFlags);
    }

    if ((infoFlags & VPSI_FREQUENCIES) != 0)
        vp_announceFrequencies(channel->rx_frequency, channel->tx_frequency,
                               localFlags);

    if ((infoFlags & VPSI_RADIO_MODE) != 0)
    {
        vp_announceRadioMode(channel->mode, localFlags);
        addSilenceIfNeeded(localFlags);
    }

    if ((infoFlags & VPSI_MODE_SPECIFIC_INFO) != 0)
    {
        switch(channel->mode)
        {
            case OPMODE_FM:
            {
                vp_announceBandwidth(channel->bandwidth, localFlags);
                addSilenceIfNeeded(localFlags);

                if (channel->fm.rxToneEn || channel->fm.txToneEn)
                {
                    vp_announceCTCSS(channel->fm.rxToneEn, channel->fm.rxTone,
                                     channel->fm.txToneEn, channel->fm.txTone,
                                     localFlags);
                }
            }
                break;

            case OPMODE_M17:
                vp_announceM17Info(channel, false, localFlags);
                break;

            case OPMODE_DMR:
            {
                vp_announceContactWithIndex(channel->dmr.contact_index,
                                            localFlags);

                // Force announcement of the words timeslot and colorcode to avoid
                // ambiguity.
                vp_announceTimeslot(channel->dmr.dmr_timeslot,
                                   (localFlags | VPQ_INCLUDE_DESCRIPTIONS));
                vp_announceColorCode(channel->dmr.rxColorCode,
                                     channel->dmr.txColorCode,
                                    (localFlags | VPQ_INCLUDE_DESCRIPTIONS));
            }
                break;
        }

        addSilenceIfNeeded(localFlags);
    }

    if ((infoFlags & VPSI_POWER) != 0)
    {
        float power = dBmToWatt(channel->power);
        vp_anouncePower(power, localFlags);
        addSilenceIfNeeded(localFlags);
    }

    if (((infoFlags & VPSI_BANK_NAME_OR_ALL_CHANNELS) != 0) && (channelNumber > 0))  // i.e. not called from VFO.
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

    uint8_t flags = VP_ANNOUNCE_SPACE
                  | VP_ANNOUNCE_COMMON_SYMBOLS
                  | VP_ANNOUNCE_LESS_COMMON_SYMBOLS;

    vp_queueString(buf, flags);
    vp_play();
}

void vp_announceInputReceiveOrTransmit(const bool tx, const VPQueueFlags_en flags)
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

void vp_announceError(const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);
    vp_queueStringTableEntry(&currentLanguage->error);
    playIfNeeded(flags);
}

void vp_announceText(const char* text, const VPQueueFlags_en flags)
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
        vp_queueString(text, VP_ANNOUNCE_COMMON_SYMBOLS);

    playIfNeeded(flags);
}

void vp_announceCTCSS(const bool rxToneEnabled, const uint8_t rxTone,
                      const bool txToneEnabled, const uint8_t txTone,
                      const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if ((rxToneEnabled == false) && (txToneEnabled == false))
    {
        if (flags & VPQ_INCLUDE_DESCRIPTIONS)
            vp_queuePrompt(PROMPT_TONE);

        vp_queueStringTableEntry(&currentLanguage->off);
        playIfNeeded(flags);
        return;
    }

    char buffer[16] = "\0";

    // If the rx and tx tones are the same and both are enabled, just say Tone.
    if ((rxToneEnabled && txToneEnabled) && (rxTone == txTone))
    {
        if (flags & VPQ_INCLUDE_DESCRIPTIONS)
            vp_queuePrompt(PROMPT_TONE);

        snprintf(buffer, 16, "%3.1f", ctcss_tone[rxTone] / 10.0f);
        vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
        vp_queuePrompt(PROMPT_HERTZ);
        playIfNeeded(flags);

        return;
    }

    // Speak the individual rx and tx tones.
    if (rxToneEnabled)
    {
        if (flags & VPQ_INCLUDE_DESCRIPTIONS)
        {
            vp_queuePrompt(PROMPT_RECEIVE);
            vp_queuePrompt(PROMPT_TONE);
        }
        snprintf(buffer, 16, "%3.1f", ctcss_tone[rxTone] / 10.0f);
        vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
        vp_queuePrompt(PROMPT_HERTZ);
    }
    if (txToneEnabled)
    {
        if (flags & VPQ_INCLUDE_DESCRIPTIONS)
        {
            vp_queuePrompt(PROMPT_TRANSMIT);
            vp_queuePrompt(PROMPT_TONE);
        }

        snprintf(buffer, 16, "%3.1f", ctcss_tone[txTone] / 10.0f);
        vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
        vp_queuePrompt(PROMPT_HERTZ);
    }

    playIfNeeded(flags);
}

void vp_announceSquelch(const uint8_t squelch, const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
    {
        vp_queuePrompt(PROMPT_SQUELCH);
    }

    vp_queueInteger(squelch);
    playIfNeeded(flags);
}

void vp_announceContact(const contact_t* contact, const VPQueueFlags_en flags)
{
    if (contact == NULL)
        return;

    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
    {
        vp_queuePrompt(PROMPT_CONTACT);
    }

    if (contact->name[0] != '\0')
    {
        vp_queueString(contact->name, VP_ANNOUNCE_COMMON_SYMBOLS);
    }

    playIfNeeded(flags);
}

bool vp_announceContactWithIndex(const uint16_t index, const VPQueueFlags_en flags)
{
    if (index == 0)
        return false;

    contact_t contact;
    if (cps_readContact(&contact, index) == -1)
        return false;

    vp_announceContact(&contact, flags);

    return true;
}

void vp_announceTimeslot(const uint8_t timeslot, const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
    {
        vp_queuePrompt(PROMPT_TIMESLOT);
    }

    vp_queueInteger(timeslot);
    playIfNeeded(flags);
}

void vp_announceColorCode(const uint8_t rxColorCode, const uint8_t txColorCode,
                          const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
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

void vp_announceBank(const uint16_t bank, const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (state.bank_enabled)
    {
        bankHdr_t bank_hdr = {0};
        cps_readBankHeader(&bank_hdr, bank);
        vp_queueString(bank_hdr.name, VP_ANNOUNCE_COMMON_SYMBOLS);
    }
    else
    {
        vp_queueStringTableEntry(&currentLanguage->allChannels);
    }

    playIfNeeded(flags);
}

void vp_announceM17Info(const channel_t* channel, bool isEditing,
                        const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
    {
        vp_queuePrompt(PROMPT_DEST_ID);
    }

    if (isEditing)
    {
        vp_queuePrompt(PROMPT_EDIT);
    }
    else if (state.settings.m17_dest[0] != '\0')
    {
        vp_queueString(state.settings.m17_dest, VP_ANNOUNCE_COMMON_SYMBOLS);
    }
    else if ((channel != NULL) && (channel->m17.contact_index != 0))
    {
        if (!vp_announceContactWithIndex(channel->m17.contact_index, flags))
                    vp_queueStringTableEntry(&currentLanguage->broadcast);
    }
    else
    {
        vp_queueStringTableEntry(&currentLanguage->broadcast);
    }

    playIfNeeded(flags);
}

#ifdef GPS_PRESENT
// cardinal point plus or minus this value is still considered cardinal point.
#define margin 3

static bool IsCompassCloseEnoughToCardinalPoint()
{
    float tmg_true = state.gps_data.tmg_true;

    return (tmg_true < (0   + margin) || tmg_true > (360 - margin)) || // north
           (tmg_true > (90  - margin) && tmg_true < (90  + margin)) || // east
           (tmg_true > (180 - margin) && tmg_true < (180 + margin)) || // south
           (tmg_true > (270 - margin) && tmg_true < (270 + margin)) || // west
           (tmg_true > (45  - margin) && tmg_true < (45  + margin)) || // n.w.
           (tmg_true > (135 - margin) && tmg_true < (135 + margin)) || // s.e.
           (tmg_true > (225 - margin) && tmg_true < (225 + margin)) || // s.w.
           (tmg_true > (315 - margin) && tmg_true < (315 + margin));   // n.w.
}

void vp_announceGPSInfo(VPGPSInfoFlags_t gpsInfoFlags)
{
    vp_flush();
    VPQueueFlags_en flags = VPQ_INCLUDE_DESCRIPTIONS
                         | VPQ_ADD_SEPARATING_SILENCE;

    if (gpsInfoFlags & VPGPS_INTRO)
    {
        vp_queueStringTableEntry(&currentLanguage->gps);
        if (!state.settings.gps_enabled)
        {
            vp_queueStringTableEntry(&currentLanguage->off);
            vp_play();

            return;
        }
    }

    if (gpsInfoFlags & VPGPS_FIX_QUALITY)
    {
        switch (state.gps_data.fix_quality)
        {
            case 0:
                vp_queueStringTableEntry(&currentLanguage->noFix);
                vp_play();
                return;
            case 1:
                vp_queueString("SPS", VP_ANNOUNCE_COMMON_SYMBOLS);
                break;

            case 2:
                vp_queueString("DGPS", VP_ANNOUNCE_COMMON_SYMBOLS);
                break;

            case 3:
                vp_queueString("PPS", VP_ANNOUNCE_COMMON_SYMBOLS);
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
    }

    if (gpsInfoFlags & VPGPS_FIX_TYPE)
    {
        switch (state.gps_data.fix_type)
        {
            case 2:
                vp_queueString("2D", VP_ANNOUNCE_COMMON_SYMBOLS);
                break;

            case 3:
                vp_queueString("3D", VP_ANNOUNCE_COMMON_SYMBOLS);
                break;
        }

        addSilenceIfNeeded(flags);
    }

    char buffer[17] = "\0";

    if (gpsInfoFlags & VPGPS_DIRECTION)
    {
        vp_queuePrompt(PROMPT_COMPASS);
        if (!IsCompassCloseEnoughToCardinalPoint())
        {
            snprintf(buffer, 16, "%3.1f", state.gps_data.tmg_true);
            vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
            vp_queuePrompt(PROMPT_DEGREES);
        }

        if ((state.gps_data.tmg_true < (45  + margin)) ||
            (state.gps_data.tmg_true > (315 - margin)))
        {
            vp_queuePrompt(PROMPT_NORTH);
        }

        if ((state.gps_data.tmg_true > (45 - margin)) &&
            (state.gps_data.tmg_true < (135 + margin)))
        {
            vp_queuePrompt(PROMPT_EAST);
        }

        if ((state.gps_data.tmg_true > (135 - margin)) &&
            (state.gps_data.tmg_true < (225 + margin)))
        {
            vp_queuePrompt(PROMPT_SOUTH);
        }

        if ((state.gps_data.tmg_true > (225 - margin)) &&
            (state.gps_data.tmg_true < (315 + margin)))
        {
            vp_queuePrompt(PROMPT_WEST);
        }

        addSilenceIfNeeded(flags);
    }

    if ((gpsInfoFlags & VPGPS_SPEED) != 0)
    {
        // speed/altitude:
        snprintf(buffer, 16, "%4.1fkm/h", state.gps_data.speed);
        vp_queuePrompt(PROMPT_SPEED);
        vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS |
                               VP_ANNOUNCE_LESS_COMMON_SYMBOLS);
    }

    if ((gpsInfoFlags & VPGPS_ALTITUDE) != 0)
    {
        vp_queuePrompt(PROMPT_ALTITUDE);

        snprintf(buffer, 16, "%4.1fm", state.gps_data.altitude);
        vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
        addSilenceIfNeeded(flags);
    }

    if ((gpsInfoFlags & VPGPS_LATITUDE) != 0)
    {
        // lat/long
        snprintf(buffer, 16, "%8.6f", state.gps_data.latitude);
        removeUnnecessaryZerosFromVoicePrompts(buffer);
        vp_queuePrompt(PROMPT_LATITUDE);
        vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
        vp_queuePrompt(PROMPT_NORTH);
    }

    if ((gpsInfoFlags & VPGPS_LONGTITUDE) != 0)
    {
        float longitude         = state.gps_data.longitude;
        VoicePrompt_en direction = (longitude < 0) ? PROMPT_WEST : PROMPT_EAST;
        longitude               = (longitude < 0) ? -longitude : longitude;
        snprintf(buffer, 16, "%8.6f", longitude);
        removeUnnecessaryZerosFromVoicePrompts(buffer);

        vp_queuePrompt(PROMPT_LONGITUDE);
        vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS);
        vp_queuePrompt(direction);
        addSilenceIfNeeded(flags);
    }

    if ((gpsInfoFlags & VPGPS_SAT_COUNT) != 0)
    {
        vp_queuePrompt(PROMPT_SATELLITES);
        vp_queueInteger(state.gps_data.satellites_in_view);
    }

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
    vp_queueStringTableEntry(&currentLanguage->Kim);

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
    vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS |
                           VP_ANNOUNCE_LESS_COMMON_SYMBOLS);

    vp_queuePrompt(PROMPT_SILENCE);
    vp_queuePrompt(PROMPT_SILENCE);

    snprintf(buffer, 16, "%02d:%02d:%02d", local_time.hour, local_time.minute,
                                           local_time.second);
    vp_queueString(buffer, VP_ANNOUNCE_COMMON_SYMBOLS |
                           VP_ANNOUNCE_LESS_COMMON_SYMBOLS);

    vp_play();
}
#endif // RTC_PRESENT

void vp_announceSettingsVoiceLevel(const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);
    switch (state.settings.vpLevel)
    {
        case VPP_NONE:
            vp_queueStringTableEntry(&currentLanguage->off);
            break;

        case VPP_BEEP:
            vp_queueStringTableEntry(&currentLanguage->beep);
            break;

        default:
            if (flags & VPQ_INCLUDE_DESCRIPTIONS)
            {
                vp_queuePrompt(PROMPT_VOICE_NAME);
                vp_queueStringTableEntry(&currentLanguage->level);
            }

            vp_queueInteger(state.settings.vpLevel-VPP_BEEP);
            break;
    }

    playIfNeeded(flags);
}

void vp_announceSettingsOnOffToggle(const char* const* stringTableStringPtr,
                                    const VPQueueFlags_en flags, bool val)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
        vp_queueStringTableEntry(stringTableStringPtr);

    vp_queueStringTableEntry(val ? &currentLanguage->on : &currentLanguage->off);

     playIfNeeded(flags);
}

void vp_announceSettingsInt(const char* const* stringTableStringPtr,
                                    const VPQueueFlags_en flags,
                                    int val)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
        vp_queueStringTableEntry(stringTableStringPtr);

    vp_queueInteger(val);

    playIfNeeded(flags);
}

void vp_announceScreen(uint8_t ui_screen)
{
    const VPSummaryInfoFlags_en infoFlags = VPSI_CHANNEL_NAME_OR_VFO
                                         | VPSI_FREQUENCIES
                                         | VPSI_RADIO_MODE;

    switch (ui_screen)
    {
        case PAGE_MAIN_VFO:
            vp_announceChannelSummary(&state.channel, 0, state.bank, infoFlags);
            break;

        case PAGE_MAIN_MEM:
            vp_announceChannelSummary(&state.channel, state.channel_index+1,
                                      state.bank, infoFlags);
            break;

        #ifdef GPS_PRESENT
        case PAGE_MENU_GPS:
            vp_announceGPSInfo(VPGPS_ALL);
            break;
        #endif

        case PAGE_MENU_BACKUP:
            vp_announceBackupScreen();
            break;

        case PAGE_MENU_RESTORE:
            vp_announceRestoreScreen();
            break;

        case PAGE_MENU_ABOUT:
            vp_announceAboutScreen();
            break;

        #ifdef RTC_PRESENT
        case PAGE_SETTINGS_TIMEDATE:
            vp_announceSettingsTimeDate();
            break;
        #endif

        case PAGE_SETTINGS_M17:
            vp_announceBuffer(&currentLanguage->callsign,
                              false, true, state.settings.callsign);
            break;
    }
}

void vp_announceBuffer(const char* const* stringTableStringPtr,
                       bool editMode, bool callsign,
                       const char* buffer)
{
    bool isPlaying = vp_isPlaying();

    vp_flush();

    if (!isPlaying)
    {
        vp_queueStringTableEntry(stringTableStringPtr);

        if (editMode)
            vp_queuePrompt(PROMPT_EDIT);
    }

    VPFlags_en flags = VP_ANNOUNCE_COMMON_SYMBOLS;
    // add edit mode flags to adjust what is spoken.
    // extra symbols not relevant when entering callsign.
    if ((editMode == true) && (callsign == false))
        flags |= VP_ANNOUNCE_LESS_COMMON_SYMBOLS
              |  VP_ANNOUNCE_SPACE
              |  VP_ANNOUNCE_ASCII_VALUE_FOR_UNKNOWN_CHARS;

    vp_queueString(buffer, flags);

    vp_play();
}

void vp_announceDisplayTimer()
{
    bool isPlaying = vp_isPlaying();

    vp_flush();

    if (isPlaying == false)
        vp_queueStringTableEntry(&currentLanguage->timer);

    uint8_t seconds = 0;
    uint8_t minutes = 0;

    switch (state.settings.display_timer)
    {
        case TIMER_OFF:
            seconds = 0;
            break;

        case TIMER_5S:
        case TIMER_10S:
        case TIMER_15S:
        case TIMER_20S:
        case TIMER_25S:
        case TIMER_30S:
            seconds = state.settings.display_timer * 5;
            break;

        case TIMER_1M:
        case TIMER_2M:
        case TIMER_3M:
        case TIMER_4M:
        case TIMER_5M:
            minutes = (state.settings.display_timer - (TIMER_1M - 1));
            break;

        case TIMER_15M:
        case TIMER_30M:
        case TIMER_45M:
            minutes = 15 * (state.settings.display_timer - (TIMER_15M - 1));
            break;

        case TIMER_1H:
            minutes = 60;
            break;
    }

    if ((seconds == 0) && (minutes == 0))
    {
        vp_queueStringTableEntry(&currentLanguage->off);
    }
    else if (seconds > 0)
    {
        vp_queueInteger(seconds);
        vp_queuePrompt(PROMPT_SECONDS);
    }
    else if (minutes > 0)
    {
        vp_queueInteger(minutes);
        vp_queuePrompt(PROMPT_MINUTES);
    }

    vp_play();
}

VPQueueFlags_en vp_getVoiceLevelQueueFlags()
{
    uint8_t vpLevel = state.settings.vpLevel;
    VPQueueFlags_en flags = VPQ_INIT
                         | VPQ_ADD_SEPARATING_SILENCE;

    switch (vpLevel)
    {
        case VPP_NONE:
        case VPP_BEEP:
            return VPQ_DEFAULT;

        case VPP_LOW:
            // Play some immediately, other things on demand.
            flags |= VPQ_PLAY_IMMEDIATELY_AT_MEDIUM_OR_HIGHER;
            break;

        case VPP_MEDIUM:
            // Play all immediately but without extra descriptions
            flags |= VPQ_PLAY_IMMEDIATELY;
            break;

        case VPP_HIGH:
            // Play immediately with descriptions unless speech is in progress.
            flags |= VPQ_PLAY_IMMEDIATELY;
            if (!vp_isPlaying())
                flags |= VPQ_INCLUDE_DESCRIPTIONS;
            break;
    }

    return flags;
}

void vp_playMenuBeepIfNeeded(bool firstItem)
{
// Since menus talk at levels above beep, there's no need to run this or you'll
// get an unwanted click.
    if (state.settings.vpLevel != VPP_BEEP)
        return;
    if (firstItem)
        vp_beep(BEEP_MENU_FIRST_ITEM, SHORT_BEEP);
    else
        vp_beep(BEEP_MENU_ITEM, SHORT_BEEP);
}

void vp_announceSplashScreen()
{
    if (state.settings.vpLevel < VPP_BEEP)
        return;

    vp_flush();

    if (state.settings.vpLevel == VPP_BEEP)
    {
        vp_beepSeries(BOOT_MELODY);
        return;
    }

    VPQueueFlags_en localFlags = VPQ_ADD_SEPARATING_SILENCE;

    // Force on the descriptions for level 3.
    if (state.settings.vpLevel == VPP_HIGH)
    {
        localFlags |= VPQ_INCLUDE_DESCRIPTIONS;
    }

    vp_queueStringTableEntry(&currentLanguage->openRTX);
    vp_queuePrompt(PROMPT_VFO);
    vp_announceFrequencies(state.channel.rx_frequency,
                           state.channel.tx_frequency,
                           localFlags);
    vp_announceRadioMode(state.channel.mode, localFlags);
    vp_play();
}

void vp_announceTimeZone(const int8_t timeZone, const VPQueueFlags_en flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & VPQ_INCLUDE_DESCRIPTIONS)
    {
        vp_queueStringTableEntry(&currentLanguage->UTCTimeZone);
    }

    int wholeHours = timeZone / 2;
    int halfHour   = timeZone % 2;

    // While vp_queueInteeger handles negative numbers, we want to explicitly
    // say the sign even when positive.
    if (timeZone > 0)
    {
        vp_queuePrompt(PROMPT_PLUS);
    }
    else if (timeZone < 0)
    {
        vp_queuePrompt(PROMPT_MINUS);
        wholeHours *= -1;
    }

    vp_queueInteger(wholeHours);

    if (halfHour != 0)
    {
        vp_queuePrompt(PROMPT_POINT);
        vp_queueInteger(5);
    }

    playIfNeeded(flags);
}
