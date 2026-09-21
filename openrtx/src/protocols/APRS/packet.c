/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "protocols/APRS/packet.h"
#include "protocols/APRS/constants.h"

struct aprsPacket *aprsPktFromFrame(const uint8_t *data, const size_t len)
{
    uint8_t frameOffset = 0;
    bool lastAddress = false;

    DEBUG_PRINT("aprsPktFromFrame frame: ");
    for (uint8_t i = 0; i < len; i++) {
        DEBUG_PRINT("%02x ", data[i]);
    }
    DEBUG_PRINT("\n");
    DEBUG_PRINT("frame len=%ld\n", len);

    /* calculate the size of the addresses */
    uint8_t addressesLen = 0;
    while (((len - frameOffset) >= 7) && !lastAddress) {
        addressesLen++;
        lastAddress = data[frameOffset + 6] & 1;
        frameOffset += 7;
    }

    DEBUG_PRINT("addressesLen=%d frameOffset=%d\n", addressesLen, frameOffset);

    /* skip the control field and protocol ID */
    frameOffset += 2;

    DEBUG_PRINT("frameOffset=%d\n", frameOffset);

    /* calculate the size of the info section excluding CRC at the end */
    uint8_t infoLen = (len - 2) - frameOffset + 1;

    DEBUG_PRINT("infoLen=%d\n", infoLen);

    /* allocate memory for a new packet and initialize */
    size_t pktLen = sizeof(struct aprsPacket)
                  + (sizeof(struct aprsAddress) * addressesLen) + infoLen;
    struct aprsPacket *pkt = (struct aprsPacket *)malloc(pktLen);
    if (!pkt)
        return NULL;

    pkt->next = NULL;
    pkt->prev = NULL;
#ifdef CONFIG_RTC
    pkt->ts = platform_getCurrentTime();
#else
    pkt->ts.hour = 0;
    pkt->ts.minute = 0;
    pkt->ts.second = 0;
    pkt->ts.day = 0;
    pkt->ts.date = 0;
    pkt->ts.month = 0;
    pkt->ts.year = 0;
#endif
    // pointer to the variable length part of the struct
    uint8_t *endPtr = (uint8_t *)pkt + sizeof(struct aprsPacket);
    pkt->addressesLen = addressesLen;
    pkt->addresses = (struct aprsAddress *)endPtr;
    pkt->infoLen = infoLen;
    pkt->info = (char *)(endPtr + sizeof(struct aprsAddress) * addressesLen);

    /* copy address from front of frame data into our packet addresses */
    frameOffset = 0;
    lastAddress = false;
    uint8_t addressIndex = 0;
    /* addresses are 7 bytes long */
    while (((len - frameOffset) >= 7) && !lastAddress) {
        memset(pkt->addresses[addressIndex].addr, 0, 7);
        uint8_t j = 0;
        for (uint8_t i = 0; i < 6; i++) {
            /* address characters are ASCII that has been left shifted once */
            uint8_t character = data[frameOffset + i] >> 1;
            /* include only printable characters */
            if ((character > 32) && (character < 127))
                pkt->addresses[addressIndex].addr[j++] = character;
        }
        /* the last byte is an SSID and some flags */
        pkt->addresses[addressIndex].ssid = (data[frameOffset + 6] & 30) >> 1;
        pkt->addresses[addressIndex].commandHeard =
            (data[frameOffset + 6] & 0x80) >> 7;
        lastAddress = data[frameOffset + 6] & 1;
        frameOffset += 7;
        addressIndex++;
    }

    DEBUG_PRINT("frameOffset=%d\n", frameOffset);

    /* skip the control field and protocol ID */
    frameOffset += 2;

    DEBUG_PRINT("frameOffset=%d\n", frameOffset);

    /* copy info into the packet */
    memcpy(pkt->info, data + frameOffset, infoLen - 1);
    pkt->info[infoLen - 1] = '\0';

    return pkt;
}

/* ------------------------------------------------------------------------ *
 * Frame construction — the inverse of aprsPktFromFrame().
 * ------------------------------------------------------------------------ */

/* An AX.25 address is seven bytes: six shifted-ASCII characters plus a flags
 * byte laid out as CRRSSSSL — command/response, two reserved bits sent as
 * ones, the four-bit SSID, and the last-address marker. */
#define ADDR_LEN 7
#define ADDR_LAST 0x01
#define ADDR_SSID_SHIFT 1
#define ADDR_COMMAND 0x80
#define ADDR_RESERVED 0x60

/**
 * Upper-case one ASCII letter; anything else is returned unchanged.
 */
static char aprsUpcase(char c)
{
    if ((c >= 'a') && (c <= 'z'))
        return (char)(c - 'a' + 'A');
    return c;
}

/**
 * Shift one address into the seven bytes AX.25 wants, setting the flag bits
 * the caller asks for.
 */
