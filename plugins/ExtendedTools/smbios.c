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

#include "exttools.h"

#include <phfirmware.h>
#include <secedit.h>

typedef struct _SMBIOS_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;

    BOOLEAN ShowUndefinedTypes;

    ULONG GroupCounter;
    ULONG IndexCounter;
} SMBIOS_WINDOW_CONTEXT, *PSMBIOS_WINDOW_CONTEXT;

static const PH_KEY_VALUE_PAIR EtSMBIOSProbeStatus[] =
{
    SIP(L"Other", SMBIOS_PROBE_STATUS_OTHER),
    SIP(L"Unknown", SMBIOS_PROBE_STATUS_UNKNOWN),
    SIP(L"Ok", SMBIOS_PROBE_STATUS_OK),
    SIP(L"Non-critical", SMBIOS_PROBE_STATUS_NON_CRITICAL),
    SIP(L"Critical", SMBIOS_PROBE_STATUS_CRITICAL),
    SIP(L"Non-recoverable", SMBIOS_PROBE_STATUS_NON_RECOVERABLE),
};

static const PH_KEY_VALUE_PAIR EtSMBIOSMemoryErrorTypes[] =
{
    SIP(L"Other", SMBIOS_MEMORY_ERROR_TYPE_OTHER),
    SIP(L"Unknown", SMBIOS_MEMORY_ERROR_TYPE_UNKNOWN),
    SIP(L"Ok", SMBIOS_MEMORY_ERROR_TYPE_OK),
    SIP(L"Bad read", SMBIOS_MEMORY_ERROR_TYPE_BAD_READ),
    SIP(L"Parity", SMBIOS_MEMORY_ERROR_TYPE_PARITY),
    SIP(L"Single-bit", SMBIOS_MEMORY_ERROR_TYPE_SINGLE_BIT),
    SIP(L"Double-bit", SMBIOS_MEMORY_ERROR_TYPE_DOUBLE_BIT),
    SIP(L"Multi-bit", SMBIOS_MEMORY_ERROR_TYPE_MULTI_BIT),
    SIP(L"Nibble", SMBIOS_MEMORY_ERROR_TYPE_NIBBLE),
    SIP(L"Checksum", SMBIOS_MEMORY_ERROR_TYPE_CHECKSUM),
    SIP(L"CRC", SMBIOS_MEMORY_ERROR_TYPE_CRC),
    SIP(L"Corrected singled-bit", SMBIOS_MEMORY_ERROR_TYPE_CORRECTED_SINGLE_BIT),
    SIP(L"Corrected", SMBIOS_MEMORY_ERROR_TYPE_CORRECTED),
    SIP(L"Uncorrectable", SMBIOS_MEMORY_ERROR_TYPE_UNCORRECTABLE),
};

static const PH_KEY_VALUE_PAIR EtSMBIOSMemoryErrorGranularities[] =
{
    SIP(L"Other", SMBIOS_MEMORY_ERROR_GRANULARITY_OTHER),
    SIP(L"Unknown", SMBIOS_MEMORY_ERROR_GRANULARITY_UNKNOWN),
    SIP(L"Device", SMBIOS_MEMORY_ERROR_GRANULARITY_DEVICE),
    SIP(L"Partition", SMBIOS_MEMORY_ERROR_GRANULARITY_PARTITION),
};

static const PH_KEY_VALUE_PAIR EtSMBIOSMemoryErrorOperations[] =
{
    SIP(L"Other", SMBIOS_MEMORY_ERROR_OPERATION_OTHER),
    SIP(L"Unknown", SMBIOS_MEMORY_ERROR_OPERATION_UNKNOWN),
    SIP(L"Read", SMBIOS_MEMORY_ERROR_OPERATION_READ),
    SIP(L"Write", SMBIOS_MEMORY_ERROR_OPERATION_WRITE),
    SIP(L"Partial write", SMBIOS_MEMORY_ERROR_OPERATION_PARTIAL_WRITE),
};

ULONG EtAddSMBIOSGroup(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ PCWSTR Name
    )
{
    ULONG group = Context->GroupCounter++;
    PhAddListViewGroup(Context->ListViewHandle, group, Name);
    return group;
}

VOID EtAddSMBIOSItem(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    )
{
    ULONG index = Context->IndexCounter++;
    PhAddListViewGroupItem(Context->ListViewHandle, Group, index, Name, NULL);
    PhSetListViewSubItem(Context->ListViewHandle, index, 1, Value);
}

VOID EtAddSMBIOSBoolean(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ BOOLEAN Value
    )
{
    EtAddSMBIOSItem(Context, Group, Name, Value ? L"true" : L"false");
}

VOID EtAddSMBIOSUInt32(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG Value
    )
{
    WCHAR buffer[PH_INT32_STR_LEN_1];

    PhPrintUInt32(buffer, Value);
    EtAddSMBIOSItem(Context, Group, Name, buffer);
}

VOID EtAddSMBIOSUInt32IX(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG Value
    )
{
    PH_FORMAT format[2];
    PPH_STRING string;

    PhInitFormatS(&format[0], L"0x");
    PhInitFormatX(&format[1], Value);
    string = PhFormat(format, 2, 10);
    EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAddSMBIOSUInt64(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG64 Value
    )
{
    PPH_STRING string;

    string = PhFormatUInt64(Value, FALSE);
    EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAddSMBIOSUInt64IX(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG64 Value
    )
{
    PH_FORMAT format[2];
    PPH_STRING string;

    PhInitFormatS(&format[0], L"0x");
    PhInitFormatI64X(&format[1], Value);
    string = PhFormat(format, 2, 10);
    EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAddSMBIOSString(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR Index
    )
{
    PPH_STRING string;

    if (NT_SUCCESS(PhGetSMBIOSString(EnumHandle, Index, &string)))
    {
        EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
        PhDereferenceObject(string);
    }
    else
    {
        EtAddSMBIOSItem(Context, Group, Name, L"INVALID STRING INDEX");
    }
}

VOID EtAddSMBIOSSize(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG64 Size
    )
{
    PPH_STRING string;

    string = PhFormatSize(Size, ULONG_MAX);
    EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAddSMBIOSUInt32Units(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG Value,
    _In_ PCWSTR Suffix
    )
{
    PH_FORMAT format[2];
    PPH_STRING string;

    PhInitFormatU(&format[0], Value);
    PhInitFormatS(&format[1], Suffix);
    string = PhFormat(format, 2, 10);
    EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAddSMBIOSFlags(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_opt_ const PH_ACCESS_ENTRY* Entries,
    _In_ ULONG Count,
    _In_ ULONG Value
    )
{
    if (Value)
    {
        PH_FORMAT format[4];
        PPH_STRING string;

        string = PhGetAccessString(Value, (PPH_ACCESS_ENTRY)Entries, Count);

        PhInitFormatSR(&format[0], string->sr);
        PhInitFormatS(&format[1], L" (0x");
        PhInitFormatX(&format[2], Value);
        PhInitFormatS(&format[3], L")");

        PhMoveReference(&string, PhFormat(format, 4, 10));

        EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
    }
    else
    {
        EtAddSMBIOSItem(Context, Group, Name, L"");
    }
}

VOID EtAddSMBIOSEnum(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ PCPH_KEY_VALUE_PAIR Values,
    _In_ ULONG SizeOfValues,
    _In_ ULONG Value
    )
{
    PCWSTR string;

    if (PhFindStringSiKeyValuePairs(Values, SizeOfValues, Value, &string))
        EtAddSMBIOSItem(Context, Group, Name, string);
    else
        EtAddSMBIOSItem(Context, Group, Name, L"Undefined");
}

#define ET_SMBIOS_GROUP(n)              ULONG group = EtAddSMBIOSGroup(Context, n)
#define ET_SMBIOS_BOOLEAN(n, v)         EtAddSMBIOSBoolean(Context, group, n, v)
#define ET_SMBIOS_UINT32(n, v)          EtAddSMBIOSUInt32(Context, group, n, v)
#define ET_SMBIOS_UINT32IX(n, v)        EtAddSMBIOSUInt32IX(Context, group, n, v)
#define ET_SMBIOS_UINT64(n, v)          EtAddSMBIOSUInt64(Context, group, n, v)
#define ET_SMBIOS_UINT64IX(n, v)        EtAddSMBIOSUInt64IX(Context, group, n, v)
#define ET_SMBIOS_STRING(n, v)          EtAddSMBIOSString(Context, group, n, EnumHandle, v)
#define ET_SMBIOS_SIZE(n, v)            EtAddSMBIOSSize(Context, group, n, v)
#define ET_SMBIOS_UINT32_UNITS(n, v, u) EtAddSMBIOSUInt32Units(Context, group, n, v, u)
#define ET_SMBIOS_FLAG(x, n)            { TEXT(#x), x, FALSE, FALSE, n }
#define ET_SMBIOS_FLAGS(n, v, f)        EtAddSMBIOSFlags(Context, group, n, f, RTL_NUMBER_OF(f), v)
#define ET_SMBIOS_ENUM(n, v, e)         EtAddSMBIOSEnum(Context, group, n, e, sizeof(e), v);

VOID EtSMBIOSFirmware(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_ACCESS_ENTRY characteristics[] =
    {
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_NOT_SUPPORTED, L"Not supported"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_ISA_SUPPORTED, L"ISA"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_MCA_SUPPORTED, L"MCA"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_EISA_SUPPORTED, L"EISA"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_PCI_SUPPORTED, L"PCI"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_PCMCIA_SUPPORTED, L"PCMCIA"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_PNP_SUPPORTED, L"PNP"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_APM_SUPPORTED, L"APM"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_UPGRADE_SUPPORTED, L"Upgradeable"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_SHADOWING_SUPPORTED, L"Shadowing"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_VL_VESA_SUPPORTED, L"VL-VESA"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_ESCD_SUPPORTED, L"ESCD"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_BOOT_FROM_CD_SUPPORTED, L"Boot from CD"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_SELECTABLE_BOOT_SUPPORTED, L"Selectable boot"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_ROM_SOCKETED, L"ROM socketed"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_PCMCIA_BOOT_SUPPORTED, L"PCMCIA boot"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_EDD_SUPPORTED, L"EDD"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_FLOPPY_NEC_9800_SUPPORTED, L"NEC 9800 floppy"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_FLOPPY_TOSHIBA_SUPPORTED, L"Toshiba floppy"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_FLOPPY_5_25_360KB_SUPPORTED, L"5.25\" 360KB floppy"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_FLOPPY_5_25_1_2_MB_SUPPORTED, L"5.25\" 1.5MB floppy"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_FLOPPY_3_5_720KB_SUPPORTED, L"3.5\" 720KM floppy"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_FLOPPY_3_5_2_88MB_SUPPORTED, L"3.5\" 2.88MB floppy"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_PRINT_SCREEN_SUPPORTED, L"Print screen"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_8042_KEYBOARD_SUPPORTED, L"8020 keyboard"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_SERIAL_SUPPORTED, L"Serial"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_PRINTER_SUPPORTED, L"Printer"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_CGA_VIDEO_SUPPORTED, L"CGA video"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_FIRMWARE_FLAG_NEC_PC_98, L"NEC PC-98"),
        // Upper bits are reserved for platform and system
    };

    static const PH_ACCESS_ENTRY characteristics2[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_ACPI_SUPPORTED, L"ACPI"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_USB_LEGACY_SUPPORTED, L"USB legacy"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_AGP_SUPPORTED, L"AGP"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_I20_BOOT_SUPPORTED, L"I20 boot"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_LS_120_BOOT_SUPPORTED, L"LS-120 boot"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_ZIP_BOOT_SUPPORTED, L"ZIP boot"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_1394_BOOT_SUPPORTED, L"1394 boot"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_SMART_BATTERY_SUPPORTED, L"Smart battery"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_BIOS_BOOT_SUPPORTED, L"BIOS boot"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_FN_KEY_NET_BOOT_SUPPORTED, L"Function key network boot"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_CONTENT_DISTRIBUTION_SUPPORTED, L"Content distribution"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_UEFI_SUPPORTED, L"UEFI"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_MANUFACTURING_MODE_ENABLED, L"Manufacturing mode"),
    };

    ET_SMBIOS_GROUP(L"Firmware");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Firmware, Vendor))
        ET_SMBIOS_STRING(L"Vendor", Entry->Firmware.Vendor);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Firmware, Version))
        ET_SMBIOS_STRING(L"Version", Entry->Firmware.Version);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, StartingAddressSegment) &&
        Entry->Firmware.StartingAddressSegment != 0)
    {
        ET_SMBIOS_UINT32IX(L"Starting address segment", Entry->Firmware.StartingAddressSegment);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Firmware, ReleaseDate))
        ET_SMBIOS_STRING(L"Release date", Entry->Firmware.ReleaseDate);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, RomSize))
    {
        if (Entry->Firmware.RomSize == UCHAR_MAX)
        {
            if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, RomSize2))
            {
                if (Entry->Firmware.RomSize2.Unit == SMBIOS_FIRMWARE_ROM_UNIT_MB)
                    ET_SMBIOS_SIZE(L"ROM size", (ULONG64)Entry->Firmware.RomSize * 0x100000);
                else if (Entry->Firmware.RomSize2.Unit == SMBIOS_FIRMWARE_ROM_UNIT_GB)
                    ET_SMBIOS_SIZE(L"ROM size", (ULONG64)Entry->Firmware.RomSize * 0x40000000);
            }
        }
        else
        {
            ET_SMBIOS_SIZE(L"ROM size", (ULONG64)(Entry->Firmware.RomSize + 1) * 0x10000);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, Characteristics))
        ET_SMBIOS_FLAGS(L"Characteristics", (ULONG)Entry->Firmware.Characteristics, characteristics);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, Characteristics2))
        ET_SMBIOS_FLAGS(L"Characteristics extended", Entry->Firmware.Characteristics2, characteristics2);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, MajorRelease))
        ET_SMBIOS_UINT32(L"Major release", Entry->Firmware.MajorRelease);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, MinorRelease))
        ET_SMBIOS_UINT32(L"Minor release", Entry->Firmware.MinorRelease);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, ControllerMajorRelease) &&
        Entry->Firmware.ControllerMajorRelease != UCHAR_MAX)
    {
        ET_SMBIOS_UINT32(L"Controller major release", Entry->Firmware.ControllerMajorRelease);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Firmware, ControllerMinorRelease) &&
        Entry->Firmware.ControllerMinorRelease != UCHAR_MAX)
    {
        ET_SMBIOS_UINT32(L"Controller major release", Entry->Firmware.ControllerMinorRelease);
    }

}

VOID EtSMBIOSSystem(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    const PH_KEY_VALUE_PAIR wakeUpTimes[] =
    {
        SIP(L"Reserved", SMBIOS_SYSTEM_WAKE_UP_TYPE_RESERVED),
        SIP(L"Other", SMBIOS_SYSTEM_WAKE_UP_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_SYSTEM_WAKE_UP_UNKNOWN),
        SIP(L"APM timer", SMBIOS_SYSTEM_WAKE_UP_APM_TIMER),
        SIP(L"Modem ring", SMBIOS_SYSTEM_WAKE_UP_MODEM_RING),
        SIP(L"LAN remote", SMBIOS_SYSTEM_WAKE_UP_LAN_REMOTE),
        SIP(L"Power switch", SMBIOS_SYSTEM_WAKE_UP_POWER_SWITCH),
        SIP(L"PCI PME", SMBIOS_SYSTEM_WAKE_UP_PCI_PME),
        SIP(L"AC power restored", SMBIOS_SYSTEM_WAKE_UP_AC_POWER_RESTORED),
    };

    ET_SMBIOS_GROUP(L"System");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->System.Manufacturer);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, ProductName))
        ET_SMBIOS_STRING(L"Product name", Entry->System.ProductName);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, Version))
        ET_SMBIOS_STRING(L"Version", Entry->System.Version);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, SerialNumber))
        ET_SMBIOS_STRING(L"Serial number", Entry->System.SerialNumber);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, System, UniqueID))
    {
        PPH_STRING string;

        string = PhFormatGuid(&Entry->System.UniqueID);
        EtAddSMBIOSItem(Context, group, L"UUID", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, System, WakeUpType))
        ET_SMBIOS_ENUM(L"Wake-up type", Entry->System.WakeUpType, wakeUpTimes);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, SKUNumber))
        ET_SMBIOS_STRING(L"SKU number", Entry->System.SKUNumber);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, System, Family))
        ET_SMBIOS_STRING(L"Family", Entry->System.Family);

}

VOID EtSMBIOSBaseboard(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_ACCESS_ENTRY features[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_BASEBOARD_FEATURE_HOSTING_BOARD, L"Hosting"),
        ET_SMBIOS_FLAG(SMBIOS_BASEBOARD_FEATURE_REQUIRES_DAUGHTER_BOARD, L"Daughter"),
        ET_SMBIOS_FLAG(SMBIOS_BASEBOARD_FEATURE_REMOVABLE_BOARD, L"Removable"),
        ET_SMBIOS_FLAG(SMBIOS_BASEBOARD_FEATURE_REPLACEABLE_BOARD, L"Replaceable"),
        ET_SMBIOS_FLAG(SMBIOS_BASEBOARD_FEATURE_HOT_SWAP_BOARD, L"Hot swappable"),
    };

    static const PH_KEY_VALUE_PAIR boardTypes[] =
    {
        SIP(L"Unknown", SMBIOS_BASEBOARD_TYPE_UNKNOWN),
        SIP( L"Other", SMBIOS_BASEBOARD_TYPE_OTHER),
        SIP(L"Server blade", SMBIOS_BASEBOARD_TYPE_SERVER_BLADE),
        SIP(L"Connectivity switch", SMBIOS_BASEBOARD_TYPE_CONNECTIVITY_SWITCH),
        SIP(L"System management module", SMBIOS_BASEBOARD_TYPE_SYSTEM_MANAGEMENT_MODULE),
        SIP(L"Processor module", SMBIOS_BASEBOARD_TYPE_PROCESSOR_MODULE),
        SIP(L"I/O module", SMBIOS_BASEBOARD_TYPE_IO_MODULE),
        SIP(L"Memory module", SMBIOS_BASEBOARD_TYPE_MEMORY_MODULE),
        SIP(L"Daughter board", SMBIOS_BASEBOARD_TYPE_DAUGHTER_BOARD),
        SIP(L"Motherboard", SMBIOS_BASEBOARD_TYPE_MOTHERBOARD),
        SIP(L"Processor memory module", SMBIOS_BASEBOARD_TYPE_PROCESSOR_MEMORY_MODULE),
        SIP(L"Processor I/O module", SMBIOS_BASEBOARD_TYPE_PROCESSOR_IO_MODULE),
        SIP(L"Interconnect", SMBIOS_BASEBOARD_TYPE_INTERCONNECT),
    };

    ET_SMBIOS_GROUP(L"Baseboard");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->Baseboard.Manufacturer);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Product))
        ET_SMBIOS_STRING(L"Product", Entry->Baseboard.Product);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Version))
        ET_SMBIOS_STRING(L"Version", Entry->Baseboard.Version);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, SerialNumber))
        ET_SMBIOS_STRING(L"Serial number", Entry->Baseboard.SerialNumber);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, AssetTag))
        ET_SMBIOS_STRING(L"Asset tag", Entry->Baseboard.AssetTag);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Baseboard, Features))
        ET_SMBIOS_FLAGS(L"Features", Entry->Baseboard.Features, features);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Baseboard, Location))
        ET_SMBIOS_STRING(L"Location", Entry->Baseboard.Location);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Baseboard, ChassisHandle))
        ET_SMBIOS_UINT32IX(L"Chassis handle", Entry->Baseboard.ChassisHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Baseboard, BoardType))
        ET_SMBIOS_ENUM(L"Board type", Entry->Baseboard.BoardType, boardTypes);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Baseboard, NumberOfHandles) &&
        Entry->Baseboard.NumberOfHandles > 0)
    {
        PH_STRING_BUILDER sb;
        PPH_STRING string;

        PhInitializeStringBuilder(&sb, 10);

        for (ULONG i = 0; i < Entry->Baseboard.NumberOfHandles; i++)
        {
            WCHAR buffer[PH_PTR_STR_LEN_1];

            PhPrintUInt32IX(buffer, Entry->Baseboard.Handles[i]);
            PhAppendStringBuilder2(&sb, L"0x");
            PhAppendStringBuilder2(&sb, buffer);
            PhAppendStringBuilder2(&sb, L", ");
        }

        if (PhEndsWithString2(sb.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&sb, 2);

        string = PhFinalStringBuilderString(&sb);

        EtAddSMBIOSItem(Context, group, L"Handles", PhGetString(string));

        PhDereferenceObject(string);
    }
}

VOID EtSMBIOSChassis(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR chassisTypes[] =
    {
        SIP(L"Other", SMBIOS_CHASSIS_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_CHASSIS_TYPE_UNKNOWN),
        SIP(L"Desktop", SMBIOS_CHASSIS_TYPE_DESKTOP),
        SIP(L"Low-profile desktop", SMBIOS_CHASSIS_TYPE_LOW_PROFILE_DESKTOP),
        SIP(L"Pizza box", SMBIOS_CHASSIS_TYPE_PIZZA_BOX),
        SIP(L"Mini tower", SMBIOS_CHASSIS_TYPE_MINI_TOWER),
        SIP(L"Tower", SMBIOS_CHASSIS_TYPE_TOWER),
        SIP(L"Portable", SMBIOS_CHASSIS_TYPE_PORTABLE),
        SIP(L"Laptop", SMBIOS_CHASSIS_TYPE_LAPTOP),
        SIP(L"Notebook", SMBIOS_CHASSIS_TYPE_NOTEBOOK),
        SIP(L"Hand held", SMBIOS_CHASSIS_TYPE_HAND_HELD),
        SIP(L"Docking station", SMBIOS_CHASSIS_TYPE_DOCKING_STATION),
        SIP(L"All-in-one", SMBIOS_CHASSIS_TYPE_ALL_IN_ONE),
        SIP(L"Sub notebook", SMBIOS_CHASSIS_TYPE_SUB_NOTEBOOK),
        SIP(L"Space saving", SMBIOS_CHASSIS_TYPE_SPACE_SAVING),
        SIP(L"Lunch box", SMBIOS_CHASSIS_TYPE_LUNCH_BOX),
        SIP(L"Main server", SMBIOS_CHASSIS_TYPE_MAIN_SERVER),
        SIP(L"Expansion", SMBIOS_CHASSIS_TYPE_EXPANSION),
        SIP(L"Sub chassis", SMBIOS_CHASSIS_TYPE_SUB),
        SIP(L"Bus expansion", SMBIOS_CHASSIS_TYPE_BUS_EXPANSION),
        SIP(L"Peripheral", SMBIOS_CHASSIS_TYPE_PERIPHERAL),
        SIP(L"RAID", SMBIOS_CHASSIS_TYPE_RAID),
        SIP(L"Rack mount", SMBIOS_CHASSIS_TYPE_RACK_MOUNT),
        SIP(L"Sealed-case", SMBIOS_CHASSIS_TYPE_SEALED_CASE_PC),
        SIP(L"Multi-system", SMBIOS_CHASSIS_TYPE_MULTI_SYSTEM),
        SIP(L"Compact PCI", SMBIOS_CHASSIS_TYPE_COMPACT_PCI),
        SIP(L"Advanced TCA", SMBIOS_CHASSIS_TYPE_ADVANCED_TCA),
        SIP(L"Blade", SMBIOS_CHASSIS_TYPE_BLADE),
        SIP(L"Blade enclosure", SMBIOS_CHASSIS_TYPE_BLADE_ENCLOSURE),
        SIP(L"Tablet", SMBIOS_CHASSIS_TYPE_TABLET),
        SIP(L"Convertible", SMBIOS_CHASSIS_TYPE_CONVERTIBLE),
        SIP(L"Detachable", SMBIOS_CHASSIS_TYPE_DETACHABLE),
        SIP(L"Gateway", SMBIOS_CHASSIS_TYPE_IOT_GATEWAY),
        SIP(L"Embedded", SMBIOS_CHASSIS_TYPE_EMBEDDED_PC),
        SIP(L"Mini", SMBIOS_CHASSIS_TYPE_MINI_PC),
        SIP(L"Sick", SMBIOS_CHASSIS_TYPE_STICK_PC),
    };

    static const PH_KEY_VALUE_PAIR chassisStates[] =
    {
        SIP(L"Other", SMBIOS_CHASSIS_STATE_OTHER),
        SIP(L"Unknown", SMBIOS_CHASSIS_STATE_UNKNOWN),
        SIP(L"Safe", SMBIOS_CHASSIS_STATE_SAFE),
        SIP(L"Warning", SMBIOS_CHASSIS_STATE_WARNING),
        SIP(L"Critical", SMBIOS_CHASSIS_STATE_CRITICAL),
        SIP(L"Non-recoverable", SMBIOS_CHASSIS_STATE_NON_RECOVERABLE),
    };

    static const PH_KEY_VALUE_PAIR securityStates[] =
    {
        SIP(L"Other", SMBIOS_CHASSIS_SECURITY_STATE_OTHER),
        SIP(L"Unknown", SMBIOS_CHASSIS_SECURITY_STATE_UNKNOWN),
        SIP(L"None", SMBIOS_CHASSIS_SECURITY_STATE_NONE),
        SIP(L"Locked out", SMBIOS_CHASSIS_SECURITY_STATE_LOCKED_OUT),
        SIP(L"Enabled", SMBIOS_CHASSIS_SECURITY_STATE_ENABLED),
    };

    ET_SMBIOS_GROUP(L"Chassis");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Chassis, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->Chassis.Manufacturer);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, Chassis))
    {
        ET_SMBIOS_ENUM(L"Type", Entry->Chassis.Chassis.Type, chassisTypes);
        ET_SMBIOS_BOOLEAN(L"Locked", !!Entry->Chassis.Chassis.Locked);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Chassis, Version))
        ET_SMBIOS_STRING(L"Version", Entry->Chassis.Version);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Chassis, SerialNumber))
        ET_SMBIOS_STRING(L"Serial number", Entry->Chassis.SerialNumber);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Chassis, AssetTag))
        ET_SMBIOS_STRING(L"Asset tag", Entry->Chassis.AssetTag);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, BootUpState))
        ET_SMBIOS_ENUM(L"Boot-up state", Entry->Chassis.BootUpState, chassisStates);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, PowerSupplyState))
        ET_SMBIOS_ENUM(L"Power supply state", Entry->Chassis.PowerSupplyState, chassisStates);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, ThermalState))
        ET_SMBIOS_ENUM(L"Thermal state", Entry->Chassis.ThermalState, chassisStates);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, SecurityState))
        ET_SMBIOS_ENUM(L"Security state", Entry->Chassis.SecurityState, securityStates);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, OEMDefined) &&
        Entry->Chassis.OEMDefined != 0)
    {
        ET_SMBIOS_UINT32IX(L"OEM defined", Entry->Chassis.OEMDefined);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, Height) &&
        Entry->Chassis.Height != 0)
    {
        ET_SMBIOS_UINT32_UNITS(L"Height", Entry->Chassis.Height, L"U");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, NumberOfPowerCords) &&
        Entry->Chassis.NumberOfPowerCords != 0)
    {
        ET_SMBIOS_UINT32(L"Number of power cords", Entry->Chassis.NumberOfPowerCords);
    }

    // TODO contained elements - SMBIOS_CHASSIS_CONTAINED_ELEMENT

    if (!PH_SMBIOS_CONTAINS_FIELD(Entry, Chassis, ElementLength))
        return;

    // PSMBIOS_CHASSIS_INFORMATION_EX

    ULONG length;
    PSMBIOS_CHASSIS_INFORMATION_EX extended = NULL;

    length = RTL_SIZEOF_THROUGH_FIELD(SMBIOS_CHASSIS_INFORMATION, ElementLength);
    length += Entry->Chassis.ElementCount * Entry->Chassis.ElementLength;

    if (Entry->Header.Length <= length)
        return;

    extended = PTR_ADD_OFFSET(Entry, length);

    if (Entry->Header.Length >= (length + RTL_SIZEOF_THROUGH_FIELD(SMBIOS_CHASSIS_INFORMATION_EX, SKUNumber)))
        ET_SMBIOS_STRING(L"SKU number", extended->SKUNumber);
}

VOID EtSMBIOSProcessor(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR processorTypes[] =
    {
        SIP(L"Other", SMBIOS_PROCESSOR_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_PROCESSOR_TYPE_UNKNOWN),
        SIP(L"Central", SMBIOS_PROCESSOR_TYPE_CENTRAL),
        SIP(L"Math", SMBIOS_PROCESSOR_TYPE_MATH),
        SIP(L"DSP", SMBIOS_PROCESSOR_TYPE_DSP),
        SIP(L"Video", SMBIOS_PROCESSOR_TYPE_VIDEO),
    };

    static const PH_KEY_VALUE_PAIR processorStatus[] =
    {
        SIP(L"Unknown", SMBIOS_PROCESSOR_STATUS_UNKNOWN),
        SIP(L"Enabled", SMBIOS_PROCESSOR_STATUS_ENABLED),
        SIP(L"Disabled by user", SMBIOS_PROCESSOR_STATUS_DISABLED_BY_USER),
        SIP(L"Disabled by firmware", SMBIOS_PROCESSOR_STATUS_DISABLED_BY_FIRMWARE),
        SIP(L"Idle", SMBIOS_PROCESSOR_STATUS_IDLE),
        SIP(L"Other", SMBIOS_PROCESSOR_STATUS_OTHER),
    };

    static const PH_ACCESS_ENTRY characteristics[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_UNKNOWN, L"Unknown"),
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_64_BIT_CAPABLE, L"64-bit capable"),
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_MILT_CORE, L"Multi-core"),
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_HARDWARE_THREADED, L"Hardware threaded"),
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_EXECUTE_PROTECTION, L"Execute protection"),
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_ENHANCED_VIRTUALIZATION, L"Enhanced virtualization"),
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_POWER_PERFORMANCE_CONTROL, L"Power performance control"),
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_128_BIT_CAPABLE, L"128-bit capable"),
        ET_SMBIOS_FLAG(SMBIOS_PROCESSOR_FLAG_ARM64_SOC, L"ARM64 SOC"),
    };

    ET_SMBIOS_GROUP(L"Processor");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, SocketDesignation))
        ET_SMBIOS_STRING(L"Socket designation", Entry->Processor.SocketDesignation);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, Type))
        ET_SMBIOS_ENUM(L"Type", Entry->Processor.Type, processorTypes);

    //if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, Family)
    //    ET_SMBIOS_ENUM(L"Family", Entry->Processor.Family, processorFamilies);
    //    ET_SMBIOS_ENUM(L"Family", Entry->Processor.Family2, processorFamilies);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->Processor.Manufacturer);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, Identifier))
        ET_SMBIOS_UINT64IX(L"Identifier", Entry->Processor.Identifier);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, Version))
        ET_SMBIOS_STRING(L"Version", Entry->Processor.Version);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, Voltage))
    {
        PH_STRING_BUILDER sb;
        PPH_STRING string;

        PhInitializeStringBuilder(&sb, 10);

        if (Entry->Processor.Voltage.Capable2900mV)
            PhAppendStringBuilder2(&sb, L"2.9V, ");
        if (Entry->Processor.Voltage.Capable3500mV)
            PhAppendStringBuilder2(&sb, L"3.5V, ");
        if (Entry->Processor.Voltage.Capable5000mV)
            PhAppendStringBuilder2(&sb, L"5V, ");
        if (!Entry->Processor.Voltage.Mode)
            PhAppendStringBuilder2(&sb, L"legacy");

        if (PhEndsWithString2(sb.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&sb, 2);

        string = PhFinalStringBuilderString(&sb);

        if (string->Length)
            EtAddSMBIOSItem(Context, group, L"Voltage", PhGetString(string));

        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, ExternalClock) &&
        Entry->Processor.ExternalClock != 0)
    {
        ET_SMBIOS_UINT32_UNITS(L"External clock", Entry->Processor.ExternalClock, L" MHz");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, MaxSpeed) &&
        Entry->Processor.MaxSpeed != 0)
    {
        ET_SMBIOS_UINT32_UNITS(L"Max speed", Entry->Processor.MaxSpeed, L" MHz");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, CurrentSpeed) &&
        Entry->Processor.CurrentSpeed != 0)
    {
        ET_SMBIOS_UINT32_UNITS(L"Current speed", Entry->Processor.CurrentSpeed, L" MHz");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, Status))
    {
        ET_SMBIOS_BOOLEAN(L"Populated", !!Entry->Processor.Status.Populated);
        ET_SMBIOS_ENUM(L"Status", Entry->Processor.Status.Status, processorStatus);
    }

    //if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, Upgrade))
    //    ET_SMBIOS_ENUM(L"Upgrade", Entry->Processor.Upgrade, processorUpgrade);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, L1CacheHandle))
        ET_SMBIOS_UINT32IX(L"L1 cache handle", Entry->Processor.L1CacheHandle);
    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, L2CacheHandle))
        ET_SMBIOS_UINT32IX(L"L2 cache handle", Entry->Processor.L2CacheHandle);
    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, L3CacheHandle))
        ET_SMBIOS_UINT32IX(L"L3 cache handle", Entry->Processor.L3CacheHandle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, SerialNumber))
        ET_SMBIOS_STRING(L"Serial number", Entry->Processor.SerialNumber);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, AssetTag))
        ET_SMBIOS_STRING(L"Asset tag", Entry->Processor.AssetTag);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, PartNumber))
        ET_SMBIOS_STRING(L"Part number", Entry->Processor.PartNumber);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, Characteristics))
        ET_SMBIOS_FLAGS(L"Characteristics", Entry->Processor.Characteristics, characteristics);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, SocketType))
        ET_SMBIOS_STRING(L"Socket type", Entry->Processor.SocketType);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, CoreCount))
    {
        if (Entry->Processor.CoreCount == 0)
        {
            NOTHING; // undefined
        }
        else if (Entry->Processor.CoreCount == UCHAR_MAX)
        {
            if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, CoreCount2))
                ET_SMBIOS_UINT32(L"Core count", Entry->Processor.CoreCount2);
        }
        else
        {
            ET_SMBIOS_UINT32(L"Core count", Entry->Processor.CoreCount);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, CoresEnabled))
    {
        if (Entry->Processor.CoresEnabled == 0)
        {
            NOTHING; // undefined
        }
        else if (Entry->Processor.CoresEnabled == UCHAR_MAX)
        {
            if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, CoresEnabled2))
                ET_SMBIOS_UINT32(L"Cores enabled", Entry->Processor.CoresEnabled2);
        }
        else
        {
            ET_SMBIOS_UINT32(L"Cores enabled", Entry->Processor.CoresEnabled);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, ThreadCount))
    {
        if (Entry->Processor.ThreadCount == 0)
        {
            NOTHING; // undefined
        }
        else if (Entry->Processor.ThreadCount == UCHAR_MAX)
        {
            if (PH_SMBIOS_CONTAINS_STRING(Entry, Processor, ThreadCount2))
                ET_SMBIOS_UINT32(L"Thread count", Entry->Processor.ThreadCount2);
        }
        else
        {
            ET_SMBIOS_UINT32(L"Thread count", Entry->Processor.ThreadCount2);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, ThreadsEnabled))
        ET_SMBIOS_UINT32(L"Threads enabled", Entry->Processor.ThreadsEnabled);
}

VOID EtSMBIOSMemoryController(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR errorDetections[] =
    {
        SIP(L"Other", SMBIOS_MEMORY_CONTROLLER_ERROR_DETECTION_OTHER),
        SIP(L"Unknown", SMBIOS_MEMORY_CONTROLLER_ERROR_DETECTION_UNKNOWN),
        SIP(L"None", SMBIOS_MEMORY_CONTROLLER_ERROR_DETECTION_NONE),
        SIP(L"8-bit parity", SMBIOS_MEMORY_CONTROLLER_ERROR_DETECTION_8_BIT_PARITY),
        SIP(L"32-bit ECC", SMBIOS_MEMORY_CONTROLLER_ERROR_DETECTION_32_BIT_ECC),
        SIP(L"64-bit ECC", SMBIOS_MEMORY_CONTROLLER_ERROR_DETECTION_64_BIT_ECC),
        SIP(L"128-bit ECC", SMBIOS_MEMORY_CONTROLLER_ERROR_DETECTION_128_BIT_ECC),
        SIP(L"CRC", SMBIOS_MEMORY_CONTROLLER_ERROR_DETECTION_CRC),
    };

    static const PH_KEY_VALUE_PAIR errorCorrections[] =
    {
        SIP(L"Other", SMBIOS_MEMORY_CONTROLLER_ERROR_CORRECTION_OTHER),
        SIP(L"Unknown", SMBIOS_MEMORY_CONTROLLER_ERROR_CORRECTION_UNKNOWN),
        SIP(L"Single-bit", SMBIOS_MEMORY_CONTROLLER_ERROR_CORRECTION_SINGLE_BIT),
        SIP(L"Double-bit", SMBIOS_MEMORY_CONTROLLER_ERROR_CORRECTION_DOUBLE_BIT),
        SIP(L"Scrubbing", SMBIOS_MEMORY_CONTROLLER_ERROR_CORRECTION_SCRUBBING),
    };

    static const PH_KEY_VALUE_PAIR interleave[] =
    {
        SIP(L"Other", SMBIOS_MEMORY_CONTROLLER_INTERLEAVE_OTHER),
        SIP(L"Unknown", SMBIOS_MEMORY_CONTROLLER_INTERLEAVE_UNKNOWN),
        SIP(L"One-way", SMBIOS_MEMORY_CONTROLLER_INTERLEAVE_ONE_WAY),
        SIP(L"Two-way", SMBIOS_MEMORY_CONTROLLER_INTERLEAVE_TWO_WAY),
        SIP(L"Four-way", SMBIOS_MEMORY_CONTROLLER_INTERLEAVE_FOUR_WAY),
        SIP(L"Eight-way", SMBIOS_MEMORY_CONTROLLER_INTERLEAVE_EIGHT_WAY),
        SIP(L"Sixteen-way", SMBIOS_MEMORY_CONTROLLER_INTERLEAVE_SIXTEEN_WAY),
    };

    static const PH_ACCESS_ENTRY speeds[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_CONTROLLER_SPEEDS_OTHER, L"Other"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_CONTROLLER_SPEEDS_UNKNOWN, L"Unknown"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_CONTROLLER_SPEEDS_70NS, L"70ns"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_CONTROLLER_SPEEDS_60NS, L"60ns"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_CONTROLLER_SPEEDS_50NS, L"50ns"),
    };

    static const PH_ACCESS_ENTRY supportedTypes[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_OTHER, L"Other"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_UNKNOWN, L"Unknown"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_STANDARD, L"Standard"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_FAST_PAGE_MODE, L"Fast-page mode"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_EDO, L"EDO"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_PARITY, L"Parity"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_ECC, L"ECC"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_SIMM, L"SIMM"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_DIMM, L"DIMM"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_BURST_EDO, L"Burst EDO"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_SDRAM, L"SDRAM"),
    };

    ET_SMBIOS_GROUP(L"Memory controller");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, ErrorDetectionMethod))
        ET_SMBIOS_ENUM(L"Error detection method", Entry->MemoryController.ErrorDetectionMethod, errorDetections);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, ErrorCorrectionCapabilities))
        ET_SMBIOS_ENUM(L"Error correction capabilities", Entry->MemoryController.ErrorCorrectionCapabilities, errorCorrections);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, SupportedInterleave))
        ET_SMBIOS_ENUM(L"Supported interleave", Entry->MemoryController.SupportedInterleave, interleave);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, MaximumModuleSize))
        ET_SMBIOS_SIZE(L"Maximum module size", 1ULL < Entry->MemoryController.MaximumModuleSize);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, SupportedSpeeds))
        ET_SMBIOS_FLAGS(L"Supported speeds", Entry->MemoryController.SupportedSpeeds, speeds);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, SupportedTypes))
        ET_SMBIOS_FLAGS(L"Supported types", Entry->MemoryController.SupportedTypes, supportedTypes);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, ModuleVoltage))
    {
        PH_STRING_BUILDER sb;
        PPH_STRING string;

        PhInitializeStringBuilder(&sb, 10);

        if (Entry->MemoryController.ModuleVoltage.Requires2900mV)
            PhAppendStringBuilder2(&sb, L"2.9V, ");
        if (Entry->MemoryController.ModuleVoltage.Requires3500mV)
            PhAppendStringBuilder2(&sb, L"3.5V, ");
        if (Entry->MemoryController.ModuleVoltage.Requires5000mV)
            PhAppendStringBuilder2(&sb, L"5V, ");

        if (PhEndsWithString2(sb.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&sb, 2);

        string = PhFinalStringBuilderString(&sb);

        if (string->Length)
            EtAddSMBIOSItem(Context, group, L"Module voltage", PhGetString(string));

        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, NumberOfSlots) &&
        Entry->MemoryController.NumberOfSlots > 0)
    {
        PH_STRING_BUILDER sb;
        PPH_STRING string;

        PhInitializeStringBuilder(&sb, 10);

        for (ULONG i = 0; i < Entry->MemoryController.NumberOfSlots; i++)
        {
            WCHAR buffer[PH_PTR_STR_LEN_1];

            PhPrintUInt32IX(buffer, Entry->MemoryController.SlotHandles[i]);
            PhAppendStringBuilder2(&sb, L"0x");
            PhAppendStringBuilder2(&sb, buffer);
            PhAppendStringBuilder2(&sb, L", ");
        }

        if (PhEndsWithString2(sb.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&sb, 2);

        string = PhFinalStringBuilderString(&sb);

        EtAddSMBIOSItem(Context, group, L"Handles", PhGetString(string));

        PhDereferenceObject(string);
    }

    if (!PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryController, NumberOfSlots))
        return;

    // SMBIOS_MEMORY_CONTROLLER_INFORMATION_EX

    ULONG length;
    PSMBIOS_MEMORY_CONTROLLER_INFORMATION_EX extended = NULL;

    length = RTL_SIZEOF_THROUGH_FIELD(SMBIOS_MEMORY_CONTROLLER_INFORMATION, NumberOfSlots);
    length += RTL_FIELD_SIZE(SMBIOS_MEMORY_CONTROLLER_INFORMATION, SlotHandles) * Entry->MemoryController.NumberOfSlots;

    if (Entry->Header.Length <= length)
        return;

    extended = PTR_ADD_OFFSET(Entry, length);

    if (Entry->Header.Length >= (length + RTL_SIZEOF_THROUGH_FIELD(SMBIOS_MEMORY_CONTROLLER_INFORMATION_EX, EnabledErrorCorrectionCapabilities)))
        ET_SMBIOS_ENUM(L"Enabled error correction capabilities", extended->EnabledErrorCorrectionCapabilities, errorCorrections);
}

VOID EtSMBIOSMemoryModule(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_ACCESS_ENTRY types[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_OTHER, L"Other"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_UNKNOWN, L"Unknown"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_STANDARD, L"Standard"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_FAST_PAGE_MODE, L"Fast-page mode"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_EDO, L"EDO"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_PARITY, L"Parity"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_ECC, L"ECC"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_SIMM, L"SIMM"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_DIMM, L"DIMM"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_BURST_EDO, L"Burst EDO"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_MODULE_TYPE_SDRAM, L"SDRAM"),
    };

    ET_SMBIOS_GROUP(L"Memory module");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, MemoryModule, SocketDesignation))
        ET_SMBIOS_STRING(L"Socket designation", Entry->MemoryModule.SocketDesignation);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryModule, BankConnections))
        ET_SMBIOS_UINT32IX(L"Bank connections", Entry->MemoryModule.BankConnections);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryModule, CurrentSpeed))
        ET_SMBIOS_UINT32_UNITS(L"Current speed", Entry->MemoryModule.CurrentSpeed, L" ns");

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryModule, MemoryType))
        ET_SMBIOS_FLAGS(L"Memory type", Entry->MemoryModule.MemoryType, types);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryModule, InstalledSize))
    {
        PPH_STRING string;

        if (Entry->MemoryModule.InstalledSize.Size == SMBIOS_MEMORY_MODULE_SIZE_VALUE_NOT_DETERMINABLE)
            string = PhCreateString(L"Not determinable");
        else if (Entry->MemoryModule.InstalledSize.Size == SMBIOS_MEMORY_MODULE_SIZE_VALUE_NOT_ENABLED)
            string = PhCreateString(L"Not enabled");
        else if (Entry->MemoryModule.InstalledSize.Size == SMBIOS_MEMORY_MODULE_SIZE_VALUE_NOT_INSTALLED)
            string = PhCreateString(L"Not installed");
        else
            string = PhFormatSize(1ULL << Entry->MemoryModule.InstalledSize.Size, ULONG_MAX);

        if (Entry->MemoryModule.InstalledSize.DoubleBank)
            PhMoveReference(&string, PhConcatStringRefZ(&string->sr, L", double-bank"));

        EtAddSMBIOSItem(Context, group, L"Installed size", PhGetString(string));

        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryModule, EnabledSize))
    {
        PPH_STRING string;

        if (Entry->MemoryModule.EnabledSize.Size == SMBIOS_MEMORY_MODULE_SIZE_VALUE_NOT_DETERMINABLE)
            string = PhCreateString(L"Not determinable");
        else if (Entry->MemoryModule.EnabledSize.Size == SMBIOS_MEMORY_MODULE_SIZE_VALUE_NOT_ENABLED)
            string = PhCreateString(L"Not enabled");
        else if (Entry->MemoryModule.EnabledSize.Size == SMBIOS_MEMORY_MODULE_SIZE_VALUE_NOT_INSTALLED)
            string = PhCreateString(L"Not installed");
        else
            string = PhFormatSize(1ULL << Entry->MemoryModule.EnabledSize.Size, ULONG_MAX);

        if (Entry->MemoryModule.EnabledSize.DoubleBank)
            PhMoveReference(&string, PhConcatStringRefZ(&string->sr, L", double-bank"));

        EtAddSMBIOSItem(Context, group, L"Enabled size", PhGetString(string));

        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryModule, ErrorStatus) &&
        Entry->MemoryModule.ErrorStatus.Value != 0)
    {
        PH_STRING_BUILDER sb;
        PPH_STRING string;

        PhInitializeStringBuilder(&sb, 10);

        if (Entry->MemoryModule.ErrorStatus.UncorrectableErrors)
            PhAppendStringBuilder2(&sb, L"Uncorrectable errors");
        if (Entry->MemoryModule.ErrorStatus.CorrectableErrors)
            PhAppendStringBuilder2(&sb, L"Correctable errors");
        if (Entry->MemoryModule.ErrorStatus.SeeEventLog)
            PhAppendStringBuilder2(&sb, L"See event log");

        if (PhEndsWithString2(sb.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&sb, 2);

        string = PhFinalStringBuilderString(&sb);

        EtAddSMBIOSItem(Context, group, L"Error status", PhGetString(string));

        PhDereferenceObject(string);
    }
}

VOID EtSMBIOSCache(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR locations[] =
    {
        SIP(L"Internal", SMBIOS_CACHE_LOCATION_INTERNAL),
        SIP(L"External", SMBIOS_CACHE_LOCATION_EXTERNAL),
        SIP(L"Reserved", SMBIOS_CACHE_LOCATION_RESERVED),
        SIP(L"Unknown", SMBIOS_CACHE_LOCATION_UNKNOWN),
    };

    static const PH_KEY_VALUE_PAIR modes[] =
    {
        SIP(L"Write through", SMBIOS_CACHE_MODE_WRITE_THROUGH),
        SIP(L"Write back", SMBIOS_CACHE_MODE_WRITE_BACK),
        SIP(L"Write with memory address", SMBIOS_CACHE_MODE_VARIES_WITH_MEMORY_ADDRESS),
        SIP(L"Unknown", SMBIOS_CACHE_MODE_UNKNOWN),
    };

    static const PH_ACCESS_ENTRY supportSRAM[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_CACHE_SUPPORTED_SRAM_OTHER, L"Other"),
        ET_SMBIOS_FLAG(SMBIOS_CACHE_SUPPORTED_SRAM_UNKNOWN, L"Unknown"),
        ET_SMBIOS_FLAG(SMBIOS_CACHE_SUPPORTED_SRAM_NON_BURST, L"Non-burst"),
        ET_SMBIOS_FLAG(SMBIOS_CACHE_SUPPORTED_SRAM_BURST, L"Burst"),
        ET_SMBIOS_FLAG(SMBIOS_CACHE_SUPPORTED_SRAM_PIPELINE_BURST, L"Pipeline burst"),
        ET_SMBIOS_FLAG(SMBIOS_CACHE_SUPPORTED_SRAM_SYNCHRONOUS, L"Synchronous"),
        ET_SMBIOS_FLAG(SMBIOS_CACHE_SUPPORTED_SRAM_ASYNCHRONOUS, L"Asynchronous"),
    };

    static const PH_KEY_VALUE_PAIR errorCorrection[] =
    {
        SIP(L"Other", SMBIOS_CACHE_ERROR_CORRECTION_OTHER),
        SIP(L"Unknown", SMBIOS_CACHE_ERROR_CORRECTION_UNKNOWN),
        SIP(L"None", SMBIOS_CACHE_ERROR_CORRECTION_NONE),
        SIP(L"Parity", SMBIOS_CACHE_ERROR_CORRECTION_PARITY),
        SIP(L"Single-bit ECC", SMBIOS_CACHE_ERROR_CORRECTION_SINGLE_BIT_ECC),
        SIP(L"Multi-bit ECC", SMBIOS_CACHE_ERROR_CORRECTION_MULTI_BIT_ECC),
    };

    static const PH_KEY_VALUE_PAIR systemCache[] =
    {
        SIP(L"Other", SMBIOS_CACHE_SYSTEM_CACHE_OTHER),
        SIP(L"Unknown", SMBIOS_CACHE_SYSTEM_CACHE_UNKNOWN),
        SIP(L"Instruction", SMBIOS_CACHE_SYSTEM_CACHE_INSTRUCTION),
        SIP(L"Data", SMBIOS_CACHE_SYSTEM_CACHE_DATA),
        SIP(L"Unified", SMBIOS_CACHE_SYSTEM_CACHE_UNIFIED),
    };

    static const PH_KEY_VALUE_PAIR associativity[] =
    {
        SIP(L"Other", SMBIOS_CACHE_ASSOCIATIVITY_OTHER),
        SIP(L"Unknown", SMBIOS_CACHE_ASSOCIATIVITY_UNKNOWN),
        SIP(L"Direct mapped", SMBIOS_CACHE_ASSOCIATIVITY_DIRECT_MAPPED),
        SIP(L"2-way", SMBIOS_CACHE_ASSOCIATIVITY_2_WAY),
        SIP(L"4-way", SMBIOS_CACHE_ASSOCIATIVITY_4_WAY),
        SIP(L"Full", SMBIOS_CACHE_ASSOCIATIVITY_FULL),
        SIP(L"8-way", SMBIOS_CACHE_ASSOCIATIVITY_8_WAY),
        SIP(L"16-way", SMBIOS_CACHE_ASSOCIATIVITY_16_WAY),
        SIP(L"12-way", SMBIOS_CACHE_ASSOCIATIVITY_12_WAY),
        SIP(L"24-way", SMBIOS_CACHE_ASSOCIATIVITY_24_WAY),
        SIP(L"32-way", SMBIOS_CACHE_ASSOCIATIVITY_32_WAY),
        SIP(L"48-way", SMBIOS_CACHE_ASSOCIATIVITY_48_WAY),
        SIP(L"64-way", SMBIOS_CACHE_ASSOCIATIVITY_64_WAY),
        SIP(L"20-way", SMBIOS_CACHE_ASSOCIATIVITY_20_WAY),
    };

    ET_SMBIOS_GROUP(L"Cache");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, Cache, SocketDesignation))
        ET_SMBIOS_STRING(L"Socket designation", Entry->Cache.SocketDesignation);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, Configuration))
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatC(&format[0], L'L');
        PhInitFormatU(&format[1], Entry->Cache.Configuration.Level + 1);

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Level", PhGetString(string));
        PhDereferenceObject(string);

        ET_SMBIOS_BOOLEAN(L"Socketed", !!Entry->Cache.Configuration.Socketed);
        ET_SMBIOS_ENUM(L"Location", Entry->Cache.Configuration.Location, locations);
        ET_SMBIOS_BOOLEAN(L"Enabled", !!Entry->Cache.Configuration.Enabled);
        ET_SMBIOS_ENUM(L"Mode", Entry->Cache.Configuration.Mode, modes);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, MaximumSize))
    {
        ULONG64 size;

        if (Entry->Cache.MaximumSize.Value == MAXUSHORT &&
            PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, MaximumSize2))
        {
            if (Entry->Cache.MaximumSize2.Granularity)
                size = (ULONG64)Entry->Cache.MaximumSize2.Size * 0x10000;
            else
                size = (ULONG64)Entry->Cache.MaximumSize2.Size * 0x400;
        }
        else
        {
            if (Entry->Cache.MaximumSize.Granularity)
                size = (ULONG64)Entry->Cache.MaximumSize.Size * 0x10000;
            else
                size = (ULONG64)Entry->Cache.MaximumSize.Size * 0x400;
        }

        ET_SMBIOS_SIZE(L"Maximum size", size);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, InstalledSize))
    {
        ULONG64 size;

        if (Entry->Cache.InstalledSize.Value == MAXUSHORT &&
            PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, InstalledSize2))
        {
            if (Entry->Cache.InstalledSize2.Granularity)
                size = (ULONG64)Entry->Cache.InstalledSize2.Size * 0x10000;
            else
                size = (ULONG64)Entry->Cache.InstalledSize2.Size * 0x400;
        }
        else
        {
            if (Entry->Cache.InstalledSize.Granularity)
                size = (ULONG64)Entry->Cache.InstalledSize.Size * 0x10000;
            else
                size = (ULONG64)Entry->Cache.InstalledSize.Size * 0x400;
        }

        ET_SMBIOS_SIZE(L"Installed size", size);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, SupportedSRAM))
        ET_SMBIOS_FLAGS(L"Supported SRAM", Entry->Cache.SupportedSRAM, supportSRAM);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, CurrentSRAM))
        ET_SMBIOS_FLAGS(L"Current SRAM", Entry->Cache.CurrentSRAM, supportSRAM);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, Speed) &&
        Entry->Cache.Speed != 0)
    {
        ET_SMBIOS_UINT32_UNITS(L"Speed", Entry->Cache.Speed, L" ns");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, ErrorCorrectionType))
        ET_SMBIOS_ENUM(L"Error correction type", Entry->Cache.ErrorCorrectionType, errorCorrection);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, SystemCacheType))
        ET_SMBIOS_ENUM(L"System cache type", Entry->Cache.SystemCacheType, systemCache);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, Associativity))
        ET_SMBIOS_ENUM(L"Associativity", Entry->Cache.Associativity, associativity);
}

VOID EtSMBIOSPortConnector(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"None", SMBIOS_PORT_CONNECTOR_TYPE_NONE),
        SIP(L"Centronics", SMBIOS_PORT_CONNECTOR_TYPE_CENTRONICS),
        SIP(L"Mini centronics", SMBIOS_PORT_CONNECTOR_TYPE_MINI_CENTRONICS),
        SIP(L"Proprietary", SMBIOS_PORT_CONNECTOR_TYPE_PROPRIETARY),
        SIP(L"DB-25 pin male", SMBIOS_PORT_CONNECTOR_TYPE_DB_25_PIN_MALE),
        SIP(L"DB-25 pin female", SMBIOS_PORT_CONNECTOR_TYPE_DB_25_PIN_FEMALE),
        SIP(L"DB-15 pin male", SMBIOS_PORT_CONNECTOR_TYPE_DB_15_PIN_MALE),
        SIP(L"DB-15 pin female", SMBIOS_PORT_CONNECTOR_TYPE_DB_15_PIN_FEMALE),
        SIP(L"DB-9 pin male", SMBIOS_PORT_CONNECTOR_TYPE_DB_9_PIN_MALE),
        SIP(L"DB-9 pin female", SMBIOS_PORT_CONNECTOR_TYPE_DB_9_PIN_FEMALE),
        SIP(L"RJ-11", SMBIOS_PORT_CONNECTOR_TYPE_RJ_11),
        SIP(L"RJ-54", SMBIOS_PORT_CONNECTOR_TYPE_RJ_54),
        SIP(L"50-pin miniSCSI", SMBIOS_PORT_CONNECTOR_TYPE_50_PIN_MINI_SCSI),
        SIP(L"Mini-DIN", SMBIOS_PORT_CONNECTOR_TYPE_MINI_DIN),
        SIP(L"Micro-DIN", SMBIOS_PORT_CONNECTOR_TYPE_MICRO_DIN),
        SIP(L"PS/2", SMBIOS_PORT_CONNECTOR_TYPE_PS2),
        SIP(L"Infrared", SMBIOS_PORT_CONNECTOR_TYPE_INFRARED),
        SIP(L"HP-HIL", SMBIOS_PORT_CONNECTOR_TYPE_HP_HIL),
        SIP(L"USB", SMBIOS_PORT_CONNECTOR_TYPE_USB),
        SIP(L"SSA-SCSI", SMBIOS_PORT_CONNECTOR_TYPE_SSA_SCSI),
        SIP(L"Circular DIN-8 male", SMBIOS_PORT_CONNECTOR_TYPE_CIRCULAR_DIN_8_MALE),
        SIP(L"Circular WIN-8 female", SMBIOS_PORT_CONNECTOR_TYPE_CIRCULAR_DIN_8_FEMALE),
        SIP(L"On board IDE", SMBIOS_PORT_CONNECTOR_TYPE_ON_BOARD_IDE),
        SIP(L"On board floppy", SMBIOS_PORT_CONNECTOR_TYPE_ON_BOARD_FLOPPY),
        SIP(L"9-pin dual inline", SMBIOS_PORT_CONNECTOR_TYPE_9_PIN_DIAL_INLINE),
        SIP(L"25-pin dual inline", SMBIOS_PORT_CONNECTOR_TYPE_25_PIN_DIAL_INLINE),
        SIP(L"50-pin dial inline", SMBIOS_PORT_CONNECTOR_TYPE_50_PIN_DIAL_INLINE),
        SIP(L"68-pin dual inline", SMBIOS_PORT_CONNECTOR_TYPE_68_PIN_DIAL_INLINE),
        SIP(L"On board input CD-ROM", SMBIOS_PORT_CONNECTOR_TYPE_ON_BOARD_INPUT_CD_ROM),
        SIP(L"Mini-centronics type-14", SMBIOS_PORT_CONNECTOR_TYPE_MINI_CENTRONICS_TYPE_14),
        SIP(L"Mini-centronics type-16", SMBIOS_PORT_CONNECTOR_TYPE_MINI_CENTRONICS_TYPE_26),
        SIP(L"Mini-jack", SMBIOS_PORT_CONNECTOR_TYPE_MINI_JACK),
        SIP(L"BNC", SMBIOS_PORT_CONNECTOR_TYPE_BNC),
        SIP(L"1394", SMBIOS_PORT_CONNECTOR_TYPE_1394),
        SIP(L"SAS/SATA", SMBIOS_PORT_CONNECTOR_TYPE_SAS_SATA),
        SIP(L"USB-C", SMBIOS_PORT_CONNECTOR_TYPE_USB_TYPE_C),
        SIP(L"PC-98", SMBIOS_PORT_CONNECTOR_TYPE_PC_98),
        SIP(L"PC-98 hireso", SMBIOS_PORT_CONNECTOR_TYPE_PC_98_HIRESO),
        SIP(L"PC-H98", SMBIOS_PORT_CONNECTOR_TYPE_PC_H98),
        SIP(L"PC-98 note", SMBIOS_PORT_CONNECTOR_TYPE_PC_98_NOTE),
        SIP(L"PC-98 full", SMBIOS_PORT_CONNECTOR_TYPE_PC_98_FULL),
        SIP(L"Other", SMBIOS_PORT_CONNECTOR_TYPE_OTHER),
    };

    static const PH_KEY_VALUE_PAIR portTypes[] =
    {
        SIP(L"None", SMBIOS_PORT_CONNECTOR_PORT_TYPE_NONE),
        SIP(L"Parallel port XT/AT", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PARALLEL_XT_AT),
        SIP(L"Parallel port PS/2", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PARALLEL_PS2),
        SIP(L"Parallel port ECP", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PARALLEL_ECP),
        SIP(L"Parallel port EPP", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PARALLEL_EPP),
        SIP(L"Parallel port ECP/EPP", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PARALLEL_ECP_EPP),
        SIP(L"Serial port XT/AT", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SERIAL_XT_AT),
        SIP(L"Serial port 16450", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SERIAL_16450),
        SIP(L"Serial port 16550", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SERIAL_16550),
        SIP(L"Serial port 16550A", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SERIAL_16550A),
        SIP(L"SCSI", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SCSI),
        SIP(L"MIDI", SMBIOS_PORT_CONNECTOR_PORT_TYPE_MIDI),
        SIP(L"Joy stick", SMBIOS_PORT_CONNECTOR_PORT_TYPE_JOY_STICK),
        SIP(L"Keyboard", SMBIOS_PORT_CONNECTOR_PORT_TYPE_KEYBOARD),
        SIP(L"Mouse", SMBIOS_PORT_CONNECTOR_PORT_TYPE_MOUSE),
        SIP(L"SSA SCSI", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SSA_SCSI),
        SIP(L"USB", SMBIOS_PORT_CONNECTOR_PORT_TYPE_USB),
        SIP(L"FireWire", SMBIOS_PORT_CONNECTOR_PORT_TYPE_FIRE_WIRE),
        SIP(L"PCMCIA type I", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PCMCIA_TYPE_I ),
        SIP(L"PCMCIA type II", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PCMCIA_TYPE_II),
        SIP(L"PCMCIA type III", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PCMCIA_TYPE_III),
        SIP(L"Card bus", SMBIOS_PORT_CONNECTOR_PORT_TYPE_CARD_BUS),
        SIP(L"Access bus", SMBIOS_PORT_CONNECTOR_PORT_TYPE_ACCESS_BUS),
        SIP(L"SCSI II", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SCSI_II),
        SIP(L"SCSI wide", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SCSI_WIDE),
        SIP(L"PC-98", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PC_98),
        SIP(L"PC-98-Hireso", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PC_98_HIRESO),
        SIP(L"PC-H98", SMBIOS_PORT_CONNECTOR_PORT_TYPE_PC_98H),
        SIP(L"Video", SMBIOS_PORT_CONNECTOR_PORT_TYPE_VIDEO),
        SIP(L"Audio", SMBIOS_PORT_CONNECTOR_PORT_TYPE_AUDIO),
        SIP(L"Modem", SMBIOS_PORT_CONNECTOR_PORT_TYPE_MODEM),
        SIP(L"Network", SMBIOS_PORT_CONNECTOR_PORT_TYPE_NETWORK),
        SIP(L"SATA", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SATA),
        SIP(L"SAS", SMBIOS_PORT_CONNECTOR_PORT_TYPE_SAS),
        SIP(L"MFDP (Display port)", SMBIOS_PORT_CONNECTOR_PORT_TYPE_MFDP),
        SIP(L"Thunderbolt", SMBIOS_PORT_CONNECTOR_PORT_TYPE_THUNDERBOLT),
        SIP(L"8251", SMBIOS_PORT_CONNECTOR_PORT_TYPE_8251),
        SIP(L"8251 FIFO", SMBIOS_PORT_CONNECTOR_PORT_TYPE_8251_FIFO),
        SIP(L"Other", SMBIOS_PORT_CONNECTOR_PORT_TYPE_8251_OTHER),
    };

    ET_SMBIOS_GROUP(L"Port connector");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortConnector, InternalReferenceDesignator))
        ET_SMBIOS_STRING(L"Internal reference designator", Entry->PortConnector.InternalReferenceDesignator);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortConnector, InternalConnectorType))
        ET_SMBIOS_ENUM(L"Internal connector type", Entry->PortConnector.InternalConnectorType, types);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortConnector, ExternalReferenceDesignator))
        ET_SMBIOS_STRING(L"External reference designator", Entry->PortConnector.ExternalReferenceDesignator);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortConnector, ExternalConnectorType))
        ET_SMBIOS_ENUM(L"External reference type", Entry->PortConnector.ExternalConnectorType, types);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortConnector, PortType))
        ET_SMBIOS_ENUM(L"Port type", Entry->PortConnector.PortType, portTypes);
}

VOID EtSMBIOSSystemSlot(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR slotTypes[] =
    {
        SIP(L"Other", SMBIOS_SYSTEM_SLOT_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_SYSTEM_SLOT_TYPE_UNKNOWN),
        SIP(L"ISA", SMBIOS_SYSTEM_SLOT_TYPE_ISA),
        SIP(L"MCA", SMBIOS_SYSTEM_SLOT_TYPE_MCA),
        SIP(L"EISA", SMBIOS_SYSTEM_SLOT_TYPE_EISA),
        SIP(L"PCI", SMBIOS_SYSTEM_SLOT_TYPE_PCI),
        SIP(L"PCMCIA", SMBIOS_SYSTEM_SLOT_TYPE_PCMCIA),
        SIP(L"VL-VESA", SMBIOS_SYSTEM_SLOT_TYPE_VL_VESA),
        SIP(L"Proprietary", SMBIOS_SYSTEM_SLOT_TYPE_PROPRIETARY),
        SIP(L"Processor card slot", SMBIOS_SYSTEM_SLOT_TYPE_PROCESSOR_CARD_SLOT),
        SIP(L"Memory card slot", SMBIOS_SYSTEM_SLOT_TYPE_PROPRIETARY_MEMORY_CARD_SLOT),
        SIP(L"Riser card slot", SMBIOS_SYSTEM_SLOT_TYPE_IO_RISER_CARD_SLOT),
        SIP(L"NuBus", SMBIOS_SYSTEM_SLOT_TYPE_NUBUS),
        SIP(L"PCI - 66MHz capable", SMBIOS_SYSTEM_SLOT_TYPE_PCI_66MHZ_CAPABLE ),
        SIP(L"AGP", SMBIOS_SYSTEM_SLOT_TYPE_AGP),
        SIP(L"AGP 2X", SMBIOS_SYSTEM_SLOT_TYPE_AGP_2X),
        SIP(L"AGP 4X", SMBIOS_SYSTEM_SLOT_TYPE_AGP_4X),
        SIP(L"PCI-X", SMBIOS_SYSTEM_SLOT_TYPE_PCI_X),
        SIP(L"AGP 8X", SMBIOS_SYSTEM_SLOT_TYPE_AGP_8X),
        SIP(L"M.2 socket 1-DP (A)", SMBIOS_SYSTEM_SLOT_TYPE_M_2_SOCKET_1_DP_MECHANICAL_KEY_A),
        SIP(L"M.2 socket 1-SD (E)", SMBIOS_SYSTEM_SLOT_TYPE_M_2_SOCKET_1_SD_MECHANICAL_KEY_E),
        SIP(L"M.2 socket 2 (B)", SMBIOS_SYSTEM_SLOT_TYPE_M_2_SOCKET_2_MECHANICAL_KEY_B),
        SIP(L"M.2 socket 3 (M)", SMBIOS_SYSTEM_SLOT_TYPE_M_2_SOCKET_3_MECHANICAL_KEY_M),
        SIP(L"MXM type I", SMBIOS_SYSTEM_SLOT_TYPE_MXM_TYPE_I),
        SIP(L"MXM type II", SMBIOS_SYSTEM_SLOT_TYPE_MXM_TYPE_II),
        SIP(L"MXM type III (standard)", SMBIOS_SYSTEM_SLOT_TYPE_MXM_TYPE_III_STANDARD_CONNECTOR),
        SIP(L"MXM type III (HE)", SMBIOS_SYSTEM_SLOT_TYPE_MXM_TYPE_III_HE_CONNECTOR),
        SIP(L"MXM type IV", SMBIOS_SYSTEM_SLOT_TYPE_MXM_TYPE_IV),
        SIP(L"MXM 3.0 type A", SMBIOS_SYSTEM_SLOT_TYPE_MXM_3_0_TYPE_A),
        SIP(L"MXM 3.0 type B", SMBIOS_SYSTEM_SLOT_TYPE_MXM_3_0_TYPE_B),
        SIP(L"PCI express gen 2 SFF-8639 (U.2)", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_2_SFF_8639_U_2),
        SIP(L"PCI express gen 3 SFF-8639 (U.2)", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_3_SFF_8639_U_2),
        SIP(L"PCI express mini 52-pin (CEM spec. 2.0) with keep-outs", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_MINI_52_PIN_CEM_2_0_WITH_KEEP_OUTS),
        SIP(L"PCI express mini 52-pin (CEM spec. 2.0) without keep-outs", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_MINI_52_PIN_CEM_2_0_WITHOUT_KEEP_OUTS),
        SIP(L"PCI Express Mini 76-pin (CEM spec. 2.0)", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_MINI_76_PIN_CEM_2_0),
        SIP(L"PCI express gen 4 SFF-8639 (U.2)", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_4_SFF_8639_U_2),
        SIP(L"PCI express gen 5 SFF-8639 (U.2)", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_5_SFF_8639_U_2),
        SIP(L"OCP NIC 3.0 (SFF)", SMBIOS_SYSTEM_SLOT_TYPE_OCP_NIC_3_0_SMALL_FORM_FACTOR_SFF),
        SIP(L"OCP NIC 3.0 (LFF)", SMBIOS_SYSTEM_SLOT_TYPE_OCP_NIC_3_0_LARGE_FORM_FACTOR_LFF),
        SIP(L"OCP NIC prior to 3.0", SMBIOS_SYSTEM_SLOT_TYPE_OCP_NIC_PRIOR_TO_3_0),
        SIP(L"CXL flexbus 1.0", SMBIOS_SYSTEM_SLOT_TYPE_CXL_FLEXBUS_1_0),
        SIP(L"PC-98/C20", SMBIOS_SYSTEM_SLOT_TYPE_PC_98_C20),
        SIP(L"PC-98/C24", SMBIOS_SYSTEM_SLOT_TYPE_PC_98_C24),
        SIP(L"PC-98/E", SMBIOS_SYSTEM_SLOT_TYPE_PC_98_E),
        SIP(L"PC-98/Local bus", SMBIOS_SYSTEM_SLOT_TYPE_PC_98_LOCAL_BUS),
        SIP(L"PC-98/Card", SMBIOS_SYSTEM_SLOT_TYPE_PC_98_CARD),
        SIP(L"PCI express", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS),
        SIP(L"PCI express x1", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_X1),
        SIP(L"PCI express x2", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_X2),
        SIP(L"PCI express x4", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_X4),
        SIP(L"PCI express x8", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_X8),
        SIP(L"PCI express x16", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_X16),
        SIP(L"PCI express gen 2", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_2),
        SIP(L"PCI express gen 2 x1", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_2_X1),
        SIP(L"PCI express gen 2 x2", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_2_X2),
        SIP(L"PCI express gen 2 x4", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_2_X4),
        SIP(L"PCI express gen 2 x8", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_2_X8),
        SIP(L"PCI express gen 2 x16", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_2_X16),
        SIP(L"PCI express gen 3", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_3),
        SIP(L"PCI express gen 3 x1", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_3_X1),
        SIP(L"PCI express gen 3 x2", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_3_X2),
        SIP(L"PCI express gen 3 x4", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_3_X4),
        SIP(L"PCI express gen 3 x8", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_3_X8),
        SIP(L"PCI express gen 3 x16", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_3_X16),
        SIP(L"PCI express gen 4", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_4),
        SIP(L"PCI express gen 4 x1", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_4_X1),
        SIP(L"PCI express gen 4 x2", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_4_X2),
        SIP(L"PCI express gen 4 x4", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_4_X4),
        SIP(L"PCI express gen 4 x8", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_4_X8),
        SIP(L"PCI express gen 4 x16", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_4_X16),
        SIP(L"PCI express gen 5", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_5),
        SIP(L"PCI express gen 5 x1", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_5_X1),
        SIP(L"PCI express gen 5 x2", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_5_X2),
        SIP(L"PCI express gen 5 x4", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_5_X4),
        SIP(L"PCI express gen 5 x8", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_5_X8),
        SIP(L"PCI express gen 5 x16", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_5_X16),
        SIP(L"PCI express gen 6", SMBIOS_SYSTEM_SLOT_TYPE_PCI_EXPRESS_GEN_6),
        SIP(L"EDSFF E1.S, E1.L", SMBIOS_SYSTEM_SLOT_TYPE_EDSFF_E1_S_E1_L),
        SIP(L"EDSFF E3.S, E3.L", SMBIOS_SYSTEM_SLOT_TYPE_EDSFF_E3_S_E3_L),
    };

    static const PH_KEY_VALUE_PAIR busWidths[] =
    {
        SIP(L"Other", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_OTHER),
        SIP(L"Unknown", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_UNKNOWN),
        SIP(L"8 bit", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_8_BIT),
        SIP(L"16 bit", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_16_BIT),
        SIP(L"32 bit", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_32_BIT),
        SIP(L"64 bit", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_64_BIT),
        SIP(L"128 bit", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_128_BIT),
        SIP(L"1x or 1x", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_1X_OR_1X),
        SIP(L"2x or 2x", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_2X_OR_2X),
        SIP(L"4x or 4x", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_4X_OR_4X),
        SIP(L"8x or 8x", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_8X_OR_8X),
        SIP(L"12x or 12x", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_12X_OR_12X),
        SIP(L"16x or 16x", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_16X_OR_16X),
        SIP(L"32x or 32x", SMBIOS_SYSTEM_SLOT_BUS_WIDTH_32X_OR_32X),
    };

    static const PH_KEY_VALUE_PAIR slotUsages[] =
    {
        SIP(L"Other", SMBIOS_SYSTEM_SLOT_USAGE_OTHER),
        SIP(L"Unknown", SMBIOS_SYSTEM_SLOT_USAGE_UNKNOWN),
        SIP(L"Available", SMBIOS_SYSTEM_SLOT_USAGE_AVAILABLE),
        SIP(L"In use", SMBIOS_SYSTEM_SLOT_USAGE_IN_USE),
        SIP(L"Unavailable", SMBIOS_SYSTEM_SLOT_USAGE_UNAVAILABLE),
    };

    static const PH_KEY_VALUE_PAIR slotLengths[] =
    {
        SIP(L"Other", SMBIOS_SYSTEM_SLOT_LENGTH_OTHER),
        SIP(L"Unknown", SMBIOS_SYSTEM_SLOT_LENGTH_UNKNOWN),
        SIP(L"Short", SMBIOS_SYSTEM_SLOT_LENGTH_SHORT),
        SIP(L"Long", SMBIOS_SYSTEM_SLOT_LENGTH_LONG),
        SIP(L"2.5\" drive", SMBIOS_SYSTEM_SLOT_LENGTH_2_5_DRIVE),
        SIP(L"3.4\" drive", SMBIOS_SYSTEM_SLOT_LENGTH_3_4_DRIVE),
    };

    static const PH_ACCESS_ENTRY slotFlags[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_UNKNOWN, L"Unknown"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_5000MV, L"5 V"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_3300MV, L"3.3 V"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_SHARED, L"Shared"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_CARD_16, L"Card-16"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_CARD_BUS, L"CardBus"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_CARD_ZOOM_VIDEO, L"Zoom video"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_CARD_MODEN_RING_RESUME, L"Modem ring resume"),
    };

    static const PH_ACCESS_ENTRY slotFlags2[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_2_PME_SIGNAL, L"PME signal"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_2_HOT_PLUG, L"Hot-plug"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_2_SMBUS_SIGNAL, L"SMBus signal"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_2_PCIE_BIFURCATION, L"PCIE Bifurcation"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_2_SURPRISE_REMOVAL, L"Surprise removal"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_2_FLEXBUS_CLX_1, L"Flexbus CLX 1"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_2_FLEXBUS_CLX_2, L"Flexbus CLX 2"),
        ET_SMBIOS_FLAG(SMBIOS_SYSTEM_SLOT_FLAG_2_FLEXBUS_CLX_4, L"Flexbus CLX 4"),
    };

    static const PH_KEY_VALUE_PAIR slotHeights[] =
    {
        SIP(L"Not applicable", SMBIOS_SYSTEM_SLOT_HEIGHT_NOT_APPLICABLE),
        SIP(L"Other", SMBIOS_SYSTEM_SLOT_HEIGHT_OTHER),
        SIP(L"Unknown", SMBIOS_SYSTEM_SLOT_HEIGHT_UNKNOWN),
        SIP(L"Full height", SMBIOS_SYSTEM_SLOT_HEIGHT_FULL_HEIGHT),
        SIP(L"Low profile", SMBIOS_SYSTEM_SLOT_HEIGHT_LOW_PROFILE),
    };

    ET_SMBIOS_GROUP(L"System slot");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, SystemSlot, SocketDesignation))
        ET_SMBIOS_STRING(L"Slot designation", Entry->SystemSlot.SocketDesignation);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, Type))
        ET_SMBIOS_ENUM(L"Type", Entry->SystemSlot.Type, slotTypes);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, BusWidth))
        ET_SMBIOS_ENUM(L"Bus width", Entry->SystemSlot.BusWidth, busWidths);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, CurrentUsage))
        ET_SMBIOS_ENUM(L"Current usage", Entry->SystemSlot.CurrentUsage, slotUsages);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, Length))
        ET_SMBIOS_ENUM(L"Length", Entry->SystemSlot.Length, slotLengths);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, Identifier))
        ET_SMBIOS_UINT32IX(L"Identifier", Entry->SystemSlot.Identifier);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, Characteristics))
        ET_SMBIOS_FLAGS(L"Characteristics", Entry->SystemSlot.Characteristics, slotFlags);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, Characteristics2))
        ET_SMBIOS_FLAGS(L"Characteristics 2", Entry->SystemSlot.Characteristics2, slotFlags2);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, SegmentGroup))
        ET_SMBIOS_UINT32(L"Segment group", Entry->SystemSlot.SegmentGroup);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, DeviceFunctionNumber));
    {
        ET_SMBIOS_UINT32(L"Device number", Entry->SystemSlot.DeviceFunctionNumber.DeviceNumber);
        ET_SMBIOS_UINT32(L"Function number", Entry->SystemSlot.DeviceFunctionNumber.FunctionNumber);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, BusWidthBase))
        ET_SMBIOS_UINT32(L"Bus width base", Entry->SystemSlot.BusWidthBase);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, PeerGroupingCount) &&
        Entry->SystemSlot.PeerGroupingCount > 0)
    {
        PH_STRING_BUILDER sb;
        PPH_STRING string;

        PhInitializeStringBuilder(&sb, 10);

        for (ULONG i = 0; i < Entry->SystemSlot.PeerGroupingCount; i++)
        {
            WCHAR buffer[PH_INT32_STR_LEN_1];

            PhPrintUInt32(buffer, Entry->SystemSlot.PeerGroups[i]);
            PhAppendStringBuilder2(&sb, buffer);
            PhAppendStringBuilder2(&sb, L", ");
        }

        if (PhEndsWithString2(sb.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&sb, 2);

        string = PhFinalStringBuilderString(&sb);

        EtAddSMBIOSItem(Context, group, L"Peer groups", PhGetString(string));

        PhDereferenceObject(string);
    }

    if (!PH_SMBIOS_CONTAINS_FIELD(Entry, SystemSlot, PeerGroupingCount))
        return;

    // SMBIOS_SYSTEM_SLOT_INFORMATION_EX

    ULONG length;
    PSMBIOS_SYSTEM_SLOT_INFORMATION_EX extended;

    length = RTL_SIZEOF_THROUGH_FIELD(SMBIOS_SYSTEM_SLOT_INFORMATION, PeerGroupingCount);
    length += RTL_FIELD_SIZE(SMBIOS_SYSTEM_SLOT_INFORMATION, PeerGroups) * Entry->SystemSlot.PeerGroupingCount;

    if (Entry->Header.Length <= length)
        return;

    extended = PTR_ADD_OFFSET(Entry, length);

    if (Entry->Header.Length >= (length + RTL_SIZEOF_THROUGH_FIELD(SMBIOS_SYSTEM_SLOT_INFORMATION_EX, Information)))
        ET_SMBIOS_UINT32(L"Information", extended->Information);

    if (Entry->Header.Length >= (length + RTL_SIZEOF_THROUGH_FIELD(SMBIOS_SYSTEM_SLOT_INFORMATION_EX, PhysicalWidth)))
        ET_SMBIOS_UINT32(L"Physical width", extended->PhysicalWidth);

    if (Entry->Header.Length >= (length + RTL_SIZEOF_THROUGH_FIELD(SMBIOS_SYSTEM_SLOT_INFORMATION_EX, Pitch)))
        ET_SMBIOS_UINT32(L"Physical width", extended->Pitch);

    if (Entry->Header.Length >= (length + RTL_SIZEOF_THROUGH_FIELD(SMBIOS_SYSTEM_SLOT_INFORMATION_EX, Height)))
        ET_SMBIOS_ENUM(L"Height", extended->Height, slotHeights);
}

VOID EtSMBIOSOnBoardDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"Other", SMBIOS_ON_BOARD_DEVICE_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_ON_BOARD_DEVICE_TYPE_UNKNOWN),
        SIP(L"Video", SMBIOS_ON_BOARD_DEVICE_TYPE_VIDEO),
        SIP(L"Controller", SMBIOS_ON_BOARD_DEVICE_TYPE_SCSI_CONTROLLER),
        SIP(L"Ethernet", SMBIOS_ON_BOARD_DEVICE_TYPE_ETHERNET),
        SIP(L"Token ring", SMBIOS_ON_BOARD_DEVICE_TYPE_TOKEN_RING),
        SIP(L"Sound", SMBIOS_ON_BOARD_DEVICE_TYPE_SOUND),
        SIP(L"PATA controller", SMBIOS_ON_BOARD_DEVICE_TYPE_PATA_CONTROLLER),
        SIP(L"SATA controller", SMBIOS_ON_BOARD_DEVICE_TYPE_SATA_CONTROLLER),
        SIP(L"SAS controller", SMBIOS_ON_BOARD_DEVICE_TYPE_SAS_CONTROLLER),
    };

    if (!PH_SMBIOS_CONTAINS_FIELD(Entry, OnBoardDevice, Devices))
        return;

    ULONG count = (Entry->Header.Length - sizeof(SMBIOS_HEADER)) / 2;
    for (ULONG i = 0; i < count; i++)
    {
        ET_SMBIOS_GROUP(L"On board device");

        ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);
        ET_SMBIOS_STRING(L"Description", Entry->OnBoardDevice.Devices[i].Description);
        ET_SMBIOS_ENUM(L"Type", Entry->OnBoardDevice.Devices[i].Device.Type, types);
        ET_SMBIOS_BOOLEAN(L"Enabled", !!Entry->OnBoardDevice.Devices[i].Device.Enabled);
    }
}

VOID EtSMBIOSOemString(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    if (!PH_SMBIOS_CONTAINS_FIELD(Entry, OEMString, Count))
        return;

    ET_SMBIOS_GROUP(L"OEM strings");
    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);
    ET_SMBIOS_UINT32(L"Number of strings", Entry->OEMString.Count);

    for (UCHAR i = 0; i < Entry->OEMString.Count; i++)
    {
        PH_FORMAT format[2];
        PPH_STRING name;
        PPH_STRING string;

        if (!NT_SUCCESS(PhGetSMBIOSString(EnumHandle, i + 1, &string)))
            break;

        PhInitFormatS(&format[0], L"String #");
        PhInitFormatU(&format[1], i + 1);

        name = PhFormat(format, 2, 10);

        EtAddSMBIOSItem(Context, group, PhGetString(name), PhGetString(string));

        PhDereferenceObject(string);
        PhDereferenceObject(name);
    }
}

VOID EtSMBIOSSystemConfigurationOption(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    if (!PH_SMBIOS_CONTAINS_FIELD(Entry, SystemConfigurationOption, Count))
        return;

    ET_SMBIOS_GROUP(L"System configuration options");
    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);
    ET_SMBIOS_UINT32(L"Number of strings", Entry->SystemConfigurationOption.Count);

    for (UCHAR i = 0; i < Entry->OEMString.Count; i++)
    {
        PH_FORMAT format[2];
        PPH_STRING name;
        PPH_STRING string;

        if (!NT_SUCCESS(PhGetSMBIOSString(EnumHandle, i + 1, &string)))
            break;

        PhInitFormatS(&format[0], L"String #");
        PhInitFormatU(&format[1], i + 1);

        name = PhFormat(format, 2, 10);

        EtAddSMBIOSItem(Context, group, PhGetString(name), PhGetString(string));

        PhDereferenceObject(string);
        PhDereferenceObject(name);
    }
}

VOID EtSMBIOSFirmwareLanguage(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Firmware language");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, FirmwareLanguage, InstallableLanguages))
        ET_SMBIOS_UINT32(L"Number of languages available", Entry->FirmwareLanguage.InstallableLanguages);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, FirmwareLanguage, Flags))
        ET_SMBIOS_BOOLEAN(L"Abbreviated format", !!Entry->FirmwareLanguage.Flags.AbbreviatedFormat);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, FirmwareLanguage, CurrentLanguage))
        ET_SMBIOS_STRING(L"Current language", Entry->FirmwareLanguage.CurrentLanguage);
}

VOID EtSMBIOSGroupAssociation(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Group association");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, GroupAssociation, Name))
        ET_SMBIOS_STRING(L"Name", Entry->GroupAssociation.Name);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, GroupAssociation, ItemType))
        ET_SMBIOS_UINT32(L"Item type", Entry->GroupAssociation.ItemType);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, GroupAssociation, ItemHandle))
        ET_SMBIOS_UINT32IX(L"Item handle", Entry->GroupAssociation.ItemHandle);
}

VOID EtSMBIOSSystemEventLog(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System event log");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    // TODO parse the logs, just dumping the meta data for now

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, EventLog, AreaLength))
        ET_SMBIOS_UINT32(L"Area length", Entry->EventLog.AreaLength);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, EventLog, HeaderStartOffset))
        ET_SMBIOS_UINT32IX(L"Header start offset", Entry->EventLog.HeaderStartOffset);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, EventLog, DataStartOffset))
        ET_SMBIOS_UINT32IX(L"Data start off", Entry->EventLog.DataStartOffset);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, EventLog, AccessMethod))
        ET_SMBIOS_UINT32(L"Access method", Entry->EventLog.AccessMethod);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, EventLog, Status))
    {
        ET_SMBIOS_BOOLEAN(L"Full", !!Entry->EventLog.Status.Full);
        ET_SMBIOS_BOOLEAN(L"Valid", !!Entry->EventLog.Status.Valid);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, EventLog, ChangeToken))
        ET_SMBIOS_UINT32(L"Change token", Entry->EventLog.ChangeToken);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, EventLog, NumberOfDescriptors))
        ET_SMBIOS_UINT32(L"Number of descriptors", Entry->EventLog.NumberOfDescriptors);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, EventLog, LengthOfDescriptors))
        ET_SMBIOS_UINT32(L"Length of descriptors", Entry->EventLog.LengthOfDescriptors);
}

VOID EtSMBIOSPhysicalMemoryArray(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR locations[] =
    {
        SIP(L"Other", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_OTHER),
        SIP(L"Unknown", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_UNKNOWN),
        SIP(L"Motherboard", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_MOTHERBOARD),
        SIP(L"ISA", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_ISA),
        SIP(L"EISA", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_EISA),
        SIP(L"PCI", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_PCI),
        SIP(L"MCA", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_MCA),
        SIP(L"PCMCIA", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_PCMCIA),
        SIP(L"Proprietary", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_PROPRIETARY),
        SIP(L"NuBus", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_NUBUS),
        SIP(L"PC-98/C20", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_PC_98_C20),
        SIP(L"PC-98/C24", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_PC_98_C24),
        SIP(L"PC-98/E", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_PC_98_E),
        SIP(L"PC-98/Local bus", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_PC_98_LOCAL_BUS),
        SIP(L"CXL", SMBIOS_PHYSICAL_MEMORY_ARRAY_LOCATION_PC_98_CXL),
    };

    static const PH_KEY_VALUE_PAIR uses[] =
    {
        SIP(L"Other", SMBIOS_PHYSICAL_MEMORY_ARRAY_USE_OTHER),
        SIP(L"Unknown", SMBIOS_PHYSICAL_MEMORY_ARRAY_USE_UNKNOWN),
        SIP(L"System", SMBIOS_PHYSICAL_MEMORY_ARRAY_USE_SYSTEM),
        SIP(L"Video", SMBIOS_PHYSICAL_MEMORY_ARRAY_USE_VIDEO),
        SIP(L"Flash", SMBIOS_PHYSICAL_MEMORY_ARRAY_USE_FLASH),
        SIP(L"Non-volatile", SMBIOS_PHYSICAL_MEMORY_ARRAY_USE_NON_VOLATILE),
        SIP(L"Cache", SMBIOS_PHYSICAL_MEMORY_ARRAY_USE_CACHE),
    };

    static const PH_KEY_VALUE_PAIR corrections[] =
    {
        SIP(L"Other", SMBIOS_PHYSICAL_MEMORY_ARRAY_ERROR_CORRECTION_OTHER),
        SIP(L"Unknown", SMBIOS_PHYSICAL_MEMORY_ARRAY_ERROR_CORRECTION_UNKNOWN),
        SIP(L"None", SMBIOS_PHYSICAL_MEMORY_ARRAY_ERROR_CORRECTION_NONE),
        SIP(L"Parity", SMBIOS_PHYSICAL_MEMORY_ARRAY_ERROR_CORRECTION_PARITY),
        SIP(L"Single-bit ECC", SMBIOS_PHYSICAL_MEMORY_ARRAY_ERROR_CORRECTION_SINGLE_BIT_ECC),
        SIP(L"Multi-bit ECC", SMBIOS_PHYSICAL_MEMORY_ARRAY_ERROR_CORRECTION_MULTI_BIT_ECC),
        SIP(L"CRC", SMBIOS_PHYSICAL_MEMORY_ARRAY_ERROR_CORRECTION_CRC),
    };

    ET_SMBIOS_GROUP(L"Physical memory array");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PhysicalMemoryArray, Location))
        ET_SMBIOS_ENUM(L"Location", Entry->PhysicalMemoryArray.Location, locations);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PhysicalMemoryArray, Use))
        ET_SMBIOS_ENUM(L"Use", Entry->PhysicalMemoryArray.Use, uses);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PhysicalMemoryArray, ErrorCorrection))
        ET_SMBIOS_ENUM(L"Error correction", Entry->PhysicalMemoryArray.ErrorCorrection, corrections);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PhysicalMemoryArray, MaximumCapacity))
    {
        if (Entry->PhysicalMemoryArray.MaximumCapacity == 0x80000000 &&
            PH_SMBIOS_CONTAINS_FIELD(Entry, PhysicalMemoryArray, ExtendedMaximumCapacity))
        {
            ET_SMBIOS_SIZE(L"Maximum capacity", Entry->PhysicalMemoryArray.ExtendedMaximumCapacity);
        }
        else
        {
            ET_SMBIOS_SIZE(L"Maximum capacity", (ULONG64)Entry->PhysicalMemoryArray.MaximumCapacity * 1024);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PhysicalMemoryArray, ErrorInformationHandle))
        ET_SMBIOS_UINT32IX(L"Error information handle", Entry->PhysicalMemoryArray.ErrorInformationHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PhysicalMemoryArray, NumberOfMemoryDevices))
        ET_SMBIOS_UINT32(L"Number of memory devices", Entry->PhysicalMemoryArray.NumberOfMemoryDevices);
}

VOID EtSMBIOSMemoryDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR formFactors[] =
    {
        SIP(L"Other", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_OTHER),
        SIP(L"Unknown", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_UNKNOWN),
        SIP(L"SIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_SIMM),
        SIP(L"SIP", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_SIP),
        SIP(L"Chip", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_CHIP),
        SIP(L"DIP", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_DIP),
        SIP(L"ZIP", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_ZIP),
        SIP(L"Proprietary", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_PROPRIETARY),
        SIP(L"DIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_DIMM),
        SIP(L"TSOP", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_TSOP),
        SIP(L"Row of chips", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_ROW_OF_CHIPS),
        SIP(L"RIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_RIMM),
        SIP(L"SODIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_SODIMM),
        SIP(L"SRIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_SRIMM),
        SIP(L"FB-DIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_FB_DIMM),
        SIP(L"Die", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_DIE),
        SIP(L"CAMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_CAMM),
    };

    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"Other", SMBIOS_MEMORY_DEVICE_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_MEMORY_DEVICE_TYPE_UNKNOWN),
        SIP(L"DRAM", SMBIOS_MEMORY_DEVICE_TYPE_DRAM),
        SIP(L"EDRAM", SMBIOS_MEMORY_DEVICE_TYPE_EDRAM),
        SIP(L"VRAM", SMBIOS_MEMORY_DEVICE_TYPE_VRAM),
        SIP(L"SRAM", SMBIOS_MEMORY_DEVICE_TYPE_SRAM),
        SIP(L"RAM", SMBIOS_MEMORY_DEVICE_TYPE_RAM),
        SIP(L"ROM", SMBIOS_MEMORY_DEVICE_TYPE_ROM),
        SIP(L"FLASH", SMBIOS_MEMORY_DEVICE_TYPE_FLASH),
        SIP(L"EEPROM", SMBIOS_MEMORY_DEVICE_TYPE_EEPROM),
        SIP(L"FEPROM", SMBIOS_MEMORY_DEVICE_TYPE_FEPROM),
        SIP(L"EPROM", SMBIOS_MEMORY_DEVICE_TYPE_EPROM),
        SIP(L"CDRAM", SMBIOS_MEMORY_DEVICE_TYPE_CDRAM),
        SIP(L"3DRAM", SMBIOS_MEMORY_DEVICE_TYPE_3DRAM),
        SIP(L"SDRAM", SMBIOS_MEMORY_DEVICE_TYPE_SDRAM),
        SIP(L"SGRAM", SMBIOS_MEMORY_DEVICE_TYPE_SGRAM),
        SIP(L"RDRAM", SMBIOS_MEMORY_DEVICE_TYPE_RDRAM),
        SIP(L"DDR", SMBIOS_MEMORY_DEVICE_TYPE_DDR),
        SIP(L"DDR2", SMBIOS_MEMORY_DEVICE_TYPE_DDR2),
        SIP(L"DDR2-FM-DIMM", SMBIOS_MEMORY_DEVICE_TYPE_DDR2_FB_DIMM),
        SIP(L"DDR3", SMBIOS_MEMORY_DEVICE_TYPE_DDR3),
        SIP(L"FBD2", SMBIOS_MEMORY_DEVICE_TYPE_FBD2),
        SIP(L"DDR4", SMBIOS_MEMORY_DEVICE_TYPE_DDR4),
        SIP(L"LPDDR", SMBIOS_MEMORY_DEVICE_TYPE_LPDDR),
        SIP(L"LPDDR2", SMBIOS_MEMORY_DEVICE_TYPE_LPDDR2),
        SIP(L"LPDDR3", SMBIOS_MEMORY_DEVICE_TYPE_LPDDR3),
        SIP(L"LPDDR4", SMBIOS_MEMORY_DEVICE_TYPE_LPDDR4),
        SIP(L"Local non-volatile", SMBIOS_MEMORY_DEVICE_TYPE_LOCAL_NON_VOLATILE),
        SIP(L"HBM", SMBIOS_MEMORY_DEVICE_TYPE_HBM),
        SIP(L"HBM2", SMBIOS_MEMORY_DEVICE_TYPE_HBM2),
        SIP(L"DDR5", SMBIOS_MEMORY_DEVICE_TYPE_DDR5),
        SIP(L"LPDDR5", SMBIOS_MEMORY_DEVICE_TYPE_LPDDR5),
        SIP(L"HBM3", SMBIOS_MEMORY_DEVICE_TYPE_HBM3),
    };

    static const PH_ACCESS_ENTRY typeDetails[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_RESERVED, L"Reserved"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_OTHER, L"Other"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_UNKNOWN, L"Unknown"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_FAST_PAGED, L"Fast-paged"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_STATIC_COL, L"Static column"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_PSEUDO_STATIC, L"Pseudo-static"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_RAMBUS, L"RAMBUS"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_SYNCHRONOUS, L"Synchronous"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_CMOS, L"CMOS"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_EDO, L"EDO"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_WINDOW_DRAM, L"Window DRAM"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_CACHE_DRAM, L"Cache DRAM"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_NON_VOLATILE, L"Non-volatile"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_BUFFERED, L"Buffered"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_UNBUFFERED, L"Unbuffered"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_TYPE_DETAIL_LRDIMM, L"LRDIMM"),
    };

    static const PH_KEY_VALUE_PAIR technologies[] =
    {
        SIP(L"Other", SMBIOS_MEMORY_DEVICE_TECHNOLOGY_OTHER),
        SIP(L"Unknown", SMBIOS_MEMORY_DEVICE_TECHNOLOGY_UNKNOWN),
        SIP(L"DRAM", SMBIOS_MEMORY_DEVICE_TECHNOLOGY_DRAM),
        SIP(L"NVDIMM-N", SMBIOS_MEMORY_DEVICE_TECHNOLOGY_NVDIMM_N),
        SIP(L"NVDIMM-F", SMBIOS_MEMORY_DEVICE_TECHNOLOGY_NVDIMM_F),
        SIP(L"NVDIMM-P", SMBIOS_MEMORY_DEVICE_TECHNOLOGY_NVDIMM_P),
        SIP(L"Intel Optane", SMBIOS_MEMORY_DEVICE_TECHNOLOGY_INTEL_OPTANE),
        SIP(L"MRDIMM", SMBIOS_MEMORY_DEVICE_TECHNOLOGY_MRDIMM),
    };

    static const PH_ACCESS_ENTRY deviceModes[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_MODE_RESERVED, L"Reserved"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_MODE_OTHER, L"Other"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_MODE_UNKNOWN, L"Unknown"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_MODE_VOLATILE, L"Volatile"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_MODE_BYTE_ACCESSIBLE_PERSISTENT, L"Byte-accessible persistent"),
        ET_SMBIOS_FLAG(SMBIOS_MEMORY_DEVICE_MODE_BLOCK_ACCESSIBLE_PERSISTENT, L"Block-accessible persistent"),
    };

    ET_SMBIOS_GROUP(L"Memory device");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, PhysicalArrayHandle))
        ET_SMBIOS_UINT32IX(L"Physical array handle", Entry->MemoryDevice.PhysicalArrayHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, MemoryErrorInformationHandle))
        ET_SMBIOS_UINT32IX(L"Memory error information handle", Entry->MemoryDevice.MemoryErrorInformationHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, TotalWidth) &&
        Entry->MemoryDevice.TotalWidth != MAXUSHORT)
    {
        ET_SMBIOS_UINT32(L"Total width", Entry->MemoryDevice.TotalWidth);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, DataWidth) &&
        Entry->MemoryDevice.DataWidth != MAXUSHORT)
    {
        ET_SMBIOS_UINT32(L"Data width", Entry->MemoryDevice.DataWidth);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, Size))
    {
        if (Entry->MemoryDevice.Size.Value == 0)
        {
            NOTHING; // not installed
        }
        else if (Entry->MemoryDevice.Size.Value == 0x7FFF) // extended size
        {
            if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ExtendedSize))
            {
                ET_SMBIOS_SIZE(L"Size", (ULONG64)Entry->MemoryDevice.ExtendedSize * 1024 * 1024);
            }
        }
        else
        {
            if (Entry->MemoryDevice.Size.Granularity) // kilobytes
            {
                ET_SMBIOS_SIZE(L"Size", Entry->MemoryDevice.Size.Size * 1024);
            }
            else // megabytes
            {
                ET_SMBIOS_SIZE(L"Size", (ULONG64)Entry->MemoryDevice.Size.Size * 1024 * 1024);
            }
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, FormFactor))
        ET_SMBIOS_ENUM(L"Form factor", Entry->MemoryDevice.FormFactor, formFactors);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, DeviceSet) &&
        Entry->MemoryDevice.DeviceSet != 0 &&
        Entry->MemoryDevice.DeviceSet != UCHAR_MAX)
    {
        ET_SMBIOS_UINT32(L"Device set", Entry->MemoryDevice.DeviceSet);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, DeviceLocator))
        ET_SMBIOS_STRING(L"Device locator", Entry->MemoryDevice.DeviceLocator);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, BankLocator))
        ET_SMBIOS_STRING(L"Bank locator", Entry->MemoryDevice.BankLocator);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, MemoryType))
        ET_SMBIOS_ENUM(L"Memory type", Entry->MemoryDevice.MemoryType, types);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, TypeDetail) &&
        Entry->MemoryDevice.TypeDetail != 0)
    {
        ET_SMBIOS_FLAGS(L"Type detail", Entry->MemoryDevice.TypeDetail, typeDetails);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, Speed) &&
        Entry->MemoryDevice.Speed != 0)
    {
        if (Entry->MemoryDevice.Speed == MAXUSHORT)
        {
            if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ExtendedSpeed))
            {
                ET_SMBIOS_UINT32_UNITS(L"Speed", Entry->MemoryDevice.ExtendedSpeed, L" MT/s");
            }
        }
        else
        {
            ET_SMBIOS_UINT32_UNITS(L"Speed", Entry->MemoryDevice.Speed, L" MT/s");
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ConfiguredSpeed) &&
        Entry->MemoryDevice.ConfiguredSpeed != 0)
    {
        if (Entry->MemoryDevice.ConfiguredSpeed == MAXUSHORT)
        {
            if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ExtendedConfiguredSpeed))
            {
                ET_SMBIOS_UINT32_UNITS(L"Configured speed", Entry->MemoryDevice.ExtendedConfiguredSpeed, L" MT/s");
            }
        }
        else
        {
            ET_SMBIOS_UINT32_UNITS(L"Configured speed", Entry->MemoryDevice.ConfiguredSpeed, L" MT/s");
        }
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->MemoryDevice.Manufacturer);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, SerialNumber))
        ET_SMBIOS_STRING(L"Serial number", Entry->MemoryDevice.SerialNumber);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, AssetTag))
        ET_SMBIOS_STRING(L"Asset tag", Entry->MemoryDevice.AssetTag);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, PartNumber))
        ET_SMBIOS_STRING(L"Part number", Entry->MemoryDevice.PartNumber);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, Attributes))
    {
        if (Entry->MemoryDevice.Attributes.Rank != 0)
            ET_SMBIOS_UINT32(L"Rank", Entry->MemoryDevice.Attributes.Rank);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, MinimumVoltage) &&
        Entry->MemoryDevice.MinimumVoltage != 0)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->MemoryDevice.MinimumVoltage / 1000, 2);
        PhInitFormatS(&format[1], L" V");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Minimum voltage", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, MaximumVoltage) &&
        Entry->MemoryDevice.MaximumVoltage != 0)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->MemoryDevice.MaximumVoltage / 1000, 2);
        PhInitFormatS(&format[1], L" V");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Maximum voltage", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ConfiguredVoltage) &&
        Entry->MemoryDevice.ConfiguredVoltage != 0)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->MemoryDevice.ConfiguredVoltage / 1000, 2);
        PhInitFormatS(&format[1], L" V");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Configured voltage", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, Technology))
        ET_SMBIOS_ENUM(L"Technology", Entry->MemoryDevice.Technology, technologies);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ModeCapabilities))
        ET_SMBIOS_FLAGS(L"Mode capabilities", Entry->MemoryDevice.ModeCapabilities, deviceModes);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, MemoryDevice, FirmwareVersion))
        ET_SMBIOS_STRING(L"Firmware version", Entry->MemoryDevice.FirmwareVersion);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ModuleManufacturerID) &&
        Entry->MemoryDevice.ModuleManufacturerID != 0)
    {
        ET_SMBIOS_UINT32IX(L"Module manufacturer ID", Entry->MemoryDevice.ModuleManufacturerID);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ModuleProductID) &&
        Entry->MemoryDevice.ModuleProductID != 0)
    {
        ET_SMBIOS_UINT32IX(L"Module product ID", Entry->MemoryDevice.ModuleProductID);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, SubsystemControllerManufacturerID) &&
        Entry->MemoryDevice.SubsystemControllerManufacturerID != 0)
    {
        ET_SMBIOS_UINT32IX(L"Subsystem controller manufacturer ID", Entry->MemoryDevice.SubsystemControllerManufacturerID);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, SubsystemControllerProductID) &&
        Entry->MemoryDevice.SubsystemControllerProductID != 0)
    {
        ET_SMBIOS_UINT32IX(L"Subsystem controller product ID", Entry->MemoryDevice.SubsystemControllerProductID);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, NonVolatileSize) &&
        Entry->MemoryDevice.NonVolatileSize != 0 &&
        Entry->MemoryDevice.NonVolatileSize != ULONG64_MAX)
    {
        ET_SMBIOS_SIZE(L"Non-volatile size", Entry->MemoryDevice.NonVolatileSize);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, VolatileSize) &&
        Entry->MemoryDevice.VolatileSize != 0 &&
        Entry->MemoryDevice.VolatileSize != ULONG64_MAX)
    {
        ET_SMBIOS_SIZE(L"Volatile size", Entry->MemoryDevice.VolatileSize);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, LogicalSize) &&
        Entry->MemoryDevice.LogicalSize != 0 &&
        Entry->MemoryDevice.LogicalSize != ULONG64_MAX)
    {
        ET_SMBIOS_SIZE(L"Logical size", Entry->MemoryDevice.LogicalSize);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, PMIC0ManufacturerID) &&
        Entry->MemoryDevice.PMIC0ManufacturerID != 0)
    {
        ET_SMBIOS_UINT32IX(L"PMIC0 manufacturer ID", Entry->MemoryDevice.PMIC0ManufacturerID);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, PMIC0Revision) &&
        Entry->MemoryDevice.PMIC0Revision != 0xFF00)
    {
        ET_SMBIOS_UINT32IX(L"PMIC0 revision", Entry->MemoryDevice.PMIC0Revision);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, RCDManufacturerID) &&
        Entry->MemoryDevice.RCDManufacturerID != 0)
    {
        ET_SMBIOS_UINT32IX(L"RCD manufacturer ID", Entry->MemoryDevice.RCDManufacturerID);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, RCDRevision) &&
        Entry->MemoryDevice.RCDRevision != 0xFF00)
    {
        ET_SMBIOS_UINT32IX(L"RCD revision", Entry->MemoryDevice.RCDRevision);
    }
}

VOID EtSMBIOS32BitMemoryError(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"32-bit memory error");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError32, Type))
        ET_SMBIOS_ENUM(L"Type", Entry->MemoryError32.Type, EtSMBIOSMemoryErrorTypes);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError32, Granularity))
        ET_SMBIOS_ENUM(L"Granularity", Entry->MemoryError32.Granularity, EtSMBIOSMemoryErrorGranularities);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError32, Operation))
        ET_SMBIOS_ENUM(L"Operation", Entry->MemoryError32.Operation, EtSMBIOSMemoryErrorOperations);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError32, VendorSyndrome) &&
        Entry->MemoryError32.VendorSyndrome != 0)
    {
        ET_SMBIOS_UINT32IX(L"Vendor syndrome", Entry->MemoryError32.VendorSyndrome);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError32, ArrayErrorAddress) &&
        Entry->MemoryError32.ArrayErrorAddress != 0x80000000)
    {
        ET_SMBIOS_UINT32IX(L"Array error address", Entry->MemoryError32.ArrayErrorAddress);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError32, DeviceErrorAddress) &&
        Entry->MemoryError32.DeviceErrorAddress != 0x80000000)
    {
        ET_SMBIOS_UINT32IX(L"Device error address", Entry->MemoryError32.DeviceErrorAddress);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError32, ErrorResolution) &&
        Entry->MemoryError32.ErrorResolution != 0x80000000)
    {
        ET_SMBIOS_UINT32IX(L"Error resolution", Entry->MemoryError32.ErrorResolution);
    }
}

VOID EtSMBIOSMemoryArrayMappedAddress(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Memory array mapped address");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryArrayMappedAddress, StartingAddress))
    {
        if (Entry->MemoryArrayMappedAddress.StartingAddress == ULONG_MAX)
        {
            if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryArrayMappedAddress, ExtendedStartingAddress))
            {
                ET_SMBIOS_UINT64IX(L"Starting address", Entry->MemoryArrayMappedAddress.ExtendedStartingAddress);
            }
        }
        else
        {
            ET_SMBIOS_UINT64IX(L"Starting address", (ULONG64)Entry->MemoryArrayMappedAddress.StartingAddress * 1024);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryArrayMappedAddress, EndingAddress))
    {
        if (Entry->MemoryArrayMappedAddress.EndingAddress == ULONG_MAX)
        {
            if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryArrayMappedAddress, ExtendedEndingAddress))
            {
                ET_SMBIOS_UINT64IX(L"Ending address", Entry->MemoryArrayMappedAddress.ExtendedEndingAddress);
            }
        }
        else
        {
            ET_SMBIOS_UINT64IX(L"Ending address", (ULONG64)Entry->MemoryArrayMappedAddress.EndingAddress * 1024);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryArrayMappedAddress, ArrayHandle))
        ET_SMBIOS_UINT32IX(L"Array handle", Entry->MemoryArrayMappedAddress.ArrayHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryArrayMappedAddress, PartitionWidth))
        ET_SMBIOS_UINT32(L"Partition width", Entry->MemoryArrayMappedAddress.PartitionWidth);
}

VOID EtSMBIOSMemoryDeviceMappedAddress(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Memory device mapped address");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, StartingAddress))
    {
        if (Entry->MemoryDeviceMappedAddress.StartingAddress == ULONG_MAX)
        {
            if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, ExtendedStartingAddress))
            {
                ET_SMBIOS_UINT64IX(L"Starting address", Entry->MemoryDeviceMappedAddress.ExtendedStartingAddress);
            }
        }
        else
        {
            ET_SMBIOS_UINT64IX(L"Starting address", (ULONG64)Entry->MemoryDeviceMappedAddress.StartingAddress * 1024);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, EndingAddress))
    {
        if (Entry->MemoryDeviceMappedAddress.EndingAddress == ULONG_MAX)
        {
            if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, ExtendedEndingAddress))
            {
                ET_SMBIOS_UINT64IX(L"Ending address", Entry->MemoryDeviceMappedAddress.ExtendedEndingAddress);
            }
        }
        else
        {
            ET_SMBIOS_UINT64IX(L"Ending address", (ULONG64)Entry->MemoryDeviceMappedAddress.EndingAddress * 1024);
        }
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, DeviceHandle))
        ET_SMBIOS_UINT32IX(L"Device handle", Entry->MemoryDeviceMappedAddress.DeviceHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, ArrayMappedAddressHandle))
        ET_SMBIOS_UINT32IX(L"Array mapped address handle", Entry->MemoryDeviceMappedAddress.ArrayMappedAddressHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, PartitionRowPosition) &&
        Entry->MemoryDeviceMappedAddress.PartitionRowPosition != 0 &&
        Entry->MemoryDeviceMappedAddress.PartitionRowPosition != UCHAR_MAX)
    {
        ET_SMBIOS_UINT32(L"Partition row position", Entry->MemoryDeviceMappedAddress.PartitionRowPosition);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, InterleavePosition) &&
        Entry->MemoryDeviceMappedAddress.InterleavePosition != 0 &&
        Entry->MemoryDeviceMappedAddress.InterleavePosition != UCHAR_MAX)
    {
        ET_SMBIOS_UINT32(L"Interleave position", Entry->MemoryDeviceMappedAddress.InterleavePosition);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDeviceMappedAddress, InterleavedDataDepth) &&
        Entry->MemoryDeviceMappedAddress.InterleavedDataDepth != 0 &&
        Entry->MemoryDeviceMappedAddress.InterleavedDataDepth != UCHAR_MAX)
    {
        ET_SMBIOS_UINT32(L"Interleave data depth", Entry->MemoryDeviceMappedAddress.InterleavedDataDepth);
    }
}

VOID EtSMBIOSBuiltInPointingDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"Other", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_UNKNOWN),
        SIP(L"Mouse", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_MOUSE),
        SIP(L"Track ball", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_TRACK_BALL),
        SIP(L"Track point", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_TRACK_POINT),
        SIP(L"Glide point", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_GLIDE_POINT),
        SIP(L"Touch pad", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_TOUCH_PAD),
        SIP(L"Touch screen", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_TOUCH_SCREEN),
        SIP(L"Optical sensor", SMBIOS_BUILT_IN_POINTING_DEVICE_TYPE_OPTICAL_SENSOR),
    };

    static const PH_KEY_VALUE_PAIR interfaces[] =
    {
        SIP(L"Other", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_OTHER),
        SIP(L"Unknown", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_UNKNOWN),
        SIP(L"Serial", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_SERIAL),
        SIP(L"PS/2", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_PS2),
        SIP(L"Infrared", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_INFRARED),
        SIP(L"HP-HIL", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_HP_HIL),
        SIP(L"Bus mouse", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_BUS_MOUSE),
        SIP(L"Apple desktop bus", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_ADB),
        SIP(L"Bus mouse DB-9", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_DB_9),
        SIP(L"Bus mouse micro-DIN", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_MICRO_DIN),
        SIP(L"USB", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_USB),
        SIP(L"I2C", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_I2C),
        SIP(L"SPI", SMBIOS_BUILT_IN_POINTING_DEVICE_INTERFACE_SPI),
    };

    ET_SMBIOS_GROUP(L"Built-in pointing device");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, BuiltInPointingDevice, PointerType))
        ET_SMBIOS_ENUM(L"Type", Entry->BuiltInPointingDevice.PointerType, types);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, BuiltInPointingDevice, PointerInterface))
        ET_SMBIOS_ENUM(L"Interface", Entry->BuiltInPointingDevice.PointerInterface, interfaces);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, BuiltInPointingDevice, PointerButtons))
        ET_SMBIOS_UINT32(L"Number of buttons", Entry->BuiltInPointingDevice.PointerButtons);
}

