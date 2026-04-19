/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * \file ui_menu.h
 * \brief Default UI list-based menu screens (implemented in ui_menu.c).
 *
 * Scrollable list primitives are in ui/ui_menu_list.h and
 * openrtx/src/ui/common/ui_menu_list.c; the
 * quick-adjust macro overlay is in ui_macro_menu.h / ui_macro_menu.c.
 */

#ifndef UI_MENU_H
#define UI_MENU_H

#include "ui/ui_default.h"

void _ui_drawMenuTop(ui_state_t *ui_state);
void _ui_drawMenuBank(ui_state_t *ui_state);
void _ui_drawMenuChannel(ui_state_t *ui_state);
void _ui_drawMenuContacts(ui_state_t *ui_state);
#ifdef CONFIG_GPS
void _ui_drawMenuGPS(void);
#endif
void _ui_drawMenuSettings(ui_state_t *ui_state);
void _ui_drawMenuBackupRestore(ui_state_t *ui_state);
void _ui_drawMenuBackup(ui_state_t *ui_state);
void _ui_drawMenuRestore(ui_state_t *ui_state);
void _ui_drawMenuInfo(ui_state_t *ui_state);
void _ui_drawMenuAbout(ui_state_t *ui_state);

/** @{ Settings row label/value callbacks (ui_menu.c; used with ui_menu_list) */

int _ui_getDisplayEntryName(char *buf, uint8_t max_len, uint8_t index);
int _ui_getDisplayValueName(char *buf, uint8_t max_len, uint8_t index);

#ifdef CONFIG_GPS
int _ui_getSettingsGPSEntryName(char *buf, uint8_t max_len, uint8_t index);
int _ui_getSettingsGPSValueName(char *buf, uint8_t max_len, uint8_t index);
#endif

#ifdef CONFIG_M17
int _ui_getM17EntryName(char *buf, uint8_t max_len, uint8_t index);
int _ui_getM17ValueName(char *buf, uint8_t max_len, uint8_t index);
#endif

int _ui_getFMEntryName(char *buf, uint8_t max_len, uint8_t index);
int _ui_getFMValueName(char *buf, uint8_t max_len, uint8_t index);

int _ui_getAccessibilityEntryName(char *buf, uint8_t max_len, uint8_t index);
int _ui_getAccessibilityValueName(char *buf, uint8_t max_len, uint8_t index);

int _ui_getRadioEntryName(char *buf, uint8_t max_len, uint8_t index);
int _ui_getRadioValueName(char *buf, uint8_t max_len, uint8_t index);

/** @} */

#endif /* UI_MENU_H */
