/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <hwprvp.h>
#include <kphuser.h>
#include <phfirmware.h>
#include <SetupAPI.h>

#define PHP_HW_LPIT_HEADER 36 // ACPI table header
#define PHP_HW_LPIT_NATIVE_LENGTH 56

/**
 * Determines whether the driver is connected at the maximum level.
 */
BOOLEAN PhpHwDriverReady(
    VOID
    )
{
    return KsiLevel() == KphLevelMax;
}

/**
 * Retrieves the architecture the application is built for.
 *
 * \return An IMAGE_FILE_MACHINE_* value.
 */
USHORT PhpHwQueryMachine(
    VOID
    )
{
#if defined(_M_ARM64)
    return IMAGE_FILE_MACHINE_ARM64;
#elif defined(_M_X64)
    return IMAGE_FILE_MACHINE_AMD64;
#else
    return IMAGE_FILE_MACHINE_I386;
#endif
}

/**
 * Determines whether the processor has core performance boost (CPUID 8000_0007h EDX bit 9), with the application's
 * own CPUID: a feature bit, not the processor identity.
 *
 * \return TRUE when the processor reports it.
 */
BOOLEAN PhpHwQueryCoreBoost(
    VOID
    )
{
#if defined(_M_IX86) || defined(_M_X64)
    INT cpuInfo[4];

    __cpuid(cpuInfo, 0x80000000);

    if ((ULONG)cpuInfo[0] < 0x80000007)
        return FALSE;

    __cpuid(cpuInfo, 0x80000007);

    return !!(cpuInfo[3] & (1 << 9));
#else
    return FALSE;
#endif
}

/**
 * Retrieves the build number of Windows.
 *
 * \return The build number.
 */
ULONG PhpHwQueryBuildNumber(
    VOID
    )
{
    return PhOsVersion.BuildNumber;
}

static ULONG PhpHwFlatProcessorIndex(
    _In_ PPHP_HW_TOPOLOGY Topology,
    _In_ USHORT Group,
    _In_ ULONG Number
    )
{
    for (ULONG i = 0; i < Topology->ProcessorCount; i++)
    {
        if (Topology->Processors[i].Number.Group == Group && Topology->Processors[i].Number.Number == Number)
            return i;
    }

    return ULONG_MAX;
}

/**
 * Numbers the cores or packages and writes each processor's index of them.
 *
 * \param Topology The processors.
 * \param Relationship RelationProcessorCore or RelationProcessorPackage.
 * \param Count Receives the number of cores or packages.
 * \return TRUE when every relationship entry names a processor.
 */
_Success_(return)
static BOOLEAN PhpHwMapRelationship(
    _Inout_ PPHP_HW_TOPOLOGY Topology,
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP Relationship,
    _Out_ PULONG Count
    )
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX information;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX entry;
    ULONG informationLength;
    ULONG count = 0;

    if (!NT_SUCCESS(PhGetSystemLogicalProcessorInformation(Relationship, &information, &informationLength)))
        return FALSE;

    for (
        entry = information;
        (ULONG_PTR)entry < (ULONG_PTR)PTR_ADD_OFFSET(information, informationLength);
        entry = PTR_ADD_OFFSET(entry, entry->Size)
        )
    {
        BOOLEAN named = FALSE;

        if (entry->Relationship != Relationship)
            continue;

        for (USHORT i = 0; i < entry->Processor.GroupCount; i++)
        {
            PGROUP_AFFINITY groupMask = &entry->Processor.GroupMask[i];

            for (ULONG number = 0; number < sizeof(KAFFINITY) * 8; number++)
            {
                ULONG index;

                if (!(groupMask->Mask & AFFINITY_MASK(number)))
                    continue;

                index = PhpHwFlatProcessorIndex(Topology, groupMask->Group, number);

                if (index == ULONG_MAX)
                    continue;

                if (Relationship == RelationProcessorCore)
                    Topology->Processors[index].CoreIndex = count;
                else
                    Topology->Processors[index].PackageIndex = count;

                named = TRUE;
            }
        }

        if (!named)
        {
            PhFree(information);
            return FALSE;
        }

        count++;
    }

    PhFree(information);

    if (count == 0)
        return FALSE;

    *Count = count;
    return TRUE;
}

