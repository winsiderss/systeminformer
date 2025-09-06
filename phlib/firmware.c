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

// https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/feature-discovery
// private
typedef enum _HV_CPUID_FUNCTION
{
    HvCpuIdFunctionVersionAndFeatures = 0x00000001, // CPUID.01h.ECX:31 // if set, virtualization present
    HvCpuIdFunctionHvVendorAndMaxFunction = 0x40000000, // HV_VENDOR_AND_MAX_FUNCTION
    HvCpuIdFunctionHvInterface = 0x40000001, // HV_HYPERVISOR_INTERFACE_INFO
    HvCpuIdFunctionMsHvVersion = 0x40000002, // HV_HYPERVISOR_VERSION_INFO
    HvCpuIdFunctionMsHvFeatures = 0x40000003, // HV_HYPERVISOR_FEATURES/HV_X64_HYPERVISOR_FEATURES
    HvCpuIdFunctionMsHvEnlightenmentInformation = 0x40000004, // HV_ENLIGHTENMENT_INFORMATION
    HvCpuIdFunctionMsHvImplementationLimits = 0x40000005, // HV_IMPLEMENTATION_LIMITS
    HvCpuIdFunctionMsHvHardwareFeatures = 0x40000006, // HV_X64_HYPERVISOR_HARDWARE_FEATURES
    HvCpuIdFunctionMsHvCpuManagementFeatures = 0x40000007, // HV_X64_HYPERVISOR_CPU_MANAGEMENT_FEATURES
    HvCpuIdFunctionMsHvSvmFeatures = 0x40000008, // HV_HYPERVISOR_SVM_FEATURES
    HvCpuIdFunctionMsHvSkipLevelFeatures = 0x40000009, // HV_HYPERVISOR_SKIP_LEVEL_FEATURES // 1511+
    HvCpuidFunctionMsHvNestedVirtFeatures = 0x4000000A, // HV_HYPERVISOR_NESTED_VIRT_FEATURES
    HvCpuidFunctionMsHvIptFeatures = 0x4000000B, // HV_HYPERVISOR_IPT_FEATURES // 1903+
    HvCpuIdFunctionMaxReserved
} HV_CPUID_FUNCTION, *PHV_CPUID_FUNCTION;

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

// private
typedef struct _HV_VENDOR_AND_MAX_FUNCTION
{
    //
    // Eax
    //
    UINT32 MaxFunctions;

    //
    // Ebx, Ecx, and Edx
    //
    UINT8 VendorName[12];
} HV_VENDOR_AND_MAX_FUNCTION, *PHV_VENDOR_AND_MAX_FUNCTION;
static_assert(sizeof(HV_VENDOR_AND_MAX_FUNCTION) == 16);

// private
typedef enum _HV_HYPERVISOR_INTERFACE
{
    HvMicrosoftHypervisorInterface = 0x31237648, // '1#vH'
    HvMicrosoftXboxNanovisor = 0x766E6258 ,      // 'vnbX'
} HV_HYPERVISOR_INTERFACE, *PHV_HYPERVISOR_INTERFACE;

// private
typedef struct _HV_HYPERVISOR_INTERFACE_INFO
{
    //
    // Eax
    //
    UINT32 Interface; // HV_HYPERVISOR_INTERFACE

    //
    // Ebx
    //
    UINT32 Reserved1;

    //
    // Ecx
    //
    UINT32 Reserved2;

    //
    // Edx
    //
    UINT32 Reserved3;
} HV_HYPERVISOR_INTERFACE_INFO, *PHV_HYPERVISOR_INTERFACE_INFO;
static_assert(sizeof(HV_HYPERVISOR_INTERFACE_INFO) == 16);

// private
typedef struct _HV_HYPERVISOR_VERSION_INFO
{
    //
    // Eax
    //
    UINT32 BuildNumber;

    //
    // Ebx
    //
    UINT32 MinorVersion : 16;
    UINT32 MajorVersion : 16;

    //
    // Ecx
    //
    UINT32 ServicePack;

    //
    // Edx
    //
    UINT32 ServiceNumber : 24;
    UINT32 ServiceBranch : 8;
} HV_HYPERVISOR_VERSION_INFO, *PHV_HYPERVISOR_VERSION_INFO;
static_assert(sizeof(HV_HYPERVISOR_VERSION_INFO) == 16);

// private
typedef struct _HV_HYPERVISOR_FEATURES
{
    //
    // Eax and Ebx
    //
    HV_PARTITION_PRIVILEGE_MASK PartitionPrivileges;

    //
    // Ecx
    //
    UINT32 MaxSupportedCState : 4;
    UINT32 HpetNeededForC3PowerState_Deprecated : 1;
    UINT32 Reserved : 27;

    //
    // Edx
    //
    UINT32 MwaitAvailable_Deprecated : 1;
    UINT32 GuestDebuggingAvailable : 1;
    UINT32 PerformanceMonitorsAvailable : 1;
    UINT32 CpuDynamicPartitioningAvailable : 1;
    UINT32 XmmRegistersForFastHypercallAvailable : 1;
    UINT32 GuestIdleAvailable : 1;
    UINT32 HypervisorSleepStateSupportAvailable : 1;
    UINT32 NumaDistanceQueryAvailable : 1;
    UINT32 FrequencyRegsAvailable : 1;
    UINT32 SyntheticMachineCheckAvailable : 1;
    UINT32 GuestCrashRegsAvailable : 1;
    UINT32 DebugRegsAvailable : 1;
    UINT32 Npiep1Available : 1;
    UINT32 DisableHypervisorAvailable : 1;
    UINT32 Reserved1 : 18;
} HV_HYPERVISOR_FEATURES, *PHV_HYPERVISOR_FEATURES;
static_assert(sizeof(HV_HYPERVISOR_FEATURES) == 16);

// private
typedef struct _HV_X64_HYPERVISOR_FEATURES
{
    //
    // Eax and Ebx
    //
    HV_PARTITION_PRIVILEGE_MASK PartitionPrivileges;

    //
    // Ecx
    //
    UINT32 MaxSupportedCState : 4;
    UINT32 HpetNeededForC3PowerState_Deprecated : 1;
    UINT32 Reserved : 27;

    //
    // Edx
    //
    UINT32 MwaitAvailable_Deprecated : 1;
    UINT32 GuestDebuggingAvailable : 1;
    UINT32 PerformanceMonitorsAvailable : 1;
    UINT32 CpuDynamicPartitioningAvailable : 1;
    UINT32 XmmRegistersForFastHypercallAvailable : 1;
    UINT32 GuestIdleAvailable : 1;
    UINT32 HypervisorSleepStateSupportAvailable : 1;
    UINT32 NumaDistanceQueryAvailable : 1;
    UINT32 FrequencyRegsAvailable : 1;
    UINT32 SyntheticMachineCheckAvailable : 1;
    UINT32 GuestCrashRegsAvailable : 1;
    UINT32 DebugRegsAvailable : 1;
    UINT32 Npiep1Available : 1;
    UINT32 DisableHypervisorAvailable : 1;
    UINT32 ExtendedGvaRangesForFlushVirtualAddressListAvailable : 1;
    UINT32 FastHypercallOutputAvailable : 1;
    UINT32 SvmFeaturesAvailable : 1;
    UINT32 SintPollingModeAvailable : 1;
    UINT32 HypercallMsrLockAvailable : 1; // 1511+
    UINT32 DirectSyntheticTimers : 1; // 1607+
    UINT32 RegisterPatAvailable : 1; // 1703+
    UINT32 RegisterBndcfgsAvailable : 1;
    UINT32 WatchdogTimerAvailable : 1;
    UINT32 SyntheticTimeUnhaltedTimerAvailable : 1; // 1709+
    UINT32 DeviceDomainsAvailable : 1;
    UINT32 S1DeviceDomainsAvailable : 1; // 1803+
    UINT32 LbrAvailable : 1; // 1809+
    UINT32 IptAvailable : 1; // 1903+
    UINT32 CrossVtlFlushAvailable : 1;
    UINT32 IdleSpecCtrlAvailable : 1; // 2004+
    UINT32 Reserved1 : 2;
} HV_X64_HYPERVISOR_FEATURES, *PHV_X64_HYPERVISOR_FEATURES;
static_assert(sizeof(HV_X64_HYPERVISOR_FEATURES) == 16);

// private
typedef struct _HV_ENLIGHTENMENT_INFORMATION
{
    //
    // Eax
    //
    UINT32 UseHypercallForAddressSpaceSwitch : 1;
    UINT32 UseHypercallForLocalFlush : 1;
    UINT32 UseHypercallForRemoteFlush : 1;
    UINT32 UseApicMsrs : 1; // UseMsrForApicAccess
    UINT32 UseMsrForReset : 1;
    UINT32 UseRelaxedTiming : 1;
    UINT32 UseDmaRemapping : 1;
    UINT32 UseInterruptRemapping : 1;
    UINT32 UseX2ApicMsrs : 1;
    UINT32 DeprecateAutoEoi : 1;
    UINT32 Reserved : 22;

    //
    // Ebx
    //
    UINT32 LongSpinWaitCount;

    //
    // Ecx
    //
    UINT32 ReservedEcx;

    //
    // Edx
    //
    UINT32 ReservedEdx;
} HV_ENLIGHTENMENT_INFORMATION, *PHV_ENLIGHTENMENT_INFORMATION;
static_assert(sizeof(HV_ENLIGHTENMENT_INFORMATION) == 16);

// private
typedef struct _HV_IMPLEMENTATION_LIMITS
{
    //
    // Eax
    //
    UINT32 MaxVirtualProcessorCount;

    //
    // Ebx
    //
    UINT32 MaxLogicalProcessorCount;

    //
    // Ecx
    //
    UINT32 MaxInterruptMappingCount;

    //
    // Edx
    //
    UINT32 Reserved;
} HV_IMPLEMENTATION_LIMITS, *PHV_IMPLEMENTATION_LIMITS;
static_assert(sizeof(HV_IMPLEMENTATION_LIMITS) == 16);

// private
typedef struct _HV_X64_HYPERVISOR_HARDWARE_FEATURES
{
    //
    // Eax
    //
    UINT32 ApicOverlayAssistInUse : 1;
    UINT32 MsrBitmapsInUse : 1;
    UINT32 ArchitecturalPerformanceCountersInUse : 1;
    UINT32 SecondLevelAddressTranslationInUse : 1;
    UINT32 DmaRemappingInUse : 1;
    UINT32 InterruptRemappingInUse : 1;
    UINT32 MemoryPatrolScrubberPresent : 1;
    UINT32 DmaProtectionInUse : 1;
    UINT32 HpetRequested : 1;
    UINT32 SyntheticTimersVolatile : 1;
    UINT32 HypervisorLevel : 4;
    UINT32 PhysicalDestinationModeRequired : 1;
    UINT32 UseVmfuncForAliasMapSwitch : 1;
    UINT32 HvRegisterForMemoryZeroingSupported : 1;
    UINT32 UnrestrictedGuestSupported : 1;
    UINT32 RdtAFeaturesSupported : 1;
    UINT32 RdtMFeaturesSupported : 1;
    UINT32 ChildPerfmonPmuSupported : 1;
    UINT32 ChildPerfmonLbrSupported : 1;
    UINT32 ChildPerfmonIptSupported : 1;
    UINT32 ApicEmulationSupported : 1;
    UINT32 ChildX2ApicRecommended : 1;
    UINT32 HardwareWatchdogReserved : 1;
    UINT32 DeviceAccessTrackingSupported : 1;
    UINT32 Reserved : 5;

    //
    // Ebx
    //
    UINT32 DeviceDomainInputWidth : 8;
    UINT32 ReservedEbx : 24;

    //
    // Ecx
    //
    UINT32 ReservedEcx;

    //
    // Edx
    //
    UINT32 ReservedEdx;
} HV_X64_HYPERVISOR_HARDWARE_FEATURES, *PHV_X64_HYPERVISOR_HARDWARE_FEATURES;
static_assert(sizeof(HV_X64_HYPERVISOR_HARDWARE_FEATURES) == 16);

