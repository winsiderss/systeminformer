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
    WCHAR buffer[PH_PTR_STR_LEN_1];

    PhPrintUInt32IX(buffer, Value);
    EtAddSMBIOSItem(Context, Group, Name, buffer);
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

    PhGetSMBIOSString(EnumHandle, Index, &string);
    EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
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

VOID EtAddSMBIOSFlags(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_opt_ const PH_ACCESS_ENTRY* EntriesLow,
    _In_ ULONG CountLow,
    _In_opt_ const PH_ACCESS_ENTRY* EntriesHigh,
    _In_ ULONG CountHigh,
    _In_ ULONG64 Value
    )
{
    PH_FORMAT format[4];
    PPH_STRING string;
    PPH_STRING low;
    PPH_STRING high;

    if (EntriesLow && CountLow)
        low = PhGetAccessString((ULONG)Value, (PPH_ACCESS_ENTRY)EntriesLow, CountLow);
    else
        low = PhReferenceEmptyString();

    if (EntriesHigh && CountHigh)
        high = PhGetAccessString((ULONG)(Value >> 32), (PPH_ACCESS_ENTRY)EntriesHigh, CountHigh);
    else
        high = PhReferenceEmptyString();

    if (low->Length && high->Length)
    {
        PhInitFormatSR(&format[0], low->sr);
        PhInitFormatS(&format[1], L", ");
        PhInitFormatSR(&format[2], high->sr);
        string = PhFormat(format, 3, 10);
    }
    else if (low->Length)
    {
        string = PhReferenceObject(low);
    }
    else
    {
        string = PhReferenceObject(high);
    }

    PhInitFormatSR(&format[0], string->sr);
    PhInitFormatS(&format[1], L" (0x");
    PhInitFormatI64X(&format[2], Value);
    PhInitFormatS(&format[3], L")");

    PhMoveReference(&string, PhFormat(format, 4, 10));
    EtAddSMBIOSItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);

    PhDereferenceObject(low);
    PhDereferenceObject(high);
}