/**
 * Lists the logical processors with their core and package.
 *
 * \param Topology Receives the processors; free it with PhpHwFreeTopology.
 * \return TRUE when every processor has a core and a package.
 */
_Success_(return)
BOOLEAN PhpHwQueryTopology(
    _Out_ PPHP_HW_TOPOLOGY Topology
    )
{
    PHP_HW_TOPOLOGY topology;
    ULONG index = 0;

    memset(&topology, 0, sizeof(PHP_HW_TOPOLOGY));
    topology.ProcessorCount = PhSystemProcessorInformation.NumberOfProcessors;
    topology.Processors = PhAllocateZero(sizeof(PHP_HW_PROCESSOR) * topology.ProcessorCount);

    for (USHORT group = 0; group < PhSystemProcessorInformation.NumberOfProcessorGroups; group++)
    {
        USHORT activeCount = PhGetActiveProcessorCount(group);

        for (USHORT number = 0; number < activeCount && index < topology.ProcessorCount; number++)
        {
            topology.Processors[index].Number.Group = group;
            topology.Processors[index].Number.Number = (UCHAR)number;
            topology.Processors[index].CoreIndex = ULONG_MAX;
            topology.Processors[index].PackageIndex = ULONG_MAX;
            index++;
        }
    }

    if (
        index != topology.ProcessorCount ||
        !PhpHwMapRelationship(&topology, RelationProcessorCore, &topology.CoreCount) ||
        !PhpHwMapRelationship(&topology, RelationProcessorPackage, &topology.PackageCount)
        )
    {
        PhpHwFreeTopology(&topology);
        return FALSE;
    }

    for (ULONG i = 0; i < topology.ProcessorCount; i++)
    {
        if (topology.Processors[i].CoreIndex == ULONG_MAX || topology.Processors[i].PackageIndex == ULONG_MAX)
        {
            PhpHwFreeTopology(&topology);
            return FALSE;
        }
    }

    *Topology = topology;
    return TRUE;
}

/**
 * Frees what PhpHwQueryTopology allocated.
 *
 * \param Topology The processors.
 */
VOID PhpHwFreeTopology(
    _Inout_ PPHP_HW_TOPOLOGY Topology
    )
{
    if (Topology->Processors)
        PhFree(Topology->Processors);

    memset(Topology, 0, sizeof(PHP_HW_TOPOLOGY));
}

/**
 * Marks the first processor of each AMD family 15h or 16h compute unit, keyed by CPUID 8000_001Eh NodeId (ECX[7:0])
 * and ComputeUnitId (EBX[7:0]) read on each processor, and gives the accumulator sample ratio. Needs
 * TopologyExtensions (8000_0001h ECX[22]) and the processor power accumulator (8000_0007h EDX[12]).
 *
 * \param Topology The processors.
 * \param FirstOfUnit Receives, per processor, whether it is the first of its compute unit.
 * \param SampleRatio Receives CPUID 8000_0007h ECX.
 * \return TRUE when every processor was read.
 */
