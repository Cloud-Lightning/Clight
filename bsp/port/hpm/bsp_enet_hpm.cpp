/**
 * @file bsp_enet_hpm.cpp
 * @brief HPM platform implementation for BSP ENET interface
 */

#include "bsp_enet.h"

extern "C" {
#include "board.h"
#include "hpm_enet_drv.h"
#if (defined(APP_ENABLE_LWIP_TCPECHO) && APP_ENABLE_LWIP_TCPECHO) || (defined(APP_ENABLE_LWIP_HTTP_SERVER) && APP_ENABLE_LWIP_HTTP_SERVER)
#include "hpm_enet_phy.h"
#include "hpm_enet_phy_common.h"
#endif
}

static_assert(BSP_ENET_MAC_ADDR_MAX_COUNT == ENET_SOC_ADDR_MAX_COUNT);
static_assert(sizeof(bsp_enet_tx_desc_t) == sizeof(enet_tx_desc_t));
static_assert(sizeof(bsp_enet_rx_desc_t) == sizeof(enet_rx_desc_t));

static int32_t bsp_status_from_hpm(hpm_stat_t status)
{
    if (status == status_success) {
        return BSP_OK;
    }
    if (status == status_invalid_argument) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR;
}

static ENET_Type *bsp_enet_get_base(bsp_enet_port_t port)
{
    if (port != BSP_ENET_PORT0) {
        return nullptr;
    }

#if defined(BOARD_ENET_RGMII)
    return BOARD_ENET_RGMII;
#else
    return nullptr;
#endif
}

static enet_tx_desc_t *to_hpm_tx_desc(bsp_enet_tx_desc_t *desc)
{
    return reinterpret_cast<enet_tx_desc_t *>(desc);
}

static enet_rx_desc_t *to_hpm_rx_desc(bsp_enet_rx_desc_t *desc)
{
    return reinterpret_cast<enet_rx_desc_t *>(desc);
}

static bsp_enet_tx_desc_t *from_hpm_tx_desc(enet_tx_desc_t *desc)
{
    return reinterpret_cast<bsp_enet_tx_desc_t *>(desc);
}

static bsp_enet_rx_desc_t *from_hpm_rx_desc(enet_rx_desc_t *desc)
{
    return reinterpret_cast<bsp_enet_rx_desc_t *>(desc);
}

static bool to_hpm_interface(bsp_enet_interface_t value, enet_inf_type_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_IF_MII:
        *out = enet_inf_mii;
        return true;
    case BSP_ENET_IF_RMII:
        *out = enet_inf_rmii;
        return true;
    case BSP_ENET_IF_RGMII:
        *out = enet_inf_rgmii;
        return true;
    default:
        return false;
    }
}

static bool to_hpm_speed(bsp_enet_speed_t value, enet_line_speed_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_SPEED_10M:
        *out = enet_line_speed_10mbps;
        return true;
    case BSP_ENET_SPEED_100M:
        *out = enet_line_speed_100mbps;
        return true;
    case BSP_ENET_SPEED_1000M:
        *out = enet_line_speed_1000mbps;
        return true;
    default:
        return false;
    }
}

static bool to_hpm_duplex(bsp_enet_duplex_t value, enet_duplex_mode_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_DUPLEX_HALF:
        *out = enet_half_duplex;
        return true;
    case BSP_ENET_DUPLEX_FULL:
        *out = enet_full_duplex;
        return true;
    default:
        return false;
    }
}

static enet_cic_insertion_control_t to_hpm_checksum(bsp_enet_tx_checksum_t value)
{
    switch (value) {
    case BSP_ENET_TX_CHECKSUM_IP:
        return enet_cic_ip;
    case BSP_ENET_TX_CHECKSUM_IP_NO_PSEUDOHEADER:
        return enet_cic_ip_no_pseudoheader;
    case BSP_ENET_TX_CHECKSUM_IP_PSEUDOHEADER:
        return enet_cic_ip_pseudoheader;
    case BSP_ENET_TX_CHECKSUM_DISABLE:
    default:
        return enet_cic_disable;
    }
}

static bsp_enet_tx_checksum_t from_hpm_checksum(uint8_t value)
{
    switch (value) {
    case enet_cic_ip:
        return BSP_ENET_TX_CHECKSUM_IP;
    case enet_cic_ip_no_pseudoheader:
        return BSP_ENET_TX_CHECKSUM_IP_NO_PSEUDOHEADER;
    case enet_cic_ip_pseudoheader:
        return BSP_ENET_TX_CHECKSUM_IP_PSEUDOHEADER;
    case enet_cic_disable:
    default:
        return BSP_ENET_TX_CHECKSUM_DISABLE;
    }
}

static enet_vlan_insertion_control_t to_hpm_vlan(bsp_enet_tx_vlan_t value)
{
    switch (value) {
    case BSP_ENET_TX_VLAN_REMOVE:
        return enet_vlic_remove_vlan_tag;
    case BSP_ENET_TX_VLAN_INSERT:
        return enet_vlic_insert_vlan_tag;
    case BSP_ENET_TX_VLAN_REPLACE:
        return enet_vlic_replace_vlan_tag;
    case BSP_ENET_TX_VLAN_DISABLE:
    default:
        return enet_vlic_disable;
    }
}

static bsp_enet_tx_vlan_t from_hpm_vlan(uint8_t value)
{
    switch (value) {
    case enet_vlic_remove_vlan_tag:
        return BSP_ENET_TX_VLAN_REMOVE;
    case enet_vlic_insert_vlan_tag:
        return BSP_ENET_TX_VLAN_INSERT;
    case enet_vlic_replace_vlan_tag:
        return BSP_ENET_TX_VLAN_REPLACE;
    case enet_vlic_disable:
    default:
        return BSP_ENET_TX_VLAN_DISABLE;
    }
}

