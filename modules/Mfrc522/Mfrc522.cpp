#include "Mfrc522.hpp"

#include <algorithm>
#include <array>

namespace {
constexpr std::uint8_t kComIrqRxIdle = 0x30U;
constexpr std::uint8_t kDivIrqCrc = 0x04U;
constexpr std::uint8_t kFifoFlush = 0x80U;
constexpr std::uint8_t kStartSend = 0x80U;
constexpr std::uint8_t kCollValuesAfterColl = 0x80U;
constexpr std::uint8_t kStatus2MFCrypto1On = 0x08U;
constexpr std::uint8_t kBitFramingMask = 0x07U;
constexpr std::uint8_t kTxControlAntennaOn = 0x03U;
constexpr std::uint8_t kErrorMaskBufferOverflowParityProtocol = 0x13U;
constexpr std::uint8_t kErrorMaskCollision = 0x08U;
constexpr std::uint8_t kSelectCascadeBit = 0x04U;
constexpr std::uint8_t kRxSelDefault = 0x86U;
constexpr std::uint8_t kRfcfgMaxGain = 0x7FU;
constexpr std::uint8_t kTimerReloadHigh = 0x00U;
constexpr std::uint8_t kTimerReloadLow = 30U;
constexpr std::uint8_t kTimerModeAuto = 0x8DU;
constexpr std::uint8_t kTimerPrescaler = 0x3EU;
constexpr std::uint8_t kUidSingleSize = 4U;
constexpr std::uint8_t kAnticollFrameSize = 2U;
constexpr std::uint8_t kAnticollResponseSize = 5U;
constexpr std::uint8_t kSelectFrameSize = 9U;
constexpr std::uint8_t kSelectResponseSize = 3U;
constexpr std::uint32_t kResetDelayMs = 50U;
constexpr std::uint32_t kCommandLoopCount = 2000U;
constexpr std::uint32_t kCrcLoopCount = 5000U;

template <typename SpiType>
Status initRc522Spi(const SpiType &spi)
{
    return spi.init(SpiMode::Mode0, 1000000U, SpiBitOrder::MsbFirst, TransferMode::Blocking);
}
} // namespace

Mfrc522::Mfrc522(Spi &bus, Gpio &chipSelect, Gpio *reset, DelayCallback delay)
    : bus_(bus), chipSelect_(chipSelect), reset_(reset), delay_(delay)
{
}

Status Mfrc522::init(bool configureBus)
{
    if (configureBus) {
        auto status = initRc522Spi(bus_);
        if (status != Status::Ok) {
            return status;
        }
    }

    auto status = chipSelect_.initOutput();
    if (status != Status::Ok) {
        return status;
    }
    chipSelect_.write(true);

    if (reset_ != nullptr) {
        status = reset_->initOutput();
        if (status != Status::Ok) {
            return status;
        }

        reset_->write(false);
        delay(kResetDelayMs);
        reset_->write(true);
        delay(kResetDelayMs);
    }

    reset();
    if (writeRegister(Register::Mode, 0x3DU) != Status::Ok ||
        writeRegister(Register::TReloadH, kTimerReloadHigh) != Status::Ok ||
        writeRegister(Register::TReloadL, kTimerReloadLow) != Status::Ok ||
        writeRegister(Register::TMode, kTimerModeAuto) != Status::Ok ||
        writeRegister(Register::TPrescaler, kTimerPrescaler) != Status::Ok ||
        writeRegister(Register::TxASK, 0x40U) != Status::Ok ||
        writeRegister(Register::RxSel, kRxSelDefault) != Status::Ok ||
        writeRegister(Register::RFCfg, kRfcfgMaxGain) != Status::Ok) {
        return Status::Error;
    }
    antennaOn();
    delay(1U);
    return Status::Ok;
}

Status Mfrc522::readVersion(std::uint8_t &version)
{
    return readRegister(Register::Version, version);
}

Status Mfrc522::isCardPresent(bool &present)
{
    std::array<std::uint8_t, 2> atqa{};
    std::uint8_t size = static_cast<std::uint8_t>(atqa.size());
    const auto status = requestA(atqa, size);
    present = status == StatusCode::Ok || status == StatusCode::Collision;
    return statusFromPicc(status);
}

