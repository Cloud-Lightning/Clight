#include "bsp_fdcan.h"

#include <cstdint>
#include <cstring>

extern "C" {
#include "fdcan.h"
}

typedef struct {
    uint32_t prescaler;
    uint32_t sync_jump_width;
    uint32_t time_seg1;
    uint32_t time_seg2;
    uint32_t actual_bitrate;
} fdcan_timing_t;

typedef struct {
    uint32_t prescaler_max;
    uint32_t sjw_max;
    uint32_t seg1_max;
    uint32_t seg2_max;
    uint32_t target_sample_per_mille;
    uint32_t max_error_ppm;
} fdcan_timing_limits_t;

typedef struct {
    FDCAN_GlobalTypeDef *base;
    IRQn_Type irq;
    bool has_pinmux;
    uint32_t ram_offset;
    uint32_t features;
    FDCAN_HandleTypeDef *handle;
    void (*init_fn)(void);
    bool initialized;
    bsp_fdcan_config_t config;
    bsp_fdcan_rx_callback_t rx_callback;
    void *rx_callback_arg;
    bsp_fdcan_tx_callback_t tx_callback;
    void *tx_callback_arg;
} fdcan_map_t;

static void fdcan_init_fn_main()
{
    MX_FDCAN1_Init();
}

static void fdcan_init_fn_aux()
{
    MX_FDCAN2_Init();
}

#define BSP_FDCAN_MAP_ITEM(name, base, irq, has_pinmux, ram_offset, features) \
    { base, irq, (has_pinmux) != 0U, ram_offset, static_cast<uint32_t>(features), nullptr, nullptr, false, {}, nullptr, nullptr, nullptr, nullptr },
static fdcan_map_t s_fdcan_map[BSP_FDCAN_MAX] = {
    BSP_FDCAN_LIST(BSP_FDCAN_MAP_ITEM)
};
#undef BSP_FDCAN_MAP_ITEM

static bool fdcan_has_feature(const fdcan_map_t &map, uint32_t feature)
{
    return (map.features & feature) != 0U;
}

static void attach_fdcan_bindings()
{
    static bool attached = false;
    if (attached) {
        return;
    }

    s_fdcan_map[BSP_FDCAN_MAIN].handle = &hfdcan1;
    s_fdcan_map[BSP_FDCAN_MAIN].init_fn = fdcan_init_fn_main;
    s_fdcan_map[BSP_FDCAN_AUX].handle = &hfdcan2;
    s_fdcan_map[BSP_FDCAN_AUX].init_fn = fdcan_init_fn_aux;
    attached = true;
}

static bool is_valid_payload_size(std::uint8_t bytes)
{
    switch (bytes) {
    case 0U:
    case 1U:
    case 2U:
    case 3U:
    case 4U:
    case 5U:
    case 6U:
    case 7U:
    case 8U:
    case 12U:
    case 16U:
    case 20U:
    case 24U:
    case 32U:
    case 48U:
    case 64U:
        return true;
    default:
        return false;
    }
}

static uint32_t dlc_to_hal(std::uint8_t bytes)
{
    switch (bytes) {
    case 0U: return FDCAN_DLC_BYTES_0;
    case 1U: return FDCAN_DLC_BYTES_1;
    case 2U: return FDCAN_DLC_BYTES_2;
    case 3U: return FDCAN_DLC_BYTES_3;
    case 4U: return FDCAN_DLC_BYTES_4;
    case 5U: return FDCAN_DLC_BYTES_5;
    case 6U: return FDCAN_DLC_BYTES_6;
    case 7U: return FDCAN_DLC_BYTES_7;
    case 8U: return FDCAN_DLC_BYTES_8;
    case 12U: return FDCAN_DLC_BYTES_12;
    case 16U: return FDCAN_DLC_BYTES_16;
    case 20U: return FDCAN_DLC_BYTES_20;
    case 24U: return FDCAN_DLC_BYTES_24;
    case 32U: return FDCAN_DLC_BYTES_32;
    case 48U: return FDCAN_DLC_BYTES_48;
    default: return FDCAN_DLC_BYTES_64;
    }
}

