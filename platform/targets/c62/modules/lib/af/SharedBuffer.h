#ifndef DROID_SHARED_BUFFER_H
#define DROID_SHARED_BUFFER_H

#include <stdint.h>

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>


#include <stdatomic.h>

//------------------------------------------------

/* flags to use with release() */
#define SHARED_BUFFER_KEEP_STORAGE 0x00000001

typedef struct SharedBuffer {
    atomic_int mRefs;
    size_t mSize;
    uint32_t mReserved[2];
} SharedBuffer;

// static methods------------------------------------------------

/*! allocate a buffer of size 'size' and acquire() it.
 *  call release() to free it.
 */
SharedBuffer* SharedBuffer_alloc(size_t size);

/*! free the memory associated with the SharedBuffer.
 * Fails if there are any users associated with this SharedBuffer.
 * In other words, the buffer must have been release by all its
 * users.
 */
ssize_t SharedBuffer_dealloc(const SharedBuffer *released);

//! get the SharedBuffer from the data pointer
static inline const SharedBuffer* SharedBuffer_sharedBuffer(const void *data);

// methods ------------------------------------------------

//! access the data for read
static inline const void* SharedBuffer_data(const SharedBuffer *sb);

//! access the data for read/write
static inline void* SharedBuffer_data_mut(SharedBuffer *sb);

//! get size of the buffer
static inline size_t SharedBuffer_size(const SharedBuffer *sb);

//! get back a SharedBuffer object from its data
static inline SharedBuffer* SharedBuffer_bufferFromData(void *data);

//! get back a SharedBuffer object from its data
static inline const SharedBuffer* SharedBuffer_bufferFromData_const(
        const void *data);

//! get the size of a SharedBuffer object from its data
static inline size_t SharedBuffer_sizeFromData(const void *data);

//! edit the buffer (get a writtable, or non-const, version of it)
SharedBuffer* SharedBuffer_edit(SharedBuffer *sb);

//! edit the buffer, resizing if needed
SharedBuffer* SharedBuffer_editResize(SharedBuffer *sb, size_t newSize);

//! like edit() but fails if a copy is required
SharedBuffer* SharedBuffer_attemptEdit(const SharedBuffer *sb);

//! resize and edit the buffer, loose it's content.
SharedBuffer* SharedBuffer_reset(SharedBuffer *sb, size_t new_size);

//! acquire/release a reference on this buffer
void SharedBuffer_acquire(SharedBuffer *sb);

/*! release a reference on this buffer, with the option of not
 * freeing the memory associated with it if it was the last reference
 * returns the previous reference count
 */
int32_t SharedBuffer_release(SharedBuffer *sb, uint32_t flags);

//! returns wether or not we're the only owner
static inline bool SharedBuffer_onlyOwner(const SharedBuffer *sb);

// static inline methods implementation-----------------------------------------

static inline const SharedBuffer* SharedBuffer_sharedBuffer(const void *data)
{
    return data ?
            (SharedBuffer*) (((const char*) data) - sizeof(SharedBuffer)) : 0;
}

static inline const void* SharedBuffer_data(const SharedBuffer *sb)
{
    return sb + 1;
}

static inline void* SharedBuffer_data_mut(SharedBuffer *sb)
{
    return sb + 1;
}

static inline size_t SharedBuffer_size(const SharedBuffer *sb)
{
    return sb->mSize;
}

static inline SharedBuffer* SharedBuffer_bufferFromData(void *data)
{
    return (SharedBuffer*) (((char*) data) - sizeof(SharedBuffer));
}

static inline const SharedBuffer* SharedBuffer_bufferFromData_const(
        const void *data)
{
    return (const SharedBuffer*) (((const char*) data) - sizeof(SharedBuffer));
}

static inline size_t SharedBuffer_sizeFromData(const void *data)
{
    return ((SharedBuffer*) (((const char*) data) - sizeof(SharedBuffer)))->mSize;
}

static inline bool SharedBuffer_onlyOwner(const SharedBuffer *sb)
{
    return (atomic_load(&sb->mRefs) == 1);
}

#endif // DROID_SHARED_BUFFER_H
