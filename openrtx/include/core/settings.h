/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "hwconfig.h"
#include <stdbool.h>

/**
 * \file
 * This file contains the definition for the device settings data structure and
 * the functions to load/save its content from non-volatile memory.
 *
 * Settings are saved alternately across two partitions using an A/B partitions
 * scheme to ensure that there is always a valid copy of the settings on the
 * device. If an error occurs during the save, or if the latest copy gets
 * corruptes, there is at lease one previous copy stored in the other
 * partition.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TIMER_OFF =  0,
    TIMER_5S  =  1,
    TIMER_10S =  2,
    TIMER_15S =  3,
    TIMER_20S =  4,
    TIMER_25S =  5,
    TIMER_30S =  6,
    TIMER_1M  =  7,
    TIMER_2M  =  8,
    TIMER_3M  =  9,
    TIMER_4M  = 10,
    TIMER_5M  = 11,
    TIMER_15M = 12,
    TIMER_30M = 13,
    TIMER_45M = 14,
    TIMER_1H  = 15
}
display_timer_t;

struct oldSettings
{
    uint8_t brightness;           // Display brightness
    uint8_t contrast;             // Display contrast
    uint8_t sqlLevel;             // Squelch level
    uint8_t voxLevel;             // Vox level
    int8_t  utc_timezone;         // Timezone, in units of half hours
    bool    gps_enabled;          // GPS active
    char    callsign[10];         // Plaintext callsign
    uint8_t display_timer   : 4,  // Standby timer
            m17_can         : 4;  // M17 CAN
    uint8_t vpLevel         : 3,  // Voice prompt level
            vpPhoneticSpell : 1,  // Phonetic spell enabled
            macroMenuLatch  : 1,  // Automatic latch of macro menu
            _reserved       : 3;
    bool    m17_can_rx;           // Check M17 CAN on RX
    char    m17_dest[10];         // M17 destination
    bool    showBatteryIcon;      // Battery display true: icon, false: percentage
    bool    gpsSetTime;           // Use GPS to ajust RTC time
    char    M17_meta_text[53];    // M17 Meta Text to send
}
__attribute__((packed));

typedef struct oldSettings settings_t;

/**
 * Device settings data structure.
 *
 * Updating the data structure MUST be done according to the following rules:
 * - obsolete fields are never removed;
 * - new fields have to be added at the end of the structure;
 * - for entries requiring less than 8 bits, use bitfields;
 * - do not use bool for true/false settings entries.
 */
struct settings
{
    uint8_t  brightness;           ///< Display brightness
    uint8_t  contrast;             ///< Display contrast
    uint8_t  voxLevel;             ///< Vox level
    int8_t   utc_timezone;         ///< Timezone, in units of half hours
    char     callsign[12];         ///< Plaintext callsign
    uint32_t display_timer   : 4,  ///< Standby timer
             vpLevel         : 3,  ///< Voice prompt level
             vpPhoneticSpell : 1,  ///< Phonetic spell enabled
             macroMenuLatch  : 1,  ///< Automatic latch of macro menu
             gps_enabled     : 1,  ///< GPS active
             gpsSetTime      : 1,  ///< Use GPS to ajust RTC time
             m17_can_rx      : 1,  ///< Check M17 CAN on RX
             showBatteryIcon : 1,  ///< Battery display true: icon, false: percentage
             _reserved       : 19; ///< Reserved for future use, keep set to 0
}
__attribute__((packed));

static const struct settings default_settings =
{
    100,                          // Brightness
#ifdef CONFIG_SCREEN_CONTRAST
    CONFIG_DEFAULT_CONTRAST,      // Contrast
#else
    255,                          // Contrast
#endif
    0,                            // Vox level
    0,                            // UTC Timezone
    "N0CALL",                     // Default callsign
    TIMER_30S,                    // 30 seconds
    0,                            // Voice prompts off
    false,                        // Phonetic spell off
    true,                         // Automatic latch of macro menu enabled
    false,                        // GPS disabled
    false,                        // Don't update RTC with GPS
    false,                        // Do not check M17 CAN on RX
    false,                        // No battery icon
    0,                            // not used
};

/**
 * Initialize a settings storage.
 *
 * @param devA: NVM device number for storage partition A
 * @param partA: partition number for storage partition A
 * @param devB: NVM device number for storage partition B
 * @param partB: partition number for storage partition B
 * @return 0 if successful, a negative error code otherwise
 */
int settings_init(const uint32_t devA, const uint32_t partA,
                  const uint32_t devB, const uint32_t partB);

/**
 * Load device settings from non-volatile memory.
 *
 * @param settings: pointer to settings data structure
 * @return 0 if successful, a negative error code otherwise
 */
int settings_load(struct settings *settings);

/**
 * Save device settings to non-volatile memory.
 * Will not perform any actual write if the settings haven't changed.
 *
 * @param settings: pointer to settings data structure
 * @return 0 if successful, a negative error code otherwise
 */
int settings_save(const struct settings *settings);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H */