static enet_saic_insertion_replacement_control_t to_hpm_tx_saic(bsp_enet_tx_saic_t value)
{
    switch (value) {
    case BSP_ENET_TX_SAIC_INSERT_MAC0:
        return enet_saic_insert_mac0;
    case BSP_ENET_TX_SAIC_REPLACE_MAC0:
        return enet_saic_replace_mac0;
    case BSP_ENET_TX_SAIC_INSERT_MAC1:
        return enet_saic_insert_mac1;
    case BSP_ENET_TX_SAIC_REPLACE_MAC1:
        return enet_saic_replace_mac1;
    case BSP_ENET_TX_SAIC_DISABLE:
    default:
        return enet_saic_disable;
    }
}

static bsp_enet_tx_saic_t from_hpm_tx_saic(uint8_t value)
{
    switch (value) {
    case enet_saic_insert_mac0:
        return BSP_ENET_TX_SAIC_INSERT_MAC0;
    case enet_saic_replace_mac0:
        return BSP_ENET_TX_SAIC_REPLACE_MAC0;
    case enet_saic_insert_mac1:
        return BSP_ENET_TX_SAIC_INSERT_MAC1;
    case enet_saic_replace_mac1:
        return BSP_ENET_TX_SAIC_REPLACE_MAC1;
    case enet_saic_disable:
    default:
        return BSP_ENET_TX_SAIC_DISABLE;
    }
}

static enet_sarc_insertion_replacement_control_t to_hpm_mac_sarc(bsp_enet_mac_sarc_t value)
{
    switch (value) {
    case BSP_ENET_MAC_SARC_INSERT_MAC0:
        return enet_sarc_insert_mac0;
    case BSP_ENET_MAC_SARC_REPLACE_MAC0:
        return enet_sarc_replace_mac0;
    case BSP_ENET_MAC_SARC_INSERT_MAC1:
        return enet_sarc_insert_mac1;
    case BSP_ENET_MAC_SARC_REPLACE_MAC1:
        return enet_sarc_replace_mac1;
    case BSP_ENET_MAC_SARC_DISABLE:
    default:
        return enet_sarc_disable;
    }
}

static bsp_enet_mac_sarc_t from_hpm_mac_sarc(uint8_t value)
{
    switch (value) {
    case enet_sarc_insert_mac0:
        return BSP_ENET_MAC_SARC_INSERT_MAC0;
    case enet_sarc_replace_mac0:
        return BSP_ENET_MAC_SARC_REPLACE_MAC0;
    case enet_sarc_insert_mac1:
        return BSP_ENET_MAC_SARC_INSERT_MAC1;
    case enet_sarc_replace_mac1:
        return BSP_ENET_MAC_SARC_REPLACE_MAC1;
    case enet_sarc_disable:
    default:
        return BSP_ENET_MAC_SARC_DISABLE;
    }
}

static bool to_hpm_ptp_update_method(bsp_enet_ptp_update_method_t value, enet_ptp_time_update_method_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_PTP_UPDATE_COARSE:
        *out = enet_ptp_time_coarse_update;
        return true;
    case BSP_ENET_PTP_UPDATE_FINE:
        *out = enet_ptp_time_fine_update;
        return true;
    default:
        return false;
    }
}

static bsp_enet_ptp_update_method_t from_hpm_ptp_update_method(uint8_t value)
{
    return (value == enet_ptp_time_fine_update) ? BSP_ENET_PTP_UPDATE_FINE : BSP_ENET_PTP_UPDATE_COARSE;
}

static bool to_hpm_ptp_rollover(bsp_enet_ptp_rollover_t value, enet_ts_rollover_control_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_PTP_ROLLOVER_BINARY:
        *out = enet_ts_bin_rollover_control;
        return true;
    case BSP_ENET_PTP_ROLLOVER_DIGITAL:
        *out = enet_ts_dig_rollover_control;
        return true;
    default:
        return false;
    }
}

static bsp_enet_ptp_rollover_t from_hpm_ptp_rollover(uint8_t value)
{
    return (value == enet_ts_dig_rollover_control) ? BSP_ENET_PTP_ROLLOVER_DIGITAL : BSP_ENET_PTP_ROLLOVER_BINARY;
}

static bool to_hpm_ptp_version(bsp_enet_ptp_version_t value, enet_ptp_version_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_PTP_VERSION_1:
        *out = enet_ptp_v1;
        return true;
    case BSP_ENET_PTP_VERSION_2:
        *out = enet_ptp_v2;
        return true;
    default:
        return false;
    }
}

static bool to_hpm_ptp_frame_type(bsp_enet_ptp_frame_type_t value, enet_ptp_frame_type_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_PTP_FRAME_IPV4:
        *out = enet_ptp_frame_ipv4;
        return true;
    case BSP_ENET_PTP_FRAME_IPV6:
        *out = enet_ptp_frame_ipv6;
        return true;
    case BSP_ENET_PTP_FRAME_ETHERNET:
        *out = enet_ptp_frame_ethernet;
        return true;
    default:
        return false;
    }
}