Status Mfrc522::readUid(UidBuffer &uid, std::size_t &length)
{
    if (!isNewCardPresent()) {
        length = 0U;
        return statusFromPicc(lastRequestStatus_);
    }
    if (!readCardSerial()) {
        length = 0U;
        return statusFromPicc(lastSelectStatus_);
    }

    length = uid_.size;
    for (std::size_t index = 0U; index < length && index < uid.size(); ++index) {
        uid[index] = uid_.bytes[index];
    }
    return Status::Ok;
}

bool Mfrc522::isNewCardPresent()
{
    std::array<std::uint8_t, 2> atqa{};
    std::uint8_t size = static_cast<std::uint8_t>(atqa.size());
    lastSelectStatus_ = StatusCode::Idle;
    const auto result = requestA(atqa, size);
    return result == StatusCode::Ok || result == StatusCode::Collision;
}

bool Mfrc522::readCardSerial()
{
    uid_.size = 0U;
    lastSelectStatus_ = select(uid_);
    return lastSelectStatus_ == StatusCode::Ok;
}

std::uint8_t Mfrc522::readVersion()
{
    return readRegisterValue(Register::Version);
}

std::uint8_t Mfrc522::readCommandReg()
{
    return readRegisterValue(Register::Command);
}

std::uint8_t Mfrc522::readTxControlReg()
{
    return readRegisterValue(Register::TxControl);
}

std::uint8_t Mfrc522::readModeReg()
{
    return readRegisterValue(Register::Mode);
}

std::uint8_t Mfrc522::readErrorReg()
{
    return readRegisterValue(Register::Error);
}

std::uint8_t Mfrc522::readComIrqReg()
{
    return readRegisterValue(Register::ComIrq);
}

void Mfrc522::haltCard()
{
    std::array<std::uint8_t, 4> buffer = {static_cast<std::uint8_t>(PiccCommand::Hlta), 0x00U, 0x00U, 0x00U};
    std::span<std::uint8_t, 2> crc(buffer.data() + 2U, 2U);
    if (calculateCrc(std::span<const std::uint8_t>(buffer.data(), 2U), crc) != StatusCode::Ok) {
        return;
    }

    std::array<std::uint8_t, 1> back{};
    std::uint8_t backLen = static_cast<std::uint8_t>(back.size());
    (void)communicateWithPicc(Command::Transceive, kComIrqRxIdle, buffer, back, backLen, nullptr, 0U, false);
}

void Mfrc522::beginTransaction() const
{
    chipSelect_.write(false);
}

void Mfrc522::endTransaction() const
{
    chipSelect_.write(true);
}

void Mfrc522::delay(std::uint32_t ms) const
{
    if (delay_ != nullptr) {
        delay_(ms);
    }
}

Status Mfrc522::writeRegister(Register reg, std::uint8_t value) const
{
    const std::array<std::uint8_t, 2> tx = {static_cast<std::uint8_t>(static_cast<std::uint8_t>(reg) & 0x7EU), value};
    beginTransaction();
    const auto status = bus_.write(ByteView(tx.data(), tx.size()));
    endTransaction();
    return status;
}

Status Mfrc522::writeRegister(Register reg, std::span<const std::uint8_t> values) const
{
    std::array<std::uint8_t, 65> tx{};
    tx[0] = static_cast<std::uint8_t>(static_cast<std::uint8_t>(reg) & 0x7EU);
    const auto count = std::min<std::size_t>(values.size(), tx.size() - 1U);
    for (std::size_t index = 0U; index < count; ++index) {
        tx[index + 1U] = values[index];
    }

    beginTransaction();
    const auto status = bus_.write(ByteView(tx.data(), count + 1U));
    endTransaction();
    return status;
}

Status Mfrc522::readRegister(Register reg, std::uint8_t &value) const
{
    const std::array<std::uint8_t, 2> tx = {static_cast<std::uint8_t>(0x80U | (static_cast<std::uint8_t>(reg) & 0x7EU)), 0x00U};
    std::array<std::uint8_t, 2> rx{};
    beginTransaction();
    const auto status = bus_.transfer(ByteView(tx.data(), tx.size()), ByteSpan(rx.data(), rx.size()));
    endTransaction();
    if (status != Status::Ok) {
        value = 0U;
        return status;
    }
    value = rx[1];
    return Status::Ok;
}

