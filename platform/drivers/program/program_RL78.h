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

#ifndef PROGRAM_RL78_H
#define PROGRAM_RL78_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#define RL78_CMD_RESET 0x00
#define RL78_CMD_VERIFY 0x13
#define RL78_CMD_BLOCK_ERASE 0x22
#define RL78_CMD_BLOCK_BLANK_CHECK 0x32
#define RL78_CMD_PROGRAMMING 0x40
#define RL78_CMD_BAUD_RATE_SET 0x9A
#define RL78_CMD_SILICON_SIGNATURE 0xC0
#define RL78_CMD_SECURITY_SET 0xA0
#define RL78_CMD_SECURITY_GET 0xA1
#define RL78_CMD_SECURITY_RELEASE 0xA2
#define RL78_CMD_CHECKSUM 0xB0

#define RL78_STATUS_COMMAND_NUMBER_ERROR 0x04
#define RL78_STATUS_PARAMETER_ERROR 0x05
#define RL78_STATUS_ACK 0x06
#define RL78_STATUS_CHECKSUM_ERROR 0x07
#define RL78_STATUS_VERIFY_ERROR 0x0F
#define RL78_STATUS_PROTECT_ERROR 0x10
#define RL78_STATUS_NACK 0x15
#define RL78_STATUS_ERASE_ERROR 0x1A
#define RL78_STATUS_IVERIFY_BLANK_ERROR 0x1B
#define RL78_STATUS_WRITE_ERROR 0x1C

#define RL78_SOH 0x01
#define RL78_STX 0x02
#define RL78_ETX 0x03
#define RL78_ETB 0x17

#define RL78_BAUD_115200 0x00
#define RL78_BAUD_250000 0x01
#define RL78_BAUD_500000 0x02
#define RL78_BAUD_1000000 0x03

#define RL78_CODE_OFFSET (0U)
#define RL78_DATA_OFFSET (0x000F1000U)

#define RL78_MAX_LENGTH 32

#define RESPONSE_OK (0)
#define RESPONSE_CHECKSUM_ERROR (-1)
#define RESPONSE_FORMAT_ERROR (-2)
#define RESPONSE_EXPECTED_LENGTH_ERROR (-3)

#define RL78_UART_MODE_1WIRE 0x3A
#define RL78_UART_MODE_2WIRE 0x00

#define RL78_MIN_VOLTAGE 1.8f
#define RL78_MAX_VOLTAGE 5.5f

/**
 * Flash RL78 with an embedded firmware image.
 */
int rl78_flash(const uint8_t *addr, const uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* PROGRAM_RL78_H */