static void aprsWriteAddress(uint8_t *dst, const struct aprsAddress *addr,
                             bool command, bool last)
{
    size_t i = 0;

    /* Callsigns shorter than six characters are padded with spaces, which
     * shift to 0x40 exactly where the parser drops them again. */
    for (; (i < ADDR_LEN - 1) && (addr->addr[i] != '\0'); i++)
        dst[i] = (uint8_t)(addr->addr[i] << 1);
    for (; i < ADDR_LEN - 1; i++)
        dst[i] = (uint8_t)(' ' << 1);

    /* The two reserved bits are transmitted as ones, which is what every
     * other station sends and what the parser ignores. */
    dst[ADDR_LEN - 1] = ADDR_RESERVED;
    dst[ADDR_LEN - 1] |= (uint8_t)((addr->ssid & 0x0f) << ADDR_SSID_SHIFT);

    if (command)
        dst[ADDR_LEN - 1] |= ADDR_COMMAND;
    if (last)
        dst[ADDR_LEN - 1] |= ADDR_LAST;
}

bool aprsAddrFromStr(const char *str, struct aprsAddress *addr)
{
    if ((str == NULL) || (addr == NULL))
        return false;

    struct aprsAddress parsed;
    memset(&parsed, 0, sizeof(parsed));

    size_t i = 0;
    for (; (str[i] != '\0') && (str[i] != '-'); i++) {
        if (i >= (ADDR_LEN - 1))
            return false; /* callsign longer than AX.25 can carry */

        char c = aprsUpcase(str[i]);
        /* Only the printable subset survives the shift-and-unshift round
         * trip, and a space would silently truncate the callsign. */
        if ((c <= ' ') || (c >= 0x7f))
            return false;

        parsed.addr[i] = c;
    }

    if (i == 0)
        return false; /* no callsign at all */

    parsed.addr[i] = '\0';

    if (str[i] == '-') {
        const char *digits = &str[i + 1];
        if (digits[0] == '\0')
            return false; /* a dash with nothing after it */

        unsigned ssid = 0;
        for (size_t j = 0; digits[j] != '\0'; j++) {
            if ((digits[j] < '0') || (digits[j] > '9'))
                return false;
            ssid = (ssid * 10u) + (unsigned)(digits[j] - '0');
            if (ssid > 15u)
                return false;
        }
        parsed.ssid = ssid & 0x0f;
    }

    *addr = parsed;
    return true;
}

size_t aprsFrameBuild(uint8_t *buf, size_t cap, const char *dest,
                      const char *src, const char *path, const char *info,
                      size_t infoLen)
{
    if ((buf == NULL) || (dest == NULL) || (src == NULL) || (info == NULL))
        return 0;
    if ((infoLen == 0) || (infoLen > APRS_PACLEN))
        return 0;

    /* Parse every address before writing anything, so a bad path leaves the
     * caller's buffer untouched rather than half-built. */
    struct aprsAddress addresses[APRS_MAX_ADDRESSES];
    uint8_t count = 0;

    if (!aprsAddrFromStr(dest, &addresses[count++]))
        return 0;
    if (!aprsAddrFromStr(src, &addresses[count++]))
        return 0;

    if ((path != NULL) && (path[0] != '\0')) {
        const char *p = path;
        while (*p != '\0') {
            if (count >= APRS_MAX_ADDRESSES)
                return 0; /* more digipeaters than AX.25 allows */

            const char *comma = strchr(p, ',');
            size_t len = (comma != NULL) ? (size_t)(comma - p) : strlen(p);

            char element[APRS_ADDR_STR_LEN];
            if ((len == 0) || (len >= sizeof(element)))
                return 0;

            memcpy(element, p, len);
            element[len] = '\0';

            if (!aprsAddrFromStr(element, &addresses[count]))
                return 0;
            count++;

            if (comma == NULL)
                break;

            p = comma + 1;
            if (*p == '\0')
                return 0; /* trailing comma: an element that is not there */
        }
    }

    const size_t total = ((size_t)count * ADDR_LEN) + 2 /* ctrl+pid */
                       + infoLen;
    if ((total > APRS_PACLEN) || (total > cap))
        return 0;

    for (uint8_t i = 0; i < count; i++) {
        /* The command bit belongs to the destination; APRS sends UI frames
         * as commands, which is what every other station does. */
        aprsWriteAddress(&buf[(size_t)i * ADDR_LEN], &addresses[i], (i == 0),
                         (i == (count - 1)));
    }

    size_t offset = (size_t)count * ADDR_LEN;
    buf[offset++] = 0x03; /* UI frame, no acknowledgement */
    buf[offset++] = 0xf0; /* no layer 3 protocol          */

    memcpy(&buf[offset], info, infoLen);

    return total;
}
