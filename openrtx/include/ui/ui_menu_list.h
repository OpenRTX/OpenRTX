/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * \file ui_menu_list.h
 * \brief Scrollable menu list drawing (name-only and name/value rows).
 *
 * Implemented in openrtx/src/ui/common/ui_menu_list.c. Voice prompts for
 * list navigation apply to the default UI only (not Module17).
 */

#ifndef UI_MENU_LIST_H
#define UI_MENU_LIST_H

#include "ui/ui_limits.h"

#if defined(OPENRTX_UI_MODULE17)
#include "ui/ui_mod17.h"
#else
#include "ui/ui_default.h"
#endif

void _ui_drawMenuList(uint8_t selected,
                      int (*getCurrentEntry)(char *buf, uint8_t max_len,
                                             uint8_t index));

void _ui_drawMenuListValue(ui_state_t *ui_state, uint8_t selected,
                           int (*getCurrentEntry)(char *buf, uint8_t max_len,
                                                  uint8_t index),
                           int (*getCurrentValue)(char *buf, uint8_t max_len,
                                                  uint8_t index));

void _ui_reset_menu_anouncement_tracking(void);

#endif /* UI_MENU_LIST_H */
