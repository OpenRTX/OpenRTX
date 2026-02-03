#ifndef APRS_DEMODULATOR_H
#define APRS_DEMODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>
#include <array>
#include "protocols/APRS/constants.h"

namespace APRS
{

class Convolver
{
public:
    /**
     * @brief Construct a Convolver
     *
     * @param ir Impulse response
     * @param irSize Size of the impulse reponse is, max 8.
     *
     * @return A Convolver object
     */
    Convolver(const int8_t *ir, size_t irSize);

    /**
     * @brief Destroy a Convolver
     */
    ~Convolver();

#ifdef APRS_DEBUG
    void print();
#endif

    /**
     * @brief Performs linear convolution
     * 
     * Performs linear convolution on chunks of input using the overlap add
     * algorithm found here:
     * https://blog.robertelder.org/overlap-add-overlap-save/
     * Convolution code is based on the following:
     * https://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch6.pdf
     * This class is stateful and can operate continuously on a stream of input.
     *
     * @param input An int16_t array of input of size APRS_BUF_SIZE
     *
     * @return An int16_t array of APRS_BUF_SIZE with the convolution result 
     */
    const int16_t *convolve(const int16_t *input);

private:
    const int8_t *ir;              // impulse response, max size 8
    size_t irSize;                 // impulse response size
    size_t overlapSize;            // size of prev and new overlaps
    int16_t prevOverlap[8 - 1];    // buffer to hold previous overlap
    int16_t newOverlap[8 - 1];     // buffer to hold new overlap
    int16_t output[APRS_BUF_SIZE]; // buffer to hold output
};

class Demodulator
{
public:
    /**
     * @brief Constructs a Demodulator object
     */
    Demodulator();

    /**
     * @brief Destroys a Demodulator object
     */
    ~Demodulator();

    /**
     * @brief Demodulates baseband audio by mark and space frequencies
     *
     * Takes baseband audio and calculates the difference between the magnitude
     * of the mark tone and the magnitued of the space tone.
     *
     * @param input An int16_t array of input of size APRS_BUF_SIZE, typically
     * baseband audio
     *
     * @return An int16_t array of the difference between the mark and space
     * tones
     */
    const int16_t *demodulate(int16_t *input);

private:
    // Based on pymodem: https://github.com/ninocarrillo/pymodem
    // See https://github.com/rxt1077/aprs_prototype/ for how these were derived
    static constexpr size_t BPF_SIZE = 7;
    static constexpr int8_t BPF_TAPS[] = { -1, -1, 1, 2, 1, -1, -1 };
    static constexpr size_t CORRELATOR_SIZE = 8;
    static constexpr int8_t MARK_CORRELATOR_I[] = { 8, 5, 0, -5, -8, -5, 0, 5 };
    static constexpr int8_t MARK_CORRELATOR_Q[] = { 0, 5, 8, 5, 0, -5, -8, -5 };
    static constexpr int8_t SPACE_CORRELATOR_I[] = {
        8, 1, -7, -3, 6, 4, -5, -6
    };
    static constexpr int8_t SPACE_CORRELATOR_Q[] = { 0, 7, 2, -7, -3, 6, 5, -4 };
    static constexpr size_t LPF_SIZE = 4;
    static constexpr int8_t LPF_TAPS[] = { 1, 2, 2, 1 };

    // input band-pass filter correlator
    Convolver bpf{ BPF_TAPS, BPF_SIZE };

    // mark/space correlators
    Convolver markI{ MARK_CORRELATOR_I, CORRELATOR_SIZE };
    Convolver markQ{ MARK_CORRELATOR_Q, CORRELATOR_SIZE };
    Convolver spaceI{ SPACE_CORRELATOR_I, CORRELATOR_SIZE };
    Convolver spaceQ{ SPACE_CORRELATOR_Q, CORRELATOR_SIZE };

    // output low-pass filter correlator
    Convolver lpf{ LPF_TAPS, LPF_SIZE };

    // buffer for mark - space
    int16_t diff[APRS_BUF_SIZE];
};

} /* APRS */

#endif
