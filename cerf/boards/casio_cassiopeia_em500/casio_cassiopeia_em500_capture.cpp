#include "casio_cassiopeia_em500_capture.h"

#include "../../boards/board_context.h"
#include "../../core/cerf_emulator.h"
#include "../../cpu/emulated_memory.h"
#include "../../host/audio_activity_widget.h"
#include "../../jit/mips/mips_mmu.h"
#include "casio_cassiopeia_em500_id.h"

#include <vector>

namespace {

constexpr uint8_t  kNoCapturedSample = 0u;
constexpr size_t   kMicFifoBytes     = 65536u;
constexpr uint16_t kChannels         = 1u;
constexpr uint16_t kBitsPerSample    = 16u;

}

bool CasioCassiopeiaEm500Capture::ShouldRegister() {
    auto* bd = emu_.TryGet<BoardContext>();
    return bd && bd->GetBoardId() == BoardId::CasioCassiopeiaEm500;
}

void CasioCassiopeiaEm500Capture::OnReady() {
    mem_      = &emu_.Get<EmulatedMemory>();
    activity_ = &emu_.Get<AudioActivityWidget>();
    mic_.Start("Em500Capture", kMicFifoBytes);
}

void CasioCassiopeiaEm500Capture::OnShutdown() { mic_.Stop(); }

void CasioCassiopeiaEm500Capture::Request(uint32_t rate_hz) {
    mic_.Request(rate_hz, kChannels, kBitsPerSample);
}

void CasioCassiopeiaEm500Capture::Clear() { mic_.Clear(); }

void CasioCassiopeiaEm500Capture::WriteBlock(uint32_t va, uint32_t length) {
    const uint32_t       slots = length / kFrameBytes;
    std::vector<uint8_t> pcm(static_cast<size_t>(slots) * 2u);
    const bool           heard = mic_.Take(pcm.data(), pcm.size()) != 0u;
    std::vector<uint8_t> block(length, kNoCapturedSample);
    for (uint32_t i = 0; i < slots; ++i) {
        block[i * kFrameBytes]      = pcm[i * 2u];
        block[i * kFrameBytes + 1u] = pcm[i * 2u + 1u];
    }
    mem_->CopyIn(MipsSeg::UnmappedPa(va), block.data(), length);
    if (heard) activity_->MarkRx();
}

REGISTER_SERVICE(CasioCassiopeiaEm500Capture);
