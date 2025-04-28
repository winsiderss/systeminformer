/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <sysinfo.h>
#include <sysinfop.h>

#include <kphuser.h>
#include <symprv.h>
#include <workqueue.h>
#include <settings.h>
#include <phfirmware.h>

#include <procprv.h>
#include <phsettings.h>

static CONST PH_KEY_VALUE_PAIR MemoryFormFactors[] =
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
    SIP(L"TOSP", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_TSOP),
    SIP(L"Row of chips", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_ROW_OF_CHIPS),
    SIP(L"RIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_RIMM),
    SIP(L"SODIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_SODIMM),
    SIP(L"SRIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_SRIMM),
    SIP(L"FB-DIMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_FB_DIMM),
    SIP(L"Die", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_DIE),
    SIP(L"CAMM", SMBIOS_MEMORY_DEVICE_FORM_FACTOR_CAMM),
};

static CONST PH_KEY_VALUE_PAIR MemoryTypes[] =
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

static CONST PH_KEY_VALUE_PAIR MemoryTechnologies[] =
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

static BOOLEAN ShowCommitInSummary;
static PPH_SYSINFO_SECTION MemorySection;
static HWND MemoryDialog;
static PH_LAYOUT_MANAGER MemoryLayoutManager;
static RECT MemoryGraphMargin;
static HWND CommitGraphHandle;
static PH_GRAPH_STATE CommitGraphState;
static HWND PhysicalGraphHandle;
static PH_GRAPH_STATE PhysicalGraphState;
static HWND MemoryPanel;
static ULONG MemoryTicked;
static PH_UINT32_DELTA PagedAllocsDelta;
static PH_UINT32_DELTA PagedFreesDelta;
static PH_UINT32_DELTA NonPagedAllocsDelta;
static PH_UINT32_DELTA NonPagedFreesDelta;
static PH_UINT32_DELTA PageFaultsDelta;
static PH_UINT32_DELTA PageReadsDelta;
static PH_UINT32_DELTA PagefileWritesDelta;
static PH_UINT32_DELTA MappedWritesDelta;
static PH_UINT64_DELTA MappedIoReadDelta;
static PH_UINT64_DELTA MappedIoWritesDelta;
static BOOLEAN MmAddressesInitialized;
static PSIZE_T MmSizeOfPagedPoolInBytes;
static PSIZE_T MmMaximumNonPagedPoolInBytes;
static ULONGLONG InstalledMemory;
static ULONGLONG ReservedMemory;
static ULONG MemorySlotsTotal;
static ULONG MemorySlotsUsed;
static UCHAR MemoryFormFactor;
static UCHAR MemoryTechnology;
static UCHAR MemoryType;
static ULONG MemorySpeed;

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
BOOLEAN NTAPI PhpSipMemorySMBIOSCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    UCHAR formFactor = 0;
    UCHAR memoryType = 0;
    UCHAR technology = 0;
    ULONG speed = 0;

    if (Entry->Header.Type != SMBIOS_MEMORY_DEVICE_INFORMATION_TYPE)
        return FALSE;

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, Technology))
        technology = Entry->MemoryDevice.Technology;
    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, MemoryType))
        memoryType = Entry->MemoryDevice.MemoryType;
    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, FormFactor))
        formFactor = Entry->MemoryDevice.FormFactor;

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ConfiguredSpeed))
        speed = Entry->MemoryDevice.ConfiguredSpeed;
    if (speed == MAXUSHORT && PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ExtendedConfiguredSpeed))
        speed = Entry->MemoryDevice.ExtendedConfiguredSpeed;

    if (!speed)
    {
        if (PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, Speed))
            speed = Entry->MemoryDevice.Speed;
        if (speed == MAXUSHORT && PH_SMBIOS_CONTAINS_FIELD(Entry, MemoryDevice, ExtendedSpeed))
            speed = Entry->MemoryDevice.ExtendedSpeed;
    }

    MemorySlotsTotal++;
    if (Entry->MemoryDevice.Size.Value)
        MemorySlotsUsed++;

    MemoryFormFactor = max(MemoryFormFactor, formFactor);
    MemoryType = max(MemoryType, memoryType);
    MemoryTechnology = max(MemoryTechnology, technology);
    MemorySpeed = max(MemorySpeed, speed);

    return FALSE;
}

