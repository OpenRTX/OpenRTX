#ifndef _BASE_H
#define _BASE_H

#include <stddef.h>
#include <stdint.h>

//------------------------------------------------

typedef int status_t;

#define container_of(ptr, type, member)					\
	({								\
		const __typeof__(((type *)0)->member ) *__mptr = (ptr);	\
		(type *)((char *)__mptr - offsetof(type,member));	\
	})

// binder -------------------------------------------
// binder_u32 = 2 bit unused + 10 bit groupID + 10 bit propertyID + 10 bit interfaceID

// 0x3FF - 10 bits
#define GROUP_ID(binder_u32)        (((uint32_t)(binder_u32) >> 20) & 0x3FF)
#define PROPERTY_ID(binder_u32)     (((uint32_t)(binder_u32) >> 10) & 0x3FF)
#define INTERFACE_ID(binder_u32)    ((uint32_t)(binder_u32) & 0x3FF)

#define BINDER_U32(group_id, property_id, interface_id) (((((uint32_t)(group_id) & 0x3FF) << 20) \
                                                          | (((uint32_t)(property_id) & 0x3FF) << 10) \
                                                          | ((uint32_t)(interface_id) & 0x3FF)) \
                                                         & 0x3FFFFFFF)

#define BINDER_INVALID_PORT         (0xFFFFFFFF)

// #define GROUP_ID(binder_u32)                (binder_u32 >> 16)
// #define PROPERTY_ID(binder_u32)             (binder_u32 & 0xFFFF)
// #define BINDER_U32(group_id, property_id)   (((uint32_t)(group_id) << 16) | (uint32_t)(property_id))

//------------------------------------------------
// binder common function ID
#define IBINDER_FUNCID_UNREF        (-1)

// IBINDER_UNREF
struct IBinder_unref_ARGS {
    uint32_t target_binder;
    int funcID;
};

struct IBinder_unref_RETS {
    int status;
};

//------------------------------------------------

#endif
