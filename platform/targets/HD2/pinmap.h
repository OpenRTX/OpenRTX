/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Active HD2 bring-up; pin assignments are partial / tentative.
 */

#ifndef PINMAP_H
#define PINMAP_H

/*
 * HD2 GPIO pin assignments.
 *
 * GPIO numbering is the HR_C7000 bank-relative convention used in the
 * vendor V2.1.3 firmware: banks A, B, C with 32 pins each, so pin id
 * 0x47 = bank C pin 7, 0x27 = bank B pin 7, etc.  See memory hd2-keypad
 * for the keypad-side mapping, and src/firmware/include/keypad.h for
 * the full LUT.
 *
 * Only pins that OpenRTX's portable interface code reaches for are
 * listed here.  Driver-internal pins (LCD CS/RST/DC, SPI clock/MISO,
 * I2C SDA/SCL) stay inside their respective driver source files until
 * we know the abstractions need them at the platform level.
 *
 * The numerical encoding matches the format the vendor `gpio_get` /
 * `gpio_set` helpers expect.  When OpenRTX core code does
 *   gpio_setMode(PTT_SW, INPUT);
 * the HD2 GPIO driver translates PTT_SW to the actual MMIO write.
 */

/* ---- keypad rows (driven, active-low strobes) ---- */
#define KEYPAD_ROW0    0x47   /* GPIOC pin 7  */
#define KEYPAD_ROW1    0x48
#define KEYPAD_ROW2    0x49
#define KEYPAD_ROW3    0x4a

/* ---- keypad columns (sense inputs) ---- */
#define KEYPAD_COL0    0x4b
#define KEYPAD_COL1    0x4c
#define KEYPAD_COL2    0x4d
#define KEYPAD_COL3    0x4e
#define KEYPAD_COL4    0x27   /* GPIOB pin 7  */
#define KEYPAD_COL5    0x29

/* ---- W25Q external SPI flash (W25Q512, 4-byte addr) ---- */
/* CS lives on GPIOA bit 18 (GPIOA base 0x14020000); SCK/MOSI/MISO at
 * bits 20/21/22 are driven inside platform/drivers/SPI/spi_hd2.c and
 * aren't exposed through this header.  (The decomp symbol that looks
 * like an "LCD latch" is a bank-flattened GBR-relative GPIO DR; the
 * flash bus is GPIOA, live-verified JEDEC=ef4020 2026-05-30, while the
 * LCD is the GPIOC instance of the same flattened symbol.)  Vendor
 * V2.1.3 ref: flash_probe_jedec_id @ 0x03057790 (expect EF/40/20). */
#define EXT_FLASH_CS_BIT  18

/* ---- PTT --------- */
/* PTT detection on the HD2 is NOT a simple GPIO read -- it's inferred
 * from PWM ch1 control word at MMIO 0x140c0030 dropping bit 1 when the
 * audio TX mute kicks in.  See memory hd2-keypad.  We expose a pseudo
 * pin id for OpenRTX's PTT-status query path and translate inside the
 * GPIO driver. */
#define PTT_SW         0xff   /* virtual pin -- see drivers/GPIO_HD2.c */

/* ---- power / charging / external accessories ---- */
/* TODO: figure out real GPIO/ADC IDs.  For the initial stub build the
 * platform_pwrButtonStatus() impl returns 1 unconditionally (the radio
 * has no power-management MCU in the sense OpenRTX expects -- power is
 * controlled by the user-facing rocker switch which cuts main rail). */

#endif /* PINMAP_H */
