#include "RngSupport.h"

#if defined NYQ_USE_RANDOM_HEADER

#include <cassert>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <random>

//#define USE_RDRAND

#ifdef USE_RDRAND
#include <immintrin.h>
#endif

using namespace std;

using namespace Nyq;

#ifdef USE_RDRAND
static bool HasRdRand()
{
    int output[4];

    __cpuid(output, 0);

    auto isIntel = 0 == memcmp("Genu" "ntel" "ineI", &output[1], 12);

    if (isIntel)
    {
        __cpuid(output, 1);

        const int IntelRdRandFlag = 1 << 30;

        return 0 != (output[2] & IntelRdRandFlag);
    }

    auto isAmd = 0 == memcmp("Auth" "cAMD" "enti", &output[1], 12);

    if (isAmd)
    {
        const int AmdRdRandFlag = 1 << 30;

        __cpuid(output, 0x80000005);

        return 0 != (output[2] & AmdRdRandFlag);
    }

    return false;
}
#endif

static vector<unsigned int> CreateRootSeedVector()
{
    random_device rd;

    std::vector<decltype(rd)::result_type> seed_data;

    const int reserve_size = nyq_generator_state_size + 3;

    seed_data.reserve(reserve_size);

    generate_n(std::back_inserter(seed_data), nyq_generator_state_size, ref(rd));

    // Protect against a broken random_device
    const auto timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    seed_data.push_back(static_cast<unsigned int>(timestamp) & 0xffffffff);
    seed_data.push_back(static_cast<unsigned int>(timestamp >> 32));

    static atomic<int> counter;

    const auto x = counter.fetch_add(1, memory_order_relaxed);

    seed_data.push_back(x);

    assert(seed_data.size() == reserve_size);

#ifdef USE_RDRAND
    if (HasRdRand())
    {
        const int RndWords = nyq_generator_state_size;

        auto required = seed_data.size() + RndWords;

        if (required > seed_data.capacity())
            seed_data.reserve(required);

        for (int i = 0; i < RndWords; ++i)
        {
            unsigned n;

            for (int retry = 10; retry > 0; --retry)
            {
                if (_rdrand32_step(&n))
                {
                    seed_data.push_back(n);

                    break;
                }
            }
        }
}
#endif

    return seed_data;
}

template <class RNG = nyq_generator>
static RNG CreateRootGenerator()
{
    auto seed_data = CreateRootSeedVector();

    std::seed_seq seed(seed_data.begin(), seed_data.end());

    return RNG{seed};
}

#ifdef _MSC_VER
#if _MSC_VER < 1900
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL thread_local
#endif
#else
#define THREAD_LOCAL __thread
#endif

template <class RNG = nyq_generator>
static RNG& GetRootGenerator()
{
    static THREAD_LOCAL RNG generator{ CreateRootGenerator<RNG>() };

    return generator;
}

namespace Nyq
{
namespace RngSupport
{
std::vector<unsigned int> CreateSeedVector(std::vector<unsigned int>::size_type size)
{
    if (size < 1)
       size = 1;

    vector<unsigned int> seed;

    seed.reserve(size);

    auto& rng = GetRootGenerator();

    std::uniform_int_distribution<unsigned> uniform;

    std::generate_n(std::back_inserter(seed), size, [&] { return uniform(rng); });

    return seed;
}
} // namespace RngSupport
} // namespace Nyq

extern "C"
void RandomFillUniformFloat(float* p, int count, float low, float high)
{
    if (count < 1)
        return;

    auto& generator = GetRootGenerator();

    nyq_uniform_float_distribution uniform{ low, high };

    std::generate_n(p, count, [&] { return uniform(generator); });
}

extern "C"
void RandomFillNormalFloat(float* p, int count, float mean, float sigma)
{
    if (count < 1)
        return;

    nyq_generator& generator = GetRootGenerator();

    nyq_normal_float_distribution normal{ mean, sigma };

    std::generate_n(p, count, [&] { return normal(generator); });
}

