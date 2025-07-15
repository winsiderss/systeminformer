/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include <peview.h>

// https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64

#ifndef UNW_FLAG_NHANDLER
#define UNW_FLAG_NHANDLER       0x0
#endif
#ifndef UNW_FLAG_EHANDLER
#define UNW_FLAG_EHANDLER       0x1
#endif
#ifndef UNW_FLAG_UHANDLER
#define UNW_FLAG_UHANDLER       0x2
#endif
#ifndef UNW_FLAG_CHAININFO
#define UNW_FLAG_CHAININFO      0x4
#endif

typedef enum _IMAGE_AMD64_UNWIND_OP_CODES
{
    AMD64_UWOP_PUSH_NONVOL = 0, /* info == register number */
    AMD64_UWOP_ALLOC_LARGE,     /* no info, alloc size in next 2 slots */
    AMD64_UWOP_ALLOC_SMALL,     /* info == size of allocation / 8 - 1 */
    AMD64_UWOP_SET_FPREG,       /* no info, FP = RSP + UNWIND_INFO.FPRegOffset*16 */
    AMD64_UWOP_SAVE_NONVOL,     /* info == register number, offset in next slot */
    AMD64_UWOP_SAVE_NONVOL_FAR, /* info == register number, offset in next 2 slots */
    AMD64_UWOP_SAVE_XMM128 = 8, /* info == XMM reg number, offset in next slot */
    AMD64_UWOP_SAVE_XMM128_FAR, /* info == XMM reg number, offset in next 2 slots */
    AMD64_UWOP_PUSH_MACHFRAME   /* info == 0: no error-code, 1: error-code */
} IMAGE_AMD64_UNWIND_CODE_OPS;

typedef union _IMAGE_AMD64_UNWIND_CODE
{
    struct
    {
        UCHAR CodeOffset;
        UCHAR UnwindOp : 4;
        UCHAR OpInfo   : 4;
    };
    USHORT FrameOffset;
} IMAGE_AMD64_UNWIND_CODE, *PIMAGE_AMD64_UNWIND_CODE;

typedef struct _IMAGE_AMD64_UNWIND_INFO
{
    UCHAR Version       : 3;
    UCHAR Flags         : 5;          // UNW_FLAG_*
    UCHAR SizeOfProlog;
    UCHAR CountOfCodes;
    UCHAR FrameRegister : 4;
    UCHAR FrameOffset   : 4;
    USHORT UnwindCode[ANYSIZE_ARRAY]; // IMAGE_AMD64_UNWIND_CODE_OPS
/*  USHORT MoreUnwindCode[((CountOfCodes + 1) & ~1) - 1];
*   union
*   {
*       ULONG ExceptionHandler;
*       ULONG FunctionEntry;
*   };
*   ULONG ExceptionData[]; */
} IMAGE_AMD64_UNWIND_INFO, *PIMAGE_AMD64_UNWIND_INFO;

#define GetAMD64UnwindCodeEntry(info, index) ((info)->UnwindCode[index])
#define GetAMD64LanguageSpecificDataPtr(info) ((PVOID)&GetAMD64UnwindCodeEntry((info),((info)->CountOfCodes + 1) & ~1))
#define GetAMD64ExceptionHandlerRva(info) (*(PULONG)GetAMD64LanguageSpecificDataPtr(info))
#define GetAMD64ChainedFunction(info) ((PIMAGE_AMD64_RUNTIME_FUNCTION_ENTRY)GetAMD64LanguageSpecificDataPtr(info))

typedef struct _PVP_PE_EXCEPTION_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
    ULONG ExceptionsFlags;
} PVP_PE_EXCEPTION_CONTEXT, *PPVP_PE_EXCEPTION_CONTEXT;

USHORT PvGetExceptionsImageMachine(
    _In_ PPVP_PE_EXCEPTION_CONTEXT Context
    )
{
    USHORT imageMachine;

    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        imageMachine = PvMappedImage.NtHeaders32->FileHeader.Machine;
    else
        imageMachine = PvMappedImage.NtHeaders->FileHeader.Machine;

    if (Context->ExceptionsFlags && PH_GET_IMAGE_EXCEPTIONS_ARM64X)
    {
        if (imageMachine == IMAGE_FILE_MACHINE_ARM64)
            imageMachine = IMAGE_FILE_MACHINE_AMD64;
        else if (imageMachine == IMAGE_FILE_MACHINE_ARM64)
            imageMachine = IMAGE_FILE_MACHINE_AMD64;
    }

    return imageMachine;
}

