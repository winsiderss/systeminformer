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

#include "devices.h"
#include <phfirmware.h>

#define MB_SLOTS 4 // 2Eh, 4Eh, and the SMSC strap ports 162Eh and 164Eh
#define MB_MAX_REGISTERS 256
#define MB_MAX_SENSORS 128
#define MB_MIN_INTERVAL_MS 900
#define MB_STALE_LIMIT 5
#define MB_STRIKE_LIMIT 2
#define MB_NOINTERFACE_POLLS 10 // polls between tries of a slot that has no hardware data yet

typedef enum _MB_SLOT_STATE
{
    MbSlotProbe,
    MbSlotActive,
    MbSlotStopped
} MB_SLOT_STATE;

typedef struct _MB_SLOT
{
    ULONG Slot;
    MB_SLOT_STATE State;
    NTSTATUS LastStatus;
    UCHAR Strikes;
    UCHAR Rejected; // consecutive snapshots the decoder rejected
    UCHAR Backoff; // polls left before the next try after STATUS_NOINTERFACE
    KPH_HW_SUPERIO_CHIP Identity;
    MB_CHIP_STATE Chip;
    ULONG RegisterCount;
    ULONG Registers[MB_MAX_REGISTERS];
    KPH_HW_VALUE32 Values[MB_MAX_REGISTERS];
    MB_REGISTERS Snapshot;
    ULONG DecodedCount;
    MB_SENSOR Decoded[MB_MAX_SENSORS];

    PCWSTR ChipName;
    WCHAR ComponentName[128];
    ULONG SensorCount;
    MB_SENSOR Sensors[MB_MAX_SENSORS];

    // Published for the hardware report.
    KPH_HW_SUPERIO_CHIP ReportIdentity;
    BOOLEAN ReportBoard; // a board map applies
    ULONG ReportCount;
    USHORT ReportRegisters[MB_MAX_REGISTERS];
    UCHAR ReportValues[MB_MAX_REGISTERS];
} MB_SLOT, *PMB_SLOT;

typedef struct _MB_CONTEXT
{
    PH_QUEUED_LOCK Lock;
    BOOLEAN SmbiosRead;
    ULONG64 LastPollTime;
    HD_WAKE Wake;
    PPH_STRING Manufacturer;
    PPH_STRING Product;
    MB_SLOT Slots[MB_SLOTS];
} MB_CONTEXT, *PMB_CONTEXT;

static CONST PCWSTR MotherboardSlotNames[MB_SLOTS] = { L"2Eh", L"4Eh", L"162Eh", L"164Eh" };
static BOOLEAN MotherboardSupported = FALSE;
static LONG MotherboardSessionEnded = FALSE;
static PMB_CONTEXT MotherboardContext = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION MotherboardRegistration = NULL;
// GUID_DEVCLASS_SYSTEM
static CONST GUID MotherboardClass = { 0x4d36e97d, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };

static ULONG64 MotherboardQueryTime(
    VOID
    )
{
    return NtGetTickCount64();
}

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
static BOOLEAN NTAPI MotherboardSmbiosCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t");
    PMB_CONTEXT context = Context;
    PPH_STRING string;
    PH_STRINGREF trimmed;

    if (!context || Entry->Header.Type != SMBIOS_BASEBOARD_INFORMATION_TYPE)
        return FALSE;

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Manufacturer) &&
        NT_SUCCESS(PhGetSMBIOSString(EnumHandle, Entry->Baseboard.Manufacturer, &string)))
    {
        trimmed = string->sr;
        PhTrimStringRef(&trimmed, &whitespace, 0);
        context->Manufacturer = PhCreateString2(&trimmed);
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Product) &&
        NT_SUCCESS(PhGetSMBIOSString(EnumHandle, Entry->Baseboard.Product, &string)))
    {
        trimmed = string->sr;
        PhTrimStringRef(&trimmed, &whitespace, 0);
        context->Product = PhCreateString2(&trimmed);
        PhDereferenceObject(string);
    }

    return TRUE;
}

