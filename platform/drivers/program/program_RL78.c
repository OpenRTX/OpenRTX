/***************************************************************************
 * The MIT License (MIT)                                                   *
 * Copyright (c) 2012-2016, 2022 Maksim Salau                              *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files                *
 * (the "Software"), to deal in the Software without restriction,          *
 * including without limitation the rights to use, copy, modify, merge,    *
 * publish, distribute, sublicense, and/or sell copies of the Software,    *
 * and to permit persons to whom the Software is furnished to do so,       *
 * subject to the following conditions:                                    *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/

#include "program_RL78.h"

#include <interfaces/delays.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#if DT_NODE_HAS_STATUS(DT_ALIAS(radio_prg), okay)
#define UART_RADIO_PROG_DEV_NODE DT_ALIAS(radio_prg)
#else
#error "Please select the correct radio programmer UART device"
#endif

#define RL78_UART_MODE RL78_UART_MODE_1WIRE

#define RADIO_PRG_RESET_NODE DT_NODELABEL(radio_prg_reset)
#define RADIO_PRG_TOOL0_NODE DT_NODELABEL(radio_prg_tool0)

static const struct device* const prog_dev =
    DEVICE_DT_GET(UART_RADIO_PROG_DEV_NODE);

static const struct gpio_dt_spec radio_prg_reset =
    GPIO_DT_SPEC_GET_OR(RADIO_PRG_RESET_NODE, gpios, {0});
static const struct gpio_dt_spec radio_prg_tool0 =
    GPIO_DT_SPEC_GET_OR(RADIO_PRG_TOOL0_NODE, gpios, {0});

static struct
{
    char buf[RL78_MAX_LENGTH];
    int pos;
} msg;

static uint8_t tmp;

static int checksum(const void* data, int len)
{
    unsigned int sum       = 0;
    const unsigned char* p = data;
    for (; len; --len)
    {
        sum -= *p++;
    }
    return sum & 0x00FF;
}

void rl78_send_cmd(int cmd, const void* data, int len)
{
    if ((255 < len) || (!data && len)) return;

    uint8_t buf[len + 5];
    buf[0] = RL78_SOH;
    buf[1] = (len + 1) & 0xFFU;
    buf[2] = cmd;
    if (len) memcpy(&buf[3], data, len);
    buf[len + 3] = checksum(&buf[1], len + 2);
    buf[len + 4] = RL78_ETX;

    for (int i = 0; i < sizeof(buf); i++)
    {
        uart_poll_out(prog_dev, buf[i]);
        printk("RL78: TX %02X\n", buf[i]);
    }

#if RL78_UART_MODE == RL78_UART_MODE_1WIRE
    // Discard echo
    for (int i = 0; i < sizeof(buf); i++)
    {
        uart_poll_in(prog_dev, &tmp);
        delayUs(2500);
        // printk("RL78: RX %02X (echo)\n", tmp);
    }
#endif
}

void rl78_send_data(const uint8_t* data, int len, int last)
{
    if (256 < len) return;

    uint8_t buf[len + 4];
    buf[0] = RL78_STX;
    buf[1] = len & 0xFFU;
    memcpy(&buf[2], data, len);
    buf[len + 2] = checksum(&buf[1], len + 1);
    buf[len + 3] = last ? RL78_ETX : RL78_ETB;

    for (int i = 0; i < sizeof(buf); i++)
    {
        uart_poll_out(prog_dev, buf[i]);
        printk("RL78: TX %02X\n", buf[i]);
#if RL78_UART_MODE == RL78_UART_MODE_1WIRE
        delayUs(2500);

        // Discard echo
        uart_poll_in(prog_dev, &tmp);
        // printk("RL78: RX %02X (echo)\n", tmp);
#endif
    }
}

int rl78_recv(void* data, int* len, int expected)
{
    // Receive header
    uart_poll_in(prog_dev, &msg.buf[0]);
    uart_poll_in(prog_dev, &msg.buf[1]);

    printk("RL78: RX %02X\n", msg.buf[0]);
    printk("RL78: RX %02X\n", msg.buf[1]);

    int data_len = msg.buf[1];

    if (0 == data_len) data_len = 256;

    if ((RL78_MAX_LENGTH - 4) < data_len || RL78_STX != msg.buf[0])
        return RESPONSE_FORMAT_ERROR;

    if (expected != data_len) return RESPONSE_EXPECTED_LENGTH_ERROR;

    // Receive data field, checksum and footer byte
    for (int i = 2; i < data_len + 4; i++)
    {
        uart_poll_in(prog_dev, &msg.buf[i]);
        printk("RL78: RX %02X\n", msg.buf[i]);
    }

    switch (msg.buf[data_len + 3])
    {
        case RL78_ETB:
        case RL78_ETX:
            break;
        default:
            return RESPONSE_FORMAT_ERROR;
    }

    if (checksum(msg.buf + 1, data_len + 1) != msg.buf[data_len + 2])
        return RESPONSE_CHECKSUM_ERROR;

    memcpy(data, &msg.buf[2], data_len);
    *len = data_len;

    return RESPONSE_OK;
}

int rl78_cmd_block_blank_check(uint32_t address_start, uint32_t address_end)
{
    printk("RL78: blank check range 0x%06x - 0x%06x\n", address_start,
           address_end);

    memcpy(&msg.buf[0], &address_start, 3);
    memcpy(&msg.buf[3], &address_end, 3);
    msg.buf[6] = 0;
    msg.pos    = 7;

    rl78_send_cmd(RL78_CMD_BLOCK_BLANK_CHECK, msg.buf, msg.pos);

    delayMs(6);

    int ret = rl78_recv(msg.buf, &msg.pos, 1);
    if (RESPONSE_OK != ret)
    {
        printk("RL78: Block blank check failed\n");
        return ret;
    }
    if (RL78_STATUS_ACK != msg.buf[0] &&
        RL78_STATUS_IVERIFY_BLANK_ERROR != msg.buf[0])
    {
        printk("RL78: ACK not received\n");
        return msg.buf[0];
    }
    if (RL78_STATUS_ACK == msg.buf[0])
    {
        ret = 0;
    }
    else
    {
        ret = 1;
    }

    return ret;
}

int rl78_cmd_block_erase(uint32_t address)
{
    printk("RL78: erase block 0x%06x - 0x%06x\n", address, address + 1024);

    rl78_send_cmd(RL78_CMD_BLOCK_ERASE, &address, 3);

    delayMs(6);

    int ret = rl78_recv(msg.buf, &msg.pos, 1);
    if (RESPONSE_OK != ret)
    {
        printk("RL78: block erase failed\n");
        return ret;
    }

    if (RL78_STATUS_ACK != msg.buf[0])
    {
        printk("RL78: ACK not received\n");
        return msg.buf[0];
    }

    return 0;
}

int rl78_cmd_programming(uint32_t address_start,
                         uint32_t address_end,
                         const uint8_t* rom)
{
    printk("RL78: programming range 0x%06x - 0x%06x\n", address_start,
           address_end);

    memcpy(&msg.buf[0], &address_start, 3);
    memcpy(&msg.buf[3], &address_end, 3);
    msg.pos = 6;

    rl78_send_cmd(RL78_CMD_PROGRAMMING, msg.buf, msg.pos);

    delayMs(1);

    int ret = rl78_recv(msg.buf, &msg.pos, 1);
    if (RESPONSE_OK != ret)
    {
        printk("RL78: programming failed (no response)\n");
        return ret;
    }
    if (RL78_STATUS_ACK != msg.buf[0])
    {
        printk("RL78: ACK not received\n");
        return msg.buf[0];
    }

    delayMs(20);

    uint32_t rom_length      = address_end - address_start + 1;
    uint32_t address_current = address_start;
    uint32_t final_delay     = (rom_length / 1024 + 1) * 2000;

    // Send data
    while (rom_length)
    {
        printk("RL78: write address 0x%06x\n", address_current);

        if (256 < rom_length)
        {
            // Not last data frame
            rl78_send_data(rom, 256, 0);
            address_current += 256;
            rom += 256;
            rom_length -= 256;
        }
        else
        {
            // Last data frame
            rl78_send_data(rom, rom_length, 1);
            address_current += rom_length;
            rom += rom_length;
            rom_length -= rom_length;

            delayMs(8);
        }

        ret = rl78_recv(msg.buf, &msg.pos, 2);
        if (RESPONSE_OK != ret)
        {
            printk("RL78: programming failed (bad response for block)\n");
            return ret;
        }
        if (RL78_STATUS_ACK != msg.buf[0])
        {
            printk("RL78: ACK not received\n");
            return msg.buf[0];
        }
        if (RL78_STATUS_ACK != msg.buf[1])
        {
            printk("RL78: data not written\n");
            return msg.buf[1];
        }
    }

    delayUs(final_delay);

    // Protocol A and D require this packet, C doesn't send it
    ret = rl78_recv(msg.buf, &msg.pos, 1);
    if (RESPONSE_OK != ret)
    {
        printk("RL78: programming failed (response not ok)\n");
        return ret;
    }
    if (RL78_STATUS_ACK != msg.buf[0])
    {
        printk("RL78: ACK not received\n");
        return msg.buf[0];
    }

    return ret;
}

void rl78_program(uint32_t address,
                  const uint8_t* data,
                  uint32_t size,
                  uint32_t blocksize)
{
    int ret = 0;

    // Make sure size is aligned to flash block boundary
    for (uint32_t i = size & ~(blocksize - 1); i; i -= blocksize)
    {
        // Check if block is ready to program new content
        // ret = rl78_cmd_block_blank_check(address, address + blocksize - 1);
        // if (0 > ret)
        // {
        //     printk("RL78: block blank check failed (%06X)\n", address);
        //     break;
        // }
        // if (0 < ret)
        // {
        // If block is not empty - erase it
        ret = rl78_cmd_block_erase(address);
        if (0 > ret)
        {
            printk("RL78: block erase failed (%06X)\n", address);
            break;
        }
        // }

        // Write new content
        ret = rl78_cmd_programming(address, address + blocksize - 1, data);
        if (0 > ret)
        {
            printk("RL78: programming failed (%06X)\n", address);
            break;
        }

        data += blocksize;
        address += blocksize;
    }
}

int rl78_flash(const uint8_t* addr, const uint32_t size)
{
    int ret;

    struct uart_config uart_config = {
        .baudrate  = 2400,
        .parity    = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
        .data_bits = UART_CFG_DATA_BITS_8,
    };

    // Initialize GPIO for RL78 reset
    if (!gpio_is_ready_dt(&radio_prg_reset))
    {
        printk("RL78: reset GPIO %s is not ready\n",
               radio_prg_reset.port->name);
        return -1;
    }

    if ((ret = gpio_pin_configure_dt(&radio_prg_reset, GPIO_OUTPUT)))
    {
        printk("RL78: error %d, failed to configure %s pin %d\n", ret,
               radio_prg_reset.port->name, radio_prg_reset.pin);
        return -1;
    }

    // Initialize UART for SA868 RL78 programming
    if (!device_is_ready(prog_dev))
    {
        printk("RL78: UART programming device not ready!\n");
        return -1;
    }

    if ((ret = uart_configure(prog_dev, &uart_config)))
    {
        printk("RL78: error while setting UART configuration!\n");
        return ret;
    }

    printk("RL78: entering bootloader mode...\n");

    delayMs(1);
    uart_poll_out(prog_dev, 0x00);
    delayMs(1);
    uart_poll_in(prog_dev, &tmp);

    if ((ret = gpio_pin_configure_dt(&radio_prg_reset, GPIO_INPUT)))
    {
        printk("RL78: error %d, failed to configure %s pin %d\n", ret,
               radio_prg_reset.port->name, radio_prg_reset.pin);
        return ret;
    }

    delayMs(4);

    uart_config.baudrate = 115200;

    if ((ret = uart_configure(prog_dev, &uart_config)))
    {
        printk("RL78: error while setting UART configuration!\n");
        return ret;
    }

    // Request communication mode: single-pin UART
    uart_poll_out(prog_dev, RL78_UART_MODE_1WIRE);
    delayMs(1);
    uart_poll_in(prog_dev, &tmp);

    delayMs(12);

    // Request symbol rate and voltage
    msg.buf[0] = RL78_BAUD_115200;
    msg.buf[1] = 33;  // 3.3V * 10
    msg.pos    = 2;
    rl78_send_cmd(RL78_CMD_BAUD_RATE_SET, msg.buf, msg.pos);
    delayMs(6);
    ret = rl78_recv(msg.buf, &msg.pos, 3);
    if (RESPONSE_OK != ret)
    {
        printk("RL78: unexpected response: %d\n", ret);
        return -1;
    }
    if (RL78_STATUS_ACK != msg.buf[0])
    {
        printk("RL78: missing acknowledge in response\n");
        return -1;
    }

    rl78_send_cmd(RL78_CMD_RESET, NULL, 0);
    delayMs(6);
    ret = rl78_recv(msg.buf, &msg.pos, 1);
    if (RESPONSE_OK != ret)
    {
        printk("RL78: unexpected response: %d\n", ret);
        return -1;
    }
    if (RL78_STATUS_ACK != msg.buf[0])
    {
        printk("RL78: missing acknowledge in response\n");
        return -1;
    }
    delayMs(12);

    rl78_send_cmd(RL78_CMD_SILICON_SIGNATURE, NULL, 0);
    ret = rl78_recv(msg.buf, &msg.pos, 1);
    if (RESPONSE_OK != ret)
    {
        printk("RL78: unexpected response: %d\n", ret);
        return -1;
    }
    if (RL78_STATUS_ACK != msg.buf[0])
    {
        printk("RL78: missing acknowledge in response\n");
        return -1;
    }

    ret = rl78_recv(msg.buf, &msg.pos, 22);
    if (RESPONSE_OK != ret)
    {
        printk("RL78: unexpected response: %d\n", ret);
        return -1;
    }

    char name[11] = {0};
    memcpy(name, &msg.buf[3], 10);

    printk("RL78: identified %s\n", name);

    delayMs(12);

    rl78_program(0x00000000, addr, size, 1024);

    if ((ret = gpio_pin_configure_dt(&radio_prg_reset, GPIO_OUTPUT)))
    {
        printk("RL78: error %d, failed to configure %s pin %d\n", ret,
               radio_prg_reset.port->name, radio_prg_reset.pin);
        return -1;
    }

    delayMs(12);

    if ((ret = gpio_pin_configure_dt(&radio_prg_reset, GPIO_INPUT)))
    {
        printk("RL78: error %d, failed to configure %s pin %d\n", ret,
               radio_prg_reset.port->name, radio_prg_reset.pin);
        return -1;
    }

    printk("RL78: firmware flash complete!\n");

    return 0;
}
