#include "casio_cassiopeia_em500_speaker.h"

#include "../../boards/board_context.h"
#include "../../core/cerf_emulator.h"
#include "../../cpu/emulated_memory.h"
#include "../../host/audio_activity_widget.h"
#include "../../jit/mips/mips_mmu.h"
#include "casio_cassiopeia_em500_id.h"

#include <vector>

bool CasioCassiopeiaEm500Speaker::ShouldRegister() {
    auto* bd = emu_.TryGet<BoardContext>();
    return bd && bd->GetBoardId() == BoardId::CasioCassiopeiaEm500;
}

void CasioCassiopeiaEm500Speaker::OnReady() {
    mem_      = &emu_.Get<EmulatedMemory>();
    activity_ = &emu_.Get<AudioActivityWidget>();
    paced_.Start("Em500Audio", 0, 0, 0, true);
    activity_->NotePresent();
}

void CasioCassiopeiaEm500Speaker::OnShutdown() { paced_.Stop(); }

void CasioCassiopeiaEm500Speaker::SetFormat(uint32_t hz, uint16_t channels, uint16_t bits) {
    paced_.SetFormat(hz, channels, bits);
}

void CasioCassiopeiaEm500Speaker::Begin() { paced_.BeginAudioOut({}); }

void CasioCassiopeiaEm500Speaker::Finish() { paced_.FinishAudioOut(); }

void CasioCassiopeiaEm500Speaker::Abort() { paced_.StopAudioOut(); }

void CasioCassiopeiaEm500Speaker::QueueBlock(uint32_t va, uint32_t length) {
    std::vector<uint8_t> block(length);
    /* VR4102 UM ch.5 p131 "(3) kseg1": references are not mapped through TLB and the
       physical address is the virtual address minus 0xA0000000. */
    mem_->CopyOut(MipsSeg::UnmappedPa(va), block.data(), length);
    activity_->MarkTx();
    paced_.QueueOutputInHostBlocks(block.data(), length);
}

REGISTER_SERVICE(CasioCassiopeiaEm500Speaker);
