#ifndef jit_RegisterRandomizer_shared_h
#define jit_RegisterRandomizer_shared_h

#if defined(JS_CODEGEN_X86)
#elif defined(JS_CODEGEN_X64)
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/RegisterRandomizer-arm.h"
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

#endif /* jit_RegisterRandomizer_shared_h */
