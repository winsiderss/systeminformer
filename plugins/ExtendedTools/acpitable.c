/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include "exttools.h"

// ACPI table structures. These map to firmware-provided structures (1-byte
// packed). Layouts are taken from the WDK header <acpitabl.h>; we declare a
// local subset rather than including the WDK header (which depends on WDK-only
// context). See also phnt\include\smbios.h for the same packed-struct style.
#include <pshpack1.h>

#define ET_ACPI_MAX_OEM_ID     6
#define ET_ACPI_MAX_TABLE_ID   8
#define ET_ACPI_MAX_CREATOR_ID 4

typedef struct _ET_ACPI_HEADER
{
    ULONG Signature;
    ULONG Length;
    UCHAR Revision;
    UCHAR Checksum;
    CHAR OEMID[ET_ACPI_MAX_OEM_ID];
    CHAR OEMTableID[ET_ACPI_MAX_TABLE_ID];
    ULONG OEMRevision;
    CHAR CreatorID[ET_ACPI_MAX_CREATOR_ID];
    ULONG CreatorRev;
} ET_ACPI_HEADER, * PET_ACPI_HEADER;

typedef struct _ET_ACPI_GEN_ADDR
{
    UCHAR AddressSpaceID;
    UCHAR BitWidth;
    UCHAR BitOffset;
    UCHAR AccessSize;
    ULONG64 Address;
} ET_ACPI_GEN_ADDR, * PET_ACPI_GEN_ADDR;

typedef struct _ET_ACPI_FADT
{
    ET_ACPI_HEADER Header;
    ULONG Facs;
    ULONG Dsdt;
    UCHAR IntModel;
    UCHAR PmProfile;
    USHORT SciIntVector;
    ULONG SmiCmdIoPort;
    UCHAR AcpiOnValue;
    UCHAR AcpiOffValue;
    UCHAR S4BiosReq;
    UCHAR PstateControl;
    ULONG Pm1aEvtBlkIoPort;
    ULONG Pm1bEvtBlkIoPort;
    ULONG Pm1aCtrlBlkIoPort;
    ULONG Pm1bCtrlBlkIoPort;
    ULONG Pm2CtrlBlkIoPort;
    ULONG PmTmrBlkIoPort;
    ULONG Gp0BlkIoPort;
    ULONG Gp1BlkIoPort;
    UCHAR Pm1EvtLen;
    UCHAR Pm1CtrlLen;
    UCHAR Pm2CtrlLen;
    UCHAR PmTmrLen;
    UCHAR Gp0BlkLen;
    UCHAR Gp1BlkLen;
    UCHAR Gp1Base;
    UCHAR CstateControl;
    USHORT Lvl2Latency;
    USHORT Lvl3Latency;
    USHORT FlushSize;
    USHORT FlushStride;
    UCHAR DutyOffset;
    UCHAR DutyWidth;
    UCHAR DayAlarmIndex;
    UCHAR MonthAlarmIndex;
    UCHAR CenturyAlarmIndex;
    USHORT BootArch;
    UCHAR Reserved3[1];
    ULONG Flags;
} ET_ACPI_FADT, * PET_ACPI_FADT;

typedef struct _ET_ACPI_FACS
{
    ULONG Signature;
    ULONG Length;
    ULONG HardwareSignature;
    ULONG FirmwareWakingVector;
    ULONG GlobalLock;
    ULONG Flags;
    ULONG64 XFirmwareWakingVector;
    UCHAR Version;
} ET_ACPI_FACS, * PET_ACPI_FACS;

typedef struct _ET_ACPI_MADT
{
    ET_ACPI_HEADER Header;
    ULONG LocalApicAddress;
    ULONG Flags;
    // Followed by a variable list of interrupt-controller structures.
} ET_ACPI_MADT, * PET_ACPI_MADT;

typedef struct _ET_ACPI_MADT_ENTRY
{
    UCHAR Type;
    UCHAR Length;
} ET_ACPI_MADT_ENTRY, * PET_ACPI_MADT_ENTRY;

// --- EXPANDED MADT SUB-STRUCTURES ---
typedef struct _ET_ACPI_MADT_LOCAL_APIC
{
    UCHAR Type;
    UCHAR Length;
    UCHAR ProcessorId;
    UCHAR ApicId;
    ULONG Flags;
} ET_ACPI_MADT_LOCAL_APIC, * PET_ACPI_MADT_LOCAL_APIC;

typedef struct _ET_ACPI_MADT_IO_APIC
{
    UCHAR Type;
    UCHAR Length;
    UCHAR IoApicId;
    UCHAR Reserved;
    ULONG Address;
    ULONG GlobalSystemInterruptBase;
} ET_ACPI_MADT_IO_APIC, * PET_ACPI_MADT_IO_APIC;

typedef struct _ET_ACPI_MADT_INT_SRC_OVR
{
    UCHAR Type;
    UCHAR Length;
    UCHAR Bus;
    UCHAR Source;
    ULONG GlobalSystemInterrupt;
    USHORT Flags;
} ET_ACPI_MADT_INT_SRC_OVR, * PET_ACPI_MADT_INT_SRC_OVR;

typedef struct _ET_ACPI_MADT_X2APIC
{
    UCHAR Type;
    UCHAR Length;
    USHORT Reserved;
    ULONG X2ApicId;
    ULONG Flags;
    ULONG Uid;
} ET_ACPI_MADT_X2APIC, * PET_ACPI_MADT_X2APIC;