static bool to_hpm_ptp_message_type(bsp_enet_ptp_message_type_t value, enet_ts_ss_ptp_msg_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_PTP_MESSAGE_0:
        *out = enet_ts_ss_ptp_msg_0;
        return true;
    case BSP_ENET_PTP_MESSAGE_1:
        *out = enet_ts_ss_ptp_msg_1;
        return true;
    case BSP_ENET_PTP_MESSAGE_2:
        *out = enet_ts_ss_ptp_msg_2;
        return true;
    case BSP_ENET_PTP_MESSAGE_3:
        *out = enet_ts_ss_ptp_msg_3;
        return true;
    case BSP_ENET_PTP_MESSAGE_4:
        *out = enet_ts_ss_ptp_msg_4;
        return true;
    case BSP_ENET_PTP_MESSAGE_5:
        *out = enet_ts_ss_ptp_msg_5;
        return true;
    case BSP_ENET_PTP_MESSAGE_6:
        *out = enet_ts_ss_ptp_msg_6;
        return true;
    case BSP_ENET_PTP_MESSAGE_7:
        *out = enet_ts_ss_ptp_msg_7;
        return true;
    default:
        return false;
    }
}

static bool to_hpm_pps_control(bsp_enet_pps_control_t value, enet_pps_ctrl_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_PPS_CTRL_PPS:
        *out = enet_pps_ctrl_pps;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_2HZ_DIGITAL_1HZ:
        *out = enet_pps_ctrl_bin_2hz_digital_1hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_4HZ_DIGITAL_2HZ:
        *out = enet_pps_ctrl_bin_4hz_digital_2hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_8HZ_DIGITAL_4HZ:
        *out = enet_pps_ctrl_bin_8hz_digital_4hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_16HZ_DIGITAL_8HZ:
        *out = enet_pps_ctrl_bin_16hz_digital_8hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_32HZ_DIGITAL_16HZ:
        *out = enet_pps_ctrl_bin_32hz_digital_16hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_64HZ_DIGITAL_32HZ:
        *out = enet_pps_ctrl_bin_64hz_digital_32hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_128HZ_DIGITAL_64HZ:
        *out = enet_pps_ctrl_bin_128hz_digital_64hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_256HZ_DIGITAL_128HZ:
        *out = enet_pps_ctrl_bin_256hz_digital_128hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_512HZ_DIGITAL_256HZ:
        *out = enet_pps_ctrl_bin_512hz_digital_256hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_1024HZ_DIGITAL_512HZ:
        *out = enet_pps_ctrl_bin_1024hz_digital_512hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_2048HZ_DIGITAL_1024HZ:
        *out = enet_pps_ctrl_bin_2048hz_digital_1024hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_4096HZ_DIGITAL_2048HZ:
        *out = enet_pps_ctrl_bin_4096hz_digital_2048hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_8192HZ_DIGITAL_4096HZ:
        *out = enet_pps_ctrl_bin_8192hz_digital_4096hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_16384HZ_DIGITAL_8192HZ:
        *out = enet_pps_ctrl_bin_16384hz_digital_8192hz;
        return true;
    case BSP_ENET_PPS_CTRL_BIN_32768HZ_DIGITAL_16384HZ:
        *out = enet_pps_ctrl_bin_32768hz_digital_16384hz;
        return true;
    default:
        return false;
    }
}

static bool to_hpm_pps_command(bsp_enet_pps_command_t value, enet_pps_cmd_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_PPS_CMD_NONE:
        *out = enet_pps_cmd_no_command;
        return true;
    case BSP_ENET_PPS_CMD_START_SINGLE_PULSE:
        *out = enet_pps_cmd_start_single_pulse;
        return true;
    case BSP_ENET_PPS_CMD_START_PULSE_TRAIN:
        *out = enet_pps_cmd_start_pulse_train;
        return true;
    case BSP_ENET_PPS_CMD_CANCEL_START:
        *out = enet_pps_cmd_cancel_start;
        return true;
    case BSP_ENET_PPS_CMD_STOP_AT_TIME:
        *out = enet_pps_cmd_stop_pulse_train_at_time;
        return true;
    case BSP_ENET_PPS_CMD_STOP_IMMEDIATE:
        *out = enet_pps_cmd_stop_pulse_train_immediately;
        return true;
    case BSP_ENET_PPS_CMD_CANCEL_STOP:
        *out = enet_pps_cmd_cancel_stop_pulse_train;
        return true;
    default:
        return false;
    }
}

static bool to_hpm_pps_index(bsp_enet_pps_index_t value, enet_pps_idx_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_PPS_INDEX0:
        *out = enet_pps_0;
        return true;
    case BSP_ENET_PPS_INDEX1:
        *out = enet_pps_1;
        return true;
    case BSP_ENET_PPS_INDEX2:
        *out = enet_pps_2;
        return true;
    case BSP_ENET_PPS_INDEX3:
        *out = enet_pps_3;
        return true;
    default:
        return false;
    }
}

static bool to_hpm_aux_trigger(bsp_enet_aux_snapshot_trigger_t value, enet_ptp_auxi_snapshot_trigger_idx_t *out)
{
    if (out == nullptr) {
        return false;
    }
    switch (value) {
    case BSP_ENET_AUX_SNAPSHOT_TRIGGER0:
        *out = enet_ptp_auxi_snapshot_trigger_0;
        return true;
    case BSP_ENET_AUX_SNAPSHOT_TRIGGER1:
        *out = enet_ptp_auxi_snapshot_trigger_1;
        return true;
    case BSP_ENET_AUX_SNAPSHOT_TRIGGER2:
        *out = enet_ptp_auxi_snapshot_trigger_2;
        return true;
    case BSP_ENET_AUX_SNAPSHOT_TRIGGER3:
        *out = enet_ptp_auxi_snapshot_trigger_3;
        return true;
    default:
        return false;
    }
}

