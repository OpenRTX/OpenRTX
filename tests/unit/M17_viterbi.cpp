/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <cstdio>
#include <cstdint>
#include <random>
#include <array>
#include "M17/M17ConvolutionalEncoder.h"
#include "M17/M17CodePuncturing.h"
#include "M17/M17Viterbi.h"
#include "M17/M17Utils.h"

using namespace std;

default_random_engine rng;

/**
 * Insert radom bit flips in input data.
 */
template < size_t N >
void generateErrors(array< uint8_t, N >& data)
{
    uniform_int_distribution< uint8_t > numErrs(0, 4);
    uniform_int_distribution< uint8_t > errPos(0, N);

    for(uint8_t i = 0; i < numErrs(rng); i++)
    {
        uint8_t pos = errPos(rng);
        bool    bit = getBit(data, pos);
        setBit(data, pos, !bit);
    }
}

int main()
{
    uniform_int_distribution< uint8_t > rndValue(0, 255);

    array< uint8_t, 18 > source;

    for(auto& byte : source)
    {
        byte = rndValue(rng);
    }

    array<uint8_t, 37> encoded;
    M17ConvolutionalEncoder encoder;
    encoder.reset();
    encoder.encode(source.data(), encoded.data(), source.size());
    encoded[36] = encoder.flush();

    array<uint8_t, 34> punctured;
    puncture(encoded, punctured, Audio_puncture);

    generateErrors(punctured);

    array< uint8_t, 18 > result;
    M17Viterbi decoder;
    decoder.decodePunctured(punctured, result, Audio_puncture);

    for(size_t i = 0; i < result.size(); i++)
    {
        if(source[i] != result[i])
        {
            printf("Error at pos %ld: got %02x, expected %02x\n", i, result[i],
                                                                     source[i]);
            return -1;
        }
    }

    return 0;
}
