/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/display.h"
#include "hwconfig.h"
#include <stddef.h>

void display_init()
{

}

void display_terminate()
{

}

void display_renderRows(uint8_t startRow, uint8_t endRow, void *fb)
{
    (void) startRow;
    (void) endRow;
    (void) fb;
}

void display_render(void *fb)
{
    (void) fb;
}

void display_setContrast(uint8_t contrast)
{
    (void) contrast;
}

void display_setBacklightLevel(uint8_t level)
{
    (void) level;
}
