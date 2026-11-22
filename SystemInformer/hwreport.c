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
#include <devprv.h>
#include <phfirmware.h>

// The hardware report: what the provider found and why, the sensors, the devices and SMBIOS.

/**
 * Determines why the hardware sensors window has nothing to show.
 *
 * \return The reason, or NULL when sensors are available.
 */
_Ret_maybenull_
PCPH_STRINGREF NTAPI PhHwGetSensorsUnavailableReason(
    VOID
    )
{
    static CONST PH_STRINGREF disabled = PH_STRINGREF_INIT(L"Hardware sensors are disabled. Enable them in Options > General.");
    static CONST PH_STRINGREF driver = PH_STRINGREF_INIT(L"Hardware sensors require the kernel driver with full access.");
    static CONST PH_STRINGREF notShipped = PH_STRINGREF_INIT(L"No hardware sensor script is shipped with this build.");
    static CONST PH_STRINGREF none = PH_STRINGREF_INIT(L"No supported hardware sensors were found on this system.");

    if (PhHwSensorsAvailable() || PhHwSourceSensorsPresent())
        return NULL;
    if (!PhCsEnableHardwareSensors)
        return &disabled;
    if (!PhpHwDriverReady())
        return &driver;
    if (PhpHwFamiliesNotShipped())
        return &notShipped;

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

/**
 * Hashes a file with SHA-256.
 *
 * \param FileName The Win32 file name.
 * \param Size Receives the file size.
 * \param Hash Receives the hash as hexadecimal.
 * \param FirstValue Receives the file's first ULONG, or 0 when it is shorter.
 * \return Successful or errant status.
 */
static NTSTATUS PhpHwReportHashFile(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PLARGE_INTEGER Size,
    _Out_ PPH_STRING* Hash,
    _Out_opt_ PULONG FirstValue
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    PH_HASH_CONTEXT hashContext;
    PVOID buffer;
    ULONG bytesRead;
    BOOLEAN first = TRUE;

    Size->QuadPart = 0;
    *Hash = NULL;

    if (FirstValue)
        *FirstValue = 0;

    status = PhCreateFileWin32(
        &fileHandle,
        FileName->Buffer,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, Size)) ||
        !NT_SUCCESS(status = PhInitializeHash(&hashContext, Sha256HashAlgorithm)))
    {
        NtClose(fileHandle);
        return status;
    }

    buffer = PhAllocate(PAGE_SIZE * 16);

    while (NT_SUCCESS(status = PhReadFile(fileHandle, buffer, PAGE_SIZE * 16, NULL, &bytesRead)) && bytesRead)
    {
        if (first && FirstValue && bytesRead >= sizeof(ULONG))
            *FirstValue = *(PULONG)buffer;

        first = FALSE;

        if (!NT_SUCCESS(status = PhUpdateHash(&hashContext, buffer, bytesRead)))
            break;
    }

    if (status == STATUS_END_OF_FILE)
        status = STATUS_SUCCESS;

    if (NT_SUCCESS(status))
        status = PhFinalHashString(&hashContext, Hash);
    else
        PhFinalHash(&hashContext, buffer, PH_HASH_SHA256_LENGTH, NULL);

    PhFree(buffer);
    NtClose(fileHandle);

    return status;
}

/**
 * Appends a line for a file the hardware sensors depend on: its size and hash.
 *
 * \param Builder The report.
 * \param Label The line's label.
 * \param FileName The Win32 file name.
 * \param Image TRUE to add the image's file version.
 * \param DynData TRUE to add the dynamic data's version.
 */
static VOID PhpHwReportFileLine(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_ PCWSTR Label,
    _In_ PCPH_STRINGREF FileName,
    _In_ BOOLEAN Image,
    _In_ BOOLEAN DynData
    )
{
    NTSTATUS status;
    LARGE_INTEGER size;
    PPH_STRING hash;
    ULONG firstValue;

    PhAppendFormatStringBuilder(Builder, L"%s: ", Label);

    if (!NT_SUCCESS(status = PhpHwReportHashFile(FileName, &size, &hash, &firstValue)))
    {
        PhAppendFormatStringBuilder(Builder, L"status 0x%08lx\n", status);
        return;
    }

    if (Image)
    {
        PH_IMAGE_VERSION_INFO versionInfo;

        if (NT_SUCCESS(PhInitializeImageVersionInfoEx(&versionInfo, FileName, FALSE)))
        {
            PhAppendFormatStringBuilder(Builder, L"version %s, ", PhGetStringOrDefault(versionInfo.FileVersion, L"none"));
            PhDeleteImageVersionInfo(&versionInfo);
        }
    }

    if (DynData)
        PhAppendFormatStringBuilder(Builder, L"version %lu, ", firstValue);

    PhAppendFormatStringBuilder(Builder, L"%I64u bytes, SHA-256 %s\n", size.QuadPart, hash->Buffer);
    PhDereferenceObject(hash);
}

typedef struct _PHP_HW_REPORT_SCRIPTS_CONTEXT
{
    PPH_STRING Directory;
    PPH_LIST Names;
} PHP_HW_REPORT_SCRIPTS_CONTEXT, *PPHP_HW_REPORT_SCRIPTS_CONTEXT;

