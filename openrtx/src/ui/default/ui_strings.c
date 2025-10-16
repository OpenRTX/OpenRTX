/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <string.h>
#include "ui/ui_strings.h"
#include "ui/EnglishStrings.h"
#include "ui/SpanishStrings.h"

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
