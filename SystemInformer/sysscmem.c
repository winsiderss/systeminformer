/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2026
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
#include <graphbar.h>

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
static RECT MemoryGraphMarginScaled;
static HWND CommitGraphHandle;
static PH_GRAPH_STATE CommitGraphState;
static HWND PhysicalGraphHandle;
static PH_GRAPH_STATE PhysicalGraphState;
static HWND MemoryGraphBarHandle;
static HWND MemoryPanel;
static ULONG MemoryTicked;
static PH_UINT32_DELTA PagedAllocsDelta;
static PH_UINT32_DELTA PagedFreesDelta;
static PH_UINT32_DELTA PagedPoolPagesDelta;
static PH_UINT32_DELTA NonPagedAllocsDelta;
static PH_UINT32_DELTA NonPagedFreesDelta;
static PH_UINT32_DELTA NonPagedPoolPagesDelta;
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

static BOOLEAN PhSipGetMemoryListInfo(
    _Out_ PSYSTEM_MEMORY_LIST_INFORMATION MemoryListInfo,
    _Out_opt_ PULONG_PTR StandbyPageCount
    );

_Function_class_(PH_GRAPHBAR_MESSAGE_CALLBACK)
static BOOLEAN NTAPI PhSipNotifyMemoryGraphBar(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

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

_Function_class_(PH_SYSINFO_SECTION_CALLBACK)
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
            ShowCommitInSummary = !!PhGetIntegerSetting(SETTING_SHOW_COMMIT_IN_SUMMARY);
            MemorySection = Section;
            MemorySlotsTotal = 0;
            MemorySlotsUsed = 0;
            MemoryFormFactor = 0;
            MemoryType = 0;
            MemorySpeed = 0;
            PhEnumSMBIOS(PhpSipMemorySMBIOSCallback, NULL);

            PhGraphBarInitialization();
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

            if (MemoryGraphBarHandle)
            {
                InvalidateRect(MemoryGraphBarHandle, NULL, FALSE);
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
    PhInitializeDelta(&PagedPoolPagesDelta);
    PhInitializeDelta(&NonPagedAllocsDelta);
    PhInitializeDelta(&NonPagedFreesDelta);
    PhInitializeDelta(&NonPagedPoolPagesDelta);
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
    MemoryGraphBarHandle = NULL;
}

VOID PhSipTickMemoryDialog(
    VOID
    )
{
    PhUpdateDelta(&PagedAllocsDelta, PhPerfInformation.PagedPoolAllocs);
    PhUpdateDelta(&PagedFreesDelta, PhPerfInformation.PagedPoolFrees);
    PhUpdateDelta(&PagedPoolPagesDelta, PhPerfInformation.PagedPoolPages);
    PhUpdateDelta(&NonPagedAllocsDelta, PhPerfInformation.NonPagedPoolAllocs);
    PhUpdateDelta(&NonPagedFreesDelta, PhPerfInformation.NonPagedPoolFrees);
    PhUpdateDelta(&NonPagedPoolPagesDelta, PhPerfInformation.NonPagedPoolPages);
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
            MemoryGraphMarginScaled = MemoryGraphMargin;
            PhGetMarginDpiValue(&MemoryGraphMarginScaled, MemorySection->Parameters->WindowDpi, TRUE);

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
            PhGetMarginDpiValue(&margin, MemorySection->Parameters->WindowDpi, TRUE);
            PhAddLayoutItemEx(&MemoryLayoutManager, MemoryPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, &margin);

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
            MemoryGraphMarginScaled = MemoryGraphMargin;
            PhGetMarginDpiValue(&MemoryGraphMarginScaled, MemorySection->Parameters->WindowDpi, TRUE);

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

            PhLayoutManagerUpdate(&MemoryLayoutManager, LOWORD(wParam));
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
    PH_GRAPHBAR_CREATEPARAMS graphBarCreateParams;

    memset(&graphCreateParams, 0, sizeof(PH_GRAPH_CREATEPARAMS));
    graphCreateParams.Size = sizeof(PH_GRAPH_CREATEPARAMS);
    graphCreateParams.Callback = PhSipNotifyCommitGraph;

    CommitGraphHandle = PhCreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER,
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
    PhBringWindowToTop(CommitGraphHandle);

    memset(&graphCreateParams, 0, sizeof(PH_GRAPH_CREATEPARAMS));
    graphCreateParams.Size = sizeof(PH_GRAPH_CREATEPARAMS);
    graphCreateParams.Callback = PhSipNotifyPhysicalGraph;

    PhysicalGraphHandle = PhCreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER,
        0,
        0,
        0,
        0,
        MemoryDialog,
        NULL,
        NULL,
        &graphCreateParams
        );
    Graph_SetTooltip(PhysicalGraphHandle, TRUE);
    PhBringWindowToTop(CommitGraphHandle);

    memset(&graphBarCreateParams, 0, sizeof(PH_GRAPHBAR_CREATEPARAMS));
    graphBarCreateParams.Size = sizeof(PH_GRAPHBAR_CREATEPARAMS);
    graphBarCreateParams.Callback = PhSipNotifyMemoryGraphBar;

    MemoryGraphBarHandle = PhCreateWindow(
        PH_GRAPHBAR_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER,
        0,
        0,
        0,
        0,
        MemoryDialog,
        NULL,
        PhInstanceHandle,
        &graphBarCreateParams
        );
    PhBringWindowToTop(MemoryGraphBarHandle);
}

