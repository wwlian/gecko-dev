#ifndef jit_arm_RegisterRandomizer_arm_h
#define jit_arm_RegisterRandomizer_arm_h

#include <array>

#ifdef JS_CODEGEN_ARM
#include "jit/arm/Architecture-arm.h"
#endif
#include "jit/RegisterSets.h"
#include "jit/RNG.h"

namespace js {
namespace jit {

class RegisterRandomizer {
  private:
    static const Registers::SetType RANDOMIZATION_MASK = (Registers::AllocatableMask
        & ~(1 << Registers::r0));

    // |substitutions_| maps a Registers::Encoding to the Registers::Encoding
    // that will actually be used in its stead in JIT code. Can be used to
    // re-diversify registers when returning from a call into native code.
    std::array<Registers::Encoding, Registers::Total> substitutions_;
    // |reverseSubstitutions_| reverses the |substitutions_| map. Can be used 
    // to undiversify registers before calling into native code.
    std::array<Registers::Encoding, Registers::Total> reverseSubstitutions_;

    Registers::Code maxRandomRegister_;
    RegisterRandomizer();

  public:
    static bool isRandomizedRegister(unsigned int r);


  public:
    static RegisterRandomizer getInstance();
    static Register randomize(const Register &physicalRegister);

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
