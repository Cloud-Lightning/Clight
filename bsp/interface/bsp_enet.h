/**
 * @file bsp_enet.h
 * @brief BSP Ethernet abstraction interface
 */

#ifndef BSP_ENET_H
#define BSP_ENET_H

#include "bsp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_ENET_MAC_ADDR_MAX_COUNT        (5U)
#define BSP_ENET_DESCRIPTOR_WORD_COUNT     (8U)

#define BSP_ENET_FEATURE_INIT        (1U << 0)
#define BSP_ENET_FEATURE_MDIO        (1U << 1)
#define BSP_ENET_FEATURE_LINK_STATE  (1U << 2)
#define BSP_ENET_FEATURE_DMA_DESC    (1U << 3)
#define BSP_ENET_FEATURE_PTP         (1U << 4)
#define BSP_ENET_FEATURE_IRQ         (1U << 5)

typedef enum {
    BSP_ENET_PORT0 = 0,
    BSP_ENET_PORT_MAX
} bsp_enet_port_t;

typedef enum {
    BSP_ENET_SPEED_10M = 0,
    BSP_ENET_SPEED_100M,
    BSP_ENET_SPEED_1000M,
    BSP_ENET_SPEED_UNKNOWN
} bsp_enet_speed_t;

typedef enum {
    BSP_ENET_DUPLEX_HALF = 0,
    BSP_ENET_DUPLEX_FULL,
    BSP_ENET_DUPLEX_UNKNOWN
} bsp_enet_duplex_t;

typedef enum {
    BSP_MDIO_OWNER_ENET = 0,
    BSP_MDIO_OWNER_ESC
} bsp_mdio_owner_t;

typedef enum {
    BSP_ENET_IF_MII = 0,
    BSP_ENET_IF_RMII,
    BSP_ENET_IF_RGMII
} bsp_enet_interface_t;

typedef enum {
    BSP_ENET_TX_CHECKSUM_DISABLE = 0,
    BSP_ENET_TX_CHECKSUM_IP = 1,
    BSP_ENET_TX_CHECKSUM_IP_NO_PSEUDOHEADER = 2,
    BSP_ENET_TX_CHECKSUM_IP_PSEUDOHEADER = 3
} bsp_enet_tx_checksum_t;

typedef enum {
    BSP_ENET_TX_VLAN_DISABLE = 0,
    BSP_ENET_TX_VLAN_REMOVE = 1,
    BSP_ENET_TX_VLAN_INSERT = 2,
    BSP_ENET_TX_VLAN_REPLACE = 3
} bsp_enet_tx_vlan_t;

typedef enum {
    BSP_ENET_TX_SAIC_DISABLE = 0,
    BSP_ENET_TX_SAIC_INSERT_MAC0 = 1,
    BSP_ENET_TX_SAIC_REPLACE_MAC0 = 2,
    BSP_ENET_TX_SAIC_INSERT_MAC1 = 5,
    BSP_ENET_TX_SAIC_REPLACE_MAC1 = 6
} bsp_enet_tx_saic_t;

typedef enum {
    BSP_ENET_MAC_SARC_DISABLE = 0,
    BSP_ENET_MAC_SARC_INSERT_MAC0 = 2,
    BSP_ENET_MAC_SARC_REPLACE_MAC0 = 3,
    BSP_ENET_MAC_SARC_INSERT_MAC1 = 6,
    BSP_ENET_MAC_SARC_REPLACE_MAC1 = 7
} bsp_enet_mac_sarc_t;

typedef enum {
    BSP_ENET_PTP_UPDATE_COARSE = 0,
    BSP_ENET_PTP_UPDATE_FINE = 1
} bsp_enet_ptp_update_method_t;

typedef enum {
    BSP_ENET_PTP_VERSION_1 = 0,
    BSP_ENET_PTP_VERSION_2 = 1
} bsp_enet_ptp_version_t;

typedef enum {
    BSP_ENET_PTP_FRAME_IPV4 = 0,
    BSP_ENET_PTP_FRAME_IPV6 = 1,
    BSP_ENET_PTP_FRAME_ETHERNET = 2
} bsp_enet_ptp_frame_type_t;

typedef enum {
    BSP_ENET_PTP_MESSAGE_0 = 0,
    BSP_ENET_PTP_MESSAGE_1 = 1,
    BSP_ENET_PTP_MESSAGE_2 = 3,
    BSP_ENET_PTP_MESSAGE_3 = 4,
    BSP_ENET_PTP_MESSAGE_4 = 5,
    BSP_ENET_PTP_MESSAGE_5 = 7,
    BSP_ENET_PTP_MESSAGE_6 = 8,
    BSP_ENET_PTP_MESSAGE_7 = 12
} bsp_enet_ptp_message_type_t;

