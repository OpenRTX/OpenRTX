/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VOICE_PROMPT_UTILS_H
#define VOICE_PROMPT_UTILS_H

#include "core/cps.h"
#include "ui/ui_strings.h"
#include "core/voicePrompts.h"

/*
Please Note!

Many of the functions take queue flags because sometimes messages must be
played in sequence (i.e. the announceXX functions may be called one after the
other) and thus the init must only be sent prior to the first message queued
and the play must only be invoked after the last message queued.

When an announceXX function is called in isolation, vpqInit|vpqPlayImmediately
should be used to ensure that the message interupts the current prompt and
plays immediately.
*/


/**
 * Note: channelNumber is 1-based, index is 0-based.
 */
void vp_announceChannelName(const channel_t* channel,
                            const uint16_t channelNumber,
                            const vpQueueFlags_t flags);

/**
 *
 */
void vp_queueFrequency(const freq_t freq);

/**
 *
 */
void vp_announceFrequencies(const freq_t rx, const freq_t tx,
                            const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceRadioMode(const uint8_t mode, const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceBandwidth(const uint8_t bandwidth, const vpQueueFlags_t flags);

/**
 * channelNumber is 1-based, channelIndex is 0-based.
 */
void vp_announceChannelSummary(const channel_t* channel,
                               const uint16_t channelNumber, const uint16_t bank,
                               const vpSummaryInfoFlags_t infoFlags);

/**
 *
 */
void vp_announceInputChar(const char ch);

/**
 *
 */
void vp_announceInputReceiveOrTransmit(const bool tx, const vpQueueFlags_t flags);

/**
 *
 */
void vp_replayLastPrompt();

/**
 *
 */
void vp_announceError(const vpQueueFlags_t flags);

/*
This function first tries to see if we have a prompt for the text
passed in and if so, queues it, but if not, just spells the text
character by character.
*/

/**
 *
 */
void vp_announceText(const char* text, const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceCTCSS(const bool rxToneEnabled, const uint8_t rxTone,
                      const bool txToneEnabled, const uint8_t txTone,
                      const vpQueueFlags_t flags);

/**
 *
 */
void vp_announcePower(const uint32_t power, const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceSquelch(const uint8_t squelch,const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceContact(const contact_t* contact, const vpQueueFlags_t flags);

/**
 *
 */
bool vp_announceContactWithIndex(const uint16_t index, const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceTimeslot(const uint8_t timeslot, const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceColorCode(const uint8_t rxColorCode, const uint8_t txColorCode,
                          const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceBank(const uint16_t bank, const vpQueueFlags_t flags);

/**
 *
 */
void vp_announceM17Info(const channel_t* channel, bool isEditing,
                        const vpQueueFlags_t flags);

/**
 *
 */
#ifdef CONFIG_GPS
void vp_announceGPSInfo(vpGPSInfoFlags_t gpsInfoFlags);
#endif

/**
 *
 */
void vp_announceAboutScreen();

/**
 *
 */
void vp_announceBackupScreen();

/**
 *
 */
void vp_announceRestoreScreen();

/**
 *
 */
#ifdef CONFIG_RTC
void vp_announceSettingsTimeDate();
#endif

/**
 * This is called to speak voice prompt level changes.
 */
void vp_announceSettingsVoiceLevel(const vpQueueFlags_t flags);

/**
 * This is called to speak generic settings on/off toggles.
 */
void vp_announceSettingsOnOffToggle(const char* const* stringTableStringPtr,
                                    const vpQueueFlags_t flags, bool val);

/**
 * This is called to speak generic settings int values.
 */
void vp_announceSettingsInt(const char* const* stringTableStringPtr,
                            const vpQueueFlags_t flags, int val);

/**
 * This function is called from  ui_updateFSM to speak informational screens.
 */
void vp_announceScreen(uint8_t ui_screen);

/**
 * This function is called from  ui_updateFSM to speak string buffers.
 */
void vp_announceBuffer(const char* const* stringTableStringPtr, bool editMode,
                       bool callsign, const char* buffer);

/**
 * This function is called from  ui_updateFSM to speak display timer changes.
 */
void vp_announceDisplayTimer();

/**
 *
 */
vpQueueFlags_t vp_getVoiceLevelQueueFlags();

/**
 *
 */
void vp_playMenuBeepIfNeeded(bool firstItem);

/**
 *
 */
void vp_announceSplashScreen();

/**
 * Announce timezone value.
 *
 * @param timeZone: timezone value.
 * @param flags: control flags.
 */
void vp_announceTimeZone(const int8_t timeZone, const vpQueueFlags_t flags);

#endif  // VOICE_PROMPT_UTILS_H
