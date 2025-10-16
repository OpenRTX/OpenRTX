/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UI_STRINGS_H
#define UI_STRINGS_H

/*
 * NOTE: This string table's order must not be altered as voice prompts will be
 * indexed in the same order as these strings.
 * Also, menus may be printed using string table offsets.
 *
 * Each field should be annotated with an i18n comment on the preceding line(s):
 *   // i18n: "English msgid"        — defines the translatable string
 *   // i18n: Translator comment      — free-form note for translators
 */
typedef struct
{
    // i18n: This should be the endonym for the language/locale; it is used in the locale selection menu
    // i18n: "English"
    const char* languageName;
    // i18n: "OFF"
    const char* off;
    // i18n: "ON"
    const char* on;
    // i18n: "Banks"
    const char* banks;
    // i18n: "Channels"
    const char* channels;
    // i18n: "Contacts"
    const char* contacts;
    // i18n: "GPS"
    const char* gps;
    // i18n: "Settings"
    const char* settings;
    // i18n: "Backup & Restore"
    const char* backupAndRestore;
    // i18n: "Info"
    const char* info;
    // i18n: "About"
    const char* about;
    // i18n: "Display"
    const char* display;
    // i18n: "Time & Date"
    const char* timeAndDate;
    // i18n: "FM"
    const char* fm;
    // i18n: "M17"
    const char* m17;
    // i18n: "DMR"
    const char* dmr;
    // i18n: "Default Settings"
    const char* defaultSettings;
    // i18n: "Brightness"
    const char* brightness;
    // i18n: "Contrast"
    const char* contrast;
    // i18n: "Timer"
    const char* timer;
    // i18n: "GPS Enabled"
    const char* gpsEnabled;
    // i18n: "GPS Set Time"
    const char* gpsSetTime;
    // i18n: "UTC Timezone"
    const char* UTCTimeZone;
    // i18n: "Voice"
    const char* voice;
    // i18n: "Level"
    const char* level;
    // i18n: "Phonetic"
    const char* phonetic;
    // i18n: "Beep"
    const char* beep;
    // i18n: "Backup"
    const char* backup;
    // i18n: "Restore"
    const char* restore;
    // i18n: "Bat. Voltage"
    const char* batteryVoltage;
    // i18n: "Bat. Charge"
    const char* batteryCharge;
    // i18n: "RSSI"
    const char* RSSI;
    // i18n: "Model"
    const char* model;
    // i18n: "Band"
    const char* band;
    // i18n: "VHF"
    const char* VHF;
    // i18n: "UHF"
    const char* UHF;
    // i18n: "LCD Type"
    const char* LCDType;
    // i18n: "Niccolo' IU2KIN"
    const char* Niccolo;
    // i18n: "Silvano IU2KWO"
    const char* Silvano;
    // i18n: "Federico IU2NUO"
    const char* Federico;
    // i18n: "Fred IU2NRO"
    const char* Fred;
    // i18n: "Joseph VK7JS"
    const char* Joseph;
    // i18n: "All channels"
    const char* allChannels;
    // i18n: "Menu"
    const char* menu;
    // i18n: "GPS OFF"
    const char* gpsOff;
    // i18n: "No Fix"
    const char* noFix;
    // i18n: "Fix Lost"
    const char* fixLost;
    // i18n: "ERROR"
    const char* error;
    // i18n: "Flash Backup"
    const char* flashBackup;
    // i18n: "Connect to RTXTool"
    const char* connectToRTXTool;
    // i18n: "to backup flash and"
    const char* toBackupFlashAnd;
    // i18n: "press PTT to start."
    const char* pressPTTToStart;
    // i18n: "Flash Restore"
    const char* flashRestore;
    // i18n: "to restore flash and"
    const char* toRestoreFlashAnd;
    // i18n: "OpenRTX"
    const char* openRTX;
    // i18n: "GPS Settings"
    const char* gpsSettings;
    // i18n: "M17 Settings"
    const char* m17settings;
    // i18n: "Callsign:"
    const char* callsign;
    // i18n: "Reset to Defaults"
    const char* resetToDefaults;
    // i18n: "To reset:"
    const char* toReset;
    // i18n: "Press Enter twice"
    const char* pressEnterTwice;
    // i18n: "Macro Menu"
    const char* macroMenu;
    // i18n: "For emergency use"
    const char* forEmergencyUse;
    // i18n: "press any button."
    const char* pressAnyButton;
    // i18n: "Accessibility"
    const char* accessibility;
    // i18n: "Used heap"
    const char* usedHeap;
    // i18n: "ALL"
    const char* broadcast;
    // i18n: "Radio Settings"
    const char* radioSettings;
    // i18n: "Offset"
    const char* offset;
    // i18n: "Macro Latch"
    const char* macroLatching;
    // i18n: "No GPS"
    const char* noGps;
    // i18n: "Battery Icon"
    const char* batteryIcon;
    // i18n: "CTCSS Tone"
    const char* CTCSSTone;
    // i18n: "CTCSS En."
    const char* CTCSSEn;
    // i18n: "Encode"
    const char* Encode;
    // i18n: "Decode"
    const char* Decode;
    // i18n: "Both"
    const char* Both;
    // i18n: "None"
    const char* None;
    // i18n: "Direction"
    const char* direction;
    // i18n: "Step"
    const char* step;
    // i18n: "Radio"
    const char* radio;
    // i18n: "CAN"
    const char* CAN;
    // i18n: "CAN RX Check"
    const char* canRxCheck;
}
stringsTable_t;

extern const stringsTable_t uiStrings;          // translated strings (generated)
extern const stringsTable_t englishMsgids;      // English keys for VP lookup (generated)

/**
 * Search for a given string into the English msgid table and, if found,
 * return its offset with respect to the beginning.
 *
 * @param text: string to be searched.
 * @return text position inside the string table or -1 if the string is not
 * present.
 */
int GetEnglishStringTableOffset(const char* text);

#endif
