/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cmath>

#include "Channel.hpp"

Channel::Channel(const uint32_t rate, const uint32_t seed, const float idleRms,
                 const float signalRms)
    : sampleRate(rate)
    , idleRms(idleRms)
    , signalRms(signalRms)
    , rng(seed)
    , spare(0.0f)
    , haveSpare(false)
{
}

void Channel::idle(const double seconds)
{
    const size_t samples = std::lround(seconds * sampleRate);
    for (size_t i = 0; i < samples; i++)
        append(0.0f, idleRms);
}

void Channel::align(const size_t period, const size_t phase)
{
    while ((baseband.size() % period) != phase)
        append(0.0f, idleRms);
}

void Channel::transmit(const std::vector<float> &signal)
{
    for (float value : signal)
        append(value, signalRms);
}

const std::vector<int16_t> &Channel::samples() const
{
    return baseband;
}

uint32_t Channel::rate() const
{
    return sampleRate;
}

float Channel::gaussian()
{
    if (haveSpare) {
        haveSpare = false;
        return spare;
    }

    double u1 = (rng() + 1.0) / 4294967296.0;
    double u2 = rng() / 4294967296.0;
    double r = std::sqrt(-2.0 * std::log(u1));

    spare = r * std::sin(2.0 * M_PI * u2);
    haveSpare = true;
    return r * std::cos(2.0 * M_PI * u2);
}

void Channel::append(const float signal, const float noiseRms)
{
    float value = std::round(signal + noiseRms * gaussian());
    value = std::min(std::max(value, -32768.0f), 32767.0f);
    baseband.push_back(static_cast<int16_t>(value));
}