static void to_hpm_tx_control(const bsp_enet_tx_control_config_t *src, enet_tx_control_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->enable_ioc = src->interrupt_on_completion;
    dst->disable_crc = src->disable_crc;
    dst->disable_pad = src->disable_pad;
    dst->enable_ttse = src->enable_timestamp;
    dst->enable_crcr = src->replace_crc;
    dst->cic = static_cast<uint8_t>(to_hpm_checksum(src->checksum));
    dst->vlic = static_cast<uint8_t>(to_hpm_vlan(src->vlan));
    dst->saic = static_cast<uint8_t>(to_hpm_tx_saic(src->source_address_insertion));
}

static void from_hpm_tx_control(const enet_tx_control_config_t *src, bsp_enet_tx_control_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->interrupt_on_completion = src->enable_ioc;
    dst->disable_crc = src->disable_crc;
    dst->disable_pad = src->disable_pad;
    dst->enable_timestamp = src->enable_ttse;
    dst->replace_crc = src->enable_crcr;
    dst->checksum = from_hpm_checksum(src->cic);
    dst->vlan = from_hpm_vlan(src->vlic);
    dst->source_address_insertion = from_hpm_tx_saic(src->saic);
}

static void to_hpm_interrupt_config(const bsp_enet_interrupt_config_t *src, enet_int_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->int_enable = src->interrupt_enable;
    dst->int_mask = src->interrupt_mask;
    dst->mmc_intr_rx = src->mmc_interrupt_rx;
    dst->mmc_intr_mask_rx = src->mmc_interrupt_mask_rx;
    dst->mmc_intr_tx = src->mmc_interrupt_tx;
    dst->mmc_intr_mask_tx = src->mmc_interrupt_mask_tx;
    dst->mmc_ipc_intr_mask_rx = src->mmc_ipc_interrupt_mask_rx;
}

static void from_hpm_interrupt_config(const enet_int_config_t *src, bsp_enet_interrupt_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->interrupt_enable = src->int_enable;
    dst->interrupt_mask = src->int_mask;
    dst->mmc_interrupt_rx = src->mmc_intr_rx;
    dst->mmc_interrupt_mask_rx = src->mmc_intr_mask_rx;
    dst->mmc_interrupt_tx = src->mmc_intr_tx;
    dst->mmc_interrupt_mask_tx = src->mmc_intr_mask_tx;
    dst->mmc_ipc_interrupt_mask_rx = src->mmc_ipc_intr_mask_rx;
}

static void to_hpm_mac_config(const bsp_enet_mac_config_t *src, enet_mac_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    for (uint32_t i = 0; i < BSP_ENET_MAC_ADDR_MAX_COUNT; ++i) {
        dst->mac_addr_high[i] = src->mac_addr_high[i];
        dst->mac_addr_low[i] = src->mac_addr_low[i];
    }
    dst->valid_max_count = src->valid_max_count;
    dst->dma_pbl = src->dma_pbl;
    dst->sarc = static_cast<uint8_t>(to_hpm_mac_sarc(src->source_address_control));
}

static void from_hpm_mac_config(const enet_mac_config_t *src, bsp_enet_mac_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    for (uint32_t i = 0; i < BSP_ENET_MAC_ADDR_MAX_COUNT; ++i) {
        dst->mac_addr_high[i] = src->mac_addr_high[i];
        dst->mac_addr_low[i] = src->mac_addr_low[i];
    }
    dst->valid_max_count = src->valid_max_count;
    dst->dma_pbl = src->dma_pbl;
    dst->source_address_control = from_hpm_mac_sarc(src->sarc);
}

static void to_hpm_buffer_config(const bsp_enet_buffer_config_t *src, enet_buff_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->buffer = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(src->buffer));
    dst->count = src->count;
    dst->size = src->size;
}

static void from_hpm_buffer_config(const enet_buff_config_t *src, bsp_enet_buffer_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->buffer = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(src->buffer));
    dst->count = src->count;
    dst->size = src->size;
}

static void to_hpm_rx_frame_info(const bsp_enet_rx_frame_info_t *src, enet_rx_frame_info_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->fs_rx_desc = to_hpm_rx_desc(src->first_desc);
    dst->ls_rx_desc = to_hpm_rx_desc(src->last_desc);
    dst->seg_count = src->segment_count;
}

static void from_hpm_rx_frame_info(const enet_rx_frame_info_t *src, bsp_enet_rx_frame_info_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->first_desc = from_hpm_rx_desc(src->fs_rx_desc);
    dst->last_desc = from_hpm_rx_desc(src->ls_rx_desc);
    dst->segment_count = src->seg_count;
}

static void to_hpm_desc(const bsp_enet_desc_t *src, enet_desc_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->tx_desc_list_head = to_hpm_tx_desc(src->tx_desc_list_head);
    dst->rx_desc_list_head = to_hpm_rx_desc(src->rx_desc_list_head);
    dst->tx_desc_list_cur = to_hpm_tx_desc(src->tx_desc_list_cur);
    dst->rx_desc_list_cur = to_hpm_rx_desc(src->rx_desc_list_cur);
    to_hpm_buffer_config(&src->tx_buffer_config, &dst->tx_buff_cfg);
    to_hpm_buffer_config(&src->rx_buffer_config, &dst->rx_buff_cfg);
    to_hpm_rx_frame_info(&src->rx_frame_info, &dst->rx_frame_info);
    to_hpm_tx_control(&src->tx_control_config, &dst->tx_control_config);
}

static void from_hpm_desc(const enet_desc_t *src, bsp_enet_desc_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->tx_desc_list_head = from_hpm_tx_desc(src->tx_desc_list_head);
    dst->rx_desc_list_head = from_hpm_rx_desc(src->rx_desc_list_head);
    dst->tx_desc_list_cur = from_hpm_tx_desc(src->tx_desc_list_cur);
    dst->rx_desc_list_cur = from_hpm_rx_desc(src->rx_desc_list_cur);
    from_hpm_buffer_config(&src->tx_buff_cfg, &dst->tx_buffer_config);
    from_hpm_buffer_config(&src->rx_buff_cfg, &dst->rx_buffer_config);
    from_hpm_rx_frame_info(&src->rx_frame_info, &dst->rx_frame_info);
    from_hpm_tx_control(&src->tx_control_config, &dst->tx_control_config);
}

