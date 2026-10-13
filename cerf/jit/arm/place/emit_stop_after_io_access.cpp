#include <cstddef>
#include <cstdint>

#include "../arm_emit_services.h"
#include "../arm_interrupt_channel.h"
#include "../block_context.h"
#include "../cpu_state.h"
#include "../decoded_insn.h"
#include "../place_fns.h"
#include "../../x86_emit.h"
#include "../../x86_emit_alu.h"

uint8_t* EmitStopAfterIoAccess(uint8_t* cursor, DecodedInsn* d, BlockContext* ctx) {
    using namespace x86;
    EmitMovRegBaseDisp32(cursor, kEax, kStateReg,
        static_cast<int32_t>(offsetof(ArmCpuState, chain_exit_request)));
    EmitTestRegImm32(cursor, kEax, kChainExitClock | kChainExitIdle);
    uint8_t* running = EmitJzLabel32(cursor);
    EmitMovRegImm32(cursor, kEcx,
        static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(ctx->emit->InterruptChannel())));
    EmitMovRegImm32(cursor, kEdx, d->guest_address + d->length);
    EmitCall(cursor, reinterpret_cast<void*>(&ArmInterruptChannel::StopAfterInsnHelper));
    EmitRet(cursor);
    FixupLabel32(running, cursor);
    return cursor;
}
