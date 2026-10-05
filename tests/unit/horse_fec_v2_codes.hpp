/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Host-only Horse v2 FEC candidates. Not linked into firmware.
 */

#ifndef HORSE_FEC_V2_CODES_HPP
#define HORSE_FEC_V2_CODES_HPP

#include "protocols/horse/ldpc_horse.h"
#include "protocols/M17/ConvolutionalEncoder.hpp"
#include "protocols/M17/CodePuncturing.hpp"
#include "protocols/M17/Interleaver.hpp"
#include "protocols/M17/Decorrelator.hpp"
#include "protocols/M17/Viterbi.hpp"
#include "core/crc.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstddef>

static constexpr size_t FEC_INFO_BITS = 144;
static constexpr size_t FEC_CODED_BITS = 368;
static constexpr size_t FEC_INFO_BYTES = 18;
static constexpr size_t FEC_CODED_BYTES = 46;
static constexpr size_t POLAR_CRC_BITS = 24;
static constexpr size_t POLAR_K = FEC_INFO_BITS + POLAR_CRC_BITS;
static constexpr size_t POLAR_N = 512;
static constexpr size_t POLAR_E = FEC_CODED_BITS;

#include "horse_fec_v2_polar_q.inc"

/* ETSI TS 138 212 V16.2.0 clause 5.1 CRC24C. */
static constexpr uint32_t CRC24C_POLY = 0x1B2B117u;

static uint32_t crc24c(const uint8_t *bits, size_t n)
{
    uint32_t r = 0;
    for (size_t i = 0; i < n; i++) {
        uint32_t b = (r >> 23) & 1u;
        r = ((r << 1) & 0xFFFFFFu) ^ bits[i];
        if (b)
            r ^= (CRC24C_POLY & 0xFFFFFFu);
    }
    for (size_t i = 0; i < 24; i++) {
        uint32_t b = (r >> 23) & 1u;
        r = (r << 1) & 0xFFFFFFu;
        if (b)
            r ^= (CRC24C_POLY & 0xFFFFFFu);
    }
    return r;
}

static void bits_from_bytes(const uint8_t *bytes, size_t nbits, uint8_t *bits)
{
    for (size_t i = 0; i < nbits; i++)
        bits[i] = (bytes[i / 8] >> (7 - (i % 8))) & 1u;
}

static void bytes_from_bits(const uint8_t *bits, size_t nbits, uint8_t *bytes)
{
    size_t nb = (nbits + 7) / 8;
    memset(bytes, 0, nb);
    for (size_t i = 0; i < nbits; i++)
        if (bits[i])
            bytes[i / 8] |= (uint8_t)(0x80u >> (i % 8));
}

/* Table 5.4.1.1-1 */
static constexpr uint8_t POLAR_P32[32] = { 0,  1,  2,  4,  3,  5,  6,  7,
                                           8,  16, 9,  17, 10, 18, 11, 19,
                                           12, 20, 13, 21, 14, 22, 15, 23,
                                           24, 25, 26, 28, 27, 29, 30, 31 };

static size_t polar_J(size_t n)
{
    const size_t B = POLAR_N / 32;
    return (size_t)POLAR_P32[n / B] * B + (n % B);
}

struct PolarCodec {
    uint8_t frozen[POLAR_N];
    uint16_t info_pos[POLAR_K];
    mutable uint64_t n_path_copy;
    mutable uint64_t n_encode_n;
    mutable uint64_t n_fcomb;
    mutable uint64_t n_leaf;