Status Mfrc522::readRegister(Register reg, std::span<std::uint8_t> values, std::uint8_t rxAlign) const
{
    if (values.empty()) {
        return Status::Ok;
    }

    const std::uint8_t address = static_cast<std::uint8_t>(0x80U | (static_cast<std::uint8_t>(reg) & 0x7EU));
    std::array<std::uint8_t, 66> tx{};
    std::array<std::uint8_t, 66> rx{};
    const auto count = std::min<std::size_t>(values.size(), tx.size() - 1U);
    tx[0] = address;
    for (std::size_t index = 1U; index < count; ++index) {
        tx[index] = address;
    }

    beginTransaction();
    const auto status = bus_.transfer(ByteView(tx.data(), count + 1U), ByteSpan(rx.data(), count + 1U));
    endTransaction();
    if (status != Status::Ok) {
        return status;
    }

    for (std::size_t index = 0U; index < count; ++index) {
        const std::uint8_t value = rx[index + 1U];
        if (index == 0U && rxAlign != 0U) {
            std::uint8_t mask = 0U;
            for (std::uint8_t bit = rxAlign; bit <= 7U; ++bit) {
                mask |= static_cast<std::uint8_t>(1U << bit);
            }
            values[0] = static_cast<std::uint8_t>((values[0] & ~mask) | (value & mask));
        } else {
            values[index] = value;
        }
    }
    return Status::Ok;
}

std::uint8_t Mfrc522::readRegisterValue(Register reg) const
{
    std::uint8_t value = 0U;
    (void)readRegister(reg, value);
    return value;
}

Status Mfrc522::setRegisterBitMask(Register reg, std::uint8_t mask) const
{
    std::uint8_t value = 0U;
    auto status = readRegister(reg, value);
    if (status != Status::Ok) {
        return status;
    }
    return writeRegister(reg, static_cast<std::uint8_t>(value | mask));
}

Status Mfrc522::clearRegisterBitMask(Register reg, std::uint8_t mask) const
{
    std::uint8_t value = 0U;
    auto status = readRegister(reg, value);
    if (status != Status::Ok) {
        return status;
    }
    return writeRegister(reg, static_cast<std::uint8_t>(value & static_cast<std::uint8_t>(~mask)));
}

Status Mfrc522::statusFromPicc(StatusCode status)
{
    switch (status) {
    case StatusCode::Ok:
    case StatusCode::Collision:
        return Status::Ok;
    case StatusCode::Timeout:
        return Status::Timeout;
    case StatusCode::NoRoom:
    case StatusCode::Invalid:
        return Status::Param;
    case StatusCode::Idle:
    case StatusCode::Error:
    case StatusCode::InternalError:
    case StatusCode::CrcWrong:
    case StatusCode::MifareNack:
    default:
        return Status::Error;
    }
}

Mfrc522::StatusCode Mfrc522::calculateCrc(std::span<const std::uint8_t> data, std::span<std::uint8_t, 2> result)
{
    if (writeRegister(Register::Command, static_cast<std::uint8_t>(Command::Idle)) != Status::Ok ||
        writeRegister(Register::DivIrq, kDivIrqCrc) != Status::Ok ||
        setRegisterBitMask(Register::FIFOLevel, kFifoFlush) != Status::Ok ||
        writeRegister(Register::FIFOData, data) != Status::Ok ||
        writeRegister(Register::Command, static_cast<std::uint8_t>(Command::CalcCRC)) != Status::Ok) {
        return StatusCode::InternalError;
    }

    for (std::uint32_t i = 0U; i < kCrcLoopCount; ++i) {
        if ((readRegisterValue(Register::DivIrq) & kDivIrqCrc) != 0U) {
            (void)writeRegister(Register::Command, static_cast<std::uint8_t>(Command::Idle));
            result[0] = readRegisterValue(Register::CRCResultL);
            result[1] = readRegisterValue(Register::CRCResultH);
            return StatusCode::Ok;
        }
    }
    return StatusCode::Timeout;
}

void Mfrc522::reset()
{
    (void)writeRegister(Register::Command, static_cast<std::uint8_t>(Command::SoftReset));
    delay(kResetDelayMs);
    while ((readRegisterValue(Register::Command) & (1U << 4U)) != 0U) {
    }
}

