#include "cycle_anchored_counter.h"

#include <numeric>

bool CycleAnchoredCounter::ReduceRatio(uint64_t cycles, uint64_t ticks, uint64_t& cyc,
                                       uint64_t& tk) {
    const uint64_t g = std::gcd(cycles, ticks);
    cyc = cycles / g;
    tk  = ticks / g;
    return (cyc - 1u) <= UINT64_MAX / tk && (tk - 1u) <= (UINT64_MAX - tk) / cyc;
}

bool CycleAnchoredCounter::RateRatio(GuestCycleClock::Rate core, GuestCycleClock::Rate tick,
                                     uint64_t& cycles, uint64_t& ticks) {
    if (core.num == 0u || core.den == 0u || tick.num == 0u || tick.den == 0u ||
        core.num > UINT64_MAX / tick.den || core.den > UINT64_MAX / tick.num) {
        return false;
    }
    cycles = core.num * tick.den;
    ticks  = core.den * tick.num;
    return true;
}

bool CycleAnchoredCounter::SetRate(GuestCycleClock::Rate core, GuestCycleClock::Rate tick) {
    uint64_t cycles = 0, ticks = 0;
    return RateRatio(core, tick, cycles, ticks) && SetRatio(cycles, ticks);
}

bool CycleAnchoredCounter::RescaleRate(uint64_t cycle, GuestCycleClock::Rate core,
                                       GuestCycleClock::Rate tick) {
    uint64_t cycles = 0, ticks = 0;
    return RateRatio(core, tick, cycles, ticks) && Rescale(cycle, cycles, ticks);
}

bool CycleAnchoredCounter::SetRatio(uint64_t cycles, uint64_t ticks) {
    uint64_t cyc = 0, tk = 0;
    if (!ReduceRatio(cycles, ticks, cyc, tk)) return false;
    cyc_unit_ = cyc;
    tk_unit_  = tk;
    return true;
}

bool CycleAnchoredCounter::Rescale(uint64_t cycle, uint64_t cycles, uint64_t ticks) {
    uint64_t cyc = 0, tk = 0;
    if (!ReduceRatio(cycles, ticks, cyc, tk)) return false;
    const uint64_t part  = PhaseAt(cycle);
    const uint64_t den   = cyc_unit_;
    const uint32_t count = CountAt(cycle);
    cyc_unit_ = cyc;
    tk_unit_  = tk;
    PlaceAnchor(cycle, count, part, den);
    return true;
}

bool CycleAnchoredCounter::AnchorAtPhase(uint64_t cycle, uint32_t count, uint64_t phase,
                                         uint64_t denominator) {
    if (denominator == 0u || phase >= denominator) return false;
    PlaceAnchor(cycle, count, phase, denominator);
    return true;
}

bool CycleAnchoredCounter::DivideChangePhase(uint64_t cycle, uint64_t old_divide,
                                             uint64_t new_divide, uint64_t& phase,
                                             uint64_t& denominator) const {
    const uint64_t widest = old_divide > new_divide ? old_divide : new_divide;
    if (old_divide == 0u || new_divide == 0u || cyc_unit_ > UINT64_MAX / widest) return false;
    const uint64_t edge = PhaseAt(cycle) * old_divide % cyc_unit_;
    phase       = (new_divide - 1u) * cyc_unit_ + edge;
    denominator = new_divide * cyc_unit_;
    return true;
}

void CycleAnchoredCounter::PlaceAnchor(uint64_t cycle, uint32_t count, uint64_t phase,
                                       uint64_t denominator) {
    anchor_count_ = count;
    anchor_cycle_ = cycle - MulDivU64(phase, cyc_unit_, denominator) / tk_unit_;
}
