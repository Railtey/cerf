#define NOMINMAX

#include "hp_jornada_700_touch.h"

#include "../../core/cerf_emulator.h"
#include "../../core/log.h"
#include "../../host/host_canvas.h"
#include "../../host/touch_input.h"
#include "../../jit/host_request_channel.h"
#include "../../socs/sa11xx/sa11xx_gpio.h"
#include "../../state/state_stream.h"
#include "../board_context.h"
#include "hp_jornada_700_family_id.h"

namespace {

/* Virtual panel raw-ADC characteristic (10-bit, linear across the surface).
   touch.dll's affine matrix calibrates it, so the align-screen applet adapts
   to whatever extents are produced here. */
constexpr uint16_t kAdcXMin = 130, kAdcXMax = 900;
constexpr uint16_t kAdcYMin = 120, kAdcYMax = 900;
/* jornada720 touch.dll sub_FB16D0 (TouchPanelGetDeviceCaps) and sub_FB1750
   (TouchPanelSetMode). */
constexpr GuestCycleClock::Rate kLowRate{100u, 1u};
constexpr GuestCycleClock::Rate kHighRate{200u, 1u};
constexpr uint8_t               kLowRateArgument  = 8u;
constexpr uint8_t               kHighRateArgument = 3u;

uint16_t MapAxis(int v, int v_max, uint16_t lo, uint16_t hi) {
    if (v_max <= 0) return lo;
    if (v <= 0)     return lo;
    if (v >= v_max) return hi;
    return static_cast<uint16_t>(lo + (uint32_t)v * (hi - lo) / (uint32_t)v_max);
}

class HpJornada700TouchInput : public TouchInput {
public:
    using TouchInput::TouchInput;
    bool ShouldRegister() override {
        auto* bd = emu_.TryGet<BoardContext>();
        return bd && bd->GetBoardFamilyId() == BoardFamilyId::HpJornada700;
    }
    void OnPenDown(int x, int y) override { emu_.Get<HpJornada700Touch>().PenDown(x, y); }
    void OnPenMove(int x, int y) override { emu_.Get<HpJornada700Touch>().PenMove(x, y); }
    void OnPenUp  (int, int) override { emu_.Get<HpJornada700Touch>().PenUp(); }
    void OnCaptureLost() override { emu_.Get<HpJornada700Touch>().PenUp(); }
};

}

REGISTER_SERVICE(HpJornada700Touch);
REGISTER_SERVICE_AS(HpJornada700TouchInput, TouchInput);

bool HpJornada700Touch::ShouldRegister() {
    auto* bd = emu_.TryGet<BoardContext>();
    return bd && bd->GetBoardFamilyId() == BoardFamilyId::HpJornada700;
}

void HpJornada700Touch::OnReady() {
    DrivePenLine(false);
    clock_       = &emu_.Get<GuestCycleClock>();
    pulse_event_ = clock_->Add([this] { OnPulseEvent(); });
    clock_->RegisterRateListener([this] { RescalePulses(); });
    host_requests_ = &emu_.Get<HostRequestChannel>();
    host_requests_->RegisterListener([this] { OnHostRequest(); });
}

void HpJornada700Touch::DrivePenLine(bool pen_down) {
    /* GPIO9: low = pen down (touch.dll collects), high = pen up. */
    emu_.Get<Sa11xxGpio>().DriveInputPin(9, !pen_down);
}

void HpJornada700Touch::PulsePenLine() {
    /* HP doc §4.4: a new sample is signalled by pulsing GPIO9 (high then low).
       The rising edge latches the driver's armed GRER bit 9 -> IRQ; by the time
       its IST reads GPLR the line is low again -> it collects a fresh sample. */
    auto& gpio = emu_.Get<Sa11xxGpio>();
    gpio.DriveInputPin(9, true);
    gpio.DriveInputPin(9, false);
}

void HpJornada700Touch::MapHostToAdc(int hx, int hy, uint16_t* ax, uint16_t* ay) const {
    auto&     hc = emu_.Get<HostCanvas>();
    const int w  = (int)hc.GuestSurfaceWidth();
    const int h  = (int)hc.GuestSurfaceHeight();
    *ax = MapAxis(hx, w - 1, kAdcXMin, kAdcXMax);
    *ay = MapAxis(hy, h - 1, kAdcYMin, kAdcYMax);
}

void HpJornada700Touch::CurrentAdc(uint16_t* adc_x, uint16_t* adc_y) const {
    *adc_x = adc_x_;
    *adc_y = adc_y_;
}

void HpJornada700Touch::PenDown(int x, int y) {
    QueueHostPen(HostPen{HostPenKind::Down, x, y, 0u, 0u});
}

void HpJornada700Touch::TapRawAdc(uint16_t adc_x, uint16_t adc_y) {
    QueueHostPen(HostPen{HostPenKind::RawTap, 0, 0, adc_x, adc_y});
}

