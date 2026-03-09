/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UI_STRINGS_H
#define UI_STRINGS_H

#define NUM_LANGUAGES 2

/*
 * NOTE: This string table's order must not be altered as voice prompts will be
 * indexed in the same order as these strings.
 * Also, menus may be printed using string table offsets.
 */
typedef struct
{
    const char* languageName;
    const char* off;
    const char* on;
    const char* banks;
    const char* channels;
    const char* contacts;
    const char* gps;
    const char* settings;
    const char* backupAndRestore;
    const char* info;
    const char* about;
    const char* display;
    const char* timeAndDate;
    const char* fm;
    const char* m17;
    const char* dmr;
    const char* defaultSettings;
    const char* brightness;
    const char* contrast;
    const char* timer;
    const char* gpsEnabled;
    const char* gpsSetTime;
    const char* UTCTimeZone;
    const char* voice;
    const char* level;
    const char* phonetic;
    const char* beep;
    const char* backup;
    const char* restore;
    const char* batteryVoltage;
    const char* batteryCharge;
    const char* RSSI;
    const char* model;
    const char* band;
    const char* VHF;
    const char* UHF;
    const char* LCDType;
    const char* Niccolo;
    const char* Silvano;
    const char* Federico;
    const char* Fred;
    const char* Joseph;
    const char* allChannels;
    const char* menu;
    const char* gpsOff;
    const char* noFix;
    const char* fixLost;
    const char* error;
    const char* flashBackup;
    const char* connectToRTXTool;
    const char* toBackupFlashAnd;
    const char* pressPTTToStart;
    const char* flashRestore;
    const char* toRestoreFlashAnd;
    const char* openRTX;
    const char* gpsSettings;
    const char* m17settings;
    const char* callsign;
    const char* resetToDefaults;
    const char* toReset;
    const char* pressEnterTwice;
    const char* macroMenu;
    const char* forEmergencyUse;
    const char* pressAnyButton;
    const char* accessibility;
    const char* usedHeap;
    const char* broadcast;
    const char* radioSettings;
    const char* offset;
    const char* macroLatching;
    const char* noGps;
    const char* batteryIcon;
    const char* CTCSSTone;
    const char* CTCSSEn;
    const char* Encode;
    const char* Decode;
    const char* Both;
    const char* None;
    const char* direction;
    const char* step;
    const char* radio;
    const char* RX_CAN;
    const char* TX_CAN;
    const char* canRxCheck;
}
stringsTable_t;

extern const stringsTable_t languages[];
extern const stringsTable_t* currentLanguage;

/**
 * Search for a given string into the string table and, if found, return its
 * offset with respect to the beginning.
 *
 * @param text: string to be searched.
 * @return text position inside the string table or -1 if the string is not
 * present.
 */
int GetEnglishStringTableOffset(const char* text);

#endif
