/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UI_LAYOUT_CONFIG_H
#define UI_LAYOUT_CONFIG_H

#include "hwconfig.h"
#include "ui/ui_layout.h"

#if CONFIG_SCREEN_HEIGHT > 127  // Large: Tytera MD380, MD-UV380
#elif CONFIG_SCREEN_HEIGHT > 63 // Medium: Radioddity GD-77
#elif CONFIG_SCREEN_HEIGHT > 47 // Small: Radioddity RD-5R
#else
#error Unsupported vertical resolution
#endif

extern const layout_t _layout_config;

#endif /* UI_LAYOUT_CONFIG_H */
