/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __USB_MISC_H__
#define __USB_MISC_H__

#ifndef ENDIANNESS

#error ENDIANNESS should be defined, and then rebulid the project.

#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Define USB printf */
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

extern int DbgConsole_Printf(const char *fmt_s, ...);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#if defined(SDK_DEBUGCONSOLE) && (SDK_DEBUGCONSOLE < 1)
#define usb_echo printf
#else
#define usb_echo
#endif

#if defined(__ICCARM__)

#ifndef STRUCT_PACKED
#define STRUCT_PACKED __packed
#endif

#ifndef STRUCT_UNPACKED
#define STRUCT_UNPACKED
#endif

#elif defined(__GNUC__)

#ifndef STRUCT_PACKED
#define STRUCT_PACKED
#endif

#ifndef STRUCT_UNPACKED
#define STRUCT_UNPACKED __attribute__((__packed__))
#endif

#elif defined(__CC_ARM)

#ifndef STRUCT_PACKED
#define STRUCT_PACKED _Pragma("pack(1U)")
#endif

#ifndef STRUCT_UNPACKED
#define STRUCT_UNPACKED _Pragma("pack()")
#endif

#endif

#define USB_SHORT_GET_LOW(x) (((uint16_t)x) & 0xFFU)
#define USB_SHORT_GET_HIGH(x) ((uint8_t)(((uint16_t)x) >> 8U) & 0xFFU)

#define USB_LONG_GET_BYTE0(x) ((uint8_t)(((uint32_t)(x))) & 0xFFU)
#define USB_LONG_GET_BYTE1(x) ((uint8_t)(((uint32_t)(x)) >> 8U) & 0xFFU)
#define USB_LONG_GET_BYTE2(x) ((uint8_t)(((uint32_t)(x)) >> 16U) & 0xFFU)
#define USB_LONG_GET_BYTE3(x) ((uint8_t)(((uint32_t)(x)) >> 24U) & 0xFFU)

#define USB_MEM4_ALIGN_MASK (0x03U)

/* accessory macro */
#define USB_MEM4_ALIGN(n) ((n + 3U) & (0xFFFFFFFCu))
#define USB_MEM32_ALIGN(n) ((n + 31U) & (0xFFFFFFE0u))
#define USB_MEM64_ALIGN(n) ((n + 63U) & (0xFFFFFFC0u))

/* big/little endian */
#define SWAP2BYTE_CONST(n) ((((n)&0x00FFU) << 8U) | (((n)&0xFF00U) >> 8U))
#define SWAP4BYTE_CONST(n) \
    ((((n)&0x000000FFU) << 24U) | (((n)&0x0000FF00U) << 8U) | (((n)&0x00FF0000U) >> 8U) | (((n)&0xFF000000U) >> 24U))

#define USB_ASSIGN_VALUE_ADDRESS_LONG_BY_BYTE(n, m)      \
    {                                                    \
        *((uint8_t *)&(n)) = *((uint8_t *)&(m));         \
        *((uint8_t *)&(n) + 1) = *((uint8_t *)&(m) + 1); \
        *((uint8_t *)&(n) + 2) = *((uint8_t *)&(m) + 2); \
        *((uint8_t *)&(n) + 3) = *((uint8_t *)&(m) + 3); \
    }

#define USB_ASSIGN_VALUE_ADDRESS_SHORT_BY_BYTE(n, m)     \
    {                                                    \
        *((uint8_t *)&(n)) = *((uint8_t *)&(m));         \
        *((uint8_t *)&(n) + 1) = *((uint8_t *)&(m) + 1); \
    }

#define USB_ASSIGN_MACRO_VALUE_ADDRESS_LONG_BY_BYTE(n, m) \
    {                                                     \
        *((uint8_t *)&(n)) = (uint8_t)m;                  \
        *((uint8_t *)&(n) + 1) = (uint8_t)(m >> 8);       \
        *((uint8_t *)&(n) + 2) = (uint8_t)(m >> 16);      \
        *((uint8_t *)&(n) + 3) = (uint8_t)(m >> 24);      \
    }

#define USB_ASSIGN_MACRO_VALUE_ADDRESS_SHORT_BY_BYTE(n, m) \
    {                                                      \
        *((uint8_t *)&(n)) = (uint8_t)m;                   \
        *((uint8_t *)&(n) + 1) = (uint8_t)(m >> 8);        \
    }