BOOLEAN PhSipMemorySectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoCreate:
        {
            ShowCommitInSummary = !!PhGetIntegerSetting(L"ShowCommitInSummary");
            MemorySection = Section;
            MemorySlotsTotal = 0;
            MemorySlotsUsed = 0;
            MemoryFormFactor = 0;
            MemoryType = 0;
            MemorySpeed = 0;
            PhEnumSMBIOS(PhpSipMemorySMBIOSCallback, NULL);
        }
        return TRUE;
    case SysInfoDestroy:
        {
            if (MemoryDialog)
            {
                PhSipUninitializeMemoryDialog();
                MemoryDialog = NULL;
            }
        }
        break;
    case SysInfoTick:
        {
            if (MemoryDialog)
            {
                PhSipTickMemoryDialog();
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)PtrToUlong(Parameter1);
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (CommitGraphHandle)
            {
                CommitGraphState.Valid = FALSE;
                CommitGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(CommitGraphHandle);
            }

            if (PhysicalGraphHandle)
            {
                PhysicalGraphState.Valid = FALSE;
                PhysicalGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(PhysicalGraphHandle);
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_MEM);
            createDialog->DialogProc = PhSipMemoryDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            ULONG i;

            if (ShowCommitInSummary)
            {
                drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
                Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorPrivate, 0, Section->Parameters->WindowDpi);
                PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhCommitHistory.Count);

                if (!Section->GraphState.Valid)
                {
                    if (PhCsEnableAvxSupport)
                    {
                        PhCopyConvertCircularBufferULONG(&PhCommitHistory, Section->GraphState.Data1, drawInfo->LineDataCount);
#ifdef DEBUG
                        for (i = 0; i < drawInfo->LineDataCount; i++)
                        {
                            assert(Section->GraphState.Data1[i] == (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i));
                        }
#endif
                    }
                    else
                    {
                        for (i = 0; i < drawInfo->LineDataCount; i++)
                        {
                            Section->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i);
                        }
                    }

                    if (PhPerfInformation.CommitLimit != 0)
                    {
                        // Scale the data.
                        PhDivideSinglesBySingle(
                            Section->GraphState.Data1,
                            (FLOAT)PhPerfInformation.CommitLimit,
                            drawInfo->LineDataCount
                            );
                    }

                    if (PhCsEnableGraphMaxText)
                    {
                        drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                        drawInfo->LabelYFunctionParameter = (FLOAT)UInt32x32To64(PhPerfInformation.CommitLimit, PAGE_SIZE);
                    }

                    Section->GraphState.Valid = TRUE;
                }
            }
            else
            {
                drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
                Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorPhysical, 0, Section->Parameters->WindowDpi);
                PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhPhysicalHistory.Count);

                if (!Section->GraphState.Valid)
                {
                    if (PhCsEnableAvxSupport)
                    {
                        PhCopyConvertCircularBufferULONG(&PhPhysicalHistory, Section->GraphState.Data1, drawInfo->LineDataCount);
#ifdef DEBUG
                        for (i = 0; i < drawInfo->LineDataCount; i++)
                        {
                            assert(Section->GraphState.Data1[i] == (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i));
                        }
#endif
                    }
                    else
                    {
                        for (i = 0; i < drawInfo->LineDataCount; i++)
                        {
                            Section->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i);
                        }
                    }

                    if (PhSystemBasicInformation.NumberOfPhysicalPages != 0)
                    {
                        // Scale the data.
                        PhDivideSinglesBySingle(
                            Section->GraphState.Data1,
                            (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages,
                            drawInfo->LineDataCount
                            );
                    }

                    if (PhCsEnableGraphMaxText)
                    {
                        drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                        drawInfo->LabelYFunctionParameter = (FLOAT)UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE);
                    }

                    Section->GraphState.Valid = TRUE;
                }
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG usedPages;
            PH_FORMAT format[3];

            if (ShowCommitInSummary)
            {
                usedPages = PhGetItemCircularBuffer_ULONG(&PhCommitHistory, getTooltipText->Index);

                // Commit charge: %s\n%s
                PhInitFormatSize(&format[0], UInt32x32To64(usedPages, PAGE_SIZE));
                PhInitFormatC(&format[1], L'\n');
                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                getTooltipText->Text = Section->GraphState.TooltipText->sr;
            }
            else
            {
                usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                // Physical memory: %s\n%s
                PhInitFormatSize(&format[0], UInt32x32To64(usedPages, PAGE_SIZE));
                PhInitFormatC(&format[1], L'\n');
                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                getTooltipText->Text = Section->GraphState.TooltipText->sr;
            }
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;
            ULONG totalPages;
            ULONG usedPages;
            PH_FORMAT format[5];

            if (ShowCommitInSummary)
            {
                totalPages = PhPerfInformation.CommitLimit;
                usedPages = PhPerfInformation.CommittedPages;
            }
            else
            {
                totalPages = PhSystemBasicInformation.NumberOfPhysicalPages;
                usedPages = totalPages - PhPerfInformation.AvailablePages;
            }

            drawPanel->Title = PhCreateString(L"Memory");

            // %.0f%%\n%s / %s
            PhInitFormatF(&format[0], ((FLOAT)usedPages / (FLOAT)totalPages) * 100, 0);
            PhInitFormatS(&format[1], L"%\n");
            PhInitFormatSizeWithPrecision(&format[2], UInt32x32To64(usedPages, PAGE_SIZE), 1);
            PhInitFormatS(&format[3], L" / ");
            PhInitFormatSizeWithPrecision(&format[4], UInt32x32To64(totalPages, PAGE_SIZE), 1);

            drawPanel->SubTitle = PhFormat(format, 5, 64);

            // %.0f%%\n%s
            PhInitFormatF(&format[0], ((FLOAT)usedPages / (FLOAT)totalPages) * 100, 0);
            PhInitFormatS(&format[1], L"%\n");
            PhInitFormatSizeWithPrecision(&format[2], UInt32x32To64(usedPages, PAGE_SIZE), 1);

            drawPanel->SubTitleOverflow = PhFormat(format, 3, 0);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhSipInitializeMemoryDialog(
    VOID
    )
{
    PhInitializeDelta(&PagedAllocsDelta);
    PhInitializeDelta(&PagedFreesDelta);
    PhInitializeDelta(&NonPagedAllocsDelta);
    PhInitializeDelta(&NonPagedFreesDelta);
    PhInitializeDelta(&PageFaultsDelta);
    PhInitializeDelta(&PageReadsDelta);
    PhInitializeDelta(&PagefileWritesDelta);
    PhInitializeDelta(&MappedWritesDelta);
    PhInitializeDelta(&MappedIoReadDelta);
    PhInitializeDelta(&MappedIoWritesDelta);

    PhInitializeGraphState(&CommitGraphState);
    PhInitializeGraphState(&PhysicalGraphState);

    MemoryTicked = 0;

    if (!MmAddressesInitialized)
    {
        PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhSipLoadMmAddresses, NULL);
        MmAddressesInitialized = TRUE;
    }
}

