/***************************************************************************
 *   Copyright (C) 2022 by Alain Carlucci                                  *
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

#include <cstdio>
#include <cstdlib>

#include "interfaces/audio_stream.h"

#define CHECK(x)                                \
    do                                          \
    {                                           \
        if (!(x))                               \
        {                                       \
            puts("Failed assertion: " #x "\n"); \
            abort();                            \
        }                                       \
    } while (0)

int main()
{
    FILE* fp = fopen("MIC.raw", "wb");
    for (int i = 0; i < 13; i++)
    {
        fputc(i, fp);
        fputc(0, fp);
    }
    fclose(fp);

    stream_sample_t tmp[128];
    auto id = inputStream_start(
        AudioSource::SOURCE_MIC, AudioPriority::PRIO_BEEP, tmp,
        sizeof(tmp) / sizeof(tmp[0]), BufMode::BUF_LINEAR, 44100);
    inputStream_getData(id);
    for (int i = 0; i < 128; i++) CHECK(tmp[i] == (i % 13));
    return 0;
}
