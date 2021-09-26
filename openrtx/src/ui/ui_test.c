/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>
#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <interfaces/platform.h>
#include <interfaces/audio_stream.h>
#include <interfaces/radio.h>
#include <ui.h>
#include <rtx.h>
#include <hwconfig.h>

static const size_t numSamples = 35*1024;       // 70kB

enum recStateEnum
{
    REC_STANDBY = 0,
    REC_RECORDING,
    REC_PLAYBACK
};

uint8_t rec_state = 0;
streamId micId = 0;

/* UI general helper functions, their implementation is in "ui.c" */
extern void _ui_menuUp(uint8_t menu_entries);
extern void _ui_menuDown(uint8_t menu_entries);
extern void _ui_menuBack(uint8_t menu_entries);
/* UI main screen helper functions, their implementation is in "ui_main.c" */
extern void _ui_drawMainTop();
extern void _ui_drawMainBottom();
/* UI menu screen helper functions, their implementation is in "ui_menu.c" */
extern void _ui_drawMenuList(uint8_t selected, int (*getCurrentEntry)(char *buf, uint8_t max_len, uint8_t index));

void _ui_eventMenuTestMic(kbd_msg_t msg, bool *sync_rtx)
{
    switch(rec_state)
    {
        case REC_STANDBY:
        if(msg.keys & KEY_ENTER)
        {
            rec_state = REC_RECORDING;
            // Start recording mic audio to buffer
            /*uint16_t *sampleBuf = ((uint16_t *) malloc(numSamples * sizeof(uint16_t)));
            micId = inputStream_start(SOURCE_MIC, PRIO_RX, sampleBuf, numSamples,
                                               BUF_LINEAR, 8000);
            */
            platform_ledOn(RED);
            // Call getData once to start recording
            //inputStream_getDataNonBlocking(micId);
        }
        break;
        case REC_RECORDING:
        if(msg.keys & KEY_ESC)
        {
            rec_state = REC_STANDBY;
            inputStream_stop(micId);
            platform_ledOff(RED);
        }
        // Poll inputStream until the buffer has been filled
        //dataBlock_t audio = inputStream_getData(micId);
        // Recording is complete
        /*if(audio.data != NULL)
        {
            rec_state = REC_PLAYBACK;
            // Close inputStream
            inputStream_stop(micId);
            platform_ledOff(RED);
            // Process recorded audio
            // Pre-amplification stage
            for(size_t i = 0; i < audio.len; i++) audio.data[i] <<= 3;
            // DC removal
            dsp_dcRemoval(audio.data, audio.len);
            // Post-amplification stage
            for(size_t i = 0; i < audio.len; i++) audio.data[i] *= 20;
            // Start audio playback
            _bufferPlaybackStart(audio.data);
            platform_ledOn(GREEN);

        }*/
        break;
        case REC_PLAYBACK:
        if(msg.keys & KEY_ESC)
        {
            rec_state = REC_STANDBY;
            platform_ledOff(GREEN);
            //_bufferPlaybackStop();
        }
        break;
    }
}

void _ui_eventMenuTestRF(kbd_msg_t msg, bool *sync_rtx)
{
}

void _ui_eventMenuTest(ui_state_t* ui_state, event_t event, bool *sync_rtx)
{
    // Handle only keyboard events
    if(event.type != EVENT_KBD)
        return;
    
    kbd_msg_t msg;
    msg.value = event.payload;
    
    // Test selection mode
    if(ui_state->input_set == 0)
    {
        if(msg.keys & KEY_UP || msg.keys & KNOB_LEFT)
            _ui_menuUp(test_num);
        else if(msg.keys & KEY_DOWN || msg.keys & KNOB_RIGHT)
            _ui_menuDown(test_num);
        else if(msg.keys & KEY_ENTER)
            // Execute selected test
            ui_state->input_set = 1;
        else if(msg.keys & KEY_ESC)
            _ui_menuBack(MENU_TOP);
    }
    // Test execution mode
    else
    {
        // Disable RTX
        rtx_terminate();
        switch(ui_state->menu_selected)
        {
            case 0:
                _ui_eventMenuTestMic(msg, sync_rtx);
            break;
            case 1:
                _ui_eventMenuTestRF(msg, sync_rtx);
            break;
        }
        if(msg.keys & KEY_ESC)
            // Return to test choice
            ui_state->input_set = 0;
    }
}

void _ui_drawMenuTestMic(ui_state_t* ui_state)
{
    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "Mic->MCU Test");
    gfx_printLine(1, 1, layout.top_h, SCREEN_HEIGHT - layout.bottom_h, layout.horizontal_pad, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "State: %d", rec_state);
    // Draw microphone input level
    uint16_t meter_width = SCREEN_WIDTH - 2 * layout.horizontal_pad;
    uint16_t meter_height = layout.bottom_h;
    point_t meter_pos = { layout.horizontal_pad,
                          SCREEN_HEIGHT - meter_height - layout.bottom_pad};
    gfx_drawSmeterLevel(meter_pos,
                    meter_width,
                    meter_height,
                    -127,
                    255);
}

void _ui_drawMenuTestRF(ui_state_t* ui_state)
{
    gfx_clearScreen();
    gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
              color_white, "RF->MCU Test");
    // Draw RF input level
    uint16_t meter_width = SCREEN_WIDTH - 2 * layout.horizontal_pad;
    uint16_t meter_height = layout.bottom_h;
    point_t meter_pos = { layout.horizontal_pad,
                          SCREEN_HEIGHT - meter_height - layout.bottom_pad};
    gfx_drawSmeterLevel(meter_pos,
                    meter_width,
                    meter_height,
                    -127,
                    255);
}

int _ui_getTestEntryName(char *buf, uint8_t max_len, uint8_t index)
{
    if(index >= test_num) return -1;
    snprintf(buf, max_len, "%s", test_items[index]);
    return 0;
}

void _ui_drawMenuTest(ui_state_t* ui_state)
{
    // Test choice
    if(ui_state->input_set == 0)
    {
        gfx_clearScreen();
        // Print "Test" on top bar
        gfx_print(layout.top_pos, layout.top_font, TEXT_ALIGN_CENTER,
                  color_white, "Test");
        // Print menu entries
        _ui_drawMenuList(ui_state->menu_selected, _ui_getTestEntryName);
    }
    // Test execution
    else
    {
        switch(ui_state->menu_selected)
        {
            case 0:
                _ui_drawMenuTestMic(ui_state);
            break;
            case 1:
                _ui_drawMenuTestRF(ui_state);
            break;
        }
    }

}