VOID PhSipUninitializeMemoryDialog(
    VOID
    )
{
    PhDeleteGraphState(&CommitGraphState);
    PhDeleteGraphState(&PhysicalGraphState);

    // Note: Required for SysInfoViewChanging (dmex)
    CommitGraphHandle = NULL;
    PhysicalGraphHandle = NULL;
}

VOID PhSipTickMemoryDialog(
    VOID
    )
{
    PhUpdateDelta(&PagedAllocsDelta, PhPerfInformation.PagedPoolAllocs);
    PhUpdateDelta(&PagedFreesDelta, PhPerfInformation.PagedPoolFrees);
    PhUpdateDelta(&NonPagedAllocsDelta, PhPerfInformation.NonPagedPoolAllocs);
    PhUpdateDelta(&NonPagedFreesDelta, PhPerfInformation.NonPagedPoolFrees);
    PhUpdateDelta(&PageFaultsDelta, PhPerfInformation.PageFaultCount);
    PhUpdateDelta(&PageReadsDelta, PhPerfInformation.PageReadCount);
    PhUpdateDelta(&PagefileWritesDelta, PhPerfInformation.DirtyPagesWriteCount);
    PhUpdateDelta(&MappedWritesDelta, PhPerfInformation.MappedPagesWriteCount);
    PhUpdateDelta(&MappedIoReadDelta, UInt32x32To64(PhPerfInformation.PageReadCount, PAGE_SIZE));
    PhUpdateDelta(&MappedIoWritesDelta, ((ULONG64)PhPerfInformation.MappedPagesWriteCount + PhPerfInformation.DirtyPagesWriteCount + PhPerfInformation.CcLazyWritePages) * PAGE_SIZE);

    if (MemoryTicked < 2)
        MemoryTicked++;

    PhSipUpdateMemoryGraphs();
    PhSipUpdateMemoryPanel();
}