void Mfrc522::antennaOn()
{
    const auto value = readRegisterValue(Register::TxControl);
    if ((value & kTxControlAntennaOn) != kTxControlAntennaOn) {
        (void)writeRegister(Register::TxControl, static_cast<std::uint8_t>(value | kTxControlAntennaOn));
    }
}

Mfrc522::StatusCode Mfrc522::transceiveData(std::span<const std::uint8_t> sendData,
                                            std::span<std::uint8_t> backData,
                                            std::uint8_t &backLen,
                                            std::uint8_t *validBits,
                                            std::uint8_t rxAlign,
                                            bool checkCrc)
{
    return communicateWithPicc(Command::Transceive, kComIrqRxIdle, sendData, backData, backLen, validBits, rxAlign, checkCrc);
}

Mfrc522::StatusCode Mfrc522::communicateWithPicc(Command command,
                                                 std::uint8_t waitIrq,
                                                 std::span<const std::uint8_t> sendData,
                                                 std::span<std::uint8_t> backData,
                                                 std::uint8_t &backLen,
                                                 std::uint8_t *validBits,
                                                 std::uint8_t rxAlign,
                                                 bool checkCrc)
{
    const std::uint8_t txLastBits = (validBits != nullptr) ? *validBits : 0U;
    const std::uint8_t bitFraming = static_cast<std::uint8_t>((rxAlign << 4U) | (txLastBits & kBitFramingMask));

    if (writeRegister(Register::Command, static_cast<std::uint8_t>(Command::Idle)) != Status::Ok ||
        writeRegister(Register::ComIrq, 0x7FU) != Status::Ok ||
        setRegisterBitMask(Register::FIFOLevel, kFifoFlush) != Status::Ok ||
        writeRegister(Register::FIFOData, sendData) != Status::Ok ||
        writeRegister(Register::BitFraming, bitFraming) != Status::Ok ||
        writeRegister(Register::Command, static_cast<std::uint8_t>(command)) != Status::Ok) {
        return StatusCode::InternalError;
    }
    if (command == Command::Transceive && setRegisterBitMask(Register::BitFraming, kStartSend) != Status::Ok) {
        return StatusCode::InternalError;
    }

    bool irqTriggered = false;
    for (std::uint32_t i = 0U; i < kCommandLoopCount; ++i) {
        const std::uint8_t irq = readRegisterValue(Register::ComIrq);
        if ((irq & waitIrq) != 0U) {
            irqTriggered = true;
            break;
        }
        if ((irq & 0x01U) != 0U) {
            return StatusCode::Timeout;
        }
    }
    if (!irqTriggered) {
        return StatusCode::Timeout;
    }

    const auto error = readRegisterValue(Register::Error);
    if ((error & kErrorMaskBufferOverflowParityProtocol) != 0U) {
        return StatusCode::Error;
    }

    std::uint8_t valid = 0U;
    if (!backData.empty()) {
        const auto fifoLevel = readRegisterValue(Register::FIFOLevel);
        if (fifoLevel > backData.size()) {
            return StatusCode::NoRoom;
        }
        backLen = fifoLevel;
        if (readRegister(Register::FIFOData, backData.subspan(0U, backLen), rxAlign) != Status::Ok) {
            return StatusCode::InternalError;
        }
        valid = static_cast<std::uint8_t>(readRegisterValue(Register::Control) & kBitFramingMask);
        if (validBits != nullptr) {
            *validBits = valid;
        }
    }

    if ((error & kErrorMaskCollision) != 0U) {
        return StatusCode::Collision;
    }

    if (backData.empty() || !checkCrc) {
        return StatusCode::Ok;
    }
    if (backLen < 2U || valid != 0U) {
        return StatusCode::CrcWrong;
    }

    std::array<std::uint8_t, 2> controlBuffer{};
    const auto payload = std::span<const std::uint8_t>(backData.data(), backLen - 2U);
    if (calculateCrc(payload, controlBuffer) != StatusCode::Ok) {
        return StatusCode::CrcWrong;
    }
    if (backData[backLen - 2U] != controlBuffer[0] || backData[backLen - 1U] != controlBuffer[1]) {
        return StatusCode::CrcWrong;
    }

    return StatusCode::Ok;
}

