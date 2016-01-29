/*
 * RNG.h
 *
 *  Created on: Jan 28, 2016
 *      Author: wilsonlian
 */

#ifdef CONSTANT_BLINDING
#ifndef jit_RNG_h
#define jit_RNG_h

#include "mozilla/Maybe.h"
#include "mozilla/XorShift128PlusRNG.h"

class RNG {
  public:
	static int32_t blindingValue(int32_t min = INT32_MIN, int64_t max = INT32_MAX) {
		static_assert(min <= max, "max must be greater than or equal to min.");
		if (randomNumberGenerator.isNothing()) {
			mozilla::Array<uint64_t, 2> seed;
			js::GenerateXorShift128PlusSeed(seed);
			randomNumberGenerator.emplace(seed[0], seed[1]);
		}
		if (min == INT64_MIN && max == INT64_MAX)
			return static_cast<int32_t>(randomNumberGenerator.ref().next());

		double range = static_cast<double>(max - min + 1);
		return min + static_cast<int32_t>(randomNumberGenerator.ref().nextDouble() * range);
	}

  private:
	// Random number generator for code diversification.
	static mozilla::Maybe<mozilla::non_crypto::XorShift128PlusRNG> randomNumberGenerator;
}

#endif /* jit_RNG_h */
#endif /* CONSTANT_BLINDING