VOID EtSMBIOSPortableBattery(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR chemistry[] =
    {
        SIP(L"Other", SMBIOS_PORTABLE_BATTERY_CHEMISTRY_OTHER),
        SIP(L"Unknown", SMBIOS_PORTABLE_BATTERY_CHEMISTRY_UNKNOWN),
        SIP(L"Lead acid", SMBIOS_PORTABLE_BATTERY_CHEMISTRY_LEAD_ACID),
        SIP(L"Nickel cadmium", SMBIOS_PORTABLE_BATTERY_CHEMISTRY_NICKEL_CADMIUM),
        SIP(L"Nickel metal", SMBIOS_PORTABLE_BATTERY_CHEMISTRY_NICKEL_METAL),
        SIP(L"Lithium-ion", SMBIOS_PORTABLE_BATTERY_CHEMISTRY_LITHIUM_ION),
        SIP(L"Zinc air", SMBIOS_PORTABLE_BATTERY_CHEMISTRY_ZINC_AIR),
        SIP(L"Lithium polymer", SMBIOS_PORTABLE_BATTERY_CHEMISTRY_LITHIUM_POLYMER),
    };

    ET_SMBIOS_GROUP(L"Portable battery");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, Location))
        ET_SMBIOS_STRING(L"Location", Entry->PortableBattery.Location);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->PortableBattery.Manufacturer);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, ManufactureDate))
        ET_SMBIOS_STRING(L"Manufacture date", Entry->PortableBattery.ManufactureDate);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, SerialNumber))
        ET_SMBIOS_STRING(L"Serial number", Entry->PortableBattery.SerialNumber);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, DeviceName))
        ET_SMBIOS_STRING(L"Device name", Entry->PortableBattery.DeviceName);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortableBattery, DeviceChemistry))
        ET_SMBIOS_ENUM(L"Device chemistry", Entry->PortableBattery.DeviceChemistry, chemistry);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortableBattery, DesignCapacity) &&
        Entry->PortableBattery.DesignCapacity != 0)
    {
        ULONG multiplier;
        ULONG capacity;

        if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortableBattery, DesignCapacityMultiplier))
            multiplier = Entry->PortableBattery.DesignCapacityMultiplier;
        else
            multiplier = 1;

        capacity = (ULONG)Entry->PortableBattery.DesignCapacity * multiplier;

        ET_SMBIOS_UINT32_UNITS(L"Design capacity", capacity, L" mWh");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortableBattery, DesignVoltage) &&
        Entry->PortableBattery.DesignVoltage != 0)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->PortableBattery.DesignVoltage / 1000, 2);
        PhInitFormatS(&format[1], L" V");
        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Design voltage", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, SBDSVersionNumber))
        ET_SMBIOS_STRING(L"SBDS version number", Entry->PortableBattery.SBDSVersionNumber);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortableBattery, MaximumError) &&
        Entry->PortableBattery.MaximumError != UCHAR_MAX)
    {
        ET_SMBIOS_UINT32_UNITS(L"Maximum error", Entry->PortableBattery.MaximumError, L"%");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortableBattery, SBDSSerialNumber) &&
        !PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, SerialNumber))
    {
        ET_SMBIOS_UINT32IX(L"SBDS serial number", Entry->PortableBattery.SBDSSerialNumber);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, PortableBattery, SBDSManufactureDate) &&
        !PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, ManufactureDate))
    {
        PH_FORMAT format[5];
        PPH_STRING string;

        PhInitFormatU(&format[0], 1980 + Entry->PortableBattery.SBDSManufactureDate.Year);
        PhInitFormatC(&format[1], L'-');
        PhInitFormatU(&format[2], Entry->PortableBattery.SBDSManufactureDate.Month);
        PhInitFormatC(&format[3], L'-');
        PhInitFormatU(&format[4], Entry->PortableBattery.SBDSManufactureDate.Day);

        string = PhFormat(format, 5, 10);
        EtAddSMBIOSItem(Context, group, L"SBDS Manufacture date", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, SBDSDeviceChemistry))
        ET_SMBIOS_STRING(L"SBDS device chemistry", Entry->PortableBattery.SBDSDeviceChemistry);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, PortableBattery, OEMSpecific))
        ET_SMBIOS_UINT32IX(L"OEM specific", Entry->PortableBattery.OEMSpecific);
}

VOID EtSMBIOSSystemReset(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR watchdog[] =
    {
        SIP(L"Reserved", SMBIOS_SYSTEM_RESET_WATCHDOG_RESERVED),
        SIP(L"Operating system", SMBIOS_SYSTEM_RESET_WATCHDOG_OPERATING_SYSTEM),
        SIP(L"System utilities", SMBIOS_SYSTEM_RESET_WATCHDOG_SYSTEM_UTILITES),
        SIP(L"Do not reboot", SMBIOS_SYSTEM_RESET_WATCHDOG_DO_NOT_REBOOT),
    };

    ET_SMBIOS_GROUP(L"System reset");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemReset, Capabilities))
    {
        ET_SMBIOS_BOOLEAN(L"User enabled", !!Entry->SystemReset.Capabilities.UserEnabled);
        ET_SMBIOS_ENUM(L"Watchdog reset", Entry->SystemReset.Capabilities.WatchdogReset, watchdog);
        ET_SMBIOS_ENUM(L"Watchdog action", Entry->SystemReset.Capabilities.WatchdogAction, watchdog);
        ET_SMBIOS_BOOLEAN(L"Watchdog exists", !!Entry->SystemReset.Capabilities.WatchdogExists);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemReset, ResetCount) &&
        Entry->SystemReset.ResetCount != 0x0FFFF)
    {
        ET_SMBIOS_UINT32(L"Reset count", Entry->SystemReset.ResetCount);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemReset, ResetLimit) &&
        Entry->SystemReset.ResetLimit != 0x0FFFF)
    {
        ET_SMBIOS_UINT32(L"Reset limit", Entry->SystemReset.ResetLimit);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemReset, TimerInterval) &&
        Entry->SystemReset.TimerInterval != 0x0FFFF)
    {
        ET_SMBIOS_UINT32_UNITS(L"Timer interval", Entry->SystemReset.TimerInterval, L" minutes");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemReset, Timeout) &&
        Entry->SystemReset.Timeout != 0x0FFFF)
    {
        ET_SMBIOS_UINT32_UNITS(L"Timeout", Entry->SystemReset.TimerInterval, L" minutes");
    }
}

VOID EtSMBIOSHardwareSecurity(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR settings[] =
    {
        SIP(L"Disabled", SMBIOS_HARDWARE_SECURITY_SETTING_DISABLED),
        SIP(L"Enabled", SMBIOS_HARDWARE_SECURITY_SETTING_ENABLED),
        SIP(L"Not implemented", SMBIOS_HARDWARE_SECURITY_SETTING_NOT_IMPLEMENTED),
        SIP(L"Unknown", SMBIOS_HARDWARE_SECURITY_SETTING_UNKNOWN),
    };

    ET_SMBIOS_GROUP(L"Hardware security");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, HardwareSecurity, HardwareSecuritySettings))
    {
        ET_SMBIOS_ENUM(L"Front panel reset", Entry->HardwareSecurity.HardwareSecuritySettings.FrontPanelReset, settings);
        ET_SMBIOS_ENUM(L"Administrator password", Entry->HardwareSecurity.HardwareSecuritySettings.AdministratorPassword, settings);
        ET_SMBIOS_ENUM(L"Administrator password", Entry->HardwareSecurity.HardwareSecuritySettings.KeyboardPassword, settings);
        ET_SMBIOS_ENUM(L"Power-on password", Entry->HardwareSecurity.HardwareSecuritySettings.PowerOnPassword, settings);
    }
}

VOID EtSMBIOSSystemPowerControls(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System power controls");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (Entry->Header.Length >= sizeof(SMBIOS_SYSTEM_POWER_CONTROLS_INFORMATION))
    {
        PH_FORMAT format[9];
        PPH_STRING string;

        PhInitFormatU(&format[0], Entry->SystemPowerControls.Month);
        PhInitFormatC(&format[1], L'-');
        PhInitFormatU(&format[2], Entry->SystemPowerControls.Day);
        PhInitFormatC(&format[3], L' ');
        PhInitFormatU(&format[4], Entry->SystemPowerControls.Hour);
        PhInitFormatC(&format[5], L':');
        PhInitFormatU(&format[6], Entry->SystemPowerControls.Minute);
        PhInitFormatC(&format[7], L':');
        PhInitFormatU(&format[8], Entry->SystemPowerControls.Second);

        string = PhFormat(format, 9, 20);
        EtAddSMBIOSItem(Context, group, L"Next scheduled power-on", PhGetString(string));
        PhDereferenceObject(string);
    }
}

VOID EtSMBIOSVoltageProbe(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR locations[] =
    {
        SIP(L"Other", SMBIOS_VOLTAGE_PROBE_LOCATION_OTHER),
        SIP(L"Unknown", SMBIOS_VOLTAGE_PROBE_LOCATION_UNKNOWN),
        SIP(L"Processor", SMBIOS_VOLTAGE_PROBE_LOCATION_PROCESSOR),
        SIP(L"Disk", SMBIOS_VOLTAGE_PROBE_LOCATION_DISK),
        SIP(L"Peripheral bay", SMBIOS_VOLTAGE_PROBE_LOCATION_PERIPHERAL_BAY),
        SIP(L"System management module", SMBIOS_VOLTAGE_PROBE_LOCATION_SYSTEM_MANAGEMENT_MODULE),
        SIP(L"Motherboard", SMBIOS_VOLTAGE_PROBE_LOCATION_MOTHERBOARD),
        SIP(L"Memory module", SMBIOS_VOLTAGE_PROBE_LOCATION_MEMORY_MODULE),
        SIP(L"Processor module", SMBIOS_VOLTAGE_PROBE_LOCATION_PROCESSOR_MODULE),
        SIP(L"Power unit", SMBIOS_VOLTAGE_PROBE_LOCATION_POWER_UNIT),
        SIP(L"Add-in card", SMBIOS_VOLTAGE_PROBE_LOCATION_ADD_IN_CARD),
    };

    ET_SMBIOS_GROUP(L"Voltage probe");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, VoltageProbe, Description))
        ET_SMBIOS_STRING(L"Description", Entry->VoltageProbe.Description);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, VoltageProbe, LocationAndStatus))
    {
        ET_SMBIOS_ENUM(L"Location", Entry->VoltageProbe.LocationAndStatus.Location, locations);
        ET_SMBIOS_ENUM(L"Status", Entry->VoltageProbe.LocationAndStatus.Status, EtSMBIOSProbeStatus);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, VoltageProbe, MaximumValue) &&
        Entry->VoltageProbe.MaximumValue != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->VoltageProbe.MaximumValue / 1000, 2);
        PhInitFormatS(&format[1], L" V");
        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Maximum value", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, VoltageProbe, MinimumValue) &&
        Entry->VoltageProbe.MinimumValue != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->VoltageProbe.MinimumValue / 1000, 2);
        PhInitFormatS(&format[1], L" V");
        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Minimum value", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, VoltageProbe, Resolution) &&
        Entry->VoltageProbe.Resolution != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->VoltageProbe.Resolution / 10, 1);
        PhInitFormatS(&format[1], L" mV");
        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Resolution", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, VoltageProbe, Tolerance) &&
        Entry->VoltageProbe.Tolerance != 0x8000)
    {
        PH_FORMAT format[3];
        PPH_STRING string;

        PhInitFormatS(&format[0], L"+/- ");
        PhInitFormatF(&format[1], (FLOAT)Entry->VoltageProbe.Tolerance / 1000, 2);
        PhInitFormatS(&format[2], L" V");
        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Tolerance", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, VoltageProbe, Accuracy) &&
        Entry->VoltageProbe.Accuracy != 0x8000)
    {
        PH_FORMAT format[3];
        PPH_STRING string;

        PhInitFormatS(&format[0], L"+/- ");
        PhInitFormatF(&format[1], (FLOAT)Entry->VoltageProbe.Accuracy / 100, 3);
        PhInitFormatS(&format[2], L"%");
        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Accuracy", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, VoltageProbe, OEMDefined))
        ET_SMBIOS_UINT32IX(L"OEM defined", Entry->VoltageProbe.OEMDefined);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, VoltageProbe, NominalValue) &&
        Entry->VoltageProbe.NominalValue != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->VoltageProbe.NominalValue / 1000, 2);
        PhInitFormatS(&format[1], L" V");
        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Nominal value", PhGetString(string));
        PhDereferenceObject(string);
    }
}

VOID EtSMBIOSCoolingDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"Other", SMBIOS_COOLING_DEVICE_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_COOLING_DEVICE_TYPE_UNKNOWN),
        SIP(L"Fan", SMBIOS_COOLING_DEVICE_TYPE_FAN),
        SIP(L"Centrifugal blower", SMBIOS_COOLING_DEVICE_TYPE_CENTRIFUGAL_BLOWER),
        SIP(L"Chip fan", SMBIOS_COOLING_DEVICE_TYPE_CHIP_FAN),
        SIP(L"Cabinet fan", SMBIOS_COOLING_DEVICE_TYPE_CABINET_FAN),
        SIP(L"Power supply fan", SMBIOS_COOLING_DEVICE_TYPE_POWER_SUPPLY_FAN),
        SIP(L"Heat pipe", SMBIOS_COOLING_DEVICE_TYPE_HEAT_PIPE),
        SIP(L"Integrated refrigeration", SMBIOS_COOLING_DEVICE_TYPE_INTEGRATED_REFRIGERATION),
        SIP(L"Active cooling", SMBIOS_COOLING_DEVICE_TYPE_ACTIVE_COOLING),
        SIP(L"Passive cooling", SMBIOS_COOLING_DEVICE_TYPE_PASSIVE_COOLING),
    };

    static const PH_KEY_VALUE_PAIR deviceStatus[] =
    {
        SIP(L"Other", SMBIOS_COOLING_DEVICE_STATUS_OTHER),
        SIP(L"Unknown", SMBIOS_COOLING_DEVICE_STATUS_UNKNOWN),
        SIP(L"Ok", SMBIOS_COOLING_DEVICE_STATUS_OK),
        SIP(L"Non-critical", SMBIOS_COOLING_DEVICE_STATUS_NON_CRITICAL),
        SIP(L"Critical", SMBIOS_COOLING_DEVICE_STATUS_CRITICAL),
        SIP(L"Non-recoverable", SMBIOS_COOLING_DEVICE_STATUS_NON_RECOVERABLE),
    };

    ET_SMBIOS_GROUP(L"Cooling device");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, CoolingDevice, TemperatureProbeHandle))
        ET_SMBIOS_UINT32IX(L"Temperature probe handle", Entry->CoolingDevice.TemperatureProbeHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, CoolingDevice, DeviceTypeAndStatus))
    {
        ET_SMBIOS_ENUM(L"Type", Entry->CoolingDevice.DeviceTypeAndStatus.DeviceType, types);
        ET_SMBIOS_ENUM(L"Status", Entry->CoolingDevice.DeviceTypeAndStatus.Status, deviceStatus);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, CoolingDevice, CoolingUnitGroup) &&
        Entry->CoolingDevice.CoolingUnitGroup != 0)
    {
        ET_SMBIOS_UINT32(L"Cooling unit group", Entry->CoolingDevice.CoolingUnitGroup);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, CoolingDevice, OEMDefined))
        ET_SMBIOS_UINT32IX(L"OEM defined", Entry->CoolingDevice.OEMDefined);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, CoolingDevice, NominalSpeed) &&
        Entry->CoolingDevice.NominalSpeed != 0x8000)
    {
        ET_SMBIOS_UINT32_UNITS(L"Nominal speed", Entry->CoolingDevice.NominalSpeed, L" rpm");
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, CoolingDevice, Description))
        ET_SMBIOS_STRING(L"Description", Entry->CoolingDevice.Description);
}

VOID EtSMBIOSTemperatureProbe(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR locations[] =
    {
        SIP(L"Other", SMBIOS_TEMPERATURE_PROBE_LOCATION_OTHER),
        SIP(L"Unknown", SMBIOS_TEMPERATURE_PROBE_LOCATION_UNKNOWN),
        SIP(L"Processor", SMBIOS_TEMPERATURE_PROBE_LOCATION_PROCESSOR),
        SIP(L"Disk", SMBIOS_TEMPERATURE_PROBE_LOCATION_DISK),
        SIP(L"Peripheral bay", SMBIOS_TEMPERATURE_PROBE_LOCATION_PERIPHERAL_BAY),
        SIP(L"System management module", SMBIOS_TEMPERATURE_PROBE_LOCATION_SYSTEM_MANAGEMENT_MODULE),
        SIP(L"Motherboard", SMBIOS_TEMPERATURE_PROBE_LOCATION_MOTHERBOARD),
        SIP(L"Memory module", SMBIOS_TEMPERATURE_PROBE_LOCATION_MEMORY_MODULE),
        SIP(L"Processor module", SMBIOS_TEMPERATURE_PROBE_LOCATION_PROCESSOR_MODULE),
        SIP(L"Power unit", SMBIOS_TEMPERATURE_PROBE_LOCATION_POWER_UNIT),
        SIP(L"Add-in card", SMBIOS_TEMPERATURE_PROBE_LOCATION_ADD_IN_CARD),
        SIP(L"Front panel board", SMBIOS_TEMPERATURE_PROBE_LOCATION_FRONT_PANEL_BOARD),
        SIP(L"Back panel board", SMBIOS_TEMPERATURE_PROBE_LOCATION_BACK_PANEL_BOARD),
        SIP(L"Power system board", SMBIOS_TEMPERATURE_PROBE_LOCATION_POWER_SYSTEM_BOARD),
        SIP(L"Drive back plane", SMBIOS_TEMPERATURE_PROBE_LOCATION_DRIVE_BACK_PLANE),
    };

    ET_SMBIOS_GROUP(L"Temperature probe");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, TemperatureProbe, Description))
        ET_SMBIOS_STRING(L"Description", Entry->TemperatureProbe.Description);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TemperatureProbe, LocationAndStatus))
    {
        ET_SMBIOS_ENUM(L"Location", Entry->TemperatureProbe.LocationAndStatus.Location, locations);
        ET_SMBIOS_ENUM(L"Status", Entry->TemperatureProbe.LocationAndStatus.Status, EtSMBIOSProbeStatus);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TemperatureProbe, MaximumValue) &&
        Entry->TemperatureProbe.MaximumValue != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->TemperatureProbe.MaximumValue / 10, 1);
        PhInitFormatS(&format[1], L" C");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Maximum value", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TemperatureProbe, MinimumValue) &&
        Entry->TemperatureProbe.MinimumValue != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->TemperatureProbe.MinimumValue / 10, 1);
        PhInitFormatS(&format[1], L" C");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Minimum value", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TemperatureProbe, Resolution) &&
        Entry->TemperatureProbe.Resolution != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->TemperatureProbe.Resolution / 1000, 3);
        PhInitFormatS(&format[1], L" C");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Resolution", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TemperatureProbe, Tolerance) &&
        Entry->TemperatureProbe.Tolerance != 0x8000)
    {
        PH_FORMAT format[3];
        PPH_STRING string;

        PhInitFormatS(&format[0], L"+/- ");
        PhInitFormatF(&format[1], (FLOAT)Entry->TemperatureProbe.Tolerance / 10, 1);
        PhInitFormatS(&format[2], L" C");

        string = PhFormat(format, 3, 10);
        EtAddSMBIOSItem(Context, group, L"Tolerance", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TemperatureProbe, Accuracy) &&
        Entry->TemperatureProbe.Tolerance != 0x8000)
    {
        PH_FORMAT format[3];
        PPH_STRING string;

        PhInitFormatS(&format[0], L"+/- ");
        PhInitFormatF(&format[1], (FLOAT)Entry->TemperatureProbe.Accuracy / 100, 2);
        PhInitFormatS(&format[2], L"%");

        string = PhFormat(format, 3, 10);
        EtAddSMBIOSItem(Context, group, L"Accuracy", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TemperatureProbe, OEMDefined))
        ET_SMBIOS_UINT32IX(L"OEM define", Entry->TemperatureProbe.OEMDefined);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TemperatureProbe, NominalValue) &&
        Entry->TemperatureProbe.NominalValue != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->TemperatureProbe.NominalValue / 10, 1);
        PhInitFormatS(&format[1], L" C");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Nominal value", PhGetString(string));
        PhDereferenceObject(string);
    }
}

VOID EtSMBIOSElectricalCurrentProbe(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR locations[] =
    {
        SIP(L"Other", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_OTHER),
        SIP(L"Unknown", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_UNKNOWN),
        SIP(L"Processor", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_PROCESSOR),
        SIP(L"Disk", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_DISK),
        SIP(L"Peripheral bay", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_PERIPHERAL_BAY),
        SIP(L"System management module", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_SYSTEM_MANAGEMENT_MODULE),
        SIP(L"Motherboard", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_MOTHERBOARD),
        SIP(L"Memory module", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_MEMORY_MODULE),
        SIP(L"Processor module", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_PROCESSOR_MODULE),
        SIP(L"Power unit", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_POWER_UNIT),
        SIP(L"Add-in card", SMBIOS_ELECTRICAL_CURRENT_PROBE_LOCATION_ADD_IN_CARD),
    };

    ET_SMBIOS_GROUP(L"Electrical current probe");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, ElectricalCurrentProbe, Description))
        ET_SMBIOS_STRING(L"Description", Entry->ElectricalCurrentProbe.Description);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ElectricalCurrentProbe, LocationAndStatus))
    {
        ET_SMBIOS_ENUM(L"Location", Entry->ElectricalCurrentProbe.LocationAndStatus.Location, locations);
        ET_SMBIOS_ENUM(L"Status", Entry->ElectricalCurrentProbe.LocationAndStatus.Status, EtSMBIOSProbeStatus);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ElectricalCurrentProbe, MaximumValue) &&
        Entry->ElectricalCurrentProbe.MaximumValue != 0x8000)
    {
        ET_SMBIOS_UINT32_UNITS(L"Maximum value", Entry->ElectricalCurrentProbe.MaximumValue, L" mA");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ElectricalCurrentProbe, MinimumValue) &&
        Entry->ElectricalCurrentProbe.MinimumValue != 0x8000)
    {
        ET_SMBIOS_UINT32_UNITS(L"Minimum value", Entry->ElectricalCurrentProbe.MinimumValue, L" mA");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ElectricalCurrentProbe, Resolution) &&
        Entry->ElectricalCurrentProbe.Resolution != 0x8000)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatF(&format[0], (FLOAT)Entry->ElectricalCurrentProbe.Resolution / 10, 1);
        PhInitFormatS(&format[1], L" mA");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Resolution", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ElectricalCurrentProbe, Tolerance) &&
        Entry->ElectricalCurrentProbe.Tolerance != 0x8000)
    {
        PH_FORMAT format[3];
        PPH_STRING string;

        PhInitFormatS(&format[0], L"+/- ");
        PhInitFormatU(&format[1], Entry->ElectricalCurrentProbe.Tolerance);
        PhInitFormatS(&format[2], L" mA");

        string = PhFormat(format, 3, 10);
        EtAddSMBIOSItem(Context, group, L"Resolution", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ElectricalCurrentProbe, Accuracy) &&
        Entry->ElectricalCurrentProbe.Accuracy != 0x8000)
    {
        PH_FORMAT format[3];
        PPH_STRING string;

        PhInitFormatS(&format[0], L"+/- ");
        PhInitFormatF(&format[1], (FLOAT)Entry->ElectricalCurrentProbe.Accuracy / 1000, 3);
        PhInitFormatS(&format[2], L"%");

        string = PhFormat(format, 3, 10);
        EtAddSMBIOSItem(Context, group, L"Accuracy", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ElectricalCurrentProbe, OEMDefined))
        ET_SMBIOS_UINT64IX(L"OEM defined", Entry->ElectricalCurrentProbe.OEMDefined);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ElectricalCurrentProbe, NominalValue) &&
        Entry->ElectricalCurrentProbe.NominalValue != 0x8000)
    {
        ET_SMBIOS_UINT32_UNITS(L"Nominal value", Entry->ElectricalCurrentProbe.NominalValue, L" mA");
    }
}

