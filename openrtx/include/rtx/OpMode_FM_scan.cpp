/***************************************************************************
 *   Frequency scan worker for OpMode_FM
 *   (adds start/stop/isScanning behaviour using a pthread)
 ***************************************************************************/

#include "OpMode_FM.hpp"
#include "interfaces/radio.h"
#include "rtx/rtx.h"
#include <errno.h>
#include <cstdio>
#include <cstring>

// Helper thread entry - C style for pthread_create
static void *fm_scan_thread_entry(void *arg)
{
    OpMode_FM *self = static_cast<OpMode_FM*>(arg);
    if(!self) return nullptr;

    // Local copy of parameters
    uint32_t start = self->scanStartHz;
    uint32_t stop  = self->scanStopHz;
    uint32_t step  = self->scanStepHz;
    uint32_t dwell = self->scanDwellMs;
    int32_t  thr   = self->scanRssiThreshold;

    bool ascending = (start <= stop);

    uint32_t freq = self->scanCurrentHz = start;

    while(self->scanning)
    {
        rssi_t level = radio_scanStep(static_cast<freq_t>(freq), static_cast<uint16_t>(dwell));

        if(level >= thr)
        {
            // Found a signal: post new configuration (thread-safe)
            rtxStatus_t cfg = rtx_getCurrentStatus();
            cfg.rxFrequency = freq;
            cfg.scan = 0; // clear the scan request
            rtx_configure(&cfg);

            // stop scanning
            self->scanning = false;
            break;
        }

        // advance
        if(ascending)
        {
            if (freq + step < freq) break; // overflow protection
            freq += step;
            if(freq > stop) freq = start; // wrap
        }
        else
        {
            if (freq < step) { freq = stop; } else freq -= step;
            if(freq < stop) freq = start; // wrap safety
        }

        // update current position
        self->scanCurrentHz = freq;

        // check if caller asked stop
        if(!self->scanning) break;
    }

    self->scanning = false;
    return nullptr;
}

/* -------------------- public methods ---------------------------------- */

void OpMode_FM::startScan(uint32_t startHz, uint32_t stopHz, uint32_t stepHz,
                          int32_t rssiThreshold, uint32_t dwellMs)
{
    if(scanning) return; // already scanning

    if(stepHz == 0) return;

    // set parameters
    scanStartHz = startHz;
    scanStopHz  = stopHz;
    scanStepHz  = stepHz;
    scanRssiThreshold = rssiThreshold;
    scanDwellMs = dwellMs;
    scanCurrentHz = startHz;

    scanning = true;

    // create thread
    int rc = pthread_create(&scanThread, nullptr, fm_scan_thread_entry, this);
    if(rc != 0)
    {
        // failed to create thread
        scanning = false;
    }
}

void OpMode_FM::stopScan()
{
    if(!scanning) return;

    scanning = false;
    // join thread if running
    pthread_join(scanThread, nullptr);
}
