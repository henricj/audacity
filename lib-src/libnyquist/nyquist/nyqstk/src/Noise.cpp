/***************************************************/
/*! \class Noise
    \brief STK noise generator.

    Generic random number generation using the
    C rand() function.  The quality of the rand()
    function varies from one OS to another.

    by Perry R. Cook and Gary P. Scavone, 1995 - 2005.
*/
/***************************************************/

#include "Noise.h"

using namespace Nyq;

Noise :: Noise() = default;

Noise :: Noise( unsigned int seed )
{
  // Seed the random number generator
  this->setSeed( seed );
}

Noise :: ~Noise() = default;

void Noise :: setSeed( unsigned int seed )
{
  std::seed_seq seq{ seed };
  generator_.seed(seq);
}

StkFloat Noise :: computeSample()
{
  lastOutput_ = distribution_(generator_);
  return lastOutput_;
}

