#pragma once

#include "../core/service.h"

#include <cstdint>

class FreescaleSdmaGuestMemory : public Service {
public:
    using Service::Service;

    bool ShouldRegister() override;

    uint32_t Read(uint32_t pa, uint8_t* dst, uint32_t n);
    uint32_t Write(uint32_t pa, const uint8_t* src, uint32_t n);
    uint32_t Writable(uint32_t pa, uint32_t n);
};
