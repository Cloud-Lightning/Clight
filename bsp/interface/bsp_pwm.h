/**
 * @file bsp_pwm.h
 * @brief Generic BSP PWM capability interface
 */

#ifndef BSP_PWM_H
#define BSP_PWM_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_PWM_BACKEND_STM32_TIM    (1U)
#define BSP_PWM_BACKEND_STM32_HRTIM  (2U)
#define BSP_PWM_BACKEND_HPM_GPTMR    (3U)
#define BSP_PWM_BACKEND_HPM_PWMV2    (4U)
#define BSP_PWM_BACKEND_HPM_PTW      (5U)
#define BSP_PWM_BACKEND_ESP32_LEDC   (6U)

#define BSP_PWM_FEATURE_EDGE_ALIGNED     (1U << 0)
#define BSP_PWM_FEATURE_CENTER_ALIGNED   (1U << 1)
#define BSP_PWM_FEATURE_COMPLEMENTARY    (1U << 2)
#define BSP_PWM_FEATURE_DEADTIME         (1U << 3)
#define BSP_PWM_FEATURE_TRIGGER_COMPARE  (1U << 4)

#define BSP_PWM_EVENT_RELOAD         (1U << 0)
#define BSP_PWM_EVENT_COMPARE        (1U << 1)
#define BSP_PWM_EVENT_FAULT          (1U << 2)

#include "board_config.h"

#ifndef BSP_PWM_LIST
#define BSP_PWM_LIST(X) \
    X(BSP_PWM_MAIN, BSP_PWM_BACKEND_STM32_TIM, BSP_PWM_MAIN_BASE, BSP_PWM_MAIN_IRQ, BSP_PWM_MAIN_CLK_NAME, 0U, BSP_PWM_MAIN_HAS_PINMUX, BSP_PWM_FEATURE_EDGE_ALIGNED)
#endif

typedef enum {
#define BSP_PWM_ENUM_ITEM(name, backend, base, irq, clk, channel_base, has_pinmux, features) name,
    BSP_PWM_LIST(BSP_PWM_ENUM_ITEM)
#undef BSP_PWM_ENUM_ITEM
    BSP_PWM_MAX
} bsp_pwm_instance_t;

typedef enum {
    BSP_PWM_ALIGN_EDGE = 0,
    BSP_PWM_ALIGN_CENTER = 1
} bsp_pwm_align_t;

typedef void (*bsp_pwm_event_callback_t)(uint32_t event_mask, void *arg);
typedef bsp_pwm_event_callback_t bsp_pwm_callback_t;

int32_t bsp_pwm_init(bsp_pwm_instance_t instance, uint32_t freq_hz, bsp_pwm_align_t align_mode);
int32_t bsp_pwm_get_frequency(bsp_pwm_instance_t instance, uint32_t *freq_hz);
int32_t bsp_pwm_set_duty(bsp_pwm_instance_t instance, uint8_t channel, float duty_percent);
int32_t bsp_pwm_set_compare_ticks(bsp_pwm_instance_t instance, uint8_t compare_index, uint32_t compare_ticks);
int32_t bsp_pwm_enable_output(bsp_pwm_instance_t instance, uint8_t channel, bsp_state_t state);
int32_t bsp_pwm_enable_pair(bsp_pwm_instance_t instance, uint8_t channel, bsp_state_t state);
int32_t bsp_pwm_set_deadtime(bsp_pwm_instance_t instance, uint8_t channel, uint16_t deadtime_ticks);
int32_t bsp_pwm_force_output(bsp_pwm_instance_t instance, uint8_t channel, bsp_level_t level);
int32_t bsp_pwm_start(bsp_pwm_instance_t instance);
int32_t bsp_pwm_stop(bsp_pwm_instance_t instance);
int32_t bsp_pwm_register_event_callback(bsp_pwm_instance_t instance, bsp_pwm_event_callback_t callback, void *arg);

static inline int32_t bsp_pwm_register_callback(bsp_pwm_instance_t instance, bsp_pwm_callback_t callback, void *arg)
{
    return bsp_pwm_register_event_callback(instance, callback, arg);
}

#ifdef __cplusplus
}
#endif

#endif /* BSP_PWM_H */
