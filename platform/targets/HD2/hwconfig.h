/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Active HD2 bring-up; capabilities below are tentative.
 */

#ifndef HWCONFIG_H
#define HWCONFIG_H

#include <stdint.h>
#include "pinmap.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward decl -- avoids pulling interfaces/nvmem.h into every consumer. */
struct nvmDevice;

/* W25Q external SPI flash (W25Q512, 4-byte addressing).  Instantiated in
 * hwconfig.c with the bitbang SPI driver in drivers/SPI/spi_hd2.c. */
extern const struct nvmDevice eflash;

/*
 * Hardware capabilities the HD2 advertises to the OpenRTX core.
 *
 * Most CONFIG_* feature toggles deliberately stay OFF for this initial
 * port -- we want a minimal build that links and boots, not the full
 * radio.  Features come on one at a time as their driver is verified.
 */

/* Screen: ST7735S, 160x128, RGB565.  See src/firmware/lcd/ for the
 * extracted vendor driver we'll port. */
#define CONFIG_SCREEN_WIDTH    160
#define CONFIG_SCREEN_HEIGHT   128
#define CONFIG_PIX_FMT_RGB565
#define CONFIG_SCREEN_BRIGHTNESS

/* Battery: Li-ion, 1 cell (BL-12 nominal 7.4V single-pack; OpenRTX's
 * NCELLS=1 means we report cell voltage directly). */
#define CONFIG_BAT_LIION
#define CONFIG_BAT_NCELLS      1

/* Microphone gain -- mid value, will tune once audio path is verified. */
#define CONFIG_MIC_GAIN        32
#define CONFIG_MIC_OVERSAMPLE  8

/* GPS: the HD2-GPS variant has an NMEA module on UART2 (0x14050000),
 * 9600 8N1.  Enables the core GPS task / UI GPS screen and the
 * nmeaRbuf used by platform/drivers/GPS/gps_HD2.c. */
#define CONFIG_GPS
#define CONFIG_NMEA_RBUF_SIZE  128

/* Real-time clock: HR_C7000 on-chip RTC on the internal DesignWare I2C2
 * master (manual 4.12).  Driver in platform/mcu/CSKY_V2/drivers/rtc_hd2.c.
 * Gates state.c / UI clock display on platform_getCurrentTime(). */
#define CONFIG_RTC

/* Broadcast-FM receive: dedicated RDA5802E tuner chip (GPIOA I2C 0x20), driven
 * by OpMode_FMBroadcast via the tuner HAL (RDA5802E_HD2.c implements tuner_*).
 * Enables the OPMODE_FM_BCAST dispatch in rtx.cpp. */
#define CONFIG_FM_BCAST

#ifdef __cplusplus
}
#endif

#endif /* HWCONFIG_H */