// --- EXPANDED SRAT SUB-STRUCTURES ---
typedef struct _ET_ACPI_SRAT_PROCESSOR_APIC
{
    UCHAR Type;
    UCHAR Length;
    UCHAR ProximityDomainLo;
    UCHAR ApicId;
    ULONG Flags;
    UCHAR LocalSapicEid;
    UCHAR ProximityDomainHi[3];
    ULONG ClockDomain;
} ET_ACPI_SRAT_PROCESSOR_APIC, * PET_ACPI_SRAT_PROCESSOR_APIC;

typedef struct _ET_ACPI_SRAT_MEMORY
{
    UCHAR Type;
    UCHAR Length;
    ULONG ProximityDomain;
    USHORT Reserved1;
    ULONG64 BaseAddress;
    ULONG64 Length2;
    ULONG Reserved2;
    ULONG Flags;
    ULONG64 Reserved3;
} ET_ACPI_SRAT_MEMORY, * PET_ACPI_SRAT_MEMORY;
// ------------------------------------

typedef struct _ET_ACPI_HPET
{
    ET_ACPI_HEADER Header;
    ULONG EventTimerBlockId;
    ET_ACPI_GEN_ADDR Address;
    UCHAR HpetNumber;
    USHORT MinimumPeriodicTickCount;
    UCHAR PageProtection;
} ET_ACPI_HPET, *PET_ACPI_HPET;

typedef struct _ET_ACPI_MCFG_ENTRY
{
    ULONG64 BaseAddress;
    USHORT SegmentNumber;
    UCHAR StartBusNumber;
    UCHAR EndBusNumber;
    ULONG Reserved;
} ET_ACPI_MCFG_ENTRY, * PET_ACPI_MCFG_ENTRY;

typedef struct _ET_ACPI_MCFG
{
    ET_ACPI_HEADER Header;
    ULONG Reserved[2];
    // Followed by a variable list of ET_ACPI_MCFG_ENTRY.
} ET_ACPI_MCFG, * PET_ACPI_MCFG;

typedef struct _ET_ACPI_SRAT
{
    ET_ACPI_HEADER Header;
    ULONG TableRevision;
    ULONG Reserved[2];
    // Followed by a variable list of resource-affinity structures.
} ET_ACPI_SRAT, * PET_ACPI_SRAT;

typedef struct _ET_ACPI_SLIT
{
    ET_ACPI_HEADER Header;
    ULONG64 LocalityCount;
} ET_ACPI_SLIT, * PET_ACPI_SLIT;

typedef struct _ET_ACPI_BGRT
{
    ET_ACPI_HEADER Header;
    USHORT Version;
    UCHAR Status;
    UCHAR ImageType;
    ULONG64 LogoAddress;
    ULONG OffsetX;
    ULONG OffsetY;
} ET_ACPI_BGRT, * PET_ACPI_BGRT;

#include <poppack.h>

#define ET_ACPI_SIGNATURE(s)    ((ULONG)(s))
#define ET_FADT_SIGNATURE       0x50434146 //  "FACP "
#define ET_FACS_SIGNATURE       0x53434146 //  "FACS "
#define ET_MADT_SIGNATURE       0x43495041 //  "APIC "
#define ET_HPET_SIGNATURE       0x54455048 //  "HPET "
#define ET_MCFG_SIGNATURE       0x4746434d //  "MCFG "
#define ET_SRAT_SIGNATURE       0x54415253 //  "SRAT "
#define ET_SLIT_SIGNATURE       0x54494c53 //  "SLIT "
#define ET_BGRT_SIGNATURE       0x54524742 //  "BGRT "

// MADT interrupt-controller structure types
#define ET_MADT_PROCESSOR_LOCAL_APIC   0
#define ET_MADT_IO_APIC                1
#define ET_MADT_ISA_VECTOR_OVERRIDE    2
#define ET_MADT_IO_NMI_SOURCE          3
#define ET_MADT_LOCAL_NMI_SOURCE       4
#define ET_MADT_PROCESSOR_LOCAL_X2APIC 9

#define ET_ACPI_FIELD_END(Type, Field) (FIELD_OFFSET(Type, Field) + RTL_FIELD_SIZE(Type, Field))
#define ET_ACPI_HAS(Length, Type, Field) ((ULONG)(Length) >= ET_ACPI_FIELD_END(Type, Field))

typedef struct _ACPI_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    ULONG GroupCounter;
    ULONG IndexCounter;
} ACPI_WINDOW_CONTEXT, * PACPI_WINDOW_CONTEXT;

ULONG EtAddAcpiGroup(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PCWSTR Name
    )
{
    ULONG group = Context->GroupCounter++;
    PhAddListViewGroup(Context->ListViewHandle, group, Name);
    return group;
}

VOID EtAddAcpiItem(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    )
{
    ULONG index = Context->IndexCounter++;
    PhAddListViewGroupItem(Context->ListViewHandle, Group, index, Name, NULL);
    PhSetListViewSubItem(Context->ListViewHandle, index, 1, Value);
}

VOID EtAddAcpiUInt32(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG Value
    )
{
    WCHAR buffer[PH_INT32_STR_LEN_1];
    PhPrintUInt32(buffer, Value);
    EtAddAcpiItem(Context, Group, Name, buffer);
}

