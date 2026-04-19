/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdint.h>
#include <string.h>
#include "ui_fsm.h"
#include "ui/ui_default.h"
#include "ui_input.h"
#include "ui/ui_menu_list.h"
#include "core/input.h"
#include "core/datetime.h"
#include "interfaces/platform.h"
#include "interfaces/cps_io.h"
#include "interfaces/delays.h"
#include "interfaces/display.h"
#include "interfaces/keyboard.h"
#include "core/voicePromptUtils.h"
#include "core/beeps.h"
#include "rtx/rtx.h"
#include "hwconfig.h"

bool macro_latched;
ui_state_t ui_state;

static bool macro_menu = false;
static bool standby = false;
static long long last_event_tick = 0;
static bool redraw_needed = true;
static uint8_t evQueue_rdPos;
static uint8_t evQueue_wrPos;
static event_t evQueue[MAX_NUM_EVENTS];

#ifdef CONFIG_GPS
static uint16_t priorGPSSpeed = 0;
static int16_t priorGPSAltitude = 0;
static int16_t priorGPSDirection = 500; // impossible value init.
static uint8_t priorGPSFixQuality = 0;
static uint8_t priorGPSFixType = 0;
static uint8_t priorSatellitesInView = 0;
static uint32_t vpGPSLastUpdate = 0;

static vpGPSInfoFlags_t GetGPSDirectionOrSpeedChanged()
{
    if (!state.settings.gps_enabled)
        return vpGPSNone;

    uint32_t now = getTick();
    if (now - vpGPSLastUpdate < 8000)
        return vpGPSNone;

    vpGPSInfoFlags_t whatChanged = vpGPSNone;

    if (state.gps_data.fix_quality != priorGPSFixQuality) {
        whatChanged |= vpGPSFixQuality;
        priorGPSFixQuality = state.gps_data.fix_quality;
    }

    if (state.gps_data.fix_type != priorGPSFixType) {
        whatChanged |= vpGPSFixType;
        priorGPSFixType = state.gps_data.fix_type;
    }

    if (state.gps_data.speed != priorGPSSpeed) {
        whatChanged |= vpGPSSpeed;
        priorGPSSpeed = state.gps_data.speed;
    }

    if (state.gps_data.altitude != priorGPSAltitude) {
        whatChanged |= vpGPSAltitude;
        priorGPSAltitude = state.gps_data.altitude;
    }

    if (state.gps_data.tmg_true != priorGPSDirection) {
        whatChanged |= vpGPSDirection;
        priorGPSDirection = state.gps_data.tmg_true;
    }

    if (state.gps_data.satellites_in_view != priorSatellitesInView) {
        whatChanged |= vpGPSSatCount;
        priorSatellitesInView = state.gps_data.satellites_in_view;
    }

    if (whatChanged)
        vpGPSLastUpdate = now;

    return whatChanged;
}
#endif // CONFIG_GPS
static freq_t _ui_freq_add_digit(freq_t freq, uint8_t pos, uint8_t number)
{
    freq_t coefficient = 100;
    for (uint8_t i = 0; i < FREQ_DIGITS - pos; i++) {
        coefficient *= 10;
    }
    return freq += number * coefficient;
}

#ifdef CONFIG_RTC
static void _ui_timedate_add_digit(datetime_t *timedate, uint8_t pos,
                                   uint8_t number)
{
    vp_flush();
    vp_queueInteger(number);
    if (pos == 2 || pos == 4)
        vp_queuePrompt(PROMPT_SLASH);
    // just indicates separation of date and time.
    if (pos == 6) // start of time.
        vp_queueString("hh:mm",
                       vpAnnounceCommonSymbols | vpAnnounceLessCommonSymbols);
    if (pos == 8)
        vp_queuePrompt(PROMPT_COLON);
    vp_play();

    switch (pos) {
        // Set date
        case 1:
            timedate->date += number * 10;
            break;
        case 2:
            timedate->date += number;
            break;
        // Set month
        case 3:
            timedate->month += number * 10;
            break;
        case 4:
            timedate->month += number;
            break;
        // Set year
        case 5:
            timedate->year += number * 10;
            break;
        case 6:
            timedate->year += number;
            break;
        // Set hour
        case 7:
            timedate->hour += number * 10;
            break;
        case 8:
            timedate->hour += number;
            break;
        // Set minute
        case 9:
            timedate->minute += number * 10;
            break;
        case 10:
            timedate->minute += number;
            break;
    }
}
#endif

static bool _ui_freq_check_limits(freq_t freq)
{
    bool valid = false;
    const hwInfo_t *hwinfo = platform_getHwInfo();
    if (hwinfo->vhf_band) {
        // hwInfo_t frequencies are in MHz
        if (freq >= (hwinfo->vhf_minFreq * 1000000)
            && freq <= (hwinfo->vhf_maxFreq * 1000000))
            valid = true;
    }
    if (hwinfo->uhf_band) {
        // hwInfo_t frequencies are in MHz
        if (freq >= (hwinfo->uhf_minFreq * 1000000)
            && freq <= (hwinfo->uhf_maxFreq * 1000000))
            valid = true;
    }
    return valid;
}

static bool _ui_channel_valid(channel_t *channel)
{
    return _ui_freq_check_limits(channel->rx_frequency)
        && _ui_freq_check_limits(channel->tx_frequency);
}

static int _ui_fsm_loadChannel(int16_t channel_index, bool *sync_rtx)
{
    channel_t channel;
    int32_t selected_channel = channel_index;
    // If a bank is active, get index from current bank
    if (state.bank_enabled) {
        bankHdr_t bank = { 0 };
        cps_readBankHeader(&bank, state.bank);
        if ((channel_index < 0) || (channel_index >= bank.ch_count))
            return -1;
        channel_index = cps_readBankData(state.bank, channel_index);
    }

    int result = cps_readChannel(&channel, channel_index);
    // Read successful and channel is valid
    if ((result != -1) && _ui_channel_valid(&channel)) {
        // Set new channel index
        state.channel_index = selected_channel;
        // Copy channel read to state
        state.channel = channel;
        *sync_rtx = true;
    }

    return result;
}

static void _ui_fsm_confirmVFOInput(bool *sync_rtx)
{
    vp_flush();
    // Switch to TX input
    if (ui_state.input_set == SET_RX) {
        ui_state.input_set = SET_TX;
        // Reset input position
        ui_state.input_position = 0;
        // announce the rx frequency just confirmed with Enter.
        vp_queueFrequency(ui_state.new_rx_frequency);
        // defer playing till the end.
        // indicate that the user has moved to the tx freq field.
        vp_announceInputReceiveOrTransmit(true, vpqDefault);
    } else if (ui_state.input_set == SET_TX) {
        // Save new frequency setting
        // If TX frequency was not set, TX = RX
        if (ui_state.new_tx_frequency == 0) {
            ui_state.new_tx_frequency = ui_state.new_rx_frequency;
        }
        // Apply new frequencies if they are valid
        if (_ui_freq_check_limits(ui_state.new_rx_frequency)
            && _ui_freq_check_limits(ui_state.new_tx_frequency)) {
            state.channel.rx_frequency = ui_state.new_rx_frequency;
            state.channel.tx_frequency = ui_state.new_tx_frequency;
            *sync_rtx = true;
            // force init to clear any prompts in progress.
            // defer play because play is called at the end of the function
            //due to above freq queuing.
            vp_announceFrequencies(state.channel.rx_frequency,
                                   state.channel.tx_frequency, vpqInit);
        } else {
            vp_announceError(vpqInit);
        }

        state.ui_screen = MAIN_VFO;
    }

    vp_play();
}