Mfrc522::StatusCode Mfrc522::requestA(std::span<std::uint8_t, 2> bufferAtqa, std::uint8_t &bufferSize)
{
    if (bufferAtqa.size() != 2U || bufferSize != 2U) {
        lastAtqa_.fill(0U);
        lastRequestStatus_ = StatusCode::NoRoom;
        return StatusCode::NoRoom;
    }

    lastAtqa_.fill(0U);
    (void)clearRegisterBitMask(Register::Status2, kStatus2MFCrypto1On);
    (void)clearRegisterBitMask(Register::Coll, kCollValuesAfterColl);
    std::array<std::uint8_t, 1> command = {static_cast<std::uint8_t>(PiccCommand::Wupa)};
    std::uint8_t validBits = 7U;
    auto result = transceiveData(command, bufferAtqa, bufferSize, &validBits, 0U, false);
    if (result == StatusCode::Ok && (bufferSize != 2U || validBits != 0U)) {
        result = StatusCode::Error;
        bufferSize = 0U;
    }
    if (bufferSize > 0U) {
        lastAtqa_[0] = bufferAtqa[0];
    }
    if (bufferSize > 1U) {
        lastAtqa_[1] = bufferAtqa[1];
    }
    lastRequestStatus_ = result;
    return result;
}

Mfrc522::StatusCode Mfrc522::anticoll(Uid &uid)
{
    (void)clearRegisterBitMask(Register::Status2, kStatus2MFCrypto1On);
    (void)writeRegister(Register::BitFraming, 0x00U);
    (void)clearRegisterBitMask(Register::Coll, kCollValuesAfterColl);

    std::array<std::uint8_t, kAnticollFrameSize> command = {
        static_cast<std::uint8_t>(PiccCommand::SelCl1),
        0x20U,
    };
    std::array<std::uint8_t, kAnticollResponseSize> response{};
    std::uint8_t responseLength = static_cast<std::uint8_t>(response.size());
    const auto result = transceiveData(command, response, responseLength, nullptr, 0U, false);
    (void)setRegisterBitMask(Register::Coll, kCollValuesAfterColl);
    if (result != StatusCode::Ok) {
        return result;
    }
    if (responseLength != response.size()) {
        return StatusCode::Error;
    }

    std::uint8_t bcc = 0U;
    for (std::uint8_t i = 0U; i < kUidSingleSize; ++i) {
        uid.bytes[i] = response[i];
        bcc ^= response[i];
    }
    if (bcc != response[kUidSingleSize]) {
        return StatusCode::CrcWrong;
    }

    uid.size = kUidSingleSize;
    return StatusCode::Ok;
}

Mfrc522::StatusCode Mfrc522::select(Uid &uid, std::uint8_t validBits)
{
    if (validBits != 0U) {
        return StatusCode::Invalid;
    }

    uid.size = 0U;
    uid.sak = 0U;

    const auto anticollStatus = anticoll(uid);
    if (anticollStatus != StatusCode::Ok) {
        return anticollStatus;
    }

    std::array<std::uint8_t, kSelectFrameSize> command = {
        static_cast<std::uint8_t>(PiccCommand::SelCl1),
        0x70U,
        uid.bytes[0],
        uid.bytes[1],
        uid.bytes[2],
        uid.bytes[3],
        static_cast<std::uint8_t>(uid.bytes[0] ^ uid.bytes[1] ^ uid.bytes[2] ^ uid.bytes[3]),
        0x00U,
        0x00U,
    };
    std::span<std::uint8_t, 2> crc(command.data() + 7U, 2U);
    if (calculateCrc(std::span<const std::uint8_t>(command.data(), 7U), crc) != StatusCode::Ok) {
        return StatusCode::Error;
    }

    (void)clearRegisterBitMask(Register::Status2, kStatus2MFCrypto1On);
    std::array<std::uint8_t, kSelectResponseSize> response{};
    std::uint8_t responseLength = static_cast<std::uint8_t>(response.size());
    const auto result = transceiveData(command, response, responseLength, nullptr, 0U, true);
    if (result != StatusCode::Ok) {
        return result;
    }
    if (responseLength != response.size()) {
        return StatusCode::Error;
    }

    uid.sak = response[0];
    if ((uid.sak & kSelectCascadeBit) != 0U) {
        return StatusCode::Invalid;
    }
    return StatusCode::Ok;
}
