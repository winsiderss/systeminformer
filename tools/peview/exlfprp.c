/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2022
 *
 */

#include <peview.h>
#include <mapimg.h>
#include <wslsup.h>

PWSTR PvpGetSymbolTypeName(
    _In_ UCHAR TypeInfo
    )
{
    switch (ELF_ST_TYPE(TypeInfo))
    {
    case STT_NOTYPE:
        return L"No type";
    case STT_OBJECT:
        return L"Object";
    case STT_FUNC:
        return L"Function";
    case STT_SECTION:
        return L"Section";
    case STT_FILE:
        return L"File";
    case STT_COMMON:
        return L"Common";
    case STT_TLS:
        return L"TLS";
    case STT_GNU_IFUNC:
        return L"IFUNC";
    }

    return L"***ERROR***";
}

PWSTR PvpGetSymbolBindingName(
    _In_ UCHAR TypeInfo
    )
{
    switch (ELF_ST_BIND(TypeInfo))
    {
    case STB_LOCAL:
        return L"Local";
    case STB_GLOBAL:
        return L"Global";
    case STB_WEAK:
        return L"Weak";
    case STB_GNU_UNIQUE:
        return L"Unique";
    }

    return L"***ERROR***";
}

PWSTR PvpGetSymbolVisibility(
    _In_ UCHAR OtherInfo
    )
{
    switch (ELF_ST_VISIBILITY(OtherInfo))
    {
    case STV_DEFAULT:
        return L"Default";
    case STV_INTERNAL:
        return L"Internal";
    case STV_HIDDEN:
        return L"Hidden";
    case STV_PROTECTED:
        return L"Protected";
    }

    return L"***ERROR***";
}

PPH_STRING PvpGetSymbolSectionName(
    _In_ ULONG Index
    )
{
    switch (Index)
    {
    case SHN_UNDEF:
        return PhCreateString(L"UND");
    case SHN_ABS:
        return PhCreateString(L"ABS");
    case SHN_COMMON:
        return PhCreateString(L"Common");
    }

    return PhaFormatUInt64(Index, TRUE);
}

VOID PvExlfProperties(
    VOID
    )
{
    PPV_PROPCONTEXT propContext;

    if (!PhExtractIcon(PvFileName->Buffer, &PvImageLargeIcon, &PvImageSmallIcon))
    {
        PhGetStockApplicationIcon(&PvImageSmallIcon, &PvImageLargeIcon);
    }

    if (propContext = PvCreatePropContext(PvFileName))
    {
        PPV_PROPPAGECONTEXT newPage;

        // General
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_ELFGENERAL),
            PvpExlfGeneralDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Dynamic
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_ELFDYNAMIC),
            PvpExlfDynamicDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Imports
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_ELFIMPORTS),
            PvpExlfImportsDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Exports
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_ELFEXPORTS),
            PvpExlfExportsDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Properties page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PEPROPSTORAGE),
            PvpPePropStoreDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Extended attributes page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PEATTR),
            PvpPeExtendedAttributesDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Streams page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PESTREAMS),
            PvpPeStreamsDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        // Layout page
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_PELAYOUT),
            PvpPeLayoutDlgProc,
            NULL
            );
        PvAddPropPage(propContext, newPage);

        PhModalPropertySheet(&propContext->PropSheetHeader);

        PhDereferenceObject(propContext);
    }
}

static NTSTATUS PvpQueryWslImageThreadStart(
    _In_ PVOID Parameter
    )
{
    HWND windowHandle = Parameter;

    PhInitializeLxssImageVersionInfo(&PvImageVersionInfo, PvFileName);

    PhSetDialogItemText(windowHandle, IDC_NAME, PvpGetStringOrNa(PvImageVersionInfo.FileDescription));
    PhSetDialogItemText(windowHandle, IDC_COMPANYNAME, PvpGetStringOrNa(PvImageVersionInfo.CompanyName));
    PhSetDialogItemText(windowHandle, IDC_VERSION, PvpGetStringOrNa(PvImageVersionInfo.FileVersion));

    return STATUS_SUCCESS;
}

