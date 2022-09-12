/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <stddef.h>
#include <errno.h>
#include <slip.h>

#define END     0xC0
#define ESC     0xDB
#define ESC_END 0xDC
#define ESC_ESC 0xDD


int slip_encode(struct FrameCtx *frame, const void *data, const size_t len,
                const bool last)
{
    if((frame == NULL) || (data == NULL))
        return -EINVAL;

    // Prepend a frame start marker when beginning a new frame
    if((frame->oPos == 0) && (frame->iPos == 0))
    {
        frame->data[frame->oPos] = END;
        frame->oPos += 1;
    }

    uint8_t *ptr = (uint8_t *) data;
    while(frame->iPos < len)
    {
        uint8_t curByte = ptr[frame->iPos];
        frame->iPos    += 1;

        switch(curByte)
        {
            case END:
                frame->data[frame->oPos]     = ESC;
                frame->data[frame->oPos + 1] = ESC_END;
                frame->oPos += 2;
                break;

            case ESC:
                frame->data[frame->oPos]     = ESC;
                frame->data[frame->oPos + 1] = ESC_ESC;
                frame->oPos += 2;
                break;

            default:
                frame->data[frame->oPos] = curByte;
                frame->oPos += 1;
        }

        if(frame->oPos >= frame->maxLen)
            return -ENOMEM;
    }

    // If last frame, append the end marker
    if(last == true)
    {
        frame->data[frame->oPos] = END;
        frame->oPos += 1;
    }

    // All input data handled, reset input position
    frame->iPos = 0;

    return frame->oPos;
}

int slip_decode(struct FrameCtx *frame, const void *data, const size_t len,
                bool *end)
{
    if((frame == NULL) || (data == NULL))
        return -EINVAL;

    uint8_t *ptr = (uint8_t *) data;
    uint8_t prev = 0;

    *end = false;
    while((frame->iPos < len) && (*end == false))
    {
        uint8_t cur  = ptr[frame->iPos];
        frame->iPos += 1;

        switch(cur)
        {
            case END:
                // Skip the END marker if there is no data in the output frame,
                // discards the leading END of a frame and empty frames.
                if(frame->oPos > 0)
                    *end = true;
                break;

            case ESC:
                // Do nothing and advance
                break;

            case ESC_END:
            {
                if(prev == ESC)
                    frame->data[frame->oPos] = END;
                else
                    frame->data[frame->oPos] = cur;

                frame->oPos += 1;
            }
                break;

            case ESC_ESC:
            {
                if(prev == ESC)
                    frame->data[frame->oPos] = ESC;
                else
                    frame->data[frame->oPos] = cur;

                frame->oPos += 1;
            }
                break;

            default:
                frame->data[frame->oPos] = cur;
                frame->oPos += 1;
                break;
        }

        prev = cur;

        if(frame->oPos >= frame->maxLen)
            return -ENOMEM;
    }

    // If all data consumed, reset the input position
    size_t ret = frame->iPos;
    if(frame->iPos >= len)
        frame->iPos = 0;

    return ret;
}
