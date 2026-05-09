/**
 * @file bsp_mcan_hpm.cpp
 * @brief HPM platform implementation for BSP MCAN interface
 */

#include "bsp_mcan.h"
#include "board.h"
#include <string.h>

extern "C" {
#include "hpm_interrupt.h"
}

#if defined(MCAN_SOC_MSG_BUF_IN_AHB_RAM) && (MCAN_SOC_MSG_BUF_IN_AHB_RAM == 1)
#define BSP_MCAN_DECLARE_MSG_BUF(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    ATTR_PLACE_AT(".ahb_sram") static uint32_t name##_msg_buf[MCAN_MSG_BUF_SIZE_IN_WORDS];
BSP_MCAN_LIST(BSP_MCAN_DECLARE_MSG_BUF)
#undef BSP_MCAN_DECLARE_MSG_BUF
#endif

typedef struct {
    MCAN_Type *base;
    uint32_t irq;
    uint32_t clock_name;
    bool has_pinmux;
    uint32_t features;
    bool initialized;
    uint8_t irq_priority;
    bsp_mcan_rx_callback_t rx_callback;
    void *rx_callback_arg;
    bsp_mcan_tx_callback_t tx_callback;
    void *tx_callback_arg;
    bsp_mcan_event_callback_t event_callback;
    void *event_callback_arg;
#if defined(MCAN_SOC_MSG_BUF_IN_AHB_RAM) && (MCAN_SOC_MSG_BUF_IN_AHB_RAM == 1)
    uint32_t *msg_buf;
    uint32_t msg_buf_size;
#endif
} mcan_instance_map_t;

#if defined(MCAN_SOC_MSG_BUF_IN_AHB_RAM) && (MCAN_SOC_MSG_BUF_IN_AHB_RAM == 1)
#define BSP_MCAN_MAP_ITEM(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    { base_addr, irq_num, clk_name, (has_pinmux) != 0U, static_cast<uint32_t>(features), false, BSP_IRQ_PRIORITY_DEFAULT, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &name##_msg_buf[0], sizeof(name##_msg_buf) },
#else
#define BSP_MCAN_MAP_ITEM(name, base_addr, irq_num, clk_name, has_pinmux, features) \
    { base_addr, irq_num, clk_name, (has_pinmux) != 0U, static_cast<uint32_t>(features), false, BSP_IRQ_PRIORITY_DEFAULT, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr },
#endif

static mcan_instance_map_t s_mcan_map[BSP_MCAN_MAX] = {
    BSP_MCAN_LIST(BSP_MCAN_MAP_ITEM)
};
#undef BSP_MCAN_MAP_ITEM

static int32_t bsp_mcan_status_from_hpm(hpm_stat_t status)
{
    switch (status) {
    case status_success:
        return BSP_OK;
    case status_invalid_argument:
        return BSP_ERROR_PARAM;
    case status_mcan_rxfifo_empty:
    case status_mcan_txbuf_full:
    case status_mcan_txfifo_full:
    case status_mcan_rxfifo0_busy:
    case status_mcan_rxfifo1_busy:
    case status_mcan_rxbuf_empty:
    case status_mcan_tx_evt_fifo_empty:
        return BSP_ERROR_BUSY;
    case status_mcan_timeout:
        return BSP_ERROR_TIMEOUT;
    default:
        return BSP_ERROR;
    }
}