    PolarCodec() : n_path_copy(0), n_encode_n(0), n_fcomb(0), n_leaf(0)
    {
        memset(frozen, 1, sizeof frozen);
        uint8_t ftmp[POLAR_N];
        memset(ftmp, 0, sizeof ftmp);
        /* K/E = 168/368 > 7/16: shortening, clause 5.4.1.1 / 5.4.1.2 */
        for (size_t n = POLAR_E; n < POLAR_N; n++)
            ftmp[polar_J(n)] = 1;
        uint16_t qn[POLAR_N];
        size_t nq = 0;
        for (size_t i = 0; i < 1024; i++) {
            if (polar_q_1024[i] < POLAR_N)
                qn[nq++] = polar_q_1024[i];
        }
        uint16_t cand[POLAR_N];
        size_t nc = 0;
        for (size_t i = 0; i < nq; i++)
            if (!ftmp[qn[i]])
                cand[nc++] = qn[i];
        memset(frozen, 1, sizeof frozen);
        for (size_t i = 0; i < POLAR_K; i++)
            frozen[cand[nc - POLAR_K + i]] = 0;
        size_t k = 0;
        for (size_t n = 0; n < POLAR_N; n++) {
            if (!frozen[n])
                info_pos[k++] = (uint16_t)n;
        }
    }

    static void encode_kernel(uint8_t *x)
    {
        for (size_t s = 0; s < 9; s++) {
            size_t half = size_t{ 1 } << s;
            size_t step = half << 1;
            for (size_t i = 0; i < POLAR_N; i += step)
                for (size_t j = 0; j < half; j++)
                    x[i + j] ^= x[i + j + half];
        }
    }

    void encode(const uint8_t info144[FEC_INFO_BYTES],
                uint8_t coded46[FEC_CODED_BYTES]) const
    {
        uint8_t ib[FEC_INFO_BITS];
        bits_from_bytes(info144, FEC_INFO_BITS, ib);
        uint32_t c = crc24c(ib, FEC_INFO_BITS);
        uint8_t kbits[POLAR_K];
        memcpy(kbits, ib, FEC_INFO_BITS);
        for (int i = 0; i < 24; i++)
            kbits[FEC_INFO_BITS + (size_t)i] = (uint8_t)((c >> (23 - i)) & 1u);
        uint8_t u[POLAR_N];
        memset(u, 0, sizeof u);
        for (size_t i = 0; i < POLAR_K; i++)
            u[info_pos[i]] = kbits[i];
        encode_kernel(u);
        uint8_t y[POLAR_N];
        for (size_t n = 0; n < POLAR_N; n++)
            y[n] = u[polar_J(n)];
        bytes_from_bits(y, POLAR_E, coded46);
    }

    static float fcomb(float a, float b)
    {
        float sa = a < 0.f ? -1.f : 1.f;
        float sb = b < 0.f ? -1.f : 1.f;
        float ma = a < 0.f ? -a : a;
        float mb = b < 0.f ? -b : b;
        return sa * sb * (ma < mb ? ma : mb);
    }

    static void encode_n(uint8_t *x, size_t n)
    {
        for (size_t s = 1; s < n; s <<= 1) {
            for (size_t i = 0; i < n; i += s * 2)
                for (size_t j = 0; j < s; j++)
                    x[i + j] ^= x[i + j + s];
        }
    }

    struct SclPath {
        uint8_t u[POLAR_N];
        float metric;
        std::vector<std::vector<float>> yst;
    };

    void scl_rec(size_t n, size_t u0, std::vector<SclPath> &ps,
                 size_t Lmax) const
    {
        if (n == 1) {
            n_leaf += ps.size();
            std::vector<SclPath> nxt;
            nxt.reserve(ps.size() * 2);
            for (SclPath &p : ps) {
                float l = p.yst.back()[0];
                if (frozen[u0]) {
                    p.u[u0] = 0;
                    if (l < 0.f)
                        p.metric += -l;
                    nxt.push_back(p);
                } else {
                    n_path_copy += 2;
                    SclPath a = p;
                    SclPath b = p;
                    a.u[u0] = 0;
                    b.u[u0] = 1;
                    if (l < 0.f)
                        a.metric += -l;
                    else
                        b.metric += l;
                    nxt.push_back(a);
                    nxt.push_back(b);
                }
            }
            if (nxt.size() > Lmax) {
                std::nth_element(nxt.begin(), nxt.begin() + (ptrdiff_t)Lmax,
                                 nxt.end(),
                                 [](const SclPath &a, const SclPath &b) {
                                     return a.metric < b.metric;
                                 });
                nxt.resize(Lmax);
            }
            ps.swap(nxt);
            return;
        }
        const size_t h = n / 2;
        for (SclPath &p : ps) {
            const std::vector<float> &y = p.yst.back();
            std::vector<float> left(h);
            for (size_t k = 0; k < h; k++) {
                left[k] = fcomb(y[k], y[k + h]);
                n_fcomb++;
            }
            p.yst.push_back(std::move(left));
        }
        scl_rec(h, u0, ps, Lmax);
        for (SclPath &p : ps) {
            p.yst.pop_back();
            uint8_t xl[POLAR_N];
            memcpy(xl, p.u + u0, h);
            encode_n(xl, h);
            n_encode_n++;
            const std::vector<float> &y = p.yst.back();
            std::vector<float> right(h);
            for (size_t k = 0; k < h; k++)
                right[k] = y[k + h] + (xl[k] ? -y[k] : y[k]);
            p.yst.push_back(std::move(right));
        }
        scl_rec(h, u0 + h, ps, Lmax);
        for (SclPath &p : ps)
            p.yst.pop_back();
    }

