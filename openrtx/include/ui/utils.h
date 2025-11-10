/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Number of extra ticks to pause at the beginning and end of scroll */
#define SCROLL_PAUSE_TICKS 5

/**
 * Convert scroll position (with pauses) to actual display offset.
 * @param scrollPosition The current scroll position (includes pause ticks).
 * @param maxPos The maximum scroll offset (bufLen - window).
 * @return The actual display offset to use.
 */
static inline size_t scrollTextGetDisplayPos(size_t scrollPosition,
                                             size_t maxPos)
{
    if (scrollPosition < SCROLL_PAUSE_TICKS) {
        /* Pause at beginning - show start of text */
        return 0;
    } else if (scrollPosition <= SCROLL_PAUSE_TICKS + maxPos) {
        /* Scrolling - return actual scroll offset (0 to maxPos inclusive) */
        return scrollPosition - SCROLL_PAUSE_TICKS;
    } else {
        /* Pause at end - show end of text */
        return maxPos;
    }
}

/**
 * Get the current scroll window without advancing the position.
 * @param buf The input buffer containing the text to scroll.
 * @param printBuf The output buffer where the scrolled text will be stored.
 * @param printBufSize Size of printBuf in bytes (including room for the terminating NUL).
 * @param scrollPosition The current scroll position (includes pause ticks).
 * @return true if the text requires scrolling, false otherwise.
 */
static inline bool scrollTextPeek(const char *buf, char *printBuf,
                                  size_t printBufSize, size_t scrollPosition)
{
    if (printBufSize <= 1) {
        if (printBufSize == 1)
            printBuf[0] = '\0';
        return false;
    }

    size_t window = printBufSize - 1;
    size_t bufLen = strlen(buf);

    /* no scrolling needed */
    if (bufLen == 0) {
        printBuf[0] = '\0';
        return false;
    }
    if (bufLen <= window) {
        memcpy(printBuf, buf, bufLen);
        printBuf[bufLen] = '\0';
        return false;
    }

    /* scrolling needed - convert position accounting for pauses */
    size_t maxPos = bufLen - window;
    size_t displayPos = scrollTextGetDisplayPos(scrollPosition, maxPos);
    memcpy(printBuf, buf + displayPos, window);
    printBuf[window] = '\0';
    return true;
}

/**
 * Advance the scroll position for text that requires scrolling.
 * Pauses at the beginning and end for SCROLL_PAUSE_TICKS intervals.
 * @param buf The input buffer containing the text to scroll.
 * @param printBufSize Size of printBuf in bytes (including room for the terminating NUL).
 * @param scrollPosition Pointer to the current scroll position. This will be updated.
 *        Position 0 to SCROLL_PAUSE_TICKS-1 = pause at start (display pos 0)
 *        Position SCROLL_PAUSE_TICKS to SCROLL_PAUSE_TICKS+maxPos = scrolling (maxPos+1 steps)
 *        Position SCROLL_PAUSE_TICKS+maxPos+1 to end = pause at end
 */
static inline void scrollTextAdvance(const char *buf, size_t printBufSize,
                                     size_t *scrollPosition)
{
    if (printBufSize <= 1) {
        return;
    }

    size_t window = printBufSize - 1;

    size_t bufLen = strlen(buf);
    if (bufLen <= window) {
        *scrollPosition = 0;
        return;
    }

    /* scrolling needed */
    size_t maxPos = bufLen - window;
    /* Total positions: pause at start + scroll range (maxPos+1 steps) + pause at end */
    size_t totalPositions = SCROLL_PAUSE_TICKS + (maxPos + 1)
                          + SCROLL_PAUSE_TICKS;

    (*scrollPosition)++;
    if (*scrollPosition >= totalPositions) {
        *scrollPosition = 0;
    }
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UI_UTILS_H */
