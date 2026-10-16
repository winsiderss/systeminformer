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

#include <phapp.h>
#include <phsettings.h>
#include <phfirmware.h>
#include <kphuser.h>
#include <devprv.h>
#include <hwprv.h>
#include <sysinfo.h>
#include <sysinfop.h>

// The hardware report: what the provider found and why, the sensors, the devices and SMBIOS.

/**
 * Determines why the hardware sensors window has nothing to show.
 *
 * \return The reason, or NULL when sensors are available.
 */
_Ret_maybenull_
PCPH_STRINGREF PhHwGetSensorsUnavailableReason(
    VOID
    )
{
    static CONST PH_STRINGREF disabled = PH_STRINGREF_INIT(L"Hardware sensors are disabled. Enable them in Options > General.");
    static CONST PH_STRINGREF driver = PH_STRINGREF_INIT(L"Hardware sensors require the kernel driver with full access.");
    static CONST PH_STRINGREF noData = PH_STRINGREF_INIT(L"The kernel driver has no hardware data for this system.");
    static CONST PH_STRINGREF none = PH_STRINGREF_INIT(L"No supported hardware sensors were found on this system.");
    KPH_DYN_ACTIVE_FLAGS flags;

    if (PhHwSensorsAvailable() || PhHwSourceSensorsPresent())
        return NULL;
    if (!PhCsEnableHardwareSensors)
        return &disabled;
    if (KsiLevel() != KphLevelMax)
        return &driver;
    if (NT_SUCCESS(KphQueryActiveDynData(&flags)) && !(flags & KPH_DYN_ACTIVE_HARDWARE))
        return &noData;

    return &none;
}

static VOID PhpHwReportSection(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ PCWSTR Title
    )
{
    PhAppendFormatStringBuilder(Builder, L"\n[%s]\n", Title);
}

static VOID PhpHwReportArmIdLine(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ ULONG First,
    _In_ ULONG Last,
    _In_reads_(3) CONST ULONG64* Values
    )
{
    ULONG64 midr = Values[0];
    ULONG64 pfr0 = Values[1];
    ULONG64 dfr0 = Values[2];

    if (First == Last)
        PhAppendFormatStringBuilder(Builder, L"CPU %lu", First);
    else
        PhAppendFormatStringBuilder(Builder, L"CPU %lu-%lu", First, Last);

    PhAppendFormatStringBuilder(
        Builder,
        L": MIDR 0x%08I64x (implementer 0x%02I64x, part 0x%03I64x, variant %I64u, revision %I64u), "
        L"PFR0 0x%016I64x (AMU %I64u, EL3 %I64u), DFR0 0x%016I64x (PMUVer 0x%I64x)\n",
        midr,
        (midr >> 24) & 0xFF,
        (midr >> 4) & 0xFFF,
        (midr >> 20) & 0xF,
        midr & 0xF,
        pfr0,
        (pfr0 >> 44) & 0xF,
        (pfr0 >> 12) & 0xF,
        dfr0,
        (dfr0 >> 8) & 0xF
        );
}

/**
 * Decodes the ARM64 ID registers Windows records per logical processor (CP 4000 MIDR_EL1, CP 4020 ID_AA64PFR0_EL1,
 * CP 4028 ID_AA64DFR0_EL1), without the driver; identical processors are listed once with their range.
 */
static VOID PhpHwReportArmIdRegisters(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PH_STRINGREF processorsKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor");
    static CONST PH_STRINGREF valueNames[3] =
    {
        PH_STRINGREF_INIT(L"CP 4000"), PH_STRINGREF_INIT(L"CP 4020"), PH_STRINGREF_INIT(L"CP 4028")
    };
    ULONG64 last[3] = { 0 };
    ULONG first = ULONG_MAX;
    ULONG processor;

    for (processor = 0; ; processor++)
    {
        WCHAR keyNameBuffer[128];
        PH_FORMAT format[3];
        PH_STRINGREF keyName;
        HANDLE keyHandle;
        ULONG64 values[3] = { 0 };
        BOOLEAN found = FALSE;

        PhInitFormatSR(&format[0], processorsKeyName);
        PhInitFormatC(&format[1], OBJ_NAME_PATH_SEPARATOR);
        PhInitFormatU(&format[2], processor);

        if (!PhFormatToBuffer(format, RTL_NUMBER_OF(format), keyNameBuffer, sizeof(keyNameBuffer), NULL))
            break;

        PhInitializeStringRefLongHint(&keyName, keyNameBuffer);

        if (!NT_SUCCESS(PhOpenKey(&keyHandle, KEY_READ, NULL, &keyName, 0)))
            break;

        for (ULONG i = 0; i < RTL_NUMBER_OF(valueNames); i++)
        {
            PKEY_VALUE_PARTIAL_INFORMATION value;

            if (NT_SUCCESS(PhQueryValueKey(keyHandle, &valueNames[i], KeyValuePartialInformation, &value)))
            {
                if (value->Type == REG_QWORD && value->DataLength == sizeof(ULONG64))
                {
                    memcpy(&values[i], value->Data, sizeof(ULONG64));
                    found |= i == 0; // MIDR_EL1 decides
                }

                PhFree(value);
            }
        }

        NtClose(keyHandle);

        if (!found)
            break;

        if (first != ULONG_MAX && RtlEqualMemory(values, last, sizeof(values)))
            continue;

        if (first != ULONG_MAX)
            PhpHwReportArmIdLine(Builder, first, processor - 1, last);

        first = processor;
        memcpy(last, values, sizeof(values));
    }

    if (first != ULONG_MAX)
        PhpHwReportArmIdLine(Builder, first, processor - 1, last);
}

static VOID PhpHwReportProcessor(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PH_STRINGREF processorKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
    PPH_STRING brand;
    HANDLE keyHandle;

    if (brand = PhSipGetCpuBrandString())
    {
        PhAppendFormatStringBuilder(Builder, L"Brand: %s\n", brand->Buffer);
        PhDereferenceObject(brand);
    }

    if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_READ, NULL, &processorKeyName, 0)))
    {
        PKEY_VALUE_PARTIAL_INFORMATION value;

        if (NT_SUCCESS(PhQueryValueKey(keyHandle, &(PH_STRINGREF)PH_STRINGREF_INIT(L"Update Revision"), KeyValuePartialInformation, &value)))
        {
            PhAppendStringBuilder2(Builder, L"Microcode (Update Revision):");

            for (ULONG i = 0; i < value->DataLength; i++)
                PhAppendFormatStringBuilder(Builder, L" %02x", value->Data[i]);

            PhAppendCharStringBuilder(Builder, L'\n');
            PhFree(value);
        }

        NtClose(keyHandle);
    }

    PhpHwReportArmIdRegisters(Builder);
}

typedef struct _PHP_HW_REPORT_DEVICE
{
    PPH_STRING InstanceId;
    PPH_STRING Name;
    PPH_STRING HardwareIds;
    ULONG ProblemCode;
} PHP_HW_REPORT_DEVICE, *PPHP_HW_REPORT_DEVICE;

static int __cdecl PhpHwReportDeviceCompare(
    _In_ const void* Elem1,
    _In_ const void* Elem2
    )
{
    const PHP_HW_REPORT_DEVICE* device1 = Elem1;
    const PHP_HW_REPORT_DEVICE* device2 = Elem2;

    return PhCompareString(device1->InstanceId, device2->InstanceId, TRUE);
}