typedef enum {
    BSP_ENET_PTP_ROLLOVER_BINARY = 0,
    BSP_ENET_PTP_ROLLOVER_DIGITAL = 1
} bsp_enet_ptp_rollover_t;

typedef enum {
    BSP_ENET_PPS_INDEX0 = -1,
    BSP_ENET_PPS_INDEX1 = 0,
    BSP_ENET_PPS_INDEX2 = 1,
    BSP_ENET_PPS_INDEX3 = 2
} bsp_enet_pps_index_t;

typedef enum {
    BSP_ENET_AUX_SNAPSHOT_TRIGGER0 = 0,
    BSP_ENET_AUX_SNAPSHOT_TRIGGER1 = 1,
    BSP_ENET_AUX_SNAPSHOT_TRIGGER2 = 2,
    BSP_ENET_AUX_SNAPSHOT_TRIGGER3 = 3
} bsp_enet_aux_snapshot_trigger_t;

typedef enum {
    BSP_ENET_PPS_CTRL_PPS = 0,
    BSP_ENET_PPS_CTRL_BIN_2HZ_DIGITAL_1HZ = 1,
    BSP_ENET_PPS_CTRL_BIN_4HZ_DIGITAL_2HZ = 2,
    BSP_ENET_PPS_CTRL_BIN_8HZ_DIGITAL_4HZ = 3,
    BSP_ENET_PPS_CTRL_BIN_16HZ_DIGITAL_8HZ = 4,
    BSP_ENET_PPS_CTRL_BIN_32HZ_DIGITAL_16HZ = 5,
    BSP_ENET_PPS_CTRL_BIN_64HZ_DIGITAL_32HZ = 6,
    BSP_ENET_PPS_CTRL_BIN_128HZ_DIGITAL_64HZ = 7,
    BSP_ENET_PPS_CTRL_BIN_256HZ_DIGITAL_128HZ = 8,
    BSP_ENET_PPS_CTRL_BIN_512HZ_DIGITAL_256HZ = 9,
    BSP_ENET_PPS_CTRL_BIN_1024HZ_DIGITAL_512HZ = 10,
    BSP_ENET_PPS_CTRL_BIN_2048HZ_DIGITAL_1024HZ = 11,
    BSP_ENET_PPS_CTRL_BIN_4096HZ_DIGITAL_2048HZ = 12,
    BSP_ENET_PPS_CTRL_BIN_8192HZ_DIGITAL_4096HZ = 13,
    BSP_ENET_PPS_CTRL_BIN_16384HZ_DIGITAL_8192HZ = 14,
    BSP_ENET_PPS_CTRL_BIN_32768HZ_DIGITAL_16384HZ = 15
} bsp_enet_pps_control_t;

typedef enum {
    BSP_ENET_PPS_CMD_NONE = 0,
    BSP_ENET_PPS_CMD_START_SINGLE_PULSE = 1,
    BSP_ENET_PPS_CMD_START_PULSE_TRAIN = 2,
    BSP_ENET_PPS_CMD_CANCEL_START = 3,
    BSP_ENET_PPS_CMD_STOP_AT_TIME = 4,
    BSP_ENET_PPS_CMD_STOP_IMMEDIATE = 5,
    BSP_ENET_PPS_CMD_CANCEL_STOP = 6
} bsp_enet_pps_command_t;

typedef struct {
    bsp_state_t link_up;
    bsp_enet_speed_t speed;
    bsp_enet_duplex_t duplex;
} bsp_enet_link_state_t;

typedef struct {
    uint8_t *buffer;
    uint32_t count;
    uint16_t size;
} bsp_enet_buffer_config_t;

typedef struct {
    uint32_t words[BSP_ENET_DESCRIPTOR_WORD_COUNT];
} bsp_enet_tx_desc_t;

typedef struct {
    uint32_t words[BSP_ENET_DESCRIPTOR_WORD_COUNT];
} bsp_enet_rx_desc_t;

typedef struct {
    bsp_enet_rx_desc_t *first_desc;
    bsp_enet_rx_desc_t *last_desc;
    uint32_t segment_count;
} bsp_enet_rx_frame_info_t;

typedef struct {
    uint32_t length;
    uint8_t *buffer;
    uint32_t segment_count;
    uint8_t used;
    bsp_enet_rx_desc_t *rx_desc;
} bsp_enet_frame_t;

