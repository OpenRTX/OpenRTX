/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
     * @brief Make bit decisions based on demodulator output
     *
     * @details Takes demodulator output and makes a bit decision if
     * possible. This class stores recovers timing from the demodulator
     * samples and makes a bit decision approximately every 8 samples
     * (9600Hz/1200Hz) when a carrier is detected. Based on slice() in
     * slicer.py from pymodem by Nino Carrillo:
     * https://github.com/ninocarrillo/pymodem/blob/561606b36af6b4d69e50a14c920efa85afd9061b/modems_codecs/slicer.py#L59
     * The Digital Carrier Detect (DCD) logic follows the algorithm in
     * section 7-3 of the NinoTNC operation manual:
     * https://tarpn.net/t/nino-tnc/n9600a/n9600a_operation.html
     *
     * @param input A int16_t sample from the demodulator output
     * @return A int8_t: -1 if no bit decision was made or 0/1 for a bit
     */
    int8_t slice(const int16_t input);

    /**
     * @brief Return the status of Digital Carrier Detect (DCD)
     *
     * @return A bool that is true if a digital carrier is detected
     */
    bool getDCD();

private:
    static constexpr int8_t NUDGE_THRESH = 2; ///< what triggers a nudge
    static constexpr int8_t NUDGE = 1;        ///< how much to move the clock

    static constexpr uint8_t SAMPLES_PER_SYMBOL =
        (APRS_SAMPLE_RATE / APRS_SYMBOL_RATE);
    static constexpr uint8_t ROLLOVER_THRESH = Slicer::SAMPLES_PER_SYMBOL / 2;
    static constexpr uint8_t DCD_THRESH =
        10; ///< amount of correct zero crossings to trigger DCD
    static constexpr uint8_t DCD_BITS =
        80; ///< minimum bits to hold DCD open for

    int16_t lastSample = 0;
    int8_t clock = 0;
    int8_t dcdCrossings = 0;
    int8_t dcdBitTimer = 0;
};

} /* APRS */

#endif
