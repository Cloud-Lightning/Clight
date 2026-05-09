#include <array>
#include <cstdio>

#include "Adc.hpp"
#include "Bmi088.hpp"
#include "Ec11.hpp"
#include "Gpio.hpp"
#include "I2c.hpp"
#include "LvglDisplay.hpp"
#include "Mfrc522.hpp"
#include "Mpu6050.hpp"
#include "Pcnt.hpp"
#include "Pwm.hpp"
#include "Rmt.hpp"
#include "SdCard.hpp"
#include "Ssd1306.hpp"
#include "Spi.hpp"
#include "St77735.hpp"
#include "St7789.hpp"
#include "Timer.hpp"
#include "Tsens.hpp"
#include "Uart.hpp"
#include "UsbSerialJtag.hpp"
#include "Ws2812.hpp"

#include "BleService.hpp"
#include "HttpServerService.hpp"
#include "WifiService.hpp"

namespace {

const char *status_name(Status status)
{
    switch (status) {
    case Status::Ok:
        return "ok";
    case Status::Param:
        return "param";
    case Status::Timeout:
        return "timeout";
    case Status::Busy:
        return "busy";
    case Status::Unsupported:
        return "unsupported";
    case Status::Error:
    default:
        return "error";
    }
}

void report(const char *name, Status status, bool unsupported_ok = false)
{
    const bool pass = (status == Status::Ok) || (unsupported_ok && status == Status::Unsupported);
    const char *tag = (status == Status::Unsupported) ? "UNSUPPORTED" : (pass ? "OK" : "FAIL");
    std::printf("[%s] %s status=%ld (%s)\n", tag, name, static_cast<long>(status), status_name(status));
}

} // namespace

extern "C" void app_main(void)
{
    Gpio led(GpioId::Gpio2);
    Gpio key(GpioId::Gpio0);
    Timer timer(TimerId::Gptimer0);
    Pwm pwm(PwmId::Ledc0);
    Adc adc(AdcId::Adc1Ch0);
    Uart uart(UartId::Uart0);
    Spi spi(SpiId::Spi2);
    I2c i2c(I2cId::I2c0);
    Tsens tsens(TsensId::Tsens);
    UsbSerialJtag usb(UsbSerialJtagId::UsbSerialJtag);
    Rmt rmt(RmtId::Rmt0);
    Pcnt pcnt(PcntId::Pcnt0);
    WifiService wifi;
    HttpServerService http;
    BleService ble;

    std::array<Rgb888, 8> pixels = {};
    Bmi088 bmi088(spi, led, key);
    Mpu6050 mpu6050(i2c);
    Ssd1306 ssd1306(i2c, 0x3CU, &led);
    St7789 st7789(spi, led, key);
    St77735 st77735(spi, led, key);
    Ec11 ec11(led, key, key, 4);
    Mfrc522 mfrc522(spi, led, &key);
    Ws2812 ws2812(spi, std::span<Rgb888>(pixels.data(), pixels.size()));
    SdCard sdcard(spi, led);
    LvglDisplay lvgl(st7789);

    static_cast<void>(bmi088);
    static_cast<void>(mpu6050);
    static_cast<void>(ssd1306);
    static_cast<void>(st77735);
    static_cast<void>(ec11);
    static_cast<void>(mfrc522);
    static_cast<void>(ws2812);
    static_cast<void>(sdcard);
    static_cast<void>(lvgl);

    led.initOutput();
    led.write(false);
    timer.init(TimerMode::Periodic, 1000U);
    pwm.init(1000U, PwmAlign::Edge);
    adc.init();
    uart.init();
    spi.init();
    i2c.init();
    tsens.init();
    usb.init();
    rmt.init();
    pcnt.init();

    std::puts("Clight ESP32-S3 API/BSP/service self-test begin");
    report("gpio led init", led.initOutput());
    report("gpio key init", key.initInput());
    report("timer gptimer0 init", timer.init(TimerMode::Periodic, 1000U));
    report("pwm ledc0 init", pwm.init(1000U, PwmAlign::Edge));
    report("adc1 ch0 init", adc.init());
    report("uart0 init", uart.init());
    report("spi2 init", spi.init());
    report("i2c0 init", i2c.init());
    report("temperature sensor init", tsens.init(), true);
    report("usb serial jtag init", usb.init(), true);
    report("rmt init", rmt.init(), true);
    report("pcnt init", pcnt.init(), true);

    const auto wifi_init = wifi.init();
    report("wifi service init", wifi_init);
    if (wifi_init == Status::Ok) {
        const auto ap_status = wifi.startAp({.ssid = "Clight-ESP32", .password = "clight1234", .channel = 6U, .maxConnections = 4U, .hidden = false});
        report("wifi ap start", ap_status);
        if (ap_status == Status::Ok) {
            const auto wifi_status = wifi.status();
            std::printf("[OK] wifi ap ssid=Clight-ESP32 ip=%s\n", wifi_status.ip);
        }
    }

    report("http server start", http.start(80U), true);
    report("ble service init", ble.init(), true);

    std::puts("Clight ESP32-S3 API/BSP/service self-test end");
}