_Success_(return)
BOOLEAN PhpHwQueryComputeUnits(
    _In_ PPHP_HW_TOPOLOGY Topology,
    _Out_writes_(Topology->ProcessorCount) PBOOLEAN FirstOfUnit,
    _Out_ PULONG SampleRatio
    )
{
#if defined(_M_IX86) || defined(_M_X64)
    INT cpuInfo[4];
    ULONG ratio;
    GROUP_AFFINITY original;
    PULONG keys;
    ULONG keyCount = 0;
    BOOLEAN failed = FALSE;

    __cpuid(cpuInfo, 0x80000000);
    if ((ULONG)cpuInfo[0] < 0x8000001E)
        return FALSE;
    __cpuid(cpuInfo, 0x80000001);
    if (!(cpuInfo[2] & (1 << 22)))
        return FALSE;
    __cpuid(cpuInfo, 0x80000007);
    if (!(cpuInfo[3] & (1 << 12)))
        return FALSE;

    ratio = (ULONG)cpuInfo[2];

    if (!ratio || !NT_SUCCESS(PhGetThreadGroupAffinity(NtCurrentThread(), &original)))
        return FALSE;

    memset(FirstOfUnit, 0, sizeof(BOOLEAN) * Topology->ProcessorCount);
    keys = PhAllocate(sizeof(ULONG) * Topology->ProcessorCount);

    for (ULONG i = 0; i < Topology->ProcessorCount; i++)
    {
        GROUP_AFFINITY affinity;
        ULONG key;
        BOOLEAN seen = FALSE;

        memset(&affinity, 0, sizeof(GROUP_AFFINITY));
        affinity.Group = Topology->Processors[i].Number.Group;
        affinity.Mask = (KAFFINITY)1 << Topology->Processors[i].Number.Number;

        if (!NT_SUCCESS(PhSetThreadGroupAffinity(NtCurrentThread(), affinity)))
        {
            failed = TRUE;
            break;
        }

        __cpuid(cpuInfo, 0x8000001E);
        key = (((ULONG)cpuInfo[2] & 0xFF) << 8) | ((ULONG)cpuInfo[1] & 0xFF);

        for (ULONG j = 0; j < keyCount; j++)
        {
            if (keys[j] == key)
            {
                seen = TRUE;
                break;
            }
        }

        if (!seen)
        {
            keys[keyCount++] = key;
            FirstOfUnit[i] = TRUE;
        }
    }

    PhSetThreadGroupAffinity(NtCurrentThread(), original);
    PhFree(keys);

    if (failed)
        return FALSE;

    *SampleRatio = ratio;
    return TRUE;
#else
    UNREFERENCED_PARAMETER(Topology);
    UNREFERENCED_PARAMETER(FirstOfUnit);
    UNREFERENCED_PARAMETER(SampleRatio);
    return FALSE;
#endif
}

/**
 * Retrieves a monotonic time.
 *
 * \return The time in 100 ns units.
 */
ULONG64 PhpHwQueryTime(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (PhBeginInitOnce(&initOnce))
    {
        PhQueryPerformanceFrequency(&frequency);
        PhEndInitOnce(&initOnce);
    }

    PhQueryPerformanceCounter(&counter);

    return ((ULONG64)counter.QuadPart / (ULONG64)frequency.QuadPart) * 10000000 +
        ((ULONG64)counter.QuadPart % (ULONG64)frequency.QuadPart) * 10000000 / (ULONG64)frequency.QuadPart;
}

/**
 * Parses the digit groups of a PCI location string as the bus driver writes it.
 *
 * \return The number of groups (3, or 4 with a segment), or 0 when malformed.
 */
_Success_(return != 0)
static ULONG PhpHwParseLocation(
    _In_reads_(Count) PCWSTR Location,
    _In_ ULONG Count,
    _Out_writes_(4) PULONG Numbers
    )
{
    ULONG numbers[4] = { 0 };
    ULONG numberCount = 0;
    BOOLEAN inNumber = FALSE;

    for (ULONG i = 0; i < Count && Location[i] != UNICODE_NULL; i++)
    {
        WCHAR c = Location[i];

        if (c < L'0' || c > L'9')
        {
            inNumber = FALSE;
            continue;
        }

        if (!inNumber)
        {
            if (numberCount == 4)
                return 0;

            inNumber = TRUE;
            numberCount++;
        }

        numbers[numberCount - 1] = numbers[numberCount - 1] * 10 + (c - L'0');

        if (numbers[numberCount - 1] > USHRT_MAX)
            return 0;
    }

    if (numberCount)
        memcpy(Numbers, numbers, sizeof(numbers));

    return numberCount;
}

