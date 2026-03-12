/*
 * VARIANT
 *
 * Copyright 1998 Jean-Claude Cote
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Daniel Remenak
 * Copyright 2006 Google (Benjamin Arai)
 *
 * The algorithm for conversion from Julian days to day/month/year is based on
 * that devised by Henry Fliegel, as implemented in PostgreSQL, which is
 * Copyright 1994-7 Regents of the University of California
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

// 本单元
#include "variant.h"

// 本模块
#include "ic_common.h"
#include "ic_allocator.h"

// log模块
#include "venus_log.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "venus_ap.h"

#define FLASH_SIZE (128 * 1024 * 1024)

#define PSRAM_SIZE (8 * 1024 * 1024)

#define DATA_RAM_BASE SYS_RAM_BASE
#define DATA_RAM_SIZE (320 * 1024)

#define DRAM1_BASE SHMEM_BASE
#define DRAM1_SIZE (768 * 1024)

#define check_broundary(addr, size, base, base_size) ( \
    (addr) >= (base) && \
    (addr) < (base) + (base_size) && \
    (addr) + (size) >= (base) && \
    (addr) + (size) < (base) + (base_size) \
)

#define aligned_dn(addr, align) ((addr) & ~((align)-1))
#define aligned_up(addr, align) ((addr + align - 1) & ~(align - 1))

enum pointer_type {
    pointer_flash = 0,
    pointer_psram = 1,
    pointer_data_ram = 2,
    pointer_dram1 = 3,
};

//------------------------------------------------

// 核间内存管理
extern IC_Allocator *g_pFreeer;

//------------------------------------------------

static int LsfVariant_validation_type(LsfVariant* self){
    return VARIANT_ERROR_NO_ERROR;
}

void LsfVariant_init(LsfVariant* self){
    memset(self, 0, sizeof(LsfVariant));
}

// 不涉及Variant本体的释放, 只清空其内部间接内存占用
// 本端使用, 释放Variant间接占用内存, ic_allocator内存, DeepFree
// 用完须free pShareBlock
//     非ic_allocator分配的var不希望
//     判断 reserved1 == 0 : 本端memBuf / 对端memBuf ic_allocator
void LsfVariant_clear(LsfVariant* self)
{
    switch (self->variant_type) {
        case type_uint:
            // nothing to do
            break;

        case type_membuf:
            if (self->reserved1 != 0) {
                size_t size = 0;
                // memBuf: data's size的长度, data长度
                size = sizeof(size_t) + self->u.memBuf.size;

                IC_Sharemem_detach((void *) self->reserved1, size);

                IC_Allocator_free(g_pFreeer, (void *) self->reserved1);
            }
            break;

        case type_pointer:
            if (self->reserved1 != 0) {
                IC_Sharemem_detach((void *) self->reserved1, sizeof(uint32_t) * 3);
                IC_Allocator_free(g_pFreeer, (void *) self->reserved1);
            }
            break;

        default:
            // 非预期
            // assert(false);
            break;
    }

    return;
}

void LsfVariant_unref(LsfVariant* self)
{
    // 清理内部: 对端分配的内存, 由本端释放
    LsfVariant_clear(self);
    // 用户负责free
    free(self);

    return;
}

int LsfVariant_copy(LsfVariant* dst, LsfVariant* src)
{
    return VARIANT_ERROR_NO_ERROR;
}

// 不包含type的长度, type放在marshal记录外部, 供外部选择位置进行mashal/unmarshal
// 返回: bytes
size_t LsfVariant_getMarshalSize(LsfVariant* self)
{
    if (self == NULL) {
        return 0; // Handle invalid input
    }

    size_t size = 0;
    switch (self->variant_type) {
        case type_uint:
            size = sizeof(uint32_t);
            break;

        case type_membuf:
            // memBuf: data's size的长度, data长度
            size = sizeof(size_t) + self->u.memBuf.size;
            break;

        case type_pointer:
            // pointer: type + size + addr
            size = sizeof(uint32_t) * 3;
            break;

        default:
            // 非预期
            // assert(false);
            break;

        // case type_string:
        //     if (self->u.s != NULL) {
        //         size = strlen(self->u.s) + 1; // Include the null terminator
        //     }
        //     break;
    }

    return size;
}

// 一个完整的marshal记录, 放在共享内存中, payload只记录指针.
// mem buf size ==> buf[0-3]
//     ==> cp端构建Variant时读取至var.buf_len
// mem buf data ==> buf[4-size]
//     ==> cp端构建Variant时读取至var.pBuf -> task收到属性set, 用完后ic_allocator_free

// marshal 不分配内存, 只记录, 根据Variant所需的MarshalSize先分配内存, 再marshal记录
// marshal使用者先获取Variant预计的marshal长度.
// 再根据预期marshal长度, 灵活分配 urpc.payload 或 ic_allocator 作为载具.
// type放在marshal记录外部, 供外部选择位置进行mashal/unmarshal.
int LsfVariantClass_marshal(LsfVariant *var, uint8_t *buf, size_t bufSize, size_t *outSize)
{
    CLOGD("[%s]", __FUNCTION__);

    int32_t res = VARIANT_ERROR_NO_ERROR;

    // Handle invalid input
    if (var == NULL || buf == NULL || bufSize == 0 || outSize == NULL) {
        res = VARIANT_ERROR_PARA_ERROR;
        return res;
    }

    res = LsfVariant_validation_type(var);

    if (res != VARIANT_ERROR_NO_ERROR){
        return res;
    }

    uint32_t tempOutSize = 0;

    switch (var->variant_type) {
    case type_uint:
        tempOutSize = sizeof(UIntType);
        if (tempOutSize > bufSize){
            return VARIANT_ERROR_MEM_INSUFF;
        }

        // 通常buf来自于urpc.payload, 会有非对齐, 特别赋值
        write_u32((uint8_t *)buf, var->u.uiVal);

        break;

    case type_membuf:
        // 写入 memBuf.size + memBuf.data
        tempOutSize = sizeof(uint32_t) + var->u.memBuf.size;

        memcpy(buf, &(var->u.memBuf.size), sizeof(size_t));

        if (var->u.memBuf.pData != NULL) {
            memcpy(buf + sizeof(uint32_t), var->u.memBuf.pData, var->u.memBuf.size);
        }

        break;

    case type_pointer: {
        uint32_t type;
        uint32_t size = var->u.pointer.size;
        uint32_t addr = (uint32_t)var->u.pointer.pAddr;

        if (check_broundary(addr, size, FLASH_BASE, FLASH_SIZE)) {
            type = pointer_flash;
            addr = addr - FLASH_BASE;
        } else if (check_broundary(addr, size, IFLASH_BASE, FLASH_SIZE)) {
            type = pointer_flash;
            addr = addr - IFLASH_BASE;
        } else if (check_broundary(addr, size, PSRAM_BASE, PSRAM_SIZE)) {
            type = pointer_psram;
            addr = addr - PSRAM_BASE;
        } else if (check_broundary(addr, size, DATA_RAM_BASE, DATA_RAM_SIZE)) {
            type = pointer_data_ram;
            addr = addr - DATA_RAM_BASE;
        } else if (check_broundary(addr, size, DRAM1_BASE, DRAM1_SIZE)) {
            type = pointer_dram1;
            addr = addr - DRAM1_BASE;
        } else {
            return VARIANT_ERROR_PARA_ERROR;
        }

        if (type == pointer_psram ||
            type == pointer_data_ram ||
            type == pointer_dram1) {
            uint32_t aligned_start = aligned_dn((uint32_t)var->u.pointer.pAddr, CACHE_LINE_SIZE(DCACHE));
            uint32_t aligned_end = aligned_up((uint32_t)(var->u.pointer.pAddr)+ size, CACHE_LINE_SIZE(DCACHE));
            IC_Sharemem_updateAndDetach((void *)aligned_start, aligned_end - aligned_start);
        }

        // 写入 pointer.type + pointer.size + pointer.addr
        tempOutSize = sizeof(uint32_t) * 3;
        if (tempOutSize > bufSize){
            return VARIANT_ERROR_MEM_INSUFF;
        }
        memcpy(buf + sizeof(uint32_t) * 0, &type, sizeof(uint32_t));
        memcpy(buf + sizeof(uint32_t) * 1, &size, sizeof(uint32_t));
        memcpy(buf + sizeof(uint32_t) * 2, &addr, sizeof(uint32_t));

        break;
    }

    default:
        return VARIANT_ERROR_TYPE_UNSUP;
    }

    *outSize = tempOutSize;

    return res;
}

// unmarshal使用场景: 外部分配对象内存; type单独; 按type读取marshal记录
// 参数: var中已经设置type
// buf, bufSize: marshal记录及其bytes
//      场景: Variant复合在marshal记录中.
//      bufSize: 0 则不关心buf长度
// out_size: unmarshal实际用掉的size?
int LsfVariantClass_unmarshal(LsfVariant *var, uint8_t *buf, size_t bufSize, size_t *outSize)
{
    int32_t res = VARIANT_ERROR_NO_ERROR;
    res = LsfVariant_validation_type(var);

    if (res != VARIANT_ERROR_NO_ERROR){
        return res;
    }

    uint32_t type = type_unknown;
    uint8_t *pBuf = buf;

    // 读取类型
    // type = read_u32(pBuf);
    // pBuf += sizeof(var->variant_type);
    // bufSize -= sizeof(var->variant_type);

    type = var->variant_type;

    switch(type){
    case type_uint:
        if (bufSize < sizeof(UIntType)) {
            return VARIANT_ERROR_MEM_INSUFF;
        }

        var->u.uiVal = read_u32(pBuf);

        // outSize: 实际用掉的size
        *outSize = 4;

        break;

    case type_membuf:
        memcpy(&(var->u.memBuf.size), buf, sizeof(uint32_t));

        // 就地使用pShareBlock
        var->u.memBuf.pData = (void *) (buf + sizeof(uint32_t));

        // memBuf free时用
        var->reserved1 = (uint32_t) buf;

        // outSize: 实际用掉的size: |size段|datad段|
        *outSize = sizeof(uint32_t) + var->u.memBuf.size;

        break;

    case type_pointer: {
        var->reserved1 = (uint32_t)buf;

        uint32_t type, size, addr;
        memcpy(&type, buf + sizeof(uint32_t) * 0, sizeof(uint32_t));
        memcpy(&size, buf + sizeof(uint32_t) * 1, sizeof(uint32_t));
        memcpy(&addr, buf + sizeof(uint32_t) * 2, sizeof(uint32_t));

        if (type == pointer_flash &&
            check_broundary(FLASH_BASE + addr, size, FLASH_BASE, FLASH_SIZE)) {
            var->u.pointer.size = size;
            var->u.pointer.pAddr = (void *)(FLASH_BASE + addr);
            *outSize = sizeof(uint32_t) * 3;
        } else if (type == pointer_psram &&
            check_broundary(PSRAM_BASE + addr, size, PSRAM_BASE, PSRAM_SIZE)) {
            var->u.pointer.size = size;
            var->u.pointer.pAddr = (void *)(PSRAM_BASE + addr);
            *outSize = sizeof(uint32_t) * 3;
        } else if (type == pointer_data_ram &&
            check_broundary(DATA_RAM_BASE + addr, size, DATA_RAM_BASE, DATA_RAM_SIZE)) {
            var->u.pointer.size = size;
            var->u.pointer.pAddr = (void *)(DATA_RAM_BASE + addr);
            *outSize = sizeof(uint32_t) * 3;
        } else if (type == pointer_dram1 &&
            check_broundary(DRAM1_BASE + addr, size, DRAM1_BASE, DRAM1_SIZE)) {
            var->u.pointer.size = size;
            var->u.pointer.pAddr = (void *)(DRAM1_BASE + addr);
            *outSize = sizeof(uint32_t) * 3;
        } else {
            return VARIANT_ERROR_PARA_ERROR;
        }

        if (type == pointer_psram ||
            type == pointer_data_ram ||
            type == pointer_dram1) {
            uint32_t aligned_start = aligned_dn((uint32_t)var->u.pointer.pAddr, CACHE_LINE_SIZE(DCACHE));
            uint32_t aligned_end = aligned_up((uint32_t)(var->u.pointer.pAddr) + size, CACHE_LINE_SIZE(DCACHE));
            IC_Sharemem_detach((void *)aligned_start, aligned_end - aligned_start);
        }

        break;
    }

    default:
        return VARIANT_ERROR_TYPE_UNSUP;
    }

    return res;
}
