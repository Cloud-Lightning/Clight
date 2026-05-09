#pragma once

#include <cstdint>

namespace clight {

using ClightSelfTestWriteFn = void (*)(const char *text, void *arg);

struct ClightSelfTestOptions {
    bool runApiTests = true;
    bool runBspDirectTests = true;
    bool runUnsupportedTests = true;
    bool enableWatchdogRuntime = false;
};

struct ClightSelfTestSummary {
    std::uint32_t passed = 0;
    std::uint32_t failed = 0;
    std::uint32_t unsupported = 0;
};

ClightSelfTestSummary runClightSelfTest(ClightSelfTestWriteFn write,
                                        void *arg,
                                        const ClightSelfTestOptions &options = {});

} // namespace clight
