# Platform Workspace

The public Clight repository is framework-first. A real firmware workspace should bring its own vendor SDK and generated board files.

Recommended product layout:

```text
YourProduct/
  Clight/                  # submodule, subtree, or copied framework
  app/                     # product application
  board/                   # private generated board package
  vendor/                  # optional local SDK/protocol stacks, not public
  build/                   # generated output, ignored
```

You can also place generated board files under `Clight/platforms/<platform>/...` while developing locally, but do not publish those generated files to the public framework repository unless you intentionally maintain a separate board package.

## CMake Boundary

Clight provides reusable source and headers. The final firmware CMake normally belongs to the product or board package because it must know:

- chip part number
- linker script
- startup file
- pinmux and clock files
- enabled HAL or SDK components
- external vendor stacks
- flash or RAM execution mode

The checked-in platform CMake files are templates only. They should not pretend to build a real board without the generated files.

## Public Include Style

Application and modules should include Clight API headers:

```cpp
#include "Gpio.hpp"
#include "Spi.hpp"
#include "Uart.hpp"
#include "Can.hpp"
```

Only `bsp/port/<platform>` should include vendor headers such as STM32 HAL, HPM SDK, or ESP-IDF headers.