static void _ui_fsm_insertVFONumber(kbd_msg_t msg, bool *sync_rtx)
{
    // Advance input position
    ui_state.input_position += 1;
    // clear any prompts in progress.
    vp_flush();
    // Save pressed number to calculate frequency and show in GUI
    ui_state.input_number = input_getPressedNumber(msg);
    // queue the digit just pressed.
    vp_queueInteger(ui_state.input_number);
    // queue  point if user has entered three digits.
    if (ui_state.input_position == 3)
        vp_queuePrompt(PROMPT_POINT);

    if (ui_state.input_set == SET_RX) {
        if (ui_state.input_position == 1)
            ui_state.new_rx_frequency = 0;
        // Calculate portion of the new RX frequency
        ui_state.new_rx_frequency =
            _ui_freq_add_digit(ui_state.new_rx_frequency,
                               ui_state.input_position, ui_state.input_number);
        if (ui_state.input_position
            >= FREQ_DIGITS) { // queue the rx freq just completed.
            vp_queueFrequency(ui_state.new_rx_frequency);
            /// now queue tx as user has changed fields.
            vp_queuePrompt(PROMPT_TRANSMIT);
            // Switch to TX input
            ui_state.input_set = SET_TX;
            // Reset input position
            ui_state.input_position = 0;
            // Reset TX frequency
            ui_state.new_tx_frequency = 0;
        }
    } else if (ui_state.input_set == SET_TX) {
        if (ui_state.input_position == 1)
            ui_state.new_tx_frequency = 0;
        // Calculate portion of the new TX frequency
        ui_state.new_tx_frequency =
            _ui_freq_add_digit(ui_state.new_tx_frequency,
                               ui_state.input_position, ui_state.input_number);
        if (ui_state.input_position >= FREQ_DIGITS) {
            // Save both inserted frequencies
            if (_ui_freq_check_limits(ui_state.new_rx_frequency)
                && _ui_freq_check_limits(ui_state.new_tx_frequency)) {
                state.channel.rx_frequency = ui_state.new_rx_frequency;
                state.channel.tx_frequency = ui_state.new_tx_frequency;
                *sync_rtx = true;
                // play is called at end.
                vp_announceFrequencies(state.channel.rx_frequency,
                                       state.channel.tx_frequency, vpqInit);
            }

            state.ui_screen = MAIN_VFO;
        }
    }

    vp_play();
}

#ifdef CONFIG_SCREEN_BRIGHTNESS
static void _ui_changeBrightness(int variation)
{
    state.settings.brightness += variation;

    // Max value for brightness is 100, min value is set to 5 to avoid complete
    //  display shutdown.
    if (state.settings.brightness > 100)
        state.settings.brightness = 100;
    if (state.settings.brightness < 5)
        state.settings.brightness = 5;

    display_setBacklightLevel(state.settings.brightness);
}
#endif

#ifdef CONFIG_SCREEN_CONTRAST
static void _ui_changeContrast(int variation)
{
    if (variation >= 0)
        state.settings.contrast = (255 - state.settings.contrast < variation) ?
                                      255 :
                                      state.settings.contrast + variation;
    else
        state.settings.contrast = (state.settings.contrast < -variation) ?
                                      0 :
                                      state.settings.contrast + variation;

    display_setContrast(state.settings.contrast);
}
#endif

static void _ui_changeTimer(int variation)
{
    if ((state.settings.display_timer == TIMER_OFF && variation < 0)
        || (state.settings.display_timer == TIMER_1H && variation > 0)) {
        return;
    }

    state.settings.display_timer += variation;
}

static void _ui_changeMacroLatch(bool newVal)
{
    state.settings.macroMenuLatch = newVal ? 1 : 0;
    vp_announceSettingsOnOffToggle(&currentLanguage->macroLatching,
                                   vp_getVoiceLevelQueueFlags(),
                                   state.settings.macroMenuLatch);
}

#ifdef CONFIG_M17
static inline void _ui_changeM17Can(int variation)
{
    uint8_t can = state.settings.m17_can;
    state.settings.m17_can = (can + variation) % 16;
}
#endif

static void _ui_changeVoiceLevel(int variation)
{
    if ((state.settings.vpLevel == vpNone && variation < 0)
        || (state.settings.vpLevel == vpHigh && variation > 0)) {
        return;
    }

    state.settings.vpLevel += variation;

    // Force these flags to ensure the changes are spoken for levels 1 through 3.
    vpQueueFlags_t flags = vpqInit | vpqAddSeparatingSilence
                         | vpqPlayImmediately;

    if (!vp_isPlaying()) {
        flags |= vpqIncludeDescriptions;
    }

    vp_announceSettingsVoiceLevel(flags);
}

static void _ui_changePhoneticSpell(bool newVal)
{
    state.settings.vpPhoneticSpell = newVal ? 1 : 0;

    vp_announceSettingsOnOffToggle(&currentLanguage->phonetic,
                                   vp_getVoiceLevelQueueFlags(),
                                   state.settings.vpPhoneticSpell);
}

bool _ui_checkStandby(long long time_since_last_event)
{
    if (standby) {
        return false;
    }

    switch (state.settings.display_timer) {
        case TIMER_OFF:
            return false;
        case TIMER_5S:
        case TIMER_10S:
        case TIMER_15S:
        case TIMER_20S:
        case TIMER_25S:
        case TIMER_30S:
            return time_since_last_event
                >= (5000 * state.settings.display_timer);
        case TIMER_1M:
        case TIMER_2M:
        case TIMER_3M:
        case TIMER_4M:
        case TIMER_5M:
            return time_since_last_event
                >= (60000 * (state.settings.display_timer - (TIMER_1M - 1)));
        case TIMER_15M:
        case TIMER_30M:
        case TIMER_45M:
            return time_since_last_event
                >= (60000 * 15
                    * (state.settings.display_timer - (TIMER_15M - 1)));
        case TIMER_1H:
            return time_since_last_event >= 60 * 60 * 1000;
    }

    // unreachable code
    return false;
}

static void _ui_enterStandby()
{
    if (standby)
        return;

    standby = true;
    redraw_needed = false;
    display_setBacklightLevel(0);
}

static bool _ui_exitStandby(long long now)
{
    last_event_tick = now;

    if (!standby)
        return false;

    standby = false;
    redraw_needed = true;
    display_setBacklightLevel(state.settings.brightness);

    return true;
}

// TODO: find a better home for this function
static int _ui_handleToneSelectScroll(bool direction_up)
{
    bool tone_tx_enable = state.channel.fm.txToneEn;
    bool tone_rx_enable = state.channel.fm.rxToneEn;
    uint8_t tone_flags = tone_tx_enable << 1 | tone_rx_enable;

    if (direction_up)
        tone_flags++;
    else
        tone_flags--;

    tone_flags %= 4;
    tone_tx_enable = tone_flags >> 1;
    tone_rx_enable = tone_flags & 1;
    state.channel.fm.txToneEn = tone_tx_enable;
    state.channel.fm.rxToneEn = tone_rx_enable;

    return 1;
}