static bool bsp_mcan_has_feature(const mcan_instance_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static int32_t bsp_mcan_validate_config_features(const mcan_instance_map_t &map, const bsp_mcan_config_t *config)
{
    if (config == nullptr) {
        return BSP_ERROR_PARAM;
    }
    if (config->enable_canfd) {
        if (!bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_CAN_FD)) {
            return BSP_ERROR_UNSUPPORTED;
        }
    } else if (!bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_CLASSIC)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((config->mode == mcan_mode_loopback_internal) &&
        !bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_INTERNAL_LOOPBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    return BSP_OK;
}

static int32_t bsp_mcan_validate_frame(const bsp_mcan_frame_t *frame)
{
    if (frame == nullptr) {
        return BSP_ERROR_PARAM;
    }
    if (frame->dlc > MCAN_MSG_DLC_64_BYTES) {
        return BSP_ERROR_PARAM;
    }
    if ((!frame->extended_id) && (frame->id > 0x7FFU)) {
        return BSP_ERROR_PARAM;
    }
    if ((frame->extended_id) && (frame->id > 0x1FFFFFFFU)) {
        return BSP_ERROR_PARAM;
    }

    const uint8_t payload_size = mcan_get_message_size_from_dlc(frame->dlc);
    if ((!frame->fd_frame) && (payload_size > 8U)) {
        return BSP_ERROR_PARAM;
    }
    if (frame->remote_frame && frame->fd_frame) {
        return BSP_ERROR_PARAM;
    }
    return BSP_OK;
}

static int32_t bsp_mcan_validate_frame_features(const mcan_instance_map_t &map, const bsp_mcan_frame_t *frame);

static int32_t bsp_mcan_prepare_instance(mcan_instance_map_t *map, uint32_t *can_clock)
{
    if ((map == nullptr) || (can_clock == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!map->has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }

    (void) map->clock_name;
    board_init_can(map->base);
    *can_clock = board_init_can_clock(map->base);
    if (*can_clock == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

#if defined(MCAN_SOC_MSG_BUF_IN_AHB_RAM) && (MCAN_SOC_MSG_BUF_IN_AHB_RAM == 1)
    const mcan_msg_buf_attr_t attr = { (uint32_t) map->msg_buf, map->msg_buf_size };
    if (mcan_set_msg_buf_attr(map->base, &attr) != status_success) {
        return BSP_ERROR;
    }
#endif
    return BSP_OK;
}

static int32_t bsp_mcan_get_default_config_internal(mcan_instance_map_t *map, bsp_mcan_config_t *config, bool enable_canfd)
{
    if ((map == nullptr) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (enable_canfd && !bsp_mcan_has_feature(*map, BSP_MCAN_FEATURE_CAN_FD)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!enable_canfd && !bsp_mcan_has_feature(*map, BSP_MCAN_FEATURE_CLASSIC)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_get_default_config(map->base, config);
    config->baudrate = 500000U;
    config->interrupt_mask = MCAN_INT_RXFIFO0_NEW_MSG | MCAN_INT_TX_COMPLETED;
    config->txbuf_trans_interrupt_mask = ~0UL;
    config->txbuf_cancel_finish_interrupt_mask = 0U;
    config->enable_canfd = enable_canfd;
    mcan_get_default_ram_config(map->base, &config->ram_config, enable_canfd);
    return BSP_OK;
}

static int32_t bsp_mcan_configure_instance(mcan_instance_map_t *map, const bsp_mcan_config_t *config, uint8_t irq_priority)
{
    if ((map == nullptr) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    uint32_t can_clock = 0U;
    const int32_t feature_status = bsp_mcan_validate_config_features(*map, config);
    if (feature_status != BSP_OK) {
        return feature_status;
    }
    const int32_t prepare_status = bsp_mcan_prepare_instance(map, &can_clock);
    if (prepare_status != BSP_OK) {
        return prepare_status;
    }

    if (mcan_init(map->base, const_cast<bsp_mcan_config_t *>(config), can_clock) != status_success) {
        return BSP_ERROR;
    }

    intc_m_enable_irq_with_priority(map->irq, irq_priority);
    map->initialized = true;
    map->irq_priority = irq_priority;
    return BSP_OK;
}

static int32_t bsp_mcan_ensure_initialized_for_frame(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *frame)
{
    if ((can_instance >= BSP_MCAN_MAX) || (frame == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    const int32_t frame_feature_status = bsp_mcan_validate_frame_features(*map, frame);
    if (frame_feature_status != BSP_OK) {
        return frame_feature_status;
    }
    if (map->initialized) {
        return BSP_OK;
    }

    bsp_mcan_config_t config{};
    const bool enable_canfd = frame->fd_frame || (frame->dlc > MCAN_MSG_DLC_8_BYTES);
    int32_t status = bsp_mcan_get_default_config_internal(map, &config, enable_canfd);
    if (status != BSP_OK) {
        return status;
    }
    return bsp_mcan_configure_instance(map, &config, map->irq_priority);
}

static void bsp_mcan_copy_to_hw(const bsp_mcan_frame_t *src, mcan_tx_frame_t *dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->dlc = src->dlc;
    dst->rtr = src->remote_frame ? 1U : 0U;
    dst->use_ext_id = src->extended_id ? 1U : 0U;
    dst->error_state_indicator = src->error_state_indicator ? 1U : 0U;
    dst->bitrate_switch = src->bitrate_switch ? 1U : 0U;
    dst->canfd_frame = src->fd_frame ? 1U : 0U;
    dst->timestamp_capture_enable = src->timestamp_capture_enable ? 1U : 0U;
    dst->event_fifo_control = src->tx_event_fifo_control ? 1U : 0U;
    dst->message_marker_l = (uint8_t) (src->message_marker & 0xFFU);
    dst->message_marker_h = (uint8_t) ((src->message_marker >> 8U) & 0xFFU);

    if (src->extended_id) {
        dst->ext_id = src->id & 0x1FFFFFFFU;
    } else {
        dst->std_id = src->id & 0x7FFU;
    }

    const uint8_t payload_size = mcan_get_message_size_from_dlc(src->dlc);
    if (payload_size > 0U) {
        memcpy(dst->data_8, src->data, payload_size);
    }
}

static void bsp_mcan_copy_from_hw(const mcan_rx_message_t *src, uint8_t fifo_index, bsp_mcan_frame_t *dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->extended_id = src->use_ext_id != 0U;
    dst->id = dst->extended_id ? src->ext_id : src->std_id;
    dst->dlc = (uint8_t) src->dlc;
    dst->remote_frame = src->rtr != 0U;
    dst->fd_frame = src->canfd_frame != 0U;
    dst->bitrate_switch = src->bitrate_switch != 0U;
    dst->error_state_indicator = src->error_state_indicator != 0U;
    dst->timestamp = src->rx_timestamp;
    dst->timestamp_captured = src->rx_timestamp_captured != 0U;
    dst->timestamp_pointer = src->rx_timestamp_pointer;
    dst->rx_fifo_index = fifo_index;
    dst->filter_index = (uint8_t) src->filter_index;
    dst->accepted_non_matching_frame = src->accepted_non_matching_frame != 0U;

    const uint8_t payload_size = mcan_get_message_size_from_dlc(dst->dlc);
    if (payload_size > 0U) {
        memcpy(dst->data, src->data_8, payload_size);
    }
}

static void bsp_mcan_copy_to_rx_hw(const bsp_mcan_frame_t *src, mcan_rx_message_t *dst)
{
    memset(dst, 0, sizeof(*dst));
    dst->use_ext_id = src->extended_id ? 1U : 0U;
    if (src->extended_id) {
        dst->ext_id = src->id & 0x1FFFFFFFU;
    } else {
        dst->std_id = src->id & 0x7FFU;
    }
    dst->rtr = src->remote_frame ? 1U : 0U;
    dst->error_state_indicator = src->error_state_indicator ? 1U : 0U;
    dst->rx_timestamp = src->timestamp;
    dst->rx_timestamp_captured = src->timestamp_captured ? 1U : 0U;
    dst->rx_timestamp_pointer = src->timestamp_pointer;
    dst->dlc = src->dlc;
    dst->bitrate_switch = src->bitrate_switch ? 1U : 0U;
    dst->canfd_frame = src->fd_frame ? 1U : 0U;
    dst->filter_index = src->filter_index;
    dst->accepted_non_matching_frame = src->accepted_non_matching_frame ? 1U : 0U;

    const uint8_t payload_size = mcan_get_message_size_from_dlc(src->dlc);
    if (payload_size > 0U) {
        memcpy(dst->data_8, src->data, payload_size);
    }
}

int32_t bsp_mcan_init(bsp_mcan_instance_t can_instance, bool internal_loopback)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }

    bsp_mcan_config_t config{};
    int32_t status = bsp_mcan_get_default_config(can_instance, &config, false);
    if (status != BSP_OK) {
        return status;
    }
    config.mode = internal_loopback ? mcan_mode_loopback_internal : mcan_mode_normal;
    return bsp_mcan_init_ex(can_instance, &config, BSP_IRQ_PRIORITY_DEFAULT);
}

int32_t bsp_mcan_get_default_config(bsp_mcan_instance_t can_instance, bsp_mcan_config_t *config, bool enable_canfd)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    return bsp_mcan_get_default_config_internal(&s_mcan_map[can_instance], config, enable_canfd);
}

int32_t bsp_mcan_get_clock(bsp_mcan_instance_t can_instance, uint32_t *freq_hz)
{
    if ((can_instance >= BSP_MCAN_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    return bsp_mcan_prepare_instance(&s_mcan_map[can_instance], freq_hz);
}

int32_t bsp_mcan_config_bitrate(bsp_mcan_instance_t can_instance,
                                bsp_mcan_config_t *config,
                                uint32_t nominal_bitrate,
                                uint32_t data_bitrate)
{
    if ((can_instance >= BSP_MCAN_MAX) || (config == nullptr) || (nominal_bitrate == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_AUTO_BITRATE)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    config->use_lowlevel_timing_setting = false;
    config->baudrate = nominal_bitrate;
    config->baudrate_fd = config->enable_canfd ? ((data_bitrate != 0U) ? data_bitrate : nominal_bitrate) : 0U;
    return BSP_OK;
}

static int32_t bsp_mcan_validate_frame_features(const mcan_instance_map_t &map, const bsp_mcan_frame_t *frame)
{
    const auto validate_status = bsp_mcan_validate_frame(frame);
    if (validate_status != BSP_OK) {
        return validate_status;
    }

    const uint8_t payload_size = mcan_get_message_size_from_dlc(frame->dlc);
    if (frame->fd_frame || (payload_size > 8U)) {
        if (!bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_CAN_FD)) {
            return BSP_ERROR_UNSUPPORTED;
        }
        if (frame->bitrate_switch && !bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_BITRATE_SWITCH)) {
            return BSP_ERROR_UNSUPPORTED;
        }
    } else if (!bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_CLASSIC)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (frame->remote_frame && !bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_REMOTE_FRAME)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (frame->tx_event_fifo_control && !bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_TX_EVENT_FIFO)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (frame->timestamp_capture_enable && !bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_TIMESTAMP)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (frame->extended_id) {
        return bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_EXTENDED_ID) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }
    return bsp_mcan_has_feature(map, BSP_MCAN_FEATURE_STANDARD_ID) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}


int32_t bsp_mcan_init_ex(bsp_mcan_instance_t can_instance, const bsp_mcan_config_t *config, uint8_t irq_priority)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    return bsp_mcan_configure_instance(&s_mcan_map[can_instance], config, irq_priority);
}

int32_t bsp_mcan_deinit(bsp_mcan_instance_t can_instance)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_OK;
    }

    mcan_deinit(map->base);
    intc_m_disable_irq(map->irq);
    map->initialized = false;
    return BSP_OK;
}

int32_t bsp_mcan_transmit(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *frame)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }

    const int32_t validate_status = bsp_mcan_validate_frame(frame);
    if (validate_status != BSP_OK) {
        return validate_status;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_TX_BLOCKING)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const int32_t init_status = bsp_mcan_ensure_initialized_for_frame(can_instance, frame);
    if (init_status != BSP_OK) {
        return init_status;
    }

    mcan_tx_frame_t tx_frame{};
    bsp_mcan_copy_to_hw(frame, &tx_frame);
    return bsp_mcan_status_from_hpm(mcan_transmit_blocking(s_mcan_map[can_instance].base, &tx_frame));
}

