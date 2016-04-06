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
    static const Registers::SetType RANDOMIZATION_MASK = (Registers::AllocatableMask
        & ~(1 << Registers::r0)
        & ~(1 << Registers::r1)
        & ~(1 << Registers::r2)
        & ~(1 << Registers::r3));

    // |substitutions_| maps a Registers::Encoding to the Registers::Encoding
    // that will actually be used in its stead in JIT code. Can be used to
    // re-diversify registers when returning from a call into native code.
    std::array<Registers::Encoding, Registers::Total> substitutions_;
    // |reverseSubstitutions_| reverses the |substitutions_| map. Can be used 
    // to undiversify registers before calling into native code.
    std::array<Registers::Encoding, Registers::Total> reverseSubstitutions_;

    Registers::Code maxRandomRegister_;

    static bool isRandomizedRegister(unsigned int r);

    RegisterRandomizer();

  public:
    static RegisterRandomizer getInstance();

    Register getRandomizedRegister(const Register &physicalRegister) const;
    Register getRandomizedRegister(const Registers::Encoding &physicalRegister) const;
    ValueOperand getUnrandomizedValueOperand(const ValueOperand &v) const;
    Register getUnrandomizedRegister(const Register &r) const;
    Registers::SetType randomizeMask(Registers::SetType mask) const;

    Registers::Code getMaxRandomRegister() {
      return maxRandomRegister_;
    }
};

} // namespace jit
} // namespace js

#endif /* jit_arm_RegisterRandomizer_arm_h */
