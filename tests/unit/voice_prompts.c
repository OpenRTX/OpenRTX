/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Test private methods
#define private public

#include "core/state.h"
#include "core/voicePromptUtils.h"

/**
 * Test voice prompts playback
 */

int main()
{
    state.settings.vpLevel = 3;
	vpQueueFlags_t flags = vp_getVoiceLevelQueueFlags();

    vp_init();
    vp_flush();
    vp_queueStringTableEntry(&currentLanguage->allChannels);
    vp_play();
    while(true)
    {
        vp_tick();
    }
}
