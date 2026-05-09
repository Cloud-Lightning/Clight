#pragma once

extern "C" {
#include "ecat_def.h"
#include "ecatappl.h"
#include "ecatslv.h"
#include "applInterface.h"
#include "digital_io.h"
#include "hpm_ecat_hw.h"
}

#include <cstdint>

#include "Ecat.hpp"

struct EcatSscStatus {
    std::uint16_t inputSizeBytes = 0U;
    std::uint16_t outputSizeBytes = 0U;
    std::uint16_t outputCounter7010 = 0U;
    std::uint16_t inputCounter6000 = 0U;
    std::uint32_t rxPdoUpdates = 0U;
    std::uint32_t txPdoUpdates = 0U;
    std::uint16_t alStatus = 0U;
    std::uint16_t alStatusCode = 0U;
    std::uint16_t dlStatus = 0U;
    std::uint8_t phy0Status = 0U;
    std::uint8_t lostLinkCount0 = 0U;
    bool linkUp = false;
    bool runLedActive = false;
    bool errorLedActive = false;
};

class EcatSscStack {
public:
    explicit EcatSscStack(Ecat ecat = Ecat(EcatId::Port0))
        : ecat_(ecat)
    {
    }

    Status start(bool initLocalIo = true)
    {
        auto status = ecat_.init();
        if (status != Status::Ok) {
            return status;
        }

        if (initLocalIo) {
            status = ecat_.initSwitchLed();
            if (status != Status::Ok) {
                return status;
            }
        }

        status = ecat_.selectMdioOwner(MdioOwner::Esc);
        if (status != Status::Ok) {
            return status;
        }

        auto *base = reinterpret_cast<ESC_Type *>(ecat_.baseAddress());
        if (base == nullptr) {
            return Status::Error;
        }

        if (ecat_hardware_init(base) != status_success) {
            return Status::Error;
        }

        if (MainInit() != 0U) {
            return Status::Error;
        }

        ecat_debug_dump_startup(base);

#if defined(ESC_EEPROM_EMULATION) && ESC_EEPROM_EMULATION
        pAPPL_EEPROM_Read = ecat_eeprom_emulation_read;
        pAPPL_EEPROM_Write = ecat_eeprom_emulation_write;
        pAPPL_EEPROM_Reload = ecat_eeprom_emulation_reload;
        pAPPL_EEPROM_Store = ecat_eeprom_emulation_store;
#endif

        const auto mappingStatus = APPL_GenerateMapping(&nPdInputSize, &nPdOutputSize);
        if (mappingStatus != 0U) {
            return Status::Error;
        }

        ecat_preconfigure_process_data_sm(base, nPdOutputSize, nPdInputSize);

        bRunApplication = TRUE;
        started_ = true;
        return Status::Ok;
    }

    void poll()
    {
        if (!started_ || (bRunApplication != TRUE)) {
            return;
        }
        MainLoop();
        auto *base = reinterpret_cast<ESC_Type *>(ecat_.baseAddress());
        if (base != nullptr) {
            ecat_debug_poll(base);
        }
    }

    bool running() const
    {
        return started_ && (bRunApplication == TRUE);
    }

    void stop()
    {
        bRunApplication = FALSE;
    }

    void processDataSize(std::uint16_t &inputSize, std::uint16_t &outputSize) const
    {
        inputSize = nPdInputSize;
        outputSize = nPdOutputSize;
    }

    std::uint16_t outputCounter() const
    {
        return OutputCounter0x7010;
    }

    std::uint16_t inputCounter() const
    {
        return InputCounter0x6000;
    }

    std::uint32_t rxPdoUpdates() const
    {
        return EcatRxPdoUpdateCount;
    }

    std::uint32_t txPdoUpdates() const
    {
        return EcatTxPdoUpdateCount;
    }

    std::uint32_t timerMs() const
    {
        return static_cast<std::uint32_t>(HW_GetTimer());
    }

    EcatSscStatus readStatus() const
    {
        EcatSscStatus status{};
        processDataSize(status.inputSizeBytes, status.outputSizeBytes);
        status.outputCounter7010 = outputCounter();
        status.inputCounter6000 = inputCounter();
        status.rxPdoUpdates = rxPdoUpdates();
        status.txPdoUpdates = txPdoUpdates();

        ecat_led_status_t leds{};
        ecat_get_led_status(&leds);
        status.runLedActive = leds.run_active;
        status.errorLedActive = leds.error_active;

        auto *base = reinterpret_cast<ESC_Type *>(ecat_.baseAddress());
        if (base != nullptr) {
            status.alStatus = base->AL_STAT;
            status.alStatusCode = base->AL_STAT_CODE;
            status.dlStatus = base->ESC_DL_STAT;
            status.phy0Status = base->PHY_STAT[ESC_PHY_STAT_PORT0];
            status.lostLinkCount0 = base->LOST_LINK_CNT[ESC_LOST_LINK_CNT_PORT0];
            status.linkUp = (ESC_ESC_DL_STAT_PLP0_GET(status.dlStatus) != 0U);
        }

        return status;
    }

    static bool isOperational(std::uint16_t alStatus)
    {
        return ((alStatus & STATE_MASK) == STATE_OP);
    }

    static const char *stateText(std::uint16_t alStatus)
    {
        switch (static_cast<std::uint8_t>(alStatus & STATE_MASK)) {
        case STATE_INIT:
            return "INIT";
        case STATE_PREOP:
            return "PREOP";
        case STATE_BOOT:
            return "BOOT";
        case STATE_SAFEOP:
            return "SAFEOP";
        case STATE_OP:
            return "OP";
        default:
            return "UNKNOWN";
        }
    }

    Ecat &ecat() { return ecat_; }
    const Ecat &ecat() const { return ecat_; }

private:
    Ecat ecat_;
    bool started_ = false;
};
