#ifndef RNGSUPPORT_H
#define RNGSUPPORT_H

#ifdef __cplusplus

#if __cplusplus >= 201103L || (defined _MSC_VER && _MSC_VER >= 1600)
#define NYQ_USE_RANDOM_HEADER
#endif

#if defined NYQ_USE_RANDOM_HEADER

#include <vector>
#include <random>

namespace Nyq
{
typedef std::mt19937 nyq_generator;
const int nyq_generator_state_size = nyq_generator::state_size;
typedef std::seed_seq nyq_seed_seq;
typedef std::uniform_real_distribution<float> nyq_uniform_float_distribution;
typedef std::normal_distribution<float> nyq_normal_float_distribution;
typedef std::uniform_int_distribution<int> nyq_uniform_int_distribution;

namespace RngSupport
{
std::vector<unsigned int> CreateSeedVector(std::vector<unsigned int>::size_type size);
} // namespace Rng

template <class RNG = nyq_generator>
static RNG CreateGenerator(int size = 32)
{
    auto seed_data = RngSupport::CreateSeedVector(size);

    nyq_seed_seq seq(seed_data.begin(), seed_data.end());

    RNG generator(seq);

    return generator;
}

template <class RNG = nyq_generator>
static void ReseedGenerator(RNG& generator, int size = 32)
{
    auto seed_data = RngSupport::CreateSeedVector(size);

    nyq_seed_seq seq(seed_data.begin(), seed_data.end());

    generator.seed(seq);
}

template <class RNG = nyq_generator>
class NyqEngine : public RNG
{
public:
   explicit NyqEngine(int size = 32) : RNG(CreateGenerator(size))
   { }
};

} // namespace Nyq

#endif // NYQ_USE_RANDOM_HEADER

extern "C" {
#endif // __cplusplus

    void RandomFillUniformFloat(float* p, int count, float low, float high);
    void RandomFillNormalFloat(float* p, int count, float mean, float sigma);
    int RandomFillClampedNormalFloat(float* p, int count, float mean, float sigma, float low, float high);
    float RandomUniformFloat(float low, float high);

    int RandomUniformInt(int lowInclusive, int highExclusive);

#ifdef __cplusplus
} //   extern "C"
#endif

#endif // RNGSUPPORT_H
