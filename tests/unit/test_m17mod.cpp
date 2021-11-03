#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "protocols/M17/M17IntegerModulator.h"
#include "protocols/M17/M17LookupModulator.h"
#include "protocols/M17/M17Modulator.h"

namespace chrono = std::chrono;

using lclock = std::chrono::high_resolution_clock;

static constexpr int N = 1000;

int x4mance_test_standard()
{
    M17Modulator m;

    std::array<uint8_t, 2> a;
    std::array<uint8_t, 46> b;

    m.init();
    for (int i = 0; i < 100; i++)
        m.send(a, b);

    auto t1 = lclock::now();
    int ctr = 0;
    for (int i = 0; i < N; i++)
    {
        m.send(a, b);
        ctr += m.getOutputBuffer()->at(i % 1920);
    }
    auto t2 = lclock::now();
    auto d  = t2 - t1;
    long x  = chrono::duration_cast<chrono::milliseconds>(d).count();

    printf("M17Modulator standard: %lf msec\n", x / float(N));
    return ctr;
}

int x4mance_test_lookup()
{
    M17LookupModulator m;

    std::array<uint8_t, 2> a;
    std::array<uint8_t, 46> b;

    m.init();
    for (int i = 0; i < 100; i++)
        m.send(a, b);
    auto t1 = lclock::now();
    int ctr = 0;

    for (int i = 0; i < N; i++)
    {
        m.send(a, b);
        ctr += m.getOutputBuffer()->at(i % 1920);
    }
    auto t2 = lclock::now();
    auto d  = t2 - t1;
    long x  = chrono::duration_cast<chrono::milliseconds>(d).count();

    printf("M17Modulator lookup: %lf msec\n", x / float(N));
    return ctr;
}

int x4mance_test_integer()
{
    M17IntegerModulator m;

    std::array<uint8_t, 2> a;
    std::array<uint8_t, 46> b;

    m.init();
    for (int i = 0; i < 100; i++)
        m.send(a, b);
    auto t1 = lclock::now();
    int ctr = 0;
    for (int i = 0; i < N; i++)
    {
        m.send(a, b);
        ctr += m.getOutputBuffer()->at(i % 1920);
    }
    auto t2 = lclock::now();
    auto d  = t2 - t1;
    long x  = chrono::duration_cast<chrono::milliseconds>(d).count();

    printf("M17Modulator integer: %lf msec\n", x / float(N));
    return ctr;
}

std::vector<int16_t> ifir_test(int input_id)
{
    srand(input_id);

    M17LookupModulator m;
    m.init();

    std::array<uint8_t, 2> a;
    std::array<uint8_t, 46> b;
    for (int i = 0; i < 2; i++)
        a[i] = rand();

    for (int i = 0; i < 46; i++)
        b[i] = rand();

    m.send(a, b);
    auto* out = m.getOutputBuffer();  // 1920 bytes
    assert(out->size() == 1920);

    std::vector<int16_t> oo(1920);
    memcpy(oo.data(), out, 1920 * 2);
    return oo;
}

std::vector<int16_t> fir_test(int input_id)
{
    srand(input_id);

    M17Modulator m;
    m.init();

    std::array<uint8_t, 2> a;
    std::array<uint8_t, 46> b;

    for (int i = 0; i < 2; i++)
        a[i] = rand();

    for (int i = 0; i < 46; i++)
        b[i] = rand();

    m.send(a, b);
    auto* out = m.getOutputBuffer();  // 1920 bytes
    assert(out->size() == 1920);

    std::vector<int16_t> oo(1920);
    memcpy(oo.data(), out, 1920 * 2);
    return oo;
}

std::vector<int16_t> jfir_test(int input_id)
{
    srand(input_id);

    M17IntegerModulator m;
    m.init();

    std::array<uint8_t, 2> a;
    std::array<uint8_t, 46> b;

    for (int i = 0; i < 2; i++)
        a[i] = rand();

    for (int i = 0; i < 46; i++)
        b[i] = rand();

    m.send(a, b);
    auto* out = m.getOutputBuffer();  // 1920 bytes
    assert(out->size() == 1920);

    std::vector<int16_t> oo(1920);
    memcpy(oo.data(), out, 1920 * 2);
    return oo;
}

bool compare(const std::vector<int16_t>& a, const std::vector<int16_t>& b,
             int maxdiff)
{
    assert(a.size() == b.size());

    bool eq = true;
    for (size_t i = 0; i < a.size(); i++)
        if (abs(a[i] - b[i]) > maxdiff)
        {
            printf("Difference at %lu: %d vs %d\n", i, a[i], b[i]);
            eq = false;
        }
    return eq;
}

int main()
{
    volatile int a = x4mance_test_standard();
    a += x4mance_test_integer();
    a += x4mance_test_lookup();

    for (int i = 0; i < 1000; i++)
    {
        auto a = fir_test(i);
        auto b = ifir_test(i);
        auto c = jfir_test(i);
        compare(a, b, 3);
        compare(a, c, 2);
    }

    return 0;
}
