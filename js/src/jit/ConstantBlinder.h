#ifdef CONSTANT_BLINDING
#ifndef jit_ConstantBlinder_h
#define jit_ConstantBlinder_h

namespace js {
namespace jit {

class ConstantBlinder final {
  private:
	RNG rng_;
	MIRGraph *graph_;
  public:
	ConstantBlinder(MIRGraph *graph) : graph_(graph) {}
	void blindConstants();

  private:
	void preComputationBlind(MBasicBlock *block, MConstant *c);
	bool areAllUsesAccumulatable(MDefinition *ins);
	void accumulationBlindAll(MBasicBlock *block, MConstant *c);
	void accumulationBitAndBlind(MBasicBlock *block, MConstant *c, MDefinition *consumer);
	void accumulationBitOrBlind(MBasicBlock *block, MConstant *c, MDefinition *consumer);
	void accumulationBitXorBlind(MBasicBlock *block, MConstant *c, MDefinition *consumer);
	void accumulationAddSubBlind(MBasicBlock *block, MConstant *c, MDefinition *consumer);
};

} /* namespace jit */
} /* namespace js */

#endif /* jit_ConstantBlinder_h */
#endif /* CONSTANT_BLINDING */
