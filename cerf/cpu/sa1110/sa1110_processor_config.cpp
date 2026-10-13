#include "../sa11xx/sa11xx_processor_config_base.h"

#include "../../core/cerf_emulator.h"
#include "../../boards/board_context.h"
#include "../../socs/sa11xx/sa1110_id.h"

namespace {

class Sa1110ProcessorConfig : public Sa11xxProcessorConfigBase {
public:
    using Sa11xxProcessorConfigBase::Sa11xxProcessorConfigBase;

    bool ShouldRegister() override {
        auto* bd = emu_.TryGet<BoardContext>();
        return bd && bd->GetSocId() == SocId::Sa1110;
    }

    /* SA-1110 Developer's Manual §5.2.1 (page 5-2): Architecture Version 01 = Version 4, Part
       Number B11 = SA1110, Stepping 1000 = B4 stepping. */
    uint32_t Midr() const override { return 0x6901B118u; }
    uint32_t Ctr()  const override { return 0x6901B118u; }
};

}  /* namespace */

REGISTER_SERVICE_AS(Sa1110ProcessorConfig, ArmProcessorConfig);
