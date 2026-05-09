#include "ClightSelfTest.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "Common.hpp"
#include "Adc.hpp"
#include "Can.hpp"
#include "Crc.hpp"
#include "Dac.hpp"
#include "Dma.hpp"
#include "Encoder.hpp"
#include "Eth.hpp"
#include "Gpio.hpp"
#include "I2c.hpp"
#include "I2s.hpp"
#include "Mmc.hpp"
#include "Ospi.hpp"
#include "Pwm.hpp"
#include "Qspi.hpp"
#include "Rng.hpp"
#include "Rtc.hpp"
#include "Sai.hpp"
#include "Sd.hpp"
#include "Sdio.hpp"
#include "Spi.hpp"
#include "Timer.hpp"
#include "Uart.hpp"
#include "Watchdog.hpp"

#if defined(BSP_CHIP_STM32H723VGTX)
#include "Bmi088.hpp"
#include "Buzzer.hpp"
#include "Comp.hpp"
#include "Cordic.hpp"
#include "Dcmi.hpp"
#include "Dfsdm.hpp"
#include "Dma2d.hpp"
#include "Fmac.hpp"
#include "Hcd.hpp"
#include "Jpeg.hpp"
#include "Ltdc.hpp"
#include "Mdma.hpp"
#include "Opamp.hpp"
#include "Pcd.hpp"
#include "Pssi.hpp"
#include "Smartcard.hpp"
#include "Smbus.hpp"
#include "Usart.hpp"
#include "Wwdg.hpp"
#include "Ws2812.hpp"
#endif

#if defined(BSP_CHIP_HPM5E31)
#include "AnaCmpr.hpp"
#include "Cache.hpp"
#include "Ecat.hpp"
#include "EncoderOutput.hpp"
#include "Eui.hpp"
#include "Lobs.hpp"
#include "Mailbox.hpp"
#include "Mchtmr.hpp"
#include "Owr.hpp"
#include "Pdgo.hpp"
#include "Plb.hpp"
#include "Plic.hpp"
#include "PllCtl.hpp"
#include "Ppi.hpp"
#include "Ppor.hpp"
#include "Ptpc.hpp"
#include "Sdm.hpp"
#include "Synt.hpp"
#include "Trgm.hpp"
#include "Tsns.hpp"
#endif

extern "C" {
#include "bsp/interface/bsp_crc.h"
#include "bsp/interface/bsp_dac.h"
#include "bsp/interface/bsp_i2c.h"
#include "bsp/interface/bsp_i2s.h"
#include "bsp/interface/bsp_mmc.h"
#include "bsp/interface/bsp_ospi.h"
#include "bsp/interface/bsp_qspi.h"
#include "bsp/interface/bsp_rng.h"
#include "bsp/interface/bsp_rtc.h"
#include "bsp/interface/bsp_sai.h"
#include "bsp/interface/bsp_sd.h"
#include "bsp/interface/bsp_sdio.h"
#include "bsp/interface/bsp_spi.h"
#include "bsp/interface/bsp_uart.h"
}

namespace clight {
namespace {

constexpr std::uint32_t kExpectedCrc32 = 0xCBF43926UL;

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

class SelfTestRunner {
public:
    SelfTestRunner(ClightSelfTestWriteFn write, void *arg, const ClightSelfTestOptions &options)
        : write_(write), arg_(arg), options_(options)
    {
    }

    void line(const char *text)
    {
        if (write_ != nullptr) {
            write_(text, arg_);
        }
    }

    void reportOk(const char *name)
    {
        summary_.passed++;
        std::snprintf(buffer_, sizeof(buffer_), "[OK] %s\r\n", name);
        line(buffer_);
    }

    void reportOkStatus(const char *name, Status status)
    {
        summary_.passed++;
        std::snprintf(buffer_,
                      sizeof(buffer_),
                      "[OK] %s status=%ld (%s)\r\n",
                      name,
                      static_cast<long>(static_cast<std::int32_t>(status)),
                      statusName(status));
        line(buffer_);
    }

