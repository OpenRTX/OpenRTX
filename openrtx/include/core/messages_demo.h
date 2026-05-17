/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MESSAGES_DEMO_H
#define MESSAGES_DEMO_H

#ifdef CONFIG_MESSAGES_DEMO

#include "core/messages.h"

/**
 * Vtable for the canned demo message source.
 * Referenced by the static source table in messages.cpp.
 */
extern const message_type_vtable_t messages_demo_vtable;

/**
 * Opaque context for the demo source (may be NULL).
 * Declared here so messages.cpp can take its address.
 */
extern void *messages_demo_ctx;

#endif /* CONFIG_MESSAGES_DEMO */
#endif /* MESSAGES_DEMO_H */
