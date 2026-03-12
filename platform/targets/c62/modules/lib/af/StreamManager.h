#ifndef STREAM_MANAGER_H
#define STREAM_MANAGER_H

#include "ic_stream.h"

#include <stdbool.h>
#include <stddef.h>

//------------------------------------------------
enum StreamType {
    STREAM_TYPE_INPUT,
    STREAM_TYPE_OUTPUT,
};

//------------------------------------------------
void StreamManager_prepareICStream(void);

void StreamManager_connect(void);

ICStream* StreamManager_getStream(int streamType, int streamID);

int StreamManager_detachStream(ICStream* stream, int streamType, int streamID);

#endif // STREAM_MANAGER_H
