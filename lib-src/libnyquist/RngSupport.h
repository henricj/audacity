#ifndef RNGSUPPORT_H
#define RNGSUPPORT_H

#ifdef __cplusplus

#if __cplusplus >= 201103L || (defined _MSC_VER && _MSC_VER >= 1600)
#define NYQ_USE_RANDOM_HEADER
#endif

#if defined NYQ_USE_RANDOM_HEADER

#include <vector>
#include <random>
#include "random_xoshiro256starstar.h"

namespace Nyq
{
typedef std::seed_seq nyq_seed_seq;
typedef std::uniform_real_distribution<float> nyq_uniform_float_distribution;
typedef std::uniform_real_distribution<double> nyq_uniform_double_distribution;
typedef std::normal_distribution<float> nyq_normal_float_distribution;
typedef std::uniform_int_distribution<int> nyq_uniform_int_distribution;
typedef std::uniform_int_distribution<long> nyq_uniform_long_distribution;

namespace RngSupport
{
typedef ExtraGenerators::xoshiro256starstar nyq_generator;

nyq_generator CreateGenerator();
} // namespace RngSupport


class NyqEngine : public RngSupport::nyq_generator
{
public:
   explicit NyqEngine() : RngSupport::nyq_generator(RngSupport::CreateGenerator())
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
    double RandomUniformDouble(double low, double high);

    int RandomUniformInt(int lowInclusive, int highInclusive);
    long RandomUniformLong(long lowInclusive, long highInclusive);

#ifdef __cplusplus
} //   extern "C"
#endif

#endif // RNGSUPPORT_H
