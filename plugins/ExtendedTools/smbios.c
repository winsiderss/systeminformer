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
    _In_ PWSTR Name
    )
{
    ULONG group = Context->GroupCounter++;
    PhAddListViewGroup(Context->ListViewHandle, group, Name);
    return group;
}

VOID EtAddSMBIOSItem(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PWSTR Name,
    _In_ PWSTR Value
    )
{
    ULONG index = Context->IndexCounter++;
    PhAddListViewGroupItem(Context->ListViewHandle, Group, index, Name, NULL);
    PhSetListViewSubItem(Context->ListViewHandle, index, 1, Value);
}

VOID EtAddSMBIOSUInt32(
    _In_ PSMBIOS_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PWSTR Name,
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
    _In_ PWSTR Name,
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
    _In_ PWSTR Name,
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
    _In_ PWSTR Name,
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
    _In_ PWSTR Name,
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

#define ET_SMBIOS_GROUP(n)              ULONG group = EtAddSMBIOSGroup(Context, n)
#define ET_SMBIOS_UINT32(n, v)          EtAddSMBIOSUInt32(Context, group, n, v)
#define ET_SMBIOS_UINT32IX(n, v)        EtAddSMBIOSUInt32IX(Context, group, n, v)
#define ET_SMBIOS_STRING(n, v)          EtAddSMBIOSString(Context, group, n, EnumHandle, v)
#define ET_SMBIOS_SIZE(n, v)            EtAddSMBIOSSize(Context, group, n, v)
#define ET_SMBIOS_FLAG(x, n)            { TEXT(#x), x, FALSE, FALSE, n }
#define ET_SMBIOS_FLAGS(n, v, f)        EtAddSMBIOSFlags(Context, group, n, f, RTL_NUMBER_OF(f), NULL, 0, v)
#define ET_SMBIOS_FLAGS64(n, v, fl, fh) EtAddSMBIOSFlags(Context, group, n, fl, RTL_NUMBER_OF(fl), fh, RTL_NUMBER_OF(fh), v)

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
    ET_SMBIOS_GROUP(L"System");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSBaseboard(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Baseboard");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSChassis(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Chassis");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSProcessor(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Processor");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSMemoryController(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Memory controller");
    ET_SMBIOS_TODO();
}

VOID EtSMBIOSMemoryModule(
    _In_ ULONG_PTR EnumHandle,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_ PSMBIOS_WINDOW_CONTEXT Context
    )
{
    ET_SMBIOS_GROUP(L"Memory module");
    ET_SMBIOS_TODO();
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
