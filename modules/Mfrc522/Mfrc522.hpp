#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "Common.hpp"
#include "DeviceTypes.hpp"
#include "Gpio.hpp"
#include "Spi.hpp"

class Mfrc522 {
public:
    using DelayCallback = void (*)(std::uint32_t ms);

    struct Uid {
        std::uint8_t size = 0U;
        UidBuffer bytes{};
        std::uint8_t sak = 0U;
    };

    Mfrc522(Spi &bus, Gpio &chipSelect, Gpio *reset = nullptr, DelayCallback delay = nullptr);

    Status init(bool configureBus = true);
    Status readVersion(std::uint8_t &version);
    Status isCardPresent(bool &present);
    Status readUid(UidBuffer &uid, std::size_t &length);

    bool isNewCardPresent();
    bool readCardSerial();
    void haltCard();

    std::uint8_t readVersion();
    std::uint8_t readCommandReg();
    std::uint8_t readTxControlReg();
    std::uint8_t readModeReg();
    std::uint8_t readErrorReg();
    std::uint8_t readComIrqReg();

    std::uint8_t lastRequestStatus() const { return static_cast<std::uint8_t>(lastRequestStatus_); }
    std::uint8_t lastSelectStatus() const { return static_cast<std::uint8_t>(lastSelectStatus_); }
    std::uint8_t lastAtqa0() const { return lastAtqa_[0]; }
    std::uint8_t lastAtqa1() const { return lastAtqa_[1]; }
    const Uid &uid() const { return uid_; }

private:
    enum class Register : std::uint8_t {
        Command = 0x01U << 1,
        ComIrq = 0x04U << 1,
        DivIrq = 0x05U << 1,
        Error = 0x06U << 1,
        Status2 = 0x08U << 1,
        FIFOData = 0x09U << 1,
        FIFOLevel = 0x0AU << 1,
        Control = 0x0CU << 1,
        BitFraming = 0x0DU << 1,
        Coll = 0x0EU << 1,
        Mode = 0x11U << 1,
        TxControl = 0x14U << 1,
        TxASK = 0x15U << 1,
        RxSel = 0x17U << 1,
        RFCfg = 0x26U << 1,
        TMode = 0x2AU << 1,
        TPrescaler = 0x2BU << 1,
        TReloadH = 0x2CU << 1,
        TReloadL = 0x2DU << 1,
        CRCResultH = 0x21U << 1,
        CRCResultL = 0x22U << 1,
        Version = 0x37U << 1,
    };

    enum class Command : std::uint8_t {
        Idle = 0x00U,
        CalcCRC = 0x03U,
        Transceive = 0x0CU,
        SoftReset = 0x0FU,
    };

    enum class PiccCommand : std::uint8_t {
        Reqa = 0x26U,
        Wupa = 0x52U,
        Ct = 0x88U,
        SelCl1 = 0x93U,
        SelCl2 = 0x95U,
        SelCl3 = 0x97U,
        Hlta = 0x50U,
    };

    enum class StatusCode : std::uint8_t {
        Idle = 0U,
        Ok = 1U,
        Error = 2U,
        Collision = 3U,
        Timeout = 4U,
        NoRoom = 5U,
        InternalError = 6U,
        Invalid = 7U,
        CrcWrong = 8U,
        MifareNack = 9U,
    };

    void beginTransaction() const;
    void endTransaction() const;
    void delay(std::uint32_t ms) const;
    Status writeRegister(Register reg, std::uint8_t value) const;
    Status writeRegister(Register reg, std::span<const std::uint8_t> values) const;
    Status readRegister(Register reg, std::uint8_t &value) const;
    Status readRegister(Register reg, std::span<std::uint8_t> values, std::uint8_t rxAlign = 0U) const;
    std::uint8_t readRegisterValue(Register reg) const;
    Status setRegisterBitMask(Register reg, std::uint8_t mask) const;
    Status clearRegisterBitMask(Register reg, std::uint8_t mask) const;

    static Status statusFromPicc(StatusCode status);
    StatusCode calculateCrc(std::span<const std::uint8_t> data, std::span<std::uint8_t, 2> result);
    void reset();
    void antennaOn();
    StatusCode transceiveData(std::span<const std::uint8_t> sendData,
                              std::span<std::uint8_t> backData,
                              std::uint8_t &backLen,
                              std::uint8_t *validBits = nullptr,
                              std::uint8_t rxAlign = 0U,
                              bool checkCrc = false);
    StatusCode communicateWithPicc(Command command,
                                   std::uint8_t waitIrq,
                                   std::span<const std::uint8_t> sendData,
                                   std::span<std::uint8_t> backData,
                                   std::uint8_t &backLen,
                                   std::uint8_t *validBits,
                                   std::uint8_t rxAlign,
                                   bool checkCrc);
    StatusCode requestA(std::span<std::uint8_t, 2> bufferAtqa, std::uint8_t &bufferSize);
    StatusCode anticoll(Uid &uid);
    StatusCode select(Uid &uid, std::uint8_t validBits = 0U);

    Spi &bus_;
    Gpio &chipSelect_;
    Gpio *reset_;
    DelayCallback delay_;
    Uid uid_{};
    std::array<std::uint8_t, 2> lastAtqa_{};
    StatusCode lastRequestStatus_ = StatusCode::Idle;
    StatusCode lastSelectStatus_ = StatusCode::Idle;
};
