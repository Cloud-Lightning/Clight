/**
 * @file bsp_mcan.h
 * @brief BSP MCAN interface
 */

#ifndef BSP_MCAN_H
#define BSP_MCAN_H

#include "bsp_common.h"
#include "hpm_mcan_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_MCAN_FEATURE_CLASSIC             (1U << 0)
#define BSP_MCAN_FEATURE_CAN_FD              (1U << 1)
#define BSP_MCAN_FEATURE_BITRATE_SWITCH      (1U << 2)
#define BSP_MCAN_FEATURE_INTERNAL_LOOPBACK   (1U << 3)
#define BSP_MCAN_FEATURE_TX_BLOCKING         (1U << 4)
#define BSP_MCAN_FEATURE_TX_FIFO             (1U << 5)
#define BSP_MCAN_FEATURE_TX_BUFFER           (1U << 6)
#define BSP_MCAN_FEATURE_RX_FIFO0            (1U << 7)
#define BSP_MCAN_FEATURE_RX_FIFO1            (1U << 8)
#define BSP_MCAN_FEATURE_RX_BUFFER           (1U << 9)
#define BSP_MCAN_FEATURE_STANDARD_ID         (1U << 10)
#define BSP_MCAN_FEATURE_EXTENDED_ID         (1U << 11)
#define BSP_MCAN_FEATURE_REMOTE_FRAME        (1U << 12)
#define BSP_MCAN_FEATURE_FILTERS             (1U << 13)
#define BSP_MCAN_FEATURE_GLOBAL_FILTER       (1U << 14)
#define BSP_MCAN_FEATURE_TX_EVENT_FIFO       (1U << 15)
#define BSP_MCAN_FEATURE_PROTOCOL_STATUS     (1U << 16)
#define BSP_MCAN_FEATURE_ERROR_COUNT         (1U << 17)
#define BSP_MCAN_FEATURE_TIMESTAMP           (1U << 18)
#define BSP_MCAN_FEATURE_CANCEL_TX           (1U << 19)
#define BSP_MCAN_FEATURE_AUTO_BITRATE        (1U << 20)
#define BSP_MCAN_FEATURE_RX_CALLBACK         (1U << 21)
#define BSP_MCAN_FEATURE_TX_CALLBACK         (1U << 22)
#define BSP_MCAN_FEATURE_EVENT_CALLBACK      (1U << 23)

#include "board_config.h"

#ifndef BSP_MCAN_LIST
#define BSP_MCAN_LIST(X) \
    X(BSP_MCAN_MAIN, BSP_MCAN_MAIN_BASE, BSP_MCAN_MAIN_IRQ, BSP_MCAN_MAIN_CLK_NAME, BSP_MCAN_MAIN_HAS_PINMUX, \
      BSP_MCAN_FEATURE_CLASSIC | BSP_MCAN_FEATURE_TX_BLOCKING | BSP_MCAN_FEATURE_RX_FIFO0 | BSP_MCAN_FEATURE_STANDARD_ID)
#endif

typedef enum {
#define BSP_MCAN_ENUM_ITEM(name, base, irq, clk, has_pinmux, features) name,
    BSP_MCAN_LIST(BSP_MCAN_ENUM_ITEM)
#undef BSP_MCAN_ENUM_ITEM
    BSP_MCAN_MAX
} bsp_mcan_instance_t;

typedef mcan_config_t bsp_mcan_config_t;
typedef mcan_ram_config_t bsp_mcan_ram_config_t;
typedef mcan_global_filter_config_t bsp_mcan_global_filter_config_t;
typedef mcan_all_filters_config_t bsp_mcan_all_filters_config_t;
typedef mcan_filter_elem_t bsp_mcan_filter_elem_t;
typedef mcan_tx_event_fifo_elem_t bsp_mcan_tx_event_fifo_elem_t;
typedef mcan_protocol_status_t bsp_mcan_protocol_status_t;
typedef mcan_error_count_t bsp_mcan_error_count_t;
typedef mcan_timestamp_value_t bsp_mcan_timestamp_value_t;