_Success_(return)
static BOOLEAN PhpHwParseHardwareIdField(
    _In_ PCPH_STRINGREF HardwareId,
    _In_ PCWSTR Field,
    _Out_ PUSHORT Value
    )
{
    PH_STRINGREF part;
    ULONG64 value;
    SIZE_T index;

    if ((index = PhFindStringInStringRefZ(HardwareId, Field, TRUE)) == SIZE_MAX)
        return FALSE;
    if (HardwareId->Length < (index + 8) * sizeof(WCHAR))
        return FALSE;

    part.Buffer = &HardwareId->Buffer[index + 4];
    part.Length = 4 * sizeof(WCHAR);

    if (!PhStringToUInt64(&part, 16, &value))
        return FALSE;

    *Value = (USHORT)value;
    return TRUE;
}

static BOOLEAN PhpHwHasCompatibleClass(
    _In_reads_(Count) PCWSTR CompatibleIds,
    _In_ ULONG Count,
    _In_ PCPH_STRINGREF Class
    )
{
    for (ULONG i = 0; i < Count && CompatibleIds[i] != UNICODE_NULL; )
    {
        PH_STRINGREF id;

        id.Buffer = (PWSTR)&CompatibleIds[i];
        id.Length = PhCountStringZ(id.Buffer) * sizeof(WCHAR);

        if (PhStartsWithStringRef(&id, Class, TRUE))
            return TRUE;

        i += (ULONG)(id.Length / sizeof(WCHAR)) + 1;
    }

    return FALSE;
}

/**
 * Finds the present PCI functions of a vendor and class, sorted by location.
 *
 * \param VendorId The vendor ID to accept, or PHP_HW_ANY_VENDOR_ID for any.
 * \param AlternateVendorId A second vendor ID to accept.
 * \param ClassCode The base class and subclass, e.g. 0600h for a host bridge, or PHP_HW_ANY_CLASS_CODE for any.
 * \param DeviceIds The device IDs to accept, or NULL for any.
 * \param DeviceIdCount The number of device IDs.
 * \param FunctionZeroOnly TRUE to accept device 0, function 0 only.
 * \param Locators Receives the helper locators: (segment << 16) | (bus << 8) | (device << 3) | function.
 * \param Count The number of locators the buffer holds.
 * \return The number of locators written.
 */
ULONG PhpHwEnumeratePciFunctions(
    _In_ USHORT VendorId,
    _In_ USHORT AlternateVendorId,
    _In_ USHORT ClassCode,
    _In_reads_opt_(DeviceIdCount) PCUSHORT DeviceIds,
    _In_ ULONG DeviceIdCount,
    _In_ BOOLEAN FunctionZeroOnly,
    _Out_writes_to_(Count, return) PULONG Locators,
    _In_ ULONG Count
    )
{
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    WCHAR classBuffer[16];
    PH_STRINGREF classPrefix;
    ULONG found = 0;

    if (ClassCode == PHP_HW_ANY_CLASS_CODE)
        _snwprintf_s(classBuffer, RTL_NUMBER_OF(classBuffer), _TRUNCATE, L"PCI\\CC_");
    else
        _snwprintf_s(classBuffer, RTL_NUMBER_OF(classBuffer), _TRUNCATE, L"PCI\\CC_%04X", ClassCode);
    PhInitializeStringRefLongHint(&classPrefix, classBuffer);

    deviceInfoSet = SetupDiGetClassDevsW(NULL, L"PCI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return 0;

    memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (ULONG i = 0; found < Count && SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
    {
        WCHAR ids[512];
        WCHAR location[128];
        PH_STRINGREF hardwareId;
        DEVPROPTYPE propertyType;
        ULONG busNumber;
        ULONG address;
        ULONG numbers[4];
        ULONG numberCount;
        ULONG segment = 0;
        USHORT vendorId;
        ULONG locator;
        ULONG j;

        memset(ids, 0, sizeof(ids));

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_HardwareIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0))
            continue;

        PhInitializeStringRefLongHint(&hardwareId, ids);

        if (!PhpHwParseHardwareIdField(&hardwareId, L"VEN_", &vendorId))
            continue;
        if (VendorId != PHP_HW_ANY_VENDOR_ID && vendorId != VendorId && vendorId != AlternateVendorId)
            continue;

        if (DeviceIds)
        {
            USHORT deviceId;

            if (!PhpHwParseHardwareIdField(&hardwareId, L"DEV_", &deviceId))
                continue;

            for (j = 0; j < DeviceIdCount; j++)
            {
                if (DeviceIds[j] == deviceId)
                    break;
            }

            if (j == DeviceIdCount)
                continue;
        }

        memset(ids, 0, sizeof(ids));

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_CompatibleIds, &propertyType, (PBYTE)ids, sizeof(ids) - sizeof(WCHAR) * 2, NULL, 0))
            continue;
        if (!PhpHwHasCompatibleClass(ids, RTL_NUMBER_OF(ids), &classPrefix))
            continue;

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_BusNumber, &propertyType, (PBYTE)&busNumber, sizeof(ULONG), NULL, 0))
            continue;
        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_Address, &propertyType, (PBYTE)&address, sizeof(ULONG), NULL, 0))
            continue;
        if ((FunctionZeroOnly && address != 0) || HIWORD(address) > 0x1F || LOWORD(address) > 0x7)
            continue;

        memset(location, 0, sizeof(location));

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_LocationInfo, &propertyType, (PBYTE)location, sizeof(location) - sizeof(WCHAR), NULL, 0))
            continue;

        numberCount = PhpHwParseLocation(location, RTL_NUMBER_OF(location), numbers);

        if (numberCount == 4)
            segment = numbers[0];
        else if (numberCount != 3)
            continue;

        // Off segment 0 the bus number carries the segment above the bus.
        locator = (segment << 16) | ((ULONG)LOBYTE(busNumber) << 8) | ((ULONG)HIWORD(address) << 3) | LOWORD(address);

        for (j = found; j > 0; j--)
        {
            if (Locators[j - 1] <= locator)
                break;

            Locators[j] = Locators[j - 1];
        }

        Locators[j] = locator;
        found++;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return found;
}