#if (ENDIANNESS == USB_BIG_ENDIAN)

#define USB_SHORT_TO_LITTLE_ENDIAN(n) SWAP2BYTE_CONST(n)
#define USB_LONG_TO_LITTLE_ENDIAN(n) SWAP4BYTE_CONST(n)
#define USB_SHORT_FROM_LITTLE_ENDIAN(n) SWAP2BYTE_CONST(n)
#define USB_LONG_FROM_LITTLE_ENDIAN(n) SWAP2BYTE_CONST(n)

#define USB_SHORT_TO_BIG_ENDIAN(n) (n)
#define USB_LONG_TO_BIG_ENDIAN(n) (n)
#define USB_SHORT_FROM_BIG_ENDIAN(n) (n)
#define USB_LONG_FROM_BIG_ENDIAN(n) (n)

#define USB_LONG_TO_LITTLE_ENDIAN_ADDRESS(n, m) \
    {                                           \
        m[0] = ((n >> 24U) & 0xFFU);            \
        m[1] = ((n >> 16U) & 0xFFU);            \
        m[2] = ((n >> 8U) & 0xFFU);             \
        m[3] = (n & 0xFFU);                     \
    }

#define USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS(n) \
    ((uint32_t)(((uint32_t)n[0] << 24U) | ((uint32_t)n[1] << 16U) | ((uint32_t)n[2] << 8U) | ((uint32_t)n[3] << 0U)))

#define USB_LONG_TO_BIG_ENDIAN_ADDRESS(n, m) \
    {                                        \
        m[3] = ((n >> 24U) & 0xFFU);         \
        m[2] = ((n >> 16U) & 0xFFU);         \
        m[1] = ((n >> 8U) & 0xFFU);          \
        m[0] = (n & 0xFFU);                  \
    }

#define USB_LONG_FROM_BIG_ENDIAN_ADDRESS(n) \
    ((uint32_t)(((uint32_t)n[3] << 24U) | ((uint32_t)n[2] << 16U) | ((uint32_t)n[1] << 8U) | ((uint32_t)n[0] << 0U)))

#define USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(n, m) \
    {                                            \
        m[0] = ((n >> 8U) & 0xFFU);              \
        m[1] = (n & 0xFFU);                      \
    }

#define USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(n) ((uint32_t)(((uint32_t)n[0] << 8U) | ((uint32_t)n[1] << 0U)))

#define USB_SHORT_TO_BIG_ENDIAN_ADDRESS(n, m) \
    {                                         \
        m[1] = ((n >> 8U) & 0xFFU);           \
        m[0] = (n & 0xFFU);                   \
    }

#define USB_SHORT_FROM_BIG_ENDIAN_ADDRESS(n) ((uint32_t)(((uint32_t)n[1] << 8U) | ((uint32_t)n[0] << 0U)))

#else

#define USB_SHORT_TO_LITTLE_ENDIAN(n) (n)
#define USB_LONG_TO_LITTLE_ENDIAN(n) (n)
#define USB_SHORT_FROM_LITTLE_ENDIAN(n) (n)
#define USB_LONG_FROM_LITTLE_ENDIAN(n) (n)

#define USB_SHORT_TO_BIG_ENDIAN(n) SWAP2BYTE_CONST(n)
#define USB_LONG_TO_BIG_ENDIAN(n) SWAP4BYTE_CONST(n)
#define USB_SHORT_FROM_BIG_ENDIAN(n) SWAP2BYTE_CONST(n)
#define USB_LONG_FROM_BIG_ENDIAN(n) SWAP4BYTE_CONST(n)

#define USB_LONG_TO_LITTLE_ENDIAN_ADDRESS(n, m) \
    {                                           \
        m[3] = ((n >> 24U) & 0xFFU);            \
        m[2] = ((n >> 16U) & 0xFFU);            \
        m[1] = ((n >> 8U) & 0xFFU);             \
        m[0] = (n & 0xFFU);                     \
    }

#define USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS(n) \
    ((uint32_t)(((uint32_t)n[3] << 24U) | ((uint32_t)n[2] << 16U) | ((uint32_t)n[1] << 8U) | ((uint32_t)n[0] << 0U)))

