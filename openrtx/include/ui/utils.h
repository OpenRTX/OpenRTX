
#ifndef UI_UTILS_H
#define UI_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
    * Scroll text in a buffer. The text scrolls left to right. It does not wrap, but instead resets, when reaching the end.
    * @param buf The input buffer containing the text to scroll.
    * @param printBuf The output buffer where the scrolled text will be stored.
    * @param printBufSize Size of printBuf in bytes (including room for the terminating NUL).
    * @param scrollPosition Pointer to the current scroll position. This will be updated.
    * @return true if scrolling was performed, false otherwise.
 */
static inline bool scrollText(const char *buf, char *printBuf, size_t printBufSize, size_t *scrollPosition)
{
    if (buf == NULL || printBuf == NULL || scrollPosition == NULL || printBufSize == 0) {
        return false;
    }

    /* usable window length (space for characters, excluding NUL) */
    size_t window = printBufSize - 1;

    /* handle empty window */
    if (window == 0) {
        printBuf[0] = '\0';
        return false;
    }

    size_t bufLen = strlen(buf);

    /* no scrolling needed: copy text (and pad with spaces to keep stable width) */
    if (bufLen == 0) {
        /* empty source */
        memset(printBuf, ' ', window);
        printBuf[0] = '\0';
        *scrollPosition = 0;
        return false;
    }
    if (bufLen <= window) {
        memcpy(printBuf, buf, bufLen);
        if (bufLen < window) {
            memset(printBuf + bufLen, ' ', window - bufLen);
        }
        printBuf[window] = '\0';
        *scrollPosition = 0;
        return false;
    }

    /* scrolling needed */
    size_t maxPos = bufLen - window; /* last valid starting index */

    /* advance position and wrap when past the last position */
    (*scrollPosition)++;
    if (*scrollPosition > maxPos) {
        *scrollPosition = 0;
    }

    /* copy the current window */
    memcpy(printBuf, buf + *scrollPosition, window);
    printBuf[window] = '\0';
    return true;
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* UI_UTILS_H */