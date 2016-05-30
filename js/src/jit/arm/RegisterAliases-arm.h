#ifndef jit_arm_RegisterAliases_arm_h
#define jit_arm_RegisterAliases_arm_h

#include "jit/arm/Architecture-arm.h"
#include "jit/arm/RegisterRandomizer-arm.h"

namespace js {
namespace jit {

// NOTE: there are duplicates in this list! Sometimes we want to specifically
// refer to the link register as a link register (bl lr is much clearer than bl
// r14). HOWEVER, this register can easily be a gpr when it is not busy holding
// the return address.
#ifdef BASELINE_REGISTER_RANDOMIZATION
static RegisterRandomizer randomizer = RegisterRandomizer::getInstance();
static const Register r0  = randomizer.getRandomizedRegister(Registers::r0);
static const Register r1  = randomizer.getRandomizedRegister(Registers::r1);
static const Register r2  = randomizer.getRandomizedRegister(Registers::r2);
static const Register r3  = randomizer.getRandomizedRegister(Registers::r3);
static const Register r4  = randomizer.getRandomizedRegister(Registers::r4);
static const Register r5  = randomizer.getRandomizedRegister(Registers::r5);
static const Register r6  = randomizer.getRandomizedRegister(Registers::r6);
static const Register r7  = randomizer.getRandomizedRegister(Registers::r7);
static const Register r8  = randomizer.getRandomizedRegister(Registers::r8);
static const Register r9  = randomizer.getRandomizedRegister(Registers::r9);
static const Register r10 = randomizer.getRandomizedRegister(Registers::r10);
static const Register r11 = randomizer.getRandomizedRegister(Registers::r11);
static const Register r12 = randomizer.getRandomizedRegister(Registers::ip);
static const Register ip  = randomizer.getRandomizedRegister(Registers::ip);
static const Register sp  = randomizer.getRandomizedRegister(Registers::sp);
static const Register r14 = randomizer.getRandomizedRegister(Registers::lr);
static const Register lr  = randomizer.getRandomizedRegister(Registers::lr);
static const Register pc  = randomizer.getRandomizedRegister(Registers::pc);
#else
static MOZ_CONSTEXPR_VAR Register r0  = { Registers::r0 };
static MOZ_CONSTEXPR_VAR Register r1  = { Registers::r1 };
static MOZ_CONSTEXPR_VAR Register r2  = { Registers::r2 };
static MOZ_CONSTEXPR_VAR Register r3  = { Registers::r3 };
static MOZ_CONSTEXPR_VAR Register r4  = { Registers::r4 };
static MOZ_CONSTEXPR_VAR Register r5  = { Registers::r5 };
static MOZ_CONSTEXPR_VAR Register r6  = { Registers::r6 };
static MOZ_CONSTEXPR_VAR Register r7  = { Registers::r7 };
static MOZ_CONSTEXPR_VAR Register r8  = { Registers::r8 };
static MOZ_CONSTEXPR_VAR Register r9  = { Registers::r9 };
static MOZ_CONSTEXPR_VAR Register r10 = { Registers::r10 };
static MOZ_CONSTEXPR_VAR Register r11 = { Registers::r11 };
static MOZ_CONSTEXPR_VAR Register r12 = { Registers::ip };
static MOZ_CONSTEXPR_VAR Register ip  = { Registers::ip };
static MOZ_CONSTEXPR_VAR Register sp  = { Registers::sp };
static MOZ_CONSTEXPR_VAR Register r14 = { Registers::lr };
static MOZ_CONSTEXPR_VAR Register lr  = { Registers::lr };
static MOZ_CONSTEXPR_VAR Register pc  = { Registers::pc };
#endif

static const Register ScratchRegister = ip;

// Holds a pointer to the frame that OSRed into an OSR entry point, if applicable.
// In most JIT code, this refers to the randomized register. In the EnterJIT
// stub, this refers to the physical register Registers::r3, since the value
// is passed to the stub via a native C++ call.
static const Register OsrFrameReg = r3;
// Used to pass an argument into the ArgumentsRectifier stub.
static const Register ArgumentsRectifierReg = r8;

// It is important that CallTempReg[0-3] do not reference a volatile virtual register,
// which could be randomized to point to physical register r0/ReturnReg. There is code that
// assumes that CallTempReg[0-3] (or js::jit::Registers that alias them) are distinct from ReturnReg.
static const Register CallTempReg0 = r5;
static const Register CallTempReg1 = r6;
static const Register CallTempReg2 = r7;
static const Register CallTempReg3 = r8;
static const Register CallTempReg4 = r0;
static const Register CallTempReg5 = r1;

// IntArgReg[0-9] refer to the physical registers into which the architecture
// ABI requires integer arguments be placed, in order. To avoid having to use
// RegisterRandomizer::getUnrandomizedRegister everywhere we reference 
// IntArgReg[0-9], we do not randomize these values.
static MOZ_CONSTEXPR_VAR Register IntArgReg0 = { Registers::r0 };
static MOZ_CONSTEXPR_VAR Register IntArgReg1 = { Registers::r1 };
static MOZ_CONSTEXPR_VAR Register IntArgReg2 = { Registers::r2 };
static MOZ_CONSTEXPR_VAR Register IntArgReg3 = { Registers::r3 };
static const Register GlobalReg = r10;
static const Register HeapReg = r11;
static const Register CallTempNonArgRegs[] = { r5, r6, r7, r8 };
static const uint32_t NumCallTempNonArgRegs =
    mozilla::ArrayLength(CallTempNonArgRegs);

static MOZ_CONSTEXPR_VAR Register ReturnReg = { Registers::r0 };
static MOZ_CONSTEXPR_VAR Register InvalidReg = { Registers::invalid_reg };

static MOZ_CONSTEXPR_VAR FloatRegister d0  = {FloatRegisters::d0, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d1  = {FloatRegisters::d1, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d2  = {FloatRegisters::d2, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d3  = {FloatRegisters::d3, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d4  = {FloatRegisters::d4, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d5  = {FloatRegisters::d5, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d6  = {FloatRegisters::d6, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d7  = {FloatRegisters::d7, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d8  = {FloatRegisters::d8, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d9  = {FloatRegisters::d9, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d10 = {FloatRegisters::d10, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d11 = {FloatRegisters::d11, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d12 = {FloatRegisters::d12, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d13 = {FloatRegisters::d13, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d14 = {FloatRegisters::d14, VFPRegister::Double};
static MOZ_CONSTEXPR_VAR FloatRegister d15 = {FloatRegisters::d15, VFPRegister::Double};


} // namespace jit
} // namespace js

#endif /* jit_arm_RegisterAliases_arm_h */