    /* CA-SCL. llr: + means bit 0 preferred. Returns false on CRC fail. */
    bool decode(const float *llr368, size_t list_size,
                uint8_t info144[FEC_INFO_BYTES]) const
    {
        float L[POLAR_N];
        for (size_t n = 0; n < POLAR_N; n++)
            L[n] = 0.f;
        for (size_t n = 0; n < POLAR_E; n++)
            L[polar_J(n)] = llr368[n];
        for (size_t n = POLAR_E; n < POLAR_N; n++)
            L[polar_J(n)] = 1.0e4f;

        std::vector<SclPath> ps(1);
        memset(ps[0].u, 0, POLAR_N);
        ps[0].metric = 0.f;
        ps[0].yst.emplace_back(L, L + POLAR_N);
        scl_rec(POLAR_N, 0, ps, list_size);

        size_t best = ps.size();
        float bm = 1.0e30f;
        uint8_t kbits[POLAR_K];
        for (size_t p = 0; p < ps.size(); p++) {
            for (size_t i = 0; i < POLAR_K; i++)
                kbits[i] = ps[p].u[info_pos[i]];
            uint32_t got = 0;
            for (int i = 0; i < 24; i++)
                got = (got << 1) | kbits[FEC_INFO_BITS + (size_t)i];
            uint32_t want = crc24c(kbits, FEC_INFO_BITS);
            if (got == want && ps[p].metric < bm) {
                bm = ps[p].metric;
                best = p;
                bytes_from_bits(kbits, FEC_INFO_BITS, info144);
            }
        }
        return best < ps.size();
    }
};

struct Repeat2Codec {
    void encode(const uint8_t info144[FEC_INFO_BYTES],
                uint8_t coded46[FEC_CODED_BYTES]) const
    {
        uint8_t raw[LDPC_VOICE_PAYLOAD_BYTES];
        memset(raw, 0, sizeof raw);
        memcpy(raw, info144, FEC_INFO_BYTES);
        ldpc_horse_encode_voice(raw, coded46);
    }
    bool decode_hard(const uint8_t coded46[FEC_CODED_BYTES],
                     uint8_t info144[FEC_INFO_BYTES]) const
    {
        uint8_t raw[LDPC_VOICE_PAYLOAD_BYTES];
        ldpc_horse_decode_voice(coded46, raw);
        memcpy(info144, raw, FEC_INFO_BYTES);
        return true;
    }
    bool decode_soft(const float *llr, uint8_t info144[FEC_INFO_BYTES]) const
    {
        uint8_t hard[FEC_CODED_BYTES];
        uint8_t bits[FEC_CODED_BITS];
        for (size_t i = 0; i < FEC_CODED_BITS; i++)
            bits[i] = llr[i] < 0.f ? 1 : 0;
        bytes_from_bits(bits, FEC_CODED_BITS, hard);
        return decode_hard(hard, info144);
    }
};

