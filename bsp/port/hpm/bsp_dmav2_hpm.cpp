/**
 * @file bsp_dmav2_hpm.cpp
 * @brief HPM platform implementation for BSP DMAV2 interface
 */

#include "bsp_dmav2.h"
#include "board.h"

extern "C" {
#include "hpm_clock_drv.h"
#include "hpm_dmav2_drv.h"
#include "hpm_interrupt.h"
}

typedef struct {
    DMAV2_Type *base;
    uint32_t irq;
    clock_name_t clock_name;
    bsp_dmav2_callback_t callback;
    void *callback_arg;
    bool active;
} dmav2_instance_map_t;

#define BSP_DMAV2_MAP_ITEM(name, base_addr, irq_num, clk_name) { base_addr, irq_num, clk_name, nullptr, nullptr, false },
static const dmav2_instance_map_t s_dmav2_map[BSP_DMAV2_MAX] = {
    BSP_DMAV2_LIST(BSP_DMAV2_MAP_ITEM)
};
#undef BSP_DMAV2_MAP_ITEM

int32_t bsp_dmav2_memcpy(bsp_dmav2_instance_t instance, void *dst, const void *src, uint32_t size)
{
    if ((instance >= BSP_DMAV2_MAX) || (dst == nullptr) || (src == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }

    const dmav2_instance_map_t *map = &s_dmav2_map[instance];
    uint32_t status;

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    dma_clear_transfer_status(map->base, 0U);
    if (dma_start_memcpy(map->base, 0U,
                         core_local_mem_to_sys_address(HPM_CORE0, (uint32_t) dst),
                         core_local_mem_to_sys_address(HPM_CORE0, (uint32_t) src),
                         size, 16U) != status_success) {
        return BSP_ERROR;
    }

    do {
        status = dma_check_transfer_status(map->base, 0U);
        if ((status & DMA_CHANNEL_STATUS_ERROR) != 0U) {
            return BSP_ERROR;
        }
    } while ((status & DMA_CHANNEL_STATUS_TC) == 0U);

    return BSP_OK;
}

int32_t bsp_dmav2_memcpy_async(bsp_dmav2_instance_t instance, void *dst, const void *src, uint32_t size)
{
    if ((instance >= BSP_DMAV2_MAX) || (dst == nullptr) || (src == nullptr) || (size == 0U)) {
        return BSP_ERROR_PARAM;
    }

    auto *map = const_cast<dmav2_instance_map_t *>(&s_dmav2_map[instance]);
    if (map->active) {
        return BSP_ERROR_BUSY;
    }

    clock_add_to_group(map->clock_name, BOARD_RUNNING_CORE & 0x1);
    dma_clear_transfer_status(map->base, 0U);
    dma_enable_channel_interrupt(map->base, 0U, DMA_INTERRUPT_MASK_TERMINAL_COUNT | DMA_INTERRUPT_MASK_ERROR | DMA_INTERRUPT_MASK_ABORT);
    intc_m_enable_irq_with_priority(map->irq, 1);
    map->active = true;
    if (dma_start_memcpy(map->base, 0U,
                         core_local_mem_to_sys_address(HPM_CORE0, (uint32_t) dst),
                         core_local_mem_to_sys_address(HPM_CORE0, (uint32_t) src),
                         size, 16U) != status_success) {
        map->active = false;
        return BSP_ERROR;
    }
    return BSP_OK;
}

int32_t bsp_dmav2_register_callback(bsp_dmav2_instance_t instance, bsp_dmav2_callback_t callback, void *arg)
{
    if (instance >= BSP_DMAV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    auto *map = const_cast<dmav2_instance_map_t *>(&s_dmav2_map[instance]);
    map->callback = callback;
    map->callback_arg = arg;
    return BSP_OK;
}

extern "C" void bsp_dmav2_irq_handler(bsp_dmav2_instance_t instance)
{
    if (instance >= BSP_DMAV2_MAX) {
        return;
    }

    auto *map = const_cast<dmav2_instance_map_t *>(&s_dmav2_map[instance]);
    const uint32_t status = dma_check_transfer_status(map->base, 0U);
    if (status == 0U) {
        return;
    }

    dma_clear_transfer_status(map->base, 0U);
    map->active = false;
    if (map->callback != nullptr) {
        if ((status & DMA_CHANNEL_STATUS_TC) != 0U) {
            map->callback(BSP_OK, map->callback_arg);
        } else {
            map->callback(BSP_ERROR, map->callback_arg);
        }
    }
}