static std::uint8_t hal_to_dlc(uint32_t dlc)
{
    switch (dlc) {
    case FDCAN_DLC_BYTES_0: return 0U;
    case FDCAN_DLC_BYTES_1: return 1U;
    case FDCAN_DLC_BYTES_2: return 2U;
    case FDCAN_DLC_BYTES_3: return 3U;
    case FDCAN_DLC_BYTES_4: return 4U;
    case FDCAN_DLC_BYTES_5: return 5U;
    case FDCAN_DLC_BYTES_6: return 6U;
    case FDCAN_DLC_BYTES_7: return 7U;
    case FDCAN_DLC_BYTES_8: return 8U;
    case FDCAN_DLC_BYTES_12: return 12U;
    case FDCAN_DLC_BYTES_16: return 16U;
    case FDCAN_DLC_BYTES_20: return 20U;
    case FDCAN_DLC_BYTES_24: return 24U;
    case FDCAN_DLC_BYTES_32: return 32U;
    case FDCAN_DLC_BYTES_48: return 48U;
    default: return 64U;
    }
}

static uint32_t element_size_to_hal(bool enable_can_fd)
{
    return enable_can_fd ? FDCAN_DATA_BYTES_64 : FDCAN_DATA_BYTES_8;
}

static uint32_t fdcan_kernel_clock_hz()
{
    auto clock = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_FDCAN);
    if (clock == 0U) {
        clock = HAL_RCC_GetPCLK1Freq();
    }
    return clock;
}

static uint64_t abs_diff_u64(uint64_t lhs, uint64_t rhs)
{
    return (lhs > rhs) ? (lhs - rhs) : (rhs - lhs);
}