static void _ui_fsm_menuMacro(kbd_msg_t msg, bool *sync_rtx)
{
    // If there is no keyboard left and right select the menu entry to edit
#if defined(CONFIG_UI_NO_KEYBOARD)
    if (msg.keys & KNOB_LEFT) {
        ui_state.macro_menu_selected--;
        ui_state.macro_menu_selected += 9;
        ui_state.macro_menu_selected %= 9;
    }
    if (msg.keys & KNOB_RIGHT) {
        ui_state.macro_menu_selected++;
        ui_state.macro_menu_selected %= 9;
    }
    if ((msg.keys & KEY_ENTER) && !msg.long_press)
        ui_state.input_number = ui_state.macro_menu_selected + 1;
    else
        ui_state.input_number = 0;
#else  // CONFIG_UI_NO_KEYBOARD
    ui_state.input_number = input_getPressedNumber(msg);
#endif // CONFIG_UI_NO_KEYBOARD
    // CTCSS Encode/Decode Selection
    vpQueueFlags_t queueFlags = vp_getVoiceLevelQueueFlags();

    switch (ui_state.input_number) {
        case 1:
            if (state.channel.mode == OPMODE_FM) {
                _ui_handleToneSelectScroll(true);
                *sync_rtx = true;
                vp_announceCTCSS(state.channel.fm.rxToneEn,
                                 state.channel.fm.rxTone,
                                 state.channel.fm.txToneEn,
                                 state.channel.fm.txTone,
                                 queueFlags | vpqIncludeDescriptions);
            }
            break;
        case 2:
            if (state.channel.mode == OPMODE_FM) {
                if (state.channel.fm.txTone == 0) {
                    state.channel.fm.txTone = CTCSS_FREQ_NUM - 1;
                } else {
                    state.channel.fm.txTone--;
                }

                state.channel.fm.txTone %= CTCSS_FREQ_NUM;
                state.channel.fm.rxTone = state.channel.fm.txTone;
                *sync_rtx = true;
                vp_announceCTCSS(state.channel.fm.rxToneEn,
                                 state.channel.fm.rxTone,
                                 state.channel.fm.txToneEn,
                                 state.channel.fm.txTone, queueFlags);
            }
            break;

        case 3:
            if (state.channel.mode == OPMODE_FM) {
                state.channel.fm.txTone++;
                state.channel.fm.txTone %= CTCSS_FREQ_NUM;
                state.channel.fm.rxTone = state.channel.fm.txTone;
                *sync_rtx = true;
                vp_announceCTCSS(state.channel.fm.rxToneEn,
                                 state.channel.fm.rxTone,
                                 state.channel.fm.txToneEn,
                                 state.channel.fm.txTone,
                                 queueFlags | vpqIncludeDescriptions);
            }
            break;
        case 4:
            if (state.channel.mode == OPMODE_FM) {
                state.channel.bandwidth++;
                state.channel.bandwidth %= 2;
                *sync_rtx = true;
                vp_announceBandwidth(state.channel.bandwidth, queueFlags);
            }
            break;
        case 5:
// Cycle through radio modes
#ifdef CONFIG_M17
            if (state.channel.mode == OPMODE_FM)
                state.channel.mode = OPMODE_M17;
            else if (state.channel.mode == OPMODE_M17)
                state.channel.mode = OPMODE_FM;
            else //catch any invalid states so they don't get locked out
#endif
                state.channel.mode = OPMODE_FM;
            *sync_rtx = true;
            vp_announceRadioMode(state.channel.mode, queueFlags);
            break;
        case 6:

            switch (state.channel.power) {
                case 1000:
                    state.channel.power = 2500;
                    break;

                case 2500:
                    state.channel.power = 5000;
                    break;

                default:
                    state.channel.power = 1000;
            }

            *sync_rtx = true;
            vp_announcePower(state.channel.power, queueFlags);
            break;
#ifdef CONFIG_SCREEN_BRIGHTNESS
        case 7:
            _ui_changeBrightness(-5);
            vp_announceSettingsInt(&currentLanguage->brightness, queueFlags,
                                   state.settings.brightness);
            break;
        case 8:
            _ui_changeBrightness(+5);
            vp_announceSettingsInt(&currentLanguage->brightness, queueFlags,
                                   state.settings.brightness);
            break;
#endif
        case 9:
            if (!ui_state.input_locked)
                ui_state.input_locked = true;
            else
                ui_state.input_locked = false;
            break;
    }

#if defined(PLATFORM_TTWRPLUS)
    if (msg.keys & KEY_VOLDOWN)
#else
    if (msg.keys & KEY_LEFT || msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT)
#endif                      // PLATFORM_TTWRPLUS
    {
#ifdef CONFIG_KNOB_ABSOLUTE // If the radio has an absolute position knob
        state.settings.sqlLevel = platform_getChSelector() - 1;
#endif                      // CONFIG_KNOB_ABSOLUTE
        if (state.settings.sqlLevel > 0) {
            state.settings.sqlLevel -= 1;
            *sync_rtx = true;
            vp_announceSquelch(state.settings.sqlLevel, queueFlags);
        }
    }
#if defined(PLATFORM_TTWRPLUS)
    else if (msg.keys & KEY_VOLUP)
#else
    else if (msg.keys & KEY_RIGHT || msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)
#endif // PLATFORM_TTWRPLUS
    {
#ifdef CONFIG_KNOB_ABSOLUTE
        state.settings.sqlLevel = platform_getChSelector() - 1;
#endif
        if (state.settings.sqlLevel < 15) {
            state.settings.sqlLevel += 1;
            *sync_rtx = true;
            vp_announceSquelch(state.settings.sqlLevel, queueFlags);
        }
    }
}

static void _ui_menuUp(uint8_t menu_entries)
{
    if (ui_state.menu_selected > 0)
        ui_state.menu_selected -= 1;
    else
        ui_state.menu_selected = menu_entries - 1;
    vp_playMenuBeepIfNeeded(ui_state.menu_selected == 0);
}

static void _ui_menuDown(uint8_t menu_entries)
{
    if (ui_state.menu_selected < menu_entries - 1)
        ui_state.menu_selected += 1;
    else
        ui_state.menu_selected = 0;
    vp_playMenuBeepIfNeeded(ui_state.menu_selected == 0);
}

static void _ui_menuBack(uint8_t prev_state)
{
    if (ui_state.edit_mode) {
        ui_state.edit_mode = false;
    } else {
        // Return to previous menu
        state.ui_screen = prev_state;
        // Reset menu selection
        ui_state.menu_selected = 0;
        vp_playMenuBeepIfNeeded(true);
    }
}

void ui_fsm_init(void)
{
    last_event_tick = getTick();
    redraw_needed = true;
}

bool ui_fsm_needRedraw(void)
{
    return redraw_needed;
}

void ui_fsm_clearRedraw(void)
{
    redraw_needed = false;
}

bool ui_fsm_is_macro_menu_visible(void)
{
    return macro_menu;
}

bool ui_pushEvent(const uint8_t type, const uint32_t data)
{
    uint8_t newHead = (evQueue_wrPos + 1) % MAX_NUM_EVENTS;

    if (newHead == evQueue_rdPos)
        return false;

    event_t event;
    event.type = type;
    event.payload = data;

    evQueue[evQueue_wrPos] = event;
    evQueue_wrPos = newHead;

    return true;
}