INT_PTR CALLBACK PhSipMemoryDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;
            RECT margin;
            HWND totalPhysicalLabel;

            PhSipInitializeMemoryDialog();

            MemoryDialog = hwndDlg;
            totalPhysicalLabel = GetDlgItem(hwndDlg, IDC_TOTALPHYSICAL);

            PhInitializeLayoutManager(&MemoryLayoutManager, hwndDlg);
            PhAddLayoutItem(&MemoryLayoutManager, totalPhysicalLabel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            MemoryGraphMargin = graphItem->Margin;

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), MemorySection->Parameters->LargeFont, FALSE);
            SetWindowFont(totalPhysicalLabel, MemorySection->Parameters->MediumFont, FALSE);

            if (NT_SUCCESS(PhGetPhysicallyInstalledSystemMemory(&InstalledMemory, &ReservedMemory)))
            {
                PhSetWindowText(totalPhysicalLabel, PhaConcatStrings2(
                    PhaFormatSize(InstalledMemory, ULONG_MAX)->Buffer, L" installed")->Buffer);
            }
            else
            {
                PhSetWindowText(totalPhysicalLabel, PhaConcatStrings2(
                    PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), ULONG_MAX)->Buffer, L" total")->Buffer);
            }

            MemoryPanel = PhCreateDialog(PhInstanceHandle, MAKEINTRESOURCE(IDD_SYSINFO_MEMPANEL), hwndDlg, PhSipMemoryPanelDialogProc, NULL);
            ShowWindow(MemoryPanel, SW_SHOW);

            margin = panelItem->Margin;
            PhGetSizeDpiValue(&margin, MemorySection->Parameters->WindowDpi, TRUE);
            PhAddLayoutItemEx(&MemoryLayoutManager, MemoryPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, margin);

            PhSipCreateMemoryGraphs();
            PhSipUpdateMemoryGraphs();
            PhSipUpdateMemoryPanel();
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&MemoryLayoutManager);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            if (MemorySection->Parameters->LargeFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), MemorySection->Parameters->LargeFont, FALSE);
            }

            if (MemorySection->Parameters->MediumFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_TOTALPHYSICAL), MemorySection->Parameters->MediumFont, FALSE);
            }

            CommitGraphState.Valid = FALSE;
            CommitGraphState.TooltipIndex = ULONG_MAX;
            PhysicalGraphState.Valid = FALSE;
            PhysicalGraphState.TooltipIndex = ULONG_MAX;
            PhLayoutManagerLayout(&MemoryLayoutManager);
            PhSipLayoutMemoryGraphs(hwndDlg);
        }
        break;
    case WM_SIZE:
        {
            CommitGraphState.Valid = FALSE;
            CommitGraphState.TooltipIndex = ULONG_MAX;
            PhysicalGraphState.Valid = FALSE;
            PhysicalGraphState.TooltipIndex = ULONG_MAX;
            PhLayoutManagerLayout(&MemoryLayoutManager);
            PhSipLayoutMemoryGraphs(hwndDlg);
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

INT_PTR CALLBACK PhSipMemoryPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NOTHING;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_MORE:
                {
                    PhShowMemoryListsDialog(PhSipWindow, PhSipRegisterDialog, PhSipUnregisterDialog);
                }
                break;
            case IDC_EMPTY:
                {
                    extern VOID PhShowMemoryListCommand(_In_ HWND ParentWindow, _In_ HWND ButtonWindow, _In_ BOOLEAN ShowTopAlign); // memlists.c (dmex);

                    PhShowMemoryListCommand(PhSipWindow, GET_WM_COMMAND_HWND(wParam, lParam), TRUE);
                }
                break;
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

VOID PhSipCreateMemoryGraphs(
    VOID
    )
{
    PH_GRAPH_CREATEPARAMS graphCreateParams;

    memset(&graphCreateParams, 0, sizeof(PH_GRAPH_CREATEPARAMS));
    graphCreateParams.Size = sizeof(PH_GRAPH_CREATEPARAMS);
    graphCreateParams.Callback = PhSipNotifyCommitGraph;

    CommitGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        0,
        0,
        MemoryDialog,
        NULL,
        PhInstanceHandle,
        &graphCreateParams
        );
    Graph_SetTooltip(CommitGraphHandle, TRUE);

    memset(&graphCreateParams, 0, sizeof(PH_GRAPH_CREATEPARAMS));
    graphCreateParams.Size = sizeof(PH_GRAPH_CREATEPARAMS);
    graphCreateParams.Callback = PhSipNotifyPhysicalGraph;

    PhysicalGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        0,
        0,
        MemoryDialog,
        NULL,
        PhInstanceHandle,
        &graphCreateParams
        );
    Graph_SetTooltip(PhysicalGraphHandle, TRUE);
}