static void from_hpm_frame(const enet_frame_t *src, bsp_enet_frame_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->length = src->length;
    dst->buffer = reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(src->buffer));
    dst->segment_count = src->seg;
    dst->used = src->used;
    dst->rx_desc = from_hpm_rx_desc(src->rx_desc);
}

static void to_hpm_ptp_config(const bsp_enet_ptp_config_t *src, enet_ptp_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    enet_ts_rollover_control_t rollover = enet_ts_bin_rollover_control;
    enet_ptp_time_update_method_t method = enet_ptp_time_coarse_update;
    (void) to_hpm_ptp_rollover(src->rollover, &rollover);
    (void) to_hpm_ptp_update_method(src->update_method, &method);
    dst->ssinc = src->subsecond_increment;
    dst->timestamp_rollover_mode = static_cast<uint8_t>(rollover);
    dst->update_method = static_cast<uint8_t>(method);
    dst->addend = src->addend;
}

static void from_hpm_ptp_config(const enet_ptp_config_t *src, bsp_enet_ptp_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->subsecond_increment = src->ssinc;
    dst->rollover = from_hpm_ptp_rollover(src->timestamp_rollover_mode);
    dst->update_method = from_hpm_ptp_update_method(src->update_method);
    dst->addend = src->addend;
}

static void to_hpm_ptp_timestamp_update(const bsp_enet_ptp_timestamp_update_t *src, enet_ptp_ts_update_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->sec = src->second;
    dst->nsec = src->nanosecond;
    dst->sign = src->sign;
}

static void from_hpm_ptp_timestamp(const enet_ptp_ts_system_t *src, bsp_enet_ptp_timestamp_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->second = src->sec;
    dst->nanosecond = src->nsec;
}

static void to_hpm_pps_config(const bsp_enet_pps_config_t *src, enet_pps_cmd_config_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->pps_interval = src->interval;
    dst->pps_width = src->width;
    dst->target_sec = src->target_second;
    dst->target_nsec = src->target_nanosecond;
}

static void from_hpm_aux_timestamp(const enet_ptp_ts_auxi_snapshot_t *src, bsp_enet_ptp_aux_timestamp_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->second = src->sec;
    dst->nanosecond = src->nsec;
}

static void from_hpm_aux_status(const enet_ptp_auxi_snapshot_status_t *src, bsp_enet_ptp_aux_status_t *dst)
{
    if ((src == nullptr) || (dst == nullptr)) {
        return;
    }
    dst->missed = src->auxi_snapshot_miss;
    dst->count = src->auxi_snapshot_count;
    dst->index = src->auxi_snapshot_id;
}

int32_t bsp_enet_init(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }

    if (board_init_enet_pins(base) != status_success) {
        return BSP_ERROR;
    }

    if (board_reset_enet_phy(base) != status_success) {
        return BSP_ERROR;
    }

    if (board_init_enet_rgmii_clock_delay(base) != status_success) {
        return BSP_ERROR;
    }

    return BSP_OK;
}

uintptr_t bsp_enet_get_base_address(bsp_enet_port_t port)
{
    return reinterpret_cast<uintptr_t>(bsp_enet_get_base(port));
}