static VOID MotherboardPublish(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot,
    _In_ ULONG Count
    )
{
    ULONG reportCount = 0;

    PhAcquireQueuedLockExclusive(&Context->Lock);
    memcpy(Slot->Sensors, Slot->Decoded, Count * sizeof(MB_SENSOR));
    Slot->SensorCount = Count;

    // The raw values the decoders saw, for the hardware report.
    for (ULONG i = 0; i < Slot->RegisterCount; i++)
    {
        ULONG reg = Slot->Registers[i];

        if (reg >= MB_REGISTER_SPACE || !(Slot->Snapshot.Valid[reg / 32] & (1ul << (reg % 32))))
            continue;

        Slot->ReportRegisters[reportCount] = (USHORT)reg;
        Slot->ReportValues[reportCount] = Slot->Snapshot.Value[reg];
        reportCount++;
    }

    Slot->ReportCount = reportCount;
    Slot->ReportIdentity = Slot->Identity;
    Slot->ReportBoard = Slot->State != MbSlotProbe && Slot->Chip.Chip && Slot->Chip.Board;
    PhReleaseQueuedLockExclusive(&Context->Lock);
}

static VOID MotherboardSetStatus(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot,
    _In_ NTSTATUS Status,
    _In_opt_ PCWSTR ChipName
    )
{
    PhAcquireQueuedLockExclusive(&Context->Lock);
    Slot->LastStatus = Status;
    Slot->ChipName = ChipName;
    PhReleaseQueuedLockExclusive(&Context->Lock);
}

static VOID MotherboardStopSlot(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot,
    _In_ NTSTATUS Status
    )
{
    MotherboardPublish(Context, Slot, 0);
    MotherboardSetStatus(Context, Slot, Status, NULL);

    Slot->State = MbSlotStopped;
    Slot->RegisterCount = 0;
}

static VOID MotherboardResetSlot(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot
    )
{
    MotherboardPublish(Context, Slot, 0);
    MotherboardSetStatus(Context, Slot, STATUS_PENDING, NULL);

    Slot->State = MbSlotProbe;
    Slot->Strikes = 0;
    Slot->Backoff = 0;
    Slot->RegisterCount = 0;
    memset(&Slot->Identity, 0, sizeof(KPH_HW_SUPERIO_CHIP));
}

static VOID MotherboardIdentify(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot,
    _In_ PCKPH_HW_SUPERIO_CHIP Identity
    )
{
    PH_FORMAT format[6];
    ULONG count = 0;
    USHORT registers[MB_MAX_REGISTERS];
    WCHAR componentName[RTL_NUMBER_OF(Slot->ComponentName)];
    PCPH_STRINGREF manufacturer = NULL;
    PCPH_STRINGREF product = NULL;

    memset(Slot->Snapshot.Valid, 0, sizeof(Slot->Snapshot.Valid));
    memset(&Slot->Chip, 0, sizeof(Slot->Chip));
    Slot->Strikes = 0;
    Slot->Rejected = 0;
    Slot->Identity = *Identity;
    MotherboardPublish(Context, Slot, 0);

    if (Context->Manufacturer)
        manufacturer = &Context->Manufacturer->sr;
    if (Context->Product)
        product = &Context->Product->sr;

    if (!SuperIoIdentifyChip(
        Identity,
        manufacturer,
        product,
        &Slot->Chip
        ))
    {
        MotherboardStopSlot(Context, Slot, STATUS_NOT_SUPPORTED);
        return;
    }

    Slot->RegisterCount = SuperIoGetRegisters(&Slot->Chip, registers, RTL_NUMBER_OF(registers));

    if (!Slot->RegisterCount)
    {
        MotherboardStopSlot(Context, Slot, STATUS_NOT_SUPPORTED);
        return;
    }

    for (ULONG i = 0; i < Slot->RegisterCount; i++)
        Slot->Registers[i] = registers[i];

    // Motherboard: <baseboard product> (<chip>)
    PhInitFormatS(&format[count++], L"Motherboard");
    if (!PhIsNullOrEmptyString(Context->Product))
    {
        PhInitFormatS(&format[count++], L": ");
        PhInitFormatSR(&format[count++], Context->Product->sr);
    }
    PhInitFormatS(&format[count++], L" (");
    PhInitFormatS(&format[count++], SuperIoGetChipName(&Slot->Chip));
    PhInitFormatS(&format[count++], L")");
    assert(count <= RTL_NUMBER_OF(format));

    if (!PhFormatToBuffer(format, count, componentName, sizeof(componentName), NULL))
        PhCopyStringZ(SuperIoGetChipName(&Slot->Chip), SIZE_MAX, componentName, RTL_NUMBER_OF(componentName), NULL);

    PhAcquireQueuedLockExclusive(&Context->Lock);
    memcpy(Slot->ComponentName, componentName, sizeof(componentName));
    PhReleaseQueuedLockExclusive(&Context->Lock);

    MotherboardSetStatus(Context, Slot, STATUS_SUCCESS, SuperIoGetChipName(&Slot->Chip));

    Slot->State = MbSlotActive;
}

