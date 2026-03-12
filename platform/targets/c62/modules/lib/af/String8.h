#ifndef DROID_STRING8_H
#define DROID_STRING8_H

#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "base.h"
#include "Errors.h"

#include "SharedBuffer.h"

typedef struct String8 {
    char* mString;
} String8;

// class init ------------------------------------

extern void initialize_string8();
extern void terminate_string8();

//------------------------------------------------
// Initializers
String8* String8_ctor(String8* self);
String8* String8_copy_ctor(String8* self, const String8* o);
String8* String8_ctor_char(String8* self, const char* o);
String8* String8_ctor_char_len(String8* self, const char* o, size_t otherLen);

// Destructor
void String8_dtor(String8* self);

// Accessors
static inline const char* String8_string(const String8* self);
static inline size_t String8_size(const String8* self);
static inline size_t String8_bytes(const String8* self);
static inline const SharedBuffer* String8_sharedBuffer(const String8* self);

// Modifiers
void String8_setTo(String8* self, const String8* other);
status_t String8_setTo_char(String8* self, const char* other);
status_t String8_setTo_char_len(String8* self, const char* other, size_t otherLen);

status_t String8_append(String8* self, const String8* other);
status_t String8_append_char(String8* self, const char* other);
status_t String8_append_char_len(String8* self, const char* other, size_t otherLen);

// Operators
static inline void String8_assign(String8* self, const String8* other);
static inline void String8_assign_char(String8* self, const char* other);

static inline void String8_add_assign(String8* self, const String8* other);
static inline String8 String8_add(const String8* self, const String8* other);

static inline void String8_add_assign_char(String8* self, const char* other);
static inline String8 String8_add_char(const String8* self, const char* other);

static inline int String8_compare(const String8* self, const String8* other);

static inline bool String8_less_than(const String8* self, const String8* other);
static inline bool String8_less_than_equal(const String8* self, const String8* other);
static inline bool String8_equal(const String8* self, const String8* other);
static inline bool String8_not_equal(const String8* self, const String8* other);
static inline bool String8_greater_than_equal(const String8* self, const String8* other);
static inline bool String8_greater_than(const String8* self, const String8* other);

static inline bool String8_less_than_char(const String8* self, const char* other);
static inline bool String8_less_than_equal_char(const String8* self, const char* other);
static inline bool String8_equal_char(const String8* self, const char* other);
static inline bool String8_not_equal_char(const String8* self, const char* other);
static inline bool String8_greater_than_equal_char(const String8* self, const char* other);
static inline bool String8_greater_than_char(const String8* self, const char* other);

static inline const char* String8_to_char(const String8* self);

char* String8_lockBuffer(String8* self, size_t size);
void String8_unlockBuffer(String8* self);
status_t String8_unlockBuffer_size(String8* self, size_t size);

ssize_t String8_find(const String8* self, const char* other, size_t start);

status_t String8_real_append(String8* self, const char* other, size_t otherLen);

//------------------------------------------------

// Utility functions

/* Compares two String8 objects */
static inline int compare_type(const String8* lhs, const String8* rhs)
{
    return String8_compare(lhs, rhs);
}

/* Strictly orders two String8 objects */
static inline int strictly_order_type(const String8* lhs, const String8* rhs)
{
    return compare_type(lhs, rhs) < 0;
}

/* Returns the string of a String8 object */
static inline const char* String8_string(const String8* self)
{
    return self->mString;
}

/* Returns the length of a String8 object */
static inline size_t String8_length(const String8* self)
{
    return SharedBuffer_sizeFromData(self->mString) - 1;
}


/* Returns the size of a String8 object */
static inline size_t String8_size(const String8* self)
{
    return String8_length(self);
}

/* Returns the number of bytes of a String8 object */
static inline size_t String8_bytes(const String8* self)
{
    return SharedBuffer_sizeFromData(self->mString) - 1;
}

/* Returns the shared buffer of a String8 object */
static inline const SharedBuffer* String8_sharedBuffer(const String8* self)
{
    return SharedBuffer_bufferFromData(self->mString);
}

static inline void String8_assign(String8* self, const String8* other)
{
    String8_setTo(self, other);
}

static inline void String8_assign_char(String8* self, const char* other)
{
    String8_setTo_char(self, other);
}

static inline void String8_add_assign(String8* self, const String8* other)
{
    String8_append(self, other);
}

static inline String8 String8_add(const String8* self, const String8* other)
{
    String8 tmp;
    String8_ctor(&tmp);
    String8_add_assign(&tmp, other);
    return tmp;
}

static inline void String8_add_assign_char(String8* self, const char* other)
{
    String8_append_char(self, other);
}

static inline String8 String8_add_char(const String8* self, const char* other)
{
    String8 tmp;
    String8_ctor(&tmp);
    String8_add_assign_char(&tmp, other);
    return tmp;
}

static inline int String8_compare(const String8* self, const String8* other)
{
    return strcmp(self->mString, other->mString);
}

static inline bool String8_less_than(const String8* self, const String8* other)
{
    return strcmp(self->mString, other->mString) < 0;
}

static inline bool String8_less_than_equal(const String8* self, const String8* other)
{
    return strcmp(self->mString, other->mString) <= 0;
}

static inline bool String8_equal(const String8* self, const String8* other)
{
    return strcmp(self->mString, other->mString) == 0;
}

static inline bool String8_not_equal(const String8* self, const String8* other)
{
    return strcmp(self->mString, other->mString) != 0;
}

static inline bool String8_greater_than_equal(const String8* self, const String8* other)
{
    return strcmp(self->mString, other->mString) >= 0;
}

static inline bool String8_greater_than(const String8* self, const String8* other)
{
    return strcmp(self->mString, other->mString) > 0;
}

static inline bool String8_less_than_char(const String8* self, const char* other)
{
    return strcmp(self->mString, other) < 0;
}

static inline bool String8_less_than_equal_char(const String8* self, const char* other)
{
    return strcmp(self->mString, other) <= 0;
}

static inline bool String8_equal_char(const String8* self, const char* other)
{
    return strcmp(self->mString, other) == 0;
}

static inline bool String8_not_equal_char(const String8* self, const char* other)
{
    return strcmp(self->mString, other) != 0;
}

static inline bool String8_greater_than_equal_char(const String8* self, const char* other)
{
    return strcmp(self->mString, other) >= 0;
}

static inline bool String8_greater_than_char(const String8* self, const char* other)
{
    return strcmp(self->mString, other) > 0;
}

static inline const char* String8_to_char(const String8* self)
{
    return self->mString;
}

#endif // DROID_STRING8_H