int32_t bsp_mcan_transmit_fifo(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *frame, uint32_t *fifo_index)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }

    const int32_t validate_status = bsp_mcan_validate_frame(frame);
    if (validate_status != BSP_OK) {
        return validate_status;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_TX_FIFO)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const int32_t init_status = bsp_mcan_ensure_initialized_for_frame(can_instance, frame);
    if (init_status != BSP_OK) {
        return init_status;
    }

    mcan_tx_frame_t tx_frame{};
    bsp_mcan_copy_to_hw(frame, &tx_frame);
    return bsp_mcan_status_from_hpm(mcan_transmit_via_txfifo_nonblocking(s_mcan_map[can_instance].base, &tx_frame, fifo_index));
}

int32_t bsp_mcan_transmit_buffer(bsp_mcan_instance_t can_instance, uint32_t buffer_index, const bsp_mcan_frame_t *frame)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }

    const int32_t validate_status = bsp_mcan_validate_frame(frame);
    if (validate_status != BSP_OK) {
        return validate_status;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_TX_BUFFER)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const int32_t init_status = bsp_mcan_ensure_initialized_for_frame(can_instance, frame);
    if (init_status != BSP_OK) {
        return init_status;
    }

    mcan_tx_frame_t tx_frame{};
    bsp_mcan_copy_to_hw(frame, &tx_frame);
    return bsp_mcan_status_from_hpm(mcan_transmit_via_txbuf_nonblocking(s_mcan_map[can_instance].base, buffer_index, &tx_frame));
}