    void reportUnsupported(const char *name, Status status)
    {
        summary_.unsupported++;
        std::snprintf(buffer_,
                      sizeof(buffer_),
                      "[UNSUPPORTED] %s status=%ld (%s)\r\n",
                      name,
                      static_cast<long>(static_cast<std::int32_t>(status)),
                      statusName(status));
        line(buffer_);
    }

    void reportFail(const char *name, Status status, const char *expected)
    {
        summary_.failed++;
        std::snprintf(buffer_,
                      sizeof(buffer_),
                      "[FAIL] %s status=%ld (%s) expected=%s\r\n",
                      name,
                      static_cast<long>(static_cast<std::int32_t>(status)),
                      statusName(status),
                      expected);
        line(buffer_);
    }

    void expectOk(const char *name, Status status)
    {
        if (status == Status::Ok) {
            reportOkStatus(name, status);
        } else {
            reportFail(name, status, "ok");
        }
    }

    void expectParam(const char *name, Status status)
    {
        if (status == Status::Param) {
            reportOkStatus(name, status);
        } else {
            reportFail(name, status, "param");
        }
    }

    void expectUnsupported(const char *name, Status status)
    {
        if (status == Status::Unsupported) {
            reportUnsupported(name, status);
        } else {
            reportFail(name, status, "unsupported");
        }
    }

    void expectRejected(const char *name, Status status)
    {
        if (status == Status::Ok) {
            reportFail(name, status, "not ok");
        } else if (status == Status::Unsupported) {
            reportUnsupported(name, status);
        } else {
            reportOkStatus(name, status);
        }
    }

    void expectBool(const char *name, bool ok)
    {
        if (ok) {
            reportOk(name);
        } else {
            reportFail(name, Status::Error, "true");
        }
    }

    const ClightSelfTestOptions &options() const
    {
        return options_;
    }

    ClightSelfTestSummary summary() const
    {
        return summary_;
    }

private:
    ClightSelfTestWriteFn write_;
    void *arg_;
    ClightSelfTestOptions options_;
    ClightSelfTestSummary summary_{};
    char buffer_[192]{};
};

Status statusFromBsp(int32_t status)
{
    return statusFromInternal(static_cast<int>(status));
}

void runCommonApiTests(SelfTestRunner &runner)
{
    const std::array<std::uint8_t, 9> crcData = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};

    Crc crc(CrcId::Main);
    std::uint32_t crcValue = 0U;
    Status status = crc.calculate(ByteView(crcData.data(), crcData.size()), crcValue);
    if ((status == Status::Ok) && (crcValue == kExpectedCrc32)) {
        runner.reportOk("crc32 api->bsp value");
    } else {
        runner.reportFail("crc32 api->bsp value", status, "ok + 0xCBF43926");
    }

    status = statusFromBsp(bsp_crc32_calculate(BSP_CRC_MAIN, crcData.data(), static_cast<std::uint32_t>(crcData.size()), &crcValue));
    if ((status == Status::Ok) && (crcValue == kExpectedCrc32)) {
        runner.reportOk("crc32 bsp direct value");
    } else {
        runner.reportFail("crc32 bsp direct value", status, "ok + 0xCBF43926");
    }

