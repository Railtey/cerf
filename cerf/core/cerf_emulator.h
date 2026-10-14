#pragma once
#include "service.h"
#include "main_config.h"

#include <servicelocator/service_locator.h>

class CerfEmulator : public servicelocator::ServiceLocator<CerfEmulator, 2048> {
public:
    CerfEmulator(const CerfConfig& config, int argc, char** argv);
    ~CerfEmulator();

    void Boot();
    void WaitForExit();

    [[noreturn]] static void OnLocatorError(const char* message);
    static void OnServiceDestroy(const servicelocator::Service& service);

    const CerfConfig& Config() const { return config_; }
    int    Argc() const { return argc_; }
    char** Argv() const { return argv_; }

private:
    CerfConfig config_;
    int        argc_ = 0;
    char**     argv_ = nullptr;
};

#define REGISTER_SERVICE(Type) SERVICELOCATOR_REGISTER(CerfEmulator, Type)
#define REGISTER_SERVICE_AS(Type, Base) SERVICELOCATOR_REGISTER_AS(CerfEmulator, Type, Base)
#define REGISTER_SERVICE_AS_FALLBACK(Type, Base) \
    SERVICELOCATOR_REGISTER_AS_FALLBACK(CerfEmulator, Type, Base)
