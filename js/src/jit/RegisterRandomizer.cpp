#include "jit/RegisterRandomizer.h"

namespace js {
namespace jit {

RegisterRandomizer::RegisterRandomizer() {
  maxRandomRegister_ = 0;
  Registers::Code maxVolatileRegister = 0;
  Registers::Code maxNonVolatileRegister = 0;

  // Initialize arrays to identity map, and find highest randomized register.
  for (size_t i = 0; i < Registers::Total; i++) {
      if (isRandomizedRegister(i)) {
          maxRandomRegister_ = i;
          if (Register::FromCode(i).volatile_()) {
              maxVolatileRegister = i;
          } else {
              maxNonVolatileRegister = i;
          }
      }
      substitutions_[i] = Registers::Encoding(i);
      reverseSubstitutions_[i] = Registers::Encoding(i);
  }

  for (size_t i = 0; i < maxRandomRegister_; i++) {
      if (!isRandomizedRegister(i)
              || i == maxVolatileRegister 
              || i == maxNonVolatileRegister) {
          continue;
      }

      // Choose a swap register that matches i in volatility and is 
      // also eligible for randomization.
      int swapRegister;
      if (Register::FromCode(i).volatile_()) {
          swapRegister = RNG::nextInt32(i, maxVolatileRegister);
          while (!Register::FromCode(swapRegister).volatile_() 
                  || !isRandomizedRegister(swapRegister)) {
              swapRegister = RNG::nextInt32(i, maxVolatileRegister);
          }
      } else {
          swapRegister = RNG::nextInt32(i, maxNonVolatileRegister);
          while (Register::FromCode(swapRegister).volatile_() 
                  || !isRandomizedRegister(swapRegister)) {
              swapRegister = RNG::nextInt32(i, maxNonVolatileRegister);
          }
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

/* static */ RegisterRandomizer
RegisterRandomizer::getInstance() {
    static bool initialized = false;
    static RegisterRandomizer instance;

    if (!initialized) {
        instance = RegisterRandomizer();
        initialized = true;
    }
    
    return instance;
}

/* static */ Register
RegisterRandomizer::randomize(const Register &physicalRegister) {
    RegisterRandomizer instance = getInstance();
    return instance.getRandomizedRegister(physicalRegister);
}

Register
RegisterRandomizer::getRandomizedRegister(const Register &physicalRegister) const {
    return getRandomizedRegister(physicalRegister.encoding());
}

Register
RegisterRandomizer::getRandomizedRegister(const Registers::Encoding &physicalRegister) const {
    return { substitutions_[physicalRegister] };
}

ValueOperand
RegisterRandomizer::getUnrandomizedValueOperand(const ValueOperand &v) const {
#if defined(JS_NUNBOX32)
    return ValueOperand(getUnrandomizedRegister(v.typeReg()),
                        getUnrandomizedRegister(v.payloadReg()));
#elif defined(JS_PUNBOX64)
    return ValueOperand(getUnrandomizedRegister(v.valueReg()));
#endif
}

Register 
RegisterRandomizer::getUnrandomizedRegister(const Register &r) const {
    return { reverseSubstitutions_[r.encoding()] };
}

Registers::SetType 
RegisterRandomizer::randomizeMask(Registers::SetType mask) const {
    Registers::SetType result = 0;
    uint32_t pos;
    while (mask) {
        pos = Registers::FirstBit(mask);
        result |= (1 << getRandomizedRegister(Registers::Encoding(pos)).encoding());
        mask ^=  (1 << pos);
    }
    return result;
}

/* static */ bool
RegisterRandomizer::isRandomizedRegister(unsigned int r) {
    return RANDOMIZATION_MASK & (1 << r);
}

} // namespace jit
} // namespace js
