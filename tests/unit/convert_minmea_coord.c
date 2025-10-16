/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <minmea.h>
#include <stdio.h>
#include <stdlib.h>
#include "core/gps.h"

static void assert_conversion(struct minmea_float *f, int32_t expected)
{
    int32_t result = minmea_tofixedpoint(f);
    if (result != expected)
    {
        printf("FAILED! result value %d - expected %d\n",
               result, expected);
        exit(1);
    }
}

int main() {
    printf("minmea coordinate conversion test\n");
    struct minmea_float test = {5333735, 1000};
    assert_conversion(&test, 53562250);
    test.scale = 1;
    test.value = 0;
    assert_conversion(&test, 0);
    test.scale = 1000;
    test.value = -5333735;
    assert_conversion(&test, -53562250);
    test.scale = 1000;
    test.value = -330;
    assert_conversion(&test, -5500);
    test.scale = 1000;
    test.value = -3296;
    assert_conversion(&test, -54933);
    printf("PASS\n");

    return 0;
}
