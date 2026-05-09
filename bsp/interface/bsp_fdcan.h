/**
 * @file bsp_fdcan.h
 * @brief BSP FDCAN interface
 */

#ifndef BSP_FDCAN_H
#define BSP_FDCAN_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_FDCAN_FEATURE_CLASSIC            (1U << 0)
#define BSP_FDCAN_FEATURE_CAN_FD             (1U << 1)
#define BSP_FDCAN_FEATURE_BITRATE_SWITCH     (1U << 2)
#define BSP_FDCAN_FEATURE_INTERNAL_LOOPBACK  (1U << 3)
#define BSP_FDCAN_FEATURE_TX_FIFO            (1U << 4)
#define BSP_FDCAN_FEATURE_RX_FIFO0           (1U << 5)
#define BSP_FDCAN_FEATURE_STANDARD_ID        (1U << 6)
#define BSP_FDCAN_FEATURE_EXTENDED_ID        (1U << 7)
#define BSP_FDCAN_FEATURE_REMOTE_FRAME       (1U << 8)
#define BSP_FDCAN_FEATURE_FILTER_MASK        (1U << 9)
#define BSP_FDCAN_FEATURE_PROTOCOL_STATUS    (1U << 10)
#define BSP_FDCAN_FEATURE_ERROR_COUNT        (1U << 11)
#define BSP_FDCAN_FEATURE_AUTO_BITRATE       (1U << 12)
#define BSP_FDCAN_FEATURE_RX_CALLBACK        (1U << 13)
#define BSP_FDCAN_FEATURE_TX_CALLBACK        (1U << 14)
#define BSP_FDCAN_FEATURE_GLOBAL_FILTER      (1U << 15)
#define BSP_FDCAN_FEATURE_TX_EVENT_FIFO      (1U << 16)
#define BSP_FDCAN_FEATURE_TIMESTAMP          (1U << 17)

#include "board_config.h"

#ifndef BSP_FDCAN_LIST
#define BSP_FDCAN_LIST(X) \
    X(BSP_FDCAN_MAIN, BSP_FDCAN_MAIN_BASE, BSP_FDCAN_MAIN_IRQ, BSP_FDCAN_MAIN_HAS_PINMUX, BSP_FDCAN_MAIN_RAM_OFFSET, \
      BSP_FDCAN_FEATURE_CLASSIC | BSP_FDCAN_FEATURE_TX_FIFO | BSP_FDCAN_FEATURE_RX_FIFO0 | BSP_FDCAN_FEATURE_STANDARD_ID)
#endif

typedef enum {
#define BSP_FDCAN_ENUM_ITEM(name, base, irq, has_pinmux, ram_offset, features) name,
    BSP_FDCAN_LIST(BSP_FDCAN_ENUM_ITEM)
#undef BSP_FDCAN_ENUM_ITEM
    BSP_FDCAN_MAX
} bsp_fdcan_instance_t;

typedef struct {
    uint32_t id;
    uint8_t dlc;
    bool extended_id;
    bool remote_frame;
    bool fd_frame;
    bool bitrate_switch;
    bool error_state_indicator;
    uint8_t data[64];
} bsp_fdcan_frame_t;

typedef struct {
    bool enable_can_fd;
    bool bitrate_switch;
    bool internal_loopback;
    uint8_t std_filter_count;
    uint8_t ext_filter_count;
    uint8_t rx_fifo0_elements;
    uint8_t tx_fifo_elements;
    uint8_t tx_event_fifo_elements;
    bool timestamp_enable;
    uint32_t timestamp_prescaler;
    uint32_t nominal_bitrate;
    uint32_t data_bitrate;
    uint32_t nominal_prescaler;
    uint32_t nominal_sync_jump_width;
    uint32_t nominal_time_seg1;
    uint32_t nominal_time_seg2;
    uint32_t data_prescaler;
    uint32_t data_sync_jump_width;
    uint32_t data_time_seg1;
    uint32_t data_time_seg2;
} bsp_fdcan_config_t;

