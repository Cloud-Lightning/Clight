#include "Bmi088.hpp"

#include <array>
#include <cstddef>

namespace {

constexpr std::uint8_t kAccelChipIdReg = 0x00U;
constexpr std::uint8_t kAccelChipId = 0x1EU;
constexpr std::uint8_t kAccelSoftResetReg = 0x7EU;
constexpr std::uint8_t kAccelSoftResetValue = 0xB6U;
constexpr std::uint8_t kGyroChipIdReg = 0x00U;
constexpr std::uint8_t kGyroChipId = 0x0FU;
constexpr std::uint8_t kGyroSoftResetReg = 0x14U;
constexpr std::uint8_t kGyroSoftResetValue = 0xB6U;
constexpr std::uint8_t kAccelPowerConfReg = 0x7CU;
constexpr std::uint8_t kAccelPowerCtrlReg = 0x7DU;
constexpr std::uint8_t kAccelConfigReg = 0x40U;
constexpr std::uint8_t kAccelRangeReg = 0x41U;
constexpr std::uint8_t kGyroRangeReg = 0x0FU;
constexpr std::uint8_t kGyroBandwidthReg = 0x10U;
constexpr std::uint8_t kGyroLpmReg = 0x11U;
constexpr std::uint8_t kGyroCtrlReg = 0x15U;
constexpr std::uint8_t kAccelInt1IoCtrlReg = 0x53U;
constexpr std::uint8_t kAccelIntMapDataReg = 0x58U;
constexpr std::uint8_t kGyroInt34IoCtrlReg = 0x16U;
constexpr std::uint8_t kGyroInt34MapReg = 0x18U;
constexpr std::uint8_t kAccelDataReg = 0x12U;
constexpr std::uint8_t kGyroDataReg = 0x02U;
constexpr std::uint8_t kTempDataReg = 0x22U;

constexpr std::uint8_t kAccelDefaultConfig = 0xABU; /* normal + 800Hz + required bit */
constexpr std::uint8_t kAccelDefaultRange = 0x00U;  /* 3g, matches the original board driver */
constexpr std::uint8_t kGyroDefaultRange = 0x00U;   /* 2000 dps */
constexpr std::uint8_t kGyroDefaultBandwidth = 0x82U;
constexpr std::uint8_t kGyroDefaultLpm = 0x00U;
constexpr std::uint8_t kGyroDefaultCtrl = 0x80U;
constexpr std::uint8_t kAccelInt1IoCtrl = 0x08U;
constexpr std::uint8_t kAccelIntMapData = 0x04U;
constexpr std::uint8_t kGyroInt34IoCtrl = 0x00U;
constexpr std::uint8_t kGyroInt34Map = 0x01U;

constexpr std::uint32_t kShortDelayIterations = 20000U;
constexpr std::uint32_t kLongDelayIterations = 12000000U;

struct RegisterWrite {
    std::uint8_t reg;
    std::uint8_t value;
};

std::int16_t readLittleEndian16(const std::uint8_t *data)
{
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(data[0]) |
                                     (static_cast<std::uint16_t>(data[1]) << 8U));
}

void busyDelay(std::uint32_t iterations)
{
    while (iterations-- > 0U) {
        __asm__ volatile("" ::: "memory");
    }
}

float accelScaleFromRange(std::uint8_t range)
{
    switch (range) {
    case 0x00U:
        return (3.0f * 9.80665f) / 32768.0f;
    case 0x01U:
        return (6.0f * 9.80665f) / 32768.0f;
    case 0x02U:
        return (12.0f * 9.80665f) / 32768.0f;
    case 0x03U:
        return (24.0f * 9.80665f) / 32768.0f;
    default:
        return (6.0f * 9.80665f) / 32768.0f;
    }
}

float gyroScaleFromRange(std::uint8_t range)
{
    switch (range) {
    case 0x00U:
        return 2000.0f / 32768.0f;
    case 0x01U:
        return 1000.0f / 32768.0f;
    case 0x02U:
        return 500.0f / 32768.0f;
    case 0x03U:
        return 250.0f / 32768.0f;
    case 0x04U:
        return 125.0f / 32768.0f;
    default:
        return 2000.0f / 32768.0f;
    }
}

