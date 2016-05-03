#include "jit/RNG.h"

using namespace js::jit;

mozilla::non_crypto::XorShift128PlusRNG* RNG::randomNumberGenerator = nullptr;

int32_t
RNG::nextInt32(const int32_t min, const int32_t max) {
    init();
    MOZ_ASSERT(min <= max);
    if (min == INT32_MIN && max == INT32_MAX)
    return static_cast<int32_t>(randomNumberGenerator->next());

    double range = static_cast<double>(max - min + 1);
    return min + static_cast<int32_t>(randomNumberGenerator->nextDouble() * range);
}

uint32_t
RNG::nextUint32(const uint32_t min, const uint32_t max) {
    init();
    MOZ_ASSERT(min <= max);
    if (min == 0 && max == UINT32_MAX)
    return static_cast<uint32_t>(randomNumberGenerator->next());

    double range = static_cast<double>(max - min + 1);
    return min + static_cast<uint32_t>(randomNumberGenerator->nextDouble() * range);
}

uint64_t
RNG::nextUint64() {
  init();
  return randomNumberGenerator->next();
}

double
RNG::nextDouble() {
  init();
  return randomNumberGenerator->nextDouble();
}

inline void
RNG::init() {
  if (randomNumberGenerator != nullptr) return;
  mozilla::Array<uint64_t, 2> seed;
  js::GenerateXorShift128PlusSeed(seed);
  randomNumberGenerator = new mozilla::non_crypto::XorShift128PlusRNG(seed[0], seed[1]);
}
