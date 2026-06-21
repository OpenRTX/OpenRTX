/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PACKET_ENGINE_H
#define PACKET_ENGINE_H

int packetEngine_init();
void packetEngine_terminate();
void packetEngine_task();

#endif
