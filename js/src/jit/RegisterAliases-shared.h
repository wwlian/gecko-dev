/*
 * RegisterAliases lifts the definitions of Assembler-specific Registers
 * out of the Assembler-*.h headers so that they can be used to define
 * diversified static values in SharedICRegisters.h without
 * having to include Assembler-*.h, which includes SharedICRegisters.h.
*/

#ifndef jit_RegisterAliases_shared_h
#define jit_RegisterAliases_shared_h

#if defined(JS_CODEGEN_X86)
#elif defined(JS_CODEGEN_X64)
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/RegisterAliases-arm.h"
#elif defined(JS_CODEGEN_ARM64)
#elif defined(JS_CODEGEN_MIPS32)
#elif defined(JS_CODEGEN_MIPS64)
#elif defined(JS_CODEGEN_NONE)
#else
# error "Unknown architecture!"
#endif

namespace js {
namespace jit {

} // namespace jit
} // namespace js

#endif /* jit_RegisterAliases_shared_h */
