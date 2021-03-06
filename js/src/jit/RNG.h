#ifndef jit_RNG_h
#define jit_RNG_h

#include <vector>

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

    // Returns an array of length |size| whose elements are a random 
    // permutation of {0, ..., |size| - 1}.
    // The caller is responsible for calling delete[] on the return value.
    static unsigned* createIndexPermutation(unsigned size);
    
    // In-place permutations.
    template <typename T>
    static void permute(T* arr, unsigned size);
    template <typename T>
    static void permute(std::vector<T> &v);

  private:
    static void init();
    static mozilla::non_crypto::XorShift128PlusRNG *randomNumberGenerator;
};
}  // namespace jit
}  // namespace js
#endif /* jit_RNG_h */