VOID EtAddAcpiUInt32IX(
    _In_ PACPI_WINDOW_CONTEXT Context,
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
    EtAddAcpiItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAddAcpiUInt64IX(
    _In_ PACPI_WINDOW_CONTEXT Context,
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
    EtAddAcpiItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAddAcpiBoolean(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ BOOLEAN Value
    )
{
    EtAddAcpiItem(Context, Group, Name, Value ? L"true" : L"false");
}

// Convert a 4-character ACPI signature (stored as a ULONG) to a string.
PPH_STRING EtAcpiSignatureToString(
    _In_ ULONG Signature
    )
{
    CHAR ansi[5];
    ansi[0] = (CHAR)((Signature >> 0) & 0xff);
    ansi[1] = (CHAR)((Signature >> 8) & 0xff);
    ansi[2] = (CHAR)((Signature >> 16) & 0xff);
    ansi[3] = (CHAR)((Signature >> 24) & 0xff);
    ansi[4] = ANSI_NULL;

    for (ULONG i = 0; i < 4; i++)
    {
        if (ansi[i] < ' ' || ansi[i] > '~')
            ansi[i] = ' ';
    }

    return PhConvertUtf8ToUtf16(ansi);
}

VOID EtAddAcpiSignature(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ ULONG Signature
    )
{
    PPH_STRING string;
    string = EtAcpiSignatureToString(Signature);
    EtAddAcpiItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAddAcpiAnsiString(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_reads_(Length) PCSTR Buffer,
    _In_ ULONG Length
    )
{
    PPH_STRING string;
    string = PhConvertUtf8ToUtf16Ex(Buffer, Length);
    PhTrimToNullTerminatorString(string);
    EtAddAcpiItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

// Maximum number of raw bytes to render in the "Raw data" value.
#define ET_ACPI_RAW_DUMP_MAX 1024

VOID EtAddAcpiRawData(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ ULONG Length
    )
{
    ULONG dumpLength;
    PPH_STRING string;
    dumpLength = min(Length, ET_ACPI_RAW_DUMP_MAX);
    string = PhBufferToHexString(Buffer, dumpLength);

    if (dumpLength < Length)
        PhMoveReference(&string, PhConcatStrings2(PhGetString(string), L"..."));

    EtAddAcpiItem(Context, Group, L"Raw data", PhGetString(string));
    PhDereferenceObject(string);
}

#define ET_ACPI_GROUP(n)        ULONG group = EtAddAcpiGroup(Context, n)
#define ET_ACPI_ITEM(n, v)      EtAddAcpiItem(Context, group, n, v)
#define ET_ACPI_UINT32(n, v)    EtAddAcpiUInt32(Context, group, n, v)
#define ET_ACPI_UINT32IX(n, v)  EtAddAcpiUInt32IX(Context, group, n, v)
#define ET_ACPI_UINT64IX(n, v)  EtAddAcpiUInt64IX(Context, group, n, v)
#define ET_ACPI_BOOLEAN(n, v)   EtAddAcpiBoolean(Context, group, n, v)
#define ET_ACPI_SIG(n, v)       EtAddAcpiSignature(Context, group, n, v)

// Emit the common DESCRIPTION_HEADER fields shared by all tables (except FACS
// and RSDP, which do not carry a standard header).
VOID EtAcpiDescriptionHeader(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PET_ACPI_HEADER Header
    )
{
    ULONG group = Group;
    ET_ACPI_SIG(L"Signature", Header->Signature);
    ET_ACPI_UINT32(L"Length", Header->Length);
    ET_ACPI_UINT32(L"Revision", Header->Revision);
    ET_ACPI_UINT32IX(L"Checksum", Header->Checksum);
    EtAddAcpiAnsiString(Context, group, L"OEM ID", Header->OEMID, sizeof(Header->OEMID));
    EtAddAcpiAnsiString(Context, group, L"OEM table ID", Header->OEMTableID, sizeof(Header->OEMTableID));
    ET_ACPI_UINT32(L"OEM revision", Header->OEMRevision);
    EtAddAcpiAnsiString(Context, group, L"Creator ID", Header->CreatorID, sizeof(Header->CreatorID));
    ET_ACPI_UINT32(L"Creator revision", Header->CreatorRev);
}

PCWSTR EtAcpiAddressSpaceString(
    _In_ UCHAR AddressSpaceId
    )
{
    switch (AddressSpaceId)
    {
    case 0:
        return L"System memory";
    case 1:
        return L"System I/O";
    case 2:
        return L"PCI configuration";
    case 3:
        return L"Embedded controller";
    case 4:
        return L"SMBus";
    case 0x0a:
        return L"PCC";
    case 0x7f:
        return L"Functional fixed hardware";
    default:
        return L"Unknown";
    }
}

VOID EtAcpiGenericAddress(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ ULONG Group,
    _In_ PCWSTR Name,
    _In_ PET_ACPI_GEN_ADDR Address
    )
{
    PH_FORMAT format[6];
    PPH_STRING string;
    PhInitFormatS(&format[0], (PWSTR)EtAcpiAddressSpaceString(Address->AddressSpaceID));
    PhInitFormatS(&format[1], L", 0x");
    PhInitFormatI64X(&format[2], Address->Address);
    PhInitFormatS(&format[3], L" (");
    PhInitFormatU(&format[4], Address->BitWidth);
    PhInitFormatS(&format[5], L" bits)");
    string = PhFormat(format, 6, 40);
    EtAddAcpiItem(Context, Group, Name, PhGetString(string));
    PhDereferenceObject(string);
}

VOID EtAcpiFadt(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_FADT Table,
    _In_ ULONG Length
    )
{
    ET_ACPI_GROUP(L"Fixed ACPI Description Table (FADT)");
    EtAcpiDescriptionHeader(Context, group, &Table->Header);

    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Facs))
        ET_ACPI_UINT32IX(L"FACS address", Table->Facs);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Dsdt))
        ET_ACPI_UINT32IX(L"DSDT address", Table->Dsdt);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, IntModel))
        ET_ACPI_UINT32(L"Interrupt model", Table->IntModel);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, PmProfile))
        ET_ACPI_UINT32(L"Power profile", Table->PmProfile);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, SciIntVector))
        ET_ACPI_UINT32(L"SCI interrupt vector", Table->SciIntVector);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, SmiCmdIoPort))
        ET_ACPI_UINT32IX(L"SMI command port", Table->SmiCmdIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, AcpiOnValue))
        ET_ACPI_UINT32IX(L"ACPI enable value", Table->AcpiOnValue);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, AcpiOffValue))
        ET_ACPI_UINT32IX(L"ACPI disable value", Table->AcpiOffValue);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, S4BiosReq))
        ET_ACPI_UINT32IX(L"S4BIOS request value", Table->S4BiosReq);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, PstateControl))
        ET_ACPI_UINT32IX(L"P-state control", Table->PstateControl);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Pm1aEvtBlkIoPort))
        ET_ACPI_UINT32IX(L"PM1a event block", Table->Pm1aEvtBlkIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Pm1bEvtBlkIoPort))
        ET_ACPI_UINT32IX(L"PM1b event block", Table->Pm1bEvtBlkIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Pm1aCtrlBlkIoPort))
        ET_ACPI_UINT32IX(L"PM1a control block", Table->Pm1aCtrlBlkIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Pm1bCtrlBlkIoPort))
        ET_ACPI_UINT32IX(L"PM1b control block", Table->Pm1bCtrlBlkIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Pm2CtrlBlkIoPort))
        ET_ACPI_UINT32IX(L"PM2 control block", Table->Pm2CtrlBlkIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, PmTmrBlkIoPort))
        ET_ACPI_UINT32IX(L"PM timer block", Table->PmTmrBlkIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Gp0BlkIoPort))
        ET_ACPI_UINT32IX(L"GP0 block", Table->Gp0BlkIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Gp1BlkIoPort))
        ET_ACPI_UINT32IX(L"GP1 block", Table->Gp1BlkIoPort);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Lvl2Latency))
        ET_ACPI_UINT32(L"C2 latency (microseconds)", Table->Lvl2Latency);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Lvl3Latency))
        ET_ACPI_UINT32(L"C3 latency (microseconds)", Table->Lvl3Latency);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, BootArch))
        ET_ACPI_UINT32IX(L"Boot architecture flags", Table->BootArch);
    if (ET_ACPI_HAS(Length, ET_ACPI_FADT, Flags))
        ET_ACPI_UINT32IX(L"Flags", Table->Flags);
}

VOID EtAcpiFacs(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_FACS Table,
    _In_ ULONG Length
    )
{
    ET_ACPI_GROUP(L"Firmware ACPI Control Structure (FACS)");
    ET_ACPI_SIG(L"Signature", Table->Signature);
    if (ET_ACPI_HAS(Length, ET_ACPI_FACS, Length))
        ET_ACPI_UINT32(L"Length", Table->Length);
    if (ET_ACPI_HAS(Length, ET_ACPI_FACS, HardwareSignature))
        ET_ACPI_UINT32IX(L"Hardware signature", Table->HardwareSignature);
    if (ET_ACPI_HAS(Length, ET_ACPI_FACS, FirmwareWakingVector))
        ET_ACPI_UINT32IX(L"Firmware waking vector", Table->FirmwareWakingVector);
    if (ET_ACPI_HAS(Length, ET_ACPI_FACS, GlobalLock))
        ET_ACPI_UINT32IX(L"Global lock", Table->GlobalLock);
    if (ET_ACPI_HAS(Length, ET_ACPI_FACS, Flags))
        ET_ACPI_UINT32IX(L"Flags", Table->Flags);
    if (ET_ACPI_HAS(Length, ET_ACPI_FACS, XFirmwareWakingVector))
        ET_ACPI_UINT64IX(L"Firmware waking vector (64-bit)", Table->XFirmwareWakingVector);
    if (ET_ACPI_HAS(Length, ET_ACPI_FACS, Version))
        ET_ACPI_UINT32(L"Version", Table->Version);
}

PCWSTR EtAcpiMadtEntryTypeString(
    _In_ UCHAR Type
    )
{
    switch (Type)
    {
    case ET_MADT_PROCESSOR_LOCAL_APIC:
        return L"Processor local APIC";
    case ET_MADT_IO_APIC:
        return L"I/O APIC";
    case ET_MADT_ISA_VECTOR_OVERRIDE:
        return L"Interrupt source override";
    case ET_MADT_IO_NMI_SOURCE:
        return L"I/O NMI source";
    case ET_MADT_LOCAL_NMI_SOURCE:
        return L"Local NMI source";
    case ET_MADT_PROCESSOR_LOCAL_X2APIC:
        return L"Processor local x2APIC";
    default:
        return L"Other";
    }
}