_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN NTAPI PhpHwReportScriptsCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_REPORT_SCRIPTS_CONTEXT context = Context;
    PFILE_DIRECTORY_INFORMATION information = Information;
    PH_STRINGREF name;

    if (!context || (information->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        return TRUE;

    name.Buffer = information->FileName;
    name.Length = information->FileNameLength;
    PhAddItemList(context->Names, PhCreateString2(&name));

    return TRUE;
}

/**
 * Appends the files the hardware sensors depend on: the driver, the dynamic data and the scripts.
 *
 * \param Builder The report.
 */
static VOID PhpHwReportSoftware(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PH_STRINGREF driverExtension = PH_STRINGREF_INIT(L".sys");
    static CONST PH_STRINGREF dynDataName = PH_STRINGREF_INIT(L"ksidyn.bin");
    static CONST PH_STRINGREF scriptsName = PH_STRINGREF_INIT(L"scripts\\");
    static CONST PH_STRINGREF scriptPattern = PH_STRINGREF_INIT(L"*.bin");
    PHP_HW_REPORT_SCRIPTS_CONTEXT context;
    PPH_STRING applicationFileName;
    PPH_STRING directory;
    PPH_STRING fileName;
    HANDLE directoryHandle;

    applicationFileName = PhGetApplicationFileNameWin32();
    fileName = PhGetBaseNameChangeExtension(&applicationFileName->sr, &driverExtension);
    PhpHwReportFileLine(Builder, L"Driver file", &fileName->sr, TRUE, FALSE);
    PhDereferenceObject(fileName);
    PhDereferenceObject(applicationFileName);

    directory = PhGetApplicationDirectoryWin32();
    fileName = PhConcatStringRef2(&directory->sr, &dynDataName);
    PhpHwReportFileLine(Builder, L"Dynamic data", &fileName->sr, FALSE, TRUE);
    PhDereferenceObject(fileName);

    context.Directory = PhConcatStringRef2(&directory->sr, &scriptsName);
    context.Names = PhCreateList(16);

    if (NT_SUCCESS(PhCreateFileWin32(
        &directoryHandle,
        context.Directory->Buffer,
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        PhEnumDirectoryFile(directoryHandle, &scriptPattern, PhpHwReportScriptsCallback, &context);
        NtClose(directoryHandle);
    }

    if (context.Names->Count == 0)
        PhAppendStringBuilder2(Builder, L"Scripts: none\n");

    for (ULONG i = 0; i < context.Names->Count; i++)
    {
        PPH_STRING name = context.Names->Items[i];
        PPH_STRING label = PhConcatStrings2(L"Script ", name->Buffer);

        fileName = PhConcatStringRef2(&context.Directory->sr, &name->sr);
        PhpHwReportFileLine(Builder, label->Buffer, &fileName->sr, FALSE, FALSE);
        PhDereferenceObject(fileName);
        PhDereferenceObject(label);
        PhDereferenceObject(name);
    }

    PhDereferenceObject(context.Names);
    PhDereferenceObject(context.Directory);
    PhDereferenceObject(directory);
}

/**
 * Appends what decides which registers Windows lets the driver reach: the hypervisor, code integrity
 * and Secure Boot.
 *
 * \param Builder The report.
 */
static VOID PhpHwReportPlatform(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    SYSTEM_CODEINTEGRITY_INFORMATION codeIntegrity;
    SYSTEM_SECUREBOOT_INFORMATION secureBoot;

#if defined(_M_IX86) || defined(_M_AMD64)
    INT cpuInfo[4];

    __cpuid(cpuInfo, 1);

    if (cpuInfo[2] & (1 << 31))
    {
        CHAR vendor[13];

        __cpuid(cpuInfo, 0x40000000);
        memcpy(vendor + 0, &cpuInfo[1], 4);
        memcpy(vendor + 4, &cpuInfo[2], 4);
        memcpy(vendor + 8, &cpuInfo[3], 4);
        vendor[12] = ANSI_NULL;

        PhAppendFormatStringBuilder(Builder, L"Hypervisor: %hs", vendor);

        // Hyper-V's partition privileges: CreatePartitions is held by the root partition only.
        if (strcmp(vendor, "Microsoft Hv") == 0 && (ULONG)cpuInfo[0] >= 0x40000003)
        {
            __cpuid(cpuInfo, 0x40000003);
            PhAppendFormatStringBuilder(Builder, L", root partition %s", (cpuInfo[1] & 1) ? L"yes" : L"no");
        }

        PhAppendCharStringBuilder(Builder, L'\n');
    }
    else
    {
        PhAppendStringBuilder2(Builder, L"Hypervisor: none\n");
    }
#else
    PhAppendStringBuilder2(Builder, L"Hypervisor: not queried on this architecture\n");
#endif

    codeIntegrity.Length = sizeof(SYSTEM_CODEINTEGRITY_INFORMATION);

    if (NT_SUCCESS(NtQuerySystemInformation(SystemCodeIntegrityInformation, &codeIntegrity, sizeof(codeIntegrity), NULL)))
    {
        PhAppendFormatStringBuilder(
            Builder,
            L"Code integrity: 0x%08lx (test signing %s, debug mode %s, HVCI %s)\n",
            codeIntegrity.CodeIntegrityOptions,
            (codeIntegrity.CodeIntegrityOptions & CODEINTEGRITY_OPTION_TESTSIGN) ? L"on" : L"off",
            (codeIntegrity.CodeIntegrityOptions & CODEINTEGRITY_OPTION_DEBUGMODE_ENABLED) ? L"on" : L"off",
            (codeIntegrity.CodeIntegrityOptions & CODEINTEGRITY_OPTION_HVCI_KMCI_ENABLED) ? L"on" : L"off"
            );
    }

    if (NT_SUCCESS(NtQuerySystemInformation(SystemSecureBootInformation, &secureBoot, sizeof(secureBoot), NULL)))
        PhAppendFormatStringBuilder(Builder, L"Secure Boot: %s\n", secureBoot.SecureBootEnabled ? L"on" : L"off");
}

static VOID PhpHwReportProcessor(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PH_STRINGREF processorKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
    PPH_STRING brand;
    HANDLE keyHandle;

    if (brand = PhpHwQueryProcessorName())
    {
        PhAppendFormatStringBuilder(Builder, L"Brand: %s\n", brand->Buffer);
        PhDereferenceObject(brand);
    }

    if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_READ, NULL, &processorKeyName, 0)))
    {
        PKEY_VALUE_PARTIAL_INFORMATION value;

        if (NT_SUCCESS(PhQueryValueKey(keyHandle, &(PH_STRINGREF)PH_STRINGREF_INIT(L"Update Revision"), KeyValuePartialInformation, &value)))
        {
            // The revision is the value's last ULONG (the whole of a 4-byte value, the high half of an 8-byte one).
            if (value->DataLength >= sizeof(ULONG))
                PhAppendFormatStringBuilder(Builder, L"Microcode: 0x%08lx\n", *(PULONG)(value->Data + value->DataLength - sizeof(ULONG)));

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
 * \param Sensors The sensors as text, or NULL to format the current sensors.
 * \return The report.
 */
PPH_STRING NTAPI PhHwCreateReport(
    _In_opt_ PCPH_STRINGREF Sensors
    )
{
    static CONST PH_STRINGREF currentVersionKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
    static CONST PH_STRINGREF revisionValueName = PH_STRINGREF_INIT(L"UBR");
    PH_STRING_BUILDER builder;
    PPH_STRING version;
    PPH_STRING sensorsText = NULL;
    static CONST PH_STRINGREF available = PH_STRINGREF_INIT(L"Hardware sensors are available.");
    PCPH_STRINGREF reason;
    HANDLE keyHandle;
    ULONG revision = ULONG_MAX;

    PhInitializeStringBuilder(&builder, 0x4000);

    if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_READ, NULL, &currentVersionKeyName, 0)))
    {
        revision = PhQueryRegistryUlong(keyHandle, &revisionValueName);
        NtClose(keyHandle);
    }

    version = PhGetApplicationVersionString(FALSE);
    PhAppendFormatStringBuilder(
        &builder,
        L"System Informer hardware report\n%s\nWindows %lu.%lu.%lu",
        version->Buffer,
        PhOsVersion.MajorVersion,
        PhOsVersion.MinorVersion,
        PhOsVersion.BuildNumber
        );
    PhDereferenceObject(version);

    if (revision != ULONG_MAX)
        PhAppendFormatStringBuilder(&builder, L".%lu", revision);

    PhAppendFormatStringBuilder(&builder, L"\nCreated %s\n", PhaFormatDateTime(NULL)->Buffer);

    PhpHwReportSection(&builder, L"Status");
    reason = PhHwGetSensorsUnavailableReason();
    PhAppendStringBuilder(&builder, reason ? reason : &available);
    PhAppendCharStringBuilder(&builder, L'\n');

    PhpHwReportSection(&builder, L"Software");
    PhpHwReportSoftware(&builder);

    PhpHwReportSection(&builder, L"Platform");
    PhpHwReportPlatform(&builder);

    PhpHwReportSection(&builder, L"Provider");
    PhHwWriteDiagnostics(&builder);

    PhpHwReportSection(&builder, L"Sources");
    PhHwWriteSourceDiagnostics(&builder);

    PhpHwReportSection(&builder, L"Processor");
    PhpHwReportProcessor(&builder);

    PhpHwReportSection(&builder, L"Sensors");

    if (!Sensors)
    {
        sensorsText = PhHwFormatSensorsText();
        Sensors = &sensorsText->sr;
    }

    if (Sensors->Length)
        PhAppendStringBuilder(&builder, Sensors);
    else
        PhAppendStringBuilder2(&builder, L"None\n");

    PhpHwReportSection(&builder, L"Devices");
    PhpHwReportDevices(&builder);

    PhpHwReportSection(&builder, L"PCI error policy");
    PhHwWriteBridgeDiagnostics(&builder);

    PhpHwReportSection(&builder, L"SMBIOS");
    PhEnumSMBIOS(PhpHwReportSmbiosCallback, &builder);

    PhClearReference(&sensorsText);

    return PhFinalStringBuilderString(&builder);
}
