#define NOMINMAX

#include "mediaq_mq200.h"

#include "mediaq_mq200_config.h"
#include "../mediaq_ge/mediaq_panel_renderer.h"
#include "../../core/byte_order.h"

#include <algorithm>

namespace {

class MediaQMq200Renderer : public MediaQPanelRenderer<MediaQMq200> {
public:
    using MediaQPanelRenderer<MediaQMq200>::MediaQPanelRenderer;

    bool ShouldRegister() override {
        return emu_.TryGet<MediaQMq200Config>() != nullptr;
    }

    void RenderInto(uint32_t* dib, uint32_t host_w, uint32_t host_h) override {
        MediaQPanelRenderer<MediaQMq200>::RenderInto(dib, host_w, host_h);
        const MediaQMq200::HwCursor c = chip_->PanelCursor();
        if (c.shown) DrawCursor(c, dib, host_w, host_h);
    }

private:
    static constexpr uint32_t kCursorSide     = 64u;
    static constexpr uint32_t kCursorRowBytes = 16u;

    /* MQ-200 Data Book Table 3-16: AND/XOR 00 background, 01 foreground, 10 transparent,
       11 inverse; X0 / A0 sit in bits 16 / 0 of each 16-pixel word. */
    void DrawCursor(const MediaQMq200::HwCursor& c, uint32_t* dib, uint32_t host_w,
                    uint32_t host_h) const {
        MediaQMq200&   mq  = *chip_;
        const uint32_t w   = std::min(host_w, mq.GetGuestW());
        const uint32_t h   = std::min(host_h, mq.GetGuestH());
        const uint8_t* img = mq.FbBytes() + c.image;
        for (uint32_t cy = c.y_off; cy < kCursorSide; ++cy) {
            const uint32_t sy = c.y + (cy - c.y_off);
            if (sy >= h) break;
            for (uint32_t cx = c.x_off; cx < kCursorSide; ++cx) {
                const uint32_t sx = c.x + (cx - c.x_off);
                if (sx >= w) break;
                const uint32_t word = cerf::le::U32(img, cy * kCursorRowBytes + (cx / 16u) * 4u);
                const uint32_t and_bit = (word >> (cx % 16u)) & 1u;
                const uint32_t xor_bit = (word >> (16u + cx % 16u)) & 1u;
                uint32_t& px = dib[static_cast<size_t>(sy) * host_w + sx];
                if (and_bit && !xor_bit) continue;
                if (and_bit) {
                    px ^= 0x00FFFFFFu;
                    continue;
                }
                px = PalRgb(xor_bit ? c.fg : c.bg);
            }
        }
    }
};

}

REGISTER_SERVICE_AS(MediaQMq200Renderer, PanelFrameRenderer);
