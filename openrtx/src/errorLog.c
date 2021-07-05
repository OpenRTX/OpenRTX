/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <errorLog.h>
#include <stdint.h>
#include <string.h>
#include <utils.h>
#include <hwconfig.h>

#if defined(ERRLOG_ENABLE) && !defined(PLATFORM_GD77) && !defined(PLATFORM_DM1801)

typedef struct __attribute__((packed))
{
    uint16_t crc;
    uint8_t  length;
    char     text[128];
}
errorData_t;

static const size_t maxTextLength = 127;
errorData_t *logData = ((errorData_t *) 0x40024000);    /* Backup SRAM region */

void errorLog_init()
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;      /* Enable PWR domain                         */
    RCC->AHB1ENR |= RCC_AHB1ENR_BKPSRAMEN;  /* Enable backup SRAM                        */
    __DSB();

    PWR->CR  |= PWR_CR_DBP;                 /* Allow writing to backup domain registers  */
    PWR->CSR |= PWR_CSR_BRE;                /* Enable backup domain voltage regulator    */
    while (!(PWR->CSR & (PWR_CSR_BRR)));    /* Wait until regulator is ready             */

    /*
     * If text length is different from zero and there is a CRC mismatch, we
     * have that the memory space contains garbage: best thing to do before
     * proceeding is clearing it.
     */
    if(logData->length != 0)
    {
        uint16_t crc = crc16(logData->text, logData->length);
        if(crc != logData->crc) errorLog_clear();
    }
}

void IRQerrorLog(const char *string)
{
    /*
     * This code always places a '\0' between two subsequent calls of the
     * function to properly separate the strings.
     * All the computations about text length are written to take into account
     * of this additional character.
     */

    size_t bytesToWrite = strlen(string);

    if((logData->length + bytesToWrite + 1) >= maxTextLength)
    {
        bytesToWrite = maxTextLength - logData->length - 1;
    }

    if(logData->length != 0) logData->length += 1;      /* Keep trailing '\0' */
    char *writePos = logData->text + logData->length;
    memcpy(writePos, string, bytesToWrite);
    logData->length += bytesToWrite;
    logData->text[logData->length] = '\0';
    logData->crc = crc16(logData->text, logData->length);
}

const char *errorLog_getMessage()
{
    uint16_t crc = crc16(logData->text, logData->length);
    if(crc != logData->crc) return NULL;
    return logData->text;
}

void errorLog_printMessage(const point_t start)
{
    const char *err = errorLog_getMessage();
    if(err == NULL) return;

    gfx_clearScreen();
    point_t printPoint = start;
    size_t pos = 0;
    const color_t color_white = {255, 255, 255, 255};

    /* Print each line one below the other */
    while(pos != logData->length)
    {
        err += pos;
        gfx_print(printPoint, FONT_SIZE_6PT, TEXT_ALIGN_LEFT, color_white, err);
        for(; pos < logData->length; pos++)
        {
            if(err[pos] == '\0')
            {
                pos += 1;
                printPoint.y += 10;
                break;
            }
        }
    }

    gfx_render();
}

bool errorLog_notEmpty()
{
    uint16_t crc = crc16(logData->text, logData->length);
    return ((logData->length != 0) && (logData->crc == crc));
}

void errorLog_clear()
{
    memset(logData->text, 0x00, sizeof(logData->text));
    logData->length = 0;
    logData->crc    = 0;
}

#else

void errorLog_init() { }

void IRQerrorLog(const char *string) { (void) string; }

const char *errorLog_getMessage() { return NULL; }

void errorLog_printMessage(const point_t start) { (void) start; }

bool errorLog_notEmpty() { return false; }

void errorLog_clear() { }

#endif
