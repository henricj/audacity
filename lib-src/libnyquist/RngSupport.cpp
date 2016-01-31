#include "RngSupport.h"

#include <algorithm>
#include <atomic>
#include <chrono>
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

    std::vector<unsigned int> seed_data(nyq_generator_state_size);

    generate_n(&seed_data[0], seed_data.size(), ref(rd));

    // Protect against a broken random_device
    auto timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    seed_data.push_back(static_cast<unsigned int>(timestamp));
    seed_data.push_back(static_cast<unsigned int>(timestamp >> 32));

    static atomic<int> counter;

    auto x = ++counter;

    seed_data.push_back(x);

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

    RNG generator(seed);

    return generator;
}

template <class RNG = nyq_generator>
static RNG& GetRootGenerator()
{
    static thread_local auto generator = CreateRootGenerator<RNG>();

    return generator;
}

namespace Nyq
{
namespace RngSupport
{
std::vector<unsigned int> CreateSeedVector(std::vector<unsigned int>::size_type size)
{
    vector<unsigned int> seed;

    seed.reserve(size);

    auto rng = GetRootGenerator();

    for (auto i = 0; i < size; ++i)
        seed.push_back(rng());

    return seed;
}
}
}

extern "C"
void RandomFillUniformFloat(float* p, int count, float low, float high)
{
    if (count < 1)
        return;

    auto& generator = GetRootGenerator();

    nyq_uniform_float_distribution uniform{ low, high };

    while (count--)
        *p++ = uniform(generator);
}

extern "C"
void RandomFillNormalFloat(float* p, int count, float mean, float sigma)
{
    if (count < 1)
        return;

    auto& generator = GetRootGenerator();

    nyq_normal_float_distribution uniform{ mean, sigma };

    while (count--)
        *p++ = uniform(generator);
}

extern "C"
int RandomFillClampedNormalFloat(float* p, int count, float mean, float sigma, float low, float high)
{
    if (count < 1)
        return 1;

    auto& generator = GetRootGenerator();

    nyq_normal_float_distribution uniform{ mean, sigma };

    while (count--)
    {
        int retry = 10;

        for (;;)
        {
            float x = uniform(generator);

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
    auto& generator = GetRootGenerator();

    nyq_uniform_float_distribution uniform{ low, high };

    return uniform(generator);
}

extern "C"
int RandomUniformInt(int lowInclusive, int highExclusive)
{
    auto& generator = GetRootGenerator();

    nyq_uniform_int_distribution uniform{ lowInclusive, highExclusive };

    return uniform(generator);
}
