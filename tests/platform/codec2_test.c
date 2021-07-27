/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
 *                         Silvano Seva IU2KWO                             *
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <interfaces/platform.h>
#include <rtx.h>
#include <hwconfig.h>
#include <memory_profiling.h>
#include <codec2.h>

#define N_FRAMES 4

/**
 * \internal Task function in charge of encoding and decoding audio with codec2.
 */
void *codec2_task(void *arg)
{
    (void) arg;

    unsigned free_stack = 0;
    struct CODEC2* codec2 = codec2_create(CODEC2_MODE_3200);
    free_stack = getAbsoluteFreeStack();

    int16_t audio[N_FRAMES * 320] = { 0 };
    uint8_t result[N_FRAMES * 16] = { 0 };

    for (size_t i = 0; i != N_FRAMES; i++)
    {
        codec2_encode(codec2, &result[0 + i * 16], &audio[0 + i * 320]);
        codec2_encode(codec2, &result[8 + i * 16], &audio[160 + i * 320]);
    }

    for (size_t j = 0; j != N_FRAMES; j++)
    {
        for (size_t i = 0; i != 16; i++)
        {
            printf("%02x ", result[i + j * 16]);
        }
        printf("\n\r");
    }

    printf("Codec2 uses %uB of stack\n\r", 16 * 1024 - free_stack);
    return 0;
}

int main()
{
    platform_init();
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setPin(GREEN_LED);

    delayMs(1000);

    gpio_clearPin(GREEN_LED);

    // Create codec2 thread
    pthread_t      codec2_thread;
    pthread_attr_t codec2_attr;

    pthread_attr_init(&codec2_attr);
    pthread_attr_setstacksize(&codec2_attr, 16 * 1024);
    pthread_create(&codec2_thread, &codec2_attr, codec2_task, NULL);

    while(1);
}
