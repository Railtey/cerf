#pragma once

#include "../../core/service.h"
#include "../../jit/guest_cycle_clock.h"
#include "../../socs/rated_tick_count.h"

#include <cstdint>
#include <mutex>
#include <vector>

class HostRequestChannel;
class StateReader;
class StateWriter;

/* Jornada touch model. Holds the current raw 10-bit pen ADC (read by the MCU's
   GetTouchSamples handler) and drives the SA-1110 GPIO9 pen line that touch.dll
   watches (sub_FB1A98: GPLR bit 9 low = pen down/collect, high = pen up). A thin
   TouchInput adapter forwards host pointer events here. */
class HpJornada700Touch : public Service {
public:
    using Service::Service;

    bool ShouldRegister() override;
    void OnReady() override;

    /* Host pointer events (guest-surface coords), from the TouchInput adapter. */
    void PenDown(int x, int y);
    void PenMove(int x, int y);
    void PenUp();

    /* MCU GetTouchSamples (0xA0): current raw 10-bit ADC for both axes. */
    void CurrentAdc(uint16_t* adc_x, uint16_t* adc_y) const;

    /* Synthetic pen tap at a fixed RAW ADC point, bypassing host-coordinate
       mapping. For the bezel soft-button zones touch.dll watches (sub_FB1314:
       X 31..67 - below MapHostToAdc's X floor, unreachable by host clicks). */
    void TapRawAdc(uint16_t adc_x, uint16_t adc_y);

    void SelectSampleRate(uint8_t mcu_argument);
    void ResetSampleRate();

    void SaveState(StateWriter& w) const;
    void RestoreState(StateReader& r);
    void PostRestore();

private:
    enum class HostPenKind : uint8_t { Down, Up, RawTap };
    struct HostPen {
        HostPenKind kind;
        int         x;
        int         y;
        uint16_t    adc_x;
        uint16_t    adc_y;
    };

    void     QueueHostPen(const HostPen& pen);
    void     OnHostRequest();
    void     ApplyHostPen(const HostPen& pen);
    void     StartPulses();
    void     RescalePulses();
    void     ArmNextPulse();
    void     OnPulseEvent();
    void     Pulse();
    void     SetHighRate(bool high);
    GuestCycleClock::Rate PulseRate() const;
    void     MapHostToAdc(int hx, int hy, uint16_t* ax, uint16_t* ay) const;
    void     DrivePenLine(bool pen_down);
    void     PulsePenLine();

    std::mutex           host_mtx_;
    std::vector<HostPen> host_pens_;
    int                  last_x_ = 0;
    int                  last_y_ = 0;

    GuestCycleClock*        clock_         = nullptr;
    GuestCycleClock::Event* pulse_event_   = nullptr;
    HostRequestChannel*     host_requests_ = nullptr;
    RatedTickCount          pulses_;
    uint64_t                next_pulse_       = 0;
    bool                    high_rate_        = false;
    bool                    pen_down_         = false;
    int                     synthetic_pulses_ = 0;
    uint16_t                adc_x_            = 0;
    uint16_t                adc_y_            = 0;
};
