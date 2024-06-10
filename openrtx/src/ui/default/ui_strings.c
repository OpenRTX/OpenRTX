/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <stdint.h>
#include <string.h>
#include <ui/ui_strings.h>
#include <ui/EnglishStrings.h>
#include <ui/SpanishStrings.h>

const stringsTable_t languages[NUM_LANGUAGES] = {englishStrings,spanishStrings};
const stringsTable_t* currentLanguage = &languages[0];

int GetEnglishStringTableOffset(const char* text)
{
    if ((text == NULL) || (*text == '\0'))
        return -1;

    uint8_t stringCount = sizeof(stringsTable_t) / sizeof(char *);

    for (uint8_t i = 0; i < stringCount; ++i)
    {
        const char* strPtr = ((const char **)&englishStrings)[i];

        if (strcmp(text, strPtr) == 0)
        {
            return i;
        }
    }

    return -1;
}
