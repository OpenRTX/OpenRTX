/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <ui/common/ui_utils.h>
#include <string.h>

bool ui_scrollString(char *string, bool reset, size_t max_len)
{
    if(reset)
    {
        // Get current string length
        size_t len = strlen(string);
        
        // Ensure we have room for the space and null terminator
        if(len + 2 <= max_len)
        {
            // Add space at the end for the gap
            string[len] = ' ';
            string[len + 1] = '\0';
        }
        return true;
    }
    
    // Save the first character
    char first = string[0];
    
    // Shift all characters to the left by one position
    size_t len = strlen(string);
    memmove(string, string + 1, len - 1);
    
    // Put the first character at the end
    string[len - 1] = first;
    
    return true;
    // TODO(i18n): add support for RTL language
}