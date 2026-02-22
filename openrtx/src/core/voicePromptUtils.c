/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * This file contains functions for announcing radio functions using the
 * building blocks in voicePrompts.h/c.
 */

#include "core/voicePromptUtils.h"

#include "core/state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/utils.h"
#include <inttypes.h>
#include "ui/ui_default.h"
#include "core/beeps.h"
#include "interfaces/cps_io.h"

const uint16_t BOOT_MELODY[] = {400, 3, 600, 3, 800, 3, 0, 0};

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



void vp_announceChannelName(const channel_t* channel,
                            const uint16_t channelNumber,
                            const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_CHANNEL);
    }

    vp_queueInteger(channelNumber);

    // Only queue the name if it is not the same as the raw number.
    // Otherwise the radio will repeat  channel 1 channel 1 for channel 1.
    char numAsStr[16] = "\0";
    sniprintf(numAsStr, 16, "Channel%d", channelNumber);

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

    sniprintf(buffer, 16, "%d.%05d", MHz, kHz);

    stripTrailingZeroes(buffer);

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

    char* bandwidths[] = {"12.5", "25"};
    vp_queueString(bandwidths[bandwidth], vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_KILOHERTZ);
    playIfNeeded(flags);
}

void vp_announcePower(const uint32_t power, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_POWER);
    }

    // Compute x.y format avoiding to pull in floating point math.
    // Remember that power is expressed in mW!
    char buffer[16] = "\0";
    sniprintf(buffer, 16, "%lu.%lu", (power / 1000lu), (power % 1000lu) / 100lu);

    vp_queueString(buffer, vpAnnounceCommonSymbols);
    vp_queuePrompt(PROMPT_WATTS);
    playIfNeeded(flags);
}

void vp_announceChannelSummary(const channel_t* channel,
                               const uint16_t channelNumber, const uint16_t bank,
                               const vpSummaryInfoFlags_t infoFlags)
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
    // channelNumber will be 0 if called from VFO mode.
    if ((infoFlags & vpChannelNameOrVFO) != 0)
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

    if ((infoFlags & vpFrequencies) != 0)
        vp_announceFrequencies(channel->rx_frequency, channel->tx_frequency,
                               localFlags);

    if ((infoFlags & vpRadioMode) != 0)
    {
        vp_announceRadioMode(channel->mode, localFlags);
        addSilenceIfNeeded(localFlags);
    }

    if ((infoFlags & vpModeSpecificInfo) != 0)
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
                                   (localFlags | vpqIncludeDescriptions));
                vp_announceColorCode(channel->dmr.rxColorCode,
                                     channel->dmr.txColorCode,
                                    (localFlags | vpqIncludeDescriptions));
            }
                break;
        }

        addSilenceIfNeeded(localFlags);
    }

    if ((infoFlags & vpPower) != 0)
    {
        vp_announcePower(channel->power, localFlags);
        addSilenceIfNeeded(localFlags);
    }

    if (((infoFlags & vpBankNameOrAllChannels) != 0) && (channelNumber > 0))  // i.e. not called from VFO.
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

    uint8_t flags = vpAnnounceSpace
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
        if (flags & vpqIncludeDescriptions)
            vp_queuePrompt(PROMPT_TONE);

        vp_queueStringTableEntry(&currentLanguage->off);
        playIfNeeded(flags);
        return;
    }

    char buffer[16] = "\0";

    // If the rx and tx tones are the same and both are enabled, just say Tone.
    if ((rxToneEnabled && txToneEnabled) && (rxTone == txTone))
    {
        if (flags & vpqIncludeDescriptions)
            vp_queuePrompt(PROMPT_TONE);

        uint16_t tone = ctcss_tone[rxTone];
        sniprintf(buffer, sizeof(buffer), "%d.%d", (tone / 10), (tone % 10));

        vp_queueString(buffer, vpAnnounceCommonSymbols);
        vp_queuePrompt(PROMPT_HERTZ);
        playIfNeeded(flags);

        return;
    }

    // Speak the individual rx and tx tones.
    if (rxToneEnabled)
    {
        if (flags & vpqIncludeDescriptions)
        {
            vp_queuePrompt(PROMPT_RECEIVE);
            vp_queuePrompt(PROMPT_TONE);
        }

        uint16_t tone = ctcss_tone[rxTone];
        sniprintf(buffer, sizeof(buffer), "%d.%d", (tone / 10), (tone % 10));

        vp_queueString(buffer, vpAnnounceCommonSymbols);
        vp_queuePrompt(PROMPT_HERTZ);
    }
    if (txToneEnabled)
    {
        if (flags & vpqIncludeDescriptions)
        {
            vp_queuePrompt(PROMPT_TRANSMIT);
            vp_queuePrompt(PROMPT_TONE);
        }

        uint16_t tone = ctcss_tone[txTone];
        sniprintf(buffer, sizeof(buffer), "%d.%d", (tone / 10), (tone % 10));

        vp_queueString(buffer, vpAnnounceCommonSymbols);
        vp_queuePrompt(PROMPT_HERTZ);
    }

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

