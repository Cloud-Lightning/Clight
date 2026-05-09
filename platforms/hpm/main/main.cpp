extern "C" {
#include "board.h"
}

#include <stdio.h>
#if APP_ENABLE_TINYUSB_CDC
#include <array>
#endif
#include <cstddef>
#include "ClightSelfTest.hpp"
#include "Gpio.hpp"
#include "pinmux.h"
#if APP_ENABLE_ETHERCAT_IO
#include "EcatSscSlave.hpp"
#include "EcatSt7789Panel.hpp"
#endif
#if APP_ENABLE_LWIP_TCPECHO || APP_ENABLE_LWIP_HTTP_SERVER
#include "EnetSt7789Panel.hpp"
#endif
#if APP_ENABLE_LWIP_TCPECHO
#include "EthLwipTcpEcho.hpp"
#endif
#if !APP_ENABLE_ETHERCAT_IO && !APP_ENABLE_LWIP_TCPECHO && !APP_ENABLE_LWIP_HTTP_SERVER && !APP_ENABLE_TINYUSB_CDC
namespace {

const Gpio kLeds[] = {
    Gpio(GpioId::Led),
    Gpio(GpioId::Led2),
    Gpio(GpioId::Led3),
    Gpio(GpioId::Led4),
};

void init_leds(void)
{
    init_led_pins();

    for (std::size_t i = 0; i < (sizeof(kLeds) / sizeof(kLeds[0])); ++i) {
        const Status status = kLeds[i].initOutput();
        printf("LED%u init status=%d\n", static_cast<unsigned>(i + 1U), static_cast<int>(status));
        if (status != Status::Ok) {
            continue;
        }
        const Gpio &led = kLeds[i];
        led.write(false);
    }
}

void set_led_state(const Gpio &led, bool on)
{
    led.write(on);
}

} /* namespace */
#endif

#if APP_ENABLE_CLIGHT_SELFTEST
namespace {

void write_selftest_text(const char *text, void *arg)
{
    (void) arg;
    printf("%s", text);
}

} /* namespace */
#endif

int main(void)
{
    board_init();

#if APP_ENABLE_CLIGHT_SELFTEST
    clight::ClightSelfTestOptions selfTestOptions{};
    (void) clight::runClightSelfTest(write_selftest_text, nullptr, selfTestOptions);
    while (1) {
        board_delay_ms(1000);
    }
#endif

#if APP_ENABLE_ETHERCAT_IO && APP_ENABLE_LWIP_TCPECHO
    EthLwipTcpEcho enet;
    EnetSt7789Panel display;
    EcatSscSlave ecat;

    printf("Ethernet demo + EtherCAT demo (shared MDIO)\n");
    printf("LwIP Version: %s\n", LWIP_VERSION_STRING);

    if (enet.start() != Status::Ok) {
        printf("Ethernet demo start failed!\n");
        return 0;
    }

    if (display.init() != Status::Ok) {
        printf("ST7789 display init failed!\n");
    } else {
        printf("ST7789 display init ok\n");
    }

    printf("EtherCAT IO demo (LAN8720A)\n");

    if (ecat.start() != Status::Ok) {
        printf("EtherCAT demo start failed!\n");
        return 0;
    }

    while (ecat.running()) {
        ecat.poll();
        enet.poll();
        display.refresh(enet);
        board_delay_ms(1);
    }
    return 0;

#elif APP_ENABLE_ETHERCAT_IO && APP_ENABLE_LWIP_HTTP_SERVER
    EthLwipHttpServer enet;
    EnetSt7789Panel display;
    EcatSscSlave ecat;

    printf("Ethernet demo + EtherCAT demo (shared MDIO)\n");
    printf("LwIP Version: %s\n", LWIP_VERSION_STRING);

    if (enet.start() != Status::Ok) {
        printf("Ethernet demo start failed!\n");
        return 0;
    }

    if (display.init() != Status::Ok) {
        printf("ST7789 display init failed!\n");
    } else {
        printf("ST7789 display init ok\n");
    }

    printf("EtherCAT IO demo (LAN8720A)\n");

    if (ecat.start() != Status::Ok) {
        printf("EtherCAT demo start failed!\n");
        return 0;
    }

    while (ecat.running()) {
        ecat.poll();
        enet.poll();
        display.refresh(enet.stack());
        board_delay_ms(1);
    }
    return 0;

#elif APP_ENABLE_ETHERCAT_IO
    EcatSscSlave ecat;
    EcatSt7789Panel display;

    printf("EtherCAT IO demo (LAN8720A)\n");

    if (display.init() != Status::Ok) {
        printf("ST7789 display init failed!\n");
    } else {
        printf("ST7789 display init ok\n");
    }

    if (ecat.start() != Status::Ok) {
        printf("EtherCAT demo start failed!\n");
        return 0;
    }

    while (ecat.running()) {
        ecat.poll();
        display.refresh(ecat);
        board_delay_ms(1);
    }
    return 0;

#elif APP_ENABLE_LWIP_HTTP_SERVER
    EthLwipHttpServer enet;
    EnetSt7789Panel display;

#if defined(__ENABLE_ENET_RECEIVE_INTERRUPT) && __ENABLE_ENET_RECEIVE_INTERRUPT
    printf("This is an ethernet demo: HTTP Server (Interrupt Usage)\n");
#else
    printf("This is an ethernet demo: HTTP Server (Polling Usage)\n");
#endif
    printf("LwIP Version: %s\n", LWIP_VERSION_STRING);

    if (enet.start() == Status::Ok) {
        if (display.init() != Status::Ok) {
            printf("ST7789 display init failed!\n");
        } else {
            printf("ST7789 display init ok\n");
        }
        while (1) {
            enet.poll();
            display.refresh(enet.stack());
            board_delay_ms(1);
        }
    }

    printf("Ethernet demo start failed!\n");
    while (1) {
    }

#elif APP_ENABLE_LWIP_TCPECHO
    EthLwipTcpEcho enet;
    EnetSt7789Panel display;

#if defined(__ENABLE_ENET_RECEIVE_INTERRUPT) && __ENABLE_ENET_RECEIVE_INTERRUPT
    printf("This is an ethernet demo: TCP Echo (Interrupt Usage)\n");
#else
    printf("This is an ethernet demo: TCP Echo (Polling Usage)\n");
#endif
    printf("LwIP Version: %s\n", LWIP_VERSION_STRING);

    if (enet.start() == Status::Ok) {
        if (display.init() != Status::Ok) {
            printf("ST7789 display init failed!\n");
        } else {
            printf("ST7789 display init ok\n");
        }
        while (1) {
            enet.poll();
            display.refresh(enet);
            board_delay_ms(1);
        }
    }

    printf("Ethernet demo start failed!\n");
    while (1) {
    }
#elif APP_ENABLE_TINYUSB_CDC
    TinyUsbCdc usb;
    if (usb.init() == Status::Ok) {
        printf("TinyUSB CDC init ok\n");
    } else {
        printf("TinyUSB CDC init failed\n");
    }

    std::array<std::uint8_t, 64> usbBuffer{};
    while (1) {
        usb.poll();
        const std::uint32_t bytes = usb.read(ByteSpan(usbBuffer.data(), usbBuffer.size()));
        if (bytes > 0U) {
            (void) usb.write(ByteView(usbBuffer.data(), bytes));
        }
        board_delay_ms(1);
    }
#else
    init_leds();

    while (1) {
        for (const Gpio &led : kLeds) {
            set_led_state(led, true);
            board_delay_ms(200);
            set_led_state(led, false);
            board_delay_ms(200);
        }
    }

#endif
}
