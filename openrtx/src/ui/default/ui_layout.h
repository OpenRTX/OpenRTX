/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include "ui/ui_default.h"

#if CONFIG_SCREEN_HEIGHT > 127    // Large: Tytera MD380, MD-UV380
#elif CONFIG_SCREEN_HEIGHT > 63   // Medium: Radioddity GD-77
#elif CONFIG_SCREEN_HEIGHT > 47   // Small: Radioddity RD-5R
#else
#error Unsupported vertical resolution
#endif

extern const layout_t _layout_config;

void _ui_calculateLayout(layout_t *layout);

#endif
