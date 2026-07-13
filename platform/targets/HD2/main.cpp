/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * OpenRTX-on-Miosix entry for the Ailunce HD2 (HR_C7000 / CK803S).
 * Built inside vendor/miosix-kernel's CMake (the HW-proven kernel build);
 * Miosix's crt0 -> _init -> startKernel runs us as the main thread, so we
 * just hand off to OpenRTX's standard threaded flow.
 */

extern "C" void openrtx_init(void);
extern "C" void *openrtx_run(void *arg);
extern "C" void audio_init(void);      // board audio routing init (audio_HD2.c)
extern "C" void hd2_diag_start(void);  // UART0 peek/poke diag thread (hd2_diag.cpp)
extern "C" void hd2_gps_start(void); // GPS UART2 RX worker (hd2_gps.cpp)

int main()
{
    openrtx_init();        // platform/state/gfx/kbd/ui/vp + codeplug + splash
    audio_init();          // amp/route GPIOs (was bundled in hd2_rtx.c's bringup;
                           // rtx.cpp's rtx_init does not init audio)
    hd2_diag_start();      // bring up the UART0 peek/poke diag thread (post UART init)
    hd2_gps_start(); // GPS UART2 RX worker thread
    // Broadcast FM is an OpMode now (OPMODE_FM_BCAST via the UI FM screen) --
    // the old hd2_fm_broadcast worker thread is gone.
    openrtx_run(nullptr);  // create_threads() (ui + rtx) then main_thread loop
    return 0;
}