/**
 * Normalizes an ACPI namespace path as the acpi script does: a backslash, then segments of 1 to 4 of A-Z, _ and (past
 * the first) 0-9 between dots, each without its trailing underscore padding.
 *
 * \param Text The path, with or without its leading backslash.
 * \param Path Receives the normalized path.
 * \param Length The number of characters Path holds.
 * \return TRUE when Text is such a path and it fits.
 */
_Success_(return)
static BOOLEAN PhpHwNormalizeAcpiPath(
    _In_ PCWSTR Text,
    _Out_writes_z_(Length) PSTR Path,
    _In_ ULONG Length
    )
{
    ULONG at = 0;

    if (*Text == L'\\')
        Text++;

    while (TRUE)
    {
        CHAR segment[4];
        ULONG count = 0;

        while (*Text && *Text != L'.')
        {
            WCHAR c = *Text++;

            if (count == RTL_NUMBER_OF(segment))
                return FALSE;
            if (!((c >= L'A' && c <= L'Z') || c == L'_' || (count && c >= L'0' && c <= L'9')))
                return FALSE;

            segment[count++] = (CHAR)c;
        }

        if (!count)
            return FALSE;

        while (count > 1 && segment[count - 1] == '_')
            count--;

        if (at + 1 + count >= Length)
            return FALSE;

        Path[at] = at ? '.' : '\\';
        at++;
        memcpy(Path + at, segment, count);
        at += count;

        if (!*Text)
            break;

        Text++;
    }

    Path[at] = ANSI_NULL;
    return TRUE;
}

/**
 * Finds the device tree's name of ACPI devices: the friendly name, else the description, of the device whose BIOS
 * device name is the namespace path.
 *
 * \param Paths The normalized namespace paths.
 * \param Count The number of paths.
 * \param Names Receives the name of each path's device, NULL when none was found.
 */