extern "C"
int RandomFillClampedNormalFloat(float* p, int count, float mean, float sigma, float low, float high)
{
    if (count < 1)
        return 1;

    nyq_generator& generator = GetRootGenerator();

    nyq_normal_float_distribution normal{ mean, sigma };

    while (count--)
    {
       auto retry = 10;

        for (;;)
        {
           const auto x = normal(generator);

            if (x <= high && x >= low)
            {
                *p++ = x;
                break;
            }

            if (--retry <= 0)
                return 0;
        }
    }

    return 1;
}

extern "C"
float RandomUniformFloat(float low, float high)
{
   nyq_uniform_float_distribution uniform{ low, high };

   return uniform(GetRootGenerator());
}

extern "C"
double RandomUniformDouble(double low, double high)
{
   nyq_uniform_double_distribution uniform{ low, high };

   return uniform(GetRootGenerator());
}

extern "C"
int RandomUniformInt(int lowInclusive, int highInclusive)
{
    nyq_uniform_int_distribution uniform{ lowInclusive, highInclusive };

    return uniform(GetRootGenerator());
}

extern "C"
long RandomUniformLong(long lowInclusive, long highInclusive)
{
   nyq_uniform_long_distribution uniform{ lowInclusive, highInclusive };

   return uniform(GetRootGenerator());
}

#else // NYQ_USE_RANDOM_HEADER

#include <stdlib.h>
#include <math.h>

const float fRandScale = 1.f / RAND_MAX;
const float f2RandScale = 2.f / RAND_MAX;

inline void TwoNormalFloats(float& a, float& b)
{
    float u, v, d2;

    do {
        u = f2RandScale * rand() - 1;
        v = f2RandScale * rand() - 1;
        d2 = u * u + v * v;
    } while (d2 >= 1 || d2 == 0);

    float scale = static_cast<float>(sqrt(-2. * log(d2) / d2));

    a = v * scale;
    b = u * scale;
}

extern "C"
void RandomFillUniformFloat(float* p, int count, float low, float high)
{
    if (count < 1)
        return;

    const float scale = fRandScale * (high - low);

    while (count--)
        *p++ = low + scale * rand();
}

extern "C"
void RandomFillNormalFloat(float* p, int count, float mean, float sigma)
{
    if (count < 1)
        return;

    for (; count >= 2; count -= 2)
    {
        float a, b;

        TwoNormalFloats(a, b);

        *p++ = mean + sigma * a;
        *p++ = mean + sigma * b;
    }

    if (count > 1)
    {
        float a, b;

        TwoNormalFloats(a, b);

        *p++ = mean + sigma * a;
    }
}

extern "C"
int RandomFillClampedNormalFloat(float* p, int count, float mean, float sigma, float low, float high)
{
    if (count < 1)
        return 1;

    int reject = 0;
    const int max_reject = 5;

    for (int i = 0; i < count; )
    {
        float a, b;

        TwoNormalFloats(a, b);

        a = mean + sigma * a;

        if (a >= low && a <= high)
        {
            ++i;
            *p++ = a;
            reject = 0;
        }
        else
        {
            if (++reject >= max_reject)
                return 0;
        }

        if (i < count)
        {
            b = mean + sigma * b;

            if (b >= low && b <= high)
            {
                ++i;
                *p++ = b;
                reject = 0;
            }
            else
            {
                if (++reject >= max_reject)
                    return 0;
            }
        }
    }

    if (count > 1)
    {
        float a, b;

        TwoNormalFloats(a, b);

        *p++ = mean + sigma * a;
    }

    return 1;
}

extern "C"
float RandomUniformFloat(float low, float high)
{
    return low + (high - low) * fRandScale * rand();
}

extern "C"
int RandomUniformInt(int lowInclusive, int highExclusive)
{
    const int range = highExclusive - lowInclusive;

    if (range < 1 || range > RAND_MAX)
        return lowInclusive;

    const int whole = RAND_MAX / range;
    const int max_uniform = whole * range;

    if (max_uniform < 1)
        return lowInclusive;

    for (;;)
    {
        int r = rand();

        if (r <= max_uniform)
            return r % range + lowInclusive;
    }
}

#endif // NYQ_USE_RANDOM_HEADER
