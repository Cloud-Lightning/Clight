/*
 * Minimal EtherCAT digital IO profile for bring-up:
 * - RxPDO 0x1600 -> 0x7010 (16-bit output word)
 * - TxPDO 0x1A00 -> 0x6000 (16-bit input word)
 */

#ifndef _DIGITAL_IO_H_
#define _DIGITAL_IO_H_

#include "objdef.h"

typedef struct OBJ_STRUCT_PACKED_START {
    UINT16 u16SubIndex0;
    UINT32 aEntries[1];
} OBJ_STRUCT_PACKED_END TOBJ1600;

typedef struct OBJ_STRUCT_PACKED_START {
    UINT16 u16SubIndex0;
    UINT32 aEntries[1];
} OBJ_STRUCT_PACKED_END TOBJ1A00;

typedef struct OBJ_STRUCT_PACKED_START {
    UINT16 u16SubIndex0;
    UINT16 aEntries[1];
} OBJ_STRUCT_PACKED_END TOBJ1C12;

typedef TOBJ1C12 TOBJ1C13;

#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
#define PROTO
#else
#define PROTO extern
#endif

PROTO TOBJ1C12 sRxPDOassign
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {1, {0x1600}}
#endif
;

PROTO TOBJ1C13 sTxPDOassign
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {1, {0x1A00}}
#endif
;

PROTO TOBJ1600 sRxPDOMap0
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {1, {0x70100010}}
#endif
;

PROTO TOBJ1A00 sTxPDOMap0
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {1, {0x60000010}}
#endif
;

PROTO UINT16 OutputCounter0x7010
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;

PROTO UINT16 InputCounter0x6000
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;

PROTO UINT32 EcatRxPdoUpdateCount
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;

PROTO UINT32 EcatTxPdoUpdateCount
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= 0
#endif
;

PROTO OBJCONST TSDOINFOENTRYDESC OBJMEM asPDOAssignEntryDesc[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {
    {DEFTYPE_UNSIGNED8, 0x08, ACCESS_READ},
    {DEFTYPE_UNSIGNED16, 0x10, ACCESS_READ}
}
#endif
;

PROTO OBJCONST TSDOINFOENTRYDESC OBJMEM asEntryDesc0x1600[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {
    {DEFTYPE_UNSIGNED8, 0x08, ACCESS_READ},
    {DEFTYPE_UNSIGNED32, 0x20, ACCESS_READ}
}
#endif
;

PROTO OBJCONST TSDOINFOENTRYDESC OBJMEM asEntryDesc0x1A00[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {
    {DEFTYPE_UNSIGNED8, 0x08, ACCESS_READ},
    {DEFTYPE_UNSIGNED32, 0x20, ACCESS_READ}
}
#endif
;

PROTO OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x6000
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {DEFTYPE_UNSIGNED16, 0x10, (ACCESS_READ | OBJACCESS_TXPDOMAPPING)}
#endif
;

PROTO OBJCONST TSDOINFOENTRYDESC OBJMEM sEntryDesc0x7010
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {DEFTYPE_UNSIGNED16, 0x10, (ACCESS_READWRITE | OBJACCESS_RXPDOMAPPING)}
#endif
;

PROTO OBJCONST UCHAR OBJMEM aName0x1C12[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= "RxPDO assign"
#endif
;

PROTO OBJCONST UCHAR OBJMEM aName0x1C13[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= "TxPDO assign"
#endif
;

PROTO OBJCONST UCHAR OBJMEM aName0x1600[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= "RxPDO-Map\000\377"
#endif
;

PROTO OBJCONST UCHAR OBJMEM aName0x1A00[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= "TxPDO-Map\000\377"
#endif
;

PROTO OBJCONST UCHAR OBJMEM aName0x6000[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= "Input Counter"
#endif
;

PROTO OBJCONST UCHAR OBJMEM aName0x7010[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= "Output Counter"
#endif
;

PROTO TOBJECT OBJMEM ApplicationObjDic[]
#if defined(_DIGITAL_IO_) && (_DIGITAL_IO_ == 1)
= {
    {NULL, NULL, 0x1C12, {DEFTYPE_UNSIGNED16, 1 | (OBJCODE_ARR << 8)}, asPDOAssignEntryDesc, aName0x1C12, &sRxPDOassign, NULL, NULL, 0x0000},
    {NULL, NULL, 0x1C13, {DEFTYPE_UNSIGNED16, 1 | (OBJCODE_ARR << 8)}, asPDOAssignEntryDesc, aName0x1C13, &sTxPDOassign, NULL, NULL, 0x0000},
    {NULL, NULL, 0x1600, {DEFTYPE_PDOMAPPING, 1 | (OBJCODE_REC << 8)}, asEntryDesc0x1600, aName0x1600, &sRxPDOMap0, NULL, NULL, 0x0000},
    {NULL, NULL, 0x1A00, {DEFTYPE_PDOMAPPING, 1 | (OBJCODE_REC << 8)}, asEntryDesc0x1A00, aName0x1A00, &sTxPDOMap0, NULL, NULL, 0x0000},
    {NULL, NULL, 0x6000, {DEFTYPE_UNSIGNED16, 0 | (OBJCODE_VAR << 8)}, &sEntryDesc0x6000, aName0x6000, &InputCounter0x6000, NULL, NULL, 0x0000},
    {NULL, NULL, 0x7010, {DEFTYPE_UNSIGNED16, 0 | (OBJCODE_VAR << 8)}, &sEntryDesc0x7010, aName0x7010, &OutputCounter0x7010, NULL, NULL, 0x0000},
    {NULL, NULL, 0xFFFF, {0, 0}, NULL, NULL, NULL, NULL, NULL, 0x0000}
}
#endif
;

PROTO void APPL_AckErrorInd(UINT16 stateTrans);
PROTO UINT16 APPL_StartMailboxHandler(void);
PROTO UINT16 APPL_StopMailboxHandler(void);
PROTO UINT16 APPL_StartInputHandler(UINT16 *pIntMask);
PROTO UINT16 APPL_StopInputHandler(void);
PROTO UINT16 APPL_StartOutputHandler(void);
PROTO UINT16 APPL_StopOutputHandler(void);
PROTO UINT16 APPL_GenerateMapping(UINT16 *pInputSize, UINT16 *pOutputSize);
PROTO void APPL_InputMapping(UINT16 *pData);
PROTO void APPL_OutputMapping(UINT16 *pData);
PROTO void APPL_Application(void);
#if EXPLICIT_DEVICE_ID
PROTO UINT16 APPL_GetDeviceID(void);
#endif

#undef PROTO

#endif /* _DIGITAL_IO_H_ */
