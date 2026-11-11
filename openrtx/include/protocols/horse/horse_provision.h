/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse identity USB serial / Linux Unix-socket provisioning protocol.
 */

#ifndef HORSE_PROVISION_H
#define HORSE_PROVISION_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void horse_provision_init(void);
void horse_provision_poll(void);

#ifdef PLATFORM_LINUX
/**
 * Resolve the default Unix socket path into out.
 * Honours OPENRTX_HORSE_PROV_SOCK, else XDG_RUNTIME_DIR, else
 * XDG_STATE_HOME/OpenRTX or ~/.local/state/OpenRTX.
 *
 * @return 0 on success, -1 on failure.
 */
int horse_provision_default_sock_path(char *out, size_t out_len);
#endif

#ifdef __cplusplus
}
#endif

#endif
