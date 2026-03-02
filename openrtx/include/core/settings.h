/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "hwconfig.h"
#include <stdbool.h>

/** \file
 * The settings.h file is the header defining the structure for the device
 * settings structure and the functions to load/save the settings to
 * non-volatile memory.
 * The settings are saved across two partitions using an A/B partitions scheme.
 * The settings are thus saved first in partition A then partition B and so on.
 * Each new settings structure is appended to the last one in the partition.
 * When the partition is full, it is erased. Because of the A/B partition
 * scheme, this ensures that there is always a copy of the settings existing
 * on the device. If an error occurs during the save, or if the latest copy
 * gets corruptes, there is at lease one previous copy stored in the other
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

typedef struct
{
    uint8_t  brightness;           ///< Display brightness
    uint8_t  contrast;             ///< Display contrast
    uint8_t  voxLevel;             ///< Vox level
    int8_t   utc_timezone;         ///< Timezone, in units of half hours
    char     callsign[10];         ///< Plaintext callsign
    uint16_t display_timer   : 4,  ///< Standby timer
             vpLevel         : 3,  ///< Voice prompt level
             vpPhoneticSpell : 1,  ///< Phonetic spell enabled
             macroMenuLatch  : 1,  ///< Automatic latch of macro menu
             gps_enabled     : 1,  ///< GPS active
             gpsSetTime      : 1,  ///< Use GPS to ajust RTC time
             m17_can_rx      : 1,  ///< Check M17 CAN on RX
             showBatteryIcon : 1,  ///< Battery display true: icon, false: percentage
             _reserved       : 3;  ///< Reserved for future use, keep set to 0
    char    M17_meta_text[53];     ///< M17 Meta Text to send
}
__attribute__((packed)) settings_t;

static const settings_t default_settings =
{
    100,                          // Brightness
#ifdef CONFIG_SCREEN_CONTRAST
    CONFIG_DEFAULT_CONTRAST,      // Contrast
#else
    255,                          // Contrast
#endif
    0,                            // Vox level
    0,                            // UTC Timezone
    "OPNRTX",                     // Default callsign
    TIMER_30S,                    // 30 seconds
    0,                            // Voice prompts off
    false,                        // Phonetic spell off
    true,                         // Automatic latch of macro menu enabled
    false,                        // GPS disabled
    false,                        // Don't update RTC with GPS
    false,                        // Do not check M17 CAN on RX
    false,                        // No battery icon
    0,                            // not used
    "OpenRTX",                    // Default M17 meta text
};

typedef enum
{
    PART_CLEAN,      ///< Partition contains entries
    PART_EMPTY,      ///< Partition contains no entry
    PART_CORRUPTED,  ///< Partition contains invalid data
} part_status;

/**
 * Structure defining the header of the settings store
 */
 typedef struct {
    uint32_t MAGIC;   ///< Must be 0x584E504F ("OPNX")
    uint16_t length;  ///< Total length of the structure (incl. header)
    uint16_t counter; ///< Free-running counter, incremented each time settings are savec
    uint16_t crc;     ///< Checksum of the structure (crc field set to 0)
 } __attribute__((packed)) settings_header_t;

/**
 * Structure defining the binary layout of the settings in NVM memory
 */
typedef struct {
    settings_header_t header; ///< Settings store header
    settings_t settings;      ///< Device settings
} __attribute__((packed)) settings_store_t;

typedef struct settings_storage_s {
    int dev;                       ///< NVM device number
    int part_A;                    ///< NVM partition number for partition A
    int part_B;                    ///< NVM partition number for partition B
    size_t part_A_offset;          ///< Offset to free space in partition A
    size_t part_B_offset;          ///< Offset to free space in partition B
    settings_store_t latest_store; ///< Contains the most up-to-date settings
    bool initialized; ///< Do latest settings contain the settings read from nvm?
    bool write_needed; ///< Do we need to write the settings (settings have changed or are an old version)
    part_status part_A_status; ///< Is partition A clean, empty, or corrupted
    part_status part_B_status; ///< Is partition B clean, empty, or corrupted
} settings_storage_t;

/**
 * Initialize a settings_storage_t structure to save and load device settings.
 *
 * @param s pointer to a pre-allocated settings_storage_t structure to be initialized
 * @param nvm_dev NVM device number in which to store the device settings
 * @param part_A NVM device partition number for storage partition A
 * @param part_B NVM device partition number for storage partition B
 * @return 0 if successful, negative error code otherwise
 */
int settings_initStorage(settings_storage_t *s, const int nvm_dev,
                         const int part_A, const int part_B);

/**
 * Load device settings from non-volatile memory.
 *
 * @param s pointer to an initialized settings_storage_t structure
 * @param settings pointer to a settings_t structure where to write the
 * loaded settings
 * @return 0 if successful, negative error code otherwise
 */
int settings_load(settings_storage_t *s, settings_t *settings);

/**
 * Save device settings to non-volatile memory.
 * Will not perform any actual write if the settings haven't changed.
 *
 * @param s pointer to an initialized settings_storage_t structure
 * @param settings pointer to a settings_t structure containing the settings
 * to save
 * @return 0 if successful, negative error code otherwise
 */
int settings_save(settings_storage_t *s, const settings_t *settings);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H */