int32_t bsp_mcan_receive(bsp_mcan_instance_t can_instance, bsp_mcan_frame_t *frame)
{
    return bsp_mcan_receive_fifo(can_instance, 0U, frame);
}

int32_t bsp_mcan_receive_fifo(bsp_mcan_instance_t can_instance, uint32_t fifo_index, bsp_mcan_frame_t *frame)
{
    if ((can_instance >= BSP_MCAN_MAX) || (frame == nullptr) || (fifo_index > 1U)) {
        return BSP_ERROR_PARAM;
    }
    if ((fifo_index == 0U) && !bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_RX_FIFO0)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((fifo_index == 1U) && !bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_RX_FIFO1)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }

    mcan_rx_message_t rx_message{};
    const hpm_stat_t read_status = mcan_read_rxfifo(map->base, fifo_index, &rx_message);
    if (read_status != status_success) {
        return bsp_mcan_status_from_hpm(read_status);
    }

    const uint32_t flag = (fifo_index == 0U) ? MCAN_INT_RXFIFO0_NEW_MSG : MCAN_INT_RXFIFO1_NEW_MSG;
    mcan_clear_interrupt_flags(map->base, flag);
    bsp_mcan_copy_from_hw(&rx_message, (uint8_t) fifo_index, frame);
    return BSP_OK;
}