VOID PhSipLayoutMemoryGraphs(
    _In_ HWND hwnd
    )
{
    RECT clientRect;
    RECT labelRect;
    RECT marginRect;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;

    marginRect = MemoryGraphMargin;
    PhGetSizeDpiValue(&marginRect, MemorySection->Parameters->WindowDpi, TRUE);

    GetClientRect(MemoryDialog, &clientRect);
    GetClientRect(GetDlgItem(MemoryDialog, IDC_COMMIT_L), &labelRect);
    graphWidth = clientRect.right - marginRect.left - marginRect.right;
    graphHeight = (clientRect.bottom - marginRect.top - marginRect.bottom - labelRect.bottom * 2 - MemorySection->Parameters->MemoryPadding * 3) / 2;

    deferHandle = BeginDeferWindowPos(4);
    y = marginRect.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(MemoryDialog, IDC_COMMIT_L),
        NULL,
        marginRect.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + MemorySection->Parameters->MemoryPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        CommitGraphHandle,
        NULL,
        marginRect.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + MemorySection->Parameters->MemoryPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(MemoryDialog, IDC_PHYSICAL_L),
        NULL,
        marginRect.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + MemorySection->Parameters->MemoryPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        PhysicalGraphHandle,
        NULL,
        marginRect.left,
        y,
        graphWidth,
        clientRect.bottom - marginRect.bottom - y,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

BOOLEAN NTAPI PhSipNotifyCommitGraph(
    _In_ HWND GraphHandle,
    _In_ ULONG GraphMessage,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    switch (GraphMessage)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Parameter1;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorPrivate, 0, MemorySection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &CommitGraphState,
                getDrawInfo,
                PhCommitHistory.Count
                );

            if (!CommitGraphState.Valid)
            {
                if (PhCsEnableAvxSupport)
                {
                    PhCopyConvertCircularBufferULONG(&PhCommitHistory, CommitGraphState.Data1, drawInfo->LineDataCount);
#ifdef DEBUG
                    for (i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        assert(CommitGraphState.Data1[i] == (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i));
                    }
#endif
                }
                else
                {
                    for (i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        CommitGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i);
                    }
                }

                if (PhPerfInformation.CommitLimit != 0)
                {
                    // Scale the data.
                    PhDivideSinglesBySingle(
                        CommitGraphState.Data1,
                        (FLOAT)PhPerfInformation.CommitLimit,
                        drawInfo->LineDataCount
                        );
                }

                if (PhCsEnableGraphMaxText)
                {
                    drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                    drawInfo->LabelYFunctionParameter = (FLOAT)UInt32x32To64(PhPerfInformation.CommitLimit, PAGE_SIZE);
                }

                CommitGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Parameter1;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (CommitGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG usedPages;
                    PH_FORMAT format[3];

                    usedPages = PhGetItemCircularBuffer_ULONG(&PhCommitHistory, getTooltipText->Index);

                    // Commit charge: %s\n%s
                    //PhInitFormatS(&format[0], L"Commit charge: ");
                    PhInitFormatSize(&format[0], UInt32x32To64(usedPages, PAGE_SIZE));
                    PhInitFormatC(&format[1], L'\n');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&CommitGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                }

                getTooltipText->Text = CommitGraphState.TooltipText->sr;
            }
        }
        break;
    }

    return TRUE;
}

BOOLEAN NTAPI PhSipNotifyPhysicalGraph(
    _In_ HWND GraphHandle,
    _In_ ULONG GraphMessage,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    switch (GraphMessage)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Parameter1;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorPhysical, 0, MemorySection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &PhysicalGraphState,
                getDrawInfo,
                PhPhysicalHistory.Count
                );

            if (!PhysicalGraphState.Valid)
            {
                if (PhCsEnableAvxSupport)
                {
                    PhCopyConvertCircularBufferULONG(&PhPhysicalHistory, PhysicalGraphState.Data1, drawInfo->LineDataCount);
#ifdef DEBUG
                    for (i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        assert(PhysicalGraphState.Data1[i] == (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i));
                    }
#endif
                }
                else
                {
                    for (i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        PhysicalGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i);
                    }
                }

                if (PhSystemBasicInformation.NumberOfPhysicalPages != 0)
                {
                    // Scale the data.
                    PhDivideSinglesBySingle(
                        PhysicalGraphState.Data1,
                        (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages,
                        drawInfo->LineDataCount
                        );
                }

                if (PhCsEnableGraphMaxText)
                {
                    drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                    drawInfo->LabelYFunctionParameter = (FLOAT)UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE);
                }

                PhysicalGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Parameter1;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (PhysicalGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG usedPages;
                    FLOAT currentCompressedMemory;
                    FLOAT totalCompressedMemory;
                    FLOAT totalSavedMemory;

                    usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                    if (PhSipGetMemoryCompressionLimits(&currentCompressedMemory, &totalCompressedMemory, &totalSavedMemory))
                    {
                        PH_FORMAT format[13];

                        PhInitFormatS(&format[0], L"Physical memory: ");
                        PhInitFormatSize(&format[1], UInt32x32To64(usedPages, PAGE_SIZE));
                        PhInitFormatC(&format[2], L'\n');
                        PhInitFormatS(&format[3], L"Compressed memory: ");
                        PhInitFormatSize(&format[4], (ULONG64)currentCompressedMemory);
                        PhInitFormatC(&format[5], L'\n');
                        PhInitFormatS(&format[6], L"Total compressed: ");
                        PhInitFormatSize(&format[7], (ULONG64)totalCompressedMemory);
                        PhInitFormatC(&format[8], L'\n');
                        PhInitFormatS(&format[9], L"Total memory saved: ");
                        PhInitFormatSize(&format[10], (ULONG64)totalSavedMemory);
                        PhInitFormatC(&format[11], L'\n');
                        PhInitFormatSR(&format[12], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                        PhMoveReference(&PhysicalGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                    }
                    else
                    {
                        PH_FORMAT format[3];

                        // Physical memory: %s\n%s
                        PhInitFormatSize(&format[0], UInt32x32To64(usedPages, PAGE_SIZE));
                        PhInitFormatC(&format[1], L'\n');
                        PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                        PhMoveReference(&PhysicalGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                    }
                }

                getTooltipText->Text = PhysicalGraphState.TooltipText->sr;
            }
        }
        break;
    }

    return TRUE;
}