#undef LogicalProcessorIdling
// private
typedef struct _HV_X64_HYPERVISOR_CPU_MANAGEMENT_FEATURES
{
    //
    // Eax
    //
    UINT32 StartLogicalProcessor : 1;
    UINT32 CreateRootVirtualProcessor : 1;
    UINT32 PerformanceCounterSync : 1; // 1703+
    UINT32 Reserved0 : 28;
    UINT32 ReservedIdentityBit : 1;

    //
    // Ebx
    //
    UINT32 ProcessorPowerManagement : 1;
    UINT32 MwaitIdleStates : 1;
    UINT32 LogicalProcessorIdling : 1;
    UINT32 Reserved1 : 29;

    //
    // Ecx
    //
    UINT32 RemapGuestUncached : 1; // 1803+
    UINT32 ReservedZ2 : 31;

    //
    // Edx
    //
    UINT32 ReservedEdx;
} HV_X64_HYPERVISOR_CPU_MANAGEMENT_FEATURES, *PHV_X64_HYPERVISOR_CPU_MANAGEMENT_FEATURES;
static_assert(sizeof(HV_X64_HYPERVISOR_CPU_MANAGEMENT_FEATURES) == 16);

// private
typedef struct _HV_HYPERVISOR_SVM_FEATURES
{
    //
    // Eax
    //
    UINT32 SvmSupported : 1;
    UINT32 Reserved0 : 10;
    UINT32 MaxPasidSpacePasidCount : 21;

    //
    // Ebx
    //
    UINT32 MaxPasidSpaceCount;

    //
    // Ecx
    //
    UINT32 MaxDevicePrqSize;

    //
    // Edx
    //
    UINT32 Reserved1;
} HV_HYPERVISOR_SVM_FEATURES, *PHV_HYPERVISOR_SVM_FEATURES;
static_assert(sizeof(HV_HYPERVISOR_SVM_FEATURES) == 16);

// private
typedef struct _HV_HYPERVISOR_SKIP_LEVEL_FEATURES
{
    //
    // Eax
    //
    UINT32 Reserved0 : 2;
    UINT32 AccessSynicRegs : 1;
    UINT32 Reserved1 : 1;
    UINT32 AccessIntrCtrlRegs : 1;
    UINT32 AccessHypercallMsrs : 1;
    UINT32 AccessVpIndex : 1;
    UINT32 Reserved2 : 5;
    UINT32 AccessReenlightenmentControls : 1;
    UINT32 Reserved3 : 19;

    //
    // Ebx
    //
    UINT32 ReservedEbx;

    //
    // Ecx
    //
    UINT32 ReservedEcx;

    //
    // Edx
    //
    UINT32 Reserved4 : 4;
    UINT32 XmmRegistersForFastHypercallAvailable : 1;
    UINT32 Reserved5 : 10;
    UINT32 FastHypercallOutputAvailable : 1;
    UINT32 Reserved6 : 1;
    UINT32 SintPollingModeAvailable : 1;
    UINT32 Reserved7 : 14;
} HV_HYPERVISOR_SKIP_LEVEL_FEATURES, *PHV_HYPERVISOR_SKIP_LEVEL_FEATURES;
static_assert(sizeof(HV_HYPERVISOR_SKIP_LEVEL_FEATURES) == 16);

// private
typedef struct _HV_HYPERVISOR_NESTED_VIRT_FEATURES
{
    //
    // Eax
    //
    UINT32 EnlightedVmcsVersionLow : 8;
    UINT32 EnlightedVmcsVesionHigh : 8;
    UINT32 FlushGuestPhysicalHypercall_Deprecated : 1; // 1607+
    UINT32 NestedFlushVirtualHypercall : 1;
    UINT32 FlushGuestPhysicalHypercall : 1;
    UINT32 MsrBitmap : 1;
    UINT32 VirtualizationException : 1; // 1709+
    UINT32 DebugCtl : 1; // 2004+
    UINT32 Reserved0 : 10;

    //
    // Ebx
    //
    UINT32 PerfGlobalCtrl : 1;
    UINT32 Reserved1 : 31;

    //
    // Ecx
    //
    UINT32 ReservedEcx;

    //
    // Edx
    //
    UINT32 ReservedEdx;
} HV_HYPERVISOR_NESTED_VIRT_FEATURES, *PHV_HYPERVISOR_NESTED_VIRT_FEATURES;
static_assert(sizeof(HV_HYPERVISOR_NESTED_VIRT_FEATURES) == 16);

