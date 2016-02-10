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
    MConstant *secretConstant = MConstant::New(graph_->alloc(), Int32Value(secret));
    MBitXor* unblindOp = MBitXor::New(graph_->alloc(), c, secretConstant);
    c->blind(Int32Value(secret ^ c->value().toInt32()), unblindOp);

    // Uses of the now-blinded MConstant should be transferred to the unblinding op.
    c->justReplaceAllUsesWithExcept(unblindOp);

    // Add new instructions after the constant.
    c->block()->insertAfter(c, secretConstant);
    c->block()->insertAfter(secretConstant, unblindOp);
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
            && !def->isBitXor()
            && !def->isAdd()
            && !def->isSub()) {
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
    int32_t secret = rng_.blindingValue();
    int32_t o1 = c->value().toInt32() | secret;
    int32_t o2 = c->value().toInt32() | ~secret;
    c->blind(Int32Value(o1), consumer);
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(o2));
    MBitAnd* unblindOp = MBitAnd::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    consumer->block()->insertAfter(consumer->toInstruction(), unblindOperand);
    consumer->block()->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindBitOr(MConstant *c, MDefinition *consumer) {
    int32_t secret = rng_.blindingValue();
    int32_t o1 = c->value().toInt32() & secret;
    int32_t o2 = c->value().toInt32() & ~secret;
    c->blind(Int32Value(o1), consumer);
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(o2));
    MBitOr* unblindOp = MBitOr::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    consumer->block()->insertAfter(consumer->toInstruction(), unblindOperand);
    consumer->block()->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindBitXor(MConstant *c, MDefinition *consumer) {
    int32_t secret = rng_.blindingValue();
    int32_t o2 = c->value().toInt32() ^ secret;
    c->blind(Int32Value(secret), consumer);
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(o2));
    MBitXor* unblindOp = MBitXor::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    consumer->block()->insertAfter(consumer->toInstruction(), unblindOperand);
    consumer->block()->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindAddSub(MConstant *c, MBinaryArithInstruction *consumer) {
    int32_t constant = c->value().toInt32();
    int32_t secret;
    if (constant <= 0) {
        secret = rng_.blindingValue(constant, 0);
    } else {
        secret = rng_.blindingValue(0, constant);
    }
    int32_t o2 = constant - secret;
    c->blind(Int32Value(secret), consumer);
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(o2));
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