VOID PvEnumerateExceptionEntries(
    _In_ HWND WindowHandle,
    _In_ PPVP_PE_EXCEPTION_CONTEXT Context
    )
{
    static const PH_ACCESS_ENTRY unwFlags[] =
    {
        { L"UNW_FLAG_EHANDLER",  UNW_FLAG_EHANDLER, FALSE, FALSE, L"Exception handler" },
        { L"UNW_FLAG_UHANDLER",  UNW_FLAG_UHANDLER, FALSE, FALSE, L"Termination handler" },
        { L"UNW_FLAG_CHAININFO", UNW_FLAG_CHAININFO, FALSE, FALSE, L"Chained unwind info" },
    };

    ULONG count = 0;
    ULONG imageMachine;
    PH_MAPPED_IMAGE_EXCEPTIONS exceptions;

    imageMachine = PvGetExceptionsImageMachine(Context);

    if (!NT_SUCCESS(PhGetMappedImageExceptionsEx(&PvMappedImage, &exceptions, Context->ExceptionsFlags)))
    {
        return;
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    if (imageMachine == IMAGE_FILE_MACHINE_I386)
    {
        for (ULONG i = 0; i < exceptions.NumberOfEntries; i++)
        {
            ULONG entry = *(PULONG)PTR_ADD_OFFSET(exceptions.ExceptionEntries, i * sizeof(ULONG));
            INT lvItemIndex;
            PPH_STRING symbol;
            PPH_STRING symbolName = NULL;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, ++count);
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, NULL);

            PhPrintPointer(value, UlongToPtr(entry));
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, value);

            symbol = PhGetSymbolFromAddress(
                PvSymbolProvider,
                PTR_ADD_OFFSET(PvMappedImage.NtHeaders32->OptionalHeader.ImageBase, entry),
                NULL,
                NULL,
                &symbolName,
                NULL
                );

            if (symbolName)
            {
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, symbolName->Buffer);
                PhDereferenceObject(symbolName);
            }

            if (symbol) PhDereferenceObject(symbol);

            if (entry)
            {
                PIMAGE_SECTION_HEADER directorySection;

                directorySection = PhMappedImageRvaToSection(
                    &PvMappedImage,
                    entry
                    );

                if (directorySection)
                {
                    WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

                    if (NT_SUCCESS(PhGetMappedImageSectionName(directorySection, sectionName, RTL_NUMBER_OF(sectionName), NULL)))
                    {
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, sectionName);
                    }
                }
            }
        }
    }
    else if (imageMachine == IMAGE_FILE_MACHINE_AMD64)
    {
        for (ULONG i = 0; i < exceptions.NumberOfEntries; i++)
        {
            PIMAGE_AMD64_RUNTIME_FUNCTION_ENTRY entry = PTR_ADD_OFFSET(exceptions.ExceptionDirectory, i * sizeof(IMAGE_AMD64_RUNTIME_FUNCTION_ENTRY));
            INT lvItemIndex;
            PPH_STRING symbol;
            PPH_STRING symbolName = NULL;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, ++count);
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, entry);

            PhPrintPointer(value, UlongToPtr(entry->BeginAddress));
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, value);
            PhPrintPointer(value, UlongToPtr(entry->EndAddress));
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, value);
            PhPrintPointer(value, UlongToPtr(entry->UnwindData)); // UnwindInfoAddress
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, value);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, PhaFormatSize(entry->EndAddress - entry->BeginAddress, ULONG_MAX)->Buffer);

            symbol = PhGetSymbolFromAddress(
                PvSymbolProvider,
                PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, entry->BeginAddress),
                NULL,
                NULL,
                &symbolName,
                NULL
                );

            if (symbolName)
            {
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, symbolName->Buffer);
                PhDereferenceObject(symbolName);
            }

            PhClearReference(&symbol);

            if (entry->BeginAddress)
            {
                PIMAGE_SECTION_HEADER directorySection;

                directorySection = PhMappedImageRvaToSection(
                    &PvMappedImage,
                    entry->BeginAddress
                    );

                if (directorySection)
                {
                    WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

                    if (NT_SUCCESS(PhGetMappedImageSectionName(directorySection, sectionName, RTL_NUMBER_OF(sectionName), NULL)))
                    {
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 6, sectionName);
                    }
                }
            }

            if (entry->UnwindData)
            {
                PIMAGE_AMD64_UNWIND_INFO unwInfo;

                unwInfo = PhMappedImageRvaToVa(&PvMappedImage, entry->UnwindData, NULL);

                if (unwInfo)
                {
                    if (unwInfo->Flags)
                    {
                        PH_FORMAT format[4];
                        PPH_STRING flagsString;

                        flagsString = PhGetAccessString(unwInfo->Flags, (PPH_ACCESS_ENTRY)unwFlags, RTL_NUMBER_OF(unwFlags));

                        PhInitFormatSR(&format[0], flagsString->sr);
                        PhInitFormatS(&format[1], L" (0x");
                        PhInitFormatX(&format[2], unwInfo->Flags);
                        PhInitFormatS(&format[3], L")");

                        PhMoveReference(&flagsString, PhFormat(format, 4, 10));

                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, flagsString->Buffer);

                        PhDereferenceObject(flagsString);
                    }

                    if (unwInfo->Flags & UNW_FLAG_CHAININFO)
                    {
                        PhPrintPointer(value, UlongToPtr(GetAMD64ChainedFunction(unwInfo)->UnwindData));

                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 8, value);
                    }
                    else if (unwInfo->Flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER))
                    {
                        symbol = PhGetSymbolFromAddress(
                            PvSymbolProvider,
                            PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, GetAMD64ExceptionHandlerRva(unwInfo)),
                            NULL,
                            NULL,
                            &symbolName,
                            NULL
                            );

                        if (symbolName)
                        {
                            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 8, symbolName->Buffer);
                            PhDereferenceObject(symbolName);
                        }

                        PhClearReference(&symbol);
                    }
                }
            }
        }
    }
    else if (imageMachine == IMAGE_FILE_MACHINE_ARM64)
    {
        for (ULONG i = 0; i < exceptions.NumberOfEntries; i++)
        {
            PIMAGE_ARM64_RUNTIME_FUNCTION_ENTRY entry = PTR_ADD_OFFSET(exceptions.ExceptionDirectory, i * sizeof(IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY));
            INT lvItemIndex;
            PPH_STRING symbol;
            PPH_STRING symbolName = NULL;
            WCHAR value[PH_INT64_STR_LEN_1];

            PhPrintUInt64(value, ++count);
            lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, value, entry);

            PhPrintPointer(value, UlongToPtr(entry->BeginAddress));
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, value);

            switch (entry->Flag)
            {
            case PdataRefToFullXdata: // 0 (union is UnwindData)
                {
                    IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA* data;

                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, L"Full");

                    PhPrintPointer(value, UlongToPtr(entry->UnwindData));
                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, value);

                    data = (IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY_XDATA*)PhMappedImageRvaToVa(&PvMappedImage, entry->UnwindData, NULL);
                    if (data && data->Version == 0)
                    {
                        ULONG functionLength = data->FunctionLength << 2;

                        PhPrintPointer(value, PTR_ADD_OFFSET(UlongToPtr(entry->BeginAddress), functionLength));
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, value);
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, PhaFormatSize(functionLength, ULONG_MAX)->Buffer);
                    }
                }
                break;
            case PdataPackedUnwindFunction: // 1
                {
                    ULONG functionLength = entry->FunctionLength << 2;

                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, L"Function");
                    PhPrintPointer(value, PTR_ADD_OFFSET(UlongToPtr(entry->BeginAddress), functionLength));
                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, value);
                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, PhaFormatSize(functionLength, ULONG_MAX)->Buffer);
                }
                break;
            case PdataPackedUnwindFragment: // 2
                {
                    ULONG functionLength = entry->FunctionLength << 2;

                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, L"Fragment");
                    PhPrintPointer(value, PTR_ADD_OFFSET(UlongToPtr(entry->BeginAddress), functionLength));
                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, value);
                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, PhaFormatSize(functionLength, ULONG_MAX)->Buffer);
                }
                break;
            case 3: // undocumented
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, L"Reserved"); // ?
                break;
            default:
                break;
            }

            symbol = PhGetSymbolFromAddress(
                PvSymbolProvider,
                PTR_ADD_OFFSET(PvMappedImage.NtHeaders->OptionalHeader.ImageBase, entry->BeginAddress),
                NULL,
                NULL,
                &symbolName,
                NULL
                );

            if (symbolName)
            {
                PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 6, symbolName->Buffer);
                PhDereferenceObject(symbolName);
            }

            if (symbol) PhDereferenceObject(symbol);

            if (entry->BeginAddress)
            {
                PIMAGE_SECTION_HEADER directorySection;

                directorySection = PhMappedImageRvaToSection(
                    &PvMappedImage,
                    entry->BeginAddress
                    );

                if (directorySection)
                {
                    WCHAR sectionName[IMAGE_SIZEOF_SHORT_NAME + 1];

                    if (NT_SUCCESS(PhGetMappedImageSectionName(directorySection, sectionName, RTL_NUMBER_OF(sectionName), NULL)))
                    {
                        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 7, sectionName);
                    }
                }
            }
        }
    }

    //ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT NTAPI PvpPeExceptionSizeCompareFunctionAmd64(
    _In_ PVOID Item1,
    _In_ PVOID Item2,
    _In_ PVOID Context
    )
{
    PIMAGE_AMD64_RUNTIME_FUNCTION_ENTRY entry1 = Item1;
    PIMAGE_AMD64_RUNTIME_FUNCTION_ENTRY entry2 = Item2;

    return uintptrcmp((ULONG_PTR)entry1->EndAddress - entry1->BeginAddress, (ULONG_PTR)entry2->EndAddress - entry2->BeginAddress);
}