typedef struct {
    uint32_t id;
    uint8_t dlc;
    bool extended_id;
    bool remote_frame;
    bool fd_frame;
    bool bitrate_switch;
    bool error_state_indicator;
    bool tx_event_fifo_control;
    bool timestamp_capture_enable;
    uint16_t message_marker;
    uint16_t timestamp;
    bool timestamp_captured;
    uint8_t timestamp_pointer;
    uint8_t rx_fifo_index;
    uint8_t filter_index;
    bool accepted_non_matching_frame;
    uint8_t data[64];
} bsp_mcan_frame_t;

typedef void (*bsp_mcan_rx_callback_t)(const bsp_mcan_frame_t *frame, void *arg);
typedef void (*bsp_mcan_tx_callback_t)(void *arg);
typedef void (*bsp_mcan_event_callback_t)(uint32_t event_mask, void *arg);

int32_t bsp_mcan_init(bsp_mcan_instance_t can_instance, bool internal_loopback);
int32_t bsp_mcan_get_default_config(bsp_mcan_instance_t can_instance, bsp_mcan_config_t *config, bool enable_canfd);
int32_t bsp_mcan_get_clock(bsp_mcan_instance_t can_instance, uint32_t *freq_hz);
int32_t bsp_mcan_config_bitrate(bsp_mcan_instance_t can_instance,
                                bsp_mcan_config_t *config,
                                uint32_t nominal_bitrate,
                                uint32_t data_bitrate);
int32_t bsp_mcan_init_ex(bsp_mcan_instance_t can_instance, const bsp_mcan_config_t *config, uint8_t irq_priority);
int32_t bsp_mcan_deinit(bsp_mcan_instance_t can_instance);
int32_t bsp_mcan_transmit(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *frame);
int32_t bsp_mcan_transmit_fifo(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *frame, uint32_t *fifo_index);
int32_t bsp_mcan_transmit_buffer(bsp_mcan_instance_t can_instance, uint32_t buffer_index, const bsp_mcan_frame_t *frame);
int32_t bsp_mcan_receive(bsp_mcan_instance_t can_instance, bsp_mcan_frame_t *frame);
int32_t bsp_mcan_receive_fifo(bsp_mcan_instance_t can_instance, uint32_t fifo_index, bsp_mcan_frame_t *frame);
int32_t bsp_mcan_receive_buffer(bsp_mcan_instance_t can_instance, uint32_t buffer_index, bsp_mcan_frame_t *frame);
int32_t bsp_mcan_loopback(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *tx_frame, bsp_mcan_frame_t *rx_frame);
int32_t bsp_mcan_config_filters(bsp_mcan_instance_t can_instance, const bsp_mcan_all_filters_config_t *config);
int32_t bsp_mcan_set_global_filter(bsp_mcan_instance_t can_instance, const bsp_mcan_global_filter_config_t *config);
int32_t bsp_mcan_read_tx_event(bsp_mcan_instance_t can_instance, bsp_mcan_tx_event_fifo_elem_t *event);
int32_t bsp_mcan_get_protocol_status(bsp_mcan_instance_t can_instance, bsp_mcan_protocol_status_t *status);
int32_t bsp_mcan_get_error_count(bsp_mcan_instance_t can_instance, bsp_mcan_error_count_t *count);
int32_t bsp_mcan_get_timestamp_from_tx_event(bsp_mcan_instance_t can_instance, const bsp_mcan_tx_event_fifo_elem_t *event, bsp_mcan_timestamp_value_t *timestamp);
int32_t bsp_mcan_get_timestamp_from_frame(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *frame, bsp_mcan_timestamp_value_t *timestamp);
int32_t bsp_mcan_cancel_tx_request(bsp_mcan_instance_t can_instance, uint32_t buffer_index);
int32_t bsp_mcan_is_tx_request_cancelled(bsp_mcan_instance_t can_instance, uint32_t buffer_index, bool *finished);
int32_t bsp_mcan_register_rx_callback(bsp_mcan_instance_t can_instance, bsp_mcan_rx_callback_t callback, void *arg);
int32_t bsp_mcan_register_tx_callback(bsp_mcan_instance_t can_instance, bsp_mcan_tx_callback_t callback, void *arg);
int32_t bsp_mcan_register_event_callback(bsp_mcan_instance_t can_instance, bsp_mcan_event_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_MCAN_H */
