/***************************************************************************
 *   Copyright (C) 2022 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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
    const char* frequencyOffset;
    const char* macroLatching;
    const char* spectrum;
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
