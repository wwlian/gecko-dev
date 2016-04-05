#ifndef jit_arm_RegisterRandomizer_arm_h
#define jit_arm_RegisterRandomizer_arm_h

#include <array>

#include "jit/arm/Architecture-arm.h"
#include "jit/RegisterSets.h"
#include "jit/RNG.h"

namespace js {
namespace jit {

class RegisterRandomizer {
  private:
    static const Registers::SetType randomizationMask = 
        (Registers::AllMask
        & ~(1 << Registers::ip)
        & ~(1 << Registers::sp)
        & ~(1 << Registers::lr)
        & ~(1 << Registers::pc));

    // |substitutions_| maps a Registers::Encoding to the Registers::Encoding
    // that will actually be used in its stead in JIT code. Can be used to
    // re-diversify registers when returning from a call into native code.
    std::array<Registers::Encoding, Registers::Total> substitutions_;
    // |reverseSubstitutions_| reverses the |substitutions_| map. Can be used 
    // to undiversify registers before calling into native code.
    std::array<Registers::Encoding, Registers::Total> reverseSubstitutions_;

    Registers::Code maxRandomRegister_;

    static bool isRandomizedRegister(unsigned int r) {
      return randomizationMask & (1 << r);
    }

    RegisterRandomizer() {
      maxRandomRegister_ = 0;
      // Initialize arrays to identity map, and find highest randomized register.
      for (size_t i = 0; i < Registers::Total; i++) {
        if (isRandomizedRegister(i)) {
          maxRandomRegister_ = i;
        }
        substitutions_[i] = Registers::Encoding(i);
        reverseSubstitutions_[i] = Registers::Encoding(i);
      }

      for (size_t i = 0; i < maxRandomRegister_; i++) {
        if (!isRandomizedRegister(i)) continue;
        int swapRegister = RNG::nextInt32(i, maxRandomRegister_);
        while (!isRandomizedRegister(swapRegister)) {
          swapRegister = RNG::nextInt32(i, maxRandomRegister_);
        }
        Registers::Encoding tmp = substitutions_[i];
        substitutions_[i] = substitutions_[swapRegister];
        substitutions_[swapRegister] = tmp;

        tmp = reverseSubstitutions_[substitutions_[i]];
        reverseSubstitutions_[substitutions_[i]] = 
            reverseSubstitutions_[substitutions_[swapRegister]];
        reverseSubstitutions_[substitutions_[swapRegister]] = tmp;
      }
    }

  public:
    static RegisterRandomizer getInstance() {
      static bool initialized = false;
      static RegisterRandomizer instance;

      if (!initialized) {
        instance = RegisterRandomizer();
        initialized = true;
      }
      
      return instance;
    }

    Register getRandomizedRegister(const Register &physicalRegister) const {
        return getRandomizedRegister(physicalRegister.encoding());
    }

    Register getRandomizedRegister(const Registers::Encoding &physicalRegister) const {
      return { substitutions_[physicalRegister] };
    }

    ValueOperand getUnrandomizedValueOperand(const ValueOperand &v) const {
#if defined(JS_NUNBOX32)
      return ValueOperand(getUnrandomizedRegister(v.typeReg()),
                          getUnrandomizedRegister(v.payloadReg()));
#elif defined(JS_PUNBOX64)
      return ValueOperand(getUnrandomizedRegister(v.valueReg()));
#endif
    }

    Register getUnrandomizedRegister(const Register &r) const {
      return { reverseSubstitutions_[r.encoding()] };
    }

    Registers::SetType randomizeMask(Registers::SetType mask) {
      Registers::SetType result = 0;
      uint32_t pos;
      while (mask) {
        pos = Registers::FirstBit(mask);
        result |= (1 << getRandomizedRegister(Registers::Encoding(pos)).encoding());
        mask ^=  (1 << pos);
      }
      return result;
    }

    Registers::Code getMaxRandomRegister() {
      return maxRandomRegister_;
    }
};

} // namespace jit
} // namespace js

#endif /* jit_arm_RegisterRandomizer_arm_h */
