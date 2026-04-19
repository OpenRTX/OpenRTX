/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * \file ui_draw_private.h
 * \brief Prototypes for default UI draw helpers shared between .c files
 *        under openrtx/src/ui/default/. Not a public API; do not include
 *        from code outside this directory.
 */

#ifndef UI_DRAW_PRIVATE_H
#define UI_DRAW_PRIVATE_H

#include "ui/ui_default.h"

const char *_ui_getToneEnabledString(bool tone_tx_enable, bool tone_rx_enable,
                                     bool use_abbreviation);

void _ui_drawFrequency(void);

void _ui_drawMainBottom(void);

#endif /* UI_DRAW_PRIVATE_H */
