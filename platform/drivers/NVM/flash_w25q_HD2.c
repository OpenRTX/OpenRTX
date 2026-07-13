/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Shared W25Q512 access for the HD2 (see flash_w25q_HD2.h).  The primitives
 * here are the originals from nvmem_settings_HD2.c (live-verified settings
 * save/load); the per-driver copies now call into this.
 */

#include "flash_w25q_HD2.h"
#include "drivers/SPI/spi_hd2.h"
#include "hd2_regs.h"                /* GPIOA_DR + hd2_irq_save/restore (inline) */

#define FLASH_CS_MASK   (1u << 18)            /* CS# = GPIOA.18, active low */
#define MMIO_SYNC()     __asm__ volatile ("sync" ::: "memory")

#define PAGE_SIZE       256u
#define OPC_WREN        0x06u
#define OPC_RDSR1       0x05u
#define OPC_JEDEC_ID    0x9Fu
#define OPC_WAKEUP      0xABu
#define OPC_READ_4B     0x13u
#define OPC_PROG_4B     0x12u
#define OPC_ERASE_4K_4B 0x21u

/* One IRQ-locked SPI transaction: CS low, send cmd[] (+ optional tx), then
 * (optional) clock in rx, CS high. */
static void flashXfer(const uint8_t *cmd, size_t cmdLen,
                      const void *txData, size_t txLen,
                      void *rxData, size_t rxLen)
{
    uint32_t irq = hd2_irq_save();
    GPIOA_DR &= ~FLASH_CS_MASK; MMIO_SYNC();
    nvm_spi.transfer(&nvm_spi, cmd, NULL, cmdLen);
    if(txLen > 0) nvm_spi.transfer(&nvm_spi, txData, NULL, txLen);
    if(rxLen > 0) nvm_spi.transfer(&nvm_spi, NULL, rxData, rxLen);
    GPIOA_DR |= FLASH_CS_MASK; MMIO_SYNC();
    hd2_irq_restore(irq);
}

static int flashWaitReady(uint32_t timeoutMs)
{
    uint32_t ticks = timeoutMs * 2;           /* ~500 us per poll */
    while(ticks > 0)
    {
        uint8_t cmd = OPC_RDSR1, sr = 0xFF;
        flashXfer(&cmd, 1, NULL, 0, &sr, 1);
        if((sr & 0x01u) == 0u) return 0;      /* WIP clear */
        for(volatile unsigned i = 0; i < 4000u; ++i) { }
        ticks--;
    }
    return -1;
}

static void flashWriteEnable(void)
{
    uint8_t cmd = OPC_WREN;
    flashXfer(&cmd, 1, NULL, 0, NULL, 0);
}

bool w25q_hd2_probe(void)
{
    spi_hd2_init();
    uint8_t cmd = OPC_WAKEUP;                  /* release from deep power-down */
    flashXfer(&cmd, 1, NULL, 0, NULL, 0);
    for(volatile unsigned i = 0; i < 8000u; ++i) { }   /* tRES2 settle */
    uint8_t id[3] = { 0, 0, 0 };
    cmd = OPC_JEDEC_ID;
    flashXfer(&cmd, 1, NULL, 0, id, 3);
    return (id[0] == 0xEF) && (id[1] == 0x40) && (id[2] == 0x20);
}

void w25q_hd2_read(uint32_t addr, void *buf, size_t len)
{
    const uint8_t cmd[] = { OPC_READ_4B,
        (uint8_t)(addr >> 24), (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),  (uint8_t)(addr) };
    flashXfer(cmd, sizeof(cmd), NULL, 0, buf, len);
}

int w25q_hd2_eraseSector(uint32_t addr)
{
    const uint8_t cmd[] = { OPC_ERASE_4K_4B,
        (uint8_t)(addr >> 24), (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),  (uint8_t)(addr) };
    flashWriteEnable();
    flashXfer(cmd, sizeof(cmd), NULL, 0, NULL, 0);
    return flashWaitReady(500);               /* sector erase: typ 45 ms */
}

int w25q_hd2_program(uint32_t addr, const void *buf, size_t len)
{
    const uint8_t *data = (const uint8_t *) buf;
    while(len > 0)
    {
        size_t pageRoom = PAGE_SIZE - (addr & (PAGE_SIZE - 1));
        size_t chunk    = (len < pageRoom) ? len : pageRoom;
        const uint8_t cmd[] = { OPC_PROG_4B,
            (uint8_t)(addr >> 24), (uint8_t)(addr >> 16),
            (uint8_t)(addr >> 8),  (uint8_t)(addr) };
        flashWriteEnable();
        flashXfer(cmd, sizeof(cmd), data, chunk, NULL, 0);
        if(flashWaitReady(10) < 0) return -1;  /* page program: typ 0.4 ms */
        addr += chunk; data += chunk; len -= chunk;
    }
    return 0;
}
