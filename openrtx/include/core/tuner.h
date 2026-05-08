/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef _TUNER_H_
#define _TUNER_H_

#include "core/cps.h"
#include "rtx/rtx.h"
#include <stdint.h>
#include <stddef.h>

/* Current tuner layout version. Increase this number when updating the tuner
layout*/
#define TUNER_VERSION 0x0000

enum tuner_mode {
    TUNER_VFO,
    TUNER_CHAN,
    TUNER_BANK
};

struct vfo {
    uint8_t op_mode;

    uint32_t rx_frequency;
    uint32_t tx_frequency;

    uint8_t bandwidth; // Ideally this would move to fmInfo_t. This is kept here for now to keep compatibility with channel_t until further development of the CPS format.

    fmInfo_t fm;
    m17Info_t m17;
    dmrInfo_t dmr;

} __attribute__((packed)); // 26 B

struct tuner_core {
    uint8_t tuner_mode;

    uint32_t power;
    struct vfo vfo;

    uint16_t bank_index;
    uint16_t channel_index;
} __attribute__((packed)); // 35 B

struct tuner {
    struct tuner_core core;
    struct {
        channel_t channel;
        bankHdr_t bank;
    } cache;
};

/**
 * @brief Return the default tuner.
 *
 * @return struct tuner default tuner
 */
struct tuner tuner_getDefault(void);

/**
 * @brief Get the tuner mode (VFO/Channel/Bank).
 *
 * @param t pointer to a tuner struct
 * @return enum tuner_mode current tuner mode
 */
enum tuner_mode tuner_get_mode(const struct tuner *t);

/**
 * @brief Get the current TX frequency of the tuner, accounting for the current tuner mode.
 *
 * @param t pointer to a tuner struct
 * @return uint32_t TX frequency
 */
uint32_t tuner_get_tx_frequency(const struct tuner *t);

/**
 * @brief Get the current RX frequency of the tuner, accounting for the current tuner mode.
 *
 * @param t pointer to a tuner struct
 * @return uint32_t RX frequency
 */
uint32_t tuner_get_rx_frequency(const struct tuner *t);

/**
 * @brief Get the current tuner transmission power.
 *
 * @param t pointer to a tuner struct
 * @return uint32_t Power in mW
 */
uint32_t tuner_get_power(const struct tuner *t);

/**
 * @brief Get the current tuner operating mode (FM/M17) accounting for the tuner mode (VFO/Channel/Bank)
 *
 * @param t pointer to a tuner struct
 * @return uint8_t current opmode
 */
uint8_t tuner_get_opmode(const struct tuner *t);

/**
 * @brief Gets the current M17 parameters of the tuner, accounting for the tuner mode
 *
 * @param t pointer to a tuner struct
 * @param params pointer to a m17Info_t structure to store parameters
 * @return int 0 if successful, negative error code otherwise
 */
int tuner_get_M17_params(const struct tuner *t, m17Info_t *params);

/**
 * @brief Gets the current FM parameters of the tuner, accounting for the tuner mode
 *
 * @param t pointer to a tuner struct
 * @param params pointer to a fmInfo_t structure to store parameters
 * @return int 0 if successful, negative error code otherwise
 */
int tuner_get_FM_params(const struct tuner *t, fmInfo_t *params);

/**
 * @brief Gets the current DMR parameters of the tuner, accounting for the tuner mode
 *
 * @param t pointer to a tuner struct
 * @param params pointer to a dmrInfo_t structure to store parameters
 * @return int 0 if successful, negative error code otherwise
 */
int tuner_get_DMR_params(const struct tuner *t, dmrInfo_t *params);

/**
 * @brief Gets the name of the current channel, returning an empty string if not in channel mode
 *
 * @param t pointer to a tuner struct
 * @param s pointer to the memory where the string will be written
 * @param n length of s
 */
void tuner_get_channel_name(const struct tuner *t, char *s, const size_t n);

/**
 * @brief Gets a pointer to the current channel if the tuner is in bank or channel mode.
 *
 * @param t pointer to a tuner struct
 * @return const channel_t* pointer to the channel, or NULL if current tuner mode does not correspond to a channel
 */
const channel_t *tuner_get_channel(const struct tuner *t);

/**
 * @brief Gets the current bandwidth setting of the tuner
 *
 * @param t pointer to a tuner struct
 * @return enum bandwidth
 */
enum bandwidth tuner_get_bandwidth(const struct tuner *t);

/**
 * @brief Switch the current tuner mode (VFO/Channel/Bank) *without* reloading cache
 *
 * @param t pointer to a tuner struct
 * @param mode tuner mode to set
 * @return int 0 if successful, negative error code otherwise
 */
int tuner_switch_mode(struct tuner *t, enum tuner_mode mode);

/**
 * @brief Set the tuner to bank mode, loading the cache with the correct data from NVM
 *
 * @param t pointer to a tuner struct
 * @param bank bank index to load
 * @param channel channel index to load
 * @return int 0 if successful, negative error code otherwise
 */
int tuner_set_mode_bank(struct tuner *t, const uint16_t bank, const uint16_t channel);

/**
 * @brief Set the tuner to channel mode, loading the cache with the correct data from NVM
 *
 * @param t pointer to a tuner struct
 * @param channel channel index to load
 * @return int 0 if successful, negative error code otherwise
 */
int tuner_set_mode_channel(struct tuner *t, const uint16_t channel);

/**
 * @brief Sets the tuner transmission power
 *
 * @param t pointer to tuner struct
 * @param power power in mW
 */
void tuner_set_power(struct tuner *t, const uint32_t power);

/**
 * @brief Sets the RX frequency of VFO mode without changing the tuner mode
 *
 * @param t pointer to tuner struct
 * @param freq RX frequency in Hz
 */
void tuner_set_vfo_rx_frequency(struct tuner *t, const uint32_t freq);

/**
 * @brief Sets the TX frequency of VFO mode without changing the tuner mode
 *
 * @param t pointer to tuner struct
 * @param freq TX frequency in Hz
 */
 void tuner_set_vfo_tx_frequency(struct tuner *t, const uint32_t freq);

/**
 * @brief Sets the operating mode of VFO mode without changing the tuner mode
 *
 * @param t pointer to tuner struct
 * @param opmode operating mode
 * @param params pointer to the adequate operating mode info structure
 */
void tuner_set_vfo_opmode(struct tuner *t, const uint8_t opmode, const void *params);

#endif