#include "bsp_enet.h"

int32_t bsp_enet_init(bsp_enet_port_t port)
{
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

uintptr_t bsp_enet_get_base_address(bsp_enet_port_t port)
{
    (void) port;
    return 0U;
}

int32_t bsp_enet_init_pins(bsp_enet_port_t port)
{
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_reset_phy(bsp_enet_port_t port)
{
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_init_rgmii_clock_delay(bsp_enet_port_t port)
{
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_init_rmii_reference_clock(bsp_enet_port_t port, bool internal)
{
    (void) internal;
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_init_mii_clock(bsp_enet_port_t port)
{
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_init_ptp_clock(bsp_enet_port_t port)
{
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_enable_irq(bsp_enet_port_t port)
{
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_disable_irq(bsp_enet_port_t port)
{
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_get_dma_pbl(bsp_enet_port_t port, uint8_t *pbl)
{
    (void) pbl;
    if ((port >= BSP_ENET_PORT_MAX) || (pbl == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_read_phy(bsp_enet_port_t port, uint32_t phy_addr, uint32_t reg_addr, uint16_t *data)
{
    (void) phy_addr;
    (void) reg_addr;
    if ((port >= BSP_ENET_PORT_MAX) || (data == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_write_phy(bsp_enet_port_t port, uint32_t phy_addr, uint32_t reg_addr, uint16_t data)
{
    (void) phy_addr;
    (void) reg_addr;
    (void) data;
    if (port >= BSP_ENET_PORT_MAX) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_set_mdio_owner(bsp_mdio_owner_t owner)
{
    (void) owner;
    return BSP_ERROR_UNSUPPORTED;
}

int32_t bsp_enet_get_link_state(bsp_enet_port_t port, bsp_enet_link_state_t *state)
{
    if ((port >= BSP_ENET_PORT_MAX) || (state == nullptr)) {
        return BSP_ERROR_PARAM;
    }
    return BSP_ERROR_UNSUPPORTED;
}
