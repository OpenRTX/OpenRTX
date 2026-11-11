/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/packet.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/*
 * Bits of the last byte of an AX.25 address, laid out as CRRSSSSL: the
 * command bit on the destination and source, or the has-been-repeated bit on
 * a digipeater; two reserved bits; the SSID; and the last-address marker.
 */
enum addrFlags {
    ADDR_LAST = 0x01,      ///< Last address of the address field.
    ADDR_SSID_SHIFT = 1,   ///< Position of the SSID.
    ADDR_SSID_MASK = 0x1e, ///< Four-bit SSID.
    ADDR_RESERVED = 0x60,  ///< Reserved bits, sent as ones.
    ADDR_CMD_OR_H = 0x80,  ///< Command bit, or H bit on a digipeater.
};

/* First non-printable character past the printable ASCII range. */
#define ASCII_DEL 0x7f

/* ":ADDRESSEE:" — the separator after the padded addressee is at a fixed
 * offset. */
#define MSG_TEXT_OFFSET (1 + APRS_MSG_ADDRESSEE_LEN + 1)

/*
 * Classify the data type identifier that opens an APRS info field.
 */
static enum aprsType classifyDti(char dti)
{
    switch (dti) {
        case ':':
            return APRS_TYPE_MESSAGE;
        case '!':
        case '=':
        case '/':
        case '@':
            return APRS_TYPE_POSITION;
        case '>':
            return APRS_TYPE_STATUS;
        case ';':
        case ')':
            return APRS_TYPE_OBJECT;
        case 'T':
            return APRS_TYPE_TELEMETRY;
        default:
            return APRS_TYPE_OTHER;
    }
}

/*
 * Unshift one seven-byte AX.25 address. The padding spaces, and any other
 * byte that does not unshift to a printable character, are dropped.
 */
static void parseAddress(const uint8_t *src, struct aprsAddress *addr)
{
    const uint8_t flags = src[APRS_ADDR_LEN - 1];
    size_t j = 0;

    for (size_t k = 0; k < APRS_ADDR_LEN - 1; k++) {
        uint8_t c = src[k] >> 1;
        if ((c > ' ') && (c < ASCII_DEL))
            addr->addr[j++] = (char)c;
    }

    addr->addr[j] = '\0';
    addr->ssid = (flags & ADDR_SSID_MASK) >> ADDR_SSID_SHIFT;
    addr->repeated = ((flags & ADDR_CMD_OR_H) != 0) ? 1 : 0;
}

/*
 * Count the addresses a frame carries, or 0 if its address field is
 * malformed: shorter than a destination and a source, never terminated, or
 * longer than AX.25 allows.
 */
static size_t countAddresses(const uint8_t *data, size_t len)
{
    size_t offset = 0;
    size_t count = 0;
    bool last = false;

    while (((len - offset) >= APRS_ADDR_LEN) && !last) {
        if (count == APRS_MAX_ADDRESSES)
            return 0;

        last = (data[offset + APRS_ADDR_LEN - 1] & ADDR_LAST) != 0;
        offset += APRS_ADDR_LEN;
        count++;
    }

    if (!last || (count <= APRS_ADDR_SRC))
        return 0;

    return count;
}

int aprsPkt_fromFrame(const uint8_t *data, size_t len, struct aprsPacket *pkt)
{
    /* Check the whole frame before writing anything, so a malformed one
     * leaves the caller's packet untouched. */
    const size_t count = countAddresses(data, len);
    if (count == 0)
        return -EINVAL;

    const size_t infoOffset = (count * APRS_ADDR_LEN) + APRS_CTRL_PID_LEN;
    if (infoOffset >= len)
        return -EINVAL;

    const size_t infoLen = len - infoOffset;
    if (infoLen > APRS_MAX_INFO)
        return -EINVAL;

    memset(pkt, 0, sizeof(*pkt));
    pkt->addressesLen = (uint8_t)count;
    pkt->infoLen = (uint16_t)infoLen;

    for (size_t i = 0; i < count; i++)
        parseAddress(&data[i * APRS_ADDR_LEN], &pkt->addresses[i]);

    pkt->info = (const char *)&data[infoOffset];
    pkt->type = classifyDti(pkt->info[0]);

    return 0;
}

size_t aprsAddr_toStr(const struct aprsAddress *addr, char *out, size_t len)
{
    int written;

    if (addr->ssid != 0)
        written = snprintf(out, len, "%s-%u", addr->addr, addr->ssid);
    else
        written = snprintf(out, len, "%s", addr->addr);

    if (written < 0) {
        out[0] = '\0';
        return 0;
    }

    return ((size_t)written < len) ? (size_t)written : (len - 1);
}

