#ifndef APRS_SLICER_H
#define APRS_SLICER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <array>
#include "protocols/APRS/constants.h"

#ifdef APRS_DEBUG
#include <vector>
#endif

namespace APRS
{

class Slicer
{
public:
    /**
     * Constructor.
     */
    Slicer();

    /**
     * Destructor.
     */
    ~Slicer();

    /**    
     * @brief Makes bit decisions based on demodulator output
     *
     * @details Takes in demodulator output, makes bit decisions, and creates a
     * byte array of packed bits. Stores state so it can be used in a streaming
     * fashion. Based on slice() in slicer.py from pymodem by Nino Carrillo
     * https://github.com/ninocarrillo/pymodem/blob/561606b36af6b4d69e50a14c920efa85afd9061b/modems_codecs/slicer.py#L59
     *
     * @param input An int16_t array from the demodulator
     * @param outputSize A reference to an integer that will store the size of the returned byte array
     * @return An uint8_t array of bytes
     */
    uint8_t *slice(const int16_t *input, size_t &outputSize);

#ifdef APRS_DEBUG
    std::vector<size_t> decisions;
    size_t sampleNum = 0;
#endif

private:
    //how far off the clock has to be it trigger a nudge
    static constexpr int8_t NUDGE_THRESH = 2;
    // how much to nudge the clock
    static constexpr int8_t NUDGE = 1;

    static constexpr uint8_t SAMPLES_PER_SYMBOL =
        (APRS_SAMPLE_RATE / APRS_SYMBOL_RATE);
    static constexpr uint8_t ROLLOVER_THRESHOLD = Slicer::SAMPLES_PER_SYMBOL
                                                / 2;

    int16_t lastSample = 0;
    uint8_t workingByte = 0;
    uint8_t bitCounter = 0;
    int8_t clock = 0;

    // depending clock drift, the amount bits sliced can vary
    // the extra 8 bits (+1 byte) seems to cover it
    std::array<uint8_t, ((APRS_BUF_SIZE / Slicer::SAMPLES_PER_SYMBOL / 8) + 1)>
        output;
};

} /* APRS */

#endif