typedef struct {
    bool interrupt_on_completion;
    bool disable_crc;
    bool disable_pad;
    bool enable_timestamp;
    bool replace_crc;
    bsp_enet_tx_checksum_t checksum;
    bsp_enet_tx_vlan_t vlan;
    bsp_enet_tx_saic_t source_address_insertion;
} bsp_enet_tx_control_config_t;

typedef struct {
    uint32_t mac_addr_high[BSP_ENET_MAC_ADDR_MAX_COUNT];
    uint32_t mac_addr_low[BSP_ENET_MAC_ADDR_MAX_COUNT];
    uint8_t valid_max_count;
    uint8_t dma_pbl;
    bsp_enet_mac_sarc_t source_address_control;
} bsp_enet_mac_config_t;

typedef struct {
    bsp_enet_tx_desc_t *tx_desc_list_head;
    bsp_enet_rx_desc_t *rx_desc_list_head;
    bsp_enet_tx_desc_t *tx_desc_list_cur;
    bsp_enet_rx_desc_t *rx_desc_list_cur;
    bsp_enet_buffer_config_t tx_buffer_config;
    bsp_enet_buffer_config_t rx_buffer_config;
    bsp_enet_rx_frame_info_t rx_frame_info;
    bsp_enet_tx_control_config_t tx_control_config;
} bsp_enet_desc_t;

typedef struct {
    uint32_t second;
    uint32_t nanosecond;
} bsp_enet_ptp_timestamp_t;

typedef struct {
    uint32_t second;
    uint32_t nanosecond;
    uint8_t sign;
} bsp_enet_ptp_timestamp_update_t;

typedef struct {
    uint8_t subsecond_increment;
    bsp_enet_ptp_rollover_t rollover;
    bsp_enet_ptp_update_method_t update_method;
    uint32_t addend;
} bsp_enet_ptp_config_t;

typedef struct {
    uint32_t interval;
    uint32_t width;
    uint32_t target_second;
    uint32_t target_nanosecond;
} bsp_enet_pps_config_t;

typedef struct {
    uint32_t second;
    uint32_t nanosecond;
} bsp_enet_ptp_aux_timestamp_t;

typedef struct {
    bool missed;
    uint8_t count;
    uint8_t index;
} bsp_enet_ptp_aux_status_t;

typedef struct {
    uint32_t interrupt_enable;
    uint32_t interrupt_mask;
    uint32_t mmc_interrupt_rx;
    uint32_t mmc_interrupt_mask_rx;
    uint32_t mmc_interrupt_tx;
    uint32_t mmc_interrupt_mask_tx;
    uint32_t mmc_ipc_interrupt_mask_rx;
} bsp_enet_interrupt_config_t;

int32_t bsp_enet_init(bsp_enet_port_t port);
uintptr_t bsp_enet_get_base_address(bsp_enet_port_t port);
int32_t bsp_enet_init_pins(bsp_enet_port_t port);
int32_t bsp_enet_reset_phy(bsp_enet_port_t port);
int32_t bsp_enet_init_rgmii_clock_delay(bsp_enet_port_t port);
int32_t bsp_enet_init_rmii_reference_clock(bsp_enet_port_t port, bool internal);
int32_t bsp_enet_init_mii_clock(bsp_enet_port_t port);
int32_t bsp_enet_init_ptp_clock(bsp_enet_port_t port);
int32_t bsp_enet_enable_irq(bsp_enet_port_t port);
int32_t bsp_enet_disable_irq(bsp_enet_port_t port);
int32_t bsp_enet_get_dma_pbl(bsp_enet_port_t port, uint8_t *pbl);
int32_t bsp_enet_read_phy(bsp_enet_port_t port, uint32_t phy_addr, uint32_t reg_addr, uint16_t *data);
int32_t bsp_enet_write_phy(bsp_enet_port_t port, uint32_t phy_addr, uint32_t reg_addr, uint16_t data);
int32_t bsp_enet_set_mdio_owner(bsp_mdio_owner_t owner);
int32_t bsp_enet_get_link_state(bsp_enet_port_t port, bsp_enet_link_state_t *state);

int32_t bsp_enet_get_default_tx_control(bsp_enet_port_t port, bsp_enet_tx_control_config_t *config);
int32_t bsp_enet_get_default_interrupt_config(bsp_enet_port_t port, bsp_enet_interrupt_config_t *config);
int32_t bsp_enet_controller_init(bsp_enet_port_t port,
                                 bsp_enet_interface_t inf_type,
                                 bsp_enet_desc_t *desc,
                                 bsp_enet_mac_config_t *mac_config,
                                 bsp_enet_interrupt_config_t *interrupt_config);