// A rejected snapshot keeps the last sensors for MB_STALE_LIMIT polls, then publishes none; the raw registers always refresh.
static VOID MotherboardDecode(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot
    )
{
    BOOLEAN rejected;
    ULONG count;

    count = SuperIoDecode(&Slot->Chip, &Slot->Snapshot, Slot->Decoded, RTL_NUMBER_OF(Slot->Decoded), &rejected);

    if (rejected && Slot->Rejected + 1 < MB_STALE_LIMIT)
    {
        Slot->Rejected++;
    }
    else
    {
        if (!rejected)
            Slot->Rejected = 0;

        Slot->DecodedCount = count;
    }

    MotherboardPublish(Context, Slot, Slot->DecodedCount);
}

static VOID MotherboardApplyValues(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot
    )
{
    ULONG kept = 0;
    BOOLEAN answered = FALSE;
    NTSTATUS failure = STATUS_SUCCESS;

    for (ULONG i = 0; i < Slot->RegisterCount; i++)
    {
        ULONG reg = Slot->Registers[i];
        NTSTATUS entryStatus = Slot->Values[i].Status;

        if (reg >= MB_REGISTER_SPACE)
            continue;

        if (NT_SUCCESS(entryStatus))
        {
            Slot->Snapshot.Value[reg] = (UCHAR)Slot->Values[i].Value;
            Slot->Snapshot.Age[reg] = 0;
            Slot->Snapshot.Valid[reg / 32] |= 1ul << (reg % 32);
            answered = TRUE;
        }
        else if ((entryStatus == STATUS_IO_TIMEOUT || entryStatus == STATUS_RETRY) && Slot->Snapshot.Age[reg] + 1 < MB_STALE_LIMIT)
        {
            // The EC page timed out or the read stopped early: keep the previous value for a few polls.
            Slot->Snapshot.Age[reg]++;
        }
        else
        {
            Slot->Snapshot.Valid[reg / 32] &= ~(1ul << (reg % 32));
        }

        if (!NT_SUCCESS(entryStatus) && NT_SUCCESS(failure))
            failure = entryStatus;

        // A register the script refuses is not asked for again this session.
        if (entryStatus != STATUS_ACCESS_DENIED)
            Slot->Registers[kept++] = reg;
    }

    Slot->RegisterCount = kept;

    if (!kept)
    {
        MotherboardStopSlot(Context, Slot, STATUS_ACCESS_DENIED);
        return;
    }

    MotherboardDecode(Context, Slot);

    // No entry answered: transient, stopped after MB_STRIKE_LIMIT polls in a row.
    if (answered)
        Slot->Strikes = 0;
    else if (++Slot->Strikes >= MB_STRIKE_LIMIT)
        MotherboardStopSlot(Context, Slot, failure);
}

// Another tool held the bus: values age out after MB_STALE_LIMIT polls instead of freezing.
static VOID MotherboardAgeSlot(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot
    )
{
    if (Slot->State != MbSlotActive)
        return;

    for (ULONG i = 0; i < Slot->RegisterCount; i++)
    {
        ULONG reg = Slot->Registers[i];

        if (reg >= MB_REGISTER_SPACE)
            continue;

        if (Slot->Snapshot.Age[reg] + 1 < MB_STALE_LIMIT)
            Slot->Snapshot.Age[reg]++;
        else
            Slot->Snapshot.Valid[reg / 32] &= ~(1ul << (reg % 32));
    }

    MotherboardDecode(Context, Slot);
}

static VOID MotherboardPollSlot(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot
    )
{
    NTSTATUS status;

    if (Slot->State == MbSlotStopped)
        return;

    if (Slot->Backoff)
    {
        Slot->Backoff--;
        MotherboardAgeSlot(Context, Slot);
        return;
    }

    if (Slot->State == MbSlotProbe)
    {
        KPH_HW_SUPERIO_CHIP identity;

        status = KphHwSuperIoIdentify(Slot->Slot, &identity);

        if (NT_SUCCESS(status) && identity.Chip)
        {
            MotherboardIdentify(Context, Slot, &identity);
            return;
        }

        if (NT_SUCCESS(status))
            status = STATUS_NOT_FOUND;
    }
    else
    {
        status = KphHwSuperIoRead(Slot->Slot, Slot->Registers, Slot->RegisterCount, Slot->Values);

        if (NT_SUCCESS(status))
        {
            MotherboardApplyValues(Context, Slot);
            return;
        }
    }

    switch (status)
    {
    case STATUS_NOT_SUPPORTED:
    case STATUS_INVALID_PARAMETER: // also: none of the registers asked for is permitted
    case STATUS_NOT_FOUND: // no chip, or no row for it
    case STATUS_ACCESS_DENIED:
    case STATUS_DEVICE_NOT_READY: // the chip's monitor is not active or has no base
    case STATUS_DEVICE_CONFIGURATION_ERROR:
    case STATUS_INTEGER_OVERFLOW:
    case STATUS_CONFLICTING_ADDRESSES:
        MotherboardStopSlot(Context, Slot, status);
        break;
    case STATUS_NOINTERFACE: // no hardware data yet: a later activation can supply it
        Slot->Backoff = MB_NOINTERFACE_POLLS - 1;
        MotherboardAgeSlot(Context, Slot);
        break;
    case STATUS_RETRY: // another tool held the bus: not a strike
    case STATUS_IO_TIMEOUT: // another tool held the NCT668x EC page
        MotherboardAgeSlot(Context, Slot);
        break;
    default:
        // Transient failure: keep the published values until MB_STRIKE_LIMIT in a row.
        if (++Slot->Strikes >= MB_STRIKE_LIMIT)
            MotherboardStopSlot(Context, Slot, status);
        break;
    }
}

