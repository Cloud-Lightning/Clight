#include "bsp_qeiv2.h"

typedef struct {
    TIM_TypeDef *base;
    IRQn_Type irq;
    uint32_t phase_count_per_rev;
    bool has_pinmux;
} qeiv2_map_t;

#define BSP_QEIV2_MAP_ITEM(name, base, irq, clk, phase_count, has_pinmux) \
    { base, irq, phase_count, (has_pinmux) != 0U },
static const qeiv2_map_t s_qeiv2_map[BSP_QEIV2_MAX] = {
    BSP_QEIV2_LIST(BSP_QEIV2_MAP_ITEM)
};
#undef BSP_QEIV2_MAP_ITEM

static int32_t qeiv2_status_for_board(bsp_qeiv2_instance_t qei)
{
    if (qei >= BSP_QEIV2_MAX) {
        return BSP_ERROR_PARAM;
    }

    return s_qeiv2_map[qei].has_pinmux ? BSP_ERROR : BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_qeiv2_init(bsp_qeiv2_instance_t qei)
{
    return qeiv2_status_for_board(qei);
}

int32_t bsp_qeiv2_get_position(bsp_qeiv2_instance_t qei, uint32_t *position)
{
    (void) position;
    return qeiv2_status_for_board(qei);
}

int32_t bsp_qeiv2_get_angle(bsp_qeiv2_instance_t qei, uint32_t *angle)
{
    (void) angle;
    return qeiv2_status_for_board(qei);
}

int32_t bsp_qeiv2_get_phase_count(bsp_qeiv2_instance_t qei, uint32_t *phase_count)
{
    (void) phase_count;
    return qeiv2_status_for_board(qei);
}

int32_t bsp_qeiv2_get_speed(bsp_qeiv2_instance_t qei, uint32_t *speed)
{
    (void) speed;
    return qeiv2_status_for_board(qei);
}
