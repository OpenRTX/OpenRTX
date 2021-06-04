/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Silvano Seva IU2KWO,                            *
 *                         Frederik Saraci IU2NRO                          *
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

#ifndef M17_H
#define M17_H

#define M17_VOICE_SAMPLE_RATE 8000
#define M17_RTX_SAMPLE_RATE   48000

// FRAME = SYNCWORD + PAYLOAD
#define M17_AUDIO_SIZE        368
#define M17_FRAME_SAMPLES     1920
#define M17_SYNCWORD_SYMBOLS  8
#define M17_PAYLOAD_SYMBOLS   184
#define M17_FRAME_SYMBOLS     192
#define M17_FRAME_BYTES       48

/**
 * This function modulates a single M17 frame, capturing the input audio from
 * the microphone and pushing the output audio to the RTX.
 */
void m17_modulate();

#endif /* M17_H */
