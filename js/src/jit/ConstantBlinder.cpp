#ifdef CONSTANT_BLINDING

#include "jit/ConstantBlinder.h"

#include "jit/JitSpewer.h"

namespace js {
namespace jit {

void
ConstantBlinder::blindConstants() {
    for (ReversePostorderIterator block(graph_->rpoBegin()); block != graph_->rpoEnd(); block++) {
        for (MInstructionIterator ins = block->begin(); *ins != block->lastIns(); ins++) {
            if (ins->isConstant() && ins->toConstant()->value().isInt32() && ins->toConstant()->isUntrusted()) {
                if (areAllUsesAccumulatable(*ins)) {
                    accumulationBlindAll(ins->toConstant());
                } else {
                    preComputationBlind(ins->toConstant());
                }
            }
        }
    }
}

void
ConstantBlinder::preComputationBlind(MConstant *c) {
    JitSpew(JitSpew_IonMIR, "Precomputation blinding  int32 %d", c->value().toInt32());
    int32_t secret = rng_.blindingValue();
    int32_t unblindOpInt = secret ^ c->value().toInt32();
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(unblindOpInt));
    MBitXor* unblindOp = MBitXor::New(graph_->alloc(), c, unblindOperand);
    c->blindBitXor(graph_->alloc(), secret, Int32Value(secret));

    // Uses of the now-blinded MConstant should be transferred to the unblinding op.
    c->justReplaceAllUsesWithExcept(unblindOp);

