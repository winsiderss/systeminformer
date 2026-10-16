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

#define MB_SLOTS 2
#define MB_MAX_REGISTERS 256
#define MB_MAX_SENSORS 128
#define MB_MIN_INTERVAL_MS 900
#define MB_STALE_LIMIT 5
#define MB_STRIKE_LIMIT 2
#define MB_TIMEOUT_MS 50 // per request; bounds the NCT668x EC page waits

typedef enum _MB_SLOT_STATE
{
    MbSlotProbe,
    MbSlotActive,
    MbSlotStopped
} MB_SLOT_STATE;

typedef struct _MB_SLOT
{
    UCHAR Slot;
    MB_SLOT_STATE State;
    NTSTATUS LastStatus;
    UCHAR Strikes;
    UCHAR Rejected; // consecutive snapshots the decoder rejected
    KPH_HW_SUPERIO_CHIP Identity;
    MB_CHIP_STATE Chip;
    ULONG RegisterCount;
    USHORT Registers[MB_MAX_REGISTERS];
    KPH_HW_SUPERIO_BATCH_ENTRY Entries[MB_MAX_REGISTERS];
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
    LONG Stopping;
    BOOLEAN GateOpen;
    ULONG64 LastPollTime;
    PPH_STRING Manufacturer;
    PPH_STRING Product;
    MB_SLOT Slots[MB_SLOTS];
} MB_CONTEXT, *PMB_CONTEXT;

static PMB_CONTEXT MotherboardContext = NULL;
static HANDLE MotherboardThreadHandle = NULL;
static HANDLE MotherboardWakeEvent = NULL;
static PPH_HW_SENSOR_SOURCE_REGISTRATION MotherboardRegistration = NULL;

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
        USHORT reg = Slot->Registers[i];

        if (reg >= MB_REGISTER_SPACE || !(Slot->Snapshot.Valid[reg / 32] & (1ul << (reg % 32))))
            continue;

        Slot->ReportRegisters[reportCount] = reg;
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
    Slot->RegisterCount = 0;
    memset(&Slot->Identity, 0, sizeof(KPH_HW_SUPERIO_CHIP));
}

