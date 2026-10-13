#pragma once

#include "../../core/service.h"
#include "../guest_cycle_clock.h"

class ArmCoreClock : public Service {
public:
    using Service::Service;

    virtual GuestCycleClock::Rate ResetRate() const = 0;
};
