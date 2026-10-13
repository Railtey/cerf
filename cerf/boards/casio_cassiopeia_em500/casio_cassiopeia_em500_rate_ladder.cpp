#include "casio_cassiopeia_em500_rate_ladder.h"

#include "../../boards/board_context.h"
#include "../../core/cerf_emulator.h"
#include "../../core/fatal.h"
#include "casio_cassiopeia_em500_id.h"

bool CasioCassiopeiaEm500RateLadder::ShouldRegister() {
    auto* bd = emu_.TryGet<BoardContext>();
    return bd && bd->GetBoardId() == BoardId::CasioCassiopeiaEm500;
}

/* loc_F62984 @0xF6298C-@0xF629C0: li $v1 select + li $a1 doubler pairs skipped by
   btnez on slti 0x2711 / 0x3A99 / 0x4A39 / 0x6591 and slt 0x9C41, at @0xF62990 /
   @0xF6299A / @0xF629A4 / @0xF629AE / @0xF629B8. */
bool CasioCassiopeiaEm500RateLadder::Lookup(uint32_t select, bool doubler, uint32_t& hz) const {
    if (!doubler) {
        switch (select) {
            case 0x40u: hz = 8000u;  return true;
            case 0x10u: hz = 11025u; return true;
            case 0x30u: hz = 16000u; return true;
            case 0x00u: hz = 22050u; return true;
            default: return false;
        }
    }
    switch (select) {
        case 0x30u: hz = 32000u; return true;
        case 0x00u: hz = 44100u; return true;
        default: return false;
    }
}

uint32_t CasioCassiopeiaEm500RateLadder::RequireRate(uint32_t select, bool doubler) const {
    uint32_t hz = 0;
    if (Lookup(select, doubler, hz)) return hz;
    emu_.Get<Fatal>().Die("EM-500 audio 0x0880 rate select 0x%02X with doubler %u is not a "
                          "loc_F62984 ladder row", select, doubler ? 1u : 0u);
}

REGISTER_SERVICE(CasioCassiopeiaEm500RateLadder);