int32_t bsp_mcan_receive_buffer(bsp_mcan_instance_t can_instance, uint32_t buffer_index, bsp_mcan_frame_t *frame)
{
    if ((can_instance >= BSP_MCAN_MAX) || (frame == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_RX_BUFFER)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }
    if (!mcan_is_rxbuf_data_available(map->base, buffer_index)) {
        return BSP_ERROR_BUSY;
    }

    mcan_rx_message_t rx_message{};
    const hpm_stat_t read_status = mcan_read_rxbuf(map->base, buffer_index, &rx_message);
    if (read_status != status_success) {
        return bsp_mcan_status_from_hpm(read_status);
    }

    mcan_clear_rxbuf_data_available_flag(map->base, buffer_index);
    mcan_clear_interrupt_flags(map->base, MCAN_INT_MSG_STORE_TO_RXBUF);
    bsp_mcan_copy_from_hw(&rx_message, 0xFFU, frame);
    return BSP_OK;
}

int32_t bsp_mcan_loopback(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *tx_frame, bsp_mcan_frame_t *rx_frame)
{
    if ((can_instance >= BSP_MCAN_MAX) || (tx_frame == nullptr) || (rx_frame == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_INTERNAL_LOOPBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const int32_t validate_status = bsp_mcan_validate_frame_features(s_mcan_map[can_instance], tx_frame);
    if (validate_status != BSP_OK) {
        return validate_status;
    }

    bsp_mcan_config_t config{};
    const bool enable_canfd = tx_frame->fd_frame || (tx_frame->dlc > MCAN_MSG_DLC_8_BYTES);
    int32_t status = bsp_mcan_get_default_config(can_instance, &config, enable_canfd);
    if (status != BSP_OK) {
        return status;
    }
    config.mode = mcan_mode_loopback_internal;
    status = bsp_mcan_init_ex(can_instance, &config, BSP_IRQ_PRIORITY_DEFAULT);
    if (status != BSP_OK) {
        return status;
    }

    status = bsp_mcan_transmit(can_instance, tx_frame);
    if (status != BSP_OK) {
        return status;
    }

    for (uint32_t retry = 0; retry < 100000U; retry++) {
        status = bsp_mcan_receive(can_instance, rx_frame);
        if (status == BSP_OK) {
            return BSP_OK;
        }
    }

    return BSP_ERROR_TIMEOUT;
}

int32_t bsp_mcan_config_filters(bsp_mcan_instance_t can_instance, const bsp_mcan_all_filters_config_t *config)
{
    if ((can_instance >= BSP_MCAN_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_FILTERS)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }
    return bsp_mcan_status_from_hpm(mcan_config_all_filters(map->base, const_cast<bsp_mcan_all_filters_config_t *>(config)));
}

int32_t bsp_mcan_set_global_filter(bsp_mcan_instance_t can_instance, const bsp_mcan_global_filter_config_t *config)
{
    if ((can_instance >= BSP_MCAN_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_GLOBAL_FILTER)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }
    return bsp_mcan_status_from_hpm(mcan_set_global_filter_config(map->base, const_cast<bsp_mcan_global_filter_config_t *>(config)));
}

int32_t bsp_mcan_read_tx_event(bsp_mcan_instance_t can_instance, bsp_mcan_tx_event_fifo_elem_t *event)
{
    if ((can_instance >= BSP_MCAN_MAX) || (event == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_TX_EVENT_FIFO)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }

    const hpm_stat_t status = mcan_read_tx_evt_fifo(map->base, event);
    if (status == status_success) {
        mcan_clear_interrupt_flags(map->base, MCAN_INT_TX_EVT_FIFO_NEW_ENTRY);
    }
    return bsp_mcan_status_from_hpm(status);
}

int32_t bsp_mcan_get_protocol_status(bsp_mcan_instance_t can_instance, bsp_mcan_protocol_status_t *status)
{
    if ((can_instance >= BSP_MCAN_MAX) || (status == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_PROTOCOL_STATUS)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }
    return bsp_mcan_status_from_hpm(mcan_get_protocol_status(map->base, status));
}

int32_t bsp_mcan_get_error_count(bsp_mcan_instance_t can_instance, bsp_mcan_error_count_t *count)
{
    if ((can_instance >= BSP_MCAN_MAX) || (count == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_ERROR_COUNT)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }
    mcan_get_error_counter(map->base, count);
    return BSP_OK;
}

int32_t bsp_mcan_get_timestamp_from_tx_event(bsp_mcan_instance_t can_instance, const bsp_mcan_tx_event_fifo_elem_t *event, bsp_mcan_timestamp_value_t *timestamp)
{
    if ((can_instance >= BSP_MCAN_MAX) || (event == nullptr) || (timestamp == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_TIMESTAMP) ||
        !bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_TX_EVENT_FIFO)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }
    return bsp_mcan_status_from_hpm(mcan_get_timestamp_from_tx_event(map->base, event, timestamp));
}

int32_t bsp_mcan_get_timestamp_from_frame(bsp_mcan_instance_t can_instance, const bsp_mcan_frame_t *frame, bsp_mcan_timestamp_value_t *timestamp)
{
    if ((can_instance >= BSP_MCAN_MAX) || (frame == nullptr) || (timestamp == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_TIMESTAMP)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }

    mcan_rx_message_t rx_message{};
    bsp_mcan_copy_to_rx_hw(frame, &rx_message);
    return bsp_mcan_status_from_hpm(mcan_get_timestamp_from_received_message(map->base, &rx_message, timestamp));
}

int32_t bsp_mcan_cancel_tx_request(bsp_mcan_instance_t can_instance, uint32_t buffer_index)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_CANCEL_TX)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }
    mcan_cancel_tx_buf_send_request(map->base, buffer_index);
    return BSP_OK;
}

