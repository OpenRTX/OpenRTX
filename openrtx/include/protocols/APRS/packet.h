/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_PACKET_H
#define APRS_PACKET_H

#include "protocols/APRS/constants.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Longest formatted "CALLSIGN-SSID" string, with its NUL: six callsign
 * characters, a dash and two SSID digits.
 */
#define APRS_ADDR_STR_LEN 10

/**
 * Longest text an addressed ":ADDRESSEE:text" message carries.
 */
#define APRS_MSG_TEXT_MAX 67

/**
 * Length of a message's addressee field, always space-padded to it.
 */
#define APRS_MSG_ADDRESSEE_LEN 9

/**
 * Position of each address in an AX.25 address field: the destination, the
 * source, then the digipeater path.
 */
enum aprsAddrIndex {
    APRS_ADDR_DEST = 0, ///< Destination address.
    APRS_ADDR_SRC = 1,  ///< Source address.
    APRS_ADDR_PATH = 2, ///< First digipeater, if any.
};

/**
 * One AX.25 address: callsign, SSID, and the has-been-repeated bit.
 */
// clang-format off
struct aprsAddress {
    char addr[APRS_ADDR_LEN]; ///< Callsign, unshifted and NUL-terminated.
    uint8_t ssid      : 4,    ///< Secondary station identifier, 0-15.
            repeated  : 1,    ///< H bit: this digipeater has repeated it.
            _reserved : 3;
};
// clang-format on

/**
 * APRS data type, from the data type identifier (DTI) that opens the info
 * field. Anything not named is APRS_TYPE_OTHER and keeps its raw text.
 */
enum aprsType {
    APRS_TYPE_OTHER = 0, ///< Unrecognised or unclassified DTI.
    APRS_TYPE_MESSAGE,   ///< ':' addressed text message.
    APRS_TYPE_POSITION,  ///< '!' '=' '/' '@' position report.
    APRS_TYPE_STATUS,    ///< '>' status text.
    APRS_TYPE_OBJECT,    ///< ';' object, ')' item.
    APRS_TYPE_TELEMETRY, ///< 'T' telemetry report.
};

/**
 * A parsed APRS packet. Its info field points into the frame it was parsed
 * from, so it is valid only as long as that frame is.
 */
struct aprsPacket {
    const char *info;     ///< Info field, inside the frame; not NUL-terminated.
    struct aprsAddress addresses[APRS_MAX_ADDRESSES];
    enum aprsType type;   ///< Classification of info[0].
    uint16_t infoLen;     ///< Info field length.
    uint8_t addressesLen; ///< Addresses actually present.
};

/**
 * Parse an AX.25 UI frame into caller-supplied packet storage.
 *
 * Rejects frames too short for two addresses, whose address field never ends
 * or holds more than APRS_MAX_ADDRESSES addresses, or that have no info field
 * or one longer than APRS_MAX_INFO.
 *
 * @param data: raw AX.25 frame, without its FCS.
 * @param len: frame length in bytes.
 * @param pkt: destination packet, pointing into data; untouched if
 * parsing fails.
 * @return 0 on success, -EINVAL if the frame is malformed.
 */
int aprsPkt_fromFrame(const uint8_t *data, size_t len, struct aprsPacket *pkt);

/**
 * Format an address as "CALLSIGN", or "CALLSIGN-SSID" when the SSID is not
 * zero.
 *
 * @param addr: address to format.
 * @param out: destination buffer.
 * @param len: size of out; APRS_ADDR_STR_LEN is always enough.
 * @return number of characters written, not counting the NUL.
 */
size_t aprsAddr_toStr(const struct aprsAddress *addr, char *out, size_t len);

/**
 * Format the digipeater path as a comma-separated list, with '*' after each
 * address that has repeated the frame, as TNC2 monitors do. Empty when the
 * packet was heard directly.
 *
 * @param pkt: packet whose path to format.
 * @param out: destination buffer.
 * @param len: size of out; the result is truncated to fit.
 * @return number of characters written, not counting the NUL.
 */
size_t aprsPath_toStr(const struct aprsPacket *pkt, char *out, size_t len);

/**
 * Split an addressed message ":ADDRESSEE:text{seq" into its addressee,
 * without the padding, and its text, without the "{seq" suffix.
 *
 * @param pkt: packet to unwrap.
 * @param addressee: destination for the addressee, NUL-terminated.
 * @param addresseeLen: size of addressee; APRS_ADDR_STR_LEN is enough.
 * @param text: destination for the text, NUL-terminated and truncated to fit.
 * @param textLen: size of text.
 * @return 0 on success, -EINVAL if the packet is not an addressed message.
 */
int aprsMsg_unwrap(const struct aprsPacket *pkt, char *addressee,
                   size_t addresseeLen, char *text, size_t textLen);

#ifdef __cplusplus
}
#endif

#endif /* APRS_PACKET_H */
