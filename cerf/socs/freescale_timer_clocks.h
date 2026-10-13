#pragma once

#include "../core/service.h"
#include "../jit/guest_cycle_clock.h"

#include <cstdint>
#include <functional>

enum class FreescaleTimerUnit : uint8_t { kEpit1, kEpit2, kGpt };

enum class FreescaleTimerInput : uint8_t { kIpg, kHighfreq, kLowfreq };

enum class FreescaleLowPowerMode : uint8_t { kRun, kWait, kDoze, kStop, kStateRetention };

class FreescaleTimerClocks : public Service {
public:
    using Service::Service;

    virtual GuestCycleClock::Rate InputRate(FreescaleTimerUnit unit,
                                            FreescaleTimerInput input) const = 0;

    virtual bool InputRunsIn(FreescaleTimerUnit unit, FreescaleTimerInput input,
                             FreescaleLowPowerMode mode) const = 0;

    virtual FreescaleLowPowerMode WfiMode() const = 0;

    virtual void RegisterRateListener(std::function<void()> fn) = 0;

    static bool PrescaledRate(GuestCycleClock::Rate input, uint32_t prescaler,
                              GuestCycleClock::Rate& tick) {
        const uint64_t divide = uint64_t{prescaler} + 1u;
        if (!input.ScaleFits(1u, divide)) return false;
        tick = input.Scaled(1u, divide);
        return true;
    }
};