// Called by the provider once per update while hardware sensors are enabled, on the provider thread.
_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI MotherboardSensorSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PMB_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;
    ULONG64 now;

    if (!context)
        return;

    if (HardwareDeviceWoke(&context->Wake, MotherboardQueryTime()))
        WriteRelease(&MotherboardSessionEnded, TRUE);

    // The Super I/O is read through the driver; without it there is nothing to report, and its return starts a
    // new session.
    if (KsiLevel() != KphLevelMax)
    {
        WriteRelease(&MotherboardSessionEnded, TRUE);
        return;
    }

    if (!context->SmbiosRead)
    {
        context->SmbiosRead = TRUE;
        PhEnumSMBIOS(MotherboardSmbiosCallback, context);
    }

    // Each opening of the gate starts a new session.
    if (InterlockedExchange(&MotherboardSessionEnded, FALSE))
    {
        context->LastPollTime = 0;

        for (ULONG i = 0; i < MB_SLOTS; i++)
            MotherboardResetSlot(context, &context->Slots[i]);
    }

    now = MotherboardQueryTime();

    if (!context->LastPollTime || now - context->LastPollTime >= MB_MIN_INTERVAL_MS)
    {
        context->LastPollTime = now;

        for (ULONG i = 0; i < MB_SLOTS; i++)
            MotherboardPollSlot(context, &context->Slots[i]);
    }

    PhAcquireQueuedLockShared(&context->Lock);

    for (ULONG i = 0; i < MB_SLOTS; i++)
    {
        PMB_SLOT slot = &context->Slots[i];

        for (ULONG j = 0; j < slot->SensorCount; j++)
        {
            memset(&report, 0, sizeof(PH_HW_SENSOR_REPORT));
            report.Size = sizeof(PH_HW_SENSOR_REPORT);
            report.Flags = PH_HW_SENSOR_REPORT_HISTORY;
            report.ComponentId = slot->Slot;
            report.Type = slot->Sensors[j].Type;
            report.ComponentName = slot->ComponentName;
            report.SensorName = slot->Sensors[j].Name;
            report.SensorId = slot->Sensors[j].SensorId;
            report.Value = slot->Sensors[j].Value;
            report.ComponentClass = &MotherboardClass;

            Report(ReportContext, &report);
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

_Function_class_(PH_HW_SENSOR_SOURCE_DIAGNOSTICS_CALLBACK)
static VOID NTAPI MotherboardSensorSourceDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder,
    _In_opt_ PVOID Context
    )
{
    PMB_CONTEXT context = Context;

    if (!context)
        return;

    PhAcquireQueuedLockShared(&context->Lock);

    for (ULONG i = 0; i < MB_SLOTS; i++)
    {
        PMB_SLOT slot = &context->Slots[i];
        PCWSTR chipName = L"no decoder";
        PCWSTR board = L"no";

        if (slot->ChipName)
            chipName = slot->ChipName;
        if (slot->ReportBoard)
            board = L"yes";

        PhAppendFormatStringBuilder(
            Builder,
            L"  SuperIO %s: status 0x%08lx, family %lu, chip 0x%04lx, customer 0x%04lx, %s, board map %s, %lu sensors\n",
            MotherboardSlotNames[i],
            slot->LastStatus,
            slot->ReportIdentity.Family,
            slot->ReportIdentity.Chip,
            slot->ReportIdentity.Customer,
            chipName,
            board,
            slot->SensorCount
            );

        for (ULONG j = 0; j < slot->ReportCount; j++)
        {
            if (j % 8 == 0)
                PhAppendStringBuilder2(Builder, L"   ");

            PhAppendFormatStringBuilder(Builder, L" %03x=%02x", slot->ReportRegisters[j], slot->ReportValues[j]);

            if (j % 8 == 7 || j + 1 == slot->ReportCount)
                PhAppendCharStringBuilder(Builder, L'\n');
        }
    }

    PhReleaseQueuedLockShared(&context->Lock);
}

static VOID MotherboardStop(
    VOID
    )
{
    PMB_CONTEXT context = MotherboardContext;

    if (!context)
        return;

    // Unregistering waits for a callback in progress.
    if (MotherboardRegistration)
    {
        PhUnregisterHardwareSensorSource(MotherboardRegistration);
        MotherboardRegistration = NULL;
    }

    PhClearReference(&context->Manufacturer);
    PhClearReference(&context->Product);
    PhFree(context);
    MotherboardContext = NULL;
}

static VOID MotherboardStart(
    VOID
    )
{
    PMB_CONTEXT context;
    PH_HW_SENSOR_SOURCE source;

    if (MotherboardContext || !MotherboardSupported)
        return;

    context = PhAllocateZero(sizeof(MB_CONTEXT));
    PhInitializeQueuedLock(&context->Lock);

    for (ULONG i = 0; i < MB_SLOTS; i++)
    {
        context->Slots[i].Slot = i;
        context->Slots[i].State = MbSlotProbe;
        context->Slots[i].LastStatus = STATUS_PENDING;
    }

    MotherboardContext = context;

    memset(&source, 0, sizeof(PH_HW_SENSOR_SOURCE));
    source.Size = sizeof(PH_HW_SENSOR_SOURCE);
    source.Callback = MotherboardSensorSourceCallback;
    source.Context = context;
    source.Diagnostics = MotherboardSensorSourceDiagnostics;

    if (!NT_SUCCESS(PhRegisterHardwareSensorSource(PluginInstance, &source, &MotherboardRegistration)))
    {
        MotherboardRegistration = NULL;
        MotherboardStop();
    }
}

/**
 * Starts the motherboard sensors when enabled. Called on plugin load.
 */
VOID MotherboardInitialize(
    VOID
    )
{
#if defined(_M_X64)
    MotherboardSupported = TRUE;

    if (PhGetIntegerSetting(SETTING_NAME_MOTHERBOARD_ENABLED))
        MotherboardStart();
#endif
}

/**
 * Stops the motherboard sensors. Called on plugin unload.
 */
VOID MotherboardUninitialize(
    VOID
    )
{
    MotherboardStop();
}

/**
 * Notes a closed gate, so the next source callback starts a new session. Called on each process provider update.
 */
VOID MotherboardUpdate(
    VOID
    )
{
    if (MotherboardSupported && !PhHwSensorsEnabled())
        WriteRelease(&MotherboardSessionEnded, TRUE);
}

static PPH_STRING MotherboardGetStatusText(
    VOID
    )
{
    PMB_CONTEXT context = MotherboardContext;
    PH_STRING_BUILDER sb;

    if (!MotherboardSupported)
        return PhCreateString(L"Motherboard sensors are available on x64 only.");
    if (!context)
        return PhCreateString(L"Motherboard sensors are disabled.");
    if (!PhHwSensorsEnabled())
        return PhCreateString(L"Waiting for hardware sensors (Options > Advanced) and full driver access.");

    PhInitializeStringBuilder(&sb, 128);

    PhAcquireQueuedLockShared(&context->Lock);

    for (ULONG i = 0; i < MB_SLOTS; i++)
    {
        PMB_SLOT slot = &context->Slots[i];
        PCWSTR port = MotherboardSlotNames[i];

        if (slot->ChipName)
            PhAppendFormatStringBuilder(&sb, L"%s: %s, %lu sensors\r\n", port, slot->ChipName, slot->SensorCount);
        else if (slot->LastStatus == STATUS_PENDING)
            PhAppendFormatStringBuilder(&sb, L"%s: detecting\r\n", port);
        else if (slot->LastStatus == STATUS_NOT_FOUND)
            PhAppendFormatStringBuilder(&sb, L"%s: no supported chip\r\n", port);
        else if (slot->LastStatus == STATUS_IO_TIMEOUT)
            PhAppendFormatStringBuilder(&sb, L"%s: stopped, the chip stopped answering\r\n", port);
        else if (slot->LastStatus == STATUS_NOT_SUPPORTED)
            PhAppendFormatStringBuilder(&sb, L"%s: not supported\r\n", port);
        else if (slot->LastStatus == STATUS_ACCESS_DENIED || slot->LastStatus == STATUS_INVALID_PARAMETER)
            PhAppendFormatStringBuilder(&sb, L"%s: the hardware data allows none of the chip's registers\r\n", port);
        else
            PhAppendFormatStringBuilder(&sb, L"%s: unavailable (0x%08lx)\r\n", port, slot->LastStatus);
    }

    PhReleaseQueuedLockShared(&context->Lock);

    return PhFinalStringBuilderString(&sb);
}

#define MB_OPTIONS_TIMER_ID 1

static VOID MotherboardUpdateStatusText(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING status;

    status = MotherboardGetStatusText();
    PhSetDialogItemText(WindowHandle, IDC_MOTHERBOARD_STATUS, status->Buffer);
    PhDereferenceObject(status);
}

INT_PTR CALLBACK MotherboardOptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            INT check = BST_UNCHECKED;

            if (PhGetIntegerSetting(SETTING_NAME_MOTHERBOARD_ENABLED))
                check = BST_CHECKED;

            Button_SetCheck(
                GetDlgItem(WindowHandle, IDC_MOTHERBOARD_ENABLE),
                check
                );

            if (!MotherboardSupported)
                EnableWindow(GetDlgItem(WindowHandle, IDC_MOTHERBOARD_ENABLE), FALSE);

            MotherboardUpdateStatusText(WindowHandle);
            SetTimer(WindowHandle, MB_OPTIONS_TIMER_ID, 1000, NULL);
        }
        break;
    case WM_DESTROY:
        {
            KillTimer(WindowHandle, MB_OPTIONS_TIMER_ID);
        }
        break;
    case WM_TIMER:
        {
            if (wParam == MB_OPTIONS_TIMER_ID)
                MotherboardUpdateStatusText(WindowHandle);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_MOTHERBOARD_ENABLE:
                {
                    BOOLEAN enabled = Button_GetCheck(GetDlgItem(WindowHandle, IDC_MOTHERBOARD_ENABLE)) == BST_CHECKED;

                    PhSetIntegerSetting(SETTING_NAME_MOTHERBOARD_ENABLED, enabled);

                    if (enabled)
                        MotherboardStart();
                    else
                        MotherboardStop();

                    MotherboardUpdateStatusText(WindowHandle);
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
