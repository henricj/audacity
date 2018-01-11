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

Noise :: Noise()
{
  lastOutput_ = static_cast<StkFloat>(0.0);
}

Noise :: Noise( unsigned int seed )
{
  // Seed the random number generator
  this->setSeed( seed );
  lastOutput_ = static_cast<StkFloat>(0.0);
}

Noise :: ~Noise() = default;

void Noise :: setSeed( unsigned int seed )
{
  if (seed == 0)
    ReseedGenerator(generator_);
  else
    generator_.seed(seed);
}

StkFloat Noise :: computeSample()
{
  lastOutput_ = distribution_(generator_);
  return lastOutput_;
}