void ui_updateFSM(bool *sync_rtx)
{
    // Check for events
    if (evQueue_wrPos == evQueue_rdPos)
        return;

    // Pop an event from the queue
    uint8_t newTail = (evQueue_rdPos + 1) % MAX_NUM_EVENTS;
    event_t event = evQueue[evQueue_rdPos];
    evQueue_rdPos = newTail;

    // There is some event to process, we need an UI redraw.
    // UI redraw request is cancelled if we're in standby mode.
    redraw_needed = true;
    if (standby)
        redraw_needed = false;

    // Check if battery has enough charge to operate.
    // Check is skipped if there is an ongoing transmission, since the voltage
    // drop caused by the RF PA power absorption causes spurious triggers of
    // the low battery alert.
    bool txOngoing = platform_getPttStatus();
#if !defined(PLATFORM_TTWRPLUS)
    if ((!state.emergency) && (!txOngoing) && (state.charge <= 0)) {
        state.ui_screen = LOW_BAT;
        if (event.type == EVENT_KBD && event.payload) {
            state.ui_screen = MAIN_VFO;
            state.emergency = true;
        }
        return;
    }
#endif // PLATFORM_TTWRPLUS

    // Unlatch and exit from macro menu on PTT press
    if (macro_latched && txOngoing) {
        macro_latched = false;
        macro_menu = false;
    }

    long long now = getTick();
    // Process pressed keys
    if (event.type == EVENT_KBD) {
        kbd_msg_t msg;
        msg.value = event.payload;
        bool f1Handled = false;
        vpQueueFlags_t queueFlags = vp_getVoiceLevelQueueFlags();
        // If we get out of standby, we ignore the kdb event
        // unless is the MONI key for the MACRO functions
        if (_ui_exitStandby(now) && !(msg.keys & KEY_MONI))
            return;
        // If MONI is pressed, activate MACRO functions
        bool moniPressed = msg.keys & KEY_MONI;
        if (moniPressed || macro_latched) {
            macro_menu = true;

            if (state.settings.macroMenuLatch == 1) {
                // long press moni on its own latches function.
                if (moniPressed && msg.long_press && !macro_latched) {
                    macro_latched = true;
                    vp_beep(BEEP_FUNCTION_LATCH_ON, LONG_BEEP);
                } else if (moniPressed && macro_latched) {
                    macro_latched = false;
                    vp_beep(BEEP_FUNCTION_LATCH_OFF, LONG_BEEP);
                }
            }

            _ui_fsm_menuMacro(msg, sync_rtx);
            return;
        } else {
            macro_menu = false;
        }
#if defined(PLATFORM_TTWRPLUS)
        // T-TWR Plus has no KEY_MONI, using KEY_VOLDOWN long press instead
        if ((msg.keys & KEY_VOLDOWN) && msg.long_press) {
            macro_menu = true;
            macro_latched = true;
        }
#endif // PLATFORM_TTWRPLUS

        if (state.tone_enabled && !(msg.keys & KEY_HASH)) {
            state.tone_enabled = false;
            *sync_rtx = true;
        }

        int priorUIScreen = state.ui_screen;
        switch (state.ui_screen) {
            // VFO screen
            case MAIN_VFO: {
                // Enable Tx in MAIN_VFO mode
                if (state.txDisable) {
                    state.txDisable = false;
                    *sync_rtx = true;
                }

                // Break out of the FSM if the keypad is locked but allow the
                // use of the hash key in FM mode for the 1750Hz tone.
                bool skipLock = (state.channel.mode == OPMODE_FM)
                             && (msg.keys == KEY_HASH);

                if ((ui_state.input_locked == true) && (skipLock == false))
                    break;

                if (ui_state.edit_mode) {
#ifdef CONFIG_M17
                    if (state.channel.mode == OPMODE_M17) {
                        if (msg.keys & KEY_ENTER) {
                            _ui_textInputConfirm(&ui_state,
                                                 ui_state.new_callsign);
                            // Save selected dst ID and disable input mode
                            strncpy(state.settings.m17_dest,
                                    ui_state.new_callsign, 10);
                            ui_state.edit_mode = false;
                            *sync_rtx = true;
                            vp_announceM17Info(NULL, ui_state.edit_mode,
                                               queueFlags);
                        } else if (msg.keys & KEY_HASH) {
                            // Save selected dst ID and disable input mode
                            strncpy(state.settings.m17_dest, "", 1);
                            ui_state.edit_mode = false;
                            *sync_rtx = true;
                            vp_announceM17Info(NULL, ui_state.edit_mode,
                                               queueFlags);
                        } else if (msg.keys & KEY_ESC)
                            // Discard selected dst ID and disable input mode
                            ui_state.edit_mode = false;
                        else if (msg.keys & KEY_UP || msg.keys & KEY_DOWN
                                 || msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT)
                            _ui_textInputDel(&ui_state, ui_state.new_callsign);
                        else if (input_isCharPressed(msg))
                            _ui_textInputKeypad(
                                &ui_state, ui_state.new_callsign, 9, msg, true);
                        break;
                    }
#endif
                } else {
                    if (msg.keys & KEY_ENTER) {
                        // Save current main state
                        ui_state.last_main_state = state.ui_screen;
                        // Open Menu
                        state.ui_screen = MENU_TOP;
                        // The selected item will be announced when the item is first selected.
                    } else if (msg.keys & KEY_ESC) {
                        // Save VFO channel
                        state.vfo_channel = state.channel;
                        int result = _ui_fsm_loadChannel(state.channel_index,
                                                         sync_rtx);
                        // Read successful and channel is valid
                        if (result != -1) {
                            // Switch to MEM screen
                            state.ui_screen = MAIN_MEM;
                            // anounce the active channel name.
                            vp_announceChannelName(&state.channel,
                                                   state.channel_index,
                                                   queueFlags);
                        }
                    } else if (msg.keys & KEY_HASH) {
#ifdef CONFIG_M17
                        // Only enter edit mode when using M17
                        if (state.channel.mode == OPMODE_M17) {
                            // Enable dst ID input
                            ui_state.edit_mode = true;
                            // Reset text input variables
                            _ui_textInputReset(&ui_state,
                                               ui_state.new_callsign);
                            vp_announceM17Info(NULL, ui_state.edit_mode,
                                               queueFlags);
                        } else
#endif
                        {
                            if (!state.tone_enabled) {
                                state.tone_enabled = true;
                                *sync_rtx = true;
                            }
                        }
                    } else if (msg.keys & KEY_UP || msg.keys & KNOB_RIGHT) {
                        // Increment TX and RX frequency of 12.5KHz
                        if (_ui_freq_check_limits(
                                state.channel.rx_frequency
                                + freq_steps[state.step_index])
                            && _ui_freq_check_limits(
                                state.channel.tx_frequency
                                + freq_steps[state.step_index])) {
                            state.channel.rx_frequency +=
                                freq_steps[state.step_index];
                            state.channel.tx_frequency +=
                                freq_steps[state.step_index];
                            *sync_rtx = true;
                            vp_announceFrequencies(state.channel.rx_frequency,
                                                   state.channel.tx_frequency,
                                                   queueFlags);
                        }
                    } else if (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT) {
                        // Decrement TX and RX frequency of 12.5KHz
                        if (_ui_freq_check_limits(
                                state.channel.rx_frequency
                                - freq_steps[state.step_index])
                            && _ui_freq_check_limits(
                                state.channel.tx_frequency
                                - freq_steps[state.step_index])) {
                            state.channel.rx_frequency -=
                                freq_steps[state.step_index];
                            state.channel.tx_frequency -=
                                freq_steps[state.step_index];
                            *sync_rtx = true;
                            vp_announceFrequencies(state.channel.rx_frequency,
                                                   state.channel.tx_frequency,
                                                   queueFlags);
                        }
                    } else if (msg.keys & KEY_F1) {
                        if (state.settings.vpLevel
                            > vpBeep) { // quick press repeat vp, long press summary.
                            if (msg.long_press)
                                vp_announceChannelSummary(
                                    &state.channel, 0, state.bank, vpAllInfo);
                            else
                                vp_replayLastPrompt();
                            f1Handled = true;
                        }
                    } else if (input_isNumberPressed(msg)) {
                        // Open Frequency input screen
                        state.ui_screen = MAIN_VFO_INPUT;
                        // Reset input position and selection
                        ui_state.input_position = 1;
                        ui_state.input_set = SET_RX;
                        // do not play  because we will also announce the number just entered.
                        vp_announceInputReceiveOrTransmit(false, vpqInit);
                        vp_queueInteger(input_getPressedNumber(msg));
                        vp_play();

                        ui_state.new_rx_frequency = 0;
                        ui_state.new_tx_frequency = 0;
                        // Save pressed number to calculare frequency and show in GUI
                        ui_state.input_number = input_getPressedNumber(msg);
                        // Calculate portion of the new frequency
                        ui_state.new_rx_frequency = _ui_freq_add_digit(
                            ui_state.new_rx_frequency, ui_state.input_position,
                            ui_state.input_number);
                    }
                }
            } break;
            // VFO frequency input screen
            case MAIN_VFO_INPUT:
                if (msg.keys & KEY_ENTER) {
                    _ui_fsm_confirmVFOInput(sync_rtx);
                } else if (msg.keys & KEY_ESC) {
                    // Cancel frequency input, return to VFO mode
                    state.ui_screen = MAIN_VFO;
                } else if (msg.keys & KEY_UP || msg.keys & KEY_DOWN) {
                    if (ui_state.input_set == SET_RX) {
                        ui_state.input_set = SET_TX;
                        vp_announceInputReceiveOrTransmit(true, queueFlags);
                    } else if (ui_state.input_set == SET_TX) {
                        ui_state.input_set = SET_RX;
                        vp_announceInputReceiveOrTransmit(false, queueFlags);
                    }
                    // Reset input position
                    ui_state.input_position = 0;
                } else if (input_isNumberPressed(msg)) {
                    _ui_fsm_insertVFONumber(msg, sync_rtx);
                }
                break;
            // MEM screen
            case MAIN_MEM:
                // Enable Tx in MAIN_MEM mode
                if (state.txDisable) {
                    state.txDisable = false;
                    *sync_rtx = true;
                }
                if (ui_state.input_locked)
                    break;
                // M17 Destination callsign input
                if (ui_state.edit_mode) {
                    {
                        if (msg.keys & KEY_ENTER) {
                            _ui_textInputConfirm(&ui_state,
                                                 ui_state.new_callsign);
                            // Save selected dst ID and disable input mode
                            strncpy(state.settings.m17_dest,
                                    ui_state.new_callsign, 10);
                            ui_state.edit_mode = false;
                            *sync_rtx = true;
                        } else if (msg.keys & KEY_HASH) {
                            // Save selected dst ID and disable input mode
                            strncpy(state.settings.m17_dest, "", 1);
                            ui_state.edit_mode = false;
                            *sync_rtx = true;
                        } else if (msg.keys & KEY_ESC)
                            // Discard selected dst ID and disable input mode
                            ui_state.edit_mode = false;
                        else if (msg.keys & KEY_F1) {
                            if (state.settings.vpLevel > vpBeep) {
                                // Quick press repeat vp, long press summary.
                                if (msg.long_press) {
                                    vp_announceChannelSummary(
                                        &state.channel, state.channel_index,
                                        state.bank, vpAllInfo);
                                } else {
                                    vp_replayLastPrompt();
                                }

                                f1Handled = true;
                            }
                        } else if (msg.keys & KEY_UP || msg.keys & KEY_DOWN
                                   || msg.keys & KEY_LEFT
                                   || msg.keys & KEY_RIGHT)
                            _ui_textInputDel(&ui_state, ui_state.new_callsign);
                        else if (input_isCharPressed(msg))
                            _ui_textInputKeypad(
                                &ui_state, ui_state.new_callsign, 9, msg, true);
                        break;
                    }
                } else {
                    if (msg.keys & KEY_ENTER) {
                        // Save current main state
                        ui_state.last_main_state = state.ui_screen;
                        // Open Menu
                        state.ui_screen = MENU_TOP;
                    } else if (msg.keys & KEY_ESC) {
                        // Restore VFO channel
                        state.channel = state.vfo_channel;
                        // Update RTX configuration
                        *sync_rtx = true;
                        // Switch to VFO screen
                        state.ui_screen = MAIN_VFO;
                    } else if (msg.keys & KEY_HASH) {
                        // Only enter edit mode when using M17
                        if (state.channel.mode == OPMODE_M17) {
                            // Enable dst ID input
                            ui_state.edit_mode = true;
                            // Reset text input variables
                            _ui_textInputReset(&ui_state,
                                               ui_state.new_callsign);
                        } else {
                            if (!state.tone_enabled) {
                                state.tone_enabled = true;
                                *sync_rtx = true;
                            }
                        }
                    } else if (msg.keys & KEY_F1) {
                        if (state.settings.vpLevel
                            > vpBeep) { // quick press repeat vp, long press summary.
                            if (msg.long_press) {
                                vp_announceChannelSummary(
                                    &state.channel, state.channel_index + 1,
                                    state.bank, vpAllInfo);
                            } else {
                                vp_replayLastPrompt();
                            }

                            f1Handled = true;
                        }
                    } else if (msg.keys & KEY_UP || msg.keys & KNOB_RIGHT) {
                        _ui_fsm_loadChannel(state.channel_index + 1, sync_rtx);
                        vp_announceChannelName(&state.channel,
                                               state.channel_index + 1,
                                               queueFlags);
                    } else if (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT) {
                        _ui_fsm_loadChannel(state.channel_index - 1, sync_rtx);
                        vp_announceChannelName(&state.channel,
                                               state.channel_index + 1,
                                               queueFlags);
                    }
                }
                break;
            // Top menu screen
            case MENU_TOP:
                if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(menu_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(menu_num);
                else if (msg.keys & KEY_ENTER) {
                    switch (ui_state.menu_selected) {
                        case M_BANK:
                            state.ui_screen = MENU_BANK;
                            break;
                        case M_CHANNEL:
                            state.ui_screen = MENU_CHANNEL;
                            break;
                        case M_CONTACTS:
                            state.ui_screen = MENU_CONTACTS;
                            break;
#ifdef CONFIG_GPS
                        case M_GPS:
                            state.ui_screen = MENU_GPS;
                            break;
#endif
                        case M_SETTINGS:
                            state.ui_screen = MENU_SETTINGS;
                            break;
                        case M_INFO:
                            state.ui_screen = MENU_INFO;
                            break;
                        case M_ABOUT:
                            state.ui_screen = MENU_ABOUT;
                            break;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                } else if (msg.keys & KEY_ESC)
                    _ui_menuBack(ui_state.last_main_state);
                break;
            // Zone menu screen
            case MENU_BANK:
            // Channel menu screen
            case MENU_CHANNEL:
            // Contacts menu screen
            case MENU_CONTACTS:
                if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    // Using 1 as parameter disables menu wrap around
                    _ui_menuUp(1);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT) {
                    if (state.ui_screen == MENU_BANK) {
                        bankHdr_t bank;
                        // manu_selected is 0-based
                        // bank 0 means "All Channel" mode
                        // banks (1, n) are mapped to banks (0, n-1)
                        if (cps_readBankHeader(&bank, ui_state.menu_selected)
                            != -1)
                            ui_state.menu_selected += 1;
                    } else if (state.ui_screen == MENU_CHANNEL) {
                        channel_t channel;
                        if (cps_readChannel(&channel,
                                            ui_state.menu_selected + 1)
                            != -1)
                            ui_state.menu_selected += 1;
                    } else if (state.ui_screen == MENU_CONTACTS) {
                        contact_t contact;
                        if (cps_readContact(&contact,
                                            ui_state.menu_selected + 1)
                            != -1)
                            ui_state.menu_selected += 1;
                    }
                } else if (msg.keys & KEY_ENTER) {
                    if (state.ui_screen == MENU_BANK) {
                        bankHdr_t newbank;
                        int result = 0;
                        // If "All channels" is selected, load default bank
                        if (ui_state.menu_selected == 0)
                            state.bank_enabled = false;
                        else {
                            state.bank_enabled = true;
                            result = cps_readBankHeader(
                                &newbank, ui_state.menu_selected - 1);
                        }
                        if (result != -1) {
                            state.bank = ui_state.menu_selected - 1;
                            // If we were in VFO mode, save VFO channel
                            if (ui_state.last_main_state == MAIN_VFO)
                                state.vfo_channel = state.channel;
                            // Load bank first channel
                            _ui_fsm_loadChannel(0, sync_rtx);
                            // Switch to MEM screen
                            state.ui_screen = MAIN_MEM;
                        }
                    }
                    if (state.ui_screen == MENU_CHANNEL) {
                        // If we were in VFO mode, save VFO channel
                        if (ui_state.last_main_state == MAIN_VFO)
                            state.vfo_channel = state.channel;
                        _ui_fsm_loadChannel(ui_state.menu_selected, sync_rtx);
                        // Switch to MEM screen
                        state.ui_screen = MAIN_MEM;
                    }
                } else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
#ifdef CONFIG_GPS
            // GPS menu screen
            case MENU_GPS:
                if ((msg.keys & KEY_F1)
                    && (state.settings.vpLevel
                        > vpBeep)) { // quick press repeat vp, long press summary.
                    if (msg.long_press)
                        vp_announceGPSInfo(vpGPSAll);
                    else
                        vp_replayLastPrompt();
                    f1Handled = true;
                } else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
#endif
            // Settings menu screen
            case MENU_SETTINGS:
                if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_num);
                else if (msg.keys & KEY_ENTER) {
                    switch (ui_state.menu_selected) {
                        case S_DISPLAY:
                            state.ui_screen = SETTINGS_DISPLAY;
                            break;
#ifdef CONFIG_RTC
                        case S_TIMEDATE:
                            state.ui_screen = SETTINGS_TIMEDATE;
                            break;
#endif
#ifdef CONFIG_GPS
                        case S_GPS:
                            state.ui_screen = SETTINGS_GPS;
                            break;
#endif
                        case S_RADIO:
                            state.ui_screen = SETTINGS_RADIO;
                            break;
#ifdef CONFIG_M17
                        case S_M17:
                            state.ui_screen = SETTINGS_M17;
                            break;
#endif
                        case S_FM:
                            state.ui_screen = SETTINGS_FM;
                            break;
                        case S_ACCESSIBILITY:
                            state.ui_screen = SETTINGS_ACCESSIBILITY;
                            break;
                        case S_RESET2DEFAULTS:
                            state.ui_screen = SETTINGS_RESET2DEFAULTS;
                            break;
                        default:
                            state.ui_screen = MENU_SETTINGS;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                } else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
            // Flash backup and restore menu screen
            case MENU_BACKUP_RESTORE:
                if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_num);
                else if (msg.keys & KEY_ENTER) {
                    switch (ui_state.menu_selected) {
                        case BR_BACKUP:
                            state.ui_screen = MENU_BACKUP;
                            break;
                        case BR_RESTORE:
                            state.ui_screen = MENU_RESTORE;
                            break;
                        default:
                            state.ui_screen = MENU_BACKUP_RESTORE;
                    }
                    // Reset menu selection
                    ui_state.menu_selected = 0;
                } else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
            case MENU_BACKUP:
            case MENU_RESTORE:
                if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
            // Info menu screen
            case MENU_INFO:
                if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(info_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(info_num);
                else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
            // About screen, scroll without rollover
            case MENU_ABOUT:
                if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT) {
                    if (ui_state.menu_selected > 0)
                        ui_state.menu_selected -= 1;
                } else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    ui_state.menu_selected += 1;
                else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_TOP);
                break;
#ifdef CONFIG_RTC
            // Time&Date settings screen
            case SETTINGS_TIMEDATE:
                if (msg.keys & KEY_ENTER) {
                    // Switch to set Time&Date mode
                    state.ui_screen = SETTINGS_TIMEDATE_SET;
                    // Reset input position and selection
                    ui_state.input_position = 0;
                    memset(&ui_state.new_timedate, 0, sizeof(datetime_t));
                    vp_announceBuffer(&currentLanguage->timeAndDate, true,
                                      false, "dd/mm/yy");
                } else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
            // Time&Date settings screen, edit mode
            case SETTINGS_TIMEDATE_SET:
                if (msg.keys & KEY_ENTER) {
                    // Save time only if all digits have been inserted
                    if (ui_state.input_position < TIMEDATE_DIGITS)
                        break;
                    // Return to Time&Date menu, saving values
                    // NOTE: The user inserted a local time, we must save an UTC time
                    datetime_t utc_time = localTimeToUtc(
                        ui_state.new_timedate, state.settings.utc_timezone);
                    platform_setTime(utc_time);
                    state.time = utc_time;
                    vp_announceSettingsTimeDate();
                    state.ui_screen = SETTINGS_TIMEDATE;
                } else if (msg.keys & KEY_ESC)
                    _ui_menuBack(SETTINGS_TIMEDATE);
                else if (input_isNumberPressed(msg)) {
                    // Discard excess digits
                    if (ui_state.input_position > TIMEDATE_DIGITS)
                        break;
                    ui_state.input_position += 1;
                    ui_state.input_number = input_getPressedNumber(msg);
                    _ui_timedate_add_digit(&ui_state.new_timedate,
                                           ui_state.input_position,
                                           ui_state.input_number);
                }
                break;
#endif
            case SETTINGS_DISPLAY:
                if (msg.keys & KEY_LEFT
                    || (ui_state.edit_mode
                        && (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT))) {
                    switch (ui_state.menu_selected) {
#ifdef CONFIG_SCREEN_BRIGHTNESS
                        case D_BRIGHTNESS:
                            _ui_changeBrightness(-5);
                            vp_announceSettingsInt(&currentLanguage->brightness,
                                                   queueFlags,
                                                   state.settings.brightness);
                            break;
#endif
#ifdef CONFIG_SCREEN_CONTRAST
                        case D_CONTRAST:
                            _ui_changeContrast(-4);
                            vp_announceSettingsInt(&currentLanguage->brightness,
                                                   queueFlags,
                                                   state.settings.contrast);
                            break;
#endif
                        case D_TIMER:
                            _ui_changeTimer(-1);
                            vp_announceDisplayTimer();
                            break;
                        case D_BATTERY:
                            state.settings.showBatteryIcon =
                                !state.settings.showBatteryIcon;
                            break;
                        default:
                            state.ui_screen = SETTINGS_DISPLAY;
                    }
                } else if (msg.keys & KEY_RIGHT
                           || (ui_state.edit_mode
                               && (msg.keys & KEY_UP
                                   || msg.keys & KNOB_RIGHT))) {
                    switch (ui_state.menu_selected) {
#ifdef CONFIG_SCREEN_BRIGHTNESS
                        case D_BRIGHTNESS:
                            _ui_changeBrightness(+5);
                            vp_announceSettingsInt(&currentLanguage->brightness,
                                                   queueFlags,
                                                   state.settings.brightness);
                            break;
#endif
#ifdef CONFIG_SCREEN_CONTRAST
                        case D_CONTRAST:
                            _ui_changeContrast(+4);
                            vp_announceSettingsInt(&currentLanguage->brightness,
                                                   queueFlags,
                                                   state.settings.contrast);
                            break;
#endif
                        case D_TIMER:
                            _ui_changeTimer(+1);
                            vp_announceDisplayTimer();
                            break;
                        default:
                            state.ui_screen = SETTINGS_DISPLAY;
                    }
                } else if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(display_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(display_num);
                else if (msg.keys & KEY_ENTER)
                    ui_state.edit_mode = !ui_state.edit_mode;
                else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
#ifdef CONFIG_GPS
            case SETTINGS_GPS:
                if (msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT
                    || (ui_state.edit_mode
                        && (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT
                            || msg.keys & KEY_UP || msg.keys & KNOB_RIGHT))) {
                    switch (ui_state.menu_selected) {
                        case G_ENABLED:
                            if (state.settings.gps_enabled)
                                state.settings.gps_enabled = 0;
                            else
                                state.settings.gps_enabled = 1;
                            vp_announceSettingsOnOffToggle(
                                &currentLanguage->gpsEnabled, queueFlags,
                                state.settings.gps_enabled);
                            break;
#ifdef CONFIG_RTC
                        case G_SET_TIME:
                            state.settings.gpsSetTime =
                                !state.settings.gpsSetTime;
                            vp_announceSettingsOnOffToggle(
                                &currentLanguage->gpsSetTime, queueFlags,
                                state.settings.gpsSetTime);
                            break;
                        case G_TIMEZONE:
                            if (msg.keys & KEY_LEFT || msg.keys & KEY_DOWN
                                || msg.keys & KNOB_LEFT)
                                state.settings.utc_timezone -= 1;
                            else if (msg.keys & KEY_RIGHT || msg.keys & KEY_UP
                                     || msg.keys & KNOB_RIGHT)
                                state.settings.utc_timezone += 1;
                            vp_announceTimeZone(state.settings.utc_timezone,
                                                queueFlags);
                            break;
#endif
                        default:
                            state.ui_screen = SETTINGS_GPS;
                    }
                } else if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_gps_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_gps_num);
                else if (msg.keys & KEY_ENTER)
                    ui_state.edit_mode = !ui_state.edit_mode;
                else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
#endif
            // Radio Settings
            case SETTINGS_RADIO:
                // If the entry is selected with enter we are in edit_mode
                if (ui_state.edit_mode) {
                    switch (ui_state.menu_selected) {
                        case R_OFFSET:
                            // Handle offset frequency input
#if defined(CONFIG_UI_NO_KEYBOARD)
                            if (msg.long_press && msg.keys & KEY_ENTER) {
                                // Long press on CONFIG_UI_NO_KEYBOARD causes digits to advance by one
                                ui_state.new_offset /= 10;
#else
                            if (msg.keys & KEY_ENTER) {
#endif
                                // Apply new offset if it is within the hardware
                                // limits of the radio
                                freq_t new_freq = state.channel.rx_frequency
                                                + ui_state.new_offset;
                                if (_ui_freq_check_limits(new_freq)) {
                                    state.channel.tx_frequency = new_freq;
                                    vp_queueStringTableEntry(
                                        &currentLanguage->offset);
                                    vp_queueFrequency(ui_state.new_offset);
                                    ui_state.edit_mode = false;
                                }
                            } else if (msg.keys & KEY_ESC) {
                                // Announce old frequency offset
                                vp_queueStringTableEntry(
                                    &currentLanguage->offset);
                                vp_queueFrequency(
                                    (int32_t)state.channel.tx_frequency
                                    - (int32_t)state.channel.rx_frequency);
                            } else if (msg.keys & KEY_UP || msg.keys & KEY_DOWN
                                       || msg.keys & KEY_LEFT
                                       || msg.keys & KEY_RIGHT) {
                                _ui_numberInputDel(&ui_state,
                                                   &ui_state.new_offset);
                            }
#if defined(CONFIG_UI_NO_KEYBOARD)
                            else if (msg.keys & KNOB_LEFT
                                     || msg.keys & KNOB_RIGHT
                                     || msg.keys & KEY_ENTER)
#else
                            else if (input_isNumberPressed(msg))
#endif
                            {
                                _ui_numberInputKeypad(
                                    &ui_state, &ui_state.new_offset, msg);
                                ui_state.input_position += 1;
                            } else if (msg.long_press && (msg.keys & KEY_F1)
                                       && (state.settings.vpLevel > vpBeep)) {
                                vp_queueFrequency(ui_state.new_offset);
                                f1Handled = true;
                            }
                            break;
                        case R_DIRECTION:
                            if (msg.keys & KEY_UP || msg.keys & KEY_DOWN
                                || msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT
                                || msg.keys & KNOB_LEFT
                                || msg.keys & KNOB_RIGHT) {
                                // Invert frequency offset direction
                                if (state.channel.tx_frequency
                                    >= state.channel.rx_frequency)
                                    state.channel.tx_frequency -=
                                        2
                                        * ((int32_t)state.channel.tx_frequency
                                           - (int32_t)
                                                 state.channel.rx_frequency);
                                else // Switch to positive offset
                                    state.channel.tx_frequency -=
                                        2
                                        * ((int32_t)state.channel.tx_frequency
                                           - (int32_t)
                                                 state.channel.rx_frequency);
                            }
                            break;
                        case R_STEP:
                            if (msg.keys & KEY_UP || msg.keys & KEY_RIGHT
                                || msg.keys & KNOB_RIGHT) {
                                // Cycle over the available frequency steps
                                state.step_index++;
                                state.step_index %= n_freq_steps;
                            } else if (msg.keys & KEY_DOWN
                                       || msg.keys & KEY_LEFT
                                       || msg.keys & KNOB_LEFT) {
                                state.step_index += n_freq_steps;
                                state.step_index--;
                                state.step_index %= n_freq_steps;
                            }
                            break;
                        default:
                            state.ui_screen = SETTINGS_RADIO;
                    }
                    // If ENTER or ESC are pressed, exit edit mode, R_OFFSET is managed separately
                    if ((ui_state.menu_selected != R_OFFSET
                         && msg.keys & KEY_ENTER)
                        || msg.keys & KEY_ESC)
                        ui_state.edit_mode = false;
                } else if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_radio_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_radio_num);
                else if (msg.keys & KEY_ENTER) {
                    ui_state.edit_mode = true;
                    // If we are entering R_OFFSET clear temp offset
                    if (ui_state.menu_selected == R_OFFSET)
                        ui_state.new_offset = 0;
                    // Reset input position
                    ui_state.input_position = 0;
                } else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
#ifdef CONFIG_M17
            // M17 Settings
            case SETTINGS_M17:
                if (ui_state.edit_mode) {
                    switch (ui_state.menu_selected) {
                        case M17_CALLSIGN:
                            // Handle text input for M17 callsign
                            if (msg.keys & KEY_ENTER) {
                                _ui_textInputConfirm(&ui_state,
                                                     ui_state.new_callsign);
                                // Save selected callsign and disable input mode
                                strncpy(state.settings.callsign,
                                        ui_state.new_callsign, 10);
                                ui_state.edit_mode = false;
                                vp_announceBuffer(&currentLanguage->callsign,
                                                  false, true,
                                                  state.settings.callsign);
                            } else if (msg.keys & KEY_ESC) {
                                // Discard selected callsign and disable input mode
                                ui_state.edit_mode = false;
                                vp_announceBuffer(&currentLanguage->callsign,
                                                  false, true,
                                                  state.settings.callsign);
                            } else if (msg.keys & KEY_UP || msg.keys & KEY_DOWN
                                       || msg.keys & KEY_LEFT
                                       || msg.keys & KEY_RIGHT) {
                                _ui_textInputDel(&ui_state,
                                                 ui_state.new_callsign);
                            } else if (input_isCharPressed(msg)) {
                                _ui_textInputKeypad(&ui_state,
                                                    ui_state.new_callsign, 9,
                                                    msg, true);
                            } else if (msg.long_press && (msg.keys & KEY_F1)
                                       && (state.settings.vpLevel > vpBeep)) {
                                vp_announceBuffer(&currentLanguage->callsign,
                                                  true, true,
                                                  ui_state.new_callsign);
                                f1Handled = true;
                            }
                            break;
                        case M17_METATEXT:
                            // Handle text input for M17 message text
                            if (msg.keys & KEY_ENTER) {
                                _ui_textInputConfirm(&ui_state,
                                                     ui_state.new_message);
                                // Save selected message and disable input mode
                                strncpy(state.settings.M17_meta_text,
                                        ui_state.new_message, 52);
                                ui_state.edit_message = false;
                                ui_state.edit_mode = false;
                                vp_announceBuffer(&currentLanguage->metaText,
                                                  false, true,
                                                  state.settings.M17_meta_text);
                            } else if (msg.keys & KEY_ESC) {
                                // Discard selected message and disable input mode
                                ui_state.edit_message = false;
                                ui_state.edit_mode = false;
                                vp_announceBuffer(&currentLanguage->metaText,
                                                  false, true,
                                                  state.settings.M17_meta_text);
                            } else if (msg.keys & KEY_UP || msg.keys & KEY_DOWN
                                       || msg.keys & KEY_LEFT
                                       || msg.keys & KEY_RIGHT) {
                                _ui_textInputDel(&ui_state,
                                                 ui_state.new_message);
                            } else if (input_isCharPressed(msg)) {
                                _ui_textInputKeypad(&ui_state,
                                                    ui_state.new_message, 52,
                                                    msg, false);
                            } else if (msg.long_press && (msg.keys & KEY_F1)
                                       && (state.settings.vpLevel > vpBeep)) {
                                vp_announceBuffer(&currentLanguage->metaText,
                                                  true, true,
                                                  ui_state.new_message);
                                f1Handled = true;
                            }
                            break;
                        case M17_CAN:
                            if (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT)
                                _ui_changeM17Can(-1);
                            else if (msg.keys & KEY_UP || msg.keys & KNOB_RIGHT)
                                _ui_changeM17Can(+1);
                            else if (msg.keys & KEY_ENTER)
                                ui_state.edit_mode = !ui_state.edit_mode;
                            else if (msg.keys & KEY_ESC)
                                ui_state.edit_mode = false;
                            break;
                        case M17_CAN_RX:
                            if (msg.keys & KEY_LEFT || msg.keys & KEY_RIGHT
                                || (ui_state.edit_mode
                                    && (msg.keys & KEY_DOWN
                                        || msg.keys & KNOB_LEFT
                                        || msg.keys & KEY_UP
                                        || msg.keys & KNOB_RIGHT))) {
                                state.settings.m17_can_rx =
                                    !state.settings.m17_can_rx;
                            } else if (msg.keys & KEY_ENTER)
                                ui_state.edit_mode = !ui_state.edit_mode;
                            else if (msg.keys & KEY_ESC)
                                ui_state.edit_mode = false;
                    }
                } else {
                    if (msg.keys & KEY_ENTER) {
                        // Enable edit mode
                        ui_state.edit_mode = true;

                        // If callsign input, reset text input variables
                        if (ui_state.menu_selected == M17_CALLSIGN) {
                            _ui_textInputReset(&ui_state,
                                               ui_state.new_callsign);
                            vp_announceBuffer(&currentLanguage->callsign, true,
                                              true, ui_state.new_callsign);
                        }
                        // If message input, reset text input variables
                        if (ui_state.menu_selected == M17_METATEXT) {
                            //   ui_state.edit_mode = false;
                            ui_state.edit_message = true;
                            _ui_textInputReset(&ui_state, ui_state.new_message);
                            vp_announceBuffer(&currentLanguage->metaText, true,
                                              true, ui_state.new_message);
                        }
                    } else if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                        _ui_menuUp(settings_m17_num);
                    else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                        _ui_menuDown(settings_m17_num);
                    else if ((msg.keys & KEY_RIGHT)
                             && (ui_state.menu_selected == M17_CAN))
                        _ui_changeM17Can(+1);
                    else if ((msg.keys & KEY_LEFT)
                             && (ui_state.menu_selected == M17_CAN))
                        _ui_changeM17Can(-1);
                    else if (msg.keys & KEY_ESC) {
                        *sync_rtx = true;
                        _ui_menuBack(MENU_SETTINGS);
                    }
                }
                break;
#endif
            case SETTINGS_FM:
                if (ui_state.edit_mode) {
                    if (msg.keys & KEY_ESC)
                        ui_state.edit_mode = false;

                    switch (ui_state.menu_selected) {
                        case CTCSS_Tone:
                            if (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT) {
                                if (state.channel.fm.txTone == 0) {
                                    state.channel.fm.txTone = CTCSS_FREQ_NUM
                                                            - 1;
                                } else {
                                    state.channel.fm.txTone--;
                                }
                            } else if (msg.keys & KEY_UP
                                       || msg.keys & KNOB_RIGHT) {
                                state.channel.fm.txTone++;
                            } else if (msg.keys & KEY_ENTER) {
                                ui_state.edit_mode = false;
                            }
                            state.channel.fm.txTone %= CTCSS_FREQ_NUM;
                            state.channel.fm.rxTone = state.channel.fm.txTone;
                            *sync_rtx = true;
                            break;
                        case CTCSS_Enabled:
                            if (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT) {
                                _ui_handleToneSelectScroll(true);
                            } else if (msg.keys & KEY_UP
                                       || msg.keys & KNOB_RIGHT) {
                                _ui_handleToneSelectScroll(false);
                            } else if (msg.keys & KEY_ENTER) {
                                ui_state.edit_mode = false;
                            }

                            *sync_rtx = true;
                            break;
                    }
                } else if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_fm_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_fm_num);
                else if (msg.keys & KEY_ENTER)
                    ui_state.edit_mode = !ui_state.edit_mode;
                else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                else if (msg.keys & KEY_ENTER)
                    ui_state.edit_mode = !ui_state.edit_mode;
                else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;

            case SETTINGS_ACCESSIBILITY:
                if (msg.keys & KEY_LEFT
                    || (ui_state.edit_mode
                        && (msg.keys & KEY_DOWN || msg.keys & KNOB_LEFT))) {
                    switch (ui_state.menu_selected) {
                        case A_MACRO_LATCH:
                            _ui_changeMacroLatch(false);
                            break;
                        case A_LEVEL:
                            _ui_changeVoiceLevel(-1);
                            break;
                        case A_PHONETIC:
                            _ui_changePhoneticSpell(false);
                            break;
                        default:
                            state.ui_screen = SETTINGS_ACCESSIBILITY;
                    }
                } else if (msg.keys & KEY_RIGHT
                           || (ui_state.edit_mode
                               && (msg.keys & KEY_UP
                                   || msg.keys & KNOB_RIGHT))) {
                    switch (ui_state.menu_selected) {
                        case A_MACRO_LATCH:
                            _ui_changeMacroLatch(true);
                            break;
                        case A_LEVEL:
                            _ui_changeVoiceLevel(1);
                            break;
                        case A_PHONETIC:
                            _ui_changePhoneticSpell(true);
                            break;
                        default:
                            state.ui_screen = SETTINGS_ACCESSIBILITY;
                    }
                } else if (msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
                    _ui_menuUp(settings_accessibility_num);
                else if (msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
                    _ui_menuDown(settings_accessibility_num);
                else if (msg.keys & KEY_ENTER)
                    ui_state.edit_mode = !ui_state.edit_mode;
                else if (msg.keys & KEY_ESC)
                    _ui_menuBack(MENU_SETTINGS);
                break;
            case SETTINGS_RESET2DEFAULTS:
                if (!ui_state.edit_mode) {
                    //require a confirmation ENTER, then another
                    //edit_mode is slightly misused to allow for this
                    if (msg.keys & KEY_ENTER) {
                        ui_state.edit_mode = true;
                    } else if (msg.keys & KEY_ESC) {
                        _ui_menuBack(MENU_SETTINGS);
                    }
                } else {
                    if (msg.keys & KEY_ENTER) {
                        ui_state.edit_mode = false;
                        state_resetSettingsAndVfo();
                        _ui_menuBack(MENU_SETTINGS);
                    } else if (msg.keys & KEY_ESC) {
                        ui_state.edit_mode = false;
                        _ui_menuBack(MENU_SETTINGS);
                    }
                }
                break;
        }

        // Enable Tx only if in MAIN_VFO or MAIN_MEM states
        bool inMemOrVfo = (state.ui_screen == MAIN_VFO)
                       || (state.ui_screen == MAIN_MEM);
        if ((macro_menu == true)
            || ((inMemOrVfo == false) && (state.txDisable == false))) {
            state.txDisable = true;
            *sync_rtx = true;
        }
        if (!f1Handled && (msg.keys & KEY_F1)
            && (state.settings.vpLevel > vpBeep)) {
            vp_replayLastPrompt();
        } else if ((priorUIScreen != state.ui_screen)
                   && state.settings.vpLevel > vpLow) {
            // When we switch to VFO or Channel screen, we need to announce it.
            // Likewise for information screens.
            // All other cases are handled as needed.
            vp_announceScreen(state.ui_screen);
        }
        // generic beep for any keydown if beep is enabled.
        // At vp levels higher than beep, keys will generate voice so no need
        // to beep or you'll get an unwanted click.
        if ((msg.keys & 0xffff) && (state.settings.vpLevel == vpBeep))
            vp_beep(BEEP_KEY_GENERIC, SHORT_BEEP);
        // If we exit and re-enter the same menu, we want to ensure it speaks.
        if (msg.keys & KEY_ESC)
            _ui_reset_menu_anouncement_tracking();
    } else if (event.type == EVENT_STATUS) {
#ifdef CONFIG_GPS
        if ((state.ui_screen == MENU_GPS) && (!vp_isPlaying())
            && (state.settings.vpLevel > vpLow)
            && (!txOngoing
                && !rtx_rxSquelchOpen())) { // automatically read speed and direction changes only!
            vpGPSInfoFlags_t whatChanged = GetGPSDirectionOrSpeedChanged();
            if (whatChanged != vpGPSNone)
                vp_announceGPSInfo(whatChanged);
        }
#endif //            CONFIG_GPS

        if (txOngoing || rtx_rxSquelchOpen()
            || (state.volume != last_state.volume)) {
            _ui_exitStandby(now);
            return;
        }

        if (_ui_checkStandby(now - last_event_tick)) {
            _ui_enterStandby();
        }
    }
}
