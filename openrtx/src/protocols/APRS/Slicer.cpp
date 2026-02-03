#include "protocols/APRS/Slicer.hpp"

using namespace APRS;

Slicer::Slicer()
{
}

Slicer::~Slicer()
{
}

uint8_t *Slicer::slice(const int16_t *input, size_t &outputSize)
{
    outputSize = 0;
    for (size_t i = 0; i < APRS_BUF_SIZE; i++) {
        // increment clock
        clock++;
        // check for symbol center
        if (clock >= Slicer::ROLLOVER_THRESHOLD) {
            // at or past symbol center, reset clock
            clock -= SAMPLES_PER_SYMBOL;
            // workingByte gets shifted to the left and the new bit gets put
            // in at the 0th position
            workingByte <<= 1;
            // make a bit decision
#ifdef APRS_DEBUG
            decisions.push_back(sampleNum);
#endif
            if (input[i] >= 0)
                // this is a '1' bit
                workingByte |= 0b00000001;
            else
                // otherwise it's a '0' bit
                workingByte &= 0b11111110;
            // If we have 8 bits, put the byte into the result but DON'T reset
            // workingByte. We use the last bit to detect bit changes.
            bitCounter++;
            if (bitCounter >= 8) {
                output[outputSize++] = workingByte;
                bitCounter = 0;
            }
        }
        // check for zero-crossing in sample stream
        if ((lastSample ^ input[i]) < 0) {
            // the clock should be zero at the crossing, if it's off by too much
            if (abs(clock) >= NUDGE_THRESH) {
                if (clock > 0)
                    // if it's ahead nudge it back
                    clock -= NUDGE;
                else
                    // if it's behind nudge it forward
                    clock += NUDGE;
            }
        }
        lastSample = input[i];
#ifdef APRS_DEBUG
        sampleNum++;
#endif
    }
    return output.data();
}
