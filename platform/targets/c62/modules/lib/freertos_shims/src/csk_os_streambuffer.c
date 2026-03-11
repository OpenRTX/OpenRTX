#include <zephyr/kernel.h>

#include "csk_os_memory.h"
#include "csk_os_streambuffer.h"

typedef struct {
    struct k_pipe pipe;
    char *pipe_buffer;
    int trigger_level;
} z_stream_buffer_t;

StreamBufferHandle_t xStreamBufferCreate( size_t xBufferSizeBytes,
                                          size_t xTriggerLevelBytes )
{
    z_stream_buffer_t *z_stream_buffer = csk_os_malloc(sizeof(z_stream_buffer_t));
    if (z_stream_buffer == NULL) {
        return NULL;
    }
    z_stream_buffer->pipe_buffer = csk_os_malloc(xBufferSizeBytes);
    if (z_stream_buffer->pipe_buffer == NULL) {
        return NULL;
    }

    z_stream_buffer->trigger_level = xTriggerLevelBytes;
    k_pipe_init(&z_stream_buffer->pipe, z_stream_buffer->pipe_buffer, xBufferSizeBytes);
    return z_stream_buffer;
}

size_t xStreamBufferSend( StreamBufferHandle_t xStreamBuffer,
                          const void *pvTxData,
                          size_t xDataLengthBytes,
                          TickType_t xTicksToWait )
{
    size_t bytes_written = 0;
    z_stream_buffer_t *z_stream_buffer = xStreamBuffer;
    k_timeout_t timeout = Z_TIMEOUT_TICKS(xTicksToWait);
    int ret = k_pipe_put(&z_stream_buffer->pipe, pvTxData, xDataLengthBytes,
                         &bytes_written, 1, timeout);
    if (ret != 0) {
        return 0;
    } else {
        return bytes_written;
    }
}

size_t xStreamBufferReceive( StreamBufferHandle_t xStreamBuffer,
                             void *pvRxData,
                             size_t xBufferLengthBytes,
                             TickType_t xTicksToWait )
{
    size_t bytes_read = 0;
    z_stream_buffer_t *z_stream_buffer = xStreamBuffer;
    k_timeout_t timeout = Z_TIMEOUT_TICKS(xTicksToWait);
    int ret = k_pipe_get(&z_stream_buffer->pipe, pvRxData, xBufferLengthBytes,
                         &bytes_read, z_stream_buffer->trigger_level, timeout);
    if (ret != 0) {
        return 0;
    } else {
        return bytes_read;
    }
}

BaseType_t xStreamBufferReset( StreamBufferHandle_t xStreamBuffer )
{
    z_stream_buffer_t *z_stream_buffer = xStreamBuffer;
    k_pipe_flush(&z_stream_buffer->pipe);
}
