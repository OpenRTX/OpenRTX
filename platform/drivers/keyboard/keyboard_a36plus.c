/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                         Dual Tachyon                                    *
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

#include <peripherals/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/keyboard.h>
#include "hwconfig.h"


void kbd_init()
{
    /* Set both as inputs */
    gpio_setMode(KBD_K0, INPUT_PULL_UP);
    gpio_setMode(KBD_K1, INPUT_PULL_UP);
    gpio_setMode(KBD_K2, INPUT_PULL_UP);
    gpio_setMode(KBD_K3, INPUT_PULL_UP);
}

void kbd_terminate()
{
    /* Back to default state */
    gpio_setMode(KBD_K0, INPUT_PULL_UP);
    gpio_setMode(KBD_K1, INPUT_PULL_UP);
    gpio_setMode(KBD_K2, INPUT_PULL_UP);
    gpio_setMode(KBD_K3, INPUT_PULL_UP);
    gpio_setMode(KBD_DB3, INPUT_PULL_UP);
    gpio_setMode(KBD_DB2, INPUT_PULL_UP);
    gpio_setMode(KBD_DB1, INPUT_PULL_UP);
    gpio_setMode(KBD_DB0, INPUT_PULL_UP);
}

keyboard_t kbd_getKeys()
{
    keyboard_t keys = 0;
    
    // if(!gpio_readPin(KBD_DB2)) 
    // {
    //     keys |= KEY_F2;
    //     return keys;
    // }
    // if(!gpio_readPin(KBD_DB3)) 
    // {
    //     keys |= KEY_F1;
    //     return keys;
    // }

    //Set all rows as output high
    gpio_setMode(KBD_DB3, OUTPUT);
    gpio_setMode(KBD_DB2, OUTPUT);
    gpio_setMode(KBD_DB1, OUTPUT);
    gpio_setMode(KBD_DB0, OUTPUT);

    gpio_setPin(KBD_DB3);
    gpio_setPin(KBD_DB2);
    gpio_setPin(KBD_DB1);
    gpio_setPin(KBD_DB0);
    
    gpio_clearPin(KBD_DB3);

    delayUs(10);
    if(!gpio_readPin(KBD_K0)) { keys |= KEY_ENTER;  }
    if(!gpio_readPin(KBD_K1)) { keys |= KEY_1;  }
    if(!gpio_readPin(KBD_K2)) { keys |= KEY_4;  }
    if(!gpio_readPin(KBD_K3)) { keys |= KEY_7;  }

    gpio_setPin(KBD_DB3);
    gpio_clearPin(KBD_DB2);

    delayUs(10);
    if(!gpio_readPin(KBD_K0)) { keys |= KEY_UP;  }
    if(!gpio_readPin(KBD_K1)) { keys |= KEY_2;  }
    if(!gpio_readPin(KBD_K2)) { keys |= KEY_5;  }
    if(!gpio_readPin(KBD_K3)) { keys |= KEY_8;  }

    gpio_setPin(KBD_DB2);
    gpio_clearPin(KBD_DB1);

    delayUs(10);
    if(!gpio_readPin(KBD_K0)) { keys |= KEY_DOWN;  }
    if(!gpio_readPin(KBD_K1)) { keys |= KEY_3;  }
    if(!gpio_readPin(KBD_K2)) { keys |= KEY_6;  }
    if(!gpio_readPin(KBD_K3)) { keys |= KEY_9;  }

    gpio_setPin(KBD_DB1);
    gpio_clearPin(KBD_DB0);

    delayUs(10);
    if(!gpio_readPin(KBD_K0)) { keys |= KEY_ESC;  }
    if(!gpio_readPin(KBD_K1)) { keys |= KEY_STAR;  }
    if(!gpio_readPin(KBD_K2)) { keys |= KEY_0;  }
    if(!gpio_readPin(KBD_K3)) { keys |= KEY_HASH;  }


    gpio_setMode(KBD_DB3, INPUT_PULL_UP);
    gpio_setMode(KBD_DB2, INPUT_PULL_UP);
    gpio_setMode(KBD_DB1, INPUT_PULL_UP);
    gpio_setMode(KBD_DB0, INPUT_PULL_UP);

    // If no key in the first column (Enter, 1, 4, 7) is pressed, we can safely check for the macro keys
    if(!gpio_readPin(KBD_DB3) && (keys & (KEY_ENTER | KEY_1 | KEY_4 | KEY_7)) == 0)
    {
        keys |= KEY_MONI;
        return keys;
    }
    // Old logic for macro key on side key 1, too
    if(!gpio_readPin(KBD_DB1)) 
    {
        keys |= KEY_MONI;
    }
    return keys;
}