VOID EtSMBIOSOutOfBandRemoteAccess(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Out-of-band remote access");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, OutOfBandRemoteAccess, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->OutOfBandRemoteAccess.Manufacturer);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, OutOfBandRemoteAccess, Connections))
    {
        ET_SMBIOS_BOOLEAN(L"Inbound enabled", !!Entry->OutOfBandRemoteAccess.Connections.InboundEnabled);
        ET_SMBIOS_BOOLEAN(L"Outbound enabled", !!Entry->OutOfBandRemoteAccess.Connections.InboundEnabled);
    }
}

VOID EtSMBIOSSystemBoot(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR bootStatus[] =
    {
        SIP(L"No error", SMBIOS_SYSTEM_BOOT_STATUS_NO_ERROR),
        SIP(L"No bootable media", SMBIOS_SYSTEM_BOOT_STATUS_NO_BOOTABLE_MEDIA),
        SIP(L"System failed to load", SMBIOS_SYSTEM_BOOT_STATUS_OPERATING_SYSTEM_FAILED_TO_LOAD),
        SIP(L"Firmware detected hardware failure", SMBIOS_SYSTEM_BOOT_STATUS_FIRMWARE_DETECTED_HARDWARE_FAILURE),
        SIP(L"Operating system detected hardware failure", SMBIOS_SYSTEM_BOOT_STATUS_OPERATING_SYSTEM_DETECTED_HARDWARE_FAILURE),
        SIP(L"User-requested boot", SMBIOS_SYSTEM_BOOT_STATUS_USER_REQUESTED_BOOT),
        SIP(L"Security violation", SMBIOS_SYSTEM_BOOT_STATUS_SECURITY_VIOLATION),
        SIP(L"Previously requested", SMBIOS_SYSTEM_BOOT_STATUS_PREVIOUSLY_REQUESTED_IMAGE),
        SIP(L"Watchdog expired", SMBIOS_SYSTEM_BOOT_STATUS_WATCHDOG_EXPIRED),
    };

    ET_SMBIOS_GROUP(L"System boot");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemBoot, BootStatus))
        ET_SMBIOS_ENUM(L"Boot status", Entry->SystemBoot.BootStatus, bootStatus);
}

VOID EtSMBIOS64BitMemoryError(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"64-bit memory error");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError64, Type))
        ET_SMBIOS_ENUM(L"Type", Entry->MemoryError32.Type, EtSMBIOSMemoryErrorTypes);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError64, Granularity))
        ET_SMBIOS_ENUM(L"Granularity", Entry->MemoryError64.Granularity, EtSMBIOSMemoryErrorGranularities);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError64, Operation))
        ET_SMBIOS_ENUM(L"Operation", Entry->MemoryError64.Operation, EtSMBIOSMemoryErrorOperations);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError32, VendorSyndrome) &&
        Entry->MemoryError64.VendorSyndrome != 0)
    {
        ET_SMBIOS_UINT32IX(L"Vendor syndrome", Entry->MemoryError64.VendorSyndrome);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError64, ArrayErrorAddress) &&
        Entry->MemoryError32.ArrayErrorAddress != 0x8000000000000000)
    {
        ET_SMBIOS_UINT64IX(L"Array error address", Entry->MemoryError64.ArrayErrorAddress);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError64, DeviceErrorAddress) &&
        Entry->MemoryError32.DeviceErrorAddress != 0x8000000000000000)
    {
        ET_SMBIOS_UINT64IX(L"Device error address", Entry->MemoryError64.DeviceErrorAddress);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryError64, ErrorResolution) &&
        Entry->MemoryError64.ErrorResolution != 0x80000000)
    {
        ET_SMBIOS_UINT32IX(L"Error resolution", Entry->MemoryError64.ErrorResolution);
    }

}

VOID EtSMBIOSManagementDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR deviceTypes[] =
    {
        SIP(L"Other", SMBIOS_MANAGEMENT_DEVICE_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_MANAGEMENT_DEVICE_TYPE_UNKNOWN),
        SIP(L"LM75", SMBIOS_MANAGEMENT_DEVICE_TYPE_LM75),
        SIP(L"LM78", SMBIOS_MANAGEMENT_DEVICE_TYPE_LM78),
        SIP(L"LM79", SMBIOS_MANAGEMENT_DEVICE_TYPE_LM79),
        SIP(L"LM80", SMBIOS_MANAGEMENT_DEVICE_TYPE_LM80),
        SIP(L"LM81", SMBIOS_MANAGEMENT_DEVICE_TYPE_LM81),
        SIP(L"ADM9240", SMBIOS_MANAGEMENT_DEVICE_TYPE_ADM9240),
        SIP(L"DS1780", SMBIOS_MANAGEMENT_DEVICE_TYPE_DS1780),
        SIP(L"1617", SMBIOS_MANAGEMENT_DEVICE_TYPE_1617),
        SIP(L"GL518SM", SMBIOS_MANAGEMENT_DEVICE_TYPE_GL518SM),
        SIP(L"W83781D", SMBIOS_MANAGEMENT_DEVICE_TYPE_W83781D),
        SIP(L"HT82H791", SMBIOS_MANAGEMENT_DEVICE_TYPE_HT82H791),
    };

    static const PH_KEY_VALUE_PAIR addressTypes[] =
    {
        SIP(L"Other", SMBIOS_MANAGEMENT_DEVICE_ADDRESS_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_MANAGEMENT_DEVICE_ADDRESS_TYPE_UNKNOWN),
        SIP(L"I/O port", SMBIOS_MANAGEMENT_DEVICE_ADDRESS_TYPE_IO_PORT),
        SIP(L"Memory", SMBIOS_MANAGEMENT_DEVICE_ADDRESS_TYPE_MEMORY),
        SIP(L"SM bus", SMBIOS_MANAGEMENT_DEVICE_ADDRESS_TYPE_SMBUS),
    };

    ET_SMBIOS_GROUP(L"Management device");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, ManagementDevice, Description))
        ET_SMBIOS_STRING(L"Description", Entry->ManagementDevice.Description);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDevice, DeviceType))
        ET_SMBIOS_ENUM(L"Device type", Entry->ManagementDevice.DeviceType, deviceTypes);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDevice, Address))
        ET_SMBIOS_UINT32IX(L"Address", Entry->ManagementDevice.Address);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDevice, AddressType))
        ET_SMBIOS_ENUM(L"Address type", Entry->ManagementDevice.AddressType, addressTypes);
}

VOID EtSMBIOSManagementDeviceComponent(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Management device component");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, ManagementDeviceComponent, Description))
        ET_SMBIOS_STRING(L"Description", Entry->ManagementDeviceComponent.Description);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceComponent, ManagementDeviceHandle))
        ET_SMBIOS_UINT32IX(L"Management device handle", Entry->ManagementDeviceComponent.ManagementDeviceHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceComponent, ComponentHandle))
        ET_SMBIOS_UINT32IX(L"Component handle", Entry->ManagementDeviceComponent.ComponentHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceComponent, ThresholdHandle))
        ET_SMBIOS_UINT32IX(L"Threshold handle", Entry->ManagementDeviceComponent.ThresholdHandle);
}

VOID EtSMBIOSManagementDeviceThreshold(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Management device threshold");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceThreshold, LowerThresholdNonCritical))
        ET_SMBIOS_UINT32(L"Lower threshold non-critical", Entry->ManagementDeviceThreshold.LowerThresholdNonCritical);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceThreshold, UpperThresholdNonCritical))
        ET_SMBIOS_UINT32(L"Upper threshold non-critical", Entry->ManagementDeviceThreshold.UpperThresholdNonCritical);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceThreshold, LowerThresholdCritical))
        ET_SMBIOS_UINT32(L"Lower threshold critical", Entry->ManagementDeviceThreshold.LowerThresholdCritical);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceThreshold, UpperThresholdCritical))
        ET_SMBIOS_UINT32(L"Upper threshold critical", Entry->ManagementDeviceThreshold.UpperThresholdCritical);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceThreshold, LowerThresholdNonRecoverable))
        ET_SMBIOS_UINT32(L"Lower threshold non-recoverable", Entry->ManagementDeviceThreshold.LowerThresholdNonRecoverable);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ManagementDeviceThreshold, UpperThresholdNonRecoverable))
        ET_SMBIOS_UINT32(L"Upper threshold non-recoverable", Entry->ManagementDeviceThreshold.UpperThresholdNonRecoverable);
}

VOID EtSMBIOSMemoryChannel(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"Other", SMBIOS_MEMORY_CHANNEL_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_MEMORY_CHANNEL_TYPE_UNKNOWN),
        SIP(L"Rambus", SMBIOS_MEMORY_CHANNEL_TYPE_RAMBUS),
        SIP(L"SyncLink", SMBIOS_MEMORY_CHANNEL_TYPE_SYNC_LINK),
    };

    ET_SMBIOS_GROUP(L"Memory channel");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryChannel, Type))
        ET_SMBIOS_ENUM(L"Type", Entry->MemoryChannel.Type, types);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryChannel, MaximumLoad))
        ET_SMBIOS_UINT32(L"Maximum load", Entry->MemoryChannel.MaximumLoad);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryChannel, Count))
    {
        for (UCHAR i = 0; i < Entry->MemoryChannel.Count; i++)
        {
            PH_FORMAT format[3];
            PPH_STRING name;

            PhInitFormatS(&format[0], L"Channel #");
            PhInitFormatU(&format[1], i + 1);
            PhInitFormatS(&format[2], L" handle");

            name = PhFormat(format, 3, 20);
            ET_SMBIOS_UINT32IX(PhGetString(name), Entry->MemoryChannel.Entries[i].Handle);
            PhDereferenceObject(name);

            PhInitFormatS(&format[2], L" load");
            name = PhFormat(format, 3, 20);
            ET_SMBIOS_UINT32(PhGetString(name), Entry->MemoryChannel.Entries[i].Load);
            PhDereferenceObject(name);
        }
    }
}

VOID EtSMBIOSIPMIDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"Unknown", SMBIOS_IPMI_INTERFACE_TYPE_UNKONWN),
        SIP(L"KCS", SMBIOS_IPMI_INTERFACE_TYPE_KCS),
        SIP(L"SMIC", SMBIOS_IPMI_INTERFACE_TYPE_SMIC),
        SIP(L"BT", SMBIOS_IPMI_INTERFACE_TYPE_BT),
        SIP(L"SSIF", SMBIOS_IPMI_INTERFACE_TYPE_SSIF),
    };

    static const PH_KEY_VALUE_PAIR spacings[] =
    {
        SIP(L"Successive", SMBIOS_IPMI_REGISTER_SPACING_SUCCESSIVE),
        SIP(L"32-bit", SMBIOS_IPMI_REGISTER_SPACING_32_BIT),
        SIP(L"16-bit", SMBIOS_IPMI_REGISTER_SPACING_16_BIT),
    };

    ET_SMBIOS_GROUP(L"IPMI device");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, IPMIDevice, Type))
        ET_SMBIOS_ENUM(L"Type", Entry->IPMIDevice.Type, types);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, IPMIDevice, SpecificationRevision))
    {
        PH_FORMAT format[3];
        PPH_STRING string;

        PhInitFormatU(&format[0], Entry->IPMIDevice.SpecificationRevision.Major);
        PhInitFormatC(&format[1], L'.');
        PhInitFormatU(&format[2], Entry->IPMIDevice.SpecificationRevision.Minor);

        string = PhFormat(format, 3, 10);
        EtAddSMBIOSItem(Context, group, L"Specification revision", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, IPMIDevice, I2CTargetAddress))
        ET_SMBIOS_UINT32IX(L"I2C target address", Entry->IPMIDevice.I2CTargetAddress);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, IPMIDevice, NVStorageDeviceAddress) &&
        Entry->IPMIDevice.NVStorageDeviceAddress != UCHAR_MAX)
    {
        ET_SMBIOS_UINT32IX(L"NV storage address", Entry->IPMIDevice.NVStorageDeviceAddress);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, IPMIDevice, BaseAddress))
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatI64X(&format[0], Entry->IPMIDevice.BaseAddress & ~1ULL);
        if (Entry->IPMIDevice.BaseAddress & 1)
            PhInitFormatS(&format[1], L", I/O space");
        else
            PhInitFormatS(&format[1], L", memory-mapped");

        string = PhFormat(format, 2, 20);
        EtAddSMBIOSItem(Context, group, L"Base address", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, IPMIDevice, Info))
    {
        EtAddSMBIOSItem(Context, group, L"Trigger mode", Entry->IPMIDevice.Info.InterruptTriggerMode ? L"Level" : L"Edge");
        EtAddSMBIOSItem(Context, group, L"Polarity", Entry->IPMIDevice.Info.InterruptPolarity ? L"Active high" : L"Active low");
        EtAddSMBIOSItem(Context, group, L"Interrupt info", Entry->IPMIDevice.Info.InterruptInfo ? L"Specified" : L"Not specified");
        EtAddSMBIOSItem(Context, group, L"LS-bit for address", Entry->IPMIDevice.Info.LSBAddress ? L"1" : L"0");
        ET_SMBIOS_ENUM(L"Register spacing", Entry->IPMIDevice.Info.RegisterSpacing, spacings);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, IPMIDevice, InterruptNumber))
        ET_SMBIOS_UINT32(L"Interrupt #", Entry->IPMIDevice.InterruptNumber);
}

VOID EtSMBIOSSystemPowerSupply(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR rangeSwitching[] =
    {
        SIP(L"Other", SMBIOS_POWER_SUPPLY_RANGE_SWITCHING_OTHER),
        SIP(L"Unknown", SMBIOS_POWER_SUPPLY_RANGE_SWITCHING_UNKNOWN),
        SIP(L"Manual", SMBIOS_POWER_SUPPLY_RANGE_SWITCHING_MANUAL),
        SIP(L"Auto-switch", SMBIOS_POWER_SUPPLY_RANGE_SWITCHING_AUTO_SWITCH),
        SIP(L"Wide range", SMBIOS_POWER_SUPPLY_RANGE_SWITCHING_WIDE_RANGE),
        SIP(L"Not applicable", SMBIOS_POWER_SUPPLY_RANGE_SWITCHING_NOT_APPLICABLE),
    };

    static const PH_KEY_VALUE_PAIR powerSupplyStatus[] =
    {
        SIP(L"Other", SMBIOS_POWER_SUPPLY_STATUS_OTHER),
        SIP(L"Unknown", SMBIOS_POWER_SUPPLY_STATUS_UNKNOWN),
        SIP(L"Ok", SMBIOS_POWER_SUPPLY_STATUS_OK),
        SIP(L"Non-critical", SMBIOS_POWER_SUPPLY_STATUS_NON_CRITICAL),
        SIP(L"Critical", SMBIOS_POWER_SUPPLY_STATUS_CRITICAL),
    };

    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"Other", SMBIOS_POWER_SUPPLY_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_POWER_SUPPLY_TYPE_UNKNOWN),
        SIP(L"Linear", SMBIOS_POWER_SUPPLY_TYPE_LINEAR),
        SIP(L"Switching", SMBIOS_POWER_SUPPLY_TYPE_SWITCHING),
        SIP(L"Battery", SMBIOS_POWER_SUPPLY_TYPE_BATTERY),
        SIP(L"UPS", SMBIOS_POWER_SUPPLY_TYPE_UPS),
        SIP(L"Converter", SMBIOS_POWER_SUPPLY_TYPE_CONVERTER),
        SIP(L"Regulator", SMBIOS_POWER_SUPPLY_TYPE_REGULATOR),
    };

    ET_SMBIOS_GROUP(L"System power supply");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemPowerSupply, PowerUnitGroup) &&
        Entry->SystemPowerSupply.PowerUnitGroup != 0)
    {
        ET_SMBIOS_UINT32(L"Power unit group", Entry->SystemPowerSupply.PowerUnitGroup);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, SystemPowerSupply, Location))
        ET_SMBIOS_STRING(L"Location", Entry->SystemPowerSupply.Location);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, SystemPowerSupply, DeviceName))
        ET_SMBIOS_STRING(L"Device name", Entry->SystemPowerSupply.DeviceName);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, SystemPowerSupply, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->SystemPowerSupply.Manufacturer);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, SystemPowerSupply, SerialNumber))
        ET_SMBIOS_STRING(L"Serial number", Entry->SystemPowerSupply.SerialNumber);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, SystemPowerSupply, AssetTag))
        ET_SMBIOS_STRING(L"Asset tag", Entry->SystemPowerSupply.AssetTag);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, SystemPowerSupply, ModelPartNumber))
        ET_SMBIOS_STRING(L"Model part number", Entry->SystemPowerSupply.ModelPartNumber);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, SystemPowerSupply, Revision))
        ET_SMBIOS_STRING(L"Revision", Entry->SystemPowerSupply.Revision);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemPowerSupply, MaxPowerCapacity) &&
        Entry->SystemPowerSupply.MaxPowerCapacity != 0x8000)
    {
        ET_SMBIOS_UINT32_UNITS(L"Max power capacity", Entry->SystemPowerSupply.MaxPowerCapacity, L" W");
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemPowerSupply, Characteristics))
    {
        ET_SMBIOS_BOOLEAN(L"Hot-swappable", !!Entry->SystemPowerSupply.Characteristics.HostSwappable);
        ET_SMBIOS_BOOLEAN(L"Present", !!Entry->SystemPowerSupply.Characteristics.Present);
        ET_SMBIOS_ENUM(L"Range switching", Entry->SystemPowerSupply.Characteristics.RangeSwitching, rangeSwitching);
        ET_SMBIOS_ENUM(L"Status", Entry->SystemPowerSupply.Characteristics.Status, powerSupplyStatus);
        ET_SMBIOS_ENUM(L"Type", Entry->SystemPowerSupply.Characteristics.Type, types);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemPowerSupply, InputVoltageProbeHandle))
        ET_SMBIOS_UINT64IX(L"Input voltage probe handle", Entry->SystemPowerSupply.InputVoltageProbeHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemPowerSupply, CoolingDeviceHandle))
        ET_SMBIOS_UINT64IX(L"Cooling device handle", Entry->SystemPowerSupply.CoolingDeviceHandle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, SystemPowerSupply, InputCurrentProbeHandle))
        ET_SMBIOS_UINT64IX(L"Input current handle", Entry->SystemPowerSupply.InputCurrentProbeHandle);
}

VOID EtSMBIOSAdditionalInformation(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Additional information");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, AdditionalInformation, Count))
    {
        PVOID end;
        PSMBIOS_ADDITIONAL_ENTRY entry;

        ET_SMBIOS_UINT32(L"Count", Entry->AdditionalInformation.Count);

        end = PTR_ADD_OFFSET(Entry, Entry->Header.Length);
        entry = Entry->AdditionalInformation.Entries;

        for (UCHAR i = 0; i < Entry->AdditionalInformation.Count; i++)
        {
            PH_FORMAT format[3];
            PPH_STRING name;

            PhInitFormatC(&format[0], L'#');
            PhInitFormatU(&format[1], i + 1);

            PhInitFormatS(&format[2], L" Referenced handle");
            name = PhFormat(format, 3, 20);
            ET_SMBIOS_UINT32IX(PhGetString(name), entry->ReferencedHandle);
            PhDereferenceObject(name);

            if (entry->String != SMBIOS_INVALID_STRING)
            {
                PhInitFormatS(&format[2], L" String");
                name = PhFormat(format, 3, 20);
                ET_SMBIOS_STRING(PhGetString(name), entry->String);
                PhDereferenceObject(name);
            }

            if (entry->Length > FIELD_OFFSET(SMBIOS_ADDITIONAL_ENTRY, Value))
            {
                ULONG length;
                PPH_STRING value;

                length = entry->Length - FIELD_OFFSET(SMBIOS_ADDITIONAL_ENTRY, Value);
                value = PhBufferToHexString(entry->Value, length);

                PhInitFormatS(&format[2], L" Value");
                name = PhFormat(format, 3, 20);
                EtAddSMBIOSItem(Context, group, PhGetString(name), PhGetString(value));
                PhDereferenceObject(name);

                PhDereferenceObject(value);
            }

            entry = PTR_ADD_OFFSET(entry, entry->Length);
            if ((ULONG_PTR)entry >= (ULONG_PTR)end)
                break;
        }
    }
}

VOID EtSMBIOSOnboardDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"Other", SMBIOS_ONBOARD_DEVICE_TYPE_OTHER),
        SIP(L"Unknown", SMBIOS_ONBOARD_DEVICE_TYPE_UNKNOWN),
        SIP(L"Video", SMBIOS_ONBOARD_DEVICE_TYPE_VIDEO),
        SIP(L"SCSI", SMBIOS_ONBOARD_DEVICE_TYPE_SCSI),
        SIP(L"Ethernet", SMBIOS_ONBOARD_DEVICE_TYPE_ETHERNET),
        SIP(L"Token ring", SMBIOS_ONBOARD_DEVICE_TYPE_TOKEN_RING),
        SIP(L"Sound", SMBIOS_ONBOARD_DEVICE_TYPE_SOUND),
        SIP(L"PATA", SMBIOS_ONBOARD_DEVICE_TYPE_PATA),
        SIP(L"SATA", SMBIOS_ONBOARD_DEVICE_TYPE_SATA),
        SIP(L"SAS", SMBIOS_ONBOARD_DEVICE_TYPE_SAS),
        SIP(L"Wireless LAN", SMBIOS_ONBOARD_DEVICE_TYPE_WIRELESS_LAN),
        SIP(L"Bluetooth", SMBIOS_ONBOARD_DEVICE_TYPE_BLUETOOTH),
        SIP(L"WWAN", SMBIOS_ONBOARD_DEVICE_TYPE_WWAN),
        SIP(L"eMMC", SMBIOS_ONBOARD_DEVICE_TYPE_EMMC),
        SIP(L"NVMe", SMBIOS_ONBOARD_DEIVCE_TYPE_NVME),
        SIP(L"UFS", SMBIOS_ONBOARD_DEVICE_TYPE_UFS),
    };

    ET_SMBIOS_GROUP(L"Onboard device");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, OnboardDevice, ReferenceDesignation))
        ET_SMBIOS_STRING(L"Reference designation", Entry->OnboardDevice.ReferenceDesignation);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, OnboardDevice, DeviceType))
    {
        ET_SMBIOS_ENUM(L"Type", Entry->OnboardDevice.DeviceType.Type, types);
        ET_SMBIOS_BOOLEAN(L"Enabled", !!Entry->OnboardDevice.DeviceType.Enabled);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, OnboardDevice, DeviceTypeInstance))
        ET_SMBIOS_UINT32(L"Device type instance", Entry->OnboardDevice.DeviceTypeInstance);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, OnboardDevice, SegmentGroupNumber))
        ET_SMBIOS_UINT32(L"Segment group number", Entry->OnboardDevice.SegmentGroupNumber);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, OnboardDevice, BusNumber))
        ET_SMBIOS_UINT32(L"Bus number", Entry->OnboardDevice.BusNumber);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, OnboardDevice, DeviceFunctionNumber))
    {
        ET_SMBIOS_UINT32(L"Function number", Entry->OnboardDevice.DeviceFunctionNumber.FunctionNumber);
        ET_SMBIOS_UINT32(L"Device number", Entry->OnboardDevice.DeviceFunctionNumber.DeviceNumber);
    }
}

