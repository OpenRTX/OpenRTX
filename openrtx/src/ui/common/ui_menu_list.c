/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ui/ui_menu_list.h"
#include "interfaces/delays.h"
#include <string.h>

#ifndef OPENRTX_UI_MODULE17
#include "ui/ui_strings.h"
#include "core/voicePromptUtils.h"

static char priorSelectedMenuName[MAX_ENTRY_LEN] = "\0";
static char priorSelectedMenuValue[MAX_ENTRY_LEN] = "\0";
static bool priorEditMode = false;
static uint32_t lastValueUpdate = 0;
#endif

void _ui_reset_menu_anouncement_tracking(void)
{
#ifndef OPENRTX_UI_MODULE17
    *priorSelectedMenuName = '\0';
    *priorSelectedMenuValue = '\0';
#endif
}

#ifndef OPENRTX_UI_MODULE17
static bool DidSelectedMenuItemChange(char *menuName, char *menuValue)
{
    // menu name can't be empty.
    if ((menuName == NULL) || (*menuName == '\0'))
        return false;

    // If value is supplied it can't be empty but it does not have to be supplied.
    if ((menuValue != NULL) && (*menuValue == '\0'))
        return false;

    if (strcmp(menuName, priorSelectedMenuName) != 0) {
        strcpy(priorSelectedMenuName, menuName);
        if (menuValue != NULL)
            strcpy(priorSelectedMenuValue, menuValue);
        else
            *priorSelectedMenuValue =
                '\0'; // reset it since we've changed menu item.

        return true;
    }

    if ((menuValue != NULL)
        && (strcmp(menuValue, priorSelectedMenuValue) != 0)) {
        // avoid chatter when value changes rapidly!
        uint32_t now = getTick();

        uint32_t interval = now - lastValueUpdate;
        lastValueUpdate = now;
        if (interval < 1000)
            return false;
        strcpy(priorSelectedMenuValue, menuValue);
        return true;
    }

    return false;
}
/*
Normally we determine if we should say the word menu if a menu entry has no
associated value that can be changed.
There are some menus however with no associated value which are not submenus,
e.g. the entries under Channels, contacts, Info,
which are navigable but not modifyable.
*/
static bool ScreenContainsReadOnlyEntries(int menuScreen)
{
    switch (menuScreen) {
        case MENU_CHANNEL:
        case MENU_CONTACTS:
        case MENU_INFO:
            return true;
    }
    return false;
}

static void announceMenuItemIfNeeded(char *name, char *value, bool editMode)
{
    if (state.settings.vpLevel < vpLow)
        return;

    if ((name == NULL) || (*name == '\0'))
        return;

    if (DidSelectedMenuItemChange(name, value) == false)
        return;

    // Stop any prompt in progress and/or clear the buffer.
    vp_flush();

    vp_announceText(name, vpqDefault);
    // We determine if we should say the word Menu as follows:
    // The name has no  associated value ,
    // i.e. does not represent a modifyable name/value pair.
    // We're not in edit mode.
    // The screen is navigable but entries  are readonly.
    if (!value && !editMode && !ScreenContainsReadOnlyEntries(state.ui_screen))
        vp_queueStringTableEntry(&currentLanguage->menu);

    if (editMode)
        vp_queuePrompt(PROMPT_EDIT);
    if ((value != NULL) && (*value != '\0'))
        vp_announceText(value, vpqDefault);

    vp_play();
}
#endif /* !OPENRTX_UI_MODULE17 */

void _ui_drawMenuList(uint8_t selected,
                      int (*getCurrentEntry)(char *buf, uint8_t max_len,
                                             uint8_t index))
{
    point_t pos = layout.line1_pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen =
        (CONFIG_SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    color_t text_color = color_white;
    for (int item = 0, result = 0;
         (result == 0) && (pos.y < CONFIG_SCREEN_HEIGHT); item++) {
        // If selection is off the screen, scroll screen
        if (selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)(entry_buf, sizeof(entry_buf),
                                    item + scroll);
        if (result != -1) {
            text_color = color_white;
            if (item + scroll == selected) {
                text_color = color_black;
                // Draw rectangle under selected item, compensating for text height
                point_t rect_pos = { 0, pos.y - layout.menu_h + 3 };
                gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h,
                             color_white, true);
#ifndef OPENRTX_UI_MODULE17
                announceMenuItemIfNeeded(entry_buf, NULL, false);
#endif
            }
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, text_color,
                      entry_buf);
            pos.y += layout.menu_h;
        }
    }
}

void _ui_drawMenuListValue(ui_state_t *ui_state, uint8_t selected,
                           int (*getCurrentEntry)(char *buf, uint8_t max_len,
                                                  uint8_t index),
                           int (*getCurrentValue)(char *buf, uint8_t max_len,
                                                  uint8_t index))
{
    point_t pos = layout.line1_pos;
    // Number of menu entries that fit in the screen height
    uint8_t entries_in_screen =
        (CONFIG_SCREEN_HEIGHT - 1 - pos.y) / layout.menu_h + 1;
    uint8_t scroll = 0;
    char entry_buf[MAX_ENTRY_LEN] = "";
    char value_buf[MAX_ENTRY_LEN] = "";
    color_t text_color = color_white;
    for (int item = 0, result = 0;
         (result == 0) && (pos.y < CONFIG_SCREEN_HEIGHT); item++) {
        // If selection is off the screen, scroll screen
        if (selected >= entries_in_screen)
            scroll = selected - entries_in_screen + 1;
        // Call function pointer to get current menu entry string
        result = (*getCurrentEntry)(entry_buf, sizeof(entry_buf),
                                    item + scroll);
        // Call function pointer to get current entry value string
        result = (*getCurrentValue)(value_buf, sizeof(value_buf),
                                    item + scroll);
        if (result != -1) {
            text_color = color_white;
            if (item + scroll == selected) {
                // Draw rectangle under selected item, compensating for text height
                // If we are in edit mode, draw a hollow rectangle
                text_color = color_black;
                bool full_rect = true;
                if (ui_state->edit_mode) {
                    text_color = color_white;
                    full_rect = false;
                }
                point_t rect_pos = { 0, pos.y - layout.menu_h + 3 };
                gfx_drawRect(rect_pos, CONFIG_SCREEN_WIDTH, layout.menu_h,
                             color_white, full_rect);
#ifndef OPENRTX_UI_MODULE17
                bool editModeChanged = priorEditMode != ui_state->edit_mode;
                priorEditMode = ui_state->edit_mode;
                // force the menu item to be spoken  when the edit mode changes.
                // E.g. when pressing Enter on Display Brightness etc.
                if (editModeChanged)
                    priorSelectedMenuName[0] = '\0';
                if (!ui_state->edit_mode
                    || editModeChanged) { // If in edit mode, only want to speak the char being entered,,
                    //not repeat the entire display.
                    announceMenuItemIfNeeded(entry_buf, value_buf,
                                             ui_state->edit_mode);
                }
#endif
            }
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_LEFT, text_color,
                      entry_buf);
            gfx_print(pos, layout.menu_font, TEXT_ALIGN_RIGHT, text_color,
                      value_buf);
            pos.y += layout.menu_h;
        }
    }
}