int32_t bsp_mcan_is_tx_request_cancelled(bsp_mcan_instance_t can_instance, uint32_t buffer_index, bool *finished)
{
    if ((can_instance >= BSP_MCAN_MAX) || (finished == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_CANCEL_TX)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    mcan_instance_map_t *map = &s_mcan_map[can_instance];
    if (!map->initialized) {
        return BSP_ERROR;
    }
    *finished = mcan_is_tx_buf_cancellation_finished(map->base, buffer_index);
    return BSP_OK;
}

int32_t bsp_mcan_register_rx_callback(bsp_mcan_instance_t can_instance, bsp_mcan_rx_callback_t callback, void *arg)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_RX_CALLBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    s_mcan_map[can_instance].rx_callback = callback;
    s_mcan_map[can_instance].rx_callback_arg = arg;
    return BSP_OK;
}

int32_t bsp_mcan_register_tx_callback(bsp_mcan_instance_t can_instance, bsp_mcan_tx_callback_t callback, void *arg)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_TX_CALLBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    s_mcan_map[can_instance].tx_callback = callback;
    s_mcan_map[can_instance].tx_callback_arg = arg;
    return BSP_OK;
}

int32_t bsp_mcan_register_event_callback(bsp_mcan_instance_t can_instance, bsp_mcan_event_callback_t callback, void *arg)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!bsp_mcan_has_feature(s_mcan_map[can_instance], BSP_MCAN_FEATURE_EVENT_CALLBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    s_mcan_map[can_instance].event_callback = callback;
    s_mcan_map[can_instance].event_callback_arg = arg;
    return BSP_OK;
}