struct M17VoiceCodec {
    void encode(const uint8_t info144[FEC_INFO_BYTES],
                uint8_t coded46[FEC_CODED_BYTES]) const
    {
        M17::ConvolutionalEncoder enc;
        std::array<uint8_t, 37> encoded{};
        enc.reset();
        enc.encode(info144, encoded.data(), FEC_INFO_BYTES);
        encoded[36] = (uint8_t)enc.flush();
        std::array<uint8_t, 34> punct{};
        M17::puncture(encoded, punct, M17::DATA_PUNCTURE);
        std::array<uint8_t, FEC_CODED_BYTES> frame{};
        memcpy(frame.data(), punct.data(), 34);
        for (size_t i = 34 * 8; i < FEC_CODED_BITS; i++) {
            size_t src = i % (34 * 8);
            bool b = M17::getBit(punct, src);
            M17::setBit(frame, i, b);
        }
        M17::interleave(frame);
        M17::decorrelate(frame);
        memcpy(coded46, frame.data(), FEC_CODED_BYTES);
    }

    bool decode_hard(const uint8_t coded46[FEC_CODED_BYTES],
                     uint8_t info144[FEC_INFO_BYTES]) const
    {
        std::array<uint8_t, FEC_CODED_BYTES> frame{};
        memcpy(frame.data(), coded46, FEC_CODED_BYTES);
        M17::decorrelate(frame);
        M17::deinterleave(frame);
        std::array<uint8_t, 34> punct{};
        memcpy(punct.data(), frame.data(), 34);
        std::array<uint8_t, FEC_INFO_BYTES> out{};
        M17::HardViterbi vit;
        vit.decodePunctured(punct, out, M17::DATA_PUNCTURE);
        memcpy(info144, out.data(), FEC_INFO_BYTES);
        return true;
    }

    bool decode_soft(const float *llr, uint8_t info144[FEC_INFO_BYTES]) const
    {
        std::array<uint8_t, FEC_CODED_BYTES> hard{};
        for (size_t i = 0; i < FEC_CODED_BITS; i++)
            M17::setBit(hard, i, llr[i] < 0.f);
        M17::decorrelate(hard);
        M17::deinterleave(hard);
        std::array<uint16_t, 34 * 8> soft{};
        for (size_t i = 0; i < 34 * 8; i++) {
            bool b = M17::getBit(hard, i);
            soft[i] = b ? 0xFFFFu : 0;
        }
        std::array<uint8_t, FEC_INFO_BYTES> out{};
        M17::SoftViterbi vit;
        vit.decodePunctured(soft, out, M17::DATA_PUNCTURE);
        memcpy(info144, out.data(), FEC_INFO_BYTES);
        return true;
    }
};

/* CCSDS 231.1-O-1 / 231.0-B-4 (512,256): G = [I | W], W from Table 4-2. */
struct Ccsds512 {
    uint8_t W[256][256];

    Ccsds512()
    {
        memset(W, 0, sizeof W);
        const char *seeds[4] = {
            "1D21794A22761FAE59945014257E130D74D60540037940142DADEB9CA25EF12E",
            "60E0B6623C5CE5124D2C81ECC7F469AB20678DBFB7523ECE2B54B906A9DBE98C",
            "F6739BCF54273E77167BDA120C6C47744C071EFF5E32A7593138670C095C39B5",
            "28706BD0453002582DAB85F05B9201D08DFDEE2D9D84CA88B371FAE63A4EB07E"
        };
        const int M = 64;
        for (int blk = 0; blk < 4; blk++) {
            uint8_t row[256];
            hex_to_bits(seeds[blk], row, 256);
            for (int r = 0; r < M; r++) {
                for (int c = 0; c < 256; c++)
                    W[blk * M + r][c] = row[c];
                rot_blocks(row, M);
            }
        }
    }

