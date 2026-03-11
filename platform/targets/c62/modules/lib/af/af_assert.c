/* QP的AF_onAssert示例实现

void AF_onAssert(char_t const * const module, int_t loc) {
    QS_BEGIN_NOCRIT_(QS_ASSERT_FAIL, (void *)0, (void *)0)
        QS_TIME_();
        QS_U16_((uint16_t)loc);
        QS_STR_((module != (char_t *)0) ? module : "?");
    QS_END_NOCRIT_()
    QS_onFlush(); // flush the assertion record to the host
    QS_onTestLoop(); // loop to wait for commands (typically reset)
    QS_onReset(); // in case the QUTEST loop ever returns, reset manually
}
*/

// 最简陋实现: while(1);

// #include <nds32_intrinsic.h>

#include "af_assert.h"

#include "AudioTrack_config.h"

#if defined(PLATFORM_PC)

    #include <assert.h>

    void AF_onAssert(char_t const * const module, int_t loc) {
        (void) module;
        (void) loc;

        assert(1);
    }

#elif defined(PLATFORM_RTOS)

    void AF_onAssert(char_t const * const module, int_t loc) {
        (void) module;
        (void) loc;

        // taskDisableInterrupt();
        // taskStopScheduler();

        while(1);

        // __nds32__break(0x2C);
    }

#endif // PLATFORM_XXX
