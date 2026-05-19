/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPENRTX_H
#define OPENRTX_H

#include <stddef.h>

/**
 * Initialisation of all the OpenRTX components, to be called before the main
 * entrypoint.
 */
void openrtx_init();

/**
 * Entrypoint of the OpenRTX firmware.
 * This function returns only on linux emulator, when the main program terminates.
 * Return type is void* to make this function be used as a pthread thread body.
 */
void *openrtx_run(void *arg);

#endif /* OPENRTX_H */
