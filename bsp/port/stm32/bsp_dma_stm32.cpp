#include "stm32_support.hpp"

extern "C" {
#include "dma.h"
}

void stm32_dma_ensure_initialized()
{
    static bool initialized = false;
    if (!initialized) {
        MX_DMA_Init();
        initialized = true;
    }
}