int32_t bsp_enet_set_line_speed(bsp_enet_port_t port, bsp_enet_speed_t speed);
int32_t bsp_enet_set_duplex_mode(bsp_enet_port_t port, bsp_enet_duplex_t duplex);
int32_t bsp_enet_rx_resume(bsp_enet_port_t port);
int32_t bsp_enet_tx_desc_chain_init(bsp_enet_port_t port, bsp_enet_desc_t *desc);
int32_t bsp_enet_rx_desc_chain_init(bsp_enet_port_t port, bsp_enet_desc_t *desc);
int32_t bsp_enet_dma_flush(bsp_enet_port_t port);
int32_t bsp_enet_get_interrupt_status(bsp_enet_port_t port, uint32_t *status);
int32_t bsp_enet_mask_interrupt_event(bsp_enet_port_t port, uint32_t mask);
int32_t bsp_enet_unmask_interrupt_event(bsp_enet_port_t port, uint32_t mask);
int32_t bsp_enet_check_received_frame(bsp_enet_rx_desc_t **current_desc,
                                      bsp_enet_rx_frame_info_t *info,
                                      bsp_state_t *available);
int32_t bsp_enet_get_received_frame(bsp_enet_rx_desc_t **current_desc,
                                    bsp_enet_rx_frame_info_t *info,
                                    bsp_enet_frame_t *frame);
int32_t bsp_enet_get_received_frame_interrupt(bsp_enet_rx_desc_t **current_desc,
                                              bsp_enet_rx_frame_info_t *info,
                                              uint32_t rx_desc_count,
                                              bsp_enet_frame_t *frame);
int32_t bsp_enet_prepare_tx_desc(bsp_enet_port_t port,
                                 bsp_enet_tx_desc_t **current_desc,
                                 const bsp_enet_tx_control_config_t *config,
                                 uint16_t frame_length,
                                 uint16_t tx_buffer_size,
                                 bsp_state_t *prepared);
int32_t bsp_enet_prepare_tx_desc_with_timestamp(bsp_enet_port_t port,
                                                bsp_enet_tx_desc_t **current_desc,
                                                const bsp_enet_tx_control_config_t *config,
                                                uint16_t frame_length,
                                                uint16_t tx_buffer_size,
                                                bsp_enet_ptp_timestamp_t *timestamp,
                                                bsp_state_t *prepared);
int32_t bsp_enet_get_default_ptp_config(bsp_enet_port_t port,
                                        uint32_t ptp_clock_hz,
                                        bsp_enet_ptp_config_t *config);
int32_t bsp_enet_init_ptp(bsp_enet_port_t port, const bsp_enet_ptp_config_t *config);
int32_t bsp_enet_set_ptp_timestamp(bsp_enet_port_t port, const bsp_enet_ptp_timestamp_update_t *timestamp);
int32_t bsp_enet_get_ptp_timestamp(bsp_enet_port_t port, bsp_enet_ptp_timestamp_t *timestamp);
int32_t bsp_enet_update_ptp_timeoffset(bsp_enet_port_t port, const bsp_enet_ptp_timestamp_update_t *timeoffset);
int32_t bsp_enet_adjust_ptp_time_freq(bsp_enet_port_t port, int32_t adjustment);
int32_t bsp_enet_set_ptp_version(bsp_enet_port_t port, bsp_enet_ptp_version_t version);
int32_t bsp_enet_enable_ptp_frame_type(bsp_enet_port_t port,
                                       bsp_enet_ptp_frame_type_t frame_type,
                                       bool enable);
int32_t bsp_enet_set_snapshot_ptp_message_type(bsp_enet_port_t port,
                                               bsp_enet_ptp_message_type_t message_type);
int32_t bsp_enet_set_pps_control_output(bsp_enet_port_t port, bsp_enet_pps_control_t control);
int32_t bsp_enet_set_pps_command(bsp_enet_port_t port,
                                 bsp_enet_pps_command_t command,
                                 bsp_enet_pps_index_t index);
int32_t bsp_enet_set_pps_config(bsp_enet_port_t port,
                                const bsp_enet_pps_config_t *config,
                                bsp_enet_pps_index_t index);
int32_t bsp_enet_enable_aux_snapshot(bsp_enet_port_t port, bsp_enet_aux_snapshot_trigger_t trigger);
int32_t bsp_enet_disable_aux_snapshot(bsp_enet_port_t port, bsp_enet_aux_snapshot_trigger_t trigger);
int32_t bsp_enet_get_aux_timestamp(bsp_enet_port_t port, bsp_enet_ptp_aux_timestamp_t *timestamp);
int32_t bsp_enet_get_aux_snapshot_status(bsp_enet_port_t port, bsp_enet_ptp_aux_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ENET_H */