VOID PhpHwQueryAcpiDeviceNames(
    _In_reads_(Count) const PCSTR* Paths,
    _In_ ULONG Count,
    _Out_writes_(Count) PPH_STRING* Names
    )
{
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;

    memset(Names, 0, sizeof(PPH_STRING) * Count);

    deviceInfoSet = SetupDiGetClassDevsW(NULL, L"ACPI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE)
        return;

    memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (ULONG i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++)
    {
        DEVPROPTYPE propertyType;
        WCHAR text[256];
        CHAR path[256];

        if (!SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_BiosDeviceName, &propertyType, (PBYTE)text, sizeof(text) - sizeof(WCHAR), NULL, 0))
            continue;
        if (propertyType != DEVPROP_TYPE_STRING)
            continue;

        text[RTL_NUMBER_OF(text) - 1] = UNICODE_NULL;

        if (!PhpHwNormalizeAcpiPath(text, path, RTL_NUMBER_OF(path)))
            continue;

        for (ULONG j = 0; j < Count; j++)
        {
            WCHAR name[256];

            if (Names[j] || strcmp(Paths[j], path) != 0)
                continue;

            if (
                (SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_FriendlyName, &propertyType, (PBYTE)name, sizeof(name) - sizeof(WCHAR), NULL, 0) && propertyType == DEVPROP_TYPE_STRING) ||
                (SetupDiGetDevicePropertyW(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_DeviceDesc, &propertyType, (PBYTE)name, sizeof(name) - sizeof(WCHAR), NULL, 0) && propertyType == DEVPROP_TYPE_STRING)
                )
            {
                name[RTL_NUMBER_OF(name) - 1] = UNICODE_NULL;

                if (name[0])
                    Names[j] = PhCreateString(name);
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
}

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
static BOOLEAN NTAPI PhpHwSmbiosMemoryCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_MEMORY_DEVICES devices = Context;
    PPH_STRING name;
    ULONG speed = 0;

    if (!devices)
        return TRUE;
    if (Entry->Header.Type != SMBIOS_MEMORY_DEVICE_INFORMATION_TYPE || !Entry->MemoryDevice.Size.Value)
        return FALSE;

    if (
        devices->Count < PHP_HW_MEMORY_DEVICE_MAX &&
        PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, DeviceLocator) &&
        NT_SUCCESS(PhGetSMBIOSString(EnumHandle, Entry->MemoryDevice.DeviceLocator, &name))
        )
    {
        devices->Locators[devices->Count] = name;

        if (
            PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, BankLocator) &&
            NT_SUCCESS(PhGetSMBIOSString(EnumHandle, Entry->MemoryDevice.BankLocator, &name))
            )
        {
            devices->Banks[devices->Count] = name;
        }
    }

    devices->Count++;

    // Before 2.8 the configured speed may be a clock in MHz rather than MT/s.
    if (MajorVersion < 2 || (MajorVersion == 2 && MinorVersion < 8))
        return FALSE;

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ConfiguredSpeed))
        speed = Entry->MemoryDevice.ConfiguredSpeed;
    if (speed == USHRT_MAX && PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ExtendedConfiguredSpeed))
        speed = Entry->MemoryDevice.ExtendedConfiguredSpeed;
    if (speed != USHRT_MAX && speed > devices->ConfiguredSpeed)
        devices->ConfiguredSpeed = speed;

    return FALSE;
}

/**
 * Reads the populated memory devices of SMBIOS type 17: their locators, in table order, and the highest configured
 * speed.
 *
 * \param Devices Receives the devices; free them with PhpHwFreeMemoryDevices.
 */
VOID PhpHwQueryMemoryDevices(
    _Out_ PPHP_HW_MEMORY_DEVICES Devices
    )
{
    memset(Devices, 0, sizeof(PHP_HW_MEMORY_DEVICES));
    PhEnumSMBIOS(PhpHwSmbiosMemoryCallback, Devices);
}

/**
 * Frees what PhpHwQueryMemoryDevices allocated.
 *
 * \param Devices The devices.
 */
VOID PhpHwFreeMemoryDevices(
    _Inout_ PPHP_HW_MEMORY_DEVICES Devices
    )
{
    for (ULONG i = 0; i < PHP_HW_MEMORY_DEVICE_MAX; i++)
    {
        PhClearReference(&Devices->Locators[i]);
        PhClearReference(&Devices->Banks[i]);
    }

    memset(Devices, 0, sizeof(PHP_HW_MEMORY_DEVICES));
}

