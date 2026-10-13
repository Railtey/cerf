#pragma once

#include "cerf_emulator.h"

#include <atomic>

template <typename T>
class DeferredService {
public:
    explicit DeferredService(CerfEmulator& emu) : emu_(emu) {}

    T& Get() const {
        T* service = service_.load(std::memory_order_acquire);
        if (service != nullptr) return *service;
        service = &emu_.Get<T>();
        service_.store(service, std::memory_order_release);
        return *service;
    }

private:
    CerfEmulator& emu_;
    mutable std::atomic<T*> service_{nullptr};
};
