/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jit_StackSlotAllocator_h
#define jit_StackSlotAllocator_h

#include "mozilla/unused.h"

#include "jit/Registers.h"
#ifdef ION_CALL_FRAME_RANDOMIZATION
#include "jit/RNG.h"
#endif

namespace js {
namespace jit {

class StackSlotAllocator
{
    js::Vector<uint32_t, 4, SystemAllocPolicy> normalSlots;
    js::Vector<uint32_t, 4, SystemAllocPolicy> doubleSlots;

    // Tracks the greatest slot height returned so far for an allocation request.
    uint32_t height_;

#ifdef ION_CALL_FRAME_RANDOMIZATION
    js::Vector<uint32_t, 4, SystemAllocPolicy> quadSlots;

    // Let minPoolSize be a multiple of 16 so that
    static constexpr uint32_t minPoolSize = 128;

    // Tracks the height of the entire pool of pre-allocated stack slots. height_ is no greater than this.
    uint32_t preallocHeight_;
#endif

    void addAvailableSlot(uint32_t index) {
        // Ignoring OOM here (and below) is fine; it just means the stack slot
        // will be unused.
        mozilla::Unused << normalSlots.append(index);
    }
    void addAvailableDoubleSlot(uint32_t index) {
        mozilla::Unused << doubleSlots.append(index);
    }

#ifdef ION_CALL_FRAME_RANDOMIZATION
    void addAvailableQuadSlot(uint32_t index) {
            mozilla::Unused << quadSlots.append(index);
    }

    /* Size of available pool for normal slot allocation. Not the same as
     * the size of normalSlots because double and quad slots contribute to the
     * normal slot pool.
     */
    uint32_t availableNormalSlots() {
        return normalSlots.length() + 2 * doubleSlots.length() + 4 * quadSlots.length();
    }

    /* Size of available pool for double slot allocation. Not the same as
     * the size of doubleSlots because quad slots contribute to the
     * double slot pool.
     */
    uint32_t availableDoubleSlots() {
        return doubleSlots.length() + 2 * quadSlots.length();
    }

    uint32_t availableQuadSlots() {
        return quadSlots.length();
    }

    void preallocQuadSlots(uint32_t n) {
        if (preallocHeight_ % 8) {
            addAvailableSlot(preallocHeight_ += 4);
        }
        if (preallocHeight_ % 16) {
            addAvailableSlot(preallocHeight_ += 8);
        }
        // Note that loop counter goes from [1, n]
        for (uint32_t i = 1; i <= n; i++) {
            addAvailableQuadSlot(preallocHeight_ + i * 16);
        }
        preallocHeight_ += 16 * n;
    }

    /* Prefer to preallocate as quads so that they can serve as candidate
     * slots for all types of allocation requests.
     */
    void preallocDoubleSlots(uint32_t n) {
        if (preallocHeight_ % 8) {
            addAvailableSlot(preallocHeight_ += 4);
        }

        /* If n is odd, preallocQuadSlots might not allocate enough space; however,
         * it might need to allocate an extra double slot first to align
         * preallocHeight_ to a 16-byte boundary. We test this after
         * preallocQuadSlots returns.
         */
        uint32_t oldPreallocHeight = preallocHeight_;
        preallocQuadSlots(n / 2);
        if (((preallocHeight_ - oldPreallocHeight) / 8) < n) {
            addAvailableSlot(preallocHeight_ += 8);
        }
    }

    /* Prefer to preallocate as quads/doubles so that they can serve as candidate
     * slots for multiple types of allocation requests.
     */
    void preallocNormalSlots(uint32_t n) {
        /* If n is odd, preallocDoubleSlots might not allocate enough space; however,
         * it might need to allocate an extra normal slot first to align
         * preallocHeight_ to an 8-byte boundary. We test this after
         * preallocDoubleSlots returns.
         */
        uint32_t oldPreallocHeight = preallocHeight_;
        preallocDoubleSlots(n / 2);
        if (((preallocHeight_ - oldPreallocHeight) / 4) < n) {
            addAvailableSlot(preallocHeight_ += 4);
        }
    }

    uint32_t allocateQuadSlot() {
        MOZ_ASSERT(SupportsSimd);
        uint32_t slotCount = availableQuadSlots();
        if (slotCount < minPoolSize) {
            preallocQuadSlots(minPoolSize - slotCount);
        }
        uint32_t result = extractSlot(RNG::nextInt32(0, availableQuadSlots() - 1), quadSlots);
        height_ = result > height_ ? result : height_;
        return result;
    }

    uint32_t allocateDoubleSlot() {
        uint32_t slotCount = availableDoubleSlots();
        if (slotCount < minPoolSize) {
            preallocDoubleSlots(minPoolSize - slotCount);
        }
        uint32_t slotIndex = RNG::nextInt32(0, availableDoubleSlots() - 1);

        uint32_t result;
        if (slotIndex < doubleSlots.length()) {
            result = extractSlot(slotIndex, doubleSlots);
        } else {
            // Split a quad slot.
            slotIndex -= doubleSlots.length();
            result = extractSlot(slotIndex / 2, quadSlots);
            if (slotIndex % 2) {
                addAvailableDoubleSlot(result);
                result -= 8;
            } else {
                addAvailableDoubleSlot(result - 8);
            }
        }
        height_ = result > height_ ? result : height_;
        return result;
    }