// private
typedef struct _HV_HYPERVISOR_IPT_FEATURES
{
    //
    // Eax
    //
    UINT32 ChainedToPA : 1;
    UINT32 Enlightened : 1;
    UINT32 Reserved0 : 10;
    UINT32 MaxTraceBufferSizePerVtl : 20;

    //
    // Ebx
    //
    UINT32 Reserved1;

    //
    // Ecx
    //
    UINT32 Reserved2;

    //
    // Ecx
    //
    UINT32 HypervisorIpt : 1; // 2004+
    UINT32 Reserved3 : 31;
} HV_HYPERVISOR_IPT_FEATURES, *PHV_HYPERVISOR_IPT_FEATURES;
static_assert(sizeof(HV_HYPERVISOR_IPT_FEATURES) == 16);

// private
typedef struct _HV_X64_PLATFORM_CAPABILITIES
{
    UINT64 AsUINT64[2];

    struct
    {
        //
        // Eax
        //
        UINT AllowRedSignedCode : 1;
        UINT AllowKernelModeDebugging : 1;
        UINT AllowUserModeDebugging : 1;
        UINT AllowTelnetServer : 1;
        UINT AllowIOPorts : 1;
        UINT AllowFullMsrSpace : 1;
        UINT AllowPerfCounters : 1;
        UINT AllowHost512MB : 1;
        UINT AllowRemoteRecovery : 1;
        UINT AllowStreaming : 1;
        UINT AllowPushDeployment : 1;
        UINT AllowPullDeployment : 1;
        UINT AllowProfiling : 1;
        UINT AllowJsProfiling : 1;
        UINT AllowCrashDump : 1;
        UINT AllowVsCrashDump : 1;
        UINT AllowToolFileIO : 1;
        UINT AllowConsoleMgmt : 1;
        UINT AllowTracing : 1;
        UINT AllowXStudio : 1;
        UINT AllowGestureBuilder : 1;
        UINT AllowSpeechLab : 1;
        UINT AllowSmartglassStudio : 1;
        UINT AllowNetworkTools : 1;
        UINT AllowTcrTool : 1;
        UINT AllowHostNetworkStack : 1;
        UINT AllowSystemUpdateTest : 1;
        UINT AllowOffChipPerfCtrStreaming : 1;
        UINT AllowToolingMemory : 1;
        UINT AllowSystemDowngrade : 1;
        UINT AllowGreenDiskLicenses : 1;

        //
        // Ebx
        //
        UINT IsLiveConnected : 1;
        UINT IsMteBoosted : 1;
        UINT IsQaSlt : 1;
        UINT IsStockImage : 1;
        UINT IsMsTestLab : 1;
        UINT IsRetailDebugger : 1;
        UINT IsXvdSort : 1;
        UINT IsGreenDebug : 1;
        UINT IsHwDevTest : 1;
        UINT AllowDiskLicenses : 1; // 1511+
        UINT AllowInstrumentation : 1;
        UINT AllowWifiTester : 1;
        UINT AllowWifiTesterDFS : 1;
        UINT IsHwTest : 1;
        UINT AllowHostOddTest : 1;
        UINT IsLiveUnrestricted : 1;
        UINT AllowDiscLicensesWithoutMediaAuth : 1;
        UINT ReservedEbx : 15;

        //
        // Ecx
        //
        UINT ReservedEcx;

        //
        // Edx
        //
        UINT ReservedEdx : 31;
        UINT UseAlternateXvd : 1;
    };
} HV_X64_PLATFORM_CAPABILITIES, *PHV_X64_PLATFORM_CAPABILITIES;
static_assert(sizeof(HV_HYPERVISOR_IPT_FEATURES) == 16);

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

    PhCpuId(&cpuid, HvCpuIdFunctionVersionAndFeatures, 0);

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

    PhCpuId(&cpuid, HvCpuIdFunctionHvVendorAndMaxFunction, 0);

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

    PhCpuId(&cpuid, HvCpuIdFunctionMsHvFeatures, 0);

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
                if (info.HvFeatures.Data[1] & 0x1000)
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

    if (!(info = PhAllocatePageZero(length)))
        return STATUS_NO_MEMORY;

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
        PhFreePage(info);

        if (!(info = PhAllocatePageZero(length)))
            return STATUS_NO_MEMORY;

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
        {
            WriteRelease((volatile LONG*)&cachedLength, (LONG)length);
        }
    }

    if (NT_SUCCESS(status))
    {
        status = PhpEnumSMBIOS(
            (PRAW_SMBIOS_DATA)info->TableBuffer,
            Callback,
            Context
            );
    }

    PhFreePage(info);

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
