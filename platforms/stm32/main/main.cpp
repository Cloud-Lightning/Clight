#include <array>
#include <cstdio>

#include "Bmi088.hpp"
#include "Buzzer.hpp"
#include "Can.hpp"
#include "ClightSelfTest.hpp"
#include "Gpio.hpp"
#include "Pwm.hpp"
#include "Spi.hpp"
#include "Uart.hpp"
#include "Ws2812.hpp"
#if STM32_ENABLE_TINYUSB_CDC
#include "TinyUsbCdc.hpp"
#endif

extern "C" {
#include "main.h"

void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
}

namespace {

void writeText(Uart &uart, const char *text)
{
    (void) uart.write(text);
}

void writeSelfTestText(const char *text, void *arg)
{
    if (arg == nullptr) {
        return;
    }

    auto *uart = static_cast<Uart *>(arg);
    writeText(*uart, text);
}

const char *statusName(Status status)
{
    switch (status) {
    case Status::Ok:
        return "ok";
    case Status::Error:
        return "error";
    case Status::Param:
        return "param";
    case Status::Timeout:
        return "timeout";
    case Status::Busy:
        return "busy";
    case Status::Unsupported:
        return "unsupported";
    default:
        return "unknown";
    }
}

void writeStatus(Uart &uart, const char *label, Status status)
{
    char line[96]{};
    std::snprintf(line,
                  sizeof(line),
                  "%s %s (%ld)\r\n",
                  label,
                  statusName(status),
                  static_cast<long>(static_cast<std::int32_t>(status)));
    writeText(uart, line);
}

Status waitCanFrame(Can &can, CanFrame &frame, std::uint32_t timeoutMs)
{
    const std::uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeoutMs) {
        const auto status = can.read(frame);
        if (status == Status::Ok) {
            return Status::Ok;
        }
        HAL_Delay(1);
    }
    return Status::Timeout;
}

bool frameMatches(const CanFrame &frame, std::uint32_t id, std::uint8_t byte0, std::uint8_t byte1)
{
    return (frame.id == id) &&
           (frame.dlc == 2U) &&
           (frame.data[0] == byte0) &&
           (frame.data[1] == byte1);
}

void drainCan(Can &can)
{
    CanFrame dropped{};
    for (std::uint32_t i = 0U; i < 8U; ++i) {
        if (can.read(dropped) != Status::Ok) {
            break;
        }
    }
}

std::int32_t scaled(float value, float scale)
{
    return static_cast<std::int32_t>(value * scale);
}

void writeImuSample(Uart &uart, const Bmi088 &imu)
{
    const auto accel = imu.accel();
    const auto gyro = imu.gyro();
    char line[192]{};
    std::snprintf(line,
                  sizeof(line),
                  "bmi088 data accel_mps2_x1000 ax=%ld ay=%ld az=%ld gyro_dps_x1000 gx=%ld gy=%ld gz=%ld temp_c_x100=%ld\r\n",
                  static_cast<long>(scaled(accel.x, 1000.0f)),
                  static_cast<long>(scaled(accel.y, 1000.0f)),
                  static_cast<long>(scaled(accel.z, 1000.0f)),
                  static_cast<long>(scaled(gyro.x, 1000.0f)),
                  static_cast<long>(scaled(gyro.y, 1000.0f)),
                  static_cast<long>(scaled(gyro.z, 1000.0f)),
                  static_cast<long>(scaled(imu.temperature(), 100.0f)));
    writeText(uart, line);
}

} // namespace

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    PeriphCommonClock_Config();

    Gpio accelCs(GpioId::AccelCs);
    Gpio gyroCs(GpioId::GyroCs);
    Gpio accelInt(GpioId::AccelInt);
    Gpio gyroInt(GpioId::GyroInt);
    (void) accelInt.initInput();
    (void) gyroInt.initInput();

    Uart debug(UartId::Usart1);
    (void) debug.init(115200U);
    writeText(debug, "stm32h723 cubemx bsp api hardware self-test\r\n");
    writeText(debug, "map: BMI088=SPI2, WS2812=SPI6, BUZZER=TIM12_CH2/PB15, FDCAN=FDCAN1+FDCAN2\r\n");

    clight::ClightSelfTestOptions selfTestOptions{};
    (void) clight::runClightSelfTest(writeSelfTestText, &debug, selfTestOptions);

#if STM32_ENABLE_TINYUSB_CDC
    TinyUsbCdc usb;
    if (usb.init() == Status::Ok) {
        writeText(debug, "tinyusb cdc init ok\r\n");
    } else {
        writeText(debug, "tinyusb cdc init failed\r\n");
    }