void HpJornada700Touch::PenMove(int x, int y) {
    std::lock_guard<std::mutex> lk(host_mtx_);
    last_x_ = x;
    last_y_ = y;
}

void HpJornada700Touch::PenUp() {
    QueueHostPen(HostPen{HostPenKind::Up, 0, 0, 0u, 0u});
}

void HpJornada700Touch::QueueHostPen(const HostPen& pen) {
    {
        std::lock_guard<std::mutex> lk(host_mtx_);
        if (pen.kind == HostPenKind::Down) {
            last_x_ = pen.x;
            last_y_ = pen.y;
        }
        host_pens_.push_back(pen);
    }
    host_requests_->Request();
}

void HpJornada700Touch::OnHostRequest() {
    std::vector<HostPen> pens;
    {
        std::lock_guard<std::mutex> lk(host_mtx_);
        pens.swap(host_pens_);
    }
    for (const HostPen& pen : pens) ApplyHostPen(pen);
}

void HpJornada700Touch::ApplyHostPen(const HostPen& pen) {
    switch (pen.kind) {
    case HostPenKind::Down:
        synthetic_pulses_ = 0;
        MapHostToAdc(pen.x, pen.y, &adc_x_, &adc_y_);
        break;
    case HostPenKind::RawTap:
        synthetic_pulses_ = 4;
        adc_x_ = pen.adc_x;
        adc_y_ = pen.adc_y;
        break;
    case HostPenKind::Up:
        if (!pen_down_) return;
        synthetic_pulses_ = 0;
        pen_down_ = false;
        clock_->Disarm(pulse_event_);
        DrivePenLine(false);
        return;
    }
    DrivePenLine(true);
    if (pen_down_) return;
    pen_down_ = true;
    StartPulses();
}

GuestCycleClock::Rate HpJornada700Touch::PulseRate() const {
    return high_rate_ ? kHighRate : kLowRate;
}

void HpJornada700Touch::StartPulses() {
    pulses_.SetRate(clock_->ClockRate(), PulseRate());
    pulses_.Start(clock_->Cycles());
    next_pulse_ = 1u;
    ArmNextPulse();
}

void HpJornada700Touch::RescalePulses() {
    if (!pen_down_) return;
    pulses_.Rescale(clock_->Cycles(), clock_->ClockRate(), PulseRate());
    ArmNextPulse();
}

void HpJornada700Touch::ArmNextPulse() {
    clock_->Arm(pulse_event_, pulses_.CycleOfTick(next_pulse_));
}

void HpJornada700Touch::OnPulseEvent() {
    if (!pen_down_) return;
    next_pulse_ = pulses_.TicksAt(clock_->Cycles()) + 1u;
    Pulse();
    if (pen_down_) ArmNextPulse();
}

void HpJornada700Touch::Pulse() {
    if (synthetic_pulses_ > 0) {
        if (--synthetic_pulses_ == 0) {
            pen_down_ = false;
            DrivePenLine(false);
            return;
        }
    } else {
        int x = 0;
        int y = 0;
        {
            std::lock_guard<std::mutex> lk(host_mtx_);
            x = last_x_;
            y = last_y_;
        }
        MapHostToAdc(x, y, &adc_x_, &adc_y_);
    }
    PulsePenLine();
}

void HpJornada700Touch::SetHighRate(bool high) {
    high_rate_ = high;
    RescalePulses();
}

void HpJornada700Touch::ResetSampleRate() { SetHighRate(false); }

void HpJornada700Touch::SelectSampleRate(uint8_t mcu_argument) {
    if (mcu_argument == kHighRateArgument) {
        SetHighRate(true);
    } else if (mcu_argument == kLowRateArgument) {
        SetHighRate(false);
    } else {
        LOG(Caution, "HpJornada700Touch: MCU 0xA2 argument 0x%02X selects no known sample rate\n",
            mcu_argument);
    }
}

void HpJornada700Touch::SaveState(StateWriter& w) const {
    w.Write<uint8_t>("touch_high_rate", high_rate_ ? 1u : 0u);
    w.Write<uint8_t>("touch_pen_down", pen_down_ ? 1u : 0u);
    w.Write<uint16_t>("touch_adc_x", adc_x_);
    w.Write<uint16_t>("touch_adc_y", adc_y_);
}

void HpJornada700Touch::RestoreState(StateReader& r) {
    uint8_t high = 0;
    uint8_t down = 0;
    r.Read("touch_high_rate", high);
    r.Read("touch_pen_down", down);
    r.Read("touch_adc_x", adc_x_);
    r.Read("touch_adc_y", adc_y_);
    high_rate_        = high != 0u;
    pen_down_         = down != 0u;
    synthetic_pulses_ = 0;
    clock_->Disarm(pulse_event_);
    std::lock_guard<std::mutex> lk(host_mtx_);
    host_pens_.clear();
}

void HpJornada700Touch::PostRestore() {
    if (!pen_down_) return;
    pen_down_ = false;
    DrivePenLine(false);
}
