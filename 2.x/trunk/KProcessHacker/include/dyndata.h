#ifndef DYNDATA_H
#define DYNDATA_H

typedef NTSTATUS (NTAPI *_PsTerminateProcess)(
    __in PEPROCESS Process,
    __in NTSTATUS ExitStatus
    );

typedef NTSTATUS (NTAPI *_PspTerminateThreadByPointer51)(
    __in PETHREAD Thread,
    __in NTSTATUS ExitStatus
    );

typedef NTSTATUS (NTAPI *_PspTerminateThreadByPointer52)(
    __in PETHREAD Thread,
    __in NTSTATUS ExitStatus,
    __in BOOLEAN DirectTerminate
    );

typedef struct _KPH_PROCEDURE_SCAN
{
    BOOLEAN Initialized;
    BOOLEAN Scanned;
    PUCHAR Bytes;
    ULONG Length;
    ULONG_PTR StartAddress;
    ULONG ScanLength;
    LONG Displacement;

    PVOID ProcedureAddress; // scan result
} KPH_PROCEDURE_SCAN, *PKPH_PROCEDURE_SCAN;

#ifdef EXT
#undef EXT
#endif

#ifdef _DYNDATA_PRIVATE
#define EXT
#define OFFDEFAULT = -1
#else
#define EXT extern
#define OFFDEFAULT
#endif

EXT ULONG KphDynNtVersion;
EXT RTL_OSVERSIONINFOEXW KphDynOsVersionInfo;

// Structures
// Ege: ETW_GUID_ENTRY
// Ep: EPROCESS
// Ere: ETW_REG_ENTRY
// Et: ETHREAD
// Oh: OBJECT_HEADER
// Ot: OBJECT_TYPE
// Oti: OBJECT_TYPE_INITIALIZER, offset measured from an OBJECT_TYPE
EXT ULONG KphDynEgeGuid OFFDEFAULT;
EXT ULONG KphDynEpObjectTable OFFDEFAULT;
EXT ULONG KphDynEpProtectedProcessOff OFFDEFAULT;
EXT ULONG KphDynEpProtectedProcessBit OFFDEFAULT;
EXT ULONG KphDynEpRundownProtect OFFDEFAULT;
EXT ULONG KphDynEreGuidEntry OFFDEFAULT;
EXT ULONG KphDynOtName OFFDEFAULT;
EXT ULONG KphDynOtIndex OFFDEFAULT;

// Procedures
EXT KPH_PROCEDURE_SCAN KphDynPsTerminateProcessScan;
EXT KPH_PROCEDURE_SCAN KphDynPspTerminateThreadByPointerScan;

NTSTATUS KphDynamicDataInitialization(
    VOID
    );

PVOID KphGetDynamicProcedureScan(
    __inout PKPH_PROCEDURE_SCAN ProcedureScan
    );

#endif
