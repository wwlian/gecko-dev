/*
 * RNG.h
 *
 *  Created on: Jan 28, 2016
 *      Author: wilsonlian
 */

#ifdef CONSTANT_BLINDING
#ifndef jit_RNG_h
#define jit_RNG_h

#include "mozilla/XorShift128PlusRNG.h"

class RNG {
  public:
  RNG() {
			mozilla::Array<uint64_t, 2> seed;
			js::GenerateXorShift128PlusSeed(seed);
      randomNumberGenerator = new mozilla::non_crypto::XorShift128PlusRNG(seed[0], seed[1]);
  }

  ~RNG() {
    delete randomNumberGenerator;
  }

	int32_t blindingValue(const int32_t min = INT32_MIN, const int32_t max = INT32_MAX) {
		MOZ_ASSERT(min <= max);
		if (min == INT32_MIN && max == INT32_MAX)
			return static_cast<int32_t>(randomNumberGenerator->next());

		double range = static_cast<double>(max - min + 1);
		return min + static_cast<int32_t>(randomNumberGenerator->nextDouble() * range);
	}

  private:
	// Random number generator for code diversification.
	mozilla::non_crypto::XorShift128PlusRNG *randomNumberGenerator;
};

#endif /* jit_RNG_h */
#endif /* CONSTANT_BLINDING */