    runner.expectParam("crc32 bsp null data rejected",
                       statusFromBsp(bsp_crc32_calculate(BSP_CRC_MAIN, nullptr, static_cast<std::uint32_t>(crcData.size()), &crcValue)));
}

void runCommonCompileCoverage(SelfTestRunner &runner)
{
    (void) sizeof(Adc);
    (void) sizeof(Can);
    (void) sizeof(Crc);
    (void) sizeof(Dac);
    (void) sizeof(Dma);
    (void) sizeof(Encoder);
    (void) sizeof(Eth);
    (void) sizeof(Gpio);
    (void) sizeof(I2c);
    (void) sizeof(I2s);
    (void) sizeof(Mmc);
    (void) sizeof(Ospi);
    (void) sizeof(Pwm);
    (void) sizeof(Qspi);
    (void) sizeof(Rng);
    (void) sizeof(Rtc);
    (void) sizeof(Sai);
    (void) sizeof(Sd);
    (void) sizeof(Sdio);
    (void) sizeof(Spi);
    (void) sizeof(Timer);
    (void) sizeof(Uart);
    (void) sizeof(Watchdog);
#if defined(BSP_CHIP_STM32H723VGTX)
    (void) sizeof(Comp);
    (void) sizeof(Cordic);
    (void) sizeof(Dcmi);
    (void) sizeof(Dfsdm);
    (void) sizeof(Dma2d);
    (void) sizeof(Fmac);
    (void) sizeof(Hcd);
    (void) sizeof(Jpeg);
    (void) sizeof(Ltdc);
    (void) sizeof(Mdma);
    (void) sizeof(Opamp);
    (void) sizeof(Pcd);
    (void) sizeof(Pssi);
    (void) sizeof(Smartcard);
    (void) sizeof(Smbus);
    (void) sizeof(Usart);
    (void) sizeof(Wwdg);
#elif defined(BSP_CHIP_HPM5E31)
    (void) sizeof(AnaCmpr);
    (void) sizeof(Cache);
    (void) sizeof(Ecat);
    (void) sizeof(EncoderOutput);
    (void) sizeof(Eui);
    (void) sizeof(Lobs);
    (void) sizeof(Mailbox);
    (void) sizeof(Mchtmr);
    (void) sizeof(Owr);
    (void) sizeof(Pdgo);
    (void) sizeof(Plb);
    (void) sizeof(Plic);
    (void) sizeof(PllCtl);
    (void) sizeof(Ppi);
    (void) sizeof(Ppor);
    (void) sizeof(Ptpc);
    (void) sizeof(Sdm);
    (void) sizeof(Synt);
    (void) sizeof(Trgm);
    (void) sizeof(Tsns);
#endif
    runner.reportOk("core api headers compile coverage");
}

#if defined(BSP_CHIP_STM32H723VGTX)

bool canFrameMatches(const CanFrame &frame, std::uint32_t id, std::uint8_t byte0, std::uint8_t byte1)
{
    return (frame.id == id) &&
           (frame.dlc == 2U) &&
           (frame.data[0] == byte0) &&
           (frame.data[1] == byte1);
}

Status waitCanFrame(Can &can, CanFrame &frame)
{
    for (std::uint32_t attempt = 0U; attempt < 100000U; ++attempt) {
        const auto status = can.read(frame);
        if (status == Status::Ok) {
            return Status::Ok;
        }
    }
    return Status::Timeout;
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

void runStm32ApiTests(SelfTestRunner &runner)
{
    Gpio accelCs(GpioId::AccelCs);
    runner.expectOk("gpio api accel cs output", accelCs.initOutput());
    accelCs.write(true);

    Uart uart(UartId::Usart1);
    runner.expectOk("uart api init usart1", uart.init(115200U));
    runner.expectOk("uart api write usart1", uart.write("clight self-test uart api\r\n"));

    std::uint8_t uartText[] = "clight self-test uart bsp\r\n";
    runner.expectOk("uart bsp direct send",
                    statusFromBsp(bsp_uart_send(BSP_UART_DEBUG, uartText, static_cast<std::uint16_t>(std::strlen(reinterpret_cast<char *>(uartText))), 0U)));
    runner.expectParam("uart bsp null buffer rejected",
                       statusFromBsp(bsp_uart_send(BSP_UART_DEBUG, nullptr, 4U, 0U)));

    Rng rng(RngId::Main);
    runner.expectOk("rng api init", rng.init());
    std::uint32_t rngValue = 0U;
    runner.expectOk("rng api read", rng.read(rngValue));
    runner.expectParam("rng bsp null output rejected", statusFromBsp(bsp_rng_get_u32(BSP_RNG_MAIN, nullptr)));

    Rtc rtc(RtcId::Main);
    runner.expectOk("rtc api init", rtc.init());
    std::uint32_t unixSeconds = 0U;
    runner.expectOk("rtc api now", rtc.now(unixSeconds));
    runner.expectParam("rtc bsp null output rejected", statusFromBsp(bsp_rtc_get_unix_time(BSP_RTC_MAIN, nullptr)));

    Timer timer(TimerId::Tim12);
    runner.expectOk("timer api tim12 periodic init", timer.init(TimerMode::Periodic, 50000U));
    runner.expectOk("timer api tim12 start", timer.start());
    std::uint32_t timerCounter = 0U;
    runner.expectOk("timer api tim12 read", timer.read(timerCounter));
    runner.expectOk("timer api tim12 stop", timer.stop());

    Pwm buzzerPwm(PwmId::Buzzer);
    Buzzer buzzer(buzzerPwm, 0U);
    const auto buzzerInitStatus = buzzer.init();
    runner.expectOk("pwm api buzzer init", buzzerInitStatus);
    if (buzzerInitStatus == Status::Ok) {
        runner.expectOk("pwm api buzzer 2khz tone", buzzer.tone(2000U, 50.0f));
        runner.expectOk("pwm api buzzer off", buzzer.off());
    }

    Spi spi6(SpiId::Spi6);
    runner.expectOk("spi6 tx-only api init", spi6.init(SpiMode::Mode0, 2400000U, SpiBitOrder::MsbFirst, TransferMode::Blocking));
    std::array<std::uint8_t, 1> tx = {0U};
    std::array<std::uint8_t, 1> rx = {};
    runner.expectOk("spi6 tx-only api write", spi6.write(ByteView(tx.data(), tx.size())));
    runner.expectUnsupported("spi6 full-duplex api rejected", spi6.transfer(ByteView(tx.data(), tx.size()), ByteSpan(rx.data(), rx.size())));
    runner.expectUnsupported("spi6 rx-only bsp rejected", statusFromBsp(bsp_spi_transfer(BSP_SPI_AUX, nullptr, rx.data(), static_cast<std::uint32_t>(rx.size()))));
    runner.expectParam("spi bsp empty transfer rejected", statusFromBsp(bsp_spi_transfer(BSP_SPI_AUX, nullptr, nullptr, 0U)));

    std::array<Rgb888, 8> wsPixels{};
    Ws2812 ws2812(spi6, std::span<Rgb888>(wsPixels.data(), wsPixels.size()));
    const auto wsInitStatus = ws2812.init();
    runner.expectOk("ws2812 spi6 tx-only init", wsInitStatus);
    if (wsInitStatus == Status::Ok) {
        ws2812.clear();
        runner.expectOk("ws2812 spi6 clear refresh", ws2812.refresh());
        ws2812.setPixel(0U, Rgb888{32U, 0U, 0U});
        ws2812.setPixel(1U, Rgb888{0U, 32U, 0U});
        ws2812.setPixel(2U, Rgb888{0U, 0U, 32U});
        runner.expectOk("ws2812 spi6 rgb refresh", ws2812.refresh());
    }

    Spi bmiSpi(SpiId::Spi2);
    Gpio gyroCs(GpioId::GyroCs);
    Bmi088 imu(bmiSpi, accelCs, gyroCs);
    const auto imuInitStatus = imu.init();
    runner.expectOk("bmi088 spi2 init", imuInitStatus);
    if (imuInitStatus == Status::Ok) {
        runner.expectOk("bmi088 spi2 update", imu.update());
    }

    Can can1(CanId::Fdcan1);
    Can can2(CanId::Fdcan2);
    const auto can1Status = can1.initBitrate(500000U);
    const auto can2Status = can2.initBitrate(500000U);
    if ((can1Status == Status::Ok) && (can2Status == Status::Ok)) {
        runner.reportOk("fdcan1/fdcan2 500k init");
        drainCan(can1);
        drainCan(can2);

        CanFrame tx12{};
        CanFrame rx12{};
        tx12.id = 0x123U;
        tx12.dlc = 2U;
        tx12.data[0] = 0xABU;
        tx12.data[1] = 0xCDU;
        Status canStatus = can1.write(tx12);
        if (canStatus == Status::Ok) {
            canStatus = waitCanFrame(can2, rx12);
        }
        runner.expectBool("fdcan1 -> fdcan2 data", (canStatus == Status::Ok) && canFrameMatches(rx12, 0x123U, 0xABU, 0xCDU));

        CanFrame tx21{};
        CanFrame rx21{};
        tx21.id = 0x321U;
        tx21.dlc = 2U;
        tx21.data[0] = 0x55U;
        tx21.data[1] = 0xAAU;
        canStatus = can2.write(tx21);
        if (canStatus == Status::Ok) {
            canStatus = waitCanFrame(can1, rx21);
        }
        runner.expectBool("fdcan2 -> fdcan1 data", (canStatus == Status::Ok) && canFrameMatches(rx21, 0x321U, 0x55U, 0xAAU));
    } else {
        runner.reportFail("fdcan1/fdcan2 500k init", (can1Status != Status::Ok) ? can1Status : can2Status, "ok");
    }

    I2c i2c(I2cId::Main);
    runner.expectOk("i2c1 api default init", i2c.init());
    runner.expectUnsupported("i2c1 api invalid 2mhz rejected", i2c.init(2000000U));
    runner.expectParam("i2c bsp null write rejected", statusFromBsp(bsp_i2c_write(BSP_I2C_MAIN, 0x50U, nullptr, 1U)));

    runner.expectParam("dac api invalid channel rejected", Dac(DacId::Main).write(0U, 0U));
}

void runStm32UnsupportedTests(SelfTestRunner &runner)
{
    runner.expectUnsupported("dac no-pinmux api init", Dac(DacId::Main).init());
    runner.expectUnsupported("dac no-pinmux bsp init", statusFromBsp(bsp_dac_init(BSP_DAC_MAIN)));

    runner.expectUnsupported("i2s no-pinmux api init", I2s(I2sId::Main).init());
    runner.expectUnsupported("i2s no-pinmux bsp init", statusFromBsp(bsp_i2s_init(BSP_I2S_MAIN)));

    runner.expectUnsupported("sai1 no-pinmux api init", Sai(SaiId::Sai1).init());
    runner.expectUnsupported("sai1 no-pinmux bsp init", statusFromBsp(bsp_sai_init(BSP_SAI1)));

    std::array<std::uint8_t, 512> blockBuffer{};
    runner.expectUnsupported("sd no-pinmux api init", Sd(SdId::Sdmmc1).init());
    runner.expectUnsupported("sd read no-pinmux api rejected", Sd(SdId::Sdmmc1).readBlocks(0U, ByteSpan(blockBuffer.data(), blockBuffer.size()), 1U));
    runner.expectUnsupported("sdio no-pinmux api init", Sdio(SdioId::Sdio1).init());
    runner.expectUnsupported("mmc no-pinmux api init", Mmc(MmcId::Mmc1).init());

    std::array<std::uint8_t, 4> memBuffer{};
    runner.expectUnsupported("ospi no-pinmux api init", Ospi(OspiId::Ospi1).init());
    runner.expectUnsupported("ospi read no-pinmux api rejected", Ospi(OspiId::Ospi1).read(0U, ByteSpan(memBuffer.data(), memBuffer.size())));
    runner.expectUnsupported("qspi no-pinmux api init", Qspi(QspiId::Qspi1).init());
    runner.expectUnsupported("qspi read no-pinmux api rejected", Qspi(QspiId::Qspi1).read(0U, ByteSpan(memBuffer.data(), memBuffer.size())));

    runner.expectUnsupported("enet no-phy api init", Eth(EthId::Port0).init());
}

#endif

#if defined(BSP_CHIP_HPM5E31)

void runHpmApiTests(SelfTestRunner &runner)
{
    Gpio led(GpioId::Led);
    runner.expectOk("gpio api led output", led.initOutput());
    led.write(false);

    Uart uart(UartId::Debug);
    runner.expectOk("uart api init debug", uart.init(115200U));
    runner.expectOk("uart api write debug", uart.write("clight self-test uart api\n"));

    std::uint8_t uartText[] = "clight self-test uart bsp\n";
    runner.expectOk("uart bsp direct send",
                    statusFromBsp(bsp_uart_send(BSP_UART_DEBUG, uartText, static_cast<std::uint16_t>(std::strlen(reinterpret_cast<char *>(uartText))), 0U)));

    runner.expectBool("enet api base address exposed", Eth(EthId::Port0).baseAddress() != 0U);
    runner.expectBool("ecat api base address exposed", Ecat(EcatId::Port0).baseAddress() != 0U);
    runner.expectUnsupported("hpm rng absent api unsupported", Rng(RngId::None).init());
    runner.expectUnsupported("hpm rtc absent api unsupported", Rtc(RtcId::None).init());
    runner.expectUnsupported("hpm dac absent api unsupported", Dac(DacId::None).init());
}

void runHpmUnsupportedTests(SelfTestRunner &runner)
{
    runner.expectUnsupported("hpm rng absent bsp unsupported", statusFromBsp(bsp_rng_init(BSP_RNG_NONE)));
    runner.expectUnsupported("hpm rtc absent bsp unsupported", statusFromBsp(bsp_rtc_init(BSP_RTC_NONE)));
    runner.expectUnsupported("hpm dac absent bsp unsupported", statusFromBsp(bsp_dac_init(BSP_DAC_NONE)));
    runner.expectUnsupported("hpm i2s absent api unsupported", I2s(I2sId::None).init());
    runner.expectUnsupported("hpm sai absent api unsupported", Sai(SaiId::None).init());
    runner.expectUnsupported("hpm sdio absent api unsupported", Sdio(SdioId::None).init());
    runner.expectUnsupported("hpm sd absent api unsupported", Sd(SdId::None).init());
    runner.expectUnsupported("hpm mmc absent api unsupported", Mmc(MmcId::None).init());
    runner.expectUnsupported("hpm ospi absent api unsupported", Ospi(OspiId::None).init());
    runner.expectUnsupported("hpm qspi absent api unsupported", Qspi(QspiId::None).init());
}

#endif

void runWatchdogCompileCoverage(SelfTestRunner &runner, const ClightSelfTestOptions &options)
{
    if (options.enableWatchdogRuntime) {
        runner.line("[SKIP] watchdog runtime is intentionally not implemented in self-test\r\n");
    }
    runner.reportOk("watchdog api compile coverage");
}

} // namespace

ClightSelfTestSummary runClightSelfTest(ClightSelfTestWriteFn write,
                                        void *arg,
                                        const ClightSelfTestOptions &options)
{
    SelfTestRunner runner(write, arg, options);
    runner.line("Clight API/BSP self-test begin\r\n");

    if (options.runApiTests) {
        runCommonCompileCoverage(runner);
        runCommonApiTests(runner);
#if defined(BSP_CHIP_STM32H723VGTX)
        runStm32ApiTests(runner);
#elif defined(BSP_CHIP_HPM5E31)
        runHpmApiTests(runner);
#endif
        runWatchdogCompileCoverage(runner, options);
    }

    if (options.runUnsupportedTests) {
#if defined(BSP_CHIP_STM32H723VGTX)
        runStm32UnsupportedTests(runner);
#elif defined(BSP_CHIP_HPM5E31)
        runHpmUnsupportedTests(runner);
#endif
    }

    const ClightSelfTestSummary summary = runner.summary();
    char line[128]{};
    std::snprintf(line,
                  sizeof(line),
                  "Clight API/BSP self-test summary: passed=%lu failed=%lu unsupported=%lu\r\n",
                  static_cast<unsigned long>(summary.passed),
                  static_cast<unsigned long>(summary.failed),
                  static_cast<unsigned long>(summary.unsupported));
    if (write != nullptr) {
        write(line, arg);
        write("Clight API/BSP self-test end\r\n", arg);
    }
    return summary;
}

} // namespace clight