size_t aprsPath_toStr(const struct aprsPacket *pkt, char *out, size_t len)
{
    size_t used = 0;

    out[0] = '\0';

    for (size_t i = APRS_ADDR_PATH; i < pkt->addressesLen; i++) {
        if ((used + 1) >= len)
            break;

        if (used > 0)
            out[used++] = ',';

        used += aprsAddr_toStr(&pkt->addresses[i], out + used, len - used);

        if ((pkt->addresses[i].repeated != 0) && ((used + 1) < len))
            out[used++] = '*';

        out[used] = '\0';
    }

    return used;
}

int aprsMsg_unwrap(const struct aprsPacket *pkt, char *addressee,
                   size_t addresseeLen, char *text, size_t textLen)
{
    /* The separator after the fixed-width addressee tells a message from any
     * other info field that merely starts with ':'. */
    if ((pkt->type != APRS_TYPE_MESSAGE) || (pkt->infoLen < MSG_TEXT_OFFSET)
        || (pkt->info[MSG_TEXT_OFFSET - 1] != ':'))
        return -EINVAL;

    size_t n = APRS_MSG_ADDRESSEE_LEN;
    while ((n > 0) && (pkt->info[n] == ' '))
        n--;

    if (n == 0)
        return -EINVAL;
    if (n >= addresseeLen)
        n = addresseeLen - 1;

    memcpy(addressee, &pkt->info[1], n);
    addressee[n] = '\0';

    /* The "{seq" suffix of an acknowledged message is not part of the
     * text. */
    const char *body = &pkt->info[MSG_TEXT_OFFSET];
    size_t bodyLen = pkt->infoLen - MSG_TEXT_OFFSET;
    const char *brace = memchr(body, '{', bodyLen);
    if (brace != NULL)
        bodyLen = (size_t)(brace - body);

    if (bodyLen >= textLen)
        bodyLen = textLen - 1;

    memcpy(text, body, bodyLen);
    text[bodyLen] = '\0';

    return 0;
}

/*
 * Upper-case an ASCII letter, leaving anything else as it is.
 */
static char upcase(char c)
{
    if ((c >= 'a') && (c <= 'z'))
        return (char)(c - 'a' + 'A');

    return c;
}

/*
 * Read a decimal SSID of 0 to APRS_SSID_MAX, leading zeros allowed.
 *
 * @return the SSID, or -EINVAL if the text is not one.
 */
static int parseSsid(const char *digits)
{
    uint32_t ssid = 0;

    if (digits[0] == '\0')
        return -EINVAL;

    for (size_t j = 0; digits[j] != '\0'; j++) {
        if ((digits[j] < '0') || (digits[j] > '9'))
            return -EINVAL;

        ssid = (ssid * 10) + (uint32_t)(digits[j] - '0');
        if (ssid > APRS_SSID_MAX)
            return -EINVAL;
    }

    return (int)ssid;
}

int aprsAddr_fromStr(const char *str, struct aprsAddress *addr)
{
    struct aprsAddress parsed;
    size_t i = 0;

    memset(&parsed, 0, sizeof(parsed));

    for (; (str[i] != '\0') && (str[i] != '-'); i++) {
        if (i >= (APRS_ADDR_LEN - 1))
            return -EINVAL;

        /* Only printable characters survive the shift, and a space would
         * end the callsign early. */
        char c = upcase(str[i]);
        if ((c <= ' ') || (c >= ASCII_DEL))
            return -EINVAL;

        parsed.addr[i] = c;
    }

    if (i == 0)
        return -EINVAL;

    parsed.addr[i] = '\0';

    if (str[i] == '-') {
        const int ssid = parseSsid(&str[i + 1]);
        if (ssid < 0)
            return ssid;

        parsed.ssid = (uint8_t)ssid;
    }

    *addr = parsed;
    return 0;
}

int aprsAddr_validate(const char *str)
{
    size_t i = 0;

    for (; (str[i] != '\0') && (str[i] != '-'); i++) {
        const char c = str[i];
        const bool alnum = ((c >= 'A') && (c <= 'Z'))
                        || ((c >= '0') && (c <= '9'));

        if ((i >= (APRS_ADDR_LEN - 1)) || !alnum)
            return -EINVAL;
    }

    if (i == 0)
        return -EINVAL;
    if (str[i] == '\0')
        return 0;

    /* No leading zero, as APRS writes them */
    const char *ssid = &str[i + 1];
    if ((ssid[0] == '0') && (ssid[1] != '\0'))
        return -EINVAL;

    if (parseSsid(ssid) < 0)
        return -EINVAL;

    return 0;
}
