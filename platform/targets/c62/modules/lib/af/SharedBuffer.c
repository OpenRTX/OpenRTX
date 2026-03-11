#include "SharedBuffer.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>

//------------------------------------------------

SharedBuffer* SharedBuffer_alloc(size_t size)
{
    SharedBuffer *sb = (SharedBuffer*) malloc(sizeof(SharedBuffer) + size);
    if (sb) {
        atomic_store(&sb->mRefs, 1);
        sb->mSize = size;
    }
    return sb;
}

ssize_t SharedBuffer_dealloc(const SharedBuffer *released)
{
    if (atomic_load(&released->mRefs) != 0)
        return -1;
    free((void*) released);
    return 0;
}

SharedBuffer* SharedBuffer_edit(SharedBuffer *sb)
{
    if (SharedBuffer_onlyOwner(sb)) {
        return (SharedBuffer*) sb;
    }
    SharedBuffer *new_sb = SharedBuffer_alloc(sb->mSize);
    if (new_sb) {
        memcpy(SharedBuffer_data_mut(new_sb), SharedBuffer_data(sb),
                SharedBuffer_size(sb));
        SharedBuffer_release(sb, 0);
    }
    return new_sb;
}

SharedBuffer* SharedBuffer_editResize(SharedBuffer *sb, size_t newSize)
{
    if (SharedBuffer_onlyOwner(sb)) {
        SharedBuffer *buf = (SharedBuffer*) sb;
        if (buf->mSize == newSize)
            return buf;
        buf = (SharedBuffer*) realloc(buf, sizeof(SharedBuffer) + newSize);
        if (buf != NULL) {
            buf->mSize = newSize;
            return buf;
        }
    }
    SharedBuffer *new_sb = SharedBuffer_alloc(newSize);
    if (new_sb) {
        const size_t mySize = sb->mSize;
        memcpy(SharedBuffer_data_mut(new_sb), SharedBuffer_data(sb),
                newSize < mySize ? newSize : mySize);
        SharedBuffer_release(sb, 0);
    }
    return new_sb;
}

SharedBuffer* SharedBuffer_attemptEdit(const SharedBuffer *sb)
{
    if (SharedBuffer_onlyOwner(sb)) {
        return (SharedBuffer*) sb;
    }
    return NULL;
}

SharedBuffer* SharedBuffer_reset(SharedBuffer *sb, size_t new_size)
{
    SharedBuffer *new_sb = SharedBuffer_alloc(new_size);
    if (new_sb) {
        SharedBuffer_release(sb, 0);
    }
    return new_sb;
}

void SharedBuffer_acquire(SharedBuffer *sb)
{
    atomic_fetch_add(&sb->mRefs, 1);
}

int32_t SharedBuffer_release(SharedBuffer *sb, uint32_t flags)
{
    int32_t prev = 1;
    if (SharedBuffer_onlyOwner(sb)
            || ((prev = atomic_fetch_sub(&sb->mRefs, 1)) == 1)) {
        atomic_store(&sb->mRefs, 0);
        if ((flags & SHARED_BUFFER_KEEP_STORAGE) == 0) {
            free((void*) sb);
        }
    }
    return prev;
}