typedef struct {
    uint32_t id;
    uint32_t mask;
    bool extended_id;
    uint8_t filter_index;
} bsp_fdcan_filter_t;

typedef struct {
    uint32_t last_error_code;
    uint32_t data_last_error_code;
    uint32_t activity;
    bool error_warning;
    bool error_passive;
    bool bus_off;
    bool received_fd;
    bool received_brs;
} bsp_fdcan_protocol_status_t;

typedef struct {
    uint32_t tx_error_count;
    uint32_t rx_error_count;
    bool rx_error_passive;
    uint32_t error_logging_count;
} bsp_fdcan_error_count_t;

typedef enum {
    BSP_FDCAN_GLOBAL_FILTER_ACCEPT_RX_FIFO0 = 0,
    BSP_FDCAN_GLOBAL_FILTER_REJECT = 1,
} bsp_fdcan_global_filter_action_t;

typedef struct {
    bsp_fdcan_global_filter_action_t non_matching_std;
    bsp_fdcan_global_filter_action_t non_matching_ext;
    bool reject_remote_std;
    bool reject_remote_ext;
} bsp_fdcan_global_filter_t;

typedef struct {
    uint16_t timestamp;
    uint8_t message_marker;
    uint32_t event_type;
} bsp_fdcan_tx_event_t;

typedef void (*bsp_fdcan_rx_callback_t)(const bsp_fdcan_frame_t *frame, void *arg);
typedef void (*bsp_fdcan_tx_callback_t)(void *arg);

int32_t bsp_fdcan_get_default_config(bsp_fdcan_instance_t can_instance, bsp_fdcan_config_t *config, bool enable_can_fd);
int32_t bsp_fdcan_get_clock(bsp_fdcan_instance_t can_instance, uint32_t *freq_hz);
int32_t bsp_fdcan_config_bitrate(bsp_fdcan_instance_t can_instance,
                                 bsp_fdcan_config_t *config,
                                 uint32_t nominal_bitrate,
                                 uint32_t data_bitrate);
int32_t bsp_fdcan_init(bsp_fdcan_instance_t can_instance, bool internal_loopback);
int32_t bsp_fdcan_init_ex(bsp_fdcan_instance_t can_instance, const bsp_fdcan_config_t *config);
int32_t bsp_fdcan_transmit(bsp_fdcan_instance_t can_instance, const bsp_fdcan_frame_t *frame);
int32_t bsp_fdcan_receive(bsp_fdcan_instance_t can_instance, bsp_fdcan_frame_t *frame);
int32_t bsp_fdcan_loopback(bsp_fdcan_instance_t can_instance, const bsp_fdcan_frame_t *tx_frame, bsp_fdcan_frame_t *rx_frame);
int32_t bsp_fdcan_config_filter_mask(bsp_fdcan_instance_t can_instance, const bsp_fdcan_filter_t *filter);
int32_t bsp_fdcan_set_global_filter(bsp_fdcan_instance_t can_instance, const bsp_fdcan_global_filter_t *filter);
int32_t bsp_fdcan_get_protocol_status(bsp_fdcan_instance_t can_instance, bsp_fdcan_protocol_status_t *status);
int32_t bsp_fdcan_get_error_count(bsp_fdcan_instance_t can_instance, bsp_fdcan_error_count_t *count);
int32_t bsp_fdcan_enable_timestamp(bsp_fdcan_instance_t can_instance, uint32_t prescaler);
int32_t bsp_fdcan_get_timestamp(bsp_fdcan_instance_t can_instance, uint16_t *timestamp);
int32_t bsp_fdcan_read_tx_event(bsp_fdcan_instance_t can_instance, bsp_fdcan_tx_event_t *event);
int32_t bsp_fdcan_register_rx_callback(bsp_fdcan_instance_t can_instance, bsp_fdcan_rx_callback_t callback, void *arg);
int32_t bsp_fdcan_register_tx_callback(bsp_fdcan_instance_t can_instance, bsp_fdcan_tx_callback_t callback, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* BSP_FDCAN_H */
