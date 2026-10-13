#include "../../socs/omap3530/omap3530_mcbsp_wiring.h"

#include "../../core/cerf_emulator.h"
#include "../../peripherals/ti_tps65950/tps65950_audio_interface.h"
#include "../board_context.h"
#include "omap_3530_evm_id.h"

namespace {

class OmapEvm3530McBspWiring : public Omap3530McBspWiring {
public:
    using Omap3530McBspWiring::Omap3530McBspWiring;

    bool ShouldRegister() override {
        return emu_.Get<BoardContext>().GetBoardId() == BoardId::Omap3530Evm;
    }

    Omap3530McBspPeer* Peer(uint32_t port) override {
        return port == 2u ? &emu_.Get<Tps65950AudioInterface>() : nullptr;
    }

    Omap3530McBspClockPad* ClksPad() override { return &emu_.Get<Tps65950AudioInterface>(); }
};

}

REGISTER_SERVICE_AS(OmapEvm3530McBspWiring, Omap3530McBspWiring);
