/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#ifndef DMR_CODEC_H
#define DMR_CODEC_H

#include "OpMode.hpp"
#include <memory>

#define DMR_FRAME_SIZE 27 // 27 bytes per frame
#define DMR_BUFFER_SIZE (DMR_FRAME_SIZE)

namespace DMR {

typedef enum {
    VOCODER_DIR_DECODE,
    VOCODER_DIR_ENCODE,
} vocoder_mode_t;

class Codec
{
public:

    /**
     * Constructor.
     */
    Codec();

    /**
     * Destructor.
     */
    ~Codec();

    /**
     * Disable the operating mode. This function ensures that, after being
     * called, the radio, the audio amplifier and the microphone are in OFF state.
     *
     * Application must ensure this function is being called when exiting the
     * current operating mode.
     */
    void start(vocoder_mode_t mode);
    void stop();
    bool isLocked();

private:

    void init();
    void startThread(void* (*func) (void *));

    pthread_t codecThread;
};

}

#endif /* DMR_CODEC_H */
