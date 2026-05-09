# Porting Checklist

Use this checklist when adding a new board or chip.

1. Generate the vendor board package locally.
2. Add or update `bsp/chip/<platform>/board_config.h`.
3. Declare every resource capability explicitly.
4. Implement missing behavior in `bsp/port/<platform>`.
5. Return `Unsupported` for absent pinmux, absent DMA, absent PHY, or unavailable protocol stacks.
6. Return `Param` for invalid arguments.
7. Add API compile coverage through `modules/ClightSelfTest`.
8. Add direct BSP return-value coverage for new C interfaces.
9. Keep product credentials and generated build output out of Git.
10. Build the target firmware and run the self-test on hardware.

## Board Capability Examples

Declare real hardware facts in the board config:

- `SPI2` supports full duplex, blocking, interrupt, and DMA.
- `SPI6` supports TX-only WS2812 output and must reject RX or full-duplex transfers.
- `I2C1` may support fixed timing only, or configurable timing if the port implements timing calculation.
- `ENET` must return `Unsupported` when no PHY and pinmux are configured.

The board config is the contract. The BSP should not guess.
