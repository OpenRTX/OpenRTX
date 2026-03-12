/*
 * Variant Inlines
 *
 * Copyright 2003 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __LSF_VARIANT_H__
#define __LSF_VARIANT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define VARIANT_ERROR_NO_ERROR            0x0
#define VARIANT_ERROR_PARA_ERROR          0x1
#define VARIANT_ERROR_MEM_INSUFF          0x2
#define VARIANT_ERROR_TYPE_UNSUP          0x3

#define RsvWord           uint32_t
#define UByteType         uint8_t
#define UShortType        uint16_t
#define UIntType          uint32_t
#define ByteType          int8_t
#define ShortType         int16_t
#define IntType           int32_t
#define StringType        char*

#define B_MAX   0x7f
#define B_MIN   ((-B_MAX)-1)
#define UB_MAX  0xff
#define UB_MIN  0
#define S_MAX   0x7fff
#define S_MIN   ((-H_MAX)-1)
#define US_MAX  0xffff
#define US_MIN  0
#define I_MAX   0x7fffffff
#define I_MIN   ((-W_MAX)-1)
#define UI_MAX  0xffffffff
#define UI_MIN  0

// should be <= 15 to be fixed into 4 bits of comm payload
typedef enum {
    type_unknown  = 0,
    type_ubyte,
    type_byte,
    type_ushort,
    type_short,
    type_uint,
    type_membuf,
    type_pointer,
} variant_type_t;

typedef struct tagVARIANT {
    uint32_t variant_type;
    // memBuf free时用, pShareBlock
    uint32_t reserved1;
    union {
        UByteType   ubVal;
        UShortType  usVal;
        UIntType    uiVal;
        ByteType    bVal;
        ShortType   sVal;
        IntType     iVal;
        struct {
            size_t  size;
            void    *pData;
        } memBuf;
        struct {
            size_t  size;
            void    *pAddr;
        } pointer;
    } u;
} LsfVariant;

void LsfVariant_init(LsfVariant* self);

// 不涉及Variant本体的释放, 只清空其内部间接内存占用
void LsfVariant_clear(LsfVariant* self);

// 用户负责 释放 整个Variant, 包含其内部间接内存占用
void LsfVariant_unref(LsfVariant* self);

int LsfVariant_copy(LsfVariant* dst, LsfVariant* src);

size_t LsfVariant_getMarshalSize(LsfVariant* self);

int LsfVariantClass_marshal(LsfVariant *var, uint8_t *buf, size_t max_size, size_t *out_size);

int LsfVariantClass_unmarshal(LsfVariant *var, uint8_t *buf, size_t buf_size, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif /* __LSF_VARIANT_H__ */
