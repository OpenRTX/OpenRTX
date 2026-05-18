/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

extern "C" {
#include "rtx/rtx.h"
}

/**
 * Helper that replicates the flag-mapping logic from threads.c:
 *
 *   rtx_cfg.txDisable  = channel_rx_only;
 *   rtx_cfg.pttDisable = channel_rx_only || ui_ptt_guard;
 */
static void apply_tx_flags(rtxStatus_t *cfg,
                            uint8_t channel_rx_only,
                            bool ui_ptt_guard)
{
    cfg->txDisable  = channel_rx_only;
    cfg->pttDisable = channel_rx_only || ui_ptt_guard;
}

TEST_CASE("RTX TX disable flag separation", "[rtx]")
{
    rtxStatus_t cfg = {};

    SECTION("Normal channel, UI on VFO/MEM: both gates open")
    {
        apply_tx_flags(&cfg, 0, false);
        REQUIRE(cfg.txDisable  == 0);
        REQUIRE(cfg.pttDisable == 0);
    }

    SECTION("RX-only channel: both gates locked")
    {
        apply_tx_flags(&cfg, 1, false);
        REQUIRE(cfg.txDisable  == 1);
        REQUIRE(cfg.pttDisable == 1);
    }

    SECTION("UI screen guard only: PTT gate closes, hard gate stays open")
    {
        apply_tx_flags(&cfg, 0, true);
        REQUIRE(cfg.txDisable  == 0);
        REQUIRE(cfg.pttDisable == 1);
    }

    SECTION("RX-only channel with UI screen guard: both gates locked")
    {
        apply_tx_flags(&cfg, 1, true);
        REQUIRE(cfg.txDisable  == 1);
        REQUIRE(cfg.pttDisable == 1);
    }
}

/**
 * Replicates the RTX-thread enforcement in rtx_task():
 *
 *   if (rtxStatus.txDisable)
 *       rtxStatus.pttDisable = 1;
 */
static void apply_rtx_enforcement(rtxStatus_t *cfg)
{
    if (cfg->txDisable)
        cfg->pttDisable = 1;
}

TEST_CASE("RTX thread pttDisable enforcement", "[rtx]")
{
    rtxStatus_t cfg = {};

    SECTION("txDisable=1 forces pttDisable=1 regardless of incoming value")
    {
        cfg.txDisable  = 1;
        cfg.pttDisable = 0;
        apply_rtx_enforcement(&cfg);
        REQUIRE(cfg.pttDisable == 1);
    }

    SECTION("txDisable=0 leaves pttDisable unchanged")
    {
        cfg.txDisable  = 0;
        cfg.pttDisable = 1;
        apply_rtx_enforcement(&cfg);
        REQUIRE(cfg.txDisable  == 0);
        REQUIRE(cfg.pttDisable == 1);
    }

    SECTION("txDisable=0 and pttDisable=0: both remain open")
    {
        cfg.txDisable  = 0;
        cfg.pttDisable = 0;
        apply_rtx_enforcement(&cfg);
        REQUIRE(cfg.txDisable  == 0);
        REQUIRE(cfg.pttDisable == 0);
    }
}
