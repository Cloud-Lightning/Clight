#pragma once

extern "C" {
#include "board.h"
#include "common.h"
#include "netconf.h"
#include "sys_arch.h"
#include "lwip/init.h"
}

#include <cstdint>

#include "Eth.hpp"

class EthLwipStack {
public:
    using AppInitCallback = void (*)();

    explicit EthLwipStack(Eth eth = Eth(EthId::Port0))
        : eth_(eth)
    {
    }

    Status start(AppInitCallback appInit = nullptr)
    {
        auto status = eth_.selectMdioOwner(MdioOwner::Enet);
        if (status != Status::Ok) {
            return status;
        }

        auto *base = reinterpret_cast<ENET_Type *>(eth_.baseAddress());
        if (base == nullptr) {
            return Status::Error;
        }

        if (enet_init(base) != status_success) {
            return Status::Error;
        }

        dumpRtl8211Negotiation("after-init");
        lwip_init();
        netif_config(&gnetif);
        syncRtl8211Link();
        dumpRtl8211Negotiation("after-sync");
        enet_services(&gnetif);

        if (appInit != nullptr) {
            appInit();
        }

        started_ = true;
        return Status::Ok;
    }

    void poll()
    {
        if (!started_) {
            return;
        }
        sys_timer_callback();
        ++pollCount_;
        if ((pollCount_ % 100U) == 0U) {
            enet_self_adaptive_port_speed();
        }
        enet_common_handler(&gnetif);
    }

    bool started() const { return started_; }

    Status linkState(EthLinkState &state) const
    {
        return eth_.linkState(state);
    }

    struct netif *netif() { return &gnetif; }
    const struct netif *netif() const { return &gnetif; }

    Eth &eth() { return eth_; }
    const Eth &eth() const { return eth_; }

private:
    static bool bitSet(std::uint16_t value, std::uint16_t mask)
    {
        return (value & mask) != 0U;
    }

    Status selectEnetMdioOwner()
    {
        return eth_.selectMdioOwner(MdioOwner::Enet);
    }

#if defined(__USE_RTL8211) && __USE_RTL8211
    Status readPhyRegister(std::uint32_t phyAddr, std::uint32_t regAddr, std::uint16_t &value)
    {
        if (selectEnetMdioOwner() != Status::Ok) {
            return Status::Error;
        }
        return eth_.readPhy(phyAddr, regAddr, value);
    }

    Status writePhyRegister(std::uint32_t phyAddr, std::uint32_t regAddr, std::uint16_t value)
    {
        if (selectEnetMdioOwner() != Status::Ok) {
            return Status::Error;
        }
        return eth_.writePhy(phyAddr, regAddr, value);
    }

    Status readPagedPhyRegister(std::uint32_t phyAddr, std::uint16_t page, std::uint16_t regAddr, std::uint16_t &value)
    {
        constexpr std::uint32_t pageSelectReg = 0x1FU;
        constexpr std::uint16_t pageUnlock = 0x0A42U;

        std::uint16_t oldPage = 0U;
        if ((readPhyRegister(phyAddr, pageSelectReg, oldPage) != Status::Ok) ||
            (writePhyRegister(phyAddr, pageSelectReg, pageUnlock) != Status::Ok) ||
            (writePhyRegister(phyAddr, pageSelectReg, page) != Status::Ok) ||
            (readPhyRegister(phyAddr, regAddr, value) != Status::Ok)) {
            return Status::Error;
        }

        return writePhyRegister(phyAddr, pageSelectReg, oldPage);
    }

    Status configureRtl8211fRgmiiDelay()
    {
        constexpr std::uint32_t phyAddr = BOARD_ENET_PHY_ADDR_RTL8211;
        constexpr std::uint16_t pageSelectReg = 0x1FU;
        constexpr std::uint16_t rgmiiPage = 0xD08U;
        constexpr std::uint16_t rgmiiReg = 0x11U;
        constexpr std::uint16_t rxDelayBit = static_cast<std::uint16_t>(1U << 15);
        constexpr std::uint16_t txDelayBit = static_cast<std::uint16_t>(1U << 13);

        std::uint16_t oldPage = 0U;
        std::uint16_t rgmii = 0U;
        if ((readPhyRegister(phyAddr, pageSelectReg, oldPage) != Status::Ok) ||
            (writePhyRegister(phyAddr, pageSelectReg, 0x0A42U) != Status::Ok) ||
            (writePhyRegister(phyAddr, pageSelectReg, rgmiiPage) != Status::Ok) ||
            (readPhyRegister(phyAddr, rgmiiReg, rgmii) != Status::Ok)) {
            return Status::Error;
        }

        const std::uint16_t updated = static_cast<std::uint16_t>(rgmii & ~(rxDelayBit | txDelayBit));
        if (updated != rgmii) {
            if (writePhyRegister(phyAddr, rgmiiReg, updated) != Status::Ok) {
                return Status::Error;
            }
        }

        if (writePhyRegister(phyAddr, pageSelectReg, oldPage) != Status::Ok) {
            return Status::Error;
        }

        printf("[PHY/FIX] RTL8211F page 0xd08 reg 0x11: 0x%04x -> 0x%04x (RX_DLY=0 TX_DLY=0)\n",
               static_cast<unsigned>(rgmii),
               static_cast<unsigned>(updated));
        return Status::Ok;
    }

    void dumpRtl8211Negotiation(const char *tag)
    {
        constexpr std::uint32_t phyAddr = BOARD_ENET_PHY_ADDR_RTL8211;
        constexpr std::uint16_t regBmcr = 0x00U;
        constexpr std::uint16_t regBmsr = 0x01U;
        constexpr std::uint16_t regAnar = 0x04U;
        constexpr std::uint16_t regAnlpar = 0x05U;
        constexpr std::uint16_t regGbcr = 0x09U;
        constexpr std::uint16_t regGbsr = 0x0AU;
        constexpr std::uint16_t regRgmiiPage = 0xD08U;
        constexpr std::uint16_t regRgmii = 0x11U;
        constexpr std::uint16_t regPhysrPage = 0xA43U;
        constexpr std::uint16_t regPhysr = 0x1AU;

        std::uint16_t bmcr = 0U;
        std::uint16_t bmsr = 0U;
        std::uint16_t anar = 0U;
        std::uint16_t anlpar = 0U;
        std::uint16_t gbcr = 0U;
        std::uint16_t gbsr = 0U;
        std::uint16_t physr = 0U;
        std::uint16_t rgmii = 0U;

        if ((readPhyRegister(phyAddr, regBmcr, bmcr) != Status::Ok) ||
            (readPhyRegister(phyAddr, regBmsr, bmsr) != Status::Ok) ||
            (readPhyRegister(phyAddr, regAnar, anar) != Status::Ok) ||
            (readPhyRegister(phyAddr, regAnlpar, anlpar) != Status::Ok) ||
            (readPhyRegister(phyAddr, regGbcr, gbcr) != Status::Ok) ||
            (readPhyRegister(phyAddr, regGbsr, gbsr) != Status::Ok) ||
            (readPagedPhyRegister(phyAddr, regPhysrPage, regPhysr, physr) != Status::Ok) ||
            (readPagedPhyRegister(phyAddr, regRgmiiPage, regRgmii, rgmii) != Status::Ok)) {
            printf("[PHY/NEG] %s dump failed\n", tag);
            return;
        }

        printf("[PHY/NEG] %s BMCR=0x%04x BMSR=0x%04x ANAR=0x%04x ANLPAR=0x%04x GBCR=0x%04x GBSR=0x%04x PHYSR=0x%04x RGMII=0x%04x\n",
               tag,
               static_cast<unsigned>(bmcr),
               static_cast<unsigned>(bmsr),
               static_cast<unsigned>(anar),
               static_cast<unsigned>(anlpar),
               static_cast<unsigned>(gbcr),
               static_cast<unsigned>(gbsr),
               static_cast<unsigned>(physr),
               static_cast<unsigned>(rgmii));

        printf("[PHY/NEG] %s an_en=%d an_done=%d link=%d 1000_local_fd=%d 1000_lp_fd=%d 100_local_fd=%d 100_lp_fd=%d 10_local_fd=%d 10_lp_fd=%d ms_fault=%d\n",
               tag,
               bitSet(bmcr, 0x1000U) ? 1 : 0,
               bitSet(bmsr, 0x0020U) ? 1 : 0,
               bitSet(physr, 0x0004U) ? 1 : 0,
               bitSet(gbcr, 0x0200U) ? 1 : 0,
               bitSet(gbsr, 0x0800U) ? 1 : 0,
               bitSet(anar, 0x0100U) ? 1 : 0,
               bitSet(anlpar, 0x0100U) ? 1 : 0,
               bitSet(anar, 0x0040U) ? 1 : 0,
               bitSet(anlpar, 0x0040U) ? 1 : 0,
               bitSet(gbsr, 0x8000U) ? 1 : 0);
    }
#else
    Status configureRtl8211fRgmiiDelay()
    {
        return Status::Ok;
    }

    void dumpRtl8211Negotiation(const char *tag)
    {
        (void)tag;
    }
#endif

    void syncRtl8211Link()
    {
        static constexpr std::uint32_t syncTimeoutMs = 3000U;
        static constexpr std::uint32_t syncStepMs = 50U;

        if (configureRtl8211fRgmiiDelay() != Status::Ok) {
            printf("[PHY/FIX] RTL8211F RGMII delay adjust failed\n");
        }

        for (std::uint32_t elapsedMs = 0U; elapsedMs <= syncTimeoutMs; elapsedMs += syncStepMs) {
            if (selectEnetMdioOwner() != Status::Ok) {
                break;
            }

            enet_self_adaptive_port_speed();

            EthLinkState link{};
            if ((eth_.linkState(link) == Status::Ok) &&
                link.linkUp &&
                (link.speed != EthSpeed::Unknown) &&
                (link.duplex != EthDuplex::Unknown)) {
                printf("[PHY] startup link synced in %lu ms\n",
                       static_cast<unsigned long>(elapsedMs));
                return;
            }

            board_delay_ms(syncStepMs);
        }

        printf("[PHY] startup link sync timed out, continue with current settings\n");
    }

    Eth eth_;
    bool started_ = false;
    std::uint32_t pollCount_ = 0U;
};
