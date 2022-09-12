
#include <string.h>
#include <stdio.h>
#include <slip.h>

#define END     0xc0
#define ESC     0xDB
#define ESC_END 0xDC
#define ESC_ESC 0xDD

static const uint8_t raw[] = {0xba, 0x74, 0xbe, 0x3a, 0xde, 0x82, 0x93, 0x9a,
                              0x5a, 0xd7, 0x25, 0x2f, 0xdb, 0x37, 0xb3, 0xfd,
                              0x0c, 0x3c, 0xd0, 0x6a, 0x35, 0xf5, 0x46, 0x4f,
                              0xf5, 0x27, 0xb2, 0xc0, 0x83, 0xa8, 0x47, 0x6c };

static const uint8_t enc[] = {END,  0xba, 0x74, 0xbe, 0x3a, 0xde, 0x82, 0x93,
                              0x9a, 0x5a, 0xd7, 0x25, 0x2f, ESC,  ESC_ESC, 0x37,
                              0xb3, 0xfd, 0x0c, 0x3c, 0xd0, 0x6a, 0x35, 0xf5,
                              0x46, 0x4f, 0xf5, 0x27, 0xb2, ESC,  ESC_END, 0x83,
                              0xa8, 0x47, 0x6c, END };


static int encodeEmpty()
{
    struct FrameCtx ctx;
    uint8_t buf[2] = {0};
    uint8_t res[] = {END, END};
    uint8_t c = 0;
    slip_initFrame(&ctx, buf, sizeof(buf));
    slip_encode(&ctx, &c, 0, true);

    void *ptr;
    size_t len = slip_popFrame(&ctx, &ptr);
    return memcmp(ptr, res, len);
}

static int encodeFull()
{
    struct FrameCtx ctx;
    uint8_t buf[64] = {0};
    slip_initFrame(&ctx, buf, sizeof(buf));
    slip_encode(&ctx, raw, sizeof(raw), true);

    void *ptr;
    size_t len = slip_popFrame(&ctx, &ptr);
    return memcmp(ptr, enc, len);
}

static int encodeChunk()
{
    struct FrameCtx ctx;
    uint8_t buf[8] = {0};
    slip_initFrame(&ctx, buf, sizeof(buf));

    int ret = -1;
    const uint8_t *res = enc;
    while(ret < 0)
    {
        ret = slip_encode(&ctx, raw, sizeof(raw), true);

        void *ptr;
        size_t len = slip_popFrame(&ctx, &ptr);
        if(memcmp(ptr, res, len) != 0)
            return -1;
        res += len;
    }

    return 0;
}

static int decodeEmpty()
{
    struct FrameCtx ctx;
    uint8_t inp[] = {END, END};
    uint8_t buf[2] = {0};
    bool end = false;
    slip_initFrame(&ctx, buf, sizeof(buf));
    slip_decode(&ctx, inp, sizeof(inp), &end);

    void *ptr;
    size_t len = slip_popFrame(&ctx, &ptr);
    return (len == 0) && (end == true);
}

static int decodeFull()
{
    struct FrameCtx ctx;
    uint8_t buf[64] = {0};
    bool end = false;
    slip_initFrame(&ctx, buf, sizeof(buf));
    slip_decode(&ctx, enc, sizeof(enc), &end);

    void *ptr;
    size_t len = slip_popFrame(&ctx, &ptr);
    return memcmp(ptr, raw, len);
}

static int decodeChunk()
{
    struct FrameCtx ctx;
    uint8_t buf[8] = {0};
    slip_initFrame(&ctx, buf, sizeof(buf));

    int ret = -1;
    const uint8_t *res = raw;
    while(ret < 0)
    {
        bool end;
        ret = slip_decode(&ctx, enc, sizeof(enc), &end);

        void *ptr;
        size_t len = slip_popFrame(&ctx, &ptr);
        if(memcmp(ptr, res, len) != 0)
            return -1;
        res += len;
    }

    return 0;
}

int main()
{
    if(encodeEmpty() != 0)
        return -1;

    if(encodeFull() != 0)
        return -1;

    if(encodeChunk() != 0)
        return -1;

    if(decodeEmpty() != 0)
        return -1;

    if(decodeFull() != 0)
        return -1;

    if(decodeChunk() != 0)
        return -1;

    printf("PASS!\n");

    return 0;
}
