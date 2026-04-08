/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/usb_serial.h"
#include "peripherals/gpio.h"
#include "pinmap.h"
#include "hwconfig.h"
#include "tusb.h"
#include <stm32h743xx.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdatomic.h>
#include <pthread.h>

/*
 * USB device mount state, maintained by tud_mount_cb / tud_umount_cb.
 * Written only from the main thread (tinyUSB callbacks run inside tud_task()).
 * Read from any thread via usb_serial_write().  _Atomic ensures the read is
 * safe without a mutex.
 *
 * Gated on tud_mounted() (device enumerated) rather than tud_cdc_connected()
 * (DTR asserted by host).  DTR-gating is too strict: many host tools - loggers,
 * scripted readers, cat - never assert DTR, which would make output appear
 * silently dropped.  If the host has not opened a terminal the ring buffer just
 * fills up and writes return short; no data is sent to the CDC FIFO until a
 * consumer connects.
 */
static _Atomic bool usb_mounted = false;

/*
 * Initialisation complete flag.  Set at the end of usb_serial_init().
 * Guards usb_serial_available() and usb_serial_read() against calls before
 * tinyUSB state is valid.  usb_serial_write() is guarded implicitly by
 * usb_mounted (false until tud_mount_cb fires after init).
 */
static _Atomic bool usb_ready = false;

/*
 * Software TX ring buffer.
 *
 * Multiple threads may call usb_serial_write() concurrently (MPSC).
 * tx_mutex serialises the write (producer) side.  usb_serial_task()
 * is the sole consumer and runs in the main thread; it snapshots
 * tx_head with an atomic acquire load so it never needs to hold the
 * mutex while writing to the CDC FIFO.
 *
 * Both tx_head and tx_tail are _Atomic: tx_tail is read by producers
 * (to detect a full ring) without holding tx_mutex, so it must be
 * accessed atomically to avoid data races.  Producers read tx_tail
 * with a relaxed load - a slightly stale value produces a conservative
 * "ring fuller than it really is" result, which can only cause short
 * writes, already the documented behaviour.
 */

static uint8_t          tx_buf[USB_SERIAL_TX_BUF_SIZE];
static _Atomic uint32_t tx_head = 0;   /* next write position, producer-owned */
static _Atomic uint32_t tx_tail = 0;   /* next read position,  consumer-owned  */
static pthread_mutex_t  tx_mutex = PTHREAD_MUTEX_INITIALIZER;

void usb_serial_init(void)
{
    /* Enable D2 SRAM1 clock so that the .usb_ram section at 0x30000000 is
     * accessible. RCC_AHB2ENR resets to 0; without this, any access to
     * tinyUSB buffers placed in D2 SRAM causes a bus fault.
     */
    RCC->AHB2ENR |= RCC_AHB2ENR_D2SRAM1EN;
    __DSB();

    /*
     * Enable the USB voltage detector (PWR_CR3_USB33DEN) and wait for
     * VDDUSB to stabilise.  The FS PHY requires a valid 3.3 V supply on
     * VDDUSB; without it the PHY clock never starts and tinyUSB's
     * reset_core() hangs forever polling GRSTCTL_AHBIDL.
     *
     * Note: PWR_CR3_USBREGEN (the internal USB voltage regulator) is
     * intentionally NOT set.  The regulator sources current from the
     * battery and creates a leakage path that keeps the battery voltage
     * reading above the power-off threshold in platform_pwrButtonStatus(),
     * preventing the device from turning off.  The external 3.3 V rail
     * is sufficient; only the detector is needed.
     */
    PWR->CR3 |= PWR_CR3_USB33DEN;
    while (!(PWR->CR3 & PWR_CR3_USB33RDY)) {}

    /* Enable GPIOA clock (AHB4 on STM32H7) and configure USB_DN/USB_DP */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    __DSB();

    gpio_setMode(USB_DN, ALTERNATE | ALTERNATE_FUNC(10));
    gpio_setMode(USB_DP, ALTERNATE | ALTERNATE_FUNC(10));
    gpio_setOutputSpeed(USB_DN, HIGH);
    gpio_setOutputSpeed(USB_DP, HIGH);

    /* Enable HSI48 and wait for it to be ready */
    RCC->CR |= RCC_CR_HSI48ON;
    while ((RCC->CR & RCC_CR_HSI48RDY) == 0) {}

    /*
     * Select HSI48 as USB clock source.
     * D2CCIP2R USBSEL[1:0]: 00=off, 01=PLL1Q, 10=PLL3Q, 11=HSI48.
     * Both USBSEL_0 (bit 20) and USBSEL_1 (bit 21) must be set for HSI48.
     */
    RCC->D2CCIP2R = (RCC->D2CCIP2R & ~RCC_D2CCIP2R_USBSEL)
                  | RCC_D2CCIP2R_USBSEL_0 | RCC_D2CCIP2R_USBSEL_1;

    /*
     * USB_OTG_FS is USB2 on STM32H743; use RCC_AHB1ENR_USB2OTGFSEN.
     * (USB1 is the HS peripheral; USB2 is the FS-only peripheral on PA11/PA12.)
     *
     * Force a peripheral reset before enabling the clock. NVIC_SystemReset()
     * should reset peripherals, but the USB OTG core can still be left in a
     * bad state after a soft reset. The explicit reset below is the safe
     * workaround regardless.
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_USB2OTGFSEN;
    __DSB();

    /* Reset the USB peripheral after enabling its clock to clear any stale
     * state left over from a previous soft reset.
     */
    RCC->AHB1RSTR |=  RCC_AHB1RSTR_USB2OTGFSRST;
    __DSB();
    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_USB2OTGFSRST;
    __DSB();

    /*
     * Disable VBUS detection so the FS PHY treats VBUS as always present.
     * On STM32H7 the F4-era NOVBUSSENS bit does not exist; clearing VBDEN
     * achieves the same result.
     * Do not set PWRDWN here; tinyUSB's dwc2_phy_init() sets it during
     * tusb_init() and would conflict if we set it beforehand.
     */
    USB_OTG_FS->GCCFG &= ~USB_OTG_GCCFG_VBDEN;

    /*
     * Set interrupt priority BEFORE tusb_init().
     * Do not call NVIC_EnableIRQ() here: tinyUSB calls dcd_int_enable()
     * at the end of tud_rhport_init(). Enabling the IRQ before the USB stack
     * is initialized causes a hard fault when the peripheral fires an event
     * with uninitialized tinyUSB state.
     */
    NVIC_SetPriority(OTG_FS_IRQn, USB_OTG_FS_IRQ_PRIORITY);

    if (!tusb_init())
    {
        return;
    }

    /*
     * Disable buffering on stdout.  usb_serial_write() is non-blocking and
     * manages its own internal buffer; a libc-level buffer would only add
     * latency and complicate partial-write handling.
     */
    (void) setvbuf(stdout, NULL, _IONBF, 0);

    atomic_store(&usb_ready, true);
}

