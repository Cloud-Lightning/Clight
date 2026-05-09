#include "Ws2812.hpp"

#include <array>

namespace {

void encodeSpiByte(std::uint8_t value, std::uint8_t *encoded)
{
    std::uint32_t pattern = 0U;
    for (int bit = 7; bit >= 0; --bit) {
        const std::uint32_t symbol = ((value >> bit) & 0x01U) != 0U ? 0b110U : 0b100U;
        pattern = (pattern << 3U) | symbol;
    }
    encoded[0] = static_cast<std::uint8_t>(pattern >> 16U);
    encoded[1] = static_cast<std::uint8_t>(pattern >> 8U);
    encoded[2] = static_cast<std::uint8_t>(pattern);
}

Status spiInit(void *context)
{
    if (context == nullptr) {
        return Status::Param;
    }
    return static_cast<Spi *>(context)->init(SpiMode::Mode0, 2400000U);
}

Status spiWrite(void *context, std::span<const Rgb888> pixels)
{
    if (context == nullptr) {
        return Status::Param;
    }

    auto &bus = *static_cast<Spi *>(context);
    std::array<std::uint8_t, 9> encoded{};
    for (const auto &pixel : pixels) {
        encodeSpiByte(pixel.g, encoded.data());
        encodeSpiByte(pixel.r, encoded.data() + 3U);
        encodeSpiByte(pixel.b, encoded.data() + 6U);
        const auto status = bus.write(ByteView(encoded.data(), encoded.size()));
        if (status != Status::Ok) {
            return status;
        }
    }
    return Status::Ok;
}

bool spiBusy(void *context)
{
    (void) context;
    return false;
}

const Ws2812Ops kSpiOps = {
    .init = spiInit,
    .write = spiWrite,
    .busy = spiBusy,
};

} // namespace

Ws2812::Ws2812(Spi &bus, std::span<Rgb888> pixels)
    : Ws2812(kSpiOps, &bus, pixels)
{
}

Ws2812::Ws2812(const Ws2812Ops &ops, void *context, std::span<Rgb888> pixels)
    : ops_(&ops), context_(context), pixels_(pixels)
{
}

Status Ws2812::init()
{
    if ((ops_ == nullptr) || (ops_->init == nullptr)) {
        return Status::Unsupported;
    }
    return ops_->init(context_);
}

void Ws2812::setPixel(std::size_t index, Rgb888 color)
{
    if (index < pixels_.size()) {
        pixels_[index] = color;
    }
}

void Ws2812::fill(Rgb888 color)
{
    for (auto &pixel : pixels_) {
        pixel = color;
    }
}

void Ws2812::clear()
{
    fill(Rgb888{});
}

Status Ws2812::refresh()
{
    if ((ops_ == nullptr) || (ops_->write == nullptr)) {
        return Status::Unsupported;
    }
    if (busy()) {
        return Status::Busy;
    }
    return ops_->write(context_, std::span<const Rgb888>(pixels_.data(), pixels_.size()));
}

bool Ws2812::busy() const
{
    if ((ops_ == nullptr) || (ops_->busy == nullptr)) {
        return false;
    }
    return ops_->busy(context_);
}