VOID PhSipUpdateMemoryGraphs(
    VOID
    )
{
    CommitGraphState.Valid = FALSE;
    CommitGraphState.TooltipIndex = ULONG_MAX;
    Graph_Update(CommitGraphHandle);

    PhysicalGraphState.Valid = FALSE;
    PhysicalGraphState.TooltipIndex = ULONG_MAX;
    Graph_Update(PhysicalGraphHandle);
}

VOID PhSipUpdateMemoryPanel(
    VOID
    )
{
    PWSTR pagedLimit;
    PWSTR nonPagedLimit;
    SYSTEM_MEMORY_LIST_INFORMATION memoryListInfo;

    // Hardware

    if (MemoryTicked == 0)
    {
        if (PhGetVirtualStatus() == PhVirtualStatusVirtualMachine)
        {
            PhSetDialogItemText(MemoryPanel, IDC_ZMEMSLOTS_V, L"N/A");
            PhSetDialogItemText(MemoryPanel, IDC_ZMEMFORMFACTOR_V, L"N/A");
            PhSetDialogItemText(MemoryPanel, IDC_ZMEMTYPE_V, L"N/A");
            PhSetDialogItemText(MemoryPanel, IDC_ZMEMTECHNOLOGY_V, L"N/A");
            PhSetDialogItemText(MemoryPanel, IDC_ZMEMSPEED_V, L"N/A");
        }
        else
        {
            PH_FORMAT format[3];
            PCWSTR string;

            PhInitFormatU(&format[0], MemorySlotsUsed);
            PhInitFormatS(&format[1], L" of ");
            PhInitFormatU(&format[2], MemorySlotsTotal);
            PhSetDialogItemText(MemoryPanel, IDC_ZMEMSLOTS_V, PhaFormat(format, 3, 10)->Buffer);

            if (PhFindStringSiKeyValuePairs(MemoryFormFactors, sizeof(MemoryFormFactors), MemoryFormFactor, &string))
                PhSetDialogItemText(MemoryPanel, IDC_ZMEMFORMFACTOR_V, string);
            else
                PhSetDialogItemText(MemoryPanel, IDC_ZMEMFORMFACTOR_V, L"Undefined");

            if (PhFindStringSiKeyValuePairs(MemoryTypes, sizeof(MemoryTypes), MemoryType, &string))
                PhSetDialogItemText(MemoryPanel, IDC_ZMEMTYPE_V, string);
            else
                PhSetDialogItemText(MemoryPanel, IDC_ZMEMTYPE_V, L"Undefined");

            if (PhFindStringSiKeyValuePairs(MemoryTechnologies, sizeof(MemoryTechnologies), MemoryTechnology, &string))
                PhSetDialogItemText(MemoryPanel, IDC_ZMEMTECHNOLOGY_V, string);
            else
                PhSetDialogItemText(MemoryPanel, IDC_ZMEMTECHNOLOGY_V, L"Undefined");

            PhInitFormatU(&format[0], MemorySpeed);
            PhInitFormatS(&format[1], L" MT/s");
            PhSetDialogItemText(MemoryPanel, IDC_ZMEMSPEED_V, PhaFormat(format, 2, 10)->Buffer);
        }
    }

    // Commit charge

    PhSetDialogItemText(MemoryPanel, IDC_ZCOMMITCURRENT_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZCOMMITPEAK_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.PeakCommitment, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZCOMMITLIMIT_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.CommitLimit, PAGE_SIZE), ULONG_MAX)->Buffer);

    // Physical memory

    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALCURRENT_V,
        PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALTOTAL_V,
        PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), ULONG_MAX)->Buffer);

    if (ReservedMemory != 0)
        PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALRESERVED_V, PhaFormatSize(ReservedMemory, ULONG_MAX)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALRESERVED_V, L"-");

    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALCACHEWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemCachePage, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALKERNELWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemCodePage, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALDRIVERWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemDriverPage, PAGE_SIZE), ULONG_MAX)->Buffer);

    // Paged pool

    PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDWORKINGSET_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentPagedPoolPage, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDVIRTUALSIZE_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.PagedPoolPages, PAGE_SIZE), ULONG_MAX)->Buffer);

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, PhaFormatUInt64(PagedAllocsDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDFREESDELTA_V, PhaFormatUInt64(PagedFreesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDFREESDELTA_V, L"-");

    // Non-paged pool

    PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDUSAGE_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.NonPagedPoolPages, PAGE_SIZE), ULONG_MAX)->Buffer);

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, PhaFormatUInt64(PagedAllocsDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDFREESDELTA_V, PhaFormatUInt64(NonPagedFreesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDFREESDELTA_V, L"-");

    // Pools

    if (MmAddressesInitialized)
    {
        SIZE_T paged;
        SIZE_T nonPaged;

        PhSipGetPoolLimits(&paged, &nonPaged);

        if (paged != MAXSIZE_T)
            pagedLimit = PhaFormatSize(paged, ULONG_MAX)->Buffer;
        else
            pagedLimit = KsiLevel() ? L"no symbols" : L"no driver";

        if (nonPaged != MAXSIZE_T)
            nonPagedLimit = PhaFormatSize(nonPaged, ULONG_MAX)->Buffer;
        else
            nonPagedLimit = L"N/A";
    }
    else
    {
        if (KsiLevel())
        {
            pagedLimit = L"no symbols";
            nonPagedLimit = L"N/A";
        }
        else
        {
            pagedLimit = L"no driver";
            nonPagedLimit = L"N/A";
        }
    }

    PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDLIMIT_V, pagedLimit);
    PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDLIMIT_V, nonPagedLimit);

    // Paging

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEFAULTSDELTA_V, PhaFormatUInt64(PageFaultsDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEFAULTSDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEREADSDELTA_V, PhaFormatUInt64(PageReadsDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEREADSDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEFILEWRITESDELTA_V, PhaFormatUInt64(PagefileWritesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEFILEWRITESDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGMAPPEDWRITESDELTA_V, PhaFormatUInt64(MappedWritesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGMAPPEDWRITESDELTA_V, L"-");

    // Mapped

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZMAPPEDREADIO, PhaFormatSize(MappedIoReadDelta.Delta, ULONG_MAX)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZMAPPEDREADIO, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZMAPPEDWRITEIO, PhaFormatSize(MappedIoWritesDelta.Delta, ULONG_MAX)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZMAPPEDWRITEIO, L"-");

    // Memory lists

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemMemoryListInformation,
        &memoryListInfo,
        sizeof(SYSTEM_MEMORY_LIST_INFORMATION),
        NULL
        )))
    {
        ULONG_PTR standbyPageCount;
        ULONG_PTR repurposedPageCount;
        ULONG i;

        standbyPageCount = 0;
        repurposedPageCount = 0;

        for (i = 0; i < 8; i++)
        {
            standbyPageCount += memoryListInfo.PageCountByPriority[i];
            repurposedPageCount += memoryListInfo.RepurposedPagesByPriority[i];
        }

        PhSetDialogItemText(MemoryPanel, IDC_ZLISTZEROED_V, PhaFormatSize((ULONG64)memoryListInfo.ZeroPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTFREE_V, PhaFormatSize((ULONG64)memoryListInfo.FreePageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIED_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDNOWRITE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedNoWritePageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY_V, PhaFormatSize((ULONG64)standbyPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY0_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[0] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY1_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[1] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY2_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[2] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY3_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[3] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY4_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[4] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY5_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[5] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY6_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[6] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY7_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[7] * PAGE_SIZE, ULONG_MAX)->Buffer);

        if (WindowsVersion >= WINDOWS_8)
            PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCountPageFile * PAGE_SIZE, ULONG_MAX)->Buffer);
        else
            PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
    }
    else
    {
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTZEROED_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTFREE_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIED_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDNOWRITE_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY0_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY1_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY2_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY3_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY4_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY5_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY6_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY7_V, L"N/A");
    }
}

