/*
 * Copyright(c) 2023 LISTENAI
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dsp, LOG_LEVEL_DBG);

#include <csk6_cm33/include/venus_ap.h>
#include <csk6_cm33/include/cache.h>
#include <csk6_cm33/include/ClockManager.h>
#include <csk6_cm33/include/SysManager.h>

#define AP_SYS_RAM_BANK_IDX 4
#define AP_SYS_RAM_BANK_SIZE 0x00010000
#define AP_SYS_RAM_BANK_ADDR \
    (SYS_RAM_BASE + AP_SYS_RAM_BANK_SIZE * AP_SYS_RAM_BANK_IDX)

/* Code and data addresses can refer to the same SRAM through aliases
 * differing by bit 29. Normalize both before comparing their ranges.
 */
#define AP_RAM_CANONICAL_ADDR(addr) ((addr) & ~0x20000000UL)
#define AP_RAM_START AP_RAM_CANONICAL_ADDR(CONFIG_SRAM_BASE_ADDRESS)
#define AP_RAM_END (AP_RAM_START + CONFIG_SRAM_SIZE * 1024UL)
#define DSP_RAM_START AP_RAM_CANONICAL_ADDR(AP_SYS_RAM_BANK_ADDR)
#define DSP_RAM_END (DSP_RAM_START + AP_SYS_RAM_BANK_SIZE)

/* Fail the build if application SRAM includes the DSP's shared audio bank.
 * This checks the configured layout; it does not reserve memory at runtime.
 */
BUILD_ASSERT(AP_RAM_END <= DSP_RAM_START || AP_RAM_START >= DSP_RAM_END,
             "Application SRAM overlaps DSP shared audio bank 4");

#define AP_PSRAM_BASE (0x30000000)
#define CP_PSRAM_BASE (0x60000000)

#define AP_FLASH_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_flash))

#if defined(CONFIG_LSF_DSP_LOAD_ON_BOOT)
#define DSP_FIRMWARE_NODE DT_CHOSEN(lsf_dsp_firmware)
#endif /* CONFIG_LSF_DSP_LOAD_ON_BOOT */

void lsf_dsp_load(const void *addr, uint32_t size)
{
    /* Enable clock for DSP */
    __HAL_CRM_CP_CLK_ENABLE();

    /* Set boot address */
    __HAL_SYS_CP_SET_BOOT_ADDR(CP_PSRAM_BASE);

    /* Stall DSP */
    __HAL_SYS_CP_STALL();

    if (__HAL_SYS_IS_CP_RESET()) {
        __HAL_SYS_CP_RELEASE();
    }

    if (__HAL_SYS_NPU_IS_RESET()) {
        __HAL_SYS_NPU_RELEASE();
    }

    __DMB();

    /* Load DSP firmware to boot address */
    memcpy((void *)AP_PSRAM_BASE, addr, size);
    dcache_clean_range(AP_PSRAM_BASE, AP_PSRAM_BASE + size);

    __DSB();

    /* Reset DSP */
    __HAL_SYS_CP_RESET();
    __HAL_SYS_CP_RELEASE();

    /* Run it */
    __HAL_SYS_CP_RUN();
}

static int lsf_dsp_init(void)
{
#if !defined(CONFIG_UART_INTERRUPT_DRIVEN) && !defined(CONFIG_UART_ASYNC_API)
    /* DSP logging can trigger UART2 IRQ 10 on the application core.
     * With a polling console there is no handler, so Zephyr halts at boot.
     * Mask the AP interrupt before starting the DSP; UART output stays active.
     */
    irq_disable(DT_IRQN(DT_NODELABEL(uart2)));
#endif

    /* Disable clocks */
    __HAL_CRM_CP_CLK_DISABLE();
    __HAL_CRM_NPU_CLK_DISABLE();

    /* Stall DSP */
    __HAL_SYS_CP_STALL();

    /* Configure share memory bank 4 to be accessed by AHB-S */
    IP_SYSCTRL->REG_AP_CTRL1.bit.AP_RAM_SEL = (1 << AP_SYS_RAM_BANK_IDX);

    /* Zero memory bank 4 */
    memset((void *)AP_SYS_RAM_BANK_ADDR, 0x00, AP_SYS_RAM_BANK_SIZE);
    dcache_invalidate_range(AP_SYS_RAM_BANK_ADDR,
                            AP_SYS_RAM_BANK_ADDR + AP_SYS_RAM_BANK_SIZE);

#if defined(CONFIG_LSF_DSP_LOAD_ON_BOOT)
    LOG_DBG("Auto load DSP firmware from %s (offset: 0x%08x, size: %u)",
            DT_LABEL(DSP_FIRMWARE_NODE), DT_REG_ADDR(DSP_FIRMWARE_NODE),
            DT_REG_SIZE(DSP_FIRMWARE_NODE));

    /* Load DSP firmware */
    lsf_dsp_load((void *)(AP_FLASH_BASE + DT_REG_ADDR(DSP_FIRMWARE_NODE)),
                 DT_REG_SIZE(DSP_FIRMWARE_NODE));
#endif /* CONFIG_LSF_DSP_LOAD_ON_BOOT */

    return 0;
}

SYS_INIT(lsf_dsp_init, POST_KERNEL, CONFIG_LSF_DSP_INIT_PRIORITY);
