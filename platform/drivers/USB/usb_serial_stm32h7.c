/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/usb_serial.h"
#include "interfaces/delays.h"
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
 * silently dropped.
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
 * If tud_cdc_notify_uart_state() fails in tud_mount_cb() (INT EP busy), retry
 * from usb_serial_task() until it succeeds or we unmount.
 */
static _Atomic bool cdc_uart_notify_pending = false;

#if CFG_TUD_CDC_NOTIFY
/*
 * After reconnect, fire several SERIAL_STATE notifications in quick succession
 * so Linux cdc-acm sees carrier before open() blocks for ~10 s waiting on DCD.
 */
static uint8_t carrier_boot_notify_remaining;
#endif

/*
 * Mutex that serialises all tinyUSB CDC calls (tud_cdc_write,
 * tud_cdc_write_flush, tud_cdc_write_clear) and tud_task() itself.
 * tud_task() is not re-entrant; holding this mutex before every call
 * ensures only one thread pumps the USB stack at a time, whether that
 * is usb_serial_task() on its regular tick or usb_serial_write()
 * waiting for the CDC TX FIFO to drain.
 */
static pthread_mutex_t cdc_mutex = PTHREAD_MUTEX_INITIALIZER;

#if CFG_TUD_CDC_NOTIFY
/*
 * Assert DCD/DSR via SERIAL_STATE so Linux cdc-acm treats the port as having
 * carrier (open() / stty do not block waiting for modem lines).  If the notify
 * endpoint is busy, usb_serial_task() retries via cdc_uart_notify_pending.
 */
static void usb_serial_send_carrier_notify(void)
{
    cdc_notify_uart_state_t state = { 0 };

    state.dcd = 1;
    state.dsr = 1;
    if (tud_cdc_notify_uart_state(&state)) {
        atomic_store(&cdc_uart_notify_pending, false);
    } else {
        atomic_store(&cdc_uart_notify_pending, true);
    }
}

/*
 * Host sends SET_CONTROL_LINE_STATE when a terminal opens the port; send
 * carrier again so the driver refreshes (helps after failed notify at mount).
 */
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;
    (void)dtr;
    (void)rts;

    if (!atomic_load(&usb_mounted)) {
        return;
    }

    usb_serial_send_carrier_notify();
}

/*
 * Host often sends SET_LINE_CODING during open(); piggyback carrier notify so
 * cdc-acm updates before or alongside SET_CONTROL_LINE_STATE.
 */
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding)
{
    (void)itf;
    (void)p_line_coding;

    if (!atomic_load(&usb_mounted)) {
        return;
    }

    usb_serial_send_carrier_notify();
}
#endif

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
    while (!(PWR->CR3 & PWR_CR3_USB33RDY)) {
    }

    /* Enable GPIOA clock (AHB4 on STM32H7) and configure USB_DN/USB_DP */
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
    __DSB();

    gpio_setMode(USB_DN, ALTERNATE | ALTERNATE_FUNC(10));
    gpio_setMode(USB_DP, ALTERNATE | ALTERNATE_FUNC(10));
    gpio_setOutputSpeed(USB_DN, HIGH);
    gpio_setOutputSpeed(USB_DP, HIGH);

    /* Enable HSI48 and wait for it to be ready */
    RCC->CR |= RCC_CR_HSI48ON;
    while ((RCC->CR & RCC_CR_HSI48RDY) == 0) {
    }

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
    RCC->AHB1RSTR |= RCC_AHB1RSTR_USB2OTGFSRST;
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

    if (!tusb_init()) {
        return;
    }

    /*
     * Disable buffering on stdout.  usb_serial_write() manages its own
     * blocking loop; a libc-level buffer would only add latency.
     */
    (void)setvbuf(stdout, NULL, _IONBF, 0);

    atomic_store(&usb_ready, true);
}

void usb_serial_terminate(void)
{
    NVIC_DisableIRQ(OTG_FS_IRQn);
    RCC->AHB1ENR &= ~RCC_AHB1ENR_USB2OTGFSEN;
    __DSB();
    atomic_store(&usb_mounted, false);
    atomic_store(&usb_ready, false);
    atomic_store(&cdc_uart_notify_pending, false);
}

void usb_serial_task(void)
{
    pthread_mutex_lock(&cdc_mutex);

    /* Process USB events (mount/unmount, receive data, transfer completions). */
    tud_task();

#if CFG_TUD_CDC_NOTIFY
    if (atomic_load(&usb_mounted)) {
        if (carrier_boot_notify_remaining > 0) {
            carrier_boot_notify_remaining--;
            usb_serial_send_carrier_notify();
        } else if (atomic_load(&cdc_uart_notify_pending)) {
            usb_serial_send_carrier_notify();
        }
    }
#endif

    /*
     * Flush any bytes still in the CDC TX FIFO that were not yet committed to
     * an IN transfer.  This happens when usb_serial_write() called
     * tud_cdc_write_flush() while the endpoint was already busy; that flush was
     * a no-op.  After tud_task() processes the transfer-complete event, a
     * second flush here picks up the residual bytes (often the trailing "\r\n"
     * from _write_r after the main text chunk).
     */
    if (atomic_load(&usb_mounted)) {
        tud_cdc_write_flush();
    }
    pthread_mutex_unlock(&cdc_mutex);
}

