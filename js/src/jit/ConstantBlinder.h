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
	void blindConstants() final;

  private:
	void preComputationBlind(MBasicBlock *block, MConstant *c) final;
	bool areAllUsesAccumulatable(MDefinition *ins) final;
	void accumulationBlindAll(MBasicBlock *block, MConstant *c) final;
	void accumulationBitAndBlind(MBasicBlock *block, MConstant *c, MDefinition *consumer) final;
	void accumulationBitOrBlind(MBasicBlock *block, MConstant *c, MDefinition *consumer) final;
	void accumulationBitXorBlind(MBasicBlock *block, MConstant *c, MDefinition *consumer) final;
	void accumulationAddSubBlind(MBasicBlock *block, MConstant *c, MDefinition *consumer) final;
};

} /* namespace jit */
} /* namespace js */

#endif /* jit_ConstantBlinder_h */
#endif /* CONSTANT_BLINDING /*
