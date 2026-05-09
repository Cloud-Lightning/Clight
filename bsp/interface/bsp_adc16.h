/**
 * @file bsp_adc16.h
 * @brief BSP ADC16 interface
 */

#ifndef BSP_ADC16_H
#define BSP_ADC16_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_ADC16_FEATURE_ONESHOT                     (1U << 0)
#define BSP_ADC16_FEATURE_BLOCKING_READ               (1U << 1)
#define BSP_ADC16_FEATURE_INTERRUPT_READ              (1U << 2)
#define BSP_ADC16_FEATURE_PERIOD                      (1U << 3)
#define BSP_ADC16_FEATURE_SEQUENCE                    (1U << 4)
#define BSP_ADC16_FEATURE_SEQUENCE_HARDWARE_TRIGGER   (1U << 5)
#define BSP_ADC16_FEATURE_PREEMPTION                  (1U << 6)
#define BSP_ADC16_FEATURE_PREEMPTION_HARDWARE_TRIGGER (1U << 7)
#define BSP_ADC16_FEATURE_CONFIGURABLE_RESOLUTION     (1U << 8)
#define BSP_ADC16_FEATURE_CONFIGURABLE_CLOCK_DIVIDER  (1U << 9)
#define BSP_ADC16_FEATURE_GET_CLOCK                   (1U << 10)
#define BSP_ADC16_FEATURE_TRIGGER_ROUTE               (1U << 11)
#define BSP_ADC16_FEATURE_SEQUENCE_DMA                (1U << 12)
#define BSP_ADC16_FEATURE_CONTINUOUS_DMA              (1U << 13)

#include "board_config.h"

#ifndef BSP_ADC16_LIST
#define BSP_ADC16_LIST(X) \
    X(BSP_ADC16_MAIN, BSP_ADC16_MAIN_BASE, BSP_ADC16_MAIN_IRQ, BSP_ADC16_MAIN_CLK_NAME, BSP_ADC16_MAIN_DEFAULT_CH, BSP_ADC16_MAIN_CLK_SRC_BUS, BSP_ADC16_MAIN_HAS_PINMUX, \
      BSP_ADC16_FEATURE_ONESHOT | BSP_ADC16_FEATURE_BLOCKING_READ)
#endif

typedef enum {
#define BSP_ADC16_ENUM_ITEM(name, base, irq, clk, ch, clk_src_bus, has_pinmux, features) name,
    BSP_ADC16_LIST(BSP_ADC16_ENUM_ITEM)
#undef BSP_ADC16_ENUM_ITEM
    BSP_ADC16_MAX
} bsp_adc16_instance_t;

typedef struct {
    uint8_t channel;
    uint8_t index;
    uint8_t trigger_channel;
    uint16_t value;
} bsp_adc16_sample_t;

typedef enum {
    BSP_ADC16_RESOLUTION_DEFAULT = 0,
    BSP_ADC16_RESOLUTION_8_BITS = 8,
    BSP_ADC16_RESOLUTION_10_BITS = 10,
    BSP_ADC16_RESOLUTION_12_BITS = 12,
    BSP_ADC16_RESOLUTION_16_BITS = 16,
} bsp_adc16_resolution_t;

typedef enum {
    BSP_ADC16_CLOCK_DIVIDER_DEFAULT = 0,
    BSP_ADC16_CLOCK_DIVIDER_1 = 1,
    BSP_ADC16_CLOCK_DIVIDER_2 = 2,
    BSP_ADC16_CLOCK_DIVIDER_3 = 3,
    BSP_ADC16_CLOCK_DIVIDER_4 = 4,
    BSP_ADC16_CLOCK_DIVIDER_5 = 5,
    BSP_ADC16_CLOCK_DIVIDER_6 = 6,
    BSP_ADC16_CLOCK_DIVIDER_7 = 7,
    BSP_ADC16_CLOCK_DIVIDER_8 = 8,
    BSP_ADC16_CLOCK_DIVIDER_9 = 9,
    BSP_ADC16_CLOCK_DIVIDER_10 = 10,
    BSP_ADC16_CLOCK_DIVIDER_11 = 11,
    BSP_ADC16_CLOCK_DIVIDER_12 = 12,
    BSP_ADC16_CLOCK_DIVIDER_13 = 13,
    BSP_ADC16_CLOCK_DIVIDER_14 = 14,
    BSP_ADC16_CLOCK_DIVIDER_15 = 15,
    BSP_ADC16_CLOCK_DIVIDER_16 = 16,
} bsp_adc16_clock_divider_t;