INT_PTR CALLBACK PvpPeExceptionDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_EXCEPTION_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_EXCEPTION_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;

            if (context->PropSheetContext->Context)
            {
                PPV_EXCEPTIONS_PAGECONTEXT exceptionsPageContext = context->PropSheetContext->Context;

                context->ExceptionsFlags = PtrToUlong(exceptionsPageContext->Context);

                if (exceptionsPageContext->FreePropPageContext)
                {
                    PhFree(exceptionsPageContext);
                    exceptionsPageContext = NULL;

                    PhFree(context->PropSheetContext);
                    context->PropSheetContext = NULL;

                    PhFree(propSheetPage);
                    propSheetPage = NULL;
                }
            }
        }
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
            ULONG imageMachine;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhSetExtendedListView(context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            imageMachine = PvGetExceptionsImageMachine(context);

            if (imageMachine == IMAGE_FILE_MACHINE_I386)
            {
                PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"SEH Handler");
                PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 200, L"Symbol");
                PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Section");
                PhLoadListViewColumnsFromSetting(L"ImageExceptionsIa32ListViewColumns", context->ListViewHandle);
            }
            else if (imageMachine == IMAGE_FILE_MACHINE_AMD64)
            {
                PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"RVA (start)");
                PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"RVA (end)");
                PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Data");
                PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Size");
                PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 200, L"Symbol");
                PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 100, L"Section");
                PhAddListViewColumn(context->ListViewHandle, 7, 7, 7, LVCFMT_LEFT, 100, L"Flags");
                PhAddListViewColumn(context->ListViewHandle, 8, 8, 8, LVCFMT_LEFT, 200, L"Handler");
                PhLoadListViewColumnsFromSetting(L"ImageExceptionsAmd64ListViewColumns", context->ListViewHandle);

                ExtendedListView_SetCompareFunction(context->ListViewHandle, 4, PvpPeExceptionSizeCompareFunctionAmd64);
            }
            else if (imageMachine == IMAGE_FILE_MACHINE_ARM64)
            {
                PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Type");
                PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"RVA (start)");
                PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"RVA (end)");
                PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Data");
                PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 100, L"Size");
                PhAddListViewColumn(context->ListViewHandle, 6, 6, 6, LVCFMT_LEFT, 200, L"Symbol");
                PhAddListViewColumn(context->ListViewHandle, 7, 7, 7, LVCFMT_LEFT, 100, L"Section");

                PhLoadListViewColumnsFromSetting(L"ImageExceptionsArm64ListViewColumns", context->ListViewHandle);
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvEnumerateExceptionEntries(hwndDlg, context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            ULONG imageMachine;

            if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                imageMachine = PvMappedImage.NtHeaders32->FileHeader.Machine;
            else
                imageMachine = PvMappedImage.NtHeaders->FileHeader.Machine;

            if (imageMachine == IMAGE_FILE_MACHINE_I386)
            {
                PhSaveListViewColumnsToSetting(L"ImageExceptionsIa32ListViewColumns", context->ListViewHandle);
            }
            else if (imageMachine == IMAGE_FILE_MACHINE_AMD64)
            {
                PhSaveListViewColumnsToSetting(L"ImageExceptionsAmd64ListViewColumns", context->ListViewHandle);
            }
            else if (imageMachine == IMAGE_FILE_MACHINE_ARM64)
            {
                PhSaveListViewColumnsToSetting(L"ImageExceptionsArm64ListViewColumns", context->ListViewHandle);
            }

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