VOID EtAddSMBIOSEnum(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ PPCH_KEY_VALUE_PAIR Values,
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
#define ET_SMBIOS_STRING(n, v)          EtAddSMBIOSString(Context, group, n, EnumHandle, v)
#define ET_SMBIOS_SIZE(n, v)            EtAddSMBIOSSize(Context, group, n, v)
#define ET_SMBIOS_FLAG(x, n)            { TEXT(#x), x, FALSE, FALSE, n }
#define ET_SMBIOS_FLAGS(n, v, f)        EtAddSMBIOSFlags(Context, group, n, f, RTL_NUMBER_OF(f), NULL, 0, v)
#define ET_SMBIOS_FLAGS64(n, v, fl, fh) EtAddSMBIOSFlags(Context, group, n, fl, RTL_NUMBER_OF(fl), fh, RTL_NUMBER_OF(fh), v)
#define ET_SMBIOS_ENUM(n, v, e)         EtAddSMBIOSEnum(Context, group, n, e, sizeof(e), v);

#ifdef DEBUG // WIP(jxy-s)
#define ET_SMBIOS_TODO() EtAddSMBIOSItem(Context, group, L"TODO", L"")
#else
#define ET_SMBIOS_TODO() DBG_UNREFERENCED_LOCAL_VARIABLE(group)
#endif

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
        ET_SMBIOS_FLAG(SMBIOS_FIRMWARE_FLAG_2_ZIP_BOOT_SUPPORTED, L"Zip boot"),
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
        ET_SMBIOS_FLAGS(L"Characteristics", Entry->Firmware.Characteristics, characteristics);

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
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatU(&format[0], Entry->Chassis.Height);
        PhInitFormatC(&format[1], L'U');
        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Height", PhGetString(string));
        PhDereferenceObject(string);
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
    {
        PH_FORMAT format;
        PPH_STRING string;

        PhInitFormatI64X(&format, Entry->Processor.Identifier);
        string = PhFormat(&format, 1, 10);
        EtAddSMBIOSItem(Context, group, L"Identifier", PhGetString(string));
        PhDereferenceObject(string);
    }

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
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatU(&format[0], Entry->Processor.ExternalClock);
        PhInitFormatS(&format[1], L" MHz");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"External clock", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, MaxSpeed) &&
        Entry->Processor.MaxSpeed != 0)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatU(&format[0], Entry->Processor.MaxSpeed);
        PhInitFormatS(&format[1], L" MHz");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Max speed", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Processor, CurrentSpeed) &&
        Entry->Processor.CurrentSpeed != 0)
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatU(&format[0], Entry->Processor.CurrentSpeed);
        PhInitFormatS(&format[1], L" MHz");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Current speed", PhGetString(string));
        PhDereferenceObject(string);
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
    {
        PH_FORMAT format[2];
        PPH_STRING string;

        PhInitFormatU(&format[0], Entry->MemoryModule.CurrentSpeed);
        PhInitFormatS(&format[1], L" ns");

        string = PhFormat(format, 2, 10);
        EtAddSMBIOSItem(Context, group, L"Current speed", PhGetString(string));
        PhDereferenceObject(string);
    }

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryModule, MemoryType))
        ET_SMBIOS_FLAGS(L"Memory type", Entry->MemoryModule.MemoryType, types);

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryModule, InstalledSize))
    {
        PPH_STRING string;

        if (Entry->MemoryModule.InstalledSize.Size == SMBIOS_MEMORY_MDOULE_SIZE_VALUE_NOT_DETERMINABLE)
            string = PhCreateString(L"Not determinable");
        else if (Entry->MemoryModule.InstalledSize.Size == SMBIOS_MEMORY_MDOULE_SIZE_VALUE_NOT_ENABLED)
            string = PhCreateString(L"Not enabled");
        else if (Entry->MemoryModule.InstalledSize.Size == SMBIOS_MEMORY_MDOULE_SIZE_VALUE_NOT_INSTALLED)
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

        if (Entry->MemoryModule.EnabledSize.Size == SMBIOS_MEMORY_MDOULE_SIZE_VALUE_NOT_DETERMINABLE)
            string = PhCreateString(L"Not determinable");
        else if (Entry->MemoryModule.EnabledSize.Size == SMBIOS_MEMORY_MDOULE_SIZE_VALUE_NOT_ENABLED)
            string = PhCreateString(L"Not enabled");
        else if (Entry->MemoryModule.EnabledSize.Size == SMBIOS_MEMORY_MDOULE_SIZE_VALUE_NOT_INSTALLED)
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
    ET_SMBIOS_GROUP(L"Cache");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSPortConnector(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Port connector");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSSystemSlot(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System slot");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSOuBoardDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Board device");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSOemString(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"OEM string");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSSystemConfigurationOption(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System configuration option");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSFirmwareLanguage(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Firmware language");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSGroupAssociation(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Group association");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSSystemEventLog(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System event log");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSPhysicalMemoryArray(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Physical memory array");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSMemoryDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Memory device");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOS32BitMemoryError(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"32-bit memory error");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSMemoryArrayMappedAddress(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Memory array mapped address");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSMemoryDeviceMappedAddress(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Memory device mapped address");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSBuiltInPointingDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Built-in pointing device");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSPortableBattery(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Portable battery");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSSystemReset(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System reset");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSHardwareSecurity(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Hardware security");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSSystemPowerControls(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System power controls");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSVoltageProbe(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Voltage probe");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSCoolingDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Cooling device");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSTemperatureProbe(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Temperature probe");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSElectricalCurrentProbe(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Electrical current probe");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSOutOfBandRemoteAccess(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Out-of-band remote access");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSSystemBoot(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System boot");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOS64BitMemoryError(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"64-bit memory error");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSManagementDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Management device");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSManagementDeviceComponent(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Management device component");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSManagementDeviceThreshold(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Management device threshold");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSMemoryChannel(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Memory channel");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSIPMIDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"IPMI device");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSSystemPowerSupply(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"System power supply");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSAdditionalInformation(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Additional information");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSOnboardDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Onboard device");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSMCHInterface(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Management controller host interface");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSTPMDevice(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"TPM device");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSProcessorAdditional(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Processor additional");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSFirmwareInventory(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Firmware inventory");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSStringProperty(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"String property");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSInactive(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Inactive");
    ET_SMBIOS_TODO();
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
        EtSMBIOSOnboardDevice(EnumHandle, Entry, context);
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
