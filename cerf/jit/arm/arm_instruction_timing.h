#pragma once

#include <cstdint>

#include "../../core/service.h"

struct DecodedInsn;

class ArmInstructionTiming : public Service {
public:
    using Service::Service;

    virtual uint16_t CycleCostFor(const DecodedInsn& d) const = 0;
};
