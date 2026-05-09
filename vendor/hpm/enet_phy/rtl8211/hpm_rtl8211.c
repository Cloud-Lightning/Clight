/*
 * Copyright (c) 2021-2025 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*---------------------------------------------------------------------
 * Includes
 *---------------------------------------------------------------------
 */
#include "hpm_enet_drv.h"
#include "hpm_rtl8211_regs.h"
#include "hpm_rtl8211.h"
#include <stdio.h>

/*---------------------------------------------------------------------
 * Internal state
 *---------------------------------------------------------------------
 */
static bool s_is_rtl8211f;            /* true = RTL8211F (paged access), false = RTL8211E (direct access) */

/*---------------------------------------------------------------------
 * Internal API
 *---------------------------------------------------------------------
 */
static bool rtl8211_check_id(ENET_Type *ptr, uint32_t phy_addr)
{
    uint16_t id1, id2;

    id1 = enet_read_phy(ptr, phy_addr, RTL8211_PHYID1);
    id2 = enet_read_phy(ptr, phy_addr, RTL8211_PHYID2);

    if (RTL8211_PHYID1_OUI_MSB_GET(id1) == RTL8211_ID1 && RTL8211_PHYID2_OUI_LSB_GET(id2) == RTL8211_ID2) {
        return true;
    } else {
        return false;
    }
}

/* Select page for RTL8211F extended register access.
   Reg 31 (0x1F): first write 0xA42 to unlock extended access,
   then write the target page. Pass 0 to restore standard registers. */
static void rtl8211f_page_select(ENET_Type *ptr, uint32_t phy_addr, uint16_t page)
{
    if (page != 0U) {
        enet_write_phy(ptr, phy_addr, RTL8211_PAGE_SELECT, 0xA42U);
    }
    enet_write_phy(ptr, phy_addr, RTL8211_PAGE_SELECT, page);
}

/* Read a register from the given page on RTL8211F.
   Saves and restores the original page. */
static uint16_t rtl8211f_read_phy_paged(ENET_Type *ptr, uint32_t phy_addr, uint16_t page, uint16_t reg_addr)
{
    uint16_t old_page = enet_read_phy(ptr, phy_addr, RTL8211_PAGE_SELECT);

    rtl8211f_page_select(ptr, phy_addr, page);
    uint16_t data = enet_read_phy(ptr, phy_addr, reg_addr);
    rtl8211f_page_select(ptr, phy_addr, old_page);

    return data;
}

/* Detect whether this is an RTL8211F by reading PHYSR (Page 0, Reg 17).
   RTL8211E: register 17 returns valid status data (has non-zero bits).
   RTL8211F: register 17 in page 0 is unused / returns 0x0000.
   Additionally, try reading RTL8211F's PHYSR1 from page 0xA43, reg 0x1A
   to confirm the PHY responds there. */
static bool rtl8211_detect_variant(ENET_Type *ptr, uint32_t phy_addr)
{
    uint16_t physr_page0 = enet_read_phy(ptr, phy_addr, RTL8211_PHYSR);

    /* RTL8211E PHYSR should have at minimum the NWay Enable bit (bit 12)
       set by default and some capability bits.  If it reads 0x0000, this
       is likely an RTL8211F which stores status in a paged register. */
    if (physr_page0 == 0x0000U) {
        /* Confirm by reading the RTL8211F PHYSR1 via paged access.
           If we get a non-zero response, it's RTL8211F. */
        uint16_t physr1 = rtl8211f_read_phy_paged(ptr, phy_addr, RTL8211F_PHYSR1_PAGE, RTL8211F_PHYSR1_REG);
        if (physr1 != 0x0000U && physr1 != 0xFFFFU) {
            return true;  /* RTL8211F */
        }
    }

    return false;  /* RTL8211E or unknown */
}

/*---------------------------------------------------------------------
 * API
 *---------------------------------------------------------------------
 */
bool rtl8211_reset(ENET_Type *ptr, uint32_t phy_addr)
{
    uint16_t data;
    uint32_t retry_cnt = ENET_PHY_SW_RESET_RETRY_CNT;

    /* PHY reset */
    enet_write_phy(ptr, phy_addr, RTL8211_BMCR, RTL8211_BMCR_RESET_SET(1));

    /* wait until the reset is completed */
    do {
        data = enet_read_phy(ptr, phy_addr, RTL8211_BMCR);
    } while (RTL8211_BMCR_RESET_GET(data) && --retry_cnt);

    return retry_cnt > 0 ? true : false;
}

void rtl8211_basic_mode_default_config(ENET_Type *ptr, rtl8211_config_t *config)
{
    (void)ptr;

    config->loopback         = false;                        /* Disable PCS loopback mode */
    #if defined(__DISABLE_AUTO_NEGO) && (__DISABLE_AUTO_NEGO)
    config->auto_negotiation = false;                        /* Disable Auto-Negotiation */
    config->speed            = enet_phy_port_speed_100mbps;
    config->duplex           = enet_phy_duplex_full;
    #else
    config->auto_negotiation = true;                         /* Enable Auto-Negotiation */
    #endif
}

