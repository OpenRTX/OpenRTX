/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/cps.h"
#include <cstdio>
#include <cstring>

int main()
{
    horseInfo_t info;
    memset(&info, 0xFF, sizeof info);
    horse_info_reset(&info);
    if (info.rxCan != 0 || info.txCan != 0 || info.encrypt_en != 0 ||
        info.sign_en != 0 || info.contact_index != 0)
    {
        std::printf("horse_info_test: reset left garbage\n");
        return -1;
    }
    return 0;
}