VOID PhSipLayoutMemoryGraphs(
    _In_ HWND hwnd
    )
{
    RECT clientRect;
    RECT labelRect;
    RECT marginRect;
    LONG availableHeight;
    LONG graphWidth;
    LONG graphHeight;
    LONG graphBarHeight;
    HDWP deferHandle;
    ULONG y;

    marginRect = MemoryGraphMarginScaled;

    if (!PhGetClientRect(MemoryDialog, &clientRect))
        return;
    if (!PhGetClientRect(GetDlgItem(MemoryDialog, IDC_COMMIT_L), &labelRect))
        return;

    graphWidth = max(clientRect.right - marginRect.left - marginRect.right, 1);
    graphBarHeight = PhScaleToDisplay(40, MemorySection->Parameters->WindowDpi);
    availableHeight = clientRect.bottom - marginRect.top - marginRect.bottom - labelRect.bottom * 2 - graphBarHeight - MemorySection->Parameters->MemoryPadding * 4;
    graphHeight = max(availableHeight / 2, 1);

    deferHandle = BeginDeferWindowPos(5);
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
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + MemorySection->Parameters->MemoryPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        MemoryGraphBarHandle,
        NULL,
        marginRect.left,
        y,
        graphWidth,
        graphBarHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

_Function_class_(PH_GRAPH_MESSAGE_CALLBACK)
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

            if (PhPerfInformation.CommitLimit != 0)
            {
                ULONG commitUsagePages = PhPerfInformation.CommittedPages;
                ULONG commitLimitPages = PhPerfInformation.CommitLimit;
                ULONG commitUsagePercent = PhMultiplyDivide(commitUsagePages, 100, commitLimitPages);
                PH_FORMAT format[6];
                HDC hdc;

                // <current> / <total> <percent>%
                PhInitFormatSize(&format[0], UInt32x32To64(commitUsagePages, PAGE_SIZE));
                PhInitFormatS(&format[1], L" / ");
                PhInitFormatSize(&format[2], UInt32x32To64(commitLimitPages, PAGE_SIZE));
                PhInitFormatS(&format[3], L" ");
                PhInitFormatU(&format[4], commitUsagePercent);
                PhInitFormatS(&format[5], L"%");

                PhMoveReference(&CommitGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                hdc = Graph_GetBufferedContext(GraphHandle);
                PhSetGraphText(
                    hdc,
                    drawInfo,
                    &CommitGraphState.Text->sr,
                    &PhNormalGraphTextMargin,
                    &PhNormalGraphTextPadding,
                    PH_ALIGN_TOP | PH_ALIGN_LEFT
                    );
            }
            else
            {
                drawInfo->Text.Buffer = NULL;
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

_Function_class_(PH_GRAPH_MESSAGE_CALLBACK)
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

            if (PhSystemBasicInformation.NumberOfPhysicalPages != 0)
            {
                ULONG physicalUsagePages = PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages;
                ULONG physicalTotalPages = PhSystemBasicInformation.NumberOfPhysicalPages;
                ULONG physicalUsagePercent = PhMultiplyDivide(physicalUsagePages, 100, physicalTotalPages);
                PH_FORMAT format[6];
                HDC hdc;

                // <current> / <total> <percent>%
                PhInitFormatSize(&format[0], UInt32x32To64(physicalUsagePages, PAGE_SIZE));
                PhInitFormatS(&format[1], L" / ");
                PhInitFormatSize(&format[2], UInt32x32To64(physicalTotalPages, PAGE_SIZE));
                PhInitFormatS(&format[3], L" ");
                PhInitFormatU(&format[4], physicalUsagePercent);
                PhInitFormatS(&format[5], L"%");

                PhMoveReference(&PhysicalGraphState.Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                hdc = Graph_GetBufferedContext(GraphHandle);
                PhSetGraphText(
                    hdc,
                    drawInfo,
                    &PhysicalGraphState.Text->sr,
                    &PhNormalGraphTextMargin,
                    &PhNormalGraphTextPadding,
                    PH_ALIGN_TOP | PH_ALIGN_LEFT
                    );
            }
            else
            {
                drawInfo->Text.Buffer = NULL;
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
                    DOUBLE currentCompressedMemory;
                    DOUBLE totalCompressedMemory;
                    DOUBLE totalSavedMemory;

                    usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                    if (NT_SUCCESS(PhSipGetMemoryCompressionLimits(&currentCompressedMemory, &totalCompressedMemory, &totalSavedMemory)))
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

static BOOLEAN PhSipGetMemoryListInfo(
    _Out_ PSYSTEM_MEMORY_LIST_INFORMATION MemoryListInfo,
    _Out_opt_ PULONG_PTR StandbyPageCount
    )
{
    if (!NT_SUCCESS(NtQuerySystemInformation(
        SystemMemoryListInformation,
        MemoryListInfo,
        sizeof(SYSTEM_MEMORY_LIST_INFORMATION),
        NULL
        )))
    {
        if (StandbyPageCount)
            *StandbyPageCount = 0;

        return FALSE;
    }

    if (StandbyPageCount)
    {
        ULONG_PTR standbyPageCount = 0;

        for (ULONG i = 0; i < RTL_NUMBER_OF(MemoryListInfo->PageCountByPriority); i++)
        {
            standbyPageCount += MemoryListInfo->PageCountByPriority[i];
        }

        *StandbyPageCount = standbyPageCount;
    }

    return TRUE;
}

_Function_class_(PH_GRAPHBAR_MESSAGE_CALLBACK)
static BOOLEAN NTAPI PhSipNotifyMemoryGraphBar(
    _In_ HWND WindowHandle,
    _In_ ULONG Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    switch (Message)
    {
    case GBN_DRAWBAR:
        {
            PPH_GRAPHBAR_DRAWBAR drawBar = (PPH_GRAPHBAR_DRAWBAR)Parameter1;
            SYSTEM_MEMORY_LIST_INFORMATION memoryListInfo;
            ULONG_PTR standbyPageCount;
            ULONG64 totalPages;
            ULONG64 segmentPages[6];
            COLORREF segmentColors[RTL_NUMBER_OF(segmentPages)];
            RECT rect;
            HBRUSH oldBrush;
            HPEN oldPen;
            LONG x;
            ULONG64 consumedPages;
            ULONG64 usedPages;

            rect = drawBar->Rect;

            FillRect(
                drawBar->hdc,
                &rect,
                PhEnableThemeSupport ? PhThemeWindowBackgroundBrush : GetSysColorBrush(COLOR_WINDOW)
                );

            if (!PhSipGetMemoryListInfo(&memoryListInfo, &standbyPageCount))
                break;

            totalPages = PhSystemBasicInformation.NumberOfPhysicalPages;

            if (!totalPages || rect.right <= rect.left || rect.bottom <= rect.top)
                break;

            usedPages = totalPages;
            usedPages -= min(usedPages, (ULONG64)memoryListInfo.ZeroPageCount);
            usedPages -= min(usedPages, (ULONG64)memoryListInfo.FreePageCount);
            usedPages -= min(usedPages, (ULONG64)memoryListInfo.ModifiedPageCount);
            usedPages -= min(usedPages, (ULONG64)memoryListInfo.ModifiedNoWritePageCount);
            usedPages -= min(usedPages, (ULONG64)standbyPageCount);

            segmentPages[0] = usedPages;
            segmentPages[1] = memoryListInfo.ModifiedPageCount;
            segmentPages[2] = memoryListInfo.ModifiedNoWritePageCount;
            segmentPages[3] = standbyPageCount;
            segmentPages[4] = memoryListInfo.ZeroPageCount;
            segmentPages[5] = memoryListInfo.FreePageCount;

            segmentColors[0] = PhEnableThemeSupport ? PhHalveColorBrightness(RGB(0x67, 0xbb, 0x46)) : RGB(0x67, 0xbb, 0x46); // In Use (green)
            segmentColors[1] = PhEnableThemeSupport ? PhHalveColorBrightness(RGB(0xee, 0x89, 0x3d)) : RGB(0xee, 0x89, 0x3d); // Modified (orange)
            segmentColors[2] = PhEnableThemeSupport ? RGB(0x95, 0x4f, 0x16) : RGB(0xc6, 0x70, 0x2e); // Modified NoWrite (darker orange)
            segmentColors[3] = PhEnableThemeSupport ? PhHalveColorBrightness(RGB(0x4e, 0xa4, 0xe0)) : RGB(0x4e, 0xa4, 0xe0); // Standby (blue)
            segmentColors[4] = PhEnableThemeSupport ? RGB(0x7a, 0x7a, 0x7a) : RGB(0x9e, 0x9e, 0x9e); // Zero (gray)
            segmentColors[5] = PhEnableThemeSupport ? PhHalveColorBrightness(RGB(0x48, 0xc0, 0xc4)) : RGB(0x48, 0xc0, 0xc4); // Free (teal)

            oldBrush = SelectBrush(drawBar->hdc, PhGetStockBrush(DC_BRUSH));
            oldPen = SelectPen(drawBar->hdc, PhGetStockPen(DC_PEN));

            x = rect.left;
            consumedPages = 0;

            for (ULONG i = 0; i < RTL_NUMBER_OF(segmentPages); i++)
            {
                RECT fillRect;
                LONG right;
                ULONG64 scaledWidth = 0;

                if (!segmentPages[i])
                    continue;

                consumedPages += segmentPages[i];

                if (i == RTL_NUMBER_OF(segmentPages) - 1)
                    right = rect.right;
                else
                {
                    PhMultiplyDivide64(consumedPages, rect.right - rect.left, totalPages, &scaledWidth);
                    right = rect.left + (LONG)scaledWidth;
                }

                if (right <= x)
                    continue;

                fillRect.left = x;
                fillRect.top = rect.top;
                fillRect.right = right;
                fillRect.bottom = rect.bottom;

                SetDCBrushColor(drawBar->hdc, segmentColors[i]);
                FillRect(drawBar->hdc, &fillRect, PhGetStockBrush(DC_BRUSH));

                if (x != rect.left)
                {
                    SetDCPenColor(drawBar->hdc, RGB(0x00, 0x00, 0x00));
                    MoveToEx(drawBar->hdc, x, rect.top, NULL);
                    LineTo(drawBar->hdc, x, rect.bottom);
                }

                x = right;
            }

            SelectBrush(drawBar->hdc, oldBrush);
            SelectPen(drawBar->hdc, oldPen);
        }
        break;
    case GBN_GETTOOLTIPTEXT:
        {
            PPH_GRAPHBAR_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPHBAR_GETTOOLTIPTEXT)Parameter1;
            SYSTEM_MEMORY_LIST_INFORMATION memoryListInfo;
            ULONG_PTR standbyPageCount;
            ULONG64 totalPages;
            ULONG64 segmentPages[6];
            PCWSTR segmentNames[RTL_NUMBER_OF(segmentPages)];
            ULONG64 usedPages;
            LONG barWidth;
            LONG x;
            ULONG64 consumedPages;
            ULONG hitIndex;
            BOOLEAN hit;
            PH_FORMAT format[3];

            if (!PhSipGetMemoryListInfo(&memoryListInfo, &standbyPageCount))
                break;

            totalPages = PhSystemBasicInformation.NumberOfPhysicalPages;
            barWidth = getTooltipText->Rect.right - getTooltipText->Rect.left;

            if (!totalPages || barWidth <= 0)
                break;

            usedPages = totalPages;
            usedPages -= min(usedPages, (ULONG64)memoryListInfo.ZeroPageCount);
            usedPages -= min(usedPages, (ULONG64)memoryListInfo.FreePageCount);
            usedPages -= min(usedPages, (ULONG64)memoryListInfo.ModifiedPageCount);
            usedPages -= min(usedPages, (ULONG64)memoryListInfo.ModifiedNoWritePageCount);
            usedPages -= min(usedPages, (ULONG64)standbyPageCount);

            segmentPages[0] = usedPages;
            segmentPages[1] = memoryListInfo.ModifiedPageCount;
            segmentPages[2] = memoryListInfo.ModifiedNoWritePageCount;
            segmentPages[3] = standbyPageCount;
            segmentPages[4] = memoryListInfo.ZeroPageCount;
            segmentPages[5] = memoryListInfo.FreePageCount;

            segmentNames[0] = L"In use";
            segmentNames[1] = L"Modified";
            segmentNames[2] = L"Modified (no write)";
            segmentNames[3] = L"Standby";
            segmentNames[4] = L"Zeroed";
            segmentNames[5] = L"Free";

            x = getTooltipText->Rect.left;
            consumedPages = 0;
            hit = FALSE;
            hitIndex = 0;

            for (ULONG i = 0; i < RTL_NUMBER_OF(segmentPages); i++)
            {
                LONG right;
                ULONG64 scaledWidth = 0;

                if (!segmentPages[i])
                    continue;

                consumedPages += segmentPages[i];

                if (i == RTL_NUMBER_OF(segmentPages) - 1)
                    right = getTooltipText->Rect.right;
                else
                {
                    PhMultiplyDivide64(consumedPages, barWidth, totalPages, &scaledWidth);
                    right = getTooltipText->Rect.left + (LONG)scaledWidth;
                }

                if (right <= x)
                    continue;

                if (getTooltipText->CursorX >= x && getTooltipText->CursorX < right)
                {
                    hit = TRUE;
                    hitIndex = i;
                    break;
                }

                x = right;
            }

            if (!hit)
                break;

            PhInitFormatS(&format[0], segmentNames[hitIndex]);
            PhInitFormatS(&format[1], L": ");
            PhInitFormatSize(&format[2], segmentPages[hitIndex] * PAGE_SIZE);

            getTooltipText->Text = PhFormat(format, RTL_NUMBER_OF(format), 0);
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

    if (MemoryGraphBarHandle)
        InvalidateRect(MemoryGraphBarHandle, NULL, FALSE);
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
    {
        LONG delta = (LONG)PagedPoolPagesDelta.Delta;

        if (delta != 0)
        {
            PH_FORMAT format[2];

            PhInitFormatS(&format[0], (delta > 0 ? L"+" : L"-"));
            PhInitFormatSize(&format[1], UInt32x32To64((ULONG)abs(delta), PAGE_SIZE));

            PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, PhFormat(format, 2, 0)->Buffer);
        }
        else
        {
            PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, L"0");
        }
    }
    else
    {
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, L"-");
    }

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDFREESDELTA_V, PhaFormatUInt64(PagedFreesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDFREESDELTA_V, L"-");

    // Non-paged pool

    PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDUSAGE_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.NonPagedPoolPages, PAGE_SIZE), ULONG_MAX)->Buffer);

    if (MemoryTicked > 1)
    {
        LONG delta = (LONG)NonPagedPoolPagesDelta.Delta;

        if (delta != 0)
        {
            PH_FORMAT format[2];
            PPH_STRING string;

            PhInitFormatS(&format[0], (delta > 0 ? L"+" : L"-"));
            PhInitFormatSize(&format[1], UInt32x32To64((ULONG)abs(delta), PAGE_SIZE));

            string = PhFormat(format, 2, 0);
            PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, string->Buffer);
            PhDereferenceObject(string);
        }
        else
        {
            PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, L"0");
        }
    }
    else
    {
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, L"-");
    }

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