VOID EtSMBIOSMCHInterface(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR interfaceTypes[] =
    {
        SIP(L"Network interface", SMBIOS_MCHI_TYPE_NETWORK_INTERFACE),
        SIP(L"OEM defined", SMBIOS_MCHI_TYPE_OEM_DEFINED)
    };

    static const PH_KEY_VALUE_PAIR protocolTypes[] =
    {
        SIP(L"Reserved 0", SMBIOS_MCHI_PROTOCOL_TYPE_RESERVED_0),
        SIP(L"Reserved 1", SMBIOS_MCHI_PROTOCOL_TYPE_RESERVED_1),
        SIP(L"IPMI", SMBIOS_MCHI_PROTOCOL_TYPE_IPMI),
        SIP(L"MCTP", SMBIOS_MCHI_PROTOCOL_TYPE_MCTP),
        SIP(L"Refresh over IP", SMBIOS_MCHI_PROTOCOL_TYPE_REFRESH_OVER_IP),
        SIP(L"OEM defined", SMBIOS_MCHI_PROTOCOL_TYPE_OEM_DEFINED),
    };

    ET_SMBIOS_GROUP(L"Management controller host interface");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MCHInterface, Type))
        ET_SMBIOS_ENUM(L"Type", Entry->MCHInterface.Type, interfaceTypes);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MCHInterface, Length))
    {
        PPH_STRING data;

        data = PhBufferToHexString(Entry->MCHInterface.Data, Entry->MCHInterface.Length);
        EtAddSMBIOSItem(Context, group, L"Data", PhGetString(data));
        PhDereferenceObject(data);
    }

    // SMBIOS_MCHI_PROTOCOL_RECORDS

    PVOID end;
    PVOID pointer;
    PSMBIOS_MCHI_PROTOCOL_RECORDS records;
    PSMBIOS_MCHI_PROTOCOL_RECORD record;

    end = PTR_ADD_OFFSET(Entry, Entry->Header.Length);
    pointer = PTR_ADD_OFFSET(Entry->MCHInterface.Data, Entry->MCHInterface.Length);

    if (pointer >= end)
        return;

    records = pointer;
    pointer = PTR_ADD_OFFSET(pointer, sizeof(SMBIOS_MCHI_PROTOCOL_RECORDS));

    if (pointer >= end)
        return;

    record = records->Records;

    for (UCHAR i = 0; i < records->Count; i++)
    {
        PH_FORMAT format[3];
        PPH_STRING name;

        PhInitFormatC(&format[0], L'#');
        PhInitFormatU(&format[1], i + 1);

        PhInitFormatS(&format[2], L" Protocol type");
        name = PhFormat(format, 3, 20);
        ET_SMBIOS_ENUM(PhGetString(name), record->Type, protocolTypes);
        PhDereferenceObject(name);

        if (record->Length > 0)
        {
            PPH_STRING data;

            // TODO format data by protocol type instead of hex buffer
            data = PhBufferToHexString(record->Data, record->Length);

            PhInitFormatS(&format[2], L" Data");
            name = PhFormat(format, 3, 20);
            EtAddSMBIOSItem(Context, group, PhGetString(name), PhGetString(data));
            PhDereferenceObject(name);

            PhDereferenceObject(data);
        }

        pointer = PTR_ADD_OFFSET(pointer, record->Length);
        pointer = PTR_ADD_OFFSET(pointer, FIELD_OFFSET(SMBIOS_MCHI_PROTOCOL_RECORD, Length));
        record = pointer;
    }
}

VOID EtSMBIOSTPMDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    UCHAR major;

    static const PH_ACCESS_ENTRY characteristics[] =
    {
        ET_SMBIOS_FLAG((ULONG)SMBIOS_TPM_DEVICE_CONFIGURABLE_VIA_FIRMWARE_UPDATE, L"Configurable via firmware update"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_TPM_DEVICE_CONFIGURABLE_VIA_SOFTWARE_UPDATE, L"Configurable via software update"),
        ET_SMBIOS_FLAG((ULONG)SMBIOS_TPM_DEVICE_CONFIGURABLE_VIA_PROPRIETARY_UPDATE, L"Configurable via proprietary update"),
    };

    ET_SMBIOS_GROUP(L"TPM device");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, VendorID))
    {
        WCHAR vendor[5];

        vendor[0] = (WCHAR)Entry->TPMDevice.VendorID[0];
        vendor[1] = (WCHAR)Entry->TPMDevice.VendorID[1];
        vendor[2] = (WCHAR)Entry->TPMDevice.VendorID[2];
        vendor[3] = (WCHAR)Entry->TPMDevice.VendorID[3];
        vendor[4] = UNICODE_NULL;

        EtAddSMBIOSItem(Context, group, L"Vendor ID", vendor);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, MajorSpecVersion))
    {
        major = Entry->TPMDevice.MajorSpecVersion;
        ET_SMBIOS_UINT32(L"Major version", Entry->TPMDevice.MajorSpecVersion);
    }
    else
    {
        major = 0;
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, MinorSpecVersion))
        ET_SMBIOS_UINT32(L"Minor version", Entry->TPMDevice.MinorSpecVersion);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, FirmwareVersion1))
    {
        ULONG64 version;

        version = Entry->TPMDevice.FirmwareVersion1;

        if (major >= 2)
            version <<= 32;

        if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, FirmwareVersion2))
            version |= Entry->TPMDevice.FirmwareVersion2;

        ET_SMBIOS_UINT64IX(L"Firmware version", version);
    }

    if (PH_SMBIOS_CONTAINS_STRING(Entry, TPMDevice, Description))
        ET_SMBIOS_STRING(L"Description", Entry->TPMDevice.Description);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, Characteristics))
        ET_SMBIOS_FLAGS(L"Characteristics", (ULONG)Entry->TPMDevice.Characteristics, characteristics);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, TPMDevice, OEMDefined))
        ET_SMBIOS_UINT32IX(L"OEM defined", Entry->TPMDevice.OEMDefined);
}

VOID EtSMBIOSProcessorAdditional(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR types[] =
    {
        SIP(L"x86", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_X86),
        SIP(L"x64", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_X64),
        SIP(L"IA-64", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_IA64),
        SIP(L"ARM 32", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_ARM32),
        SIP(L"ARM 64", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_ARM64),
        SIP(L"RISC-V 32", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_RISCV32),
        SIP(L"RISC-V 64", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_RISCV64),
        SIP(L"RISC-V 128", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_RISCV128),
        SIP(L"LoongArch 32", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_LOONGARCH32),
        SIP(L"LoongArch 64", SMBIOS_PROCESSOR_ARCHITECTURE_TYPE_LOONGARCH64),
    };

    ET_SMBIOS_GROUP(L"Processor additional");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ProcessorAdditional, Handle))
        ET_SMBIOS_UINT32IX(L"Associated handle", Entry->ProcessorAdditional.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, ProcessorAdditional, Blocks))
    {
        ULONG count;
        PVOID end;
        PSMBIOS_PROCESSOR_SPECIFIC_BLOCK block;

        count = 0;
        end = PTR_ADD_OFFSET(Entry, Entry->Header.Length);
        block = Entry->ProcessorAdditional.Blocks;

        while ((ULONG_PTR)block < (ULONG_PTR)end)
        {
            PH_FORMAT format[3];
            PPH_STRING name;

            PhInitFormatC(&format[0], L'#');
            PhInitFormatU(&format[1], ++count);

            PhInitFormatS(&format[2], L" Type");
            name = PhFormat(format, 3, 20);
            ET_SMBIOS_ENUM(PhGetString(name), block->Type, types);
            PhDereferenceObject(name);

            if (block->Length > 0)
            {
                PPH_STRING data;

                // TODO format data by processor type instead of hex buffer
                data = PhBufferToHexString(block->Data, block->Length);

                PhInitFormatS(&format[2], L" Data");
                name = PhFormat(format, 3, 20);
                EtAddSMBIOSItem(Context, group, PhGetString(name), PhGetString(data));
                PhDereferenceObject(name);

                PhDereferenceObject(data);
            }

            block = PTR_ADD_OFFSET(block, block->Length);
            block = PTR_ADD_OFFSET(block, FIELD_OFFSET(SMBIOS_PROCESSOR_SPECIFIC_BLOCK, Data));
        }
    }
}

VOID EtSMBIOSFirmwareInventory(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_ACCESS_ENTRY flags[] =
    {
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_INVENTORY_FLAG_UPDATABLE, L"Updatable"),
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_INVENTORY_FLAG_WRITE_PROTECTED, L"Write protected"),
    };

    static const PH_KEY_VALUE_PAIR states[] =
    {
        SIP(L"Other", SMBIOS_FIRMWARE_INVENTORY_STATE_OTHER),
        SIP(L"Unknown", SMBIOS_FIRMWARE_INVENTORY_STATE_UNKNOWN),
        SIP(L"Disabled", SMBIOS_FIRMWARE_INVENTORY_STATE_DISABLED),
        SIP(L"Enabled", SMBIOS_FIRMWARE_INVENTORY_STATE_ENABLED),
        SIP(L"Absent", SMBIOS_FIRMWARE_INVENTORY_STATE_ABSENT),
        SIP(L"Standby (offline)", SMBIOS_FIRMWARE_INVENTORY_STATE_STANDBY_OFFLINE),
        SIP(L"Standby (spare)", SMBIOS_FIRMWARE_INVENTORY_STATE_STANDBY_SPARE),
        SIP(L"Offline", SMBIOS_FIRMWARE_INVENTORY_STATE_OFFLINE),
    };

    ET_SMBIOS_GROUP(L"Firmware inventory");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, FirmwareInventory, ComponentName))
        ET_SMBIOS_STRING(L"Component name", Entry->FirmwareInventory.ComponentName);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, FirmwareInventory, Version))
        ET_SMBIOS_STRING(L"Version", Entry->FirmwareInventory.Version);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, FirmwareInventory, VersionFormat))
        ET_SMBIOS_UINT32(L"Version format", Entry->FirmwareInventory.VersionFormat);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, FirmwareInventory, Identifier))
        ET_SMBIOS_STRING(L"Identifier", Entry->FirmwareInventory.Identifier);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, FirmwareInventory, IdentifierFormat))
        ET_SMBIOS_UINT32(L"Version format", Entry->FirmwareInventory.VersionFormat);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, FirmwareInventory, ReleaseDate))
        ET_SMBIOS_STRING(L"Release date", Entry->FirmwareInventory.ReleaseDate);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, FirmwareInventory, Manufacturer))
        ET_SMBIOS_STRING(L"Manufacturer", Entry->FirmwareInventory.Manufacturer);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, FirmwareInventory, LowestSupportedVersion))
        ET_SMBIOS_STRING(L"Lowest supported version", Entry->FirmwareInventory.LowestSupportedVersion);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, FirmwareInventory, ImageSize) &&
        Entry->FirmwareInventory.ImageSize != ULONG64_MAX)
    {
        ET_SMBIOS_SIZE(L"Image size", Entry->FirmwareInventory.ImageSize);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, FirmwareInventory, Characteristics))
        ET_SMBIOS_FLAGS(L"Characteristics", Entry->FirmwareInventory.Characteristics, flags);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, FirmwareInventory, State))
        ET_SMBIOS_ENUM(L"State", Entry->FirmwareInventory.State, states);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, FirmwareInventory, AssociatedComponents) &&
        Entry->FirmwareInventory.AssociatedComponents > 0)
    {
        PH_STRING_BUILDER sb;
        PPH_STRING string;

        PhInitializeStringBuilder(&sb, 10);

        for (UCHAR i = 0; i < Entry->FirmwareInventory.AssociatedComponents; i++)
        {
            WCHAR buffer[PH_PTR_STR_LEN_1];

            PhPrintUInt32IX(buffer, Entry->FirmwareInventory.AssociatedComponentHandles[i]);
            PhAppendStringBuilder2(&sb, L"0x");
            PhAppendStringBuilder2(&sb, buffer);
            PhAppendStringBuilder2(&sb, L", ");
        }

        if (PhEndsWithString2(sb.String, L", ", FALSE))
            PhRemoveEndStringBuilder(&sb, 2);

        string = PhFinalStringBuilderString(&sb);

        EtAddSMBIOSItem(Context, group, L"Associated component handles", PhGetString(string));

        PhDereferenceObject(string);
    }
}

VOID EtSMBIOSStringProperty(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    static const PH_KEY_VALUE_PAIR identifiers[] =
    {
        SIP(L"Reserved", SMBIOS_STRING_PROPERTY_ID_RESERVED),
        SIP(L"UEFI device path", SMBIOS_STRING_PROPERTY_ID_UEIF_DEVICE_PATH),
    };

    ET_SMBIOS_GROUP(L"String property");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, StringProperty, Identifier))
        ET_SMBIOS_UINT32(L"Identifier", Entry->StringProperty.Identifier);

    if (PH_SMBIOS_CONTAINS_STRING(Entry, StringProperty, String))
        ET_SMBIOS_STRING(L"String", Entry->StringProperty.String);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, StringProperty, ParentHandle))
        ET_SMBIOS_UINT32IX(L"Parent handle", Entry->StringProperty.ParentHandle);
}

VOID EtSMBIOSInactive(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Inactive");

    ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);
}

VOID EtSMBIOSUndefinedType(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    if (Context->ShowUndefinedTypes)
    {
        PH_FORMAT format[2];
        PPH_STRING type;

        PhInitFormatS(&format[0], L"Type ");
        PhInitFormatU(&format[1], Entry->Header.Type);
        type = PhFormat(format, 2, 10);

        ET_SMBIOS_GROUP(PhGetString(type));
        ET_SMBIOS_UINT32IX(L"Handle", Entry->Header.Handle);
        ET_SMBIOS_UINT32(L"Length", Entry->Header.Length);

        PhDereferenceObject(type);
    }
}

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
BOOLEAN NTAPI EtEnumerateSMBIOSEntriesCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PSMBIOS_WINDOW_CONTEXT context;

    assert(Context);

    context = Context;

    switch (Entry->Header.Type)
    {
    case SMBIOS_FIRMWARE_INFORMATION_TYPE:
        EtSMBIOSFirmware(EnumHandle, Entry, context);
        break;
    case SMBIOS_SYSTEM_INFORMATION_TYPE:
        EtSMBIOSSystem(EnumHandle, Entry, context);
        break;
    case SMBIOS_BASEBOARD_INFORMATION_TYPE:
        EtSMBIOSBaseboard(EnumHandle, Entry, context);
        break;
    case SMBIOS_CHASSIS_INFORMATION_TYPE:
        EtSMBIOSChassis(EnumHandle, Entry, context);
        break;
    case SMBIOS_PROCESSOR_INFORMATION_TYPE:
        EtSMBIOSProcessor(EnumHandle, Entry, context);
        break;
    case SMBIOS_MEMORY_CONTROLLER_INFORMATION_TYPE:
        EtSMBIOSMemoryController(EnumHandle, Entry, context);
        break;
    case SMBIOS_MEMORY_MODULE_INFORMATION_TYPE:
        EtSMBIOSMemoryModule(EnumHandle, Entry, context);
        break;
    case SMBIOS_CACHE_INFORMATION_TYPE:
        EtSMBIOSCache(EnumHandle, Entry, context);
        break;
    case SMBIOS_PORT_CONNECTOR_INFORMATION_TYPE:
        EtSMBIOSPortConnector(EnumHandle, Entry, context);
        break;
    case SMBIOS_SYSTEM_SLOT_INFORMATION_TYPE:
        EtSMBIOSSystemSlot(EnumHandle, Entry, context);
        break;
    case SMBIOS_ON_BOARD_DEVICE_INFORMATION_TYPE:
        EtSMBIOSOnBoardDevice(EnumHandle, Entry, context);
        break;
    case SMBIOS_OEM_STRING_INFORMATION_TYPE:
        EtSMBIOSOemString(EnumHandle, Entry, context);
        break;
    case SMBIOS_SYSTEM_CONFIGURATION_OPTION_INFORMATION_TYPE:
        EtSMBIOSSystemConfigurationOption(EnumHandle, Entry, context);
        break;
    case SMBIOS_FIRMWARE_LANGUAGE_INFORMATION_TYPE:
        EtSMBIOSFirmwareLanguage(EnumHandle, Entry, context);
        break;
    case SMBIOS_GROUP_ASSOCIATION_INFORMATION_TYPE:
        EtSMBIOSGroupAssociation(EnumHandle, Entry, context);
        break;
    case SMBIOS_SYSTEM_EVENT_LOG_INFORMATION_TYPE:
        EtSMBIOSSystemEventLog(EnumHandle, Entry, context);
        break;
    case SMBIOS_PHYSICAL_MEMORY_ARRAY_INFORMATION_TYPE:
        EtSMBIOSPhysicalMemoryArray(EnumHandle, Entry, context);
        break;
    case SMBIOS_MEMORY_DEVICE_INFORMATION_TYPE:
        EtSMBIOSMemoryDevice(EnumHandle, Entry, context);
        break;
    case SMBIOS_32_BIT_MEMORY_ERROR_INFORMATION_TYPE:
        EtSMBIOS32BitMemoryError(EnumHandle, Entry, context);
        break;
    case SMBIOS_MEMORY_ARRAY_MAPPED_ADDRESS_INFORMATION_TYPE:
        EtSMBIOSMemoryArrayMappedAddress(EnumHandle, Entry, context);
        break;
    case SMBIOS_MEMORY_DEVICE_MAPPED_ADDRESS_INFORMATION_TYPE:
        EtSMBIOSMemoryDeviceMappedAddress(EnumHandle, Entry, context);
        break;
    case SMBIOS_BUILT_IN_POINTING_DEVICE_INFORMATION_TYPE:
        EtSMBIOSBuiltInPointingDevice(EnumHandle, Entry, context);
        break;
    case SMBIOS_PORTABLE_BATTERY_INFORMATION_TYPE:
        EtSMBIOSPortableBattery(EnumHandle, Entry, context);
        break;
    case SMBIOS_SYSTEM_RESET_INFORMATION_TYPE:
        EtSMBIOSSystemReset(EnumHandle, Entry, context);
        break;
    case SMBIOS_HARDWARE_SECURITY_INFORMATION_TYPE:
        EtSMBIOSHardwareSecurity(EnumHandle, Entry, context);
        break;
    case SMBIOS_SYSTEM_POWER_CONTROLS_INFORMATION_TYPE:
        EtSMBIOSSystemPowerControls(EnumHandle, Entry, context);
        break;
    case SMBIOS_VOLTAGE_PROBE_INFORMATION_TYPE:
        EtSMBIOSVoltageProbe(EnumHandle, Entry, context);
        break;
    case SMBIOS_COOLING_DEVICE_INFORMATION_TYPE:
        EtSMBIOSCoolingDevice(EnumHandle, Entry, context);
        break;
    case SMBIOS_TEMPERATURE_PROBE_INFORMATION_TYPE:
        EtSMBIOSTemperatureProbe(EnumHandle, Entry, context);
        break;
    case SMBIOS_ELECTRICAL_CURRENT_PROBE_INFORMATION_TYPE:
        EtSMBIOSElectricalCurrentProbe(EnumHandle, Entry, context);
        break;
    case SMBIOS_OUT_OF_BAND_REMOTE_ACCESS_INFORMATION_TYPE:
        EtSMBIOSOutOfBandRemoteAccess(EnumHandle, Entry, context);
        break;
    case SMBIOS_SYSTEM_BOOT_INFORMATION_TYPE:
        EtSMBIOSSystemBoot(EnumHandle, Entry, context);
        break;
    case SMBIOS_64_BIT_MEMORY_ERROR_INFORMATION_TYPE:
        EtSMBIOS64BitMemoryError(EnumHandle, Entry, context);
        break;
    case SMBIOS_MANAGEMENT_DEVICE_INFORMATION_TYPE:
        EtSMBIOSManagementDevice(EnumHandle, Entry, context);
        break;
    case SMBIOS_MANAGEMENT_DEVICE_COMPONENT_INFORMATION_TYPE:
        EtSMBIOSManagementDeviceComponent(EnumHandle, Entry, context);
        break;
    case SMBIOS_MANAGEMENT_DEVICE_THRESHOLD_INFORMATION_TYPE:
        EtSMBIOSManagementDeviceThreshold(EnumHandle, Entry, context);
        break;
    case SMBIOS_MEMORY_CHANNEL_INFORMATION_TYPE:
        EtSMBIOSMemoryChannel(EnumHandle, Entry, context);
        break;
    case SMBIOS_IPMI_DEVICE_INFORMATION_TYPE:
        EtSMBIOSIPMIDevice(EnumHandle, Entry, context);
        break;
    case SMBIOS_SYSTEM_POWER_SUPPLY_INFORMATION_TYPE:
        EtSMBIOSSystemPowerSupply(EnumHandle, Entry, context);
        break;
    case SMBIOS_ADDITIONAL_INFORMATION_TYPE:
        EtSMBIOSAdditionalInformation(EnumHandle, Entry, context);
        break;
    case SMBIOS_ONBOARD_DEVICE_INFORMATION_TYPE:
        EtSMBIOSOnboardDevice(EnumHandle, Entry, context);
        break;
    case SMBIOS_MCHI_INFORMATION_TYPE:
        EtSMBIOSMCHInterface(EnumHandle, Entry, context);
        break;
    case SMBIOS_TPM_DEVICE_INFORMATION_TYPE:
        EtSMBIOSTPMDevice(EnumHandle, Entry, context);
        break;
    case SMBIOS_PROCESSOR_ADDITIONAL_INFORMATION_TYPE:
        EtSMBIOSProcessorAdditional(EnumHandle, Entry, context);
        break;
    case SMBIOS_FIRMWARE_INVENTORY_INFORMATION_TYPE:
        EtSMBIOSFirmwareInventory(EnumHandle, Entry, context);
        break;
    case SMBIOS_STRING_PROPERTY_TYPE:
        EtSMBIOSStringProperty(EnumHandle, Entry, context);
        break;
    case SMBIOS_INACTIVE_TYPE:
        EtSMBIOSInactive(EnumHandle, Entry, context);
        break;
    case SMBIOS_END_OF_TABLE_TYPE:
        NOTHING;
        break;
    default:
        EtSMBIOSUndefinedType(EnumHandle, Entry, context);
        break;
    }

    return FALSE;
}

VOID EtEnumerateSMBIOSEntries(
    PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);
    ListView_EnableGroupView(Context->ListViewHandle, TRUE);

    PhEnumSMBIOS(EtEnumerateSMBIOSEntriesCallback, Context);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK EtSMBIOSDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSMBIOS_WINDOW_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(SMBIOS_WINDOW_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ParentWindowHandle = (HWND)lParam;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_SMBIOS_INFO);

            context->ShowUndefinedTypes = !!PhGetIntegerSetting(SETTING_NAME_SMBIOS_SHOW_UNDEFINED_TYPES);

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PhLoadListViewColumnsFromSetting(SETTING_NAME_SMBIOS_INFO_COLUMNS, context->ListViewHandle);
            if (PhGetIntegerPairSetting(SETTING_NAME_SMBIOS_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_SMBIOS_WINDOW_POSITION, SETTING_NAME_SMBIOS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            EtEnumerateSMBIOSEntries(context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_SMBIOS_INFO_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_SMBIOS_WINDOW_POSITION, SETTING_NAME_SMBIOS_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;

    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID EtShowSMBIOSDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SMBIOS),
        NULL,
        EtSMBIOSDlgProc,
        ParentWindowHandle
        );
}