static uint32_t clamp_u32(uint32_t value, uint32_t min_value, uint32_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static bool calculate_bit_timing(uint32_t kernel_clock_hz,
                                 uint32_t bitrate,
                                 const fdcan_timing_limits_t &limits,
                                 fdcan_timing_t &timing)
{
    if ((kernel_clock_hz == 0U) || (bitrate == 0U)) {
        return false;
    }

    const uint32_t min_total_tq = 3U;
    const uint32_t max_total_tq = 1U + limits.seg1_max + limits.seg2_max;
    bool found = false;
    uint64_t best_error = UINT64_MAX;
    uint32_t best_sample_error = UINT32_MAX;
    uint32_t best_total_tq = 0U;
    fdcan_timing_t best{};

    for (uint32_t prescaler = 1U; prescaler <= limits.prescaler_max; ++prescaler) {
        for (uint32_t total_tq = min_total_tq; total_tq <= max_total_tq; ++total_tq) {
            const uint64_t target_clock = static_cast<uint64_t>(bitrate) * prescaler * total_tq;
            const auto bitrate_error = abs_diff_u64(kernel_clock_hz, target_clock);

            const uint32_t sample_tq =
                clamp_u32((total_tq * limits.target_sample_per_mille + 500U) / 1000U, 2U, total_tq - 1U);
            const uint32_t seg1 = sample_tq - 1U;
            const uint32_t seg2 = total_tq - 1U - seg1;
            if ((seg1 == 0U) || (seg2 == 0U) || (seg1 > limits.seg1_max) || (seg2 > limits.seg2_max)) {
                continue;
            }

            const uint32_t actual_sample_per_mille = ((1U + seg1) * 1000U) / total_tq;
            const uint32_t sample_error = (actual_sample_per_mille > limits.target_sample_per_mille)
                                              ? (actual_sample_per_mille - limits.target_sample_per_mille)
                                              : (limits.target_sample_per_mille - actual_sample_per_mille);

            if (!found ||
                (bitrate_error < best_error) ||
                ((bitrate_error == best_error) && (sample_error < best_sample_error)) ||
                ((bitrate_error == best_error) && (sample_error == best_sample_error) && (total_tq > best_total_tq))) {
                found = true;
                best_error = bitrate_error;
                best_sample_error = sample_error;
                best_total_tq = total_tq;
                best.prescaler = prescaler;
                best.sync_jump_width = (seg2 > limits.sjw_max) ? limits.sjw_max : seg2;
                best.time_seg1 = seg1;
                best.time_seg2 = seg2;
                best.actual_bitrate = static_cast<uint32_t>(kernel_clock_hz / (prescaler * total_tq));
            }
        }
    }

    if (!found) {
        return false;
    }

    const uint64_t requested_clock = static_cast<uint64_t>(bitrate) * best.prescaler * best_total_tq;
    const uint64_t error_ppm = (best_error * 1000000ULL) / requested_clock;
    if (error_ppm > limits.max_error_ppm) {
        return false;
    }

    timing = best;
    return true;
}

static bool config_has_nominal_timing(const bsp_fdcan_config_t &config)
{
    return (config.nominal_prescaler != 0U) &&
           (config.nominal_sync_jump_width != 0U) &&
           (config.nominal_time_seg1 != 0U) &&
           (config.nominal_time_seg2 != 0U);
}

static bool config_has_data_timing(const bsp_fdcan_config_t &config)
{
    return (config.data_prescaler != 0U) &&
           (config.data_sync_jump_width != 0U) &&
           (config.data_time_seg1 != 0U) &&
           (config.data_time_seg2 != 0U);
}

static int32_t configure_accept_all_filters(fdcan_map_t &map)
{
    if (map.config.std_filter_count > 0U) {
        FDCAN_FilterTypeDef std_filter = {};
        std_filter.IdType = FDCAN_STANDARD_ID;
        std_filter.FilterIndex = 0U;
        std_filter.FilterType = FDCAN_FILTER_MASK;
        std_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        std_filter.FilterID1 = 0U;
        std_filter.FilterID2 = 0U;
        if (HAL_FDCAN_ConfigFilter(map.handle, &std_filter) != HAL_OK) {
            return BSP_ERROR;
        }
    }

    if (map.config.ext_filter_count > 0U) {
        FDCAN_FilterTypeDef ext_filter = {};
        ext_filter.IdType = FDCAN_EXTENDED_ID;
        ext_filter.FilterIndex = 0U;
        ext_filter.FilterType = FDCAN_FILTER_MASK;
        ext_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        ext_filter.FilterID1 = 0U;
        ext_filter.FilterID2 = 0U;
        if (HAL_FDCAN_ConfigFilter(map.handle, &ext_filter) != HAL_OK) {
            return BSP_ERROR;
        }
    }

    return (HAL_FDCAN_ConfigGlobalFilter(map.handle,
                                         FDCAN_ACCEPT_IN_RX_FIFO0,
                                         FDCAN_ACCEPT_IN_RX_FIFO0,
                                         FDCAN_REJECT_REMOTE,
                                         FDCAN_REJECT_REMOTE) == HAL_OK)
               ? BSP_OK
               : BSP_ERROR;
}

static int32_t validate_fdcan_config(const fdcan_map_t &map, const bsp_fdcan_config_t &config)
{
    if (config.enable_can_fd) {
        if (!fdcan_has_feature(map, BSP_FDCAN_FEATURE_CAN_FD)) {
            return BSP_ERROR_UNSUPPORTED;
        }
        if (config.bitrate_switch && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_BITRATE_SWITCH)) {
            return BSP_ERROR_UNSUPPORTED;
        }
    } else if (!fdcan_has_feature(map, BSP_FDCAN_FEATURE_CLASSIC)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (config.internal_loopback && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_INTERNAL_LOOPBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((config.tx_fifo_elements > 0U) && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_TX_FIFO)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((config.tx_event_fifo_elements > 0U) && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_TX_EVENT_FIFO)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (config.timestamp_enable && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_TIMESTAMP)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if ((config.rx_fifo0_elements > 0U) && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_RX_FIFO0)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (((config.std_filter_count > 0U) || (config.ext_filter_count > 0U)) &&
        !fdcan_has_feature(map, BSP_FDCAN_FEATURE_FILTER_MASK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    return BSP_OK;
}

static int32_t validate_fdcan_frame_features(const fdcan_map_t &map, const bsp_fdcan_frame_t &frame)
{
    if (frame.fd_frame || (frame.dlc > 8U)) {
        if (!fdcan_has_feature(map, BSP_FDCAN_FEATURE_CAN_FD)) {
            return BSP_ERROR_UNSUPPORTED;
        }
        if (frame.bitrate_switch && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_BITRATE_SWITCH)) {
            return BSP_ERROR_UNSUPPORTED;
        }
    } else if (!fdcan_has_feature(map, BSP_FDCAN_FEATURE_CLASSIC)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (frame.remote_frame && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_REMOTE_FRAME)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (frame.extended_id) {
        return fdcan_has_feature(map, BSP_FDCAN_FEATURE_EXTENDED_ID) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
    }
    return fdcan_has_feature(map, BSP_FDCAN_FEATURE_STANDARD_ID) ? BSP_OK : BSP_ERROR_UNSUPPORTED;
}

static int32_t configure_fdcan(fdcan_map_t &map, const bsp_fdcan_config_t &config)
{
    if (!map.has_pinmux) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (map.handle == nullptr || map.init_fn == nullptr) {
        return BSP_ERROR_PARAM;
    }
    const auto config_status = validate_fdcan_config(map, config);
    if (config_status != BSP_OK) {
        return config_status;
    }

    map.init_fn();
    (void) HAL_FDCAN_Stop(map.handle);
    (void) HAL_FDCAN_DeInit(map.handle);

    map.config = config;
    map.handle->Init.FrameFormat = config.enable_can_fd
                                       ? (config.bitrate_switch ? FDCAN_FRAME_FD_BRS : FDCAN_FRAME_FD_NO_BRS)
                                       : FDCAN_FRAME_CLASSIC;
    map.handle->Init.Mode = config.internal_loopback ? FDCAN_MODE_INTERNAL_LOOPBACK : FDCAN_MODE_NORMAL;
    if (config_has_nominal_timing(config)) {
        map.handle->Init.NominalPrescaler = config.nominal_prescaler;
        map.handle->Init.NominalSyncJumpWidth = config.nominal_sync_jump_width;
        map.handle->Init.NominalTimeSeg1 = config.nominal_time_seg1;
        map.handle->Init.NominalTimeSeg2 = config.nominal_time_seg2;
    }
    if (config_has_data_timing(config)) {
        map.handle->Init.DataPrescaler = config.data_prescaler;
        map.handle->Init.DataSyncJumpWidth = config.data_sync_jump_width;
        map.handle->Init.DataTimeSeg1 = config.data_time_seg1;
        map.handle->Init.DataTimeSeg2 = config.data_time_seg2;
    }
    map.handle->Init.MessageRAMOffset = map.ram_offset;
    map.handle->Init.StdFiltersNbr = config.std_filter_count;
    map.handle->Init.ExtFiltersNbr = config.ext_filter_count;
    map.handle->Init.RxFifo0ElmtsNbr = config.rx_fifo0_elements;
    map.handle->Init.RxFifo0ElmtSize = element_size_to_hal(config.enable_can_fd);
    map.handle->Init.RxFifo1ElmtsNbr = 0U;
    map.handle->Init.RxBuffersNbr = 0U;
    map.handle->Init.TxEventsNbr = config.tx_event_fifo_elements;
    map.handle->Init.TxBuffersNbr = 0U;
    map.handle->Init.TxFifoQueueElmtsNbr = config.tx_fifo_elements;
    map.handle->Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    map.handle->Init.TxElmtSize = element_size_to_hal(config.enable_can_fd);

    if (HAL_FDCAN_Init(map.handle) != HAL_OK) {
        map.initialized = false;
        return BSP_ERROR;
    }
    if (configure_accept_all_filters(map) != BSP_OK) {
        map.initialized = false;
        return BSP_ERROR;
    }
    if (config.timestamp_enable) {
        const uint32_t prescaler = (config.timestamp_prescaler == 0U) ? FDCAN_TIMESTAMP_PRESC_1 : config.timestamp_prescaler;
        if (HAL_FDCAN_ConfigTimestampCounter(map.handle, prescaler) != HAL_OK) {
            map.initialized = false;
            return BSP_ERROR;
        }
        if (HAL_FDCAN_EnableTimestampCounter(map.handle, FDCAN_TIMESTAMP_INTERNAL) != HAL_OK) {
            map.initialized = false;
            return BSP_ERROR;
        }
    }
    if (HAL_FDCAN_Start(map.handle) != HAL_OK) {
        map.initialized = false;
        return BSP_ERROR;
    }

    (void) HAL_FDCAN_ActivateNotification(
        map.handle,
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_TX_COMPLETE | FDCAN_IT_TX_EVT_FIFO_NEW_DATA,
        FDCAN_TX_BUFFER0);

    map.initialized = true;
    return BSP_OK;
}

static int32_t ensure_fdcan_initialized(bsp_fdcan_instance_t can_instance, bool loopback)
{
    if (can_instance >= BSP_FDCAN_MAX) {
        return BSP_ERROR_PARAM;
    }

    attach_fdcan_bindings();
    bsp_fdcan_config_t config{};
    const auto config_status = bsp_fdcan_get_default_config(can_instance, &config, false);
    if (config_status != BSP_OK) {
        return config_status;
    }
    config.internal_loopback = loopback;
    return configure_fdcan(s_fdcan_map[can_instance], config);
}

static int32_t ensure_started(bsp_fdcan_instance_t can_instance)
{
    if (can_instance >= BSP_FDCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    attach_fdcan_bindings();
    if (!s_fdcan_map[can_instance].initialized) {
        return ensure_fdcan_initialized(can_instance, false);
    }
    return BSP_OK;
}

int32_t bsp_fdcan_get_default_config(bsp_fdcan_instance_t can_instance, bsp_fdcan_config_t *config, bool enable_can_fd)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    attach_fdcan_bindings();
    const auto &map = s_fdcan_map[can_instance];
    if (enable_can_fd && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_CAN_FD)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!enable_can_fd && !fdcan_has_feature(map, BSP_FDCAN_FEATURE_CLASSIC)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    config->enable_can_fd = enable_can_fd;
    config->bitrate_switch = enable_can_fd;
    config->internal_loopback = false;
    config->std_filter_count = 1U;
    config->ext_filter_count = 1U;
    config->rx_fifo0_elements = enable_can_fd ? 2U : 3U;
    config->tx_fifo_elements = enable_can_fd ? 2U : 3U;
    config->tx_event_fifo_elements = 0U;
    config->timestamp_enable = false;
    config->timestamp_prescaler = FDCAN_TIMESTAMP_PRESC_1;
    return BSP_OK;
}

int32_t bsp_fdcan_get_clock(bsp_fdcan_instance_t can_instance, uint32_t *freq_hz)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (freq_hz == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    *freq_hz = fdcan_kernel_clock_hz();
    return (*freq_hz != 0U) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_fdcan_config_bitrate(bsp_fdcan_instance_t can_instance,
                                 bsp_fdcan_config_t *config,
                                 uint32_t nominal_bitrate,
                                 uint32_t data_bitrate)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (config == nullptr) || (nominal_bitrate == 0U)) {
        return BSP_ERROR_PARAM;
    }
    attach_fdcan_bindings();
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_AUTO_BITRATE)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    const auto kernel_clock = fdcan_kernel_clock_hz();
    if (kernel_clock == 0U) {
        return BSP_ERROR;
    }

    fdcan_timing_t nominal{};
    const fdcan_timing_limits_t nominal_limits{
        512U,
        128U,
        256U,
        128U,
        800U,
        5000U,
    };
    if (!calculate_bit_timing(kernel_clock, nominal_bitrate, nominal_limits, nominal)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    config->nominal_bitrate = nominal.actual_bitrate;
    config->nominal_prescaler = nominal.prescaler;
    config->nominal_sync_jump_width = nominal.sync_jump_width;
    config->nominal_time_seg1 = nominal.time_seg1;
    config->nominal_time_seg2 = nominal.time_seg2;

    if (config->enable_can_fd) {
        if (data_bitrate == 0U) {
            data_bitrate = nominal_bitrate;
        }

        fdcan_timing_t data{};
        const fdcan_timing_limits_t data_limits{
            32U,
            16U,
            32U,
            16U,
            750U,
            5000U,
        };
        if (!calculate_bit_timing(kernel_clock, data_bitrate, data_limits, data)) {
            return BSP_ERROR_UNSUPPORTED;
        }

        config->data_bitrate = data.actual_bitrate;
        config->data_prescaler = data.prescaler;
        config->data_sync_jump_width = data.sync_jump_width;
        config->data_time_seg1 = data.time_seg1;
        config->data_time_seg2 = data.time_seg2;
    } else {
        config->data_bitrate = 0U;
        config->data_prescaler = 0U;
        config->data_sync_jump_width = 0U;
        config->data_time_seg1 = 0U;
        config->data_time_seg2 = 0U;
    }

    return BSP_OK;
}

int32_t bsp_fdcan_init(bsp_fdcan_instance_t can_instance, bool internal_loopback)
{
    return ensure_fdcan_initialized(can_instance, internal_loopback);
}

int32_t bsp_fdcan_init_ex(bsp_fdcan_instance_t can_instance, const bsp_fdcan_config_t *config)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if ((config->std_filter_count == 0U) && (config->ext_filter_count == 0U)) {
        return BSP_ERROR_PARAM;
    }
    if ((config->rx_fifo0_elements == 0U) || (config->tx_fifo_elements == 0U)) {
        return BSP_ERROR_PARAM;
    }

    attach_fdcan_bindings();
    return configure_fdcan(s_fdcan_map[can_instance], *config);
}

int32_t bsp_fdcan_transmit(bsp_fdcan_instance_t can_instance, const bsp_fdcan_frame_t *frame)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (frame == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!is_valid_payload_size(frame->dlc)) {
        return BSP_ERROR_PARAM;
    }

    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    const auto &map = s_fdcan_map[can_instance];
    const auto feature_status = validate_fdcan_frame_features(map, *frame);
    if (feature_status != BSP_OK) {
        return feature_status;
    }
    if (!fdcan_has_feature(map, BSP_FDCAN_FEATURE_TX_FIFO)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    if (!map.config.enable_can_fd && (frame->fd_frame || (frame->dlc > 8U))) {
        return BSP_ERROR_PARAM;
    }

    FDCAN_TxHeaderTypeDef header = {};
    header.Identifier = frame->id;
    header.IdType = frame->extended_id ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    header.TxFrameType = frame->remote_frame ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
    header.DataLength = dlc_to_hal(frame->dlc);
    header.ErrorStateIndicator = frame->error_state_indicator ? FDCAN_ESI_PASSIVE : FDCAN_ESI_ACTIVE;
    header.BitRateSwitch = frame->bitrate_switch ? FDCAN_BRS_ON : FDCAN_BRS_OFF;
    header.FDFormat = frame->fd_frame ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;
    header.TxEventFifoControl = (map.config.tx_event_fifo_elements > 0U) ? FDCAN_STORE_TX_EVENTS : FDCAN_NO_TX_EVENTS;
    header.MessageMarker = 0U;

    return (HAL_FDCAN_AddMessageToTxFifoQ(map.handle, &header, const_cast<uint8_t *>(frame->data)) == HAL_OK)
               ? BSP_OK
               : BSP_ERROR_BUSY;
}

int32_t bsp_fdcan_receive(bsp_fdcan_instance_t can_instance, bsp_fdcan_frame_t *frame)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (frame == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_RX_FIFO0)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    auto *handle = s_fdcan_map[can_instance].handle;
    if (HAL_FDCAN_GetRxFifoFillLevel(handle, FDCAN_RX_FIFO0) == 0U) {
        return BSP_ERROR_TIMEOUT;
    }

    FDCAN_RxHeaderTypeDef header = {};
    std::memset(frame, 0, sizeof(*frame));
    if (HAL_FDCAN_GetRxMessage(handle, FDCAN_RX_FIFO0, &header, frame->data) != HAL_OK) {
        return BSP_ERROR;
    }

    frame->id = header.Identifier;
    frame->dlc = hal_to_dlc(header.DataLength);
    frame->extended_id = (header.IdType == FDCAN_EXTENDED_ID);
    frame->remote_frame = (header.RxFrameType == FDCAN_REMOTE_FRAME);
    frame->fd_frame = (header.FDFormat == FDCAN_FD_CAN);
    frame->bitrate_switch = (header.BitRateSwitch == FDCAN_BRS_ON);
    frame->error_state_indicator = (header.ErrorStateIndicator == FDCAN_ESI_PASSIVE);
    return BSP_OK;
}

int32_t bsp_fdcan_loopback(bsp_fdcan_instance_t can_instance, const bsp_fdcan_frame_t *tx_frame, bsp_fdcan_frame_t *rx_frame)
{
    if ((tx_frame == nullptr) || (rx_frame == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (can_instance >= BSP_FDCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    attach_fdcan_bindings();
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_INTERNAL_LOOPBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }

    bsp_fdcan_config_t config{};
    const auto config_status = bsp_fdcan_get_default_config(can_instance, &config, tx_frame->fd_frame);
    if (config_status != BSP_OK) {
        return config_status;
    }
    config.bitrate_switch = tx_frame->bitrate_switch;
    config.internal_loopback = true;

    const auto init_status = bsp_fdcan_init_ex(can_instance, &config);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (bsp_fdcan_transmit(can_instance, tx_frame) != BSP_OK) {
        return BSP_ERROR_BUSY;
    }

    const uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < 100U) {
        const auto status = bsp_fdcan_receive(can_instance, rx_frame);
        if (status == BSP_OK) {
            return BSP_OK;
        }
    }

    return BSP_ERROR_TIMEOUT;
}

int32_t bsp_fdcan_config_filter_mask(bsp_fdcan_instance_t can_instance, const bsp_fdcan_filter_t *filter)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (filter == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_FILTER_MASK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    FDCAN_FilterTypeDef hal_filter = {};
    hal_filter.IdType = filter->extended_id ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    hal_filter.FilterIndex = filter->filter_index;
    hal_filter.FilterType = FDCAN_FILTER_MASK;
    hal_filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    hal_filter.FilterID1 = filter->id;
    hal_filter.FilterID2 = filter->mask;

    return (HAL_FDCAN_ConfigFilter(s_fdcan_map[can_instance].handle, &hal_filter) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

static uint32_t fdcan_global_action_to_hal(bsp_fdcan_global_filter_action_t action)
{
    switch (action) {
    case BSP_FDCAN_GLOBAL_FILTER_REJECT:
        return FDCAN_REJECT;
    case BSP_FDCAN_GLOBAL_FILTER_ACCEPT_RX_FIFO0:
    default:
        return FDCAN_ACCEPT_IN_RX_FIFO0;
    }
}

int32_t bsp_fdcan_set_global_filter(bsp_fdcan_instance_t can_instance, const bsp_fdcan_global_filter_t *filter)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (filter == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_GLOBAL_FILTER)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    return (HAL_FDCAN_ConfigGlobalFilter(
                s_fdcan_map[can_instance].handle,
                fdcan_global_action_to_hal(filter->non_matching_std),
                fdcan_global_action_to_hal(filter->non_matching_ext),
                filter->reject_remote_std ? FDCAN_REJECT_REMOTE : FDCAN_FILTER_REMOTE,
                filter->reject_remote_ext ? FDCAN_REJECT_REMOTE : FDCAN_FILTER_REMOTE) == HAL_OK)
               ? BSP_OK
               : BSP_ERROR;
}

int32_t bsp_fdcan_get_protocol_status(bsp_fdcan_instance_t can_instance, bsp_fdcan_protocol_status_t *status)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (status == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_PROTOCOL_STATUS)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    FDCAN_ProtocolStatusTypeDef hal_status = {};
    if (HAL_FDCAN_GetProtocolStatus(s_fdcan_map[can_instance].handle, &hal_status) != HAL_OK) {
        return BSP_ERROR;
    }

    status->last_error_code = hal_status.LastErrorCode;
    status->data_last_error_code = hal_status.DataLastErrorCode;
    status->activity = hal_status.Activity;
    status->error_warning = hal_status.Warning != 0U;
    status->error_passive = hal_status.ErrorPassive != 0U;
    status->bus_off = hal_status.BusOff != 0U;
    status->received_fd = hal_status.RxFDFflag != 0U;
    status->received_brs = hal_status.RxBRSflag != 0U;
    return BSP_OK;
}

int32_t bsp_fdcan_get_error_count(bsp_fdcan_instance_t can_instance, bsp_fdcan_error_count_t *count)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (count == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_ERROR_COUNT)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    FDCAN_ErrorCountersTypeDef hal_count = {};
    if (HAL_FDCAN_GetErrorCounters(s_fdcan_map[can_instance].handle, &hal_count) != HAL_OK) {
        return BSP_ERROR;
    }

    count->tx_error_count = hal_count.TxErrorCnt;
    count->rx_error_count = hal_count.RxErrorCnt;
    count->rx_error_passive = hal_count.RxErrorPassive != 0U;
    count->error_logging_count = hal_count.ErrorLogging;
    return BSP_OK;
}

int32_t bsp_fdcan_enable_timestamp(bsp_fdcan_instance_t can_instance, uint32_t prescaler)
{
    if (can_instance >= BSP_FDCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_TIMESTAMP)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    if (prescaler == 0U) {
        prescaler = FDCAN_TIMESTAMP_PRESC_1;
    }
    if (HAL_FDCAN_ConfigTimestampCounter(s_fdcan_map[can_instance].handle, prescaler) != HAL_OK) {
        return BSP_ERROR;
    }
    return (HAL_FDCAN_EnableTimestampCounter(s_fdcan_map[can_instance].handle, FDCAN_TIMESTAMP_INTERNAL) == HAL_OK) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_fdcan_get_timestamp(bsp_fdcan_instance_t can_instance, uint16_t *timestamp)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (timestamp == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_TIMESTAMP)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }

    *timestamp = HAL_FDCAN_GetTimestampCounter(s_fdcan_map[can_instance].handle);
    return BSP_OK;
}

