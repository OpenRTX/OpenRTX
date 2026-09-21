/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_PACKET_H
#define APRS_PACKET_H

#include <string.h>
#include <stdlib.h>
#include "interfaces/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

struct aprsAddress {
    char addr[7]; /* include null termination */
    uint8_t ssid         : 4;
    uint8_t commandHeard : 1;
    uint8_t _unused      : 3;
};

struct aprsPacket {
    struct aprsPacket *prev;
    struct aprsPacket *next;
    datetime_t ts;
    uint8_t addressesLen;
    uint8_t infoLen;
    struct aprsAddress *addresses;
    char *info;
};

/**
 * @brief Maximum number of addresses an AX.25 frame may carry.
 *
 * A destination, a source, and up to eight digipeaters.
 */
#define APRS_MAX_ADDRESSES 10

/**
 * @brief Length of a formatted "CALLSIGN-SSID" string, including the NUL.
 *
 * Six callsign characters, a dash, two SSID digits, and the terminator.
 */
#define APRS_ADDR_STR_LEN 10

/**
 * @brief AX.25 destination address used for every frame OpenRTX transmits.
 *
 * APRS calls this the "device identifier" or TOCALL: receiving stations read
 * it to tell what software produced the packet. APORT? is registered to
 * OpenRTX in aprsorg/aprs-deviceid, with the final character ours to assign;
 * the digit marks the firmware generation. Using the registered allocation
 * rather than an experimental APZ*** prefix is what lets receiving stations
 * attribute a packet correctly, and OpenRTX usage be measured on APRS-IS.
 */
#define APRS_TOCALL "APORT1"

/**
 * @brief Digipeater path used for every frame OpenRTX transmits.
 *
 * The conventional one-hop-local, one-hop-wide path: the first digipeater
 * that hears the frame repeats it, and one wide-area digipeater repeats it
 * again. Adequate everywhere and antisocial nowhere, which is what a fixed
 * default has to be until a path setting ships with beaconing.
 */
#define APRS_DEFAULT_PATH "WIDE1-1,WIDE2-1"

/**
 * @brief Creates an APRS packet from frame data
 *
 * @param data: pointer to frame data.
 * @param len: frame length in byte.
 * @return An aprsPacket pointer to a packet
 */
struct aprsPacket *aprsPktFromFrame(const uint8_t *data, const size_t len);

/**
 * @brief Parse a "CALLSIGN" or "CALLSIGN-SSID" string into an address.
 *
 * The callsign is upper-cased on the way in, so a recipient typed in lower
 * case still produces a valid frame. Rejects an empty or over-long callsign,
 * a non-numeric or out-of-range SSID, and any character AX.25 cannot carry.
 *
 * @param str: address text, NUL-terminated.
 * @param addr: destination address; untouched if the text is not an address.
 * @return true if @p str is a well-formed address.
 */
bool aprsAddrFromStr(const char *str, struct aprsAddress *addr);

/**
 * @brief Build an AX.25 UI frame from TNC2-style address text.
 *
 * The inverse of aprsPktFromFrame(), and message-agnostic on purpose: an
 * addressed message, a position beacon, and a digipeated frame differ only
 * in what they put in @p info, so everything that transmits shares this
 * builder.
 *
 * The frame carries the addresses, the UI control byte, the no-layer-3
 * protocol identifier, and the info field, but NOT the frame check sequence:
 * whatever puts the frame on the air appends that. The parser expects the
 * FCS to be present, so a caller round-tripping a built frame through
 * aprsPktFromFrame() must append crc_hdlc() first.
 *
 * @param buf: destination buffer; untouched if the arguments are invalid.
 * @param cap: size of buf; APRS_PACLEN is always enough.
 * @param dest: destination address, i.e. the TOCALL for a transmitted frame.
 * @param src: source address, "CALLSIGN" or "CALLSIGN-SSID".
 * @param path: comma-separated digipeater path, or NULL/"" for none.
 * @param info: info field bytes; need not be NUL-terminated.
 * @param infoLen: number of info bytes, which must be at least one.
 * @return frame length in bytes, or 0 if the frame could not be built.
 */
size_t aprsFrameBuild(uint8_t *buf, size_t cap, const char *dest,
                      const char *src, const char *path, const char *info,
                      size_t infoLen);

#ifdef __cplusplus
}
#endif

#endif /* APRS_PACKET_H */