VOID EtAcpiMadt(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_MADT Table,
    _In_ ULONG Length
    )
{
    PUCHAR entry;
    PUCHAR end;
    ULONG counts[16] = { 0 };

    {
        ET_ACPI_GROUP(L"Multiple APIC Description Table (APIC)");
        EtAcpiDescriptionHeader(Context, group, &Table->Header);

        if (ET_ACPI_HAS(Length, ET_ACPI_MADT, LocalApicAddress))
            ET_ACPI_UINT32IX(L"Local APIC address", Table->LocalApicAddress);
        if (ET_ACPI_HAS(Length, ET_ACPI_MADT, Flags))
            ET_ACPI_BOOLEAN(L"PC-AT Compatible", (Table->Flags & 1) != 0);

        if (Length < sizeof(ET_ACPI_MADT))
            return;
    }
    {
        ET_ACPI_GROUP(L"APIC Interrupt Controllers");
        entry = (PUCHAR)Table + sizeof(ET_ACPI_MADT);
        end = (PUCHAR)Table + Length;

        while (entry + sizeof(ET_ACPI_MADT_ENTRY) <= end)
        {
            PET_ACPI_MADT_ENTRY hdr = (PET_ACPI_MADT_ENTRY)entry;

            if (hdr->Length < sizeof(ET_ACPI_MADT_ENTRY) || entry + hdr->Length > end)
                break;

            ULONG idx = (hdr->Type < RTL_NUMBER_OF(counts)) ? counts[hdr->Type]++ : 0;
            PH_FORMAT fmt[3];
            PhInitFormatS(&fmt[0], (PWSTR)EtAcpiMadtEntryTypeString(hdr->Type));
            PhInitFormatS(&fmt[1], L" #");
            PhInitFormatU(&fmt[2], idx);
            PPH_STRING nameStr = PhFormat(fmt, 3, 32);

            // Create a subgroup for this specific entry so details are nested correctly
            ULONG entryGroup = EtAddAcpiGroup(Context, PhGetString(nameStr));
            PhDereferenceObject(nameStr);

            switch (hdr->Type)
            {
            case ET_MADT_PROCESSOR_LOCAL_APIC:
                {
                    PET_ACPI_MADT_LOCAL_APIC lapic = (PET_ACPI_MADT_LOCAL_APIC)entry;
                    if (entry + sizeof(*lapic) <= end) {
                        EtAddAcpiUInt32(Context, entryGroup, L"Processor ID", lapic->ProcessorId);
                        EtAddAcpiUInt32(Context, entryGroup, L"APIC ID", lapic->ApicId);
                        EtAddAcpiUInt32IX(Context, entryGroup, L"Flags", lapic->Flags);
                        EtAddAcpiBoolean(Context, entryGroup, L"Enabled", (lapic->Flags & 1) != 0);
                    }
                }
                break;
            case ET_MADT_IO_APIC:
                {
                    PET_ACPI_MADT_IO_APIC ioapic = (PET_ACPI_MADT_IO_APIC)entry;
                    if (entry + sizeof(*ioapic) <= end) {
                        EtAddAcpiUInt32(Context, entryGroup, L"I/O APIC ID", ioapic->IoApicId);
                        EtAddAcpiUInt32IX(Context, entryGroup, L"Memory Address", ioapic->Address);
                        EtAddAcpiUInt32(Context, entryGroup, L"Global IRQ Base", ioapic->GlobalSystemInterruptBase);
                    }
                }
                break;
            case ET_MADT_ISA_VECTOR_OVERRIDE:
                {
                    PET_ACPI_MADT_INT_SRC_OVR src = (PET_ACPI_MADT_INT_SRC_OVR)entry;
                    if (entry + sizeof(*src) <= end) {
                        EtAddAcpiUInt32(Context, entryGroup, L"Bus", src->Bus);
                        EtAddAcpiUInt32(Context, entryGroup, L"Source IRQ", src->Source);
                        EtAddAcpiUInt32(Context, entryGroup, L"Global IRQ", src->GlobalSystemInterrupt);
                        EtAddAcpiUInt32IX(Context, entryGroup, L"Flags", src->Flags);
                    }
                }
                break;
            case ET_MADT_PROCESSOR_LOCAL_X2APIC:
                {
                    PET_ACPI_MADT_X2APIC x2 = (PET_ACPI_MADT_X2APIC)entry;
                    if (entry + sizeof(*x2) <= end) {
                        EtAddAcpiUInt32IX(Context, entryGroup, L"x2APIC ID", x2->X2ApicId);
                        EtAddAcpiUInt32IX(Context, entryGroup, L"Flags", x2->Flags);
                        EtAddAcpiUInt32(Context, entryGroup, L"UID", x2->Uid);
                        EtAddAcpiBoolean(Context, entryGroup, L"Enabled", (x2->Flags & 1) != 0);
                    }
                }
                break;
            default:
                // Fallback for unknown or unexpanded entry types
                EtAddAcpiRawData(Context, entryGroup, entry, hdr->Length);
                break;
            }
            entry += hdr->Length;
        }
    }
}