VOID PvpSetWslmageVersionInfo(
    _In_ HWND WindowHandle
    )
{
    PhSetDialogItemText(WindowHandle, IDC_NAME, L"Loading...");
    PhSetDialogItemText(WindowHandle, IDC_COMPANYNAME, L"Loading...");
    PhSetDialogItemText(WindowHandle, IDC_VERSION, L"Loading...");

    PhCreateThread2(PvpQueryWslImageThreadStart, WindowHandle);

    Static_SetIcon(GetDlgItem(WindowHandle, IDC_FILEICON), PvImageLargeIcon);
}

VOID PvpSetWslImageType(
    _In_ HWND hwndDlg
    )
{
    PWSTR type = L"N/A";

    switch (PvMappedImage.Header->e_type)
    {
    case ET_REL:
        type = L"Relocatable";
        break;
    case ET_DYN:
        type = L"Dynamic";
        break;
    case ET_EXEC:
        type = L"Executable";
        break;
    default:
        type = L"ERROR";
        break;
    }

    PhSetDialogItemText(hwndDlg, IDC_IMAGETYPE, type);
}

VOID PvpSetWslImageMachineType(
    _In_ HWND hwndDlg
    )
{
    PWSTR type = L"N/A";

    switch (PvMappedImage.Header->e_machine)
    {
    case EM_386:
        type = L"i386";
        break;
    case EM_X86_64:
        type = L"AMD64";
        break;
    default:
        type = L"ERROR";
        break;
    }

    PhSetDialogItemText(hwndDlg, IDC_TARGETMACHINE, type);
}

VOID PvpSetWslImageBase(
    _In_ HWND hwndDlg
    )
{
    PPH_STRING string;

    if (PvMappedImage.Header->e_ident[EI_CLASS] == ELFCLASS32)
    {
        string = PhFormatString(L"0x%I32x", (ULONG)PhGetMappedWslImageBaseAddress(&PvMappedImage));
        PhSetDialogItemText(hwndDlg, IDC_IMAGEBASE, string->Buffer);
        PhDereferenceObject(string);
    }
    else if (PvMappedImage.Header->e_ident[EI_CLASS] == ELFCLASS64)
    {
        string = PhFormatString(L"0x%I64x", PhGetMappedWslImageBaseAddress(&PvMappedImage));
        PhSetDialogItemText(hwndDlg, IDC_IMAGEBASE, string->Buffer);
        PhDereferenceObject(string);
    }
}

VOID PvpSetWslEntrypoint(
    _In_ HWND hwndDlg
    )
{
    PPH_STRING string;

    if (PvMappedImage.Header->e_ident[EI_CLASS] == ELFCLASS32)
    {
        string = PhFormatString(L"0x%I32x", PvMappedImage.Headers32->e_entry);
        PhSetDialogItemText(hwndDlg, IDC_ENTRYPOINT, string->Buffer);
        PhDereferenceObject(string);
    }
    else if (PvMappedImage.Header->e_ident[EI_CLASS] == ELFCLASS64)
    {
        string = PhFormatString(L"0x%I64x", PvMappedImage.Headers64->e_entry);
        PhSetDialogItemText(hwndDlg, IDC_ENTRYPOINT, string->Buffer);
        PhDereferenceObject(string);
    }
}

PWSTR PvpGetWslImageSectionTypeName(
    _In_ UINT32 Type
    )
{
    switch (Type)
    {
    case SHT_NULL:
        return L"NULL";
    case SHT_PROGBITS:
        return L"PROGBITS";
    case SHT_SYMTAB:
        return L"SYMTAB";
    case SHT_STRTAB:
        return L"STRTAB";
    case SHT_RELA:
        return L"RELA";
    case SHT_HASH:
        return L"HASH";
    case SHT_DYNAMIC:
        return L"DYNAMIC";
    case SHT_NOTE:
        return L"NOTE";
    case SHT_NOBITS:
        return L"NOBITS";
    case SHT_REL:
        return L"REL";
    case SHT_SHLIB:
        return L"SHLIB";
    case SHT_DYNSYM:
        return L"DYNSYM";
    case SHT_NUM:
        return L"NUM";
    case SHT_INIT_ARRAY:
        return L"INIT_ARRAY";
    case SHT_FINI_ARRAY:
        return L"FINI_ARRAY";
    case SHT_PREINIT_ARRAY:
        return L"PREINIT_ARRAY";
    case SHT_GROUP:
        return L"GROUP";
    case SHT_SYMTAB_SHNDX:
        return L"SYMTAB_SHNDX";
    case SHT_GNU_INCREMENTAL_INPUTS:
        return L"GNU_INCREMENTAL_INPUTS";
    case SHT_GNU_ATTRIBUTES:
        return L"GNU_ATTRIBUTES";
    case SHT_GNU_HASH:
        return L"GNU_HASH";
    case SHT_GNU_LIBLIST:
        return L"GNU_LIBLIST";
    case SHT_SUNW_verdef:
        return L"VERDEF";
    case SHT_SUNW_verneed:
        return L"VERNEED";
    case SHT_SUNW_versym:
        return L"VERSYM";
    }

    return L"***ERROR***";
}

