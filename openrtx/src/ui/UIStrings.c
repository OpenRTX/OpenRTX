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
#include <string.h>
#include "ui/UIStrings.h"
 #include "ui/EnglishStrings.h"
 #include <stdint.h>

 // add more languages here.
 const stringsTable_t languages[NUM_LANGUAGES]={ englishStrings };
  // default to English.
 const stringsTable_t* currentLanguage=&languages[0];


/*
Given an english string such as a menu item or value,
search the english string table and return the offset if found.
This can then be used to look up the localized string in the currentLanguages 
struct, or to announce an indexed voice prompt.
*/
int GetEnglishStringTableOffset( char* text)
{
	if (!text || !*text)
		return -1; // error.
                
	uint8_t stringCount =sizeof(stringsTable_t)/sizeof(char*);
                
	for (uint8_t i = 0; i < stringCount; ++i)
	{
		uint16_t offset = (i * sizeof(char*));
		const char* const * strPtr =(const char* const*) (&englishStrings.languageName + offset);
		if (strcmp(text, *strPtr) == 0)
		{
			return offset;
		}
	}
	
	return -1;
}
