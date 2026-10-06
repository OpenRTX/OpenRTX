/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse identity USB serial provisioning protocol handler.
 */

#ifndef HORSE_PROVISION_H
#define HORSE_PROVISION_H

#ifdef __cplusplus
extern "C" {
#endif

void horse_provision_init(void);
void horse_provision_poll(void);

#ifdef __cplusplus
}
#endif

#endif