PPH_STRING PvpGetWslImageSectionFlagsString(
    _In_ ULONGLONG Flags
    )
{
    PH_STRING_BUILDER sb;

    PhInitializeStringBuilder(&sb, 100);

    if (Flags & SHF_ALLOC)
        PhAppendStringBuilder2(&sb, L"Allocated, ");

    if (!(Flags & SHF_WRITE))
        PhAppendStringBuilder2(&sb, L"Read-only, ");

    if (Flags & SHF_EXECINSTR)
        PhAppendStringBuilder2(&sb, L"Code, ");
    else
        PhAppendStringBuilder2(&sb, L"Data, ");

    if (sb.String->Length != 0)
        PhRemoveEndStringBuilder(&sb, 2);
    else
        PhAppendStringBuilder2(&sb, L"(None)");

    // TODO: The "objdump -h /bin/su --wide" command shows section flags
    // such as CONTENT which appears to be based on ElfSectionType != SHT_NOBITS
    // but I can't find the relevant source-code and check.

    return PhFinalStringBuilderString(&sb);
}

VOID PvpLoadWslSections(
    _In_ HWND LvHandle
    )
{
    USHORT sectionCount;
    PPH_ELF_IMAGE_SECTION imageSections;

    if (PhGetMappedWslImageSections(&PvMappedImage, &sectionCount, &imageSections))
    {
        for (USHORT i = 0; i < sectionCount; i++)
        {
            INT lvItemIndex;
            WCHAR pointer[PH_PTR_STR_LEN_1];

            if (!imageSections[i].Address && !imageSections[i].Size)
                continue;

            lvItemIndex = PhAddListViewItem(LvHandle, MAXINT, imageSections[i].Name, NULL);
            PhSetListViewSubItem(LvHandle, lvItemIndex, 1, PvpGetWslImageSectionTypeName(imageSections[i].Type));

            if (imageSections[i].Address)
            {
                PhPrintPointer(pointer, (PVOID)imageSections[i].Address);
                PhSetListViewSubItem(LvHandle, lvItemIndex, 2, pointer);
            }

            PhPrintPointer(pointer, (PVOID)imageSections[i].Offset);
            PhSetListViewSubItem(LvHandle, lvItemIndex, 3, pointer);

            PhSetListViewSubItem(LvHandle, lvItemIndex, 4, PhaFormatSize(imageSections[i].Size, ULONG_MAX)->Buffer);
            PhSetListViewSubItem(LvHandle, lvItemIndex, 5, PH_AUTO_T(PH_STRING, PvpGetWslImageSectionFlagsString(imageSections[i].Flags))->Buffer);
        }

        PhFree(imageSections);
    }
}

INT_PTR CALLBACK PvpExlfGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND lvHandle;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Type");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"VA");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Offset");
            PhAddListViewColumn(lvHandle, 4, 4, 4, LVCFMT_LEFT, 80, L"Size");
            PhAddListViewColumn(lvHandle, 5, 5, 5, LVCFMT_LEFT, 80, L"Flags");
            PhSetExtendedListView(lvHandle);
            PhLoadListViewColumnsFromSetting(L"GeneralWslTreeListColumns", lvHandle);

            PvpSetWslmageVersionInfo(hwndDlg);
            PvpSetWslImageType(hwndDlg);
            PvpSetWslImageMachineType(hwndDlg);
            PvpSetWslImageBase(hwndDlg);
            PvpSetWslEntrypoint(hwndDlg);

            PvpLoadWslSections(lvHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(L"GeneralWslTreeListColumns", GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILE), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_COMPANYNAME), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_LIST), dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, GetDlgItem(hwndDlg, IDC_LIST));
        }
        break;
    }

    return FALSE;
}
