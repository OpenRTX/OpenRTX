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
