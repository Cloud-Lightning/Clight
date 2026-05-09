# UART 事件回调说明

这次给 `UART` 补了一层统一事件回调，目的很简单：

- 以前只能分开用
  - `onReceive(...)`
  - `onTransmit(...)`
- 现在多了一层
  - `onEvent(...)`

这样你就能在一个回调里区分：

- `RxReady`
- `RxDone`
- `RxHalf`
- `RxIdle`
- `RxTimeout`
- `TxDone`
- `TxHalf`
- `Error`

## 1. 现在怎么用

```cpp
static void on_uart_event(bsp_uart_event_t event,
                          std::uint8_t *data,
                          std::uint16_t len,
                          std::int32_t status,
                          void *arg)
{
    (void)arg;

    switch (event) {
    case BSP_UART_EVENT_RX_DONE:
        // 收满
        break;
    case BSP_UART_EVENT_RX_IDLE:
        // 空闲线结束一帧
        break;
    case BSP_UART_EVENT_RX_HALF:
        // 收到一半
        break;
    case BSP_UART_EVENT_RX_TIMEOUT:
        // 超时
        break;
    case BSP_UART_EVENT_TX_DONE:
        // 发送完成
        break;
    case BSP_UART_EVENT_TX_HALF:
        // 发送过半
        break;
    case BSP_UART_EVENT_ERROR:
        // 错误，status 一般是 BSP_ERROR / BSP_ERROR_TIMEOUT
        break;
    case BSP_UART_EVENT_RX_READY:
        // 普通中断模式下一次收到 1 字节
        break;
    default:
        break;
    }
}

void app_uart_example()
{
    Uart uart(UartId::Debug);
    std::array<std::uint8_t, 64> rx{};

    (void) uart.init(115200U,
                     UartParity::None,
                     UartStopBits::One,
                     UartWordLength::Bits8,
                     TransferMode::Dma,
                     2U);
    (void) uart.onEvent(on_uart_event, nullptr);
    (void) uart.readAsync(ByteSpan(rx.data(), rx.size()));
    (void) uart.writeAsync("hello\r\n");
}
```

## 2. 旧接口还能不能用

可以，没删：

- `onReceive(...)`
- `onTransmit(...)`

只是：

- 旧接口适合简单场景
- `onEvent(...)` 适合你这种要区分“收/发/idle/timeout/half”的场景

## 3. 当前三个平台接通情况

### HPM

当前已接：

- `RxReady`
- `RxDone`
- `RxHalf`
- `RxIdle`
- `RxTimeout`
- `TxDone`
- `TxHalf`
- `Error`

说明：

- `DMA half` 用的是 HPM DMAV2 的 `HALF_TC`
- `RxIdle` 用的是 HPM UART 的 idle 检测

### STM32

当前已接：

- `RxDone`
- `RxHalf`
- `RxIdle`
- `TxDone`
- `TxHalf`
- `Error`

说明：

- `ReceiveToIdle_DMA` 事件类型来自 `HAL_UARTEx_GetRxEventType(...)`

### ESP32

当前先接基础版：

- `RxDone`
- `RxTimeout`
- `TxDone`
- `Error`

因为 ESP32 这边现在本身还是简单封装，没有继续往半传输/idle 细化。

## 4. 什么时候用哪个

- 只想收数据：`onReceive(...)`
- 只想知道发送完成：`onTransmit(...)`
- 想统一判断“这次到底是收满、空闲、超时、半传输还是发送完成”：`onEvent(...)`