#define USB_LONG_TO_BIG_ENDIAN_ADDRESS(n, m) \
    {                                        \
        m[0] = ((n >> 24U) & 0xFFU);         \
        m[1] = ((n >> 16U) & 0xFFU);         \
        m[2] = ((n >> 8U) & 0xFFU);          \
        m[3] = (n & 0xFFU);                  \
    }

#define USB_LONG_FROM_BIG_ENDIAN_ADDRESS(n) \
    ((uint32_t)(((uint32_t)n[0] << 24U) | ((uint32_t)n[1] << 16U) | ((uint32_t)n[2] << 8U) | ((uint32_t)n[3] << 0U)))

#define USB_SHORT_TO_LITTLE_ENDIAN_ADDRESS(n, m) \
    {                                            \
        m[1] = ((n >> 8U) & 0xFFU);              \
        m[0] = (n & 0xFFU);                      \
    }

#define USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(n) ((uint32_t)(((uint32_t)n[1] << 8U) | ((uint32_t)n[0] << 0U)))

#define USB_SHORT_TO_BIG_ENDIAN_ADDRESS(n, m) \
    {                                         \
        m[0] = ((n >> 8U) & 0xFFU);           \
        m[1] = (n & 0xFFU);                   \
    }

#define USB_SHORT_FROM_BIG_ENDIAN_ADDRESS(n) ((uint32_t)(((uint32_t)n[0] << 8U) | ((uint32_t)n[1] << 0U)))
#endif

/*
 * The following MACROs (USB_GLOBAL, USB_BDT, USB_RAM_ADDRESS_ALIGNMENT, etc) are only used for USB device stack.
 * The USB device global variables are put into the section m_usb_global and m_usb_bdt or the section
 * .bss.m_usb_global and .bss.m_usb_bdt by using the MACRO USB_GLOBAL and USB_BDT. In this way, the USB device
 * global variables can be linked into USB dedicated RAM by USB_STACK_USE_DEDICATED_RAM.
 * The MACRO USB_STACK_USE_DEDICATED_RAM is used to decide the USB stack uses dedicated RAM or not. The value of
 * the marco can be set as 0, USB_STACK_DEDICATED_RAM_TYPE_BDT_GLOBAL, or USB_STACK_DEDICATED_RAM_TYPE_BDT.
 * The MACRO USB_STACK_DEDICATED_RAM_TYPE_BDT_GLOBAL means USB device global variables, including USB_BDT and
 * USB_GLOBAL, are put into the USB dedicated RAM. This feature can only be enabled when the USB dedicated RAM
 * is not less than 2K Bytes.
 * The MACRO USB_STACK_DEDICATED_RAM_TYPE_BDT means USB device global variables, only including USB_BDT, are put
 * into the USB dedicated RAM, the USB_GLOBAL will be put into .bss section. This feature is used for some SOCs,
 * the USB dedicated RAM size is not more than 512 Bytes.
 */
#define USB_STACK_DEDICATED_RAM_TYPE_BDT_GLOBAL 1
#define USB_STACK_DEDICATED_RAM_TYPE_BDT 2

#if defined(USB_STACK_USE_DEDICATED_RAM) && (USB_STACK_USE_DEDICATED_RAM == USB_STACK_DEDICATED_RAM_TYPE_BDT_GLOBAL)

#if defined(__ICCARM__)

#define USB_GLOBAL _Pragma("location = \"m_usb_global\"")
#define USB_BDT _Pragma("location = \"m_usb_bdt\"")

/* disable misra 19.13 */
_Pragma("diag_suppress=Pm120")
#define USB_ALIGN_PRAGMA(x) _Pragma(#x)
    _Pragma("diag_default=Pm120")

#define USB_RAM_ADDRESS_ALIGNMENT(n) USB_ALIGN_PRAGMA(data_alignment = n)

#elif defined(__CC_ARM)

#define USB_GLOBAL __attribute__((section("m_usb_global"))) __attribute__((zero_init))
#define USB_BDT __attribute__((section("m_usb_bdt"))) __attribute__((zero_init))
#define USB_RAM_ADDRESS_ALIGNMENT(n) __attribute__((aligned(n)))

#elif defined(__GNUC__)