VOID EtAcpiHpet(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_HPET Table,
    _In_ ULONG Length
    )
{
    ET_ACPI_GROUP(L"High Precision Event Timer (HPET)");
    EtAcpiDescriptionHeader(Context, group, &Table->Header);

    if (ET_ACPI_HAS(Length, ET_ACPI_HPET, EventTimerBlockId))
        ET_ACPI_UINT32IX(L"Event timer block ID", Table->EventTimerBlockId);
    if (ET_ACPI_HAS(Length, ET_ACPI_HPET, Address))
        EtAcpiGenericAddress(Context, group, L"Base address", &Table->Address);
    if (ET_ACPI_HAS(Length, ET_ACPI_HPET, HpetNumber))
        ET_ACPI_UINT32(L"HPET number", Table->HpetNumber);
    if (ET_ACPI_HAS(Length, ET_ACPI_HPET, MinimumPeriodicTickCount))
        ET_ACPI_UINT32(L"Minimum tick count", Table->MinimumPeriodicTickCount);
    if (ET_ACPI_HAS(Length, ET_ACPI_HPET, PageProtection))
        ET_ACPI_UINT32IX(L"Page protection", Table->PageProtection);
}

VOID EtAcpiMcfg(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_MCFG Table,
    _In_ ULONG Length
    )
{
    PUCHAR entry;
    PUCHAR end;
    ULONG number = 0;

    ET_ACPI_GROUP(L"PCI Memory-Mapped Configuration (MCFG)");
    EtAcpiDescriptionHeader(Context, group, &Table->Header);

    if (Length < sizeof(ET_ACPI_MCFG))
        return;

    entry = (PUCHAR)Table + sizeof(ET_ACPI_MCFG);
    end = (PUCHAR)Table + Length;

    while (entry + sizeof(ET_ACPI_MCFG_ENTRY) <= end)
    {
        PET_ACPI_MCFG_ENTRY item = (PET_ACPI_MCFG_ENTRY)entry;
        PH_FORMAT format[2];
        PPH_STRING name;

        PhInitFormatS(&format[0], L"Configuration space #");
        PhInitFormatU(&format[1], number++);
        name = PhFormat(format, 2, 30);

        ULONG group = EtAddAcpiGroup(Context, PhGetString(name));

        ET_ACPI_UINT64IX(L"Base address", item->BaseAddress);
        ET_ACPI_UINT32(L"Segment", item->SegmentNumber);
        ET_ACPI_UINT32(L"Start bus", item->StartBusNumber);
        ET_ACPI_UINT32(L"End bus", item->EndBusNumber);

        PhDereferenceObject(name);
        entry += sizeof(ET_ACPI_MCFG_ENTRY);
    }
}

VOID EtAcpiSrat(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_SRAT Table,
    _In_ ULONG Length
    )
{
    PUCHAR entry;
    PUCHAR end;

    ET_ACPI_GROUP(L"System Resource Affinity Table (SRAT)");
    EtAcpiDescriptionHeader(Context, group, &Table->Header);

    if (ET_ACPI_HAS(Length, ET_ACPI_SRAT, TableRevision))
        ET_ACPI_UINT32(L"Table revision", Table->TableRevision);

    if (Length < sizeof(ET_ACPI_SRAT))
        return;

    //ET_ACPI_GROUP(L"Resource Affinity Entries");
    entry = (PUCHAR)Table + sizeof(ET_ACPI_SRAT);
    end = (PUCHAR)Table + Length;

    while (entry + 2 <= end)
    {
        UCHAR type = entry[0];
        UCHAR length = entry[1];

        if (length < 2 || entry + length > end)
            break;

        switch (type)
        {
        case 0: { // Processor Local APIC Affinity
            PET_ACPI_SRAT_PROCESSOR_APIC proc = (PET_ACPI_SRAT_PROCESSOR_APIC)entry;
            if (entry + sizeof(*proc) <= end) {
                ULONG proxDomain = proc->ProximityDomainLo | (proc->ProximityDomainHi[0] << 8) | (proc->ProximityDomainHi[1] << 16) | (proc->ProximityDomainHi[2] << 24);
                PH_FORMAT fmt[4];
                PhInitFormatS(&fmt[0], L"Processor APIC (Node: ");
                PhInitFormatU(&fmt[1], proxDomain);
                PhInitFormatS(&fmt[2], L", APIC ID: ");
                PhInitFormatU(&fmt[3], proc->ApicId);
                PPH_STRING nameStr = PhFormat(fmt, 4, 40);
                ULONG entryGroup = EtAddAcpiGroup(Context, PhGetString(nameStr));
                PhDereferenceObject(nameStr);

                EtAddAcpiUInt32(Context, entryGroup, L"Proximity Domain", proxDomain);
                EtAddAcpiUInt32(Context, entryGroup, L"APIC ID", proc->ApicId);
                EtAddAcpiUInt32IX(Context, entryGroup, L"Flags", proc->Flags);
                EtAddAcpiBoolean(Context, entryGroup, L"Enabled", (proc->Flags & 1) != 0);
            }
            break;
        }
        case 1:
            { // Memory Affinity
                PET_ACPI_SRAT_MEMORY mem = (PET_ACPI_SRAT_MEMORY)entry;
                if (entry + sizeof(*mem) <= end) {
                    PH_FORMAT fmt[4];
                    PhInitFormatS(&fmt[0], L"Memory (Node: ");
                    PhInitFormatU(&fmt[1], mem->ProximityDomain);
                    PhInitFormatS(&fmt[2], L", Base: 0x");
                    PhInitFormatI64X(&fmt[3], mem->BaseAddress);
                    PPH_STRING nameStr = PhFormat(fmt, 4, 50);
                    ULONG entryGroup = EtAddAcpiGroup(Context, PhGetString(nameStr));
                    PhDereferenceObject(nameStr);

                    EtAddAcpiUInt32(Context, entryGroup, L"Proximity Domain", mem->ProximityDomain);
                    EtAddAcpiUInt64IX(Context, entryGroup, L"Base Address", mem->BaseAddress);
                    EtAddAcpiUInt64IX(Context, entryGroup, L"Length", mem->Length);
                    EtAddAcpiUInt32IX(Context, entryGroup, L"Flags", mem->Flags);
                }
                break;
            }
        default:
            {
                PH_FORMAT fmt[2];
                PhInitFormatS(&fmt[0], L"Unknown SRAT Entry Type ");
                PhInitFormatU(&fmt[1], type);
                PPH_STRING nameStr = PhFormat(fmt, 2, 40);
                ULONG entryGroup = EtAddAcpiGroup(Context, PhGetString(nameStr));
                PhDereferenceObject(nameStr);
                EtAddAcpiRawData(Context, entryGroup, entry, length);
            }
            break;
        }
        entry += length;
    }
}

