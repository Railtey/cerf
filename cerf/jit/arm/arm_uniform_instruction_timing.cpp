#include "arm_instruction_timing.h"

#include "../../boards/board_context.h"
#include "../../core/cerf_emulator.h"

namespace {

class ArmUniformInstructionTiming : public ArmInstructionTiming {
public:
    using ArmInstructionTiming::ArmInstructionTiming;

    bool ShouldRegister() override {
        return emu_.Get<BoardContext>().GetCpuArch() == CpuArch::Arm;
    }

    uint16_t CycleCostFor(const DecodedInsn&) const override { return 1; }
};

}

REGISTER_SERVICE_AS_FALLBACK(ArmUniformInstructionTiming, ArmInstructionTiming);
