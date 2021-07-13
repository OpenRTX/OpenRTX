// Copyright 2020 Mobilinkd LLC.

#pragma once

#include "Trellis.h"
#include "Convolution.h"
#include "Util.h"

#include <limits>
//#include <iostream>
//#include <iomanip>

namespace mobilinkd
{

template <typename Trellis_>
constexpr std::array<std::array<uint8_t, (1 << Trellis_::k)>, (1 << Trellis_::K)> makeNextState(Trellis_)
{
    std::array<std::array<uint8_t, (1 << Trellis_::k)>, (1 << Trellis_::K)> result{};
    for (size_t i = 0; i != (1 << Trellis_::K); ++i)
    {
        for (size_t j = 0; j != (1 << Trellis_::k); ++j)
        {
            result[i][j] = static_cast<uint8_t>(update_memory<Trellis_::K, Trellis_::k>(i, j) & ((1 << Trellis_::K) - 1));
        }
    }
    return result;
}

template <typename Trellis_>
constexpr std::array<std::array<uint8_t, (1 << Trellis_::k)>, (1 << Trellis_::K)> makePrevState(Trellis_)
{
    constexpr size_t NumStates = (1 << Trellis_::K);
    constexpr size_t HalfStates = NumStates / 2;

    std::array<std::array<uint8_t, (1 << Trellis_::k)>, (1 << Trellis_::K)> result{};
    for (size_t i = 0; i != (1 << Trellis_::K); ++i)
    {
        size_t k = i >= HalfStates;
        for (size_t j = 0; j != (1 << Trellis_::k); ++j)
        {
            size_t l = update_memory<Trellis_::K, Trellis_::k>(i, j) & (NumStates - 1);
            result[l][k] = i; 
        }
    }
    return result;
}

template <typename Trellis_, size_t LLR = 2>
constexpr auto makeCost(Trellis_ trellis)
{
    constexpr size_t NumStates = (1 << Trellis_::K);
    constexpr size_t NumOutputs = Trellis_::n;

    std::array<std::array<int16_t, NumOutputs>, NumStates> result{};
    for (uint32_t i = 0; i != NumStates; ++i)
    {
        for (uint32_t j = 0; j != NumOutputs; ++j)
        {
            auto bit = convolve_bit(trellis.polynomials[j], i << 1);
            result[i][j] = to_int<int8_t, LLR>(((bit << 1) - 1) * ((1 << (LLR - 1)) - 1));
        }
    }
    return result;
}

template <typename Trellis_, size_t LLR_ = 2>
struct Viterbi
{
    static_assert(LLR_ < 7);    // Need to be < 7 to avoid overflow errors.

    static constexpr size_t K = Trellis_::K;
    static constexpr size_t k = Trellis_::k;
    static constexpr size_t n = Trellis_::n;
    static constexpr size_t InputValues = 1 << n;
    static constexpr size_t NumStates = (1 << K);
    static constexpr int32_t METRIC = ((1 << (LLR_ - 1)) - 1) << 2;

    using metrics_t = std::array<int32_t, NumStates>;
    using cost_t = std::array<std::array<int16_t, n>, NumStates>;
    using state_transition_t = std::array<std::array<uint8_t, 2>, NumStates>;

    metrics_t pathMetrics_{};
    cost_t cost_;
    state_transition_t nextState_;
    state_transition_t prevState_;

    Viterbi(Trellis_ trellis)
    : cost_(makeCost<Trellis_, LLR_>(trellis))
    , nextState_(makeNextState(trellis))
    , prevState_(makePrevState(trellis))
    {}

    /**
     * Viterbi soft decoder using LLR inputs where 0 == erasure.
     * 
     * @return path metric for computing BER.
     */
    template <size_t IN, size_t OUT>
    size_t decode(std::array<int8_t, IN> in, std::array<uint8_t, OUT>& out)
    {
        constexpr auto MAX_METRIC = std::numeric_limits<typename metrics_t::value_type>::max() / 2;

        metrics_t prevMetrics, currMetrics;
        prevMetrics.fill(MAX_METRIC);
        prevMetrics[0] = 0;     // Starting point.

        std::array<std::bitset<NumStates>, IN / 2> history;
        // history.fill(0);

        constexpr size_t BUTTERFLY_SIZE = NumStates / 2;

        size_t hindex = 0;
        std::array<int16_t, BUTTERFLY_SIZE> cost0;
        std::array<int16_t, BUTTERFLY_SIZE> cost1;

        for (size_t i = 0; i != IN; i += 2)
        {
            auto& hist = history[hindex];
            int16_t s0 = in[i];
            int16_t s1 = in[i + 1];

            for (size_t j = 0; j != BUTTERFLY_SIZE; ++j)
            {
                int16_t c = std::abs(cost_[j][0] - s0) + std::abs(cost_[j][1] - s1);
                cost0[j] = c;
                // cost1[j] = METRIC - c;
                cost1[j] = std::abs(cost_[j][0] + s0) + std::abs(cost_[j][1] + s1);
            }
            
            for (size_t j = 0; j != BUTTERFLY_SIZE; ++j)
            {
                auto& i0 = nextState_[j][0];
                auto& i1 = nextState_[j][1];

                int16_t c0 = cost0[j];
                int16_t c1 = cost1[j];

                auto& p0 = prevMetrics[j];
                auto& p1 = prevMetrics[j + BUTTERFLY_SIZE];

                int32_t m0 = p0 + c0;
                int32_t m1 = p0 + c1;
                int32_t m2 = p1 + c1;
                int32_t m3 = p1 + c0;

                bool d0 = m0 > m2;
                bool d1 = m1 > m3;

                hist.set(i0, d0);
                hist.set(i1, d1);
                currMetrics[i0] = d0 ? m2 : m0;
                currMetrics[i1] = d1 ? m3 : m1;
            }
            std::swap(currMetrics, prevMetrics);
            hindex += 1;
            // for (size_t i = 0; i != NumStates; ++i) std::cerr << std::setw(5) << prevMetrics[i] << ",";
            // std::cerr << std::endl;
        }

        // Find starting point. Should be 0 for properly flushed CCs.
        size_t min_element = 0;
        int32_t min_cost = prevMetrics[0];

        for (size_t i = 0; i != NumStates; ++i)
        {
            if (prevMetrics[i] < min_cost)
            {
                min_cost = prevMetrics[i];
                min_element = i;
            }
        }

        size_t ber = min_cost / (METRIC >> 1); // Cost is at least equal to # of erasures.

        // Do chainback.
        auto oit = std::rbegin(out);
        auto hit = std::rbegin(history);
        size_t next_element = min_element;
        size_t index = IN / 2;
        while (oit != std::rend(out) && hit != std::rend(history))
        {
            auto v = (*hit++)[next_element];
            if (index-- <= OUT) *oit++ = next_element & 1;
            next_element = prevState_[next_element][v];
        }

        return ber;
    }
};

} // mobilinkd