extern "C" void bsp_mcan_irq_handler(bsp_mcan_instance_t can_instance)
{
    if (can_instance >= BSP_MCAN_MAX) {
        return;
    }

    auto &map = s_mcan_map[can_instance];
    const uint32_t flags = mcan_get_interrupt_flags(map.base);
    if (flags == 0U) {
        return;
    }

    if (map.rx_callback != nullptr) {
        if ((flags & MCAN_INT_RXFIFO0_NEW_MSG) != 0U) {
            while (true) {
                bsp_mcan_frame_t frame{};
                if (bsp_mcan_receive_fifo(can_instance, 0U, &frame) != BSP_OK) {
                    break;
                }
                map.rx_callback(&frame, map.rx_callback_arg);
            }
        }
        if ((flags & MCAN_INT_RXFIFO1_NEW_MSG) != 0U) {
            while (true) {
                bsp_mcan_frame_t frame{};
                if (bsp_mcan_receive_fifo(can_instance, 1U, &frame) != BSP_OK) {
                    break;
                }
                map.rx_callback(&frame, map.rx_callback_arg);
            }
        }
    }

    if (((flags & MCAN_INT_TX_COMPLETED) != 0U) && (map.tx_callback != nullptr)) {
        map.tx_callback(map.tx_callback_arg);
    }

    if (map.event_callback != nullptr) {
        map.event_callback(flags, map.event_callback_arg);
    }

    mcan_clear_interrupt_flags(map.base, flags);
}