static VOID MotherboardIdentify(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot,
    _In_ const KPH_HW_SUPERIO_CHIP* Identity
    )
{
    PH_FORMAT format[6];
    ULONG count = 0;
    WCHAR componentName[RTL_NUMBER_OF(Slot->ComponentName)];

    memset(Slot->Snapshot.Valid, 0, sizeof(Slot->Snapshot.Valid));
    memset(&Slot->Chip, 0, sizeof(Slot->Chip));
    Slot->Strikes = 0;
    Slot->Rejected = 0;
    Slot->Identity = *Identity;
    MotherboardPublish(Context, Slot, 0);

    if (!SuperIoIdentifyChip(
        Identity,
        Context->Manufacturer ? &Context->Manufacturer->sr : NULL,
        Context->Product ? &Context->Product->sr : NULL,
        &Slot->Chip
        ))
    {
        MotherboardStopSlot(Context, Slot, STATUS_NOT_SUPPORTED);
        return;
    }

    Slot->RegisterCount = SuperIoGetRegisters(&Slot->Chip, Slot->Registers, RTL_NUMBER_OF(Slot->Registers));

    if (!Slot->RegisterCount)
    {
        MotherboardStopSlot(Context, Slot, STATUS_NOT_SUPPORTED);
        return;
    }

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

static BOOLEAN MotherboardSameChip(
    _In_ const KPH_HW_SUPERIO_CHIP* Chip1,
    _In_ const KPH_HW_SUPERIO_CHIP* Chip2
    )
{
    return Chip1->VendorFamily == Chip2->VendorFamily && Chip1->ChipId == Chip2->ChipId && Chip1->CustomerId == Chip2->CustomerId;
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

static VOID MotherboardApplyBatch(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot
    )
{
    ULONG kept = 0;
    BOOLEAN answered = FALSE;
    NTSTATUS failure = STATUS_SUCCESS;

    for (ULONG i = 0; i < Slot->RegisterCount; i++)
    {
        USHORT reg = Slot->Entries[i].Register;
        NTSTATUS entryStatus = Slot->Entries[i].Status;

        if (reg >= MB_REGISTER_SPACE)
            continue;

        if (NT_SUCCESS(entryStatus))
        {
            Slot->Snapshot.Value[reg] = Slot->Entries[i].Value;
            Slot->Snapshot.Age[reg] = 0;
            Slot->Snapshot.Valid[reg / 32] |= 1ul << (reg % 32);
            answered = TRUE;
        }
        else if (entryStatus == STATUS_IO_TIMEOUT && Slot->Snapshot.Age[reg] + 1 < MB_STALE_LIMIT)
        {
            // The EC page timed out: keep the previous value for a few polls.
            Slot->Snapshot.Age[reg]++;
        }
        else
        {
            Slot->Snapshot.Valid[reg / 32] &= ~(1ul << (reg % 32));
        }

        if (!NT_SUCCESS(entryStatus) && NT_SUCCESS(failure))
            failure = entryStatus;

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

// The bus was held by another agent: values age out after MB_STALE_LIMIT polls instead of freezing.
static VOID MotherboardAgeSlot(
    _In_ PMB_CONTEXT Context,
    _Inout_ PMB_SLOT Slot
    )
{
    if (Slot->State != MbSlotActive)
        return;

    for (ULONG i = 0; i < Slot->RegisterCount; i++)
    {
        USHORT reg = Slot->Registers[i];

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
    KPH_HW_SUPERIO_CHIP identity;
    LARGE_INTEGER timeout;
    ULONG count;

    if (Slot->State == MbSlotStopped)
        return;

    if (Slot->State == MbSlotProbe)
    {
        count = 0; // identify the chip only
    }
    else
    {
        for (ULONG i = 0; i < Slot->RegisterCount; i++)
        {
            Slot->Entries[i].Register = Slot->Registers[i];
            Slot->Entries[i].Reserved = 0;
        }

        count = Slot->RegisterCount;
    }

    status = KphHwReadSuperIoBatch(Slot->Slot, 0, Slot->Entries, count, &identity, PhTimeoutFromMilliseconds(&timeout, MB_TIMEOUT_MS));

    if (identity.ChipId)
    {
        if (Slot->State == MbSlotProbe || !MotherboardSameChip(&identity, &Slot->Identity))
        {
            MotherboardIdentify(Context, Slot, &identity);
            return;
        }

        if (NT_SUCCESS(status))
        {
            MotherboardApplyBatch(Context, Slot);
            return;
        }
    }

    switch (status)
    {
    case STATUS_NOT_SUPPORTED: // SuperIO is x64 only
    case STATUS_INVALID_PARAMETER:
    case STATUS_NOT_FOUND: // no chip, or no row for it
    case STATUS_ACCESS_DENIED:
    case STATUS_NOINTERFACE: // no hardware data
    case STATUS_DEVICE_NOT_READY: // the chip's monitor is not active or has no base
    case STATUS_DEVICE_CONFIGURATION_ERROR:
    case STATUS_INTEGER_OVERFLOW:
    case STATUS_CONFLICTING_ADDRESSES:
        MotherboardStopSlot(Context, Slot, status);
        break;
    case STATUS_RETRY: // another agent held the bus: not a strike
        MotherboardAgeSlot(Context, Slot);
        break;
    default:
        // Transient failure: keep the published values until MB_STRIKE_LIMIT in a row.
        if (++Slot->Strikes >= MB_STRIKE_LIMIT)
            MotherboardStopSlot(Context, Slot, status);
        break;
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
static NTSTATUS NTAPI MotherboardThreadStart(
    _In_ PVOID Parameter
    )
{
    PMB_CONTEXT context = Parameter;
    BOOLEAN gateOpen = FALSE;

    PhSetThreadName(NtCurrentThread(), L"MotherboardSensors");

    PhEnumSMBIOS(MotherboardSmbiosCallback, context);

    while (TRUE)
    {
        ULONG64 now;

        NtWaitForSingleObject(MotherboardWakeEvent, FALSE, NULL);

        if (ReadAcquire(&context->Stopping))
            break;

        // Same gate as the core sensors; each opening starts a new session.
        if (!PhHwSensorsEnabled())
        {
            if (gateOpen)
            {
                gateOpen = FALSE;

                for (ULONG i = 0; i < MB_SLOTS; i++)
                    MotherboardResetSlot(context, &context->Slots[i]);

                PhAcquireQueuedLockExclusive(&context->Lock);
                context->GateOpen = FALSE;
                PhReleaseQueuedLockExclusive(&context->Lock);
            }

            continue;
        }

        if (!gateOpen)
        {
            gateOpen = TRUE;
            context->LastPollTime = 0;

            for (ULONG i = 0; i < MB_SLOTS; i++)
                MotherboardResetSlot(context, &context->Slots[i]);

            PhAcquireQueuedLockExclusive(&context->Lock);
            context->GateOpen = TRUE;
            PhReleaseQueuedLockExclusive(&context->Lock);
        }

        // Woken by each provider update; the monitors refresh about once a second.
        now = NtGetTickCount64();

        if (context->LastPollTime && now - context->LastPollTime < MB_MIN_INTERVAL_MS)
            continue;

        context->LastPollTime = now;

        for (ULONG i = 0; i < MB_SLOTS; i++)
            MotherboardPollSlot(context, &context->Slots[i]);
    }

    return STATUS_SUCCESS;
}

_Function_class_(PH_HW_SENSOR_SOURCE_CALLBACK)
static VOID NTAPI MotherboardSensorSourceCallback(
    _In_ PPH_HW_SENSOR_REPORT_FUNCTION Report,
    _In_ PVOID ReportContext,
    _In_opt_ PVOID Context
    )
{
    PMB_CONTEXT context = Context;
    PH_HW_SENSOR_REPORT report;

    if (!context)
        return;

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

        PhAppendFormatStringBuilder(
            Builder,
            L"  SuperIO %s: status 0x%08lx, family %lu, chip 0x%04x, customer 0x%04x, %s, board map %s, %lu sensors\n",
            i ? L"4Eh" : L"2Eh",
            slot->LastStatus,
            (ULONG)slot->ReportIdentity.VendorFamily,
            slot->ReportIdentity.ChipId,
            slot->ReportIdentity.CustomerId,
            slot->ChipName ? slot->ChipName : L"no decoder",
            slot->ReportBoard ? L"yes" : L"no",
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

    if (MotherboardThreadHandle)
    {
        WriteRelease(&context->Stopping, TRUE);
        NtSetEvent(MotherboardWakeEvent, NULL);
        NtWaitForSingleObject(MotherboardThreadHandle, FALSE, NULL);
        NtClose(MotherboardThreadHandle);
        MotherboardThreadHandle = NULL;
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

    if (MotherboardContext || !MotherboardWakeEvent)
        return;

    context = PhAllocateZero(sizeof(MB_CONTEXT));
    PhInitializeQueuedLock(&context->Lock);

    for (ULONG i = 0; i < MB_SLOTS; i++)
    {
        context->Slots[i].Slot = (UCHAR)i;
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
        return;
    }

    if (!NT_SUCCESS(PhCreateThreadEx(&MotherboardThreadHandle, MotherboardThreadStart, context)))
    {
        MotherboardThreadHandle = NULL;
        MotherboardStop();
        return;
    }

    NtSetEvent(MotherboardWakeEvent, NULL);
}

/**
 * Starts the motherboard sensors when enabled. Called on plugin load.
 */
VOID MotherboardInitialize(
    VOID
    )
{
#if defined(_M_X64)
    // The event lives for the process so update callbacks never race its close.
    if (!NT_SUCCESS(NtCreateEvent(&MotherboardWakeEvent, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
    {
        MotherboardWakeEvent = NULL;
        return;
    }

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
 * Wakes the poller. Called on each process provider update.
 */
VOID MotherboardUpdate(
    VOID
    )
{
    if (MotherboardWakeEvent)
        NtSetEvent(MotherboardWakeEvent, NULL);
}

static PPH_STRING MotherboardGetStatusText(
    VOID
    )
{
    PMB_CONTEXT context = MotherboardContext;
    PH_STRING_BUILDER sb;

    if (!MotherboardWakeEvent)
        return PhCreateString(L"Motherboard sensors are available on x64 only.");
    if (!context)
        return PhCreateString(L"Motherboard sensors are disabled.");

    PhInitializeStringBuilder(&sb, 128);

    PhAcquireQueuedLockShared(&context->Lock);

    if (!context->GateOpen)
        PhAppendStringBuilder2(&sb, L"Waiting for hardware sensors (Options > Advanced) and full driver access.\r\n");

    for (ULONG i = 0; context->GateOpen && i < MB_SLOTS; i++)
    {
        PMB_SLOT slot = &context->Slots[i];
        PCWSTR port = i ? L"4Eh" : L"2Eh";

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
            Button_SetCheck(
                GetDlgItem(WindowHandle, IDC_MOTHERBOARD_ENABLE),
                PhGetIntegerSetting(SETTING_NAME_MOTHERBOARD_ENABLED) ? BST_CHECKED : BST_UNCHECKED
                );

            if (!MotherboardWakeEvent)
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
