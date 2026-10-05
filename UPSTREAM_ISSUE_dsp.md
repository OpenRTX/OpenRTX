# Upstream issue: UBSan left shift of negative value in `dsp_dcBlockFilter`

Do not fix this in the fork. File against OpenRTX/OpenRTX.

## Reproduced on

- Tree: `upstream/master` at `34052ea7` (OpenRTX version 0.4.5)
- Worktree: clean checkout, no Horse patches
- Host: Ubuntu, gcc 13.3.0
- Command:

```bash
meson setup build_asan -Db_sanitize=address,undefined
meson compile -C build_asan m17_demodulator_test
meson test -C build_asan "M17 Demodulator Test" --print-errorlogs
```

Without sanitizers the same test is green. With UBSan it aborts.

## Sanitizer trace (verbatim)

```
../openrtx/src/core/dsp.cpp:19:48: runtime error: left shift of negative value -60
    #0 ... in dsp_dcBlockFilter ../openrtx/src/core/dsp.cpp:19
    #1 ... in M17::Demodulator::sample(short, bool) ../openrtx/src/protocols/M17/Demodulator.cpp:217
    #2 ... in CATCH2_INTERNAL_TEST_16 ../tests/unit/M17_demodulator.cpp:264

SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior ../openrtx/src/core/dsp.cpp:19:48 in
```

Catch2 reports:

```
Demodulator maintains lock across multiple consecutive stream frames
../tests/unit/M17_demodulator.cpp:228
FAILED: SIGABRT
test cases:  8 |  7 passed | 1 failed
```

## Minimal reproduction

The failing line is:

```c
dcb->prevIn = static_cast<int32_t>(sample) << 15;
```

`sample` is an `int16_t`. A left shift of a negative signed value is undefined behaviour in C and C++. The Catch2 case feeds baseband that includes negative samples (here `-60`).

A one-file check:

```c
#include <stdint.h>
int32_t f(int16_t sample) { return (int32_t)sample << 15; }
```

compiled with `-fsanitize=undefined` and called as `f(-60)` produces the same diagnostic.

## Suggested fix (for upstream)

Scale with a multiply, which is defined for this range (`sample * 32768` fits in `int32_t`):

```c
dcb->prevIn = (int32_t)sample * 32768;
```

Alternatively shift the bit pattern as unsigned:

```c
dcb->prevIn = (int32_t)((uint32_t)(int32_t)sample << 15);
```

The multiply form matches the comment (`32768.0 * ...`) and is easier to review.

Do not change `dsp.cpp` in this fork; keep Horse tests passing without sanitizers, and expect "M17 Demodulator Test" to abort under UBSan until upstream lands a fix.