    // Add new instructions after the constant.
    c->block()->insertAfter(c, unblindOperand);
    c->block()->insertAfter(unblindOperand, unblindOp);
}

bool
ConstantBlinder::areAllUsesAccumulatable(MDefinition *ins) {
    MOZ_ASSERT(ins != nullptr);

    for (MUseIterator i(ins->usesBegin()), e(ins->usesEnd()); i != e; ++i) {
        MNode* consumer = i->consumer();
        if (!consumer->isDefinition()) return false;
        MDefinition *def = consumer->toDefinition();
        if (!def->isBitAnd()
            && !def->isBitOr()
            && !def->isBitXor()) { /*
            && !def->isAdd()
            && !def->isSub()) { */
            return false;
        }
    }
    return true;
}

void
ConstantBlinder::accumulationBlindAll(MConstant *c) {
    JitSpew(JitSpew_IonMIR, "Accumulation blinding int32 %d", c->value().toInt32());
    for (MUseIterator i(c->usesBegin()), e(c->usesEnd()); i != e; ++i) {
        MDefinition* consumer = i->consumer()->toDefinition();
        if (consumer->isBitAnd()) {
            accumulationBlindBitAnd(c, consumer);
        } else if (consumer->isBitOr()) {
            accumulationBlindBitOr(c, consumer);
        } else if (consumer->isBitXor()) {
            accumulationBlindBitXor(c, consumer);
        } else if (consumer->isAdd() || consumer->isSub()) {
            accumulationBlindAddSub(c, static_cast<MBinaryArithInstruction*>(consumer));
        }
    }
}

void
ConstantBlinder::accumulationBlindBitAnd(MConstant *c, MDefinition *consumer) {
    int32_t unblindOpInt;
    if (c->isBitAndBlinded()) {
        unblindOpInt = c->unblindedValue().toInt32() | ~c->bitAndBlindedVariant()->secret();
    } else {
        int32_t secret = rng_.blindingValue();
        int32_t o1 = c->value().toInt32() | secret;
        unblindOpInt = c->value().toInt32() | ~secret;
        c->blindBitAnd(graph_->alloc(), secret, Int32Value(o1));

        if (c != c->bitAndBlindedVariant()) {
            consumer->block()->insertBefore(consumer->toInstruction(), c->bitAndBlindedVariant());
        }
    }
    // Redirect one or both of the consumer's operand slots if necessary.
    if (c != c->bitAndBlindedVariant()) {
        if (consumer->getOperand(0) == static_cast<MDefinition*>(c)) {
            consumer->replaceOperand(0, c->bitAndBlindedVariant());
        }
        if (consumer->getOperand(1) == static_cast<MDefinition*>(c)) {
            consumer->replaceOperand(1, c->bitAndBlindedVariant());
        }
    }
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(unblindOpInt));
    MBinaryBitwiseInstruction* unblindOp = MBitAnd::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    consumer->block()->insertAfter(consumer->toInstruction(), unblindOperand);
    consumer->block()->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindBitOr(MConstant *c, MDefinition *consumer) {
    int32_t unblindOpInt;
    if (c->isBitOrBlinded()) {
        unblindOpInt = c->unblindedValue().toInt32() & ~c->bitOrBlindedVariant()->secret();
    } else {
        int32_t secret = rng_.blindingValue();
        int32_t o1 = c->value().toInt32() & secret;
        unblindOpInt = c->value().toInt32() & ~secret;
        c->blindBitOr(graph_->alloc(), secret, Int32Value(o1));

        if (c != c->bitOrBlindedVariant()) {
            consumer->block()->insertBefore(consumer->toInstruction(), c->bitOrBlindedVariant());
        }
    }
    // Redirect one or both of the consumer's operand slots if necessary.
    if (c != c->bitOrBlindedVariant()) {
        if (consumer->getOperand(0) == static_cast<MDefinition*>(c)) {
            consumer->replaceOperand(0, c->bitOrBlindedVariant());
        }
        if (consumer->getOperand(1) == static_cast<MDefinition*>(c)) {
            consumer->replaceOperand(1, c->bitOrBlindedVariant());
        }
    }
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(unblindOpInt));
    MBinaryBitwiseInstruction* unblindOp = MBitOr::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    consumer->block()->insertAfter(consumer->toInstruction(), unblindOperand);
    consumer->block()->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindBitXor(MConstant *c, MDefinition *consumer) {
    int32_t unblindOpInt;
    if (c->isBitXorBlinded()) {
        unblindOpInt = c->unblindedValue().toInt32() ^ c->bitXorBlindedVariant()->secret();
    } else {
        int32_t secret = rng_.blindingValue();
        unblindOpInt = c->value().toInt32() ^ secret;
        c->blindBitXor(graph_->alloc(), secret, Int32Value(secret));

        if (c != c->bitXorBlindedVariant()) {
            consumer->block()->insertBefore(consumer->toInstruction(), c->bitXorBlindedVariant());
        }
    }
    // Redirect one or both of the consumer's operand slots if necessary.
    if (c != c->bitXorBlindedVariant()) {
        if (consumer->getOperand(0) == static_cast<MDefinition*>(c)) {
            consumer->replaceOperand(0, c->bitXorBlindedVariant());
        }
        if (consumer->getOperand(1) == static_cast<MDefinition*>(c)) {
            consumer->replaceOperand(1, c->bitXorBlindedVariant());
        }
    }
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(unblindOpInt));
    MBinaryBitwiseInstruction* unblindOp = MBitXor::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    consumer->block()->insertAfter(consumer->toInstruction(), unblindOperand);
    consumer->block()->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindAddSub(MConstant *c, MBinaryArithInstruction *consumer) {
    int32_t unblindOpInt;
    if (c->isAddSubBlinded()) {
        unblindOpInt = c->unblindedValue().toInt32() - c->addSubBlindedVariant()->secret();
    } else {
        int32_t constant = c->value().toInt32();
        int32_t secret;
        if (constant <= 0) {
            secret = rng_.blindingValue(constant, 0);
        } else {
            secret = rng_.blindingValue(0, constant);
        }
        JitSpew(JitSpew_IonMIR, "Constant %d Secret %d", c->value().toInt32(), secret);
        unblindOpInt = constant - secret;
        c->blindAddSub(graph_->alloc(), secret, Int32Value(secret));

        if (c != c->addSubBlindedVariant()) {
            consumer->block()->insertBefore(consumer->toInstruction(), c->addSubBlindedVariant());
        }
    }
    // Redirect one or both of the consumer's operand slots if necessary.
    if (c != c->addSubBlindedVariant()) {
        if (consumer->getOperand(0) == static_cast<MDefinition*>(c)) {
            consumer->replaceOperand(0, c->addSubBlindedVariant());
        }
        if (consumer->getOperand(1) == static_cast<MDefinition*>(c)) {
            consumer->replaceOperand(1, c->addSubBlindedVariant());
        }
    }
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(unblindOpInt));
    MBinaryArithInstruction* unblindOp;
    if (consumer->isAdd()) {
        unblindOp = MAdd::New(graph_->alloc(), consumer, unblindOperand);
    } else {
        unblindOp = MSub::New(graph_->alloc(), consumer, unblindOperand);
    }
    unblindOp->setSpecialization(consumer->specialization());

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    consumer->block()->insertAfter(consumer, unblindOperand);
    consumer->block()->insertAfter(unblindOperand, unblindOp);
}

} /* namespace jit */
} /* namespace js */
#endif /* CONSTANT_BLINDING */
