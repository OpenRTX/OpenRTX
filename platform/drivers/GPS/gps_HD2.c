/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * GPS driver for the Ailunce HD2 (HR_C7000 / CK803S).
 *
 * The GPS module is on the HR_C7000's UART2 (DesignWare 16550 @ 0x14050000,
 * 9600 8N1) with its RXD/TXD on PTA11/PTA12. The pad mux (DIPLEX0 bits
 * 11/12 = 0 -> UART2 function) is already set by platform_init. The module's
 * power rail is GPIOB.15 (active-high), off at boot and raised here.
 *
 * This is a POLLED driver: no PIC/ISR wiring. getSentence() first pumps every
 * byte waiting in the RX FIFO into OpenRTX's NMEA ring buffer, then hands the
 * core a complete sentence if one is ready. The core gps_task() (called from
 * the UI/main thread) drives it and parses the sentences via minmea.
 */

#include "gps_HD2.h"
#include "hwconfig.h"
#include "registers.h"
#include "pinmap.h"
#include "peripherals/gpio.h"
#include "nmea_rbuf.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* UART input clock is the 42 MHz post-PLL clock (the same the console UART0
 * uses); 16550 divisor = clk / (16 * baud). */
#define GPS_UART_CLK_HZ 42000000u
#define GPS_BAUD 9600u

static struct nmeaRbuf nmea;
static bool initialized = false;

static void gps_setBaud(uint32_t baud)
{
    uint32_t divisor = GPS_UART_CLK_HZ / (16u * baud);
    if (divisor == 0u)
        divisor = 1u;

    UART2->LCR = UART2->LCR | UART_LCR_DLAB; /* expose DLL/DLH */
    UART2->DLL = divisor & 0xffu;
    UART2->DLH = (divisor >> 8) & 0xffu;
    UART2->LCR = UART_LCR_8N1; /* DLAB cleared, 8N1 */
}

static void gps_uart_bringup(void)
{
    UART2->FCR = UART_FCR_ENABLE;
    UART2->IER = 0u; /* polled: no UART interrupts */
    gps_setBaud(GPS_BAUD);
    nmeaRbuf_reset(&nmea);
}

/* Drain the RX FIFO into the ring buffer. nmeaRbuf_putChar runs an FSM that
 * only stores bytes of a valid NMEA sentence, so raw UART bytes are safe. */
static void gps_drain(void)
{
    while ((UART2->LSR & UART_LSR_DR) != 0u)
        nmeaRbuf_putChar(&nmea, (char)(UART2->RBR & 0xffu));
}

static void gps_HD2_enable(void *priv)
{
    (void)priv;
    gpio_setPin(GPS_PWR); /* power the GPS module */
    gps_uart_bringup();
    gps_drain();          /* flush stale bytes */
}

static void gps_HD2_disable(void *priv)
{
    (void)priv;
    gpio_clearPin(GPS_PWR); /* cut GPS module power */
}

static int gps_HD2_getSentence(void *priv, char *buf, const size_t bufSize)
{
    (void)priv;
    if (!initialized)
        return 0;

    gps_drain(); /* non-blocking pump + extract */
    return nmeaRbuf_getSentence(&nmea, buf, bufSize);
}

const struct gpsDevice *gps_HD2_init(void)
{
    if (!initialized) {
        gpio_setPin(GPS_PWR); /* power on so NMEA buffers at once */
        gps_uart_bringup();
        gps_drain();
        initialized = true;
    }

    static const struct gpsDevice dev = {
        .priv = NULL,
        .enable = gps_HD2_enable,
        .disable = gps_HD2_disable,
        .getSentence = gps_HD2_getSentence,
    };
    return &dev;
}
