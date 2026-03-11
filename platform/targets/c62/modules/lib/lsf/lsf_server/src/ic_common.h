#ifndef __IC_COMMON_H__
#define __IC_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 取两个核dcache_linesize的最大值. hifi4: XCHAL_DCACHE_LINESIZE = 32. arm: 32.
#define IC_MAX_DCACHE_LINESIZE 32

// 对(N)按(dcacheline size)向上取整数倍, 单位: bytes
#define IC_DCACHELINE_ROUNDUP_SIZE(N)   (((N) + IC_MAX_DCACHE_LINESIZE-1) & -IC_MAX_DCACHE_LINESIZE)

#ifndef _STRIZE
#define _STRIZE(x) #x
#endif

#define IC_ASSERT(a) \
    if ((a) != true) { \
        CLOGD("IC_ASSERT: %s: " _STRIZE(a) "\n", __FUNCTION__); \
        while(1); \
    }

enum {
    /*! No Error */
    IC_OK                                           =  0,
    /*! General Error */
    IC_ERROR                                        =  -1,
    /*! Arguments to the API functions are not valid  */
    IC_ERROR_RPC                                    = -2

};

// transaction: remote, variant_type -------------------------------------------

// 0x3F - 6 bits
#define REMOTE_GROUP_ID(remote_and_vartype_u16)         (((uint32_t)(remote_and_vartype_u16) >> 10) & 0x3F)
#define REMOTE_PROPERTY_ID(remote_and_vartype_u16)      (((uint32_t)(remote_and_vartype_u16) >> 4) & 0x3F)
#define REMOTE_VARTYPE(remote_and_vartype_u16)          ((uint32_t)(remote_and_vartype_u16) & 0xF)

// 6 bit groupID + 6 bit propertyID + 4 bit variant_type
#define REMOTE_AND_VARTYPE_U16(group_id, property_id, variant_type) \
            ((((uint16_t)(group_id) & 0x3F) << 10) \
             | (((uint16_t)(property_id) & 0x3F) << 4) \
             | ((uint16_t)(variant_type) & 0xF))

// override the lowest 4 bit variant_type
#define REMOTE_PATCH_VARTYPE_U16(remote_and_vartype_u16, variant_type) \
            (((uint32_t)(remote_and_vartype_u16)  & 0xFFF0) \
             | ((uint16_t)(variant_type) & 0xF))

//------------------------------------------------
// 对(N)按(4)向上取整数倍, 单位: bytes
#define IC_ROUNDUP_4(N)   (((N) + 4-1) & -4)

//------------------------------------------------
// uint32 read & write

#define BITS_PER_BYTE 8
#define ALIGN_MASK_32BITS (4-1)

static inline  uint32_t read_u32(uint8_t *array)
{
    // The address is 4-byte aligned here
    if (!(((uint32_t) array) & ALIGN_MASK_32BITS)) {
        // load 32bit
        return *((uint32_t *)(array));
    }

    uint32_t u32;

    u32 =  array[0]
           | (array[1] << (1*BITS_PER_BYTE))
           | (array[2] << (2*BITS_PER_BYTE))
           | (array[3] << (3*BITS_PER_BYTE));

    return u32;
}

static inline void write_u32(uint8_t *array, uint32_t value)
{
    // The address is 4-byte aligned here
    if (!(((uint32_t) array) & ALIGN_MASK_32BITS)) {
        // store 32bit
        *((uint32_t *)(array)) = value;

        return;
    }

    array[0] = value & 0xff;
    array[1] = (value >> (1*BITS_PER_BYTE)) & 0xff;
    array[2] = (value >> (2*BITS_PER_BYTE)) & 0xff;
    array[3] = (value >> (3*BITS_PER_BYTE)) & 0xff;

    return;
}

//------------------------------------------------
// uint16 read & write

#define ALIGN_MASK_16BITS (2-1)

static inline  uint16_t read_u16(uint8_t *array)
{
    // The address is 2-byte aligned here
    if (!(((uint32_t) array) & ALIGN_MASK_16BITS)) {
        // load 16bit
        return *((uint16_t *)(array));
    }

    uint16_t u16;

    u16 =  array[0]
           | (array[1] << (BITS_PER_BYTE));

    return u16;
}

static inline void write_u16(uint8_t *array, uint16_t value)
{
    // The address is 2-byte aligned here
    if (!(((uint32_t) array) & ALIGN_MASK_16BITS)) {
        // store 16bit
        *((uint16_t *)(array)) = value;

        return;
    }

    array[0] = value & 0xff;
    array[1] = (value >> (BITS_PER_BYTE)) & 0xff;

    return;
}

#ifdef __cplusplus
}
#endif

#endif /* __IC_COMMON_H__ */