NTSTATUS PhSipLoadMmAddresses(
    _In_ PVOID Parameter
    )
{
    PPH_STRING kernelFileName;
    PVOID kernelImageBase;
    ULONG kernelImageSize;
    PPH_SYMBOL_PROVIDER symbolProvider;
    PH_SYMBOL_INFORMATION symbolInfo;

    if (NT_SUCCESS(PhGetKernelFileNameEx(&kernelFileName, &kernelImageBase, &kernelImageSize)))
    {
        symbolProvider = PhCreateSymbolProvider(NULL);
        PhLoadSymbolProviderOptions(symbolProvider);

        PhLoadModuleSymbolProvider(
            symbolProvider,
            kernelFileName,
            kernelImageBase,
            kernelImageSize
            );

        if (PhGetSymbolFromName(
            symbolProvider,
            L"MmSizeOfPagedPoolInBytes",
            &symbolInfo
            ))
        {
            MmSizeOfPagedPoolInBytes = (PSIZE_T)symbolInfo.Address;
        }

        if (WindowsVersion < WINDOWS_8)
        {
            if (PhGetSymbolFromName(
                symbolProvider,
                L"MmMaximumNonPagedPoolInBytes",
                &symbolInfo
                ))
            {
                MmMaximumNonPagedPoolInBytes = (PSIZE_T)symbolInfo.Address;
            }
        }

        PhDereferenceObject(symbolProvider);
        PhDereferenceObject(kernelFileName);
    }

    return STATUS_SUCCESS;
}