// Devices by instance ID; devices with the same name and hardware IDs (one per logical processor,
// say) are listed once with a count.
static VOID PhpHwReportDevices(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PPH_DEVICE_TREE tree;
    PPHP_HW_REPORT_DEVICE devices;
    ULONG count = 0;

    if (!(tree = PhReferenceDeviceTree()))
        return;

    devices = PhAllocate(sizeof(PHP_HW_REPORT_DEVICE) * max(tree->DeviceList->Count, 1));

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        PPH_DEVICE_ITEM item = tree->DeviceList->Items[i];

        if (!item->InstanceId)
            continue;

        devices[count].InstanceId = PhReferenceObject(item->InstanceId);
        devices[count].Name = PhGetDeviceProperty(item, PhDevicePropertyName)->AsString;
        devices[count].HardwareIds = PhGetDeviceProperty(item, PhDevicePropertyHardwareIds)->AsString;
        devices[count].ProblemCode = item->ProblemCode;
        PhSetReference(&devices[count].Name, devices[count].Name);
        PhSetReference(&devices[count].HardwareIds, devices[count].HardwareIds);
        count++;
    }

    qsort(devices, count, sizeof(PHP_HW_REPORT_DEVICE), PhpHwReportDeviceCompare);

    for (ULONG i = 0; i < count; i++)
    {
        PPHP_HW_REPORT_DEVICE device = &devices[i];
        ULONG duplicates = 0;

        if (!device->InstanceId)
            continue;

        for (ULONG j = i + 1; j < count; j++)
        {
            if (
                devices[j].InstanceId &&
                devices[j].ProblemCode == device->ProblemCode &&
                PhEqualStringZ(PhGetStringOrEmpty(devices[j].Name), PhGetStringOrEmpty(device->Name), FALSE) &&
                PhEqualStringZ(PhGetStringOrEmpty(devices[j].HardwareIds), PhGetStringOrEmpty(device->HardwareIds), FALSE)
                )
            {
                PhClearReference(&devices[j].InstanceId);
                duplicates++;
            }
        }

        PhAppendFormatStringBuilder(Builder, L"%s\n", device->InstanceId->Buffer);

        if (duplicates)
            PhAppendFormatStringBuilder(Builder, L"  Devices: %lu like this\n", duplicates + 1);

        PhAppendFormatStringBuilder(
            Builder,
            L"  Name: %s\n  Hardware IDs: %s\n",
            PhGetStringOrEmpty(device->Name),
            PhGetStringOrEmpty(device->HardwareIds)
            );

        if (device->ProblemCode)
            PhAppendFormatStringBuilder(Builder, L"  Problem: %lu\n", device->ProblemCode);
    }

    for (ULONG i = 0; i < count; i++)
    {
        PhClearReference(&devices[i].InstanceId);
        PhClearReference(&devices[i].Name);
        PhClearReference(&devices[i].HardwareIds);
    }

    PhFree(devices);
    PhDereferenceObject(tree);
}

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
static BOOLEAN NTAPI PhpHwReportSmbiosCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING_BUILDER builder = Context;
    PUCHAR bytes = (PUCHAR)Entry;

    if (!builder)
        return TRUE;

    PhAppendFormatStringBuilder(
        builder,
        L"Type %u, handle 0x%04x, length %u:",
        Entry->Header.Type,
        Entry->Header.Handle,
        Entry->Header.Length
        );

    for (ULONG i = 0; i < Entry->Header.Length; i++)
    {
        if (i % 16 == 0)
            PhAppendStringBuilder2(builder, L"\n ");

        PhAppendFormatStringBuilder(builder, L" %02x", bytes[i]);
    }

    PhAppendCharStringBuilder(builder, L'\n');

    for (UCHAR index = 1; index != 0; index++)
    {
        PPH_STRING string;

        if (!NT_SUCCESS(PhGetSMBIOSString(EnumHandle, index, &string)))
            break;

        if (string->Length)
            PhAppendFormatStringBuilder(builder, L"  %u: %s\n", index, string->Buffer);

        PhDereferenceObject(string);
    }

    return FALSE;
}

/**
 * Creates the hardware report.
 *
 * \param Sensors The sensors window tree as text, or NULL when it shows none.
 * \return The report.
 */
PPH_STRING PhHwCreateReport(
    _In_opt_ PCPH_STRINGREF Sensors
    )
{
    PH_STRING_BUILDER builder;
    PPH_STRING version;
    static CONST PH_STRINGREF available = PH_STRINGREF_INIT(L"Hardware sensors are available.");
    PCPH_STRINGREF reason;

    PhInitializeStringBuilder(&builder, 0x4000);

    version = PhGetApplicationVersionString(FALSE);
    PhAppendFormatStringBuilder(
        &builder,
        L"System Informer hardware report\n%s\nWindows %lu.%lu.%lu\nCreated %s\n",
        version->Buffer,
        PhOsVersion.MajorVersion,
        PhOsVersion.MinorVersion,
        PhOsVersion.BuildNumber,
        PhaFormatDateTime(NULL)->Buffer
        );
    PhDereferenceObject(version);

    PhpHwReportSection(&builder, L"Status");
    reason = PhHwGetSensorsUnavailableReason();
    PhAppendStringBuilder(&builder, reason ? reason : &available);
    PhAppendCharStringBuilder(&builder, L'\n');

    PhpHwReportSection(&builder, L"Provider");
    PhHwWriteDiagnostics(&builder);

    PhpHwReportSection(&builder, L"Sources");
    PhHwWriteSourceDiagnostics(&builder);

    PhpHwReportSection(&builder, L"Processor");
    PhpHwReportProcessor(&builder);

    PhpHwReportSection(&builder, L"Sensors");

    if (Sensors && Sensors->Length)
        PhAppendStringBuilder(&builder, Sensors);
    else
        PhAppendStringBuilder2(&builder, L"None\n");

    PhpHwReportSection(&builder, L"Devices");
    PhpHwReportDevices(&builder);

    PhpHwReportSection(&builder, L"SMBIOS");
    PhEnumSMBIOS(PhpHwReportSmbiosCallback, &builder);

    return PhFinalStringBuilderString(&builder);
}