bool rtl8211_basic_mode_init(ENET_Type *ptr, uint32_t phy_addr, rtl8211_config_t *config)
{
    uint16_t data = 0;

    data |= RTL8211_BMCR_RESET_SET(0)                        /* Normal operation */
         |  RTL8211_BMCR_LOOPBACK_SET(config->loopback)      /* configure PCS loopback mode */
         |  RTL8211_BMCR_ANE_SET(config->auto_negotiation)   /* configure Auto-Negotiation */
         |  RTL8211_BMCR_PWD_SET(0)                          /* Normal operation */
         |  RTL8211_BMCR_ISOLATE_SET(0)                      /* Normal operation */
         |  RTL8211_BMCR_RESTART_AN_SET(0)                   /* Normal operation */
         |  RTL8211_BMCR_COLLISION_TEST_SET(0);              /* Normal operation */

    if (config->auto_negotiation == 0) {
        data |= RTL8211_BMCR_SPEED0_SET(config->speed) | RTL8211_BMCR_SPEED1_SET(config->speed >> 1);
        data |= RTL8211_BMCR_DUPLEX_SET(config->duplex);
    }

    /* check the id of rtl8211 */
    if (rtl8211_check_id(ptr, phy_addr) == false) {
        return false;
    }

    enet_write_phy(ptr, phy_addr, RTL8211_BMCR, data);

    /* Auto-detect RTL8211E vs RTL8211F variant */
    s_is_rtl8211f = rtl8211_detect_variant(ptr, phy_addr);

    if (s_is_rtl8211f) {
        /* RTL8211F: Enable RGMII TX+RX internal delays.
           Page 0xd08, Register 0x11:
             Bit 15 (RX_DLY): shift RXC by 1.5-2.0ns
             Bit 13 (TX_DLY): shift PHY internal clock by 1.5-2.0ns
           The page_select now writes 0xA42 to unlock extended register
           access before setting the target page. */
        uint16_t rgmii = rtl8211f_read_phy_paged(ptr, phy_addr, 0xd08U, 0x11U);
        printf("[PHY] RTL8211F RGMII reg 0xd08.11 before=0x%04x\n", (unsigned)rgmii);
        rgmii |= (1U << 15) | (1U << 13);
        rtl8211f_page_select(ptr, phy_addr, 0xd08U);
        enet_write_phy(ptr, phy_addr, 0x11U, rgmii);
        rtl8211f_page_select(ptr, phy_addr, 0U);
        rgmii = rtl8211f_read_phy_paged(ptr, phy_addr, 0xd08U, 0x11U);
        printf("[PHY] RTL8211F RGMII reg 0xd08.11 after =0x%04x (RX_DLY=%d TX_DLY=%d)\n",
               (unsigned)rgmii,
               (int)((rgmii >> 15) & 1U),
               (int)((rgmii >> 13) & 1U));
    }

    return true;
}


void rtl8211_get_phy_status(ENET_Type *ptr, uint32_t phy_addr, enet_phy_status_t *status)
{
    uint16_t data;
    uint8_t raw_speed;

    if (s_is_rtl8211f) {
        /* RTL8211F: PHYSR1 at Page 0xA43, Address 0x1A with different bit layout */
        data = rtl8211f_read_phy_paged(ptr, phy_addr, RTL8211F_PHYSR1_PAGE, RTL8211F_PHYSR1_REG);

        status->enet_phy_link = RTL8211F_PHYSR_LINK_REAL_TIME_GET(data);

        raw_speed = RTL8211F_PHYSR_SPEED_GET(data);
        status->enet_phy_speed = (raw_speed == 2U) ? enet_phy_port_speed_1000mbps
                               : (raw_speed == 1U) ? enet_phy_port_speed_100mbps
                               : enet_phy_port_speed_10mbps;

        status->enet_phy_duplex = RTL8211F_PHYSR_DUPLEX_GET(data);
    } else {
        /* RTL8211E: PHYSR at Page 0, Address 0x11 (17) */
        data = enet_read_phy(ptr, phy_addr, RTL8211_PHYSR);

        status->enet_phy_link = RTL8211_PHYSR_LINK_REAL_TIME_GET(data);

        raw_speed = RTL8211_PHYSR_SPEED_GET(data);
        status->enet_phy_speed = (raw_speed == 0U) ? enet_phy_port_speed_10mbps
                               : (raw_speed == 1U) ? enet_phy_port_speed_100mbps
                               : enet_phy_port_speed_1000mbps;

        status->enet_phy_duplex = RTL8211_PHYSR_DUPLEX_GET(data);
    }
}