typedef struct _PHP_HW_SYSTEM
{
    PPH_STRING Manufacturer;
    PPH_STRING Product;
    PPH_STRING Family;
} PHP_HW_SYSTEM, *PPHP_HW_SYSTEM;

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
static BOOLEAN NTAPI PhpHwSmbiosSystemCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_SYSTEM system = Context;

    UNREFERENCED_PARAMETER(MajorVersion);
    UNREFERENCED_PARAMETER(MinorVersion);

    if (!system)
        return TRUE;
    if (Entry->Header.Type != SMBIOS_SYSTEM_INFORMATION_TYPE)
        return FALSE;

    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, Manufacturer))
        PhGetSMBIOSString(EnumHandle, Entry->System.Manufacturer, &system->Manufacturer);
    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, ProductName))
        PhGetSMBIOSString(EnumHandle, Entry->System.ProductName, &system->Product);
    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, Family))
        PhGetSMBIOSString(EnumHandle, Entry->System.Family, &system->Family);

    return TRUE;
}

/**
 * Reads the system's manufacturer, product name and family from SMBIOS type 1.
 *
 * \param Manufacturer Receives the manufacturer.
 * \param Product Receives the product name; NULL when the table has none.
 * \param Family Receives the family (SMBIOS 2.4); NULL when the table has none.
 * \return TRUE when the manufacturer was read.
 */
_Success_(return)
BOOLEAN PhpHwQuerySystem(
    _Out_ PPH_STRING* Manufacturer,
    _Out_ PPH_STRING* Product,
    _Out_ PPH_STRING* Family
    )
{
    PHP_HW_SYSTEM system;

    memset(&system, 0, sizeof(PHP_HW_SYSTEM));
    PhEnumSMBIOS(PhpHwSmbiosSystemCallback, &system);

    if (!system.Manufacturer)
    {
        PhClearReference(&system.Product);
        PhClearReference(&system.Family);
        return FALSE;
    }

    *Manufacturer = system.Manufacturer;
    *Product = system.Product;
    *Family = system.Family;
    return TRUE;
}

typedef struct _PHP_HW_BASEBOARD
{
    PPH_STRING Manufacturer;
    PPH_STRING Product;
} PHP_HW_BASEBOARD, *PPHP_HW_BASEBOARD;

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
static BOOLEAN NTAPI PhpHwSmbiosBaseboardCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_BASEBOARD baseboard = Context;

    UNREFERENCED_PARAMETER(MajorVersion);
    UNREFERENCED_PARAMETER(MinorVersion);

    if (!baseboard)
        return TRUE;
    if (Entry->Header.Type != SMBIOS_BASEBOARD_INFORMATION_TYPE)
        return FALSE;

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Manufacturer))
        PhGetSMBIOSString(EnumHandle, Entry->Baseboard.Manufacturer, &baseboard->Manufacturer);
    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Product))
        PhGetSMBIOSString(EnumHandle, Entry->Baseboard.Product, &baseboard->Product);

    return TRUE;
}

/**
 * Reads the baseboard's manufacturer and product from SMBIOS type 2.
 *
 * \param Manufacturer Receives the manufacturer.
 * \param Product Receives the product.
 * \return TRUE when both were read.
 */
_Success_(return)
BOOLEAN PhpHwQueryBaseboard(
    _Out_ PPH_STRING* Manufacturer,
    _Out_ PPH_STRING* Product
    )
{
    PHP_HW_BASEBOARD baseboard;

    memset(&baseboard, 0, sizeof(PHP_HW_BASEBOARD));
    PhEnumSMBIOS(PhpHwSmbiosBaseboardCallback, &baseboard);

    if (!baseboard.Manufacturer || !baseboard.Product)
    {
        PhClearReference(&baseboard.Manufacturer);
        PhClearReference(&baseboard.Product);
        return FALSE;
    }

    *Manufacturer = baseboard.Manufacturer;
    *Product = baseboard.Product;
    return TRUE;
}

/**
 * Reads the counter frequency of the first usable LPIT residency counter, by the kernel's ordinal rule: native
 * entries that are enabled and have a SYSTEM_MEMORY counter at bit offset 0 of 32 or 64 bits.
 *
 * \param Frequency Receives the counter frequency in Hz; 0 is the TSC rate.
 * \return TRUE when the table has such an entry.
 */
