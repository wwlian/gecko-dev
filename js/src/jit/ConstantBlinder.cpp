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
                    accumulationBlindAll(*block, ins->toConstant());
                } else {
                    preComputationBlind(*block, ins->toConstant());
                }
            }
        }
    }
}

void
ConstantBlinder::preComputationBlind(MBasicBlock *block, MConstant *c) {
    JitSpew(JitSpew_IonMIR, "Precomputation blinding  int32 %d", c->value().toInt32());
    int32_t secret = rng_.blindingValue();
    MConstant *secretConstant = MConstant::New(graph_->alloc(), Int32Value(secret));
    MBitXor* unblindOp = MBitXor::New(graph_->alloc(), c, secretConstant);
    c->blind(Int32Value(secret ^ c->value().toInt32()), unblindOp);

    // Uses of the now-blinded MConstant should be transferred to the unblinding op.
    c->justReplaceAllUsesWithExcept(unblindOp);

    // Add new instructions to the block.
    block->insertAfter(c, secretConstant);
    block->insertAfter(secretConstant, unblindOp);
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
ConstantBlinder::accumulationBlindAll(MBasicBlock *block, MConstant *c) {
    JitSpew(JitSpew_IonMIR, "Accumulation blinding  int32 %d", c->value().toInt32());
    for (MUseIterator i(c->usesBegin()), e(c->usesEnd()); i != e; ++i) {
        MDefinition* consumer = i->consumer()->toDefinition();
        if (consumer->isBitAnd()) {
            accumulationBlindBitAnd(block, c, consumer);
        } else if (consumer->isBitOr()) {
            accumulationBlindBitOr(block, c, consumer);
        } else if (consumer->isBitXor()) {
            accumulationBlindBitXor(block, c, consumer);
        } else if (consumer->isAdd() || consumer->isSub()) {
            accumulationBlindAddSub(block, c, consumer);
        }
    }
}

void
ConstantBlinder::accumulationBlindBitAnd(MBasicBlock *block, MConstant *c, MDefinition *consumer) {
    uint32_t secret = static_cast<uint32_t>(rng_.blindingValue());
    c->blind(Int32Value(c->value().toInt32() | secret), consumer);
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(c->value().toInt32() | ~secret));
    MBitAnd* unblindOp = MBitAnd::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    block->insertAfter(consumer->toInstruction(), unblindOperand);
    block->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindBitOr(MBasicBlock *block, MConstant *c, MDefinition *consumer) {
    uint32_t secret = static_cast<uint32_t>(rng_.blindingValue());
    c->blind(Int32Value(c->value().toInt32() & secret), consumer);
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(c->value().toInt32() & ~secret));
    MBitOr* unblindOp = MBitOr::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    block->insertAfter(consumer->toInstruction(), unblindOperand);
    block->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindBitXor(MBasicBlock *block, MConstant *c, MDefinition *consumer) {
    uint32_t secret = static_cast<uint32_t>(rng_.blindingValue());
    c->blind(Int32Value(c->value().toInt32() ^ secret), consumer);
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(secret));
    MBitXor* unblindOp = MBitXor::New(graph_->alloc(), consumer, unblindOperand);

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    block->insertAfter(consumer->toInstruction(), unblindOperand);
    block->insertAfter(unblindOperand, unblindOp);
}

void
ConstantBlinder::accumulationBlindAddSub(MBasicBlock *block, MConstant *c, MDefinition *consumer) {
    int32_t constant = c->value().toInt32();
    int32_t secret;
    if (constant <= 0) {
        secret = rng_.blindingValue(constant, 0);
    } else {
        secret = rng_.blindingValue(0, constant);
    }
    c->blind(Int32Value(constant - secret), consumer);
    MConstant *unblindOperand = MConstant::New(graph_->alloc(), Int32Value(secret));
    MInstruction* unblindOp;
    if (consumer->isAdd()) {
        unblindOp = MAdd::New(graph_->alloc(), consumer, unblindOperand);
    } else {
        unblindOp = MSub::New(graph_->alloc(), consumer, unblindOperand);
    }

    consumer->justReplaceAllUsesWithExcept(unblindOp);
    block->insertAfter(consumer->toInstruction(), unblindOperand);
    block->insertAfter(unblindOperand, unblindOp);
}

} /* namespace jit */
} /* namespace js */
#endif /* CONSTANT_BLINDING */