#define USB_GLOBAL __attribute__((section("m_usb_global, \"aw\", %nobits @")))
#define USB_BDT __attribute__((section("m_usb_bdt, \"aw\", %nobits @")))
#define USB_RAM_ADDRESS_ALIGNMENT(n) __attribute__((aligned(n)))

#else
#error The tool-chain is not supported.
#endif

#elif defined(USB_STACK_USE_DEDICATED_RAM) && (USB_STACK_USE_DEDICATED_RAM == USB_STACK_DEDICATED_RAM_TYPE_BDT)

#if defined(__ICCARM__)

#define USB_GLOBAL _Pragma("location = \".bss.m_usb_global\"")
#define USB_BDT _Pragma("location = \"m_usb_bdt\"")

/* disable misra 19.13 */
_Pragma("diag_suppress=Pm120")
#define USB_ALIGN_PRAGMA(x) _Pragma(#x)
    _Pragma("diag_default=Pm120")

#define USB_RAM_ADDRESS_ALIGNMENT(n) USB_ALIGN_PRAGMA(data_alignment = n)

#elif defined(__CC_ARM)

#define USB_GLOBAL __attribute__((section(".bss.m_usb_global"))) __attribute__((zero_init))
#define USB_BDT __attribute__((section("m_usb_bdt"))) __attribute__((zero_init))
#define USB_RAM_ADDRESS_ALIGNMENT(n) __attribute__((aligned(n)))

#elif defined(__GNUC__)

#define USB_GLOBAL __attribute__((section(".bss.m_usb_global, \"aw\", %nobits @")))
#define USB_BDT __attribute__((section("m_usb_bdt, \"aw\", %nobits @")))
#define USB_RAM_ADDRESS_ALIGNMENT(n) __attribute__((aligned(n)))

#else
#error The tool-chain is not supported.
#endif

#else

#if defined(__ICCARM__)

#define USB_GLOBAL _Pragma("location = \".bss.m_usb_global\"")
#define USB_BDT _Pragma("location = \".bss.m_usb_bdt\"")

/* disable misra 19.13 */
_Pragma("diag_suppress=Pm120")
#define USB_ALIGN_PRAGMA(x) _Pragma(#x)
    _Pragma("diag_default=Pm120")

#define USB_RAM_ADDRESS_ALIGNMENT(n) USB_ALIGN_PRAGMA(data_alignment = n)

#elif defined(__CC_ARM)

#define USB_GLOBAL __attribute__((section(".bss.m_usb_global"))) __attribute__((zero_init))
#define USB_BDT __attribute__((section(".bss.m_usb_bdt"))) __attribute__((zero_init))
#define USB_RAM_ADDRESS_ALIGNMENT(n) __attribute__((aligned(n)))

#elif defined(__GNUC__)

#define USB_GLOBAL __attribute__((section(".bss.m_usb_global, \"aw\", %nobits @")))
#define USB_BDT __attribute__((section(".bss.m_usb_bdt, \"aw\", %nobits @")))
#define USB_RAM_ADDRESS_ALIGNMENT(n) __attribute__((aligned(n)))

#else
#error The tool-chain is not supported.
#endif

#endif

#if defined(__ICCARM__)

#define USB_GLOBAL_DEDICATED_RAM _Pragma("location = \"m_usb_global\"")

#elif defined(__CC_ARM)

#define USB_GLOBAL_DEDICATED_RAM __attribute__((section("m_usb_global"))) __attribute__((zero_init))

#elif defined(__GNUC__)

#define USB_GLOBAL_DEDICATED_RAM __attribute__((section("m_usb_global, \"aw\", %nobits @")))

#else
#error The tool-chain is not supported.
#endif

#if (((defined(USB_DEVICE_CONFIG_LPCIP3511FS)) && (USB_DEVICE_CONFIG_LPCIP3511FS > 0U)) || \
     ((defined(USB_DEVICE_CONFIG_LPCIP3511HS)) && (USB_DEVICE_CONFIG_LPCIP3511HS > 0U)))
#define USB_DATA_ALIGNMENT USB_RAM_ADDRESS_ALIGNMENT(64)
#else
#define USB_DATA_ALIGNMENT
#endif

#endif /* __USB_MISC_H__ */