_Success_(return)
BOOLEAN PhpHwQueryLpitFrequency(
    _Out_ PULONG64 Frequency
    )
{
    NTSTATUS status;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;
    ULONG length = sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION) + 0x1000;
    BOOLEAN found = FALSE;

    info = PhAllocateZero(length);
    info->ProviderSignature = 'ACPI';
    info->TableID = 'TIPL';
    info->Action = SystemFirmwareTableGet;
    info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

    status = NtQuerySystemInformation(SystemFirmwareTableInformation, info, length, &length);

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(info);
        info = PhAllocateZero(length);
        info->ProviderSignature = 'ACPI';
        info->TableID = 'TIPL';
        info->Action = SystemFirmwareTableGet;
        info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

        status = NtQuerySystemInformation(SystemFirmwareTableInformation, info, length, &length);
    }

    if (NT_SUCCESS(status) && info->TableBufferLength >= PHP_HW_LPIT_HEADER)
    {
        const UCHAR* table = info->TableBuffer;
        ULONG tableLength;
        ULONG offset = PHP_HW_LPIT_HEADER;

        memcpy(&tableLength, table + 4, sizeof(ULONG));
        tableLength = min(tableLength, info->TableBufferLength);

        while (offset + 8 <= tableLength)
        {
            ULONG type;
            ULONG entryLength;
            ULONG flags;

            memcpy(&type, table + offset, sizeof(ULONG));
            memcpy(&entryLength, table + offset + 4, sizeof(ULONG));

            if (entryLength < 8 || entryLength > tableLength - offset)
                break;

            if (type == 0 && entryLength >= PHP_HW_LPIT_NATIVE_LENGTH)
            {
                memcpy(&flags, table + offset + 12, sizeof(ULONG));

                // Not disabled (bit 0), has a counter (bit 1); the counter GAS at +36: space, bit width, bit offset.
                if (
                    !(flags & 0x3) &&
                    table[offset + 36] == 0 &&
                    (table[offset + 37] == 32 || table[offset + 37] == 64) &&
                    table[offset + 38] == 0
                    )
                {
                    memcpy(Frequency, table + offset + 48, sizeof(ULONG64));
                    found = TRUE;
                    break;
                }
            }

            offset += entryLength;
        }
    }

    PhFree(info);

    return found;
}

/**
 * Reads the processor's brand string, else (ARM64 has none) the processor name Windows keeps for processor 0.
 *
 * \return The name, trimmed, or NULL when the system has none.
 */
_Ret_maybenull_
PPH_STRING PhpHwQueryProcessorName(
    VOID
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" ");
    static CONST PH_STRINGREF processorKeyName = PH_STRINGREF_INIT(L"Hardware\\Description\\System\\CentralProcessor\\0");
    SYSTEM_PROCESSOR_BRAND_STRING brandString;
    PPH_STRING brand = NULL;
    PH_STRINGREF trimmed;
    HANDLE keyHandle;

    if (NT_SUCCESS(NtQuerySystemInformation(SystemProcessorBrandString, &brandString, sizeof(brandString), NULL)))
        brand = PhConvertUtf8ToUtf16Ex(brandString.BrandString, strnlen(brandString.BrandString, sizeof(brandString.BrandString)));

    if (PhIsNullOrEmptyString(brand) && NT_SUCCESS(PhOpenKey(&keyHandle, KEY_READ, PH_KEY_LOCAL_MACHINE, &processorKeyName, 0)))
    {
        PhMoveReference(&brand, PhQueryRegistryStringZ(keyHandle, L"ProcessorNameString"));
        NtClose(keyHandle);
    }

    if (!brand)
        return NULL;

    trimmed = brand->sr;
    PhTrimStringRef(&trimmed, &whitespace, 0);

    if (trimmed.Length == 0)
    {
        PhDereferenceObject(brand);
        return NULL;
    }

    if (trimmed.Length != brand->Length)
        PhMoveReference(&brand, PhCreateString2(&trimmed));

    return brand;
}
