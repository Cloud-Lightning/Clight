/*
 * Minimal 16-bit output / 16-bit input EtherCAT application for bring-up.
 */

#include "ecat_def.h"
#include "applInterface.h"
#include "board.h"
#include "hpm_gpio_drv.h"

#define _DIGITAL_IO_ 1
#include "digital_io.h"
#undef _DIGITAL_IO_

static UINT16 read_switch_word(void)
{
    UINT16 value = 0;

    value |= (UINT16)(gpio_read_pin(BOARD_ECAT_IN1_GPIO, BOARD_ECAT_IN1_GPIO_PORT_INDEX, BOARD_ECAT_IN1_GPIO_PIN_INDEX) ? 1U : 0U) << 0U;
    value |= (UINT16)(gpio_read_pin(BOARD_ECAT_IN2_GPIO, BOARD_ECAT_IN2_GPIO_PORT_INDEX, BOARD_ECAT_IN2_GPIO_PIN_INDEX) ? 1U : 0U) << 1U;
    return value;
}

static void write_leds_from_word(UINT16 value)
{
    gpio_write_pin(BOARD_ECAT_OUT1_GPIO,
                   BOARD_ECAT_OUT1_GPIO_PORT_INDEX,
                   BOARD_ECAT_OUT1_GPIO_PIN_INDEX,
                   (value & 0x0001U) ? BOARD_ECAT_OUT_ON_LEVEL : !BOARD_ECAT_OUT_ON_LEVEL);
    gpio_write_pin(BOARD_ECAT_OUT2_GPIO,
                   BOARD_ECAT_OUT2_GPIO_PORT_INDEX,
                   BOARD_ECAT_OUT2_GPIO_PIN_INDEX,
                   (value & 0x0002U) ? BOARD_ECAT_OUT_ON_LEVEL : !BOARD_ECAT_OUT_ON_LEVEL);
    gpio_write_pin(BOARD_ECAT_OUT3_GPIO,
                   BOARD_ECAT_OUT3_GPIO_PORT_INDEX,
                   BOARD_ECAT_OUT3_GPIO_PIN_INDEX,
                   (value & 0x0004U) ? BOARD_ECAT_OUT_ON_LEVEL : !BOARD_ECAT_OUT_ON_LEVEL);
    gpio_write_pin(BOARD_ECAT_OUT4_GPIO,
                   BOARD_ECAT_OUT4_GPIO_PORT_INDEX,
                   BOARD_ECAT_OUT4_GPIO_PIN_INDEX,
                   (value & 0x0008U) ? BOARD_ECAT_OUT_ON_LEVEL : !BOARD_ECAT_OUT_ON_LEVEL);
}

void APPL_AckErrorInd(UINT16 stateTrans)
{
    (void) stateTrans;
}

UINT16 APPL_StartMailboxHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StopMailboxHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StartInputHandler(UINT16 *pIntMask)
{
    (void) pIntMask;
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StopInputHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StartOutputHandler(void)
{
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_StopOutputHandler(void)
{
    OutputCounter0x7010 = 0U;
    write_leds_from_word(0U);
    return ALSTATUSCODE_NOERROR;
}

UINT16 APPL_GenerateMapping(UINT16 *pInputSize, UINT16 *pOutputSize)
{
    UINT16 result = ALSTATUSCODE_NOERROR;
    UINT16 InputSize = 0;
    UINT16 OutputSize = 0;

#if COE_SUPPORTED
    UINT16 PDOAssignEntryCnt = 0;
    OBJCONST TOBJECT OBJMEM *pPDO = NULL;
    UINT16 PDOSubindex0 = 0;
    UINT32 *pPDOEntry = NULL;
    UINT16 PDOEntryCnt = 0;

    for (PDOAssignEntryCnt = 0; PDOAssignEntryCnt < sRxPDOassign.u16SubIndex0; PDOAssignEntryCnt++) {
        pPDO = OBJ_GetObjectHandle(sRxPDOassign.aEntries[PDOAssignEntryCnt]);
        if (pPDO != NULL) {
            PDOSubindex0 = *((UINT16 *)pPDO->pVarPtr);
            for (PDOEntryCnt = 0; PDOEntryCnt < PDOSubindex0; PDOEntryCnt++) {
                pPDOEntry = (UINT32 *)(((UINT16 *)pPDO->pVarPtr) + (OBJ_GetEntryOffset((PDOEntryCnt + 1), pPDO) >> 4));
                OutputSize += (UINT16)((*pPDOEntry) & 0xFFU);
            }
        } else {
            OutputSize = 0;
            result = ALSTATUSCODE_INVALIDOUTPUTMAPPING;
            break;
        }
    }
    OutputSize = (OutputSize + 7U) >> 3U;

    if (result == 0U) {
        for (PDOAssignEntryCnt = 0; PDOAssignEntryCnt < sTxPDOassign.u16SubIndex0; PDOAssignEntryCnt++) {
            pPDO = OBJ_GetObjectHandle(sTxPDOassign.aEntries[PDOAssignEntryCnt]);
            if (pPDO != NULL) {
                PDOSubindex0 = *((UINT16 *)pPDO->pVarPtr);
                for (PDOEntryCnt = 0; PDOEntryCnt < PDOSubindex0; PDOEntryCnt++) {
                    pPDOEntry = (UINT32 *)(((UINT16 *)pPDO->pVarPtr) + (OBJ_GetEntryOffset((PDOEntryCnt + 1), pPDO) >> 4));
                    InputSize += (UINT16)((*pPDOEntry) & 0xFFU);
                }
            } else {
                InputSize = 0;
                result = ALSTATUSCODE_INVALIDINPUTMAPPING;
                break;
            }
        }
    }
    InputSize = (InputSize + 7U) >> 3U;
#endif

    *pInputSize = InputSize;
    *pOutputSize = OutputSize;
    return result;
}

void APPL_InputMapping(UINT16 *pData)
{
    MEMCPY(pData, &InputCounter0x6000, SIZEOF(InputCounter0x6000));
    ++EcatTxPdoUpdateCount;
}

void APPL_OutputMapping(UINT16 *pData)
{
    MEMCPY(&OutputCounter0x7010, pData, SIZEOF(OutputCounter0x7010));
    ++EcatRxPdoUpdateCount;
}

void APPL_Application(void)
{
    InputCounter0x6000 = read_switch_word();
    write_leds_from_word(OutputCounter0x7010);
}

#if EXPLICIT_DEVICE_ID
UINT16 APPL_GetDeviceID(void)
{
    return 0x5U;
}
#endif