int32_t bsp_enet_init_pins(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return (board_init_enet_pins(base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_enet_reset_phy(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return (board_reset_enet_phy(base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_enet_init_rgmii_clock_delay(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return (board_init_enet_rgmii_clock_delay(base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_enet_init_rmii_reference_clock(bsp_enet_port_t port, bool internal)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return (board_init_enet_rmii_reference_clock(base, internal) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_enet_init_mii_clock(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return (board_init_enet_mii_clock(base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_enet_init_ptp_clock(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return (board_init_enet_ptp_clock(base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_enet_enable_irq(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return (board_enable_enet_irq(base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_enet_disable_irq(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    return (board_disable_enet_irq(base) == status_success) ? BSP_OK : BSP_ERROR;
}

int32_t bsp_enet_get_dma_pbl(bsp_enet_port_t port, uint8_t *pbl)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (pbl == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *pbl = board_get_enet_dma_pbl(base);
    return BSP_OK;
}

int32_t bsp_enet_read_phy(bsp_enet_port_t port, uint32_t phy_addr, uint32_t reg_addr, uint16_t *data)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (data == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *data = enet_read_phy(base, phy_addr, reg_addr);
    return BSP_OK;
}

int32_t bsp_enet_write_phy(bsp_enet_port_t port, uint32_t phy_addr, uint32_t reg_addr, uint16_t data)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    enet_write_phy(base, phy_addr, reg_addr, data);
    return BSP_OK;
}

int32_t bsp_enet_set_mdio_owner(bsp_mdio_owner_t owner)
{
    if (owner == BSP_MDIO_OWNER_ENET) {
        board_select_mdio_owner_for_enet();
        return BSP_OK;
    }

    if (owner == BSP_MDIO_OWNER_ESC) {
        board_select_mdio_owner_for_esc();
        return BSP_OK;
    }

    return BSP_ERROR_PARAM;
}

int32_t bsp_enet_get_link_state(bsp_enet_port_t port, bsp_enet_link_state_t *state)
{
    if ((port != BSP_ENET_PORT0) || (state == nullptr)) {
        return BSP_ERROR_PARAM;
    }

#if defined(__USE_RTL8211) && __USE_RTL8211
    enet_phy_status_t phy_state = {0};
    rtl8211_get_phy_status(BOARD_ENET_RGMII, BOARD_ENET_PHY_ADDR_RTL8211, &phy_state);

    state->link_up = phy_state.enet_phy_link ? BSP_ENABLE : BSP_DISABLE;
    switch (phy_state.enet_phy_speed) {
    case enet_phy_port_speed_10mbps:
        state->speed = BSP_ENET_SPEED_10M;
        break;
    case enet_phy_port_speed_100mbps:
        state->speed = BSP_ENET_SPEED_100M;
        break;
    case enet_phy_port_speed_1000mbps:
        state->speed = BSP_ENET_SPEED_1000M;
        break;
    default:
        state->speed = BSP_ENET_SPEED_UNKNOWN;
        break;
    }

    switch (phy_state.enet_phy_duplex) {
    case enet_phy_duplex_half:
        state->duplex = BSP_ENET_DUPLEX_HALF;
        break;
    case enet_phy_duplex_full:
        state->duplex = BSP_ENET_DUPLEX_FULL;
        break;
    default:
        state->duplex = BSP_ENET_DUPLEX_UNKNOWN;
        break;
    }
    return BSP_OK;
#else
    (void) state;
    return BSP_ERROR;
#endif
}

int32_t bsp_enet_get_default_tx_control(bsp_enet_port_t port, bsp_enet_tx_control_config_t *config)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    enet_tx_control_config_t local{};
    enet_get_default_tx_control_config(base, &local);
    from_hpm_tx_control(&local, config);
    return BSP_OK;
}

int32_t bsp_enet_get_default_interrupt_config(bsp_enet_port_t port, bsp_enet_interrupt_config_t *config)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    enet_int_config_t local{};
    enet_get_default_interrupt_config(base, &local);
    from_hpm_interrupt_config(&local, config);
    return BSP_OK;
}

int32_t bsp_enet_controller_init(bsp_enet_port_t port,
                                 bsp_enet_interface_t inf_type,
                                 bsp_enet_desc_t *desc,
                                 bsp_enet_mac_config_t *mac_config,
                                 bsp_enet_interrupt_config_t *interrupt_config)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (desc == nullptr) || (mac_config == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    enet_inf_type_t hpm_if{};
    if (!to_hpm_interface(inf_type, &hpm_if)) {
        return BSP_ERROR_PARAM;
    }

    enet_desc_t local_desc{};
    enet_mac_config_t local_mac{};
    to_hpm_desc(desc, &local_desc);
    to_hpm_mac_config(mac_config, &local_mac);

    enet_int_config_t local_irq{};
    enet_int_config_t *irq_ptr = nullptr;
    if (interrupt_config != nullptr) {
        to_hpm_interrupt_config(interrupt_config, &local_irq);
        irq_ptr = &local_irq;
    }

    const auto status = bsp_status_from_hpm(enet_controller_init(base, hpm_if, &local_desc, &local_mac, irq_ptr));
    from_hpm_desc(&local_desc, desc);
    from_hpm_mac_config(&local_mac, mac_config);
    if (interrupt_config != nullptr) {
        from_hpm_interrupt_config(&local_irq, interrupt_config);
    }
    return status;
}

int32_t bsp_enet_set_line_speed(bsp_enet_port_t port, bsp_enet_speed_t speed)
{
    auto *base = bsp_enet_get_base(port);
    enet_line_speed_t local{};
    if ((base == nullptr) || !to_hpm_speed(speed, &local)) {
        return BSP_ERROR_PARAM;
    }
    enet_set_line_speed(base, local);
    return BSP_OK;
}

int32_t bsp_enet_set_duplex_mode(bsp_enet_port_t port, bsp_enet_duplex_t duplex)
{
    auto *base = bsp_enet_get_base(port);
    enet_duplex_mode_t local{};
    if ((base == nullptr) || !to_hpm_duplex(duplex, &local)) {
        return BSP_ERROR_PARAM;
    }
    enet_set_duplex_mode(base, local);
    return BSP_OK;
}

int32_t bsp_enet_rx_resume(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    enet_rx_resume(base);
    return BSP_OK;
}

int32_t bsp_enet_tx_desc_chain_init(bsp_enet_port_t port, bsp_enet_desc_t *desc)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (desc == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    enet_desc_t local{};
    to_hpm_desc(desc, &local);
    enet_dma_tx_desc_chain_init(base, &local);
    from_hpm_desc(&local, desc);
    return BSP_OK;
}

int32_t bsp_enet_rx_desc_chain_init(bsp_enet_port_t port, bsp_enet_desc_t *desc)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (desc == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    enet_desc_t local{};
    to_hpm_desc(desc, &local);
    enet_dma_rx_desc_chain_init(base, &local);
    from_hpm_desc(&local, desc);
    return BSP_OK;
}

int32_t bsp_enet_dma_flush(bsp_enet_port_t port)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    enet_dma_flush(base);
    return BSP_OK;
}

int32_t bsp_enet_get_interrupt_status(bsp_enet_port_t port, uint32_t *status)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (status == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    *status = enet_get_interrupt_status(base);
    return BSP_OK;
}

int32_t bsp_enet_mask_interrupt_event(bsp_enet_port_t port, uint32_t mask)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    enet_mask_interrupt_event(base, mask);
    return BSP_OK;
}

int32_t bsp_enet_unmask_interrupt_event(bsp_enet_port_t port, uint32_t mask)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    enet_unmask_interrupt_event(base, mask);
    return BSP_OK;
}

int32_t bsp_enet_check_received_frame(bsp_enet_rx_desc_t **current_desc,
                                      bsp_enet_rx_frame_info_t *info,
                                      bsp_state_t *available)
{
    if ((current_desc == nullptr) || (info == nullptr) || (available == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    auto *local_desc = to_hpm_rx_desc(*current_desc);
    enet_rx_frame_info_t local_info{};
    to_hpm_rx_frame_info(info, &local_info);
    *available = (enet_check_received_frame(&local_desc, &local_info) != 0U) ? BSP_ENABLE : BSP_DISABLE;
    *current_desc = from_hpm_rx_desc(local_desc);
    from_hpm_rx_frame_info(&local_info, info);
    return BSP_OK;
}

int32_t bsp_enet_get_received_frame(bsp_enet_rx_desc_t **current_desc,
                                    bsp_enet_rx_frame_info_t *info,
                                    bsp_enet_frame_t *frame)
{
    if ((current_desc == nullptr) || (info == nullptr) || (frame == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    auto *local_desc = to_hpm_rx_desc(*current_desc);
    enet_rx_frame_info_t local_info{};
    to_hpm_rx_frame_info(info, &local_info);
    const enet_frame_t local_frame = enet_get_received_frame(&local_desc, &local_info);
    *current_desc = from_hpm_rx_desc(local_desc);
    from_hpm_rx_frame_info(&local_info, info);
    from_hpm_frame(&local_frame, frame);
    return BSP_OK;
}

int32_t bsp_enet_get_received_frame_interrupt(bsp_enet_rx_desc_t **current_desc,
                                              bsp_enet_rx_frame_info_t *info,
                                              uint32_t rx_desc_count,
                                              bsp_enet_frame_t *frame)
{
    if ((current_desc == nullptr) || (info == nullptr) || (frame == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    auto *local_desc = to_hpm_rx_desc(*current_desc);
    enet_rx_frame_info_t local_info{};
    to_hpm_rx_frame_info(info, &local_info);
    const enet_frame_t local_frame = enet_get_received_frame_interrupt(&local_desc, &local_info, rx_desc_count);
    *current_desc = from_hpm_rx_desc(local_desc);
    from_hpm_rx_frame_info(&local_info, info);
    from_hpm_frame(&local_frame, frame);
    return BSP_OK;
}

int32_t bsp_enet_prepare_tx_desc(bsp_enet_port_t port,
                                 bsp_enet_tx_desc_t **current_desc,
                                 const bsp_enet_tx_control_config_t *config,
                                 uint16_t frame_length,
                                 uint16_t tx_buffer_size,
                                 bsp_state_t *prepared)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (current_desc == nullptr) || (config == nullptr) || (prepared == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    auto *local_desc = to_hpm_tx_desc(*current_desc);
    enet_tx_control_config_t local_config{};
    to_hpm_tx_control(config, &local_config);
    *prepared = (enet_prepare_tx_desc(base, &local_desc, &local_config, frame_length, tx_buffer_size) != 0U)
        ? BSP_ENABLE
        : BSP_DISABLE;
    *current_desc = from_hpm_tx_desc(local_desc);
    return BSP_OK;
}

int32_t bsp_enet_prepare_tx_desc_with_timestamp(bsp_enet_port_t port,
                                                bsp_enet_tx_desc_t **current_desc,
                                                const bsp_enet_tx_control_config_t *config,
                                                uint16_t frame_length,
                                                uint16_t tx_buffer_size,
                                                bsp_enet_ptp_timestamp_t *timestamp,
                                                bsp_state_t *prepared)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (current_desc == nullptr) || (config == nullptr) || (timestamp == nullptr) || (prepared == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    auto *local_desc = to_hpm_tx_desc(*current_desc);
    enet_tx_control_config_t local_config{};
    enet_ptp_ts_system_t local_timestamp{};
    to_hpm_tx_control(config, &local_config);
    *prepared = (enet_prepare_tx_desc_with_ts_record(base, &local_desc, &local_config, frame_length, tx_buffer_size, &local_timestamp) != 0U)
        ? BSP_ENABLE
        : BSP_DISABLE;
    *current_desc = from_hpm_tx_desc(local_desc);
    from_hpm_ptp_timestamp(&local_timestamp, timestamp);
    return BSP_OK;
}

int32_t bsp_enet_get_default_ptp_config(bsp_enet_port_t port,
                                        uint32_t ptp_clock_hz,
                                        bsp_enet_ptp_config_t *config)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }

    enet_ptp_config_t local{};
    const auto status = bsp_status_from_hpm(enet_get_default_ptp_config(base, ptp_clock_hz, &local));
    if (status == BSP_OK) {
        from_hpm_ptp_config(&local, config);
    }
    return status;
}

int32_t bsp_enet_init_ptp(bsp_enet_port_t port, const bsp_enet_ptp_config_t *config)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (config == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    enet_ptp_config_t local{};
    to_hpm_ptp_config(config, &local);
    enet_init_ptp(base, &local);
    return BSP_OK;
}

int32_t bsp_enet_set_ptp_timestamp(bsp_enet_port_t port, const bsp_enet_ptp_timestamp_update_t *timestamp)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (timestamp == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    enet_ptp_ts_update_t local{};
    to_hpm_ptp_timestamp_update(timestamp, &local);
    enet_set_ptp_timestamp(base, &local);
    return BSP_OK;
}

int32_t bsp_enet_get_ptp_timestamp(bsp_enet_port_t port, bsp_enet_ptp_timestamp_t *timestamp)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (timestamp == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    enet_ptp_ts_system_t local{};
    enet_get_ptp_timestamp(base, &local);
    from_hpm_ptp_timestamp(&local, timestamp);
    return BSP_OK;
}

int32_t bsp_enet_update_ptp_timeoffset(bsp_enet_port_t port, const bsp_enet_ptp_timestamp_update_t *timeoffset)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (timeoffset == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    enet_ptp_ts_update_t local{};
    to_hpm_ptp_timestamp_update(timeoffset, &local);
    enet_update_ptp_timeoffset(base, &local);
    return BSP_OK;
}

int32_t bsp_enet_adjust_ptp_time_freq(bsp_enet_port_t port, int32_t adjustment)
{
    auto *base = bsp_enet_get_base(port);
    if (base == nullptr) {
        return BSP_ERROR_PARAM;
    }
    enet_adjust_ptp_time_freq(base, adjustment);
    return BSP_OK;
}

int32_t bsp_enet_set_ptp_version(bsp_enet_port_t port, bsp_enet_ptp_version_t version)
{
    auto *base = bsp_enet_get_base(port);
    enet_ptp_version_t local{};
    if ((base == nullptr) || !to_hpm_ptp_version(version, &local)) {
        return BSP_ERROR_PARAM;
    }
    enet_set_ptp_version(base, local);
    return BSP_OK;
}

int32_t bsp_enet_enable_ptp_frame_type(bsp_enet_port_t port,
                                       bsp_enet_ptp_frame_type_t frame_type,
                                       bool enable)
{
    auto *base = bsp_enet_get_base(port);
    enet_ptp_frame_type_t local{};
    if ((base == nullptr) || !to_hpm_ptp_frame_type(frame_type, &local)) {
        return BSP_ERROR_PARAM;
    }
    return bsp_status_from_hpm(enet_enable_ptp_frame_type(base, local, enable));
}

int32_t bsp_enet_set_snapshot_ptp_message_type(bsp_enet_port_t port,
                                               bsp_enet_ptp_message_type_t message_type)
{
    auto *base = bsp_enet_get_base(port);
    enet_ts_ss_ptp_msg_t local{};
    if ((base == nullptr) || !to_hpm_ptp_message_type(message_type, &local)) {
        return BSP_ERROR_PARAM;
    }
    enet_set_snapshot_ptp_message_type(base, local);
    return BSP_OK;
}

int32_t bsp_enet_set_pps_control_output(bsp_enet_port_t port, bsp_enet_pps_control_t control)
{
    auto *base = bsp_enet_get_base(port);
    enet_pps_ctrl_t local{};
    if ((base == nullptr) || !to_hpm_pps_control(control, &local)) {
        return BSP_ERROR_PARAM;
    }
    enet_set_pps0_control_output(base, local);
    return BSP_OK;
}

int32_t bsp_enet_set_pps_command(bsp_enet_port_t port,
                                 bsp_enet_pps_command_t command,
                                 bsp_enet_pps_index_t index)
{
    auto *base = bsp_enet_get_base(port);
    enet_pps_cmd_t local_command{};
    enet_pps_idx_t local_index{};
    if ((base == nullptr)
        || !to_hpm_pps_command(command, &local_command)
        || !to_hpm_pps_index(index, &local_index)) {
        return BSP_ERROR_PARAM;
    }
    return bsp_status_from_hpm(enet_set_ppsx_command(base, local_command, local_index));
}

int32_t bsp_enet_set_pps_config(bsp_enet_port_t port,
                                const bsp_enet_pps_config_t *config,
                                bsp_enet_pps_index_t index)
{
    auto *base = bsp_enet_get_base(port);
    enet_pps_idx_t local_index{};
    if ((base == nullptr) || (config == nullptr) || !to_hpm_pps_index(index, &local_index)) {
        return BSP_ERROR_PARAM;
    }

    enet_pps_cmd_config_t local_config{};
    to_hpm_pps_config(config, &local_config);
    return bsp_status_from_hpm(enet_set_ppsx_config(base, &local_config, local_index));
}

int32_t bsp_enet_enable_aux_snapshot(bsp_enet_port_t port, bsp_enet_aux_snapshot_trigger_t trigger)
{
    auto *base = bsp_enet_get_base(port);
    enet_ptp_auxi_snapshot_trigger_idx_t local{};
    if ((base == nullptr) || !to_hpm_aux_trigger(trigger, &local)) {
        return BSP_ERROR_PARAM;
    }
    enet_enable_ptp_auxiliary_snapshot(base, local);
    return BSP_OK;
}

int32_t bsp_enet_disable_aux_snapshot(bsp_enet_port_t port, bsp_enet_aux_snapshot_trigger_t trigger)
{
    auto *base = bsp_enet_get_base(port);
    enet_ptp_auxi_snapshot_trigger_idx_t local{};
    if ((base == nullptr) || !to_hpm_aux_trigger(trigger, &local)) {
        return BSP_ERROR_PARAM;
    }
    enet_disable_ptp_auxiliary_snapshot(base, local);
    return BSP_OK;
}

int32_t bsp_enet_get_aux_timestamp(bsp_enet_port_t port, bsp_enet_ptp_aux_timestamp_t *timestamp)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (timestamp == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    enet_ptp_ts_auxi_snapshot_t local{};
    enet_get_ptp_auxi_timestamp(base, &local);
    from_hpm_aux_timestamp(&local, timestamp);
    return BSP_OK;
}

int32_t bsp_enet_get_aux_snapshot_status(bsp_enet_port_t port, bsp_enet_ptp_aux_status_t *status)
{
    auto *base = bsp_enet_get_base(port);
    if ((base == nullptr) || (status == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    enet_ptp_auxi_snapshot_status_t local{};
    enet_get_ptp_auxi_snapshot_status(base, &local);
    from_hpm_aux_status(&local, status);
    return BSP_OK;
}
