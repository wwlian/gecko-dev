/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef BASELINE_REGISTER_RANDOMIZATION
#include "jit/arm/SharedICRegisters.h"
#include "jit/RNG.h"

namespace js {
namespace jit {

/* static */ bool
initializeSharedICRegisterMapping() {
    if (Registers::SetSize(R1Mask) < 2)
        return false;

    Registers::SetType mask = R1Mask;
    uint32_t pos = selectRandomOneBitPosition(mask);
    Register R1PayloadReg = Register::FromCode(pos);

    mask ^= (1 << pos);
    pos = selectRandomOneBitPosition(mask);
    Register R1TypeReg = Register::FromCode(pos);

    R1 = ValueOperand(R1TypeReg, R1PayloadReg);
    return true;
}

} // namespace jit
} // namespace js
#endif  // BASELINE_REGISTER_RANDOMIZATION