void usb_serial_terminate(void)
{
    NVIC_DisableIRQ(OTG_FS_IRQn);
    RCC->AHB1ENR &= ~RCC_AHB1ENR_USB2OTGFSEN;
    __DSB();
    atomic_store(&usb_mounted, false);
    atomic_store(&usb_ready, false);
}

void usb_serial_task(void)
{
    /* Service tinyUSB first so that mount/unmount state changes are
     * processed before we inspect usb_mounted or write to the CDC FIFO.
     * Without this, the first bytes after a terminal connects can be
     * discarded because usb_mounted is still false on this tick.
     */
    tud_task();

    if (atomic_load(&usb_mounted))
    {
        /* Drain the TX ring buffer into the CDC FIFO.
         *
         * Snapshot tx_head with an acquire load so we see all bytes written
         * by producers before their release store.  Load tx_tail with a
         * relaxed load - it is consumer-private and only ever modified here.
         */
        uint32_t head = atomic_load_explicit(&tx_head, memory_order_acquire);
        uint32_t tail = atomic_load_explicit(&tx_tail, memory_order_relaxed);

        while (tail != head)
        {
            /* Number of contiguous bytes available before wrap */
            uint32_t chunk = (head >= tail) ? (head - tail)
                                            : (USB_SERIAL_TX_BUF_SIZE - tail);

            uint32_t sent = tud_cdc_write(&tx_buf[tail], chunk);
            tail = (tail + sent) % USB_SERIAL_TX_BUF_SIZE;

            if (sent < chunk)
            {
                break;  /* CDC FIFO full; flush and retry next tick */
            }
        }

        atomic_store_explicit(&tx_tail, tail, memory_order_relaxed);
        tud_cdc_write_flush();
    }
}

bool usb_serial_connected(void)
{
    return atomic_load(&usb_mounted);
}

uint32_t usb_serial_available(void)
{
    if (!atomic_load(&usb_ready))
    {
        return 0;
    }

    return tud_cdc_available();
}

ssize_t usb_serial_write(const void *buf, size_t len)
{
    if (!atomic_load(&usb_mounted))
    {
        return -1;
    }

    const uint8_t *p       = (const uint8_t *) buf;
    size_t         written = 0;

    pthread_mutex_lock(&tx_mutex);

    /* Use a local head so all bytes of this write are committed atomically.
     * The mutex guarantees exclusive producer access; the release store at
     * the end makes the bytes visible to the consumer in one step, preventing
     * usb_serial_task() from observing a partial write mid-buffer.
     */
    uint32_t head = atomic_load_explicit(&tx_head, memory_order_relaxed);

    while (written < len)
    {
        uint32_t next = (head + 1) % USB_SERIAL_TX_BUF_SIZE;
        if (next == atomic_load_explicit(&tx_tail, memory_order_relaxed))
        {
            break;  /* ring buffer full */
        }
        tx_buf[head] = p[written++];
        head = next;
    }

    atomic_store_explicit(&tx_head, head, memory_order_release);

    pthread_mutex_unlock(&tx_mutex);

    return (ssize_t) written;
}

ssize_t usb_serial_read(void *buf, size_t len)
{
    if (!atomic_load(&usb_ready))
    {
        return -1;
    }

    if (!atomic_load(&usb_mounted))
    {
        return -1;
    }

    return (ssize_t) tud_cdc_read(buf, len);
}

/* Funky function name required because of C/C++ mangling from Miosix
 * See: https://openrtx.org/#/software?id=parts-with-c-code-in-the-codebase */
void _Z17OTG_FS_IRQHandlerv(void)
{
    tud_int_handler(USB_SERIAL_RHPORT);
}

void tud_mount_cb(void)
{
    atomic_store(&usb_mounted, true);
}

void tud_umount_cb(void)
{
    atomic_store(&usb_mounted, false);
}
