#pragma once

#include "os_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* StreamBufferHandle_t;

StreamBufferHandle_t xStreamBufferCreate( size_t xBufferSizeBytes,
                                          size_t xTriggerLevelBytes );

size_t xStreamBufferSend( StreamBufferHandle_t xStreamBuffer,
                          const void *pvTxData,
                          size_t xDataLengthBytes,
                          TickType_t xTicksToWait );

size_t xStreamBufferReceive( StreamBufferHandle_t xStreamBuffer,
                             void *pvRxData,
                             size_t xBufferLengthBytes,
                             TickType_t xTicksToWait );

BaseType_t xStreamBufferReset( StreamBufferHandle_t xStreamBuffer );

#ifdef __cplusplus
}
#endif