/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <ph.h>

#include <phfirmware.h>
#include <ntintsafe.h>

// https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/datatypes/hv_partition_privilege_mask
typedef union _HV_PARTITION_PRIVILEGE_MASK
{
    struct
    {
        // Access to virtual MSRs
        UINT64 AccessVpRunTimeReg : 1;
        UINT64 AccessPartitionReferenceCounter : 1;
        UINT64 AccessSynicRegs : 1;
        UINT64 AccessSyntheticTimerRegs : 1;
        UINT64 AccessIntrCtrlRegs : 1;
        UINT64 AccessHypercallMsrs : 1;
        UINT64 AccessVpIndex : 1;
        UINT64 AccessResetReg : 1;
        UINT64 AccessStatsReg : 1;
        UINT64 AccessPartitionReferenceTsc : 1;
        UINT64 AccessGuestIdleReg : 1;
        UINT64 AccessFrequencyRegs : 1;
        UINT64 AccessDebugRegs : 1;
        UINT64 AccessReenlightenmentControls : 1;
        UINT64 Reserved1 : 18;

        // Access to hypercalls
        UINT64 CreatePartitions : 1;
        UINT64 AccessPartitionId : 1;
        UINT64 AccessMemoryPool : 1;
        UINT64 Reserved2 : 1;
        UINT64 PostMessages : 1;
        UINT64 SignalEvents : 1;
        UINT64 CreatePort : 1;
        UINT64 ConnectPort : 1;
        UINT64 AccessStats : 1;
        UINT64 Reserved3 : 2;
        UINT64 Debugging : 1;
        UINT64 CpuManagement : 1;
        UINT64 Reserved4 : 1;
        UINT64 Reserved5 : 1;
        UINT64 Reserved6 : 1;
        UINT64 AccessVSM : 1;
        UINT64 AccessVpRegisters : 1;
        UINT64 Reserved7 : 1;
        UINT64 Reserved8 : 1;
        UINT64 EnableExtendedHypercalls : 1;
        UINT64 StartVirtualProcessor : 1;
        UINT64 Reserved9 : 10;
    };

    UINT32 LowPart;
    UINT32 HighPart;
} HV_PARTITION_PRIVILEGE_MASK, *PHV_PARTITION_PRIVILEGE_MASK;

#if defined(_M_IX86) || defined(_M_AMD64)
typedef union _PH_CPUID
{
    struct
    {
        ULONG EAX;
        ULONG EBX;
        ULONG ECX;
        ULONG EDX;
    };

    struct
    {
        ULONG64 LowPart;
        ULONG64 HighPart;
    };

    INT32 Data[4];
} PH_CPUID, *PPH_CPUID;

VOID PhCpuId(
    _Out_ PPH_CPUID CpuId,
    _In_ ULONG Function,
    _In_ ULONG SubFunction
    )
{
    CpuId->LowPart = 0;
    CpuId->HighPart = 0;
    CpuIdEx(CpuId->Data, Function, SubFunction);
}

BOOLEAN PhCpuIsIntel(
    VOID
    )
{
    PH_CPUID cpuid;

    PhCpuId(&cpuid, 0, 0);

    // GenuineIntel
    if (cpuid.EBX == 'uneG' && cpuid.EDX == 'Ieni' && cpuid.ECX == 'letn')
        return TRUE;

    return FALSE;
}

BOOLEAN PhCpuIsAMD(
    VOID
    )
{
    PH_CPUID cpuid;

    PhCpuId(&cpuid, 0, 0);

    // AuthenticAMD
    if (cpuid.EBX == 'htuA' && cpuid.EDX == 'itne' && cpuid.ECX == 'DMAc')
        return TRUE;

    return FALSE;
}

BOOLEAN PhHypervisorIsPresent(
    VOID
    )
{

    PH_CPUID cpuid;

    PhCpuId(&cpuid, 1, 0);

    // https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery#hypervisor-discovery
    if (cpuid.ECX & 0x80000000)
        return TRUE;

    return FALSE;
}

BOOLEAN PhVCpuIsMicrosoftHyperV(
    VOID
    )
{
    PH_CPUID cpuid;

    // https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery#hypervisor-cpuid-leaf-range---0x40000000
    PhCpuId(&cpuid, 0x40000000, 0);

    // Microsoft Hv
    if (cpuid.EBX == 'rciM' && cpuid.ECX == 'foso' && cpuid.EDX == 'vH t')
        return TRUE;

    return FALSE;
}

VOID PhGetHvPartitionPrivilegeMask(
    _Out_ PHV_PARTITION_PRIVILEGE_MASK Mask
    )
{
    PH_CPUID cpuid;

    // https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery#hypervisor-feature-identification---0x40000003
    PhCpuId(&cpuid, 0x40000003, 0);

    Mask->LowPart = cpuid.EAX;
    Mask->HighPart = cpuid.EBX;
}
#endif

NTSTATUS PhGetHypervisorVendorId(
    _Out_ PULONG VendorId
    )
{
    NTSTATUS status;
    SYSTEM_HYPERVISOR_DETAIL_INFORMATION info;

    *VendorId = 0;

    if (!NT_SUCCESS(status = NtQuerySystemInformation(
        SystemHypervisorDetailInformation,
        &info,
        sizeof(info),
        NULL
        )))
        return status;

    *VendorId = info.HvVendorAndMaxFunction.Data[1];

    return STATUS_SUCCESS;
}

PH_VIRTUAL_STATUS PhGetVirtualStatus(
    VOID
    )
{
#if defined(_ARM64_)
    ULONG vendorId;

    if (NT_SUCCESS(PhGetHypervisorVendorId(&vendorId)))
    {
        if (vendorId == 'MOCQ')
        {
            if (USER_SHARED_DATA->QcSlIsSupported)
            {
                return PhVirtualStatusEnabledFirmware;
            }

            return PhVirtualStatusNotCapable;
        }
        else
        {
            SYSTEM_HYPERVISOR_DETAIL_INFORMATION info;

            if (NT_SUCCESS(NtQuerySystemInformation(
                SystemHypervisorDetailInformation,
                &info,
                sizeof(info),
                NULL
                )))
            {
                if (info.HvFeatures.Data[1] && 0x1000)
                {
                    return PhVirtualStatusEnabledHyperV;
                }
            }

            return PhVirtualStatusVirtualMachine;
        }
    }

    if (USER_SHARED_DATA->ArchStartedInEl2 || USER_SHARED_DATA->QcSlIsSupported)
    {
        return PhVirtualStatusEnabledFirmware;
    }

    return PhVirtualStatusNotCapable;
#else
    if (PhHypervisorIsPresent())
    {
        if (PhVCpuIsMicrosoftHyperV())
        {
            HV_PARTITION_PRIVILEGE_MASK mask;

            PhGetHvPartitionPrivilegeMask(&mask);

            if (mask.AccessVpRunTimeReg)
            {
                return PhVirtualStatusEnabledHyperV;
            }
        }

        return PhVirtualStatusVirtualMachine;
    }

    if (PhCpuIsIntel())
    {
        PH_CPUID cpuid;

        PhCpuId(&cpuid, 1, 0);

        // Virtual Machine Extensions (VMX)
        if (!(cpuid.ECX & 0x20))
        {
            return PhVirtualStatusNotCapable;
        }
    }
    else if (PhCpuIsAMD())
    {
        PH_CPUID cpuid;

        PhCpuId(&cpuid, 0x80000001, 0);

        // Secure Virtual Machine (SVM)
        if (!(cpuid.ECX & 0x2))
        {
            return PhVirtualStatusNotCapable;
        }
    }
    else
    {
        //return PhVirtualStatusUnknown;
        return PhVirtualStatusNotCapable;
    }

    if (USER_SHARED_DATA->ProcessorFeatures[PF_VIRT_FIRMWARE_ENABLED])
    {
        return PhVirtualStatusEnabledFirmware;
    }

    if (USER_SHARED_DATA->ProcessorFeatures[PF_SECOND_LEVEL_ADDRESS_TRANSLATION] &&
        USER_SHARED_DATA->ProcessorFeatures[PF_NX_ENABLED])
    {
        return PhVirtualStatusDiabledWithHyperV;
    }

    return PhVirtualStatusDiabled;
#endif
}

typedef struct _PH_SMBIOS_STRINGREF
{
    USHORT Length;
    PSTR Buffer;
} PH_SMBIOS_STRINGREF, *PPH_SMBIOS_STRINGREF;

typedef struct _PH_SMBIOS_PARSE_CONTEXT
{
    PRAW_SMBIOS_DATA Data;
    PVOID End;
    PH_SMBIOS_STRINGREF Strings[256];
} PH_SMBIOS_PARSE_CONTEXT, *PPH_SMBIOS_PARSE_CONTEXT;

NTSTATUS PhpParseSMBIOS(
    _In_ PPH_SMBIOS_PARSE_CONTEXT Context,
    _In_ PSMBIOS_HEADER Current,
    _Out_ PSMBIOS_HEADER* Next
    )
{
    NTSTATUS status;
    PSTR pointer;
    PSTR string;
    UCHAR nulls;
    USHORT length;
    UCHAR count;

    *Next = NULL;

    memset(Context->Strings, 0, sizeof(Context->Strings));

    pointer = SMBIOS_STRING_TABLE(Current);
    string = pointer;

    if ((ULONG_PTR)pointer >= (ULONG_PTR)Context->End)
        return STATUS_BUFFER_OVERFLOW;

    nulls = 0;
    length = 0;
    count = 0;

    while ((ULONG_PTR)pointer < (ULONG_PTR)Context->End)
    {
        if (*pointer++ != ANSI_NULL)
        {
            if (!NT_SUCCESS(status = RtlUShortAdd(length, 1, &length)))
                return status;

            nulls = 0;
            continue;
        }

        if (nulls++)
            break;

        Context->Strings[count].Length = length;
        Context->Strings[count].Buffer = string;

        count++;
        string = pointer;
        length = 0;
    }

    if (nulls != 2)
        return STATUS_INVALID_ADDRESS;

    if (Current->Type != SMBIOS_END_OF_TABLE_TYPE)
        *Next = (PSMBIOS_HEADER)pointer;

    return STATUS_SUCCESS;
}

NTSTATUS PhpEnumSMBIOS(
    _In_ PRAW_SMBIOS_DATA Data,
    _In_ PPH_ENUM_SMBIOS_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PH_SMBIOS_PARSE_CONTEXT context;
    PSMBIOS_HEADER current;
    PSMBIOS_HEADER next;

    memset(&context, 0, sizeof(context));

    context.Data = Data;
    context.End = PTR_ADD_OFFSET(Data->SMBIOSTableData, Data->Length);

    current = (PSMBIOS_HEADER)Data->SMBIOSTableData;
    next = NULL;

    while (NT_SUCCESS(status = PhpParseSMBIOS(&context, current, &next)))
    {
        if (Callback(
            (ULONG_PTR)&context,
            Data->SMBIOSMajorVersion,
            Data->SMBIOSMinorVersion,
            (PPH_SMBIOS_ENTRY)current,
            Context
            ))
            break;

        if (!next)
            break;

        current = next;
    }

    return status;
}

NTSTATUS PhEnumSMBIOS(
    _In_ PPH_ENUM_SMBIOS_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    static volatile ULONG cachedLength = sizeof(PSYSTEM_FIRMWARE_TABLE_INFORMATION) + 64;
    NTSTATUS status;
    ULONG length;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;

    length = (ULONG)ReadAcquire((volatile LONG*)&cachedLength);

    info = PhAllocateZero(length);

    info->ProviderSignature = 'RSMB';
    info->TableID = 0;
    info->Action = SystemFirmwareTableGet;
    info->TableBufferLength = length - sizeof(PSYSTEM_FIRMWARE_TABLE_INFORMATION);

    status = NtQuerySystemInformation(
        SystemFirmwareTableInformation,
        info,
        length,
        &length
        );
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(info);
        info = PhAllocateZero(length);

        info->ProviderSignature = 'RSMB';
        info->TableID = 0;
        info->Action = SystemFirmwareTableGet;
        info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

        status = NtQuerySystemInformation(
            SystemFirmwareTableInformation,
            info,
            length,
            &length
            );
        if (NT_SUCCESS(status))
            WriteRelease((volatile LONG*)&cachedLength, (LONG)length);
    }

    if (NT_SUCCESS(status))
    {
        status = PhpEnumSMBIOS(
            (PRAW_SMBIOS_DATA)info->TableBuffer,
            Callback,
            Context
            );
    }

    return status;
}

NTSTATUS PhGetSMBIOSString(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR Index,
    _Out_ PPH_STRING* String
    )
{
    PPH_SMBIOS_PARSE_CONTEXT context;
    PPH_SMBIOS_STRINGREF string;

    context = (PPH_SMBIOS_PARSE_CONTEXT)EnumHandle;

    *String = NULL;

    if (Index == SMBIOS_INVALID_STRING)
        return STATUS_INVALID_PARAMETER;

    string = &context->Strings[Index - 1];
    if (!string->Buffer)
        return STATUS_INVALID_PARAMETER;

    *String = PhConvertUtf8ToUtf16Ex(string->Buffer, string->Length);

    return STATUS_SUCCESS;
}
