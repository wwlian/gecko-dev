#ifdef ION_CONSTANT_BLINDING
#ifndef jit_ConstantBlinder_h
#define jit_ConstantBlinder_h

namespace js {
namespace jit {

class ConstantBlinder final {
  private:
    MIRGraph *graph_;
  public:
    ConstantBlinder(MIRGraph *graph) : graph_(graph) {}

	void blindConstants();

  private:
	bool isConsumerAccumulationBlindable(MNode* def);
	void blindConstant(MConstant* c);
	void preComputationBlindAll(MConstant *c);
	void preComputationBlind(MConstant *c, MUse* currentUse);
	void accumulationBlind(MConstant *c, MUse* currentUse);
	void accumulationBlindBitAnd(MConstant *c, MDefinition *consumer);
	void accumulationBlindBitOr(MConstant *c, MDefinition *consumer);
	void accumulationBlindBitXor(MConstant *c, MDefinition *consumer);
	void accumulationBlindAddSub(MConstant *c, MBinaryArithInstruction *consumer);
};

} /* namespace jit */
} /* namespace js */

#endif /* jit_ConstantBlinder_h */
#endif /* ION_CONSTANT_BLINDING */