Status writeRegisterTable(const RegisterWrite *table,
                          std::size_t size,
                          const auto &writer,
                          const auto &reader)
{
    for (std::size_t index = 0; index < size; ++index) {
        const auto status = writer(table[index].reg, table[index].value);
        if (status != Status::Ok) {
            return status;
        }
        busyDelay(kShortDelayIterations);

        std::array<std::uint8_t, 1> verify{};
        const auto readStatus = reader(table[index].reg, ByteSpan(verify.data(), verify.size()));
        if (readStatus != Status::Ok) {
            return readStatus;
        }
        if (verify[0] != table[index].value) {
            const auto retryStatus = writer(table[index].reg, table[index].value);
            if (retryStatus != Status::Ok) {
                return retryStatus;
            }
            busyDelay(kShortDelayIterations);
            const auto retryReadStatus = reader(table[index].reg, ByteSpan(verify.data(), verify.size()));
            if (retryReadStatus != Status::Ok) {
                return retryReadStatus;
            }
            if (verify[0] != table[index].value) {
                return Status::Error;
            }
        }
        busyDelay(kShortDelayIterations);
    }
    return Status::Ok;
}

Status readChipIdTwice(std::uint8_t expected, const auto &reader)
{
    std::array<std::uint8_t, 1> chipId{};
    auto status = reader(kAccelChipIdReg, ByteSpan(chipId.data(), chipId.size()));
    if (status != Status::Ok) {
        return status;
    }
    busyDelay(kShortDelayIterations);

    status = reader(kAccelChipIdReg, ByteSpan(chipId.data(), chipId.size()));
    if (status != Status::Ok) {
        return status;
    }
    busyDelay(kShortDelayIterations);
    return (chipId[0] == expected) ? Status::Ok : Status::Error;
}

} // namespace

Bmi088::Bmi088(Spi &spi, Gpio &accelCs, Gpio &gyroCs)
    : spi_(spi),
      accelCs_(accelCs),
      gyroCs_(gyroCs),
      accelScale_(accelScaleFromRange(kAccelDefaultRange)),
      gyroScale_(gyroScaleFromRange(kGyroDefaultRange))
{
}

Status Bmi088::init()
{
    auto status = spi_.init();
    if (status != Status::Ok) {
        return status;
    }

    status = accelCs_.initOutput();
    if (status != Status::Ok) {
        return status;
    }
    status = gyroCs_.initOutput();
    if (status != Status::Ok) {
        return status;
    }
    accelCs_.write(true);
    gyroCs_.write(true);

    const auto accelReader = [this](std::uint8_t reg, ByteSpan data) {
        return readAccelBlock(reg, data);
    };
    const auto gyroReader = [this](std::uint8_t reg, ByteSpan data) {
        return readGyroBlock(reg, data);
    };

    status = readChipIdTwice(kAccelChipId, accelReader);
    if (status != Status::Ok) {
        return status;
    }
    status = readChipIdTwice(kGyroChipId, gyroReader);
    if (status != Status::Ok) {
        return status;
    }

    status = writeAccelRegister(kAccelSoftResetReg, kAccelSoftResetValue);
    if (status != Status::Ok) {
        return status;
    }
    status = writeGyroRegister(kGyroSoftResetReg, kGyroSoftResetValue);
    if (status != Status::Ok) {
        return status;
    }
    busyDelay(kLongDelayIterations);

    /*
     * The accelerometer interface needs dummy SPI traffic again after reset.
     * Without this, the first power-control write can be lost and XYZ stays 0.
     */
    status = readChipIdTwice(kAccelChipId, accelReader);
    if (status != Status::Ok) {
        return status;
    }
    status = readChipIdTwice(kGyroChipId, gyroReader);
    if (status != Status::Ok) {
        return status;
    }

    constexpr RegisterWrite accelInitTable[] = {
        {kAccelPowerCtrlReg, 0x04U},
        {kAccelPowerConfReg, 0x00U},
        {kAccelConfigReg, kAccelDefaultConfig},
        {kAccelRangeReg, kAccelDefaultRange},
        {kAccelInt1IoCtrlReg, kAccelInt1IoCtrl},
        {kAccelIntMapDataReg, kAccelIntMapData},
    };
    status = writeRegisterTable(accelInitTable,
                                sizeof(accelInitTable) / sizeof(accelInitTable[0]),
                                [this](std::uint8_t reg, std::uint8_t value) {
                                    return writeAccelRegister(reg, value);
                                },
                                accelReader);
    if (status != Status::Ok) {
        return status;
    }

    constexpr RegisterWrite gyroInitTable[] = {
        {kGyroRangeReg, kGyroDefaultRange},
        {kGyroBandwidthReg, kGyroDefaultBandwidth},
        {kGyroLpmReg, kGyroDefaultLpm},
        {kGyroCtrlReg, kGyroDefaultCtrl},
        {kGyroInt34IoCtrlReg, kGyroInt34IoCtrl},
        {kGyroInt34MapReg, kGyroInt34Map},
    };
    return writeRegisterTable(gyroInitTable,
                              sizeof(gyroInitTable) / sizeof(gyroInitTable[0]),
                              [this](std::uint8_t reg, std::uint8_t value) {
                                  return writeGyroRegister(reg, value);
                              },
                              gyroReader);
}