    static int hexv(char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return c - 'a' + 10;
    }
    static void hex_to_bits(const char *h, uint8_t *bits, int n)
    {
        for (int i = 0; i < n / 4; i++) {
            int v = hexv(h[i]);
            bits[4 * i] = (uint8_t)((v >> 3) & 1);
            bits[4 * i + 1] = (uint8_t)((v >> 2) & 1);
            bits[4 * i + 2] = (uint8_t)((v >> 1) & 1);
            bits[4 * i + 3] = (uint8_t)(v & 1);
        }
    }
    static void rot_blocks(uint8_t *row, int M)
    {
        /* Independent right circular shift of each M-bit circulant. */
        for (int b = 0; b < 256 / M; b++) {
            uint8_t last = row[b * M + M - 1];
            memmove(row + b * M + 1, row + b * M, (size_t)(M - 1));
            row[b * M] = last;
        }
    }

    void encode_cw(const uint8_t info256[256], uint8_t cw512[512]) const
    {
        memcpy(cw512, info256, 256);
        for (int i = 0; i < 256; i++) {
            uint8_t s = 0;
            for (int j = 0; j < 256; j++)
                s ^= (uint8_t)(info256[j] & W[j][i]);
            cw512[256 + i] = s;
        }
    }

    void encode(const uint8_t info144[FEC_INFO_BYTES],
                uint8_t coded46[FEC_CODED_BYTES]) const
    {
        uint8_t ib[256];
        memset(ib, 0, sizeof ib);
        bits_from_bytes(info144, FEC_INFO_BITS, ib);
        uint8_t cw[512];
        encode_cw(ib, cw);
        bytes_from_bits(cw, FEC_CODED_BITS, coded46);
    }

    bool decode_minsum(const float *llr368, int iters,
                       uint8_t info144[FEC_INFO_BYTES]) const
    {
        float L[512];
        for (int i = 0; i < 512; i++)
            L[i] = (i < (int)FEC_CODED_BITS) ? llr368[i] : 0.f;
        for (int i = FEC_INFO_BITS; i < 256; i++)
            L[i] = 1.0e4f; /* known filler zeros */
        /* H = [W^T | I] */
        std::vector<std::pair<int, int>> edges;
        for (int r = 0; r < 256; r++) {
            for (int c = 0; c < 256; c++)
                if (W[c][r])
                    edges.push_back({ r, c });
            edges.push_back({ r, 256 + r });
        }
        const int E = (int)edges.size();
        std::vector<float> msg_c2v(E, 0.f);
        std::vector<float> msg_v2c(E, 0.f);
        std::vector<std::vector<int>> vn(512), cn(256);
        for (int e = 0; e < E; e++) {
            cn[edges[e].first].push_back(e);
            vn[edges[e].second].push_back(e);
        }
        for (int it = 0; it < iters; it++) {
            for (int v = 0; v < 512; v++) {
                for (int e : vn[v]) {
                    float s = L[v];
                    for (int e2 : vn[v])
                        if (e2 != e)
                            s += msg_c2v[e2];
                    msg_v2c[e] = s;
                }
            }
            for (int c = 0; c < 256; c++) {
                for (int e : cn[c]) {
                    float s = 1.f, m = 1.0e6f;
                    for (int e2 : cn[c]) {
                        if (e2 == e)
                            continue;
                        float x = msg_v2c[e2];
                        s *= (x < 0.f) ? -1.f : 1.f;
                        float ax = x < 0.f ? -x : x;
                        if (ax < m)
                            m = ax;
                    }
                    msg_c2v[e] = s * m;
                }
            }
        }
        uint8_t hard[256];
        for (int v = 0; v < 256; v++) {
            float s = L[v];
            for (int e : vn[v])
                s += msg_c2v[e];
            hard[v] = s < 0.f ? 1 : 0;
        }
        bytes_from_bits(hard, FEC_INFO_BITS, info144);
        uint8_t cw[512];
        encode_cw(hard, cw);
        int syn = 0;
        for (int i = 0; i < 256; i++) {
            float s = L[256 + i];
            for (int e : vn[256 + i])
                s += msg_c2v[e];
            uint8_t b = s < 0.f ? 1 : 0;
            if (b != cw[256 + i])
                syn = 1;
        }
        (void)syn;
        return true;
    }
};

#endif