_Function_class_(USER_THREAD_START_ROUTINE)
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

    if (MmSizeOfPagedPoolInBytes && (KsiLevel() == KphLevelMax))
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
    else if (WindowsVersion < WINDOWS_8 && MmMaximumNonPagedPoolInBytes && (KsiLevel() == KphLevelMax))
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

NTSTATUS PhSipGetMemoryCompressionLimits(
    _Out_ PDOUBLE CurrentCompressedMemory,
    _Out_ PDOUBLE TotalCompressedMemory,
    _Out_ PDOUBLE TotalSavedMemory
    )
{
    PH_SYSTEM_STORE_COMPRESSION_INFORMATION compressionInfo;
    NTSTATUS status;
    DOUBLE current;
    DOUBLE ratio;
    DOUBLE total;
    DOUBLE saved;

    status = PhGetSystemCompressionStoreInformation(&compressionInfo);

    if (!NT_SUCCESS(status))
        return status;
    if (!(compressionInfo.TotalDataCompressed && compressionInfo.TotalCompressedSize))
        return STATUS_FAIL_CHECK;

    current = (DOUBLE)compressionInfo.WorkingSetSize;
    ratio = (DOUBLE)compressionInfo.TotalDataCompressed / (DOUBLE)compressionInfo.TotalCompressedSize;
    total = current * ratio;
    saved = total - current;

    *CurrentCompressedMemory = current;
    *TotalCompressedMemory = total;
    *TotalSavedMemory = saved;
    return status;
}
