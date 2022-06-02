#include <cstdio>
#include <cstdlib>

#include "interfaces/audio_stream.h"

#define CHECK(x)                                \
    do                                          \
    {                                           \
        if (!(x))                               \
        {                                       \
            puts("Failed assertion: " #x "\n"); \
            abort();                            \
        }                                       \
    } while (0)

int main()
{
    FILE* fp = fopen("MIC.raw", "wb");
    for (int i = 0; i < 13; i++)
    {
        fputc(i, fp);
        fputc(0, fp);
    }
    fclose(fp);

    stream_sample_t tmp[128];
    auto id = inputStream_start(
        AudioSource::SOURCE_MIC, AudioPriority::PRIO_BEEP, tmp,
        sizeof(tmp) / sizeof(tmp[0]), BufMode::BUF_LINEAR, 44100);
    inputStream_getData(id);
    for (int i = 0; i < 128; i++) CHECK(tmp[i] == (i % 13));
    return 0;
}
