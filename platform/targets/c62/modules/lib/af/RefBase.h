#ifndef DROID_REFBASE_H
#define DROID_REFBASE_H

//------------------------------------------------
// lib: clib
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

#include <stdatomic.h>

//------------------------------------------------
// class definition

typedef struct _RefBase RefBase;

struct _RefBase {
    //------------------------------------------------
    // virtual method declarations

    void (*dtor)(RefBase* self);

    //------------------------------------------------
    // private members

    volatile atomic_int mRefs;
};

//------------------------------------------------
// class methods: public

static inline
void RefBase_ctor(RefBase* self)
{
    atomic_store_explicit(&self->mRefs, 1, memory_order_relaxed);
    return;
}

static inline
void RefBase_incStrong(RefBase* self)
{
    atomic_fetch_add_explicit(&self->mRefs, 1, memory_order_relaxed);
    return;
}

int32_t RefBase_decStrong(RefBase* self);

//------------------------------------------------
// virtual methods declaration

static inline
void RefBase_dtor(RefBase* self)
{
    self->dtor(self);
    return;
}

//------------------------------------------------
// private methods

static inline
bool RefBase_onlyOwner(RefBase* self)
{
    return (atomic_load_explicit(&self->mRefs,
                                 memory_order_acquire) == 1);
}

#endif // DROID_REFBASE_H