VOID PhSipGetPoolLimits(
    _Out_ PSIZE_T Paged,
    _Out_ PSIZE_T NonPaged
    )
{
    SIZE_T paged = MAXSIZE_T;
    SIZE_T nonPaged = MAXSIZE_T;

    if (MmSizeOfPagedPoolInBytes && (KsiLevel() >= KphLevelMed))
    {
        KphReadVirtualMemory(
            NtCurrentProcess(),
            MmSizeOfPagedPoolInBytes,
            &paged,
            sizeof(SIZE_T),
            NULL
            );
    }

    if (WindowsVersion >= WINDOWS_8)
    {
        // The maximum nonpaged pool size was fixed on Windows 8 and above? The kernel does use
        // a function named MmGetMaximumNonPagedPoolInBytes() but it's not exported. We return
        // the fixed limit value from the documentation here similar to MS tools. (dmex)
        // https://learn.microsoft.com/en-us/windows/win32/memory/memory-limits-for-windows-releases#memory-and-address-space-limits
        //
        // Windows 8.1 and Windows Server 2012 R2: RAM or 16 TB, whichever is smaller:

        if (UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE) <= 16ULL * 1024ULL * 1024ULL * 1024ULL)
        {
            nonPaged = UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE);
        }
        else
        {
            nonPaged = (SIZE_T)(16ULL * 1024ULL * 1024ULL * 1024ULL);
        }
    }
    else if (WindowsVersion < WINDOWS_8 && MmMaximumNonPagedPoolInBytes && (KsiLevel() >= KphLevelMed))
    {
        KphReadVirtualMemory(
            NtCurrentProcess(),
            MmMaximumNonPagedPoolInBytes,
            &nonPaged,
            sizeof(SIZE_T),
            NULL
            );
    }

    *Paged = paged;
    *NonPaged = nonPaged;
}

_Success_(return)
BOOLEAN PhSipGetMemoryCompressionLimits(
    _Out_ FLOAT *CurrentCompressedMemory,
    _Out_ FLOAT *TotalCompressedMemory,
    _Out_ FLOAT *TotalSavedMemory
    )
{
    PH_SYSTEM_STORE_COMPRESSION_INFORMATION compressionInfo;
    FLOAT current;
    FLOAT total;
    FLOAT saved;

    if (!NT_SUCCESS(PhGetSystemCompressionStoreInformation(&compressionInfo)))
        return FALSE;
    if (!(compressionInfo.TotalDataCompressed && compressionInfo.TotalCompressedSize))
        return FALSE;

    current = (FLOAT)compressionInfo.WorkingSetSize / (FLOAT)(1024 * 1024);
    total = current * (FLOAT)compressionInfo.TotalDataCompressed / (FLOAT)compressionInfo.TotalCompressedSize;
    saved = total - current;

    *CurrentCompressedMemory = current * 1024 * 1024;
    *TotalCompressedMemory = total * 1024 * 1024;
    *TotalSavedMemory = saved * 1024 * 1024;
    return TRUE;
}
