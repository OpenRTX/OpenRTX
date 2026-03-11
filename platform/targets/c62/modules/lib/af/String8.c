#include "String8.h"

// #include "Log.h"

// #include "Static.h"

#include <ctype.h>
#include <string.h>

// ---------------------------------------------------------------------------

static SharedBuffer *gEmptyStringBuf = NULL;
static char *gEmptyString = NULL;

static inline char* getEmptyString()
{
    SharedBuffer_acquire(gEmptyStringBuf);
    return gEmptyString;
}

void initialize_string8()
{
    SharedBuffer *buf = SharedBuffer_alloc(1);
    char *str = (char*) SharedBuffer_data(buf);
    *str = 0;
    gEmptyStringBuf = buf;
    gEmptyString = str;
}

void terminate_string8()
{
    SharedBuffer_release(SharedBuffer_bufferFromData(gEmptyString), 0);
    gEmptyStringBuf = NULL;
    gEmptyString = NULL;
}

// ---------------------------------------------------------------------------

static char* allocFromUTF8(const char *in, size_t len)
{
    if (len > 0) {
        SharedBuffer *buf = SharedBuffer_alloc(len + 1);
        // LOG_ASSERT(buf, "Unable to allocate shared buffer");
        if (buf) {
            char *str = (char*) SharedBuffer_data(buf);
            memcpy(str, in, len);
            str[len] = 0;
            return str;
        }
        return NULL;
    }

    return getEmptyString();
}

// ---------------------------------------------------------------------------

String8* String8_ctor(String8 *self)
{
    if (self == NULL)
        return NULL;

    self->mString = getEmptyString();

    return self;
}

String8* String8_copy_ctor(String8 *self, const String8 *o)
{
    if (self == NULL || o == NULL)
        return NULL;

    self->mString = o->mString;
    SharedBuffer_acquire(SharedBuffer_bufferFromData(self->mString));

    return self;
}

String8* String8_ctor_char(String8 *self, const char *o)
{
    if (self == NULL)
        return NULL;

    self->mString = allocFromUTF8(o, strlen(o));
    if (self->mString == NULL) {
        self->mString = getEmptyString();
    }

    return self;
}

String8* String8_ctor_char_len(String8 *self, const char *o, size_t len)
{
    if (self == NULL)
        return NULL;

    self->mString = allocFromUTF8(o, len);
    if (self->mString == NULL) {
        self->mString = getEmptyString();
    }

    return self;
}

void String8_dtor(String8 *self)
{
    SharedBuffer_release(SharedBuffer_bufferFromData(self->mString), 0);
}

void String8_setTo(String8 *self, const String8 *other)
{
    SharedBuffer_acquire(SharedBuffer_bufferFromData(other->mString));
    SharedBuffer_release(SharedBuffer_bufferFromData(self->mString), 0);
    self->mString = other->mString;
}

status_t String8_setTo_char(String8 *self, const char *other)
{
    SharedBuffer_release(SharedBuffer_bufferFromData(self->mString), 0);
    self->mString = allocFromUTF8(other, strlen(other));
    if (self->mString)
        return NO_ERROR;

    self->mString = getEmptyString();
    return NO_MEMORY;
}

status_t String8_setTo_char_len(String8 *self, const char *other, size_t len)
{
    SharedBuffer_release(SharedBuffer_bufferFromData(self->mString), 0);
    self->mString = allocFromUTF8(other, len);
    if (self->mString)
        return NO_ERROR;

    self->mString = getEmptyString();
    return NO_MEMORY;
}

status_t String8_append(String8 *self, const String8 *other)
{
    const size_t otherLen = String8_bytes(other);
    if (String8_bytes(self) == 0) {
        String8_setTo(self, other);
        return NO_ERROR;
    }
    else if (otherLen == 0) {
        return NO_ERROR;
    }

    return String8_real_append(self, String8_string(other), otherLen);
}

status_t String8_append_char(String8 *self, const char *other)
{
    return String8_append_char_len(self, other, strlen(other));
}

status_t String8_append_char_len(String8 *self, const char *other,
        size_t otherLen)
{
    if (String8_bytes(self) == 0) {
        return String8_setTo_char_len(self, other, otherLen);
    }
    else if (otherLen == 0) {
        return NO_ERROR;
    }

    return String8_real_append(self, other, otherLen);
}

status_t String8_real_append(String8 *self, const char *other, size_t otherLen)
{
    const size_t myLen = String8_bytes(self);

    SharedBuffer *buf = SharedBuffer_editResize(
            SharedBuffer_bufferFromData(self->mString), myLen + otherLen + 1);
    if (buf) {
        char *str = (char*) SharedBuffer_data(buf);
        self->mString = str;
        str += myLen;
        memcpy(str, other, otherLen);
        str[otherLen] = '\0';
        return NO_ERROR;
    }
    return NO_MEMORY;
}

char* String8_lockBuffer(String8 *self, size_t size)
{
    SharedBuffer *buf = SharedBuffer_editResize(
            SharedBuffer_bufferFromData(self->mString), size + 1);
    if (buf) {
        char *str = (char*) SharedBuffer_data(buf);
        self->mString = str;
        return str;
    }
    return NULL;
}

void String8_unlockBuffer(String8 *self)
{
    String8_unlockBuffer_size(self, strlen(self->mString));
}

status_t String8_unlockBuffer_size(String8 *self, size_t size)
{
    if (size != String8_size(self)) {
        SharedBuffer *buf = SharedBuffer_editResize(
                SharedBuffer_bufferFromData(self->mString), size + 1);
        if (buf) {
            char *str = (char*) SharedBuffer_data(buf);
            str[size] = 0;
            self->mString = str;
            return NO_ERROR;
        }
    }

    return NO_MEMORY;
}

ssize_t String8_find(const String8 *self, const char *other, size_t start)
{
    size_t len = String8_size(self);
    if (start >= len) {
        return -1;
    }
    const char *s = self->mString + start;
    const char *p = strstr(s, other);
    return p ? p - self->mString : -1;
}
