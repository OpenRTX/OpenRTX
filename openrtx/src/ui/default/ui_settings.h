/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * \file ui_settings.h
 * \brief Settings sub-screens for the default UI (see ui_settings.c).
 */

#ifndef UI_SETTINGS_H
#define UI_SETTINGS_H

#include "ui/ui_default.h"

void _ui_drawSettingsDisplay(ui_state_t *ui_state);

#ifdef CONFIG_GPS
void _ui_drawSettingsGPS(ui_state_t *ui_state);
#endif

#ifdef CONFIG_RTC
void _ui_drawSettingsTimeDate(void);
void _ui_drawSettingsTimeDateSet(ui_state_t *ui_state);
#endif

#ifdef CONFIG_M17
void _ui_drawSettingsM17(ui_state_t *ui_state);
#endif

void _ui_drawSettingsFM(ui_state_t *ui_state);
void _ui_drawSettingsAccessibility(ui_state_t *ui_state);
void _ui_drawSettingsReset2Defaults(ui_state_t *ui_state);
void _ui_drawSettingsRadio(ui_state_t *ui_state);

#endif /* UI_SETTINGS_H */