bool usb_serial_connected(void)
{
    return atomic_load(&usb_mounted);
}

uint32_t usb_serial_available(void)
{
    if (!atomic_load(&usb_ready)) {
        return 0;
    }

    return tud_cdc_available();
}

ssize_t usb_serial_write(const void *buf, size_t len)
{
    /*
     * Return immediately if the USB device is not mounted.
     * This avoids blocking the caller when the cable is unplugged.
     */
    if (!atomic_load(&usb_mounted)) {
        return -1;
    }

    const uint8_t *p = (const uint8_t *)buf;
    size_t rem = len;
    uint32_t timeout = 50; /* ~50 ms: 50 retries * 1 ms sleep */

    pthread_mutex_lock(&cdc_mutex);

    /*
     * If the link is in USB suspend (host idle / selective suspend), IN
     * traffic may not run until the bus resumes.  Advertise remote wakeup in
     * the configuration descriptor and pulse here when we have data to send.
     */
    if (len > 0 && tud_suspended()) {
        (void)tud_remote_wakeup();
    }

    while (rem > 0) {
        /* Bail out if the host disconnects mid-transfer */
        if (!atomic_load(&usb_mounted)) {
            tud_cdc_write_clear();
            pthread_mutex_unlock(&cdc_mutex);
            return -1;
        }

        uint32_t accepted = tud_cdc_write(p, rem);
        p += accepted;
        rem -= accepted;

        if (rem == 0) {
            break;
        }

        /*
         * The CDC TX FIFO is full (64 bytes at FS speed).  Flush what has
         * been accepted, release the mutex so usb_serial_task() can run
         * during the sleep, then re-acquire and pump the stack ourselves.
         * tud_task() is called *inside* the mutex so it cannot execute
         * concurrently with usb_serial_task()'s tud_task() call; concurrent
         * calls corrupt tinyUSB state and cause hangs on USB disconnect.
         */
        tud_cdc_write_flush();

        if (timeout == 0) {
            tud_cdc_write_clear();
            pthread_mutex_unlock(&cdc_mutex);
            return -1;
        }
        timeout--;

        pthread_mutex_unlock(&cdc_mutex);
        delayMs(1); /* yield; allow the ISR to queue the xfer-complete event */
        pthread_mutex_lock(&cdc_mutex);

        if (tud_suspended()) {
            (void)tud_remote_wakeup();
        }

        tud_task(); /* process events under the mutex — cannot race with
                         * usb_serial_task(), preventing concurrent tud_task()
                         * calls that corrupt tinyUSB state on USB disconnect */
    }

    tud_cdc_write_flush();
    pthread_mutex_unlock(&cdc_mutex);
    return (ssize_t)len;
}

ssize_t usb_serial_read(void *buf, size_t len)
{
    if (!atomic_load(&usb_ready)) {
        return -1;
    }

    if (!atomic_load(&usb_mounted)) {
        return -1;
    }

    return (ssize_t)tud_cdc_read(buf, len);
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
    atomic_store(&cdc_uart_notify_pending, false);
#if CFG_TUD_CDC_NOTIFY
    carrier_boot_notify_remaining = 8;

    /*
     * Signal carrier present (DCD=1, DSR=1) to the host via a CDC
     * SERIAL_STATE notification on the interrupt endpoint.
     *
     * The Linux cdc-acm driver updates its carrier state when it receives
     * this notification.  Without it, the driver never asserts DCD and any
     * open() call on /dev/ttyACMx that does not pass O_NONBLOCK blocks
     * indefinitely (or for a long kernel timeout) waiting for carrier.
     * This is why stty and picocom hang for ~11 s after the device enumerates.
     *
     * tud_mount_cb() is called from within tud_task(), so the cdc_mutex is
     * already held; tud_cdc_notify_uart_state() uses only internal tinyUSB
     * state (no mutex acquisition) and is safe to call here.
     */
    usb_serial_send_carrier_notify();
#endif
}

void tud_umount_cb(void)
{
    atomic_store(&usb_mounted, false);
    atomic_store(&cdc_uart_notify_pending, false);
#if CFG_TUD_CDC_NOTIFY
    carrier_boot_notify_remaining = 0;
#endif
}

/*
 * Do not override tud_suspend_cb() / tud_resume_cb().
 *
 * tinyUSB's stack documents that during enumeration the D+/D- lines can
 * momentarily meet the USB "suspend" condition (bus idle for 3 ms).  Clearing
 * application state on suspend therefore races normal control transfers
 * (SET_LINE_CODING, SET_CONTROL_LINE_STATE) and was flipping usb_mounted off
 * and back on, breaking host tools and motivating fragile resume workarounds.
 *
 * Physical disconnect is reported as DCD_EVENT_UNPLUGGED -> tud_umount_cb().
 * While the device remains configured but the host is not polling IN (e.g.
 * real bus suspend), usb_serial_write() may wait for FIFO space; the bounded
 * retry loop (~50 ms) prevents indefinite blocking.
 */
