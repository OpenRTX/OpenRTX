#include <interfaces/audio_stream.h>

streamId inputStream_start(const enum AudioSource source,
                           const enum AudioPriority prio,
                           const stream_sample_t* buf,
                           const size_t bufLength,
                           const enum BufMode mode,
                           const uint32_t sampleRate)
{
    ;
}

void inputStream_stop(streamId id)
{
    ;
}

streamId outputStream_start(const enum AudioSink destination,
                            const enum AudioPriority prio,
                            const stream_sample_t* const buf,
                            const size_t length,
                            const uint32_t sampleRate)
{
    ;
}