Status Bmi088::update()
{
    std::array<std::uint8_t, 6> accelRaw{};
    std::array<std::uint8_t, 6> gyroRaw{};
    std::array<std::uint8_t, 2> tempRaw{};

    auto status = readAccelBlock(kAccelDataReg, accelRaw);
    if (status != Status::Ok) {
        return status;
    }
    status = readGyroBlock(kGyroDataReg, gyroRaw);
    if (status != Status::Ok) {
        return status;
    }
    status = readAccelBlock(kTempDataReg, tempRaw);
    if (status != Status::Ok) {
        return status;
    }

    sample_.accel.x = static_cast<float>(readLittleEndian16(&accelRaw[0])) * accelScale_;
    sample_.accel.y = static_cast<float>(readLittleEndian16(&accelRaw[2])) * accelScale_;
    sample_.accel.z = static_cast<float>(readLittleEndian16(&accelRaw[4])) * accelScale_;
    sample_.gyro.x = static_cast<float>(readLittleEndian16(&gyroRaw[0])) * gyroScale_;
    sample_.gyro.y = static_cast<float>(readLittleEndian16(&gyroRaw[2])) * gyroScale_;
    sample_.gyro.z = static_cast<float>(readLittleEndian16(&gyroRaw[4])) * gyroScale_;

    const std::int16_t tempRaw12 =
        static_cast<std::int16_t>((static_cast<std::int16_t>(tempRaw[0]) << 3U) |
                                  (static_cast<std::int16_t>(tempRaw[1]) >> 5U));
    const std::int16_t signedTempRaw = (tempRaw12 > 1023) ? static_cast<std::int16_t>(tempRaw12 - 2048) : tempRaw12;
    sample_.temperature = (static_cast<float>(signedTempRaw) * 0.125f) + 23.0f;
    return Status::Ok;
}

Status Bmi088::setAccelRange(std::uint8_t value)
{
    const auto status = writeAccelRegister(kAccelRangeReg, value);
    if (status == Status::Ok) {
        accelScale_ = accelScaleFromRange(value);
    }
    return status;
}

Status Bmi088::setGyroRange(std::uint8_t value)
{
    const auto status = writeGyroRegister(kGyroRangeReg, value);
    if (status == Status::Ok) {
        gyroScale_ = gyroScaleFromRange(value);
    }
    return status;
}

Status Bmi088::readAccelBlock(std::uint8_t reg, ByteSpan data) const
{
    std::array<std::uint8_t, 16> tx{};
    std::array<std::uint8_t, 16> rx{};
    if ((data.size() + 2U) > tx.size()) {
        return Status::Param;
    }
    tx.fill(0x55U);
    tx[0] = static_cast<std::uint8_t>(reg | 0x80U);
    const auto status = transfer(accelCs_, ByteView(tx.data(), data.size() + 2U), ByteSpan(rx.data(), data.size() + 2U));
    if (status != Status::Ok) {
        return status;
    }
    for (std::size_t index = 0; index < data.size(); ++index) {
        data[index] = rx[index + 2U];
    }
    return Status::Ok;
}

Status Bmi088::readGyroBlock(std::uint8_t reg, ByteSpan data) const
{
    std::array<std::uint8_t, 16> tx{};
    std::array<std::uint8_t, 16> rx{};
    if ((data.size() + 1U) > tx.size()) {
        return Status::Param;
    }
    tx.fill(0x55U);
    tx[0] = static_cast<std::uint8_t>(reg | 0x80U);
    const auto status = transfer(gyroCs_, ByteView(tx.data(), data.size() + 1U), ByteSpan(rx.data(), data.size() + 1U));
    if (status != Status::Ok) {
        return status;
    }
    for (std::size_t index = 0; index < data.size(); ++index) {
        data[index] = rx[index + 1U];
    }
    return Status::Ok;
}

Status Bmi088::writeAccelRegister(std::uint8_t reg, std::uint8_t value) const
{
    std::array<std::uint8_t, 2> tx = {reg, value};
    std::array<std::uint8_t, 2> rx{};
    return transfer(accelCs_, ByteView(tx.data(), tx.size()), ByteSpan(rx.data(), rx.size()));
}

Status Bmi088::writeGyroRegister(std::uint8_t reg, std::uint8_t value) const
{
    std::array<std::uint8_t, 2> tx = {reg, value};
    std::array<std::uint8_t, 2> rx{};
    return transfer(gyroCs_, ByteView(tx.data(), tx.size()), ByteSpan(rx.data(), rx.size()));
}

Status Bmi088::transfer(Gpio &chipSelect, ByteView tx, ByteSpan rx) const
{
    chipSelect.write(false);
    const auto status = spi_.transfer(tx, rx);
    chipSelect.write(true);
    return status;
}