bool vp_announceContactWithIndex(const uint16_t index, const vpQueueFlags_t flags)
{
    if (index == 0)
        return false;

    contact_t contact;
    if (cps_readContact(&contact, index) == -1)
        return false;

    vp_announceContact(&contact, flags);

    return true;
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

void vp_announceM17Info(const channel_t* channel, bool isEditing,
                        const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
    {
        vp_queuePrompt(PROMPT_DEST_ID);
    }

    if (isEditing)
    {
        vp_queuePrompt(PROMPT_EDIT);
    }
    else if (state.channel.m17_dest[0] != '\0')
    {
        vp_queueString(state.channel.m17_dest, vpAnnounceCommonSymbols);
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

#ifdef CONFIG_GPS
// cardinal point plus or minus this value is still considered cardinal point.
#define margin 3

static bool IsCompassCloseEnoughToCardinalPoint()
{
    int16_t tmg_true = state.gps_data.tmg_true;

    return (tmg_true < (0   + margin) || tmg_true > (360 - margin)) || // north
           (tmg_true > (90  - margin) && tmg_true < (90  + margin)) || // east
           (tmg_true > (180 - margin) && tmg_true < (180 + margin)) || // south
           (tmg_true > (270 - margin) && tmg_true < (270 + margin)) || // west
           (tmg_true > (45  - margin) && tmg_true < (45  + margin)) || // n.w.
           (tmg_true > (135 - margin) && tmg_true < (135 + margin)) || // s.e.
           (tmg_true > (225 - margin) && tmg_true < (225 + margin)) || // s.w.
           (tmg_true > (315 - margin) && tmg_true < (315 + margin));   // n.w.
}

void vp_announceGPSInfo(vpGPSInfoFlags_t gpsInfoFlags)
{
    vp_flush();
    vpQueueFlags_t flags = vpqIncludeDescriptions
                         | vpqAddSeparatingSilence;

    if (gpsInfoFlags & vpGPSIntro)
    {
        vp_queueStringTableEntry(&currentLanguage->gps);
        if (!state.settings.gps_enabled)
        {
            vp_queueStringTableEntry(&currentLanguage->off);
            vp_play();

            return;
        }
    }

    if (gpsInfoFlags & vpGPSFixQuality)
    {
        switch (state.gps_data.fix_quality)
        {
            case 0:
                vp_queueStringTableEntry(&currentLanguage->noFix);
                vp_play();
                return;
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
    }

    if (gpsInfoFlags & vpGPSFixType)
    {
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
    }

    char buffer[17] = "\0";

    if (gpsInfoFlags & vpGPSDirection)
    {
        vp_queuePrompt(PROMPT_COMPASS);
        if (!IsCompassCloseEnoughToCardinalPoint())
        {
            sniprintf(buffer, 16, "%d", state.gps_data.tmg_true);
            vp_queueString(buffer, vpAnnounceCommonSymbols);
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

    if ((gpsInfoFlags & vpGPSSpeed) != 0)
    {
        // speed/altitude:
        sniprintf(buffer, 16, "%dkm/h", state.gps_data.speed);
        vp_queuePrompt(PROMPT_SPEED);
        vp_queueString(buffer, vpAnnounceCommonSymbols |
                               vpAnnounceLessCommonSymbols);
    }

    if ((gpsInfoFlags & vpGPSAltitude) != 0)
    {
        vp_queuePrompt(PROMPT_ALTITUDE);

        sniprintf(buffer, 16, "%dm", state.gps_data.altitude);
        vp_queueString(buffer, vpAnnounceCommonSymbols);
        addSilenceIfNeeded(flags);
    }

    if ((gpsInfoFlags & vpGPSLatitude) != 0)
    {
        // Convert from signed longitude, to unsigned + direction
        int32_t latitude        = abs(state.gps_data.latitude);
        uint8_t latitude_int    = latitude / 1000000;
        int32_t latitude_dec    = latitude % 1000000;
        voicePrompt_t direction = (state.gps_data.latitude < 0) ? PROMPT_SOUTH : PROMPT_NORTH;
        sniprintf(buffer, 16, "%d.%06"PRId32, latitude_int, latitude_dec);
        stripTrailingZeroes(buffer);
        vp_queuePrompt(PROMPT_LATITUDE);
        vp_queueString(buffer, vpAnnounceCommonSymbols);
        vp_queuePrompt(direction);
    }

    if ((gpsInfoFlags & vpGPSLongitude) != 0)
    {
        // Convert from signed longitude, to unsigned + direction
        int32_t longitude       = abs(state.gps_data.longitude);
        uint8_t longitude_int   = longitude / 1000000;
        int32_t longitude_dec   = longitude % 1000000;
        voicePrompt_t direction = (state.gps_data.longitude < 0) ? PROMPT_WEST : PROMPT_EAST;
        sniprintf(buffer, 16, "%d.%06"PRId32, longitude_int, longitude_dec);
        stripTrailingZeroes(buffer);

        vp_queuePrompt(PROMPT_LONGITUDE);
        vp_queueString(buffer, vpAnnounceCommonSymbols);
        vp_queuePrompt(direction);
        addSilenceIfNeeded(flags);
    }

    if ((gpsInfoFlags & vpGPSSatCount) != 0)
    {
        vp_queuePrompt(PROMPT_SATELLITES);
        vp_queueInteger(state.gps_data.satellites_in_view);
    }

    vp_play();
}
#endif // CONFIG_GPS

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

#ifdef CONFIG_RTC
void vp_announceSettingsTimeDate()
{
    vp_flush();

    vp_queueStringTableEntry(&currentLanguage->timeAndDate);

    datetime_t local_time = utcToLocalTime(state.time,
                                           state.settings.utc_timezone);

    char buffer[16] = "\0";
    sniprintf(buffer, 16, "%02d/%02d/%02d", local_time.date, local_time.month,
                                           local_time.year);
    vp_queueString(buffer, vpAnnounceCommonSymbols |
                           vpAnnounceLessCommonSymbols);

    vp_queuePrompt(PROMPT_SILENCE);
    vp_queuePrompt(PROMPT_SILENCE);

    sniprintf(buffer, 16, "%02d:%02d:%02d", local_time.hour, local_time.minute,
                                           local_time.second);
    vp_queueString(buffer, vpAnnounceCommonSymbols |
                           vpAnnounceLessCommonSymbols);

    vp_play();
}
#endif // CONFIG_RTC

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

void vp_announceSettingsInt(const char* const* stringTableStringPtr,
                                    const vpQueueFlags_t flags,
                                    int val)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
        vp_queueStringTableEntry(stringTableStringPtr);

    vp_queueInteger(val);

    playIfNeeded(flags);
}

void vp_announceScreen(uint8_t ui_screen)
{
    const vpSummaryInfoFlags_t infoFlags = vpChannelNameOrVFO
                                         | vpFrequencies
                                         | vpRadioMode;

    switch (ui_screen)
    {
        case MAIN_VFO:
            vp_announceChannelSummary(&state.channel, 0, state.bank, infoFlags);
            break;

        case MAIN_MEM:
            vp_announceChannelSummary(&state.channel, state.channel_index+1,
                                      state.bank, infoFlags);
            break;

        #ifdef CONFIG_GPS
        case MENU_GPS:
            vp_announceGPSInfo(vpGPSAll);
            break;
        #endif

        case MENU_BACKUP:
            vp_announceBackupScreen();
            break;

        case MENU_RESTORE:
            vp_announceRestoreScreen();
            break;

        case MENU_ABOUT:
            vp_announceAboutScreen();
            break;

        #ifdef CONFIG_RTC
        case SETTINGS_TIMEDATE:
            vp_announceSettingsTimeDate();
            break;
        #endif

        case SETTINGS_M17:
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

    vpFlags_t flags = vpAnnounceCommonSymbols;
    // add edit mode flags to adjust what is spoken.
    // extra symbols not relevant when entering callsign.
    if ((editMode == true) && (callsign == false))
        flags |= vpAnnounceLessCommonSymbols
              |  vpAnnounceSpace
              |  vpAnnounceASCIIValueForUnknownChars;

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

void vp_playMenuBeepIfNeeded(bool firstItem)
{
// Since menus talk at levels above beep, there's no need to run this or you'll
// get an unwanted click.
    if (state.settings.vpLevel != vpBeep)
        return;
    if (firstItem)
        vp_beep(BEEP_MENU_FIRST_ITEM, SHORT_BEEP);
    else
        vp_beep(BEEP_MENU_ITEM, SHORT_BEEP);
}

void vp_announceSplashScreen()
{
    if (state.settings.vpLevel < vpBeep)
        return;

    vp_flush();

    if (state.settings.vpLevel == vpBeep)
    {
        vp_beepSeries(BOOT_MELODY);
        return;
    }

    vpQueueFlags_t localFlags = vpqAddSeparatingSilence;

    // Force on the descriptions for level 3.
    if (state.settings.vpLevel == vpHigh)
    {
        localFlags |= vpqIncludeDescriptions;
    }

    vp_queueStringTableEntry(&currentLanguage->openRTX);
    vp_queuePrompt(PROMPT_VFO);
    vp_announceFrequencies(state.channel.rx_frequency,
                           state.channel.tx_frequency,
                           localFlags);
    vp_announceRadioMode(state.channel.mode, localFlags);
    vp_play();
}

void vp_announceTimeZone(const int8_t timeZone, const vpQueueFlags_t flags)
{
    clearCurrPromptIfNeeded(flags);

    if (flags & vpqIncludeDescriptions)
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
