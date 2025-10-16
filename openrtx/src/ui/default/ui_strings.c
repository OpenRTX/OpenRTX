/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <string.h>
#include "ui/ui_strings.h"

int GetEnglishStringTableOffset(const char* text)
{
    if ((text == NULL) || (*text == '\0'))
        return -1;

    uint8_t stringCount = sizeof(stringsTable_t) / sizeof(char *);

    for (uint8_t i = 0; i < stringCount; ++i)
    {
        const char* strPtr = ((const char **)&englishMsgids)[i];

        if (strcmp(text, strPtr) == 0)
        {
            return i;
        }
    }

    return -1;
}