VOID EtAcpiSlit(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_SLIT Table,
    _In_ ULONG Length
    )
{
    ET_ACPI_GROUP(L"System Locality Information Table (SLIT)");
    EtAcpiDescriptionHeader(Context, group, &Table->Header);

    if (ET_ACPI_HAS(Length, ET_ACPI_SLIT, LocalityCount))
        ET_ACPI_UINT64IX(L"Locality count", Table->LocalityCount);
}

VOID EtAcpiBgrt(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_BGRT Table,
    _In_ ULONG Length
    )
{
    ET_ACPI_GROUP(L"Boot Graphics Resource Table (BGRT)");
    EtAcpiDescriptionHeader(Context, group, &Table->Header);

    if (ET_ACPI_HAS(Length, ET_ACPI_BGRT, Version))
        ET_ACPI_UINT32(L"Version", Table->Version);
    if (ET_ACPI_HAS(Length, ET_ACPI_BGRT, Status))
        ET_ACPI_UINT32IX(L"Status", Table->Status);
    if (ET_ACPI_HAS(Length, ET_ACPI_BGRT, ImageType))
        ET_ACPI_UINT32(L"Image type", Table->ImageType);
    if (ET_ACPI_HAS(Length, ET_ACPI_BGRT, LogoAddress))
        ET_ACPI_UINT64IX(L"Logo address", Table->LogoAddress);
    if (ET_ACPI_HAS(Length, ET_ACPI_BGRT, OffsetX))
        ET_ACPI_UINT32(L"Offset X", Table->OffsetX);
    if (ET_ACPI_HAS(Length, ET_ACPI_BGRT, OffsetY))
        ET_ACPI_UINT32(L"Offset Y", Table->OffsetY);
}

// Fallback for tables without a dedicated decoder: emit the common header
// fields plus a raw hex dump of the entire table.
VOID EtAcpiGenericTable(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_ PET_ACPI_HEADER Header,
    _In_ ULONG Length
    )
{
    PPH_STRING name;
    PPH_STRING signature;
    signature = EtAcpiSignatureToString(Header->Signature);
    name = PhConcatStrings2(L"Table ", PhGetString(signature));

    ULONG group = EtAddAcpiGroup(Context, PhGetString(name));

    EtAcpiDescriptionHeader(Context, group, Header);
    EtAddAcpiRawData(Context, group, (PUCHAR)Header, Length);

    PhDereferenceObject(name);
    PhDereferenceObject(signature);
}

VOID EtAcpiDispatchTable(
    _In_ PACPI_WINDOW_CONTEXT Context,
    _In_reads_bytes_(BufferLength) PUCHAR Buffer,
    _In_ ULONG BufferLength
    )
{
    PET_ACPI_HEADER header;
    ULONG length;

    if (BufferLength < RTL_SIZEOF_THROUGH_FIELD(ET_ACPI_HEADER, Length))
        return;

    header = (PET_ACPI_HEADER)Buffer;

    // The header Length is authoritative, but firmware can lie. Clamp it to the
    // buffer the kernel actually returned before reading any fields.
    length = header->Length;
    if (length > BufferLength || length == 0)
        length = BufferLength;

    switch (header->Signature)
    {
    case ET_FADT_SIGNATURE:
        EtAcpiFadt(Context, (PET_ACPI_FADT)header, length);
        break;
    case ET_FACS_SIGNATURE:
        EtAcpiFacs(Context, (PET_ACPI_FACS)header, length);
        break;
    case ET_MADT_SIGNATURE:
        EtAcpiMadt(Context, (PET_ACPI_MADT)header, length);
        break;
    case ET_HPET_SIGNATURE:
        EtAcpiHpet(Context, (PET_ACPI_HPET)header, length);
        break;
    case ET_MCFG_SIGNATURE:
        EtAcpiMcfg(Context, (PET_ACPI_MCFG)header, length);
        break;
    case ET_SRAT_SIGNATURE:
        EtAcpiSrat(Context, (PET_ACPI_SRAT)header, length);
        break;
    case ET_SLIT_SIGNATURE:
        EtAcpiSlit(Context, (PET_ACPI_SLIT)header, length);
        break;
    case ET_BGRT_SIGNATURE:
        EtAcpiBgrt(Context, (PET_ACPI_BGRT)header, length);
        break;
    default:
        EtAcpiGenericTable(Context, header, length);
        break;
    }
}

