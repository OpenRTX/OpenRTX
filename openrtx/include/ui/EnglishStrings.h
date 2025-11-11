/***************************************************************************
  *   Copyright (C) 2022 - 2025 by Federico Amedeo Izzo IU2NUO,            *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
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
#ifndef ENGLISHSTRINGS_H
#define ENGLISHSTRINGS_H

#include "core/ui.h"
#include "ui/ui_strings.h"

const stringsTable_t englishStrings =
{
  // i18n: This should be the endonym for the language/locale; it is used in the locale selection menu
    .languageName      = gettext("English"),
    .off               = gettext("OFF"),
    .on                = gettext("ON"),
    .banks             = gettext("Banks"),
    .channels          = gettext("Channels"),
    .contacts          = gettext("Contacts"),
    .gps               = gettext("GPS"),
    .settings          = gettext("Settings"),
    .backupAndRestore  = gettext("Backup & Restore"),
    .info              = gettext("Info"),
    .about             = gettext("About"),
    .display           = gettext("Display"),
    .timeAndDate       = gettext("Time & Date"),
    .fm                = gettext("FM"),
    .m17               = gettext("M17"),
    .dmr               = gettext("DMR"),
    .defaultSettings   = gettext("Default Settings"),
    .brightness        = gettext("Brightness"),
    .contrast          = gettext("Contrast"),
    .timer             = gettext("Timer"),
    .gpsEnabled        = gettext("GPS Enabled"),
    .gpsSetTime        = gettext("GPS Set Time"),
    .UTCTimeZone       = gettext("UTC Timezone"),
    .voice             = gettext("Voice"),
    .level             = gettext("Level"),
    .phonetic          = gettext("Phonetic"),
    .beep              = gettext("Beep"),
    .backup            = gettext("Backup"),
    .restore           = gettext("Restore"),
    .batteryVoltage    = gettext("Bat. Voltage"),
    .batteryCharge     = gettext("Bat. Charge"),
    .RSSI              = gettext("RSSI"),
    .model             = gettext("Model"),
    .band              = gettext("Band"),
    .VHF               = gettext("VHF"),
    .UHF               = gettext("UHF"),
    .LCDType           = gettext("LCD Type"),
    .Niccolo           = gettext("Niccolo' IU2KIN"),
    .Silvano           = gettext("Silvano IU2KWO"),
    .Federico          = gettext("Federico IU2NUO"),
    .Fred              = gettext("Fred IU2NRO"),
    .Joseph            = gettext("Joseph VK7JS"),
    .allChannels       = gettext("All channels"),
    .menu              = gettext("Menu"),
    .gpsOff            = gettext("GPS OFF"),
    .noFix             = gettext("No Fix"),
    .fixLost           = gettext("Fix Lost"),
    .error             = gettext("ERROR"),
    .flashBackup       = gettext("Flash Backup"),
    .connectToRTXTool  = gettext("Connect to RTXTool"),
    .toBackupFlashAnd  = gettext("to backup flash and"),
    .pressPTTToStart   = gettext("press PTT to start."),
    .flashRestore      = gettext("Flash Restore"),
    .toRestoreFlashAnd = gettext("to restore flash and"),
    .openRTX           = gettext("OpenRTX"),
    .gpsSettings       = gettext("GPS Settings"),
    .m17settings       = gettext("M17 Settings"),
    .callsign          = gettext("Callsign:"),
    .resetToDefaults   = gettext("Reset to Defaults"),
    .toReset           = gettext("To reset:"),
    .pressEnterTwice   = gettext("Press Enter twice"),
    .macroMenu         = gettext("Macro Menu"),
    .forEmergencyUse   = gettext("For emergency use"),
    .pressAnyButton    = gettext("press any button."),
    .accessibility     = gettext("Accessibility"),
    .usedHeap          = gettext("Used heap"),
    .broadcast         = gettext("ALL"),
    .radioSettings     = gettext("Radio Settings"),
    .offset            = gettext("Offset"),
    .macroLatching     = gettext("Macro Latch"),
    .noGps             = gettext("No GPS"),
    .batteryIcon       = gettext("Battery Icon"),
    .CTCSSTone         = gettext("CTCSS Tone"),
    .CTCSSEn           = gettext("CTCSS En."),
    .Encode            = gettext("Encode"),
    .Decode            = gettext("Decode"),
    .Both              = gettext("Both"),
    .None              = gettext("None"),
    .direction         = gettext("Direction"),
    .step              = gettext("Step"),
    .radio             = gettext("Radio"),
};
#endif  // ENGLISHSTRINGS_H
