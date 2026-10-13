#include "freescale_sdma_guest_memory.h"

#include "../boards/board_context.h"
#include "../core/cerf_emulator.h"
#include "../cpu/emulated_memory.h"
#include "imx31/imx31_id.h"
#include "imx51/imx51_id.h"

#include <cstring>

REGISTER_SERVICE(FreescaleSdmaGuestMemory);

bool FreescaleSdmaGuestMemory::ShouldRegister() {
    auto* bd = emu_.TryGet<BoardContext>();
    return bd && (bd->GetSocId() == SocId::Imx31 || bd->GetSocId() == SocId::Imx51);
}

uint32_t FreescaleSdmaGuestMemory::Read(uint32_t pa, uint8_t* dst, uint32_t n) {
    if (n == 0u) return 0u;
    auto& mem = emu_.Get<EmulatedMemory>();
    if (const uint8_t* span = mem.TryTranslateRange(pa, n)) {
        std::memcpy(dst, span, n);
        return n;
    }
    uint32_t k = 0;
    for (; k < n; ++k) {
        const uint8_t* byte = mem.TryTranslate(pa + k);
        if (byte == nullptr) break;
        dst[k] = *byte;
    }
    return k;
}

uint32_t FreescaleSdmaGuestMemory::Write(uint32_t pa, const uint8_t* src, uint32_t n) {
    if (n == 0u) return 0u;
    auto& mem = emu_.Get<EmulatedMemory>();
    if (uint8_t* span = mem.TryTranslateRange(pa, n, true)) {
        std::memcpy(span, src, n);
        return n;
    }
    uint32_t k = 0;
    for (; k < n; ++k) {
        uint8_t* byte = mem.TryTranslateWrite(pa + k);
        if (byte == nullptr) break;
        *byte = src[k];
    }
    return k;
}

uint32_t FreescaleSdmaGuestMemory::Writable(uint32_t pa, uint32_t n) {
    if (n == 0u) return 0u;
    auto& mem = emu_.Get<EmulatedMemory>();
    if (mem.TryTranslateRange(pa, n, true) != nullptr) return n;
    uint32_t k = 0;
    while (k < n && mem.TryTranslateWrite(pa + k) != nullptr) ++k;
    return k;
}