// Issue a 'Get' query for a single ACPI table identified by TableId, growing
// the buffer as needed. Mirrors the buffer-growth pattern of PhEnumSMBIOS.
NTSTATUS EtGetAcpiTable(
    _In_ ULONG TableId,
    _Out_ PVOID* Buffer,
    _Out_ PULONG BufferLength
    )
{
    NTSTATUS status;
    ULONG length;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;

    *Buffer = NULL;
    *BufferLength = 0;

    length = sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION) + 0x1000;

    if (!(info = PhAllocate(length)))
        return STATUS_NO_MEMORY;

    RtlZeroMemory(info, length);
    info->ProviderSignature = 'ACPI';
    info->TableID = TableId;
    info->Action = SystemFirmwareTableGet;
    info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

    status = NtQuerySystemInformation(
        SystemFirmwareTableInformation,
        info,
        length,
        &length
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(info);

        if (!(info = PhAllocate(length)))
            return STATUS_NO_MEMORY;

        RtlZeroMemory(info, length);
        info->ProviderSignature = 'ACPI';
        info->TableID = TableId;
        info->Action = SystemFirmwareTableGet;
        info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

        status = NtQuerySystemInformation(
            SystemFirmwareTableInformation,
            info,
            length,
            &length
            );
    }

    if (NT_SUCCESS(status))
    {
        PVOID buffer;

        buffer = PhAllocateCopy(info->TableBuffer, info->TableBufferLength);
        *Buffer = buffer;
        *BufferLength = info->TableBufferLength;
    }

    PhFree(info);

    return status;
}

// Enumerate the list of ACPI table IDs and decode each one.
NTSTATUS EtEnumerateAcpiTables(
    _In_ PACPI_WINDOW_CONTEXT Context
    )
{
    NTSTATUS status;
    ULONG length;
    PSYSTEM_FIRMWARE_TABLE_INFORMATION info;

    length = sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION) + 0x400;

    if (!(info = PhAllocate(length)))
        return STATUS_NO_MEMORY;

    RtlZeroMemory(info, length);
    info->ProviderSignature = 'ACPI';
    info->TableID = 0;
    info->Action = SystemFirmwareTableEnumerate;
    info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

    status = NtQuerySystemInformation(
        SystemFirmwareTableInformation,
        info,
        length,
        &length
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(info);

        if (!(info = PhAllocate(length)))
            return STATUS_NO_MEMORY;

        RtlZeroMemory(info, length);
        info->ProviderSignature = 'ACPI';
        info->TableID = 0;
        info->Action = SystemFirmwareTableEnumerate;
        info->TableBufferLength = length - sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);

        status = NtQuerySystemInformation(
            SystemFirmwareTableInformation,
            info,
            length,
            &length
            );
    }

    if (NT_SUCCESS(status))
    {
        PULONG tableIds = (PULONG)info->TableBuffer;
        ULONG count = info->TableBufferLength / sizeof(ULONG);

        for (ULONG i = 0; i < count; i++)
        {
            PVOID buffer;
            ULONG bufferLength;

            if (NT_SUCCESS(EtGetAcpiTable(tableIds[i], &buffer, &bufferLength)))
            {
                EtAcpiDispatchTable(Context, (PUCHAR)buffer, bufferLength);
                PhFree(buffer);
            }
        }
    }

    PhFree(info);

    return status;
}

VOID EtEnumerateAcpiTablesList(
    _In_ PACPI_WINDOW_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);
    ListView_EnableGroupView(Context->ListViewHandle, TRUE);
    Context->GroupCounter = 0;
    Context->IndexCounter = 0;

    EtEnumerateAcpiTables(Context);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK EtAcpiTableDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PACPI_WINDOW_CONTEXT context = NULL;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(ACPI_WINDOW_CONTEXT));
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ParentWindowHandle = (HWND)lParam;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_ACPI_INFO);

            PhSetApplicationWindowIcon(WindowHandle);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 220, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PhLoadListViewColumnsFromSetting(SETTING_NAME_ACPI_INFO_COLUMNS, context->ListViewHandle);
            if (PhValidWindowPlacementFromSetting(SETTING_NAME_ACPI_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_ACPI_WINDOW_POSITION, SETTING_NAME_ACPI_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            EtEnumerateAcpiTablesList(context);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT));
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_ACPI_INFO_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_ACPI_WINDOW_POSITION, SETTING_NAME_ACPI_WINDOW_SIZE, WindowHandle);
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDOK);
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
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        WindowHandle,
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
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID EtShowAcpiTableDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PhDialogBox(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDD_ACPI),
        NULL,
        EtAcpiTableDlgProc,
        ParentWindowHandle
        );
}
