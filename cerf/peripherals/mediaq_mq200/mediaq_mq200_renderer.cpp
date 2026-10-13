#define NOMINMAX

#include "mediaq_mq200.h"

#include "../mediaq_ge/mediaq_panel_renderer.h"
#include "../../boards/board_context.h"
#include "../../boards/siemens_simpad/siemens_simpad_family_id.h"
#include "../../boards/pangolin/pangolin_family_id.h"

namespace {

class MediaQMq200Renderer : public MediaQPanelRenderer<MediaQMq200> {
public:
    using MediaQPanelRenderer<MediaQMq200>::MediaQPanelRenderer;

    bool ShouldRegister() override {
        auto* bd = emu_.TryGet<BoardContext>();
        if (!bd) return false;
        const std::string_view b = bd->GetBoardFamilyId();
        return b == BoardFamilyId::SiemensSimpad || b == BoardFamilyId::Pangolin;
    }
};

}

REGISTER_SERVICE_AS(MediaQMq200Renderer, PanelFrameRenderer);
