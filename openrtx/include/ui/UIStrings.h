/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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
/*
This string table's order must not be altered as voice prompts will be indexed in the same order as these strings.
*/
#ifndef UIStrings_h_included
#define UIStrings_h_included
#define NUM_LANGUAGES 1

typedef struct
{
	const char* languageName;
	const char* off;
	const char* on;
	const char* zone;
	const char* channels;
	const char* contacts;
	const char* gps;
	const char* settings;
	const char* backupAndRestore;
	const char* info;
	const char* about;
	const char* display;
	const char* timeAndDate;
	const char* m17;
	const char* defaultSettings;
	const char* brightness;
	const char* contrast;
	const char* timer;
	const char* gpsEnabled;
	const char* gpsSetTime;
		const char* UTCTimeZone;
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
} stringsTable_t;

extern const stringsTable_t languages[];
extern const stringsTable_t* currentLanguage;

#endif