#pragma once

#include <array>
#include <cstdint>

struct Vec3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ImuSample {
    Vec3f accel{};
    Vec3f gyro{};
    float temperature = 0.0f;
};

struct DisplaySize {
    std::uint16_t width = 0;
    std::uint16_t height = 0;
};

enum class PixelFormat : std::uint8_t {
    Mono1 = 0,
    Rgb565 = 1,
};

struct Rgb888 {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
};

using UidBuffer = std::array<std::uint8_t, 10>;
