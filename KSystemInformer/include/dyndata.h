#ifndef DYNDATA_H
#define DYNDATA_H

#ifdef EXT
#undef EXT
#endif

#ifdef _DYNDATA_PRIVATE
#define EXT
#define OFFDEFAULT = ULONG_MAX
#define DYNIMPORTDEFAULT = NULL
#else
#define EXT extern
#define OFFDEFAULT
#define DYNIMPORTDEFAULT
#endif

EXT ULONG KphDynNtVersion;
EXT RTL_OSVERSIONINFOEXW KphDynOsVersionInfo;

// Structures
// Ege: ETW_GUID_ENTRY
// Ep: EPROCESS
// Ere: ETW_REG_ENTRY
// Et: ETHREAD
// Ht: HANDLE_TABLE
// Oh: OBJECT_HEADER
// Ot: OBJECT_TYPE
// Oti: OBJECT_TYPE_INITIALIZER, offset measured from an OBJECT_TYPE
// ObDecodeShift: shift value in ObpDecodeObject
// ObAttributesShift: shift value in ObpGetHandleAttributes
EXT ULONG KphDynEgeGuid OFFDEFAULT;
EXT ULONG KphDynEpObjectTable OFFDEFAULT;
EXT ULONG KphDynEreGuidEntry OFFDEFAULT;
EXT ULONG KphDynHtHandleContentionEvent OFFDEFAULT;
EXT ULONG KphDynOtName OFFDEFAULT;
EXT ULONG KphDynOtIndex OFFDEFAULT;
EXT ULONG KphDynObDecodeShift OFFDEFAULT;
EXT ULONG KphDynObAttributesShift OFFDEFAULT;

EXT PPS_GET_PROCESS_PROTECTION KphDynPsGetProcessProtection DYNIMPORTDEFAULT;
EXT PRTL_IMAGE_NT_HEADER_EX KphDynRtlImageNtHeaderEx DYNIMPORTDEFAULT;

NTSTATUS KphDynamicDataInitialization(
    VOID
    );

NTSTATUS KphReadDynamicDataParameters(
    _In_opt_ HANDLE KeyHandle
    );

#endif
