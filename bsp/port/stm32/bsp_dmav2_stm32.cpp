#include "bsp_dmav2.h"

#include <cstdint>

#include "stm32_support.hpp"

typedef struct {
    DMA_Stream_TypeDef *stream;
    IRQn_Type irq;
    DMA_HandleTypeDef handle;
    bool initialized;
    bool active;
    bsp_dmav2_callback_t callback;
    void *callback_arg;
} dmav2_map_t;

#define BSP_DMAV2_MAP_ITEM(name, base, irq, clk) { base, irq, {}, false, false, nullptr, nullptr },
static dmav2_map_t s_dmav2_map[BSP_DMAV2_MAX] = {
    BSP_DMAV2_LIST(BSP_DMAV2_MAP_ITEM)
};
#undef BSP_DMAV2_MAP_ITEM

static void stm32_dmav2_complete(DMA_HandleTypeDef *hdma)
{
    for (std::uint32_t index = 0; index < BSP_DMAV2_MAX; ++index) {
        auto &map = s_dmav2_map[index];
        if (&map.handle == hdma) {
            map.active = false;
            if (map.callback != nullptr) {
                map.callback(BSP_OK, map.callback_arg);
            }
            break;
        }
    }
}

static void stm32_dmav2_error(DMA_HandleTypeDef *hdma)
{
    for (std::uint32_t index = 0; index < BSP_DMAV2_MAX; ++index) {
        auto &map = s_dmav2_map[index];
        if (&map.handle == hdma) {
            map.active = false;
            if (map.callback != nullptr) {
                map.callback(BSP_ERROR, map.callback_arg);
            }
            break;
        }
    }
}

static int32_t ensure_dmav2_initialized(bsp_dmav2_instance_t instance)
{
    if (instance >= BSP_DMAV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto &map = s_dmav2_map[instance];
    if (map.initialized) {
        return BSP_OK;
    }

    stm32_dma_ensure_initialized();

    map.handle.Instance = map.stream;
    map.handle.Init.Request = DMA_REQUEST_MEM2MEM;
    map.handle.Init.Direction = DMA_MEMORY_TO_MEMORY;
    map.handle.Init.PeriphInc = DMA_PINC_ENABLE;
    map.handle.Init.MemInc = DMA_MINC_ENABLE;
    map.handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    map.handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    map.handle.Init.Mode = DMA_NORMAL;
    map.handle.Init.Priority = DMA_PRIORITY_LOW;
    map.handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    map.handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    map.handle.Init.MemBurst = DMA_MBURST_SINGLE;
    map.handle.Init.PeriphBurst = DMA_PBURST_SINGLE;

    if (HAL_DMA_Init(&map.handle) != HAL_OK) {
        return BSP_ERROR;
    }

    map.handle.XferCpltCallback = stm32_dmav2_complete;
    map.handle.XferErrorCallback = stm32_dmav2_error;
    HAL_NVIC_SetPriority(map.irq, 5, 0);
    HAL_NVIC_EnableIRQ(map.irq);

    map.initialized = true;
    return BSP_OK;
}

int32_t bsp_dmav2_memcpy(bsp_dmav2_instance_t instance, void *dst, const void *src, uint32_t size)
{
    if ((instance >= BSP_DMAV2_MAX) || (dst == nullptr) || (src == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }

    const auto init_status = ensure_dmav2_initialized(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_dmav2_map[instance];
    if (map.handle.State != HAL_DMA_STATE_READY) {
        (void) HAL_DMA_Abort(&map.handle);
    }

    if (HAL_DMA_Start(
            &map.handle,
            static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(src)),
            static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(dst)),
            size) != HAL_OK) {
        return BSP_ERROR;
    }

    const auto poll_status = HAL_DMA_PollForTransfer(&map.handle, HAL_DMA_FULL_TRANSFER, HAL_MAX_DELAY);
    if (poll_status == HAL_TIMEOUT) {
        return BSP_ERROR_TIMEOUT;
    }
    return (poll_status == HAL_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_dmav2_memcpy_async(bsp_dmav2_instance_t instance, void *dst, const void *src, uint32_t size)
{
    if ((instance >= BSP_DMAV2_MAX) || (dst == nullptr) || (src == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }

    const auto init_status = ensure_dmav2_initialized(instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    auto &map = s_dmav2_map[instance];
    if (map.active) {
        return BSP_ERROR_BUSY;
    }

    map.active = true;
    if (HAL_DMA_Start_IT(
            &map.handle,
            static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(src)),
            static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(dst)),
            size) != HAL_OK) {
        map.active = false;
        return BSP_ERROR;
    }

    return BSP_OK;
}

int32_t bsp_dmav2_register_callback(bsp_dmav2_instance_t instance, bsp_dmav2_callback_t callback, void *arg)
{
    if (instance >= BSP_DMAV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    s_dmav2_map[instance].callback = callback;
    s_dmav2_map[instance].callback_arg = arg;
    return BSP_OK;
}

extern "C" void DMA1_Stream3_IRQHandler(void)
{
    if (s_dmav2_map[BSP_DMAV2_MAIN].initialized) {
        HAL_DMA_IRQHandler(&s_dmav2_map[BSP_DMAV2_MAIN].handle);
    }
}