    uint32_t allocateSlot() {
        uint32_t slotCount = availableNormalSlots();
        if (slotCount < minPoolSize) {
            preallocNormalSlots(minPoolSize - slotCount);
        }
        uint32_t slotIndex = RNG::nextInt32(0, availableNormalSlots() - 1);

        uint32_t result;
        if (slotIndex < normalSlots.length()) {
            result = extractSlot(slotIndex, normalSlots);
        } else if (slotIndex - normalSlots.length() < 2 * doubleSlots.length()) {
            // Split a double slot.
            slotIndex -= normalSlots.length();
            result = extractSlot(slotIndex / 2, doubleSlots);
            if (slotIndex % 2) {
                addAvailableSlot(result);
                result -= 4;
            } else {
                addAvailableSlot(result - 4);
            }
        } else {
            // Split a quad slot.
            slotIndex -= (normalSlots.length() + 2 * doubleSlots.length());
            result = extractSlot(slotIndex / 4, quadSlots);
            switch (slotIndex % 4) {
                case 0:
                    addAvailableSlot(result - 4);
                    addAvailableDoubleSlot(result - 8);
                    break;
                case 1:
                    addAvailableSlot(result);
                    addAvailableDoubleSlot(result - 8);
                    result -= 4;
                    break;
                case 2:
                    addAvailableSlot(result - 12);
                    addAvailableDoubleSlot(result);
                    result -= 8;
                    break;
                case 3:
                    addAvailableSlot(result - 8);
                    addAvailableDoubleSlot(result);
                    result -= 12;
                    break;
            }
        }
        height_ = result > height_ ? result : height_;
        return result;
    }

    uint32_t extractSlot(uint32_t index, js::Vector<uint32_t, 4, SystemAllocPolicy>& slots) {
        uint32_t slotCount = slots.length();
        MOZ_ASSERT(index < slotCount);

        // Swap the selected slot into the end of the pool since we
        // can only remove by popping off the end.
        if (index < (slotCount - 1)) {
            uint32_t tmp = slots[index];
            slots[index] = slots.back();
            slots[slotCount - 1] = tmp;
        }
        return slots.popCopy();
    }
#else
    uint32_t allocateQuadSlot() {
        MOZ_ASSERT(SupportsSimd);
        // This relies on the fact that any architecture specific
        // alignment of the stack pointer is done a priori.
        if (height_ % 8 != 0)
            addAvailableSlot(height_ += 4);
        if (height_ % 16 != 0)
            addAvailableDoubleSlot(height_ += 8);
        return height_ += 16;
    }
    uint32_t allocateDoubleSlot() {
        if (!doubleSlots.empty())
            return doubleSlots.popCopy();
        if (height_ % 8 != 0)
            addAvailableSlot(height_ += 4);
        return height_ += 8;
    }
    uint32_t allocateSlot() {
        if (!normalSlots.empty())
            return normalSlots.popCopy();
        if (!doubleSlots.empty()) {
            uint32_t index = doubleSlots.popCopy();
            addAvailableSlot(index - 4);
            return index;
        }
        return height_ += 4;
    }
#endif

  public:
    StackSlotAllocator()
      : height_(0)
#ifdef ION_CALL_FRAME_RANDOMIZATION
      , preallocHeight_(0)
#endif
    { }

    static uint32_t width(LDefinition::Type type) {
        switch (type) {
#if JS_BITS_PER_WORD == 32
          case LDefinition::GENERAL:
          case LDefinition::OBJECT:
          case LDefinition::SLOTS:
#endif
          case LDefinition::INT32:
          case LDefinition::FLOAT32:   return 4;
#if JS_BITS_PER_WORD == 64
          case LDefinition::GENERAL:
          case LDefinition::OBJECT:
          case LDefinition::SLOTS:
#endif
#ifdef JS_PUNBOX64
          case LDefinition::BOX:
#endif
#ifdef JS_NUNBOX32
          case LDefinition::TYPE:
          case LDefinition::PAYLOAD:
#endif
          case LDefinition::DOUBLE:    return 8;
          case LDefinition::SINCOS:
          case LDefinition::FLOAT32X4:
          case LDefinition::INT32X4:   return 16;
        }
        MOZ_CRASH("Unknown slot type");
    }

    uint32_t allocateSlot(LDefinition::Type type) {
        switch (width(type)) {
          case 4:  return allocateSlot();
          case 8:  return allocateDoubleSlot();
          case 16: return allocateQuadSlot();
        }
        MOZ_CRASH("Unknown slot width");
    }

    uint32_t stackHeight() const {
        return height_;
    }
};

} // namespace jit
} // namespace js

#endif /* jit_StackSlotAllocator_h */