int32_t bsp_fdcan_read_tx_event(bsp_fdcan_instance_t can_instance, bsp_fdcan_tx_event_t *event)
{
    if ((can_instance >= BSP_FDCAN_MAX) || (event == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_TX_EVENT_FIFO)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    const auto init_status = ensure_started(can_instance);
    if (init_status != BSP_OK) {
        return init_status;
    }
    if (s_fdcan_map[can_instance].config.tx_event_fifo_elements == 0U) {
        return BSP_ERROR_UNSUPPORTED;
    }

    FDCAN_TxEventFifoTypeDef hal_event = {};
    if (HAL_FDCAN_GetTxEvent(s_fdcan_map[can_instance].handle, &hal_event) != HAL_OK) {
        return BSP_ERROR_TIMEOUT;
    }
    event->timestamp = static_cast<uint16_t>(hal_event.TxTimestamp);
    event->message_marker = static_cast<uint8_t>(hal_event.MessageMarker);
    event->event_type = hal_event.EventType;
    return BSP_OK;
}

int32_t bsp_fdcan_register_rx_callback(bsp_fdcan_instance_t can_instance, bsp_fdcan_rx_callback_t callback, void *arg)
{
    if (can_instance >= BSP_FDCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    attach_fdcan_bindings();
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_RX_CALLBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    s_fdcan_map[can_instance].rx_callback = callback;
    s_fdcan_map[can_instance].rx_callback_arg = arg;
    return BSP_OK;
}

int32_t bsp_fdcan_register_tx_callback(bsp_fdcan_instance_t can_instance, bsp_fdcan_tx_callback_t callback, void *arg)
{
    if (can_instance >= BSP_FDCAN_MAX) {
        return BSP_ERROR_PARAM;
    }
    attach_fdcan_bindings();
    if (!fdcan_has_feature(s_fdcan_map[can_instance], BSP_FDCAN_FEATURE_TX_CALLBACK)) {
        return BSP_ERROR_UNSUPPORTED;
    }
    s_fdcan_map[can_instance].tx_callback = callback;
    s_fdcan_map[can_instance].tx_callback_arg = arg;
    return BSP_OK;
}

extern "C" void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t rxFifo0ITs)
{
    if ((rxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U) {
        return;
    }

    for (std::uint32_t index = 0; index < BSP_FDCAN_MAX; ++index) {
        auto &map = s_fdcan_map[index];
        if ((map.handle == hfdcan) && (map.rx_callback != nullptr)) {
            while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0U) {
                bsp_fdcan_frame_t frame{};
                if (bsp_fdcan_receive(static_cast<bsp_fdcan_instance_t>(index), &frame) != BSP_OK) {
                    break;
                }
                map.rx_callback(&frame, map.rx_callback_arg);
            }
            break;
        }
    }
}

extern "C" void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t bufferIndexes)
{
    (void) bufferIndexes;
    for (std::uint32_t index = 0; index < BSP_FDCAN_MAX; ++index) {
        auto &map = s_fdcan_map[index];
        if ((map.handle == hfdcan) && (map.tx_callback != nullptr)) {
            map.tx_callback(map.tx_callback_arg);
            break;
        }
    }
}