typedef struct {
    bsp_adc16_resolution_t resolution;
    bsp_adc16_clock_divider_t clock_divider;
} bsp_adc16_init_config_t;

typedef struct {
    uint32_t *buffer;
    uint32_t sample_count;
    bsp_state_t circular;
} bsp_adc16_dma_sequence_config_t;

typedef enum {
    BSP_ADC16_TRIGGER_OUTPUT_SAME_AS_INPUT = 0,
    BSP_ADC16_TRIGGER_OUTPUT_PULSE_FALLING = 1,
    BSP_ADC16_TRIGGER_OUTPUT_PULSE_RISING = 2,
    BSP_ADC16_TRIGGER_OUTPUT_PULSE_BOTH = 3,
} bsp_adc16_trigger_output_t;

typedef struct {
    uint32_t input;
    uint32_t output;
    bsp_state_t invert;
    bsp_adc16_trigger_output_t type;
} bsp_adc16_trigger_route_t;

typedef void (*bsp_adc16_callback_t)(uint8_t channel, uint16_t value, void *arg);
typedef void (*bsp_adc16_sequence_callback_t)(const bsp_adc16_sample_t *samples, uint8_t count, void *arg);
typedef void (*bsp_adc16_preemption_callback_t)(uint8_t trigger_channel, const bsp_adc16_sample_t *samples, uint8_t count, void *arg);

void bsp_adc16_get_default_init_config(bsp_adc16_init_config_t *config);
int32_t bsp_adc16_init(bsp_adc16_instance_t adc);
int32_t bsp_adc16_init_ex(bsp_adc16_instance_t adc, const bsp_adc16_init_config_t *config);
int32_t bsp_adc16_get_clock(bsp_adc16_instance_t adc, uint32_t *freq_hz);
int32_t bsp_adc16_read_channel(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value);
int32_t bsp_adc16_read_default(bsp_adc16_instance_t adc, uint16_t *value);
int32_t bsp_adc16_get_default_channel(bsp_adc16_instance_t adc, uint8_t *channel);
int32_t bsp_adc16_configure_period(bsp_adc16_instance_t adc, uint8_t channel, uint8_t prescale, uint8_t period_count);
int32_t bsp_adc16_read_period(bsp_adc16_instance_t adc, uint8_t channel, uint16_t *value);
int32_t bsp_adc16_read_channel_async(bsp_adc16_instance_t adc, uint8_t channel);
int32_t bsp_adc16_read_default_async(bsp_adc16_instance_t adc);
int32_t bsp_adc16_register_callback(bsp_adc16_instance_t adc, bsp_adc16_callback_t callback, void *arg);
int32_t bsp_adc16_set_sequence_trigger_route(bsp_adc16_instance_t adc, const bsp_adc16_trigger_route_t *route);
int32_t bsp_adc16_configure_sequence(bsp_adc16_instance_t adc,
                                     const uint8_t *channels,
                                     uint8_t count,
                                     bsp_state_t continuous,
                                     bsp_state_t hardware_trigger);
int32_t bsp_adc16_start_sequence(bsp_adc16_instance_t adc);
int32_t bsp_adc16_start_sequence_dma(bsp_adc16_instance_t adc, const bsp_adc16_dma_sequence_config_t *config);
int32_t bsp_adc16_register_sequence_callback(bsp_adc16_instance_t adc, bsp_adc16_sequence_callback_t callback, void *arg);
int32_t bsp_adc16_set_preemption_trigger_route(bsp_adc16_instance_t adc, const bsp_adc16_trigger_route_t *route);
int32_t bsp_adc16_configure_preemption(bsp_adc16_instance_t adc,
                                       uint8_t trigger_channel,
                                       const uint8_t *channels,
                                       uint8_t count,
                                       bsp_state_t hardware_trigger);
int32_t bsp_adc16_start_preemption(bsp_adc16_instance_t adc);
int32_t bsp_adc16_register_preemption_callback(bsp_adc16_instance_t adc, bsp_adc16_preemption_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ADC16_H */
