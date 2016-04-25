#ifndef jit_RNG_h
#define jit_RNG_h

#include "jsmath.h"
#include "mozilla/Array.h"
#include "mozilla/XorShift128PlusRNG.h"

namespace js {
namespace jit{

class RNG {
  public:
    static int32_t nextInt32(const int32_t min = INT32_MIN, const int32_t max = INT32_MAX);
    static uint32_t nextUint32(const uint32_t min = 0, const uint32_t max = UINT32_MAX);
    static uint64_t nextUint64();
    static double nextDouble();

  private:
    friend bool ::JS_Init();
    static bool inited_;

    static void init();
    static mozilla::non_crypto::XorShift128PlusRNG *randomNumberGenerator;
};
}  // namespace jit
}  // namespace js
#endif /* jit_RNG_h */