#endif

    Pwm buzzerPwm(PwmId::Buzzer);
    Buzzer buzzer(buzzerPwm, 0U);
    const auto buzzer_init_status = buzzer.init();
    writeStatus(debug, "buzzer tim12_ch2 init", buzzer_init_status);
    if (buzzer_init_status == Status::Ok) {
        const auto tone_status = buzzer.tone(2000U, 50.0f);
        writeStatus(debug, "buzzer tim12_ch2 2khz tone", tone_status);
        HAL_Delay(120);
        writeStatus(debug, "buzzer tim12_ch2 off", buzzer.off());
    }

    Spi wsSpi(SpiId::Spi6);
    std::array<Rgb888, 8> wsPixels{};
    Ws2812 ws2812(wsSpi, std::span<Rgb888>(wsPixels.data(), wsPixels.size()));
    const auto ws_init_status = ws2812.init();
    writeStatus(debug, "ws2812 spi6 init", ws_init_status);
    Status ws_refresh_status = Status::Unsupported;
    if (ws_init_status == Status::Ok) {
        ws2812.clear();
        ws_refresh_status = ws2812.refresh();
        writeStatus(debug, "ws2812 spi6 clear refresh", ws_refresh_status);
        ws2812.setPixel(0U, Rgb888{32U, 0U, 0U});
        ws2812.setPixel(1U, Rgb888{0U, 32U, 0U});
        ws2812.setPixel(2U, Rgb888{0U, 0U, 32U});
        ws_refresh_status = ws2812.refresh();
        writeStatus(debug, "ws2812 spi6 rgb refresh", ws_refresh_status);
    }

    Spi bmiSpi(SpiId::Spi2);
    Bmi088 imu(bmiSpi, accelCs, gyroCs);
    const auto imu_status = imu.init();
    writeStatus(debug, "bmi088 spi2 init", imu_status);
    if (imu_status == Status::Ok) {
        const auto update_status = imu.update();
        writeStatus(debug, "bmi088 spi2 first update", update_status);
        if (update_status == Status::Ok) {
            writeImuSample(debug, imu);
        }
    }

    Can can1(CanId::Fdcan1);
    Can can2(CanId::Fdcan2);
    const auto can1_status = can1.initBitrate(500000U);
    const auto can2_status = can2.initBitrate(500000U);
    bool can_ok = false;
    if (can1_status == Status::Ok && can2_status == Status::Ok) {
        writeText(debug, "fdcan1/fdcan2 500k init ok\r\n");

        drainCan(can1);
        drainCan(can2);

        CanFrame tx12{};
        CanFrame rx12{};
        tx12.id = 0x123U;
        tx12.dlc = 2U;
        tx12.data[0] = 0xABU;
        tx12.data[1] = 0xCDU;
        if (can1.write(tx12) == Status::Ok &&
            waitCanFrame(can2, rx12, 100U) == Status::Ok &&
            frameMatches(rx12, 0x123U, 0xABU, 0xCDU)) {
            writeText(debug, "fdcan1 -> fdcan2 ok\r\n");
        } else {
            writeText(debug, "fdcan1 -> fdcan2 failed\r\n");
        }

        CanFrame tx21{};
        CanFrame rx21{};
        tx21.id = 0x321U;
        tx21.dlc = 2U;
        tx21.data[0] = 0x55U;
        tx21.data[1] = 0xAAU;
        if (can2.write(tx21) == Status::Ok &&
            waitCanFrame(can1, rx21, 100U) == Status::Ok &&
            frameMatches(rx21, 0x321U, 0x55U, 0xAAU)) {
            writeText(debug, "fdcan2 -> fdcan1 ok\r\n");
            can_ok = true;
        } else {
            writeText(debug, "fdcan2 -> fdcan1 failed\r\n");
        }
    } else {
        writeText(debug, "fdcan1/fdcan2 init failed\r\n");
    }

    std::uint32_t lastReportMs = HAL_GetTick();
    std::uint32_t wsStep = 0U;
#if STM32_ENABLE_TINYUSB_CDC
    std::array<std::uint8_t, 64> usbBuffer{};
#endif

    while (1) {
#if STM32_ENABLE_TINYUSB_CDC
        usb.poll();
        const std::uint32_t usbBytes = usb.read(ByteSpan(usbBuffer.data(), usbBuffer.size()));
        if (usbBytes > 0U) {
            (void) usb.write(ByteView(usbBuffer.data(), usbBytes));
        }
#endif

        const std::uint32_t now = HAL_GetTick();
        if ((now - lastReportMs) >= 1000U) {
            lastReportMs = now;

            if (imu_status == Status::Ok) {
                const auto update_status = imu.update();
                writeStatus(debug, "bmi088 spi2 update", update_status);
                if (update_status == Status::Ok) {
                    writeImuSample(debug, imu);
                }
            } else {
                writeText(debug, "bmi088 spi2 not ready\r\n");
            }

            if (ws_init_status == Status::Ok) {
                ws2812.clear();
                const std::size_t index = wsStep % ws2812.size();
                const Rgb888 color = (wsStep % 3U) == 0U ? Rgb888{24U, 0U, 0U} :
                                     (wsStep % 3U) == 1U ? Rgb888{0U, 24U, 0U} :
                                                           Rgb888{0U, 0U, 24U};
                ws2812.setPixel(index, color);
                ws_refresh_status = ws2812.refresh();
                writeStatus(debug, "ws2812 spi6 refresh", ws_refresh_status);
                wsStep++;
            } else {
                writeText(debug, "ws2812 spi6 not ready\r\n");
            }

            writeText(debug, can_ok ? "fdcan self-test ok\r\n" : "fdcan self-test failed\r\n");
            writeText(debug, "systick alive\r\n");
        }
#if STM32_ENABLE_TINYUSB_CDC
        HAL_Delay(1);
#else
        HAL_Delay(100);
#endif
    }
}
