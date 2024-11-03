/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <emenu.h>
#include <mainwnd.h>
#include <memsrch.h>
#include <memprv.h>
#include <procprv.h>
#include <settings.h>
#include <strsrch.h>
#include <colmgr.h>
#include <cpysave.h>

#define WM_PH_MEMORY_STATUS_UPDATE (WM_APP + 301)

#define PH_SEARCH_UPDATE 1
#define PH_SEARCH_COMPLETED 2

typedef struct _MEMORY_STRING_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ProcessHandle;
    ULONG MinimumLength;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN DetectUnicode : 1;
            BOOLEAN Private : 1;
            BOOLEAN Image : 1;
            BOOLEAN Mapped : 1;
            BOOLEAN EnableCloseDialog : 1;
            BOOLEAN ExtendedUnicode : 1;
            BOOLEAN Spare : 2;
        };
    };

    HWND ParentWindowHandle;
    HWND WindowHandle;
    WNDPROC DefaultWindowProc;
    PH_MEMORY_STRING_OPTIONS Options;
    PPH_LIST Results;
} MEMORY_STRING_CONTEXT, *PMEMORY_STRING_CONTEXT;

typedef struct _MEMORY_STRING_SEARCH_CONTEXT
{
    PPH_MEMORY_SEARCH_OPTIONS Options;
    HANDLE ProcessHandle;
    ULONG TypeMask;
    BOOLEAN DetectUnicode;

    MEMORY_BASIC_INFORMATION BasicInfo;
    PVOID NextReadAddress;
    SIZE_T ReadRemaning;
    PBYTE Buffer;
    SIZE_T BufferSize;
} MEMORY_STRING_SEARCH_CONTEXT, *PMEMORY_STRING_SEARCH_CONTEXT;

typedef struct _PH_MEMSTRINGS_SETTINGS
{
    ULONG MinimumLength;

    union
    {
        struct
        {
            ULONG Ansi : 1;
            ULONG Unicode : 1;
            ULONG ExtendedCharSet : 1;
            ULONG Private : 1;
            ULONG Image : 1;
            ULONG Mapped : 1;
            ULONG ZeroPadAddresses : 1;
            ULONG Spare : 25;
        };

        ULONG Flags;
    };
} PH_MEMSTRINGS_SETTINGS, * PPH_MEMSTRINGS_SETTINGS;

typedef struct _PH_MEMSTRINGS_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    HANDLE ProcessHandle;

    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND MessageHandle;
    HWND SearchHandle;
    HWND FilterHandle;
    RECT MinimumSize;
    ULONG_PTR SearchMatchHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_TN_FILTER_SUPPORT FilterSupport;
    PH_CM_MANAGER Cm;
    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    ULONG StringsCount;
    PPH_LIST NodeList;

    ULONG State;
    BOOLEAN StopSearch;
    HANDLE SearchThreadHandle;
    ULONG SearchResultsAddIndex;
    PPH_LIST SearchResults;
    PH_QUEUED_LOCK SearchResultsLock;
    PPH_LIST PrevNodeList;

    PH_MEMSTRINGS_SETTINGS Settings;
} PH_MEMSTRINGS_CONTEXT, *PPH_MEMSTRINGS_CONTEXT;

typedef enum _PH_MEMSTRINGS_TREE_COLUMN_ITEM
{
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_BASE_ADDRESS,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_ADDRESS,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_TYPE,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_LENGTH,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_STRING,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_PROTECTION,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_MEMORY_TYPE,
    PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM
} PH_MEMSTRINGS_TREE_COLUMN_ITEM;

typedef struct _PH_MEMSTRINGS_NODE
{
    PH_TREENEW_NODE Node;

    ULONG Index;
    BOOLEAN Unicode;
    PVOID BaseAddress;
    PVOID Address;
    PPH_STRING String;
    ULONG Protection;
    ULONG MemoryType;

    WCHAR IndexString[PH_INT64_STR_LEN_1];
    WCHAR BaseAddressString[PH_PTR_STR_LEN_1];
    WCHAR AddressString[PH_PTR_STR_LEN_1];
    WCHAR LengthString[PH_INT64_STR_LEN_1];
    WCHAR ProtectionText[17];

    PH_STRINGREF TextCache[PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM];
} PH_MEMSTRINGS_NODE, *PPH_MEMSTRINGS_NODE;

typedef struct _PH_MEMSTRINGS_SEARCH_CONTEXT
{
    PPH_MEMSTRINGS_CONTEXT TreeContext;
    ULONG TypeMask;
    MEMORY_BASIC_INFORMATION BasicInfo;
    PVOID NextReadAddress;
    SIZE_T ReadRemaning;
    PBYTE Buffer;
    SIZE_T BufferSize;
} PH_MEMSTRINGS_SEARCH_CONTEXT, *PPH_MEMSTRINGS_SEARCH_CONTEXT;

INT_PTR CALLBACK PhpMemoryStringDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN PhpShowMemoryStringProgressDialog(
    _In_ PMEMORY_STRING_CONTEXT Context
    );

BOOLEAN PhpShowMemoryStringDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_LIST PrevNodeList
    );

#define WM_PH_MEMSEARCH_SHOWMENU    (WM_USER + 3000)
#define WM_PH_MEMSEARCH_FINISHED    (WM_USER + 3001)

#define PH_MEMSEARCH_STATE_STOPPED   0
#define PH_MEMSEARCH_STATE_SEARCHING 1
#define PH_MEMSEARCH_STATE_FINISHED  2

static PH_STRINGREF EmptyStringsText = PH_STRINGREF_INIT(L"There are no strings to display.");
static PH_STRINGREF LoadingStringsText = PH_STRINGREF_INIT(L"Loading strings...");

PVOID PhMemorySearchHeap = NULL;
LONG PhMemorySearchHeapRefCount = 0;
PH_QUEUED_LOCK PhMemorySearchHeapLock = PH_QUEUED_LOCK_INIT;

PVOID PhAllocateForMemorySearch(
    _In_ SIZE_T Size
    )
{
    PVOID memory;

    PhAcquireQueuedLockExclusive(&PhMemorySearchHeapLock);

    if (!PhMemorySearchHeap)
    {
        assert(PhMemorySearchHeapRefCount == 0);
        PhMemorySearchHeap = RtlCreateHeap(
            HEAP_GROWABLE | HEAP_CLASS_1,
            NULL,
            8192 * 1024, // 8 MB
            2048 * 1024, // 2 MB
            NULL,
            NULL
            );
    }

    if (PhMemorySearchHeap)
    {
        RtlSetHeapInformation(
            PhMemorySearchHeap,
            HeapCompatibilityInformation,
            &(ULONG){ HEAP_COMPATIBILITY_LFH },
            sizeof(ULONG)
            );

        // Don't use HEAP_NO_SERIALIZE - it's very slow on Vista and above.
        memory = RtlAllocateHeap(PhMemorySearchHeap, 0, Size);

        if (memory)
            PhMemorySearchHeapRefCount++;
    }
    else
    {
        memory = NULL;
    }

    PhReleaseQueuedLockExclusive(&PhMemorySearchHeapLock);

    return memory;
}

VOID PhFreeForMemorySearch(
    _In_ _Post_invalid_ PVOID Memory
    )
{
    PhAcquireQueuedLockExclusive(&PhMemorySearchHeapLock);

    RtlFreeHeap(PhMemorySearchHeap, 0, Memory);

    if (--PhMemorySearchHeapRefCount == 0)
    {
        RtlDestroyHeap(PhMemorySearchHeap);
        PhMemorySearchHeap = NULL;
    }

    PhReleaseQueuedLockExclusive(&PhMemorySearchHeapLock);
}

PVOID PhCreateMemoryResult(
    _In_ PVOID Address,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Length
    )
{
    PPH_MEMORY_RESULT result;

    result = PhAllocateForMemorySearch(sizeof(PH_MEMORY_RESULT));

    if (!result)
        return NULL;

    result->RefCount = 1;
    result->Address = Address;
    result->BaseAddress = BaseAddress;
    result->Length = Length;
    result->Display.Length = 0;
    result->Display.Buffer = NULL;

    return result;
}

VOID PhReferenceMemoryResult(
    _In_ PPH_MEMORY_RESULT Result
    )
{
    _InterlockedIncrement(&Result->RefCount);
}

VOID PhDereferenceMemoryResult(
    _In_ PPH_MEMORY_RESULT Result
    )
{
    if (_InterlockedDecrement(&Result->RefCount) == 0)
    {
        if (Result->Display.Buffer)
            PhFreeForMemorySearch(Result->Display.Buffer);

        PhFreeForMemorySearch(Result);
    }
}

VOID PhDereferenceMemoryResults(
    _In_reads_(NumberOfResults) PPH_MEMORY_RESULT *Results,
    _In_ ULONG NumberOfResults
    )
{
    for (ULONG i = 0; i < NumberOfResults; i++)
        PhDereferenceMemoryResult(Results[i]);
}

_Function_class_(PH_STRING_SEARCH_NEXT_BUFFER)
_Must_inspect_result_
NTSTATUS NTAPI PhpMemoryStringSearchNextBuffer(
    _Inout_bytecount_(*Length) PVOID* Buffer,
    _Out_ PSIZE_T Length,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PMEMORY_STRING_SEARCH_CONTEXT context;

    assert(Context);

    context = Context;

    *Buffer = NULL;
    *Length = 0;

    if (context->ReadRemaning)
        goto ReadMemory;

    while (NT_SUCCESS(status = NtQueryVirtualMemory(
        context->ProcessHandle,
        PTR_ADD_OFFSET(context->BasicInfo.BaseAddress, context->BasicInfo.RegionSize),
        MemoryBasicInformation,
        &context->BasicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        SIZE_T length;

        if (context->Options->Cancel)
            break;
        if (context->BasicInfo.State != MEM_COMMIT)
            continue;
        if (FlagOn(context->BasicInfo.Protect, PAGE_NOACCESS | PAGE_GUARD))
            continue;
        if (!FlagOn(context->BasicInfo.Type, context->TypeMask))
            continue;

        context->NextReadAddress = context->BasicInfo.BaseAddress;
        context->ReadRemaning = context->BasicInfo.RegionSize;

        // Don't allocate a huge buffer (16 MiB max)
        length = min(context->BasicInfo.RegionSize, 16 * 1024 * 1024);

        if (length > context->BufferSize)
        {
            context->Buffer = PhReAllocate(context->Buffer, length);
            context->BufferSize = length;
        }

        if (context->ReadRemaning)
        {
ReadMemory:
            length = min(context->ReadRemaning, context->BufferSize);

            if (NT_SUCCESS(status = NtReadVirtualMemory(
                context->ProcessHandle,
                context->NextReadAddress,
                context->Buffer,
                length,
                &length
                )))
            {
                *Buffer = context->Buffer;
                *Length = length;
                context->ReadRemaning -= length;
                context->NextReadAddress = PTR_ADD_OFFSET(context->NextReadAddress, length);
                break;
            }
        }

        context->NextReadAddress = NULL;
        context->ReadRemaning = 0;
    }

    return status;
}

_Function_class_(PH_STRING_SEARCH_CALLBACK)
BOOLEAN NTAPI PhpMemoryStringSearchCallback(
    _In_ PPH_STRING_SEARCH_RESULT Result,
    _In_opt_ PVOID Context
    )
{
    PMEMORY_STRING_SEARCH_CONTEXT context = Context;
    PPH_MEMORY_RESULT result;

    assert(context);

    if (!context->DetectUnicode && Result->Unicode)
        return context->Options->Cancel;

    result = PhCreateMemoryResult(Result->Address, context->BasicInfo.BaseAddress, Result->Length);

    result->Display.Buffer = PhAllocateForMemorySearch(Result->String->Length + sizeof(WCHAR));
    memcpy(result->Display.Buffer, Result->String->Buffer, Result->String->Length);
    result->Display.Buffer[Result->String->Length / sizeof(WCHAR)] = UNICODE_NULL;
    result->Display.Length = Result->String->Length;

    context->Options->Callback(result, context->Options->Context);

    return context->Options->Cancel;
}

VOID PhSearchMemoryString(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_MEMORY_STRING_OPTIONS Options
    )
{
    MEMORY_STRING_SEARCH_CONTEXT context;

    memset(&context, 0, sizeof(MEMORY_STRING_SEARCH_CONTEXT));

    context.Options = &Options->Header;
    context.ProcessHandle = ProcessHandle;
    context.TypeMask = Options->MemoryTypeMask;
    context.DetectUnicode = Options->DetectUnicode;

    PhSearchStrings(
        Options->MinimumLength,
        Options->ExtendedUnicode,
        PhpMemoryStringSearchNextBuffer,
        PhpMemoryStringSearchCallback,
        &context
        );
}

VOID PhShowMemoryStringDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    MEMORY_STRING_CONTEXT context;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessItem->ProcessId
        )))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process", status, 0);
        return;
    }

    memset(&context, 0, sizeof(MEMORY_STRING_CONTEXT));
    context.ParentWindowHandle = ParentWindowHandle;
    context.ProcessId = ProcessItem->ProcessId;
    context.ProcessHandle = processHandle;

    if (PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMSTRING),
        ParentWindowHandle,
        PhpMemoryStringDlgProc,
        &context
        ) != IDOK)
    {
        NtClose(processHandle);
        return;
    }

    context.Results = PhCreateList(1024);

    if (PhpShowMemoryStringProgressDialog(&context))
    {
        PPH_SHOW_MEMORY_RESULTS showMemoryResults;

        showMemoryResults = PhAllocate(sizeof(PH_SHOW_MEMORY_RESULTS));
        showMemoryResults->ProcessId = ProcessItem->ProcessId;
        showMemoryResults->Results = context.Results;

        PhReferenceObject(context.Results);
        SystemInformer_ShowMemoryResults(showMemoryResults);
    }

    PhDereferenceObject(context.Results);
    NtClose(processHandle);
}

INT_PTR CALLBACK PhpMemoryStringDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PMEMORY_STRING_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PMEMORY_STRING_CONTEXT)lParam;

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
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetDialogItemText(hwndDlg, IDC_MINIMUMLENGTH, L"10");

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_DETECTUNICODE), BST_CHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PRIVATE), BST_CHECKED);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    ULONG64 minimumLength = 10;

                    PhStringToInteger64(&PhaGetDlgItemText(hwndDlg, IDC_MINIMUMLENGTH)->sr, 0, &minimumLength);

                    if (minimumLength < 4)
                    {
                        PhShowError(hwndDlg, L"%s", L"The minimum length must be at least 4.");
                        break;
                    }

                    context->MinimumLength = (ULONG)minimumLength;
                    context->DetectUnicode = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DETECTUNICODE)) == BST_CHECKED;
                    context->ExtendedUnicode = Button_GetCheck(GetDlgItem(hwndDlg, IDC_EXTENDEDUNICODE)) == BST_CHECKED;
                    context->Private = Button_GetCheck(GetDlgItem(hwndDlg, IDC_PRIVATE)) == BST_CHECKED;
                    context->Image = Button_GetCheck(GetDlgItem(hwndDlg, IDC_IMAGE)) == BST_CHECKED;
                    context->Mapped = Button_GetCheck(GetDlgItem(hwndDlg, IDC_MAPPED)) == BST_CHECKED;

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_DETECTUNICODE:
                {
                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_DETECTUNICODE)) == BST_UNCHECKED)
                    {
                        Button_SetCheck(GetDlgItem(hwndDlg, IDC_EXTENDEDUNICODE), BST_UNCHECKED);
                        Button_Enable(GetDlgItem(hwndDlg, IDC_EXTENDEDUNICODE), FALSE);
                    }
                    else
                    {
                        Button_Enable(GetDlgItem(hwndDlg, IDC_EXTENDEDUNICODE), TRUE);
                    }
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

static VOID NTAPI PhpMemoryStringResultCallback(
    _In_ _Assume_refs_(1) PPH_MEMORY_RESULT Result,
    _In_opt_ PVOID Context
    )
{
    PMEMORY_STRING_CONTEXT context = Context;

    if (context)
        PhAddItemList(context->Results, Result);
}

NTSTATUS PhpMemoryStringThreadStart(
    _In_ PVOID Parameter
    )
{
    PMEMORY_STRING_CONTEXT context = Parameter;

    context->Options.Header.Callback = PhpMemoryStringResultCallback;
    context->Options.Header.Context = context;
    context->Options.MinimumLength = context->MinimumLength;
    context->Options.DetectUnicode = context->DetectUnicode;
    context->Options.ExtendedUnicode = context->ExtendedUnicode;

    if (context->Private)
        context->Options.MemoryTypeMask |= MEM_PRIVATE;
    if (context->Image)
        context->Options.MemoryTypeMask |= MEM_IMAGE;
    if (context->Mapped)
        context->Options.MemoryTypeMask |= MEM_MAPPED;

    PhSearchMemoryString(context->ProcessHandle, &context->Options);

    SendMessage(
        context->WindowHandle,
        WM_PH_MEMORY_STATUS_UPDATE,
        PH_SEARCH_COMPLETED,
        0
        );

    return STATUS_SUCCESS;
}

LRESULT CALLBACK PhpMemoryStringTaskDialogSubclassProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PMEMORY_STRING_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hwndDlg, 0xF)))
        return 0;

    oldWndProc = context->DefaultWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwndDlg, 0xF);
        }
        break;
    case WM_PH_MEMORY_STATUS_UPDATE:
        {
            switch (wParam)
            {
            case PH_SEARCH_COMPLETED:
                {
                    context->EnableCloseDialog = TRUE;
                    SendMessage(hwndDlg, TDM_CLICK_BUTTON, IDOK, 0);
                }
                break;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwndDlg, uMsg, wParam, lParam);
}

HRESULT CALLBACK PhpMemoryStringTaskDialogCallback(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PMEMORY_STRING_CONTEXT context = (PMEMORY_STRING_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            context->WindowHandle = hwndDlg;

            // Create the Taskdialog icons.
            PhSetApplicationWindowIcon(hwndDlg);
            SendMessage(hwndDlg, TDM_UPDATE_ICON, TDIE_ICON_MAIN, (LPARAM)PhGetApplicationIcon(FALSE));

            // Set the progress state.
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            // Subclass the Taskdialog.
            context->DefaultWindowProc = (WNDPROC)GetWindowLongPtr(hwndDlg, GWLP_WNDPROC);
            PhSetWindowContext(hwndDlg, 0xF, context);
            SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)PhpMemoryStringTaskDialogSubclassProc);

            // Create the search thread.
            PhCreateThread2(PhpMemoryStringThreadStart, context);
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDCANCEL)
                context->Options.Header.Cancel = TRUE;

            if (!context->EnableCloseDialog)
                return S_FALSE;
        }
        break;
    case TDN_TIMER:
        {
            PPH_STRING numberText;
            PPH_STRING progressText;

            numberText = PhFormatUInt64(context->Results->Count, TRUE);
            progressText = PhFormatString(L"%s strings found...", numberText->Buffer);

            SendMessage(hwndDlg, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)progressText->Buffer);

            PhDereferenceObject(progressText);
            PhDereferenceObject(numberText);
        }
        break;
    }

    return S_OK;
}

BOOLEAN PhpShowMemoryStringProgressDialog(
    _In_ PMEMORY_STRING_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;
    INT result = 0;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CALLBACK_TIMER;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pfCallback = PhpMemoryStringTaskDialogCallback;
    config.lpCallbackData = (LONG_PTR)Context;
    config.hwndParent = Context->ParentWindowHandle;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Searching memory strings...";
    config.pszContent = L" ";
    config.cxWidth = 200;

    if (SUCCEEDED(TaskDialogIndirect(&config, &result, NULL, NULL)) && result == IDOK)
    {
        return TRUE;
    }

    return FALSE;
}

//
//
//

BOOLEAN PhpShowMemoryStringTreeProgressDialog(
    _In_ PMEMORY_STRING_CONTEXT Context
    );

BOOLEAN PhpShowMemoryStringTreeDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_LIST PrevNodeList
    );

_Function_class_(PH_STRING_SEARCH_CALLBACK)
BOOLEAN NTAPI PhpMemoryStringTreeSearchCallback(
    _In_ PPH_STRING_SEARCH_RESULT Result,
    _In_ PVOID Context
    )
{
    PPH_MEMSTRINGS_SEARCH_CONTEXT context = Context;
    PPH_MEMSTRINGS_CONTEXT treeContext;
    PPH_MEMSTRINGS_NODE node;

    treeContext = context->TreeContext;

    node = PhAllocateZero(sizeof(PH_MEMSTRINGS_NODE));
    node->Index = ++treeContext->StringsCount;
    PhPrintUInt64(node->IndexString, node->Index);
    node->BaseAddress = context->BasicInfo.BaseAddress;
    node->Address = PTR_ADD_OFFSET(node->BaseAddress, PTR_SUB_OFFSET(Result->Address, context->Buffer));

    if (context->TreeContext->Settings.ZeroPadAddresses)
    {
        PhPrintPointerPadZeros(node->BaseAddressString, node->BaseAddress);
        PhPrintPointerPadZeros(node->AddressString, node->Address);
    }
    else
    {
        PhPrintPointer(node->BaseAddressString, node->BaseAddress);
        PhPrintPointer(node->AddressString, node->Address);
    }

    node->Unicode = Result->Unicode;
    node->String = PhReferenceObject(Result->String);
    PhPrintUInt64(node->LengthString, node->String->Length / 2);

    PhAcquireQueuedLockExclusive(&treeContext->SearchResultsLock);
    PhAddItemList(treeContext->SearchResults, node);
    PhReleaseQueuedLockExclusive(&treeContext->SearchResultsLock);

    node->Protection = context->BasicInfo.Protect;
    PhGetMemoryProtectionString(node->Protection, node->ProtectionText);
    node->MemoryType = context->BasicInfo.Type;

    return !!treeContext->StopSearch;
}

NTSTATUS PhpMemorySearchStringsTreeThread(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    PH_MEMSTRINGS_SEARCH_CONTEXT context;

    memset(&context, 0, sizeof(context));

    context.TreeContext = Context;

    context.TypeMask = 0;
    if (Context->Settings.Private)
        SetFlag(context.TypeMask, MEM_PRIVATE);
    if (Context->Settings.Image)
        SetFlag(context.TypeMask, MEM_IMAGE);
    if (Context->Settings.Mapped)
        SetFlag(context.TypeMask, MEM_MAPPED);

    if (context.TypeMask)
    {
        PhSearchStrings(
            Context->Settings.MinimumLength,
            !!Context->Settings.ExtendedCharSet,
            PhpMemoryStringSearchNextBuffer,
            PhpMemoryStringSearchCallback,
            &context
            );
    }

    if (context.Buffer)
        PhFree(context.Buffer);

    PostMessage(Context->WindowHandle, WM_PH_MEMSEARCH_FINISHED, 0, 0);

    return STATUS_SUCCESS;
}

VOID PhpMemoryStringsTreeAddTreeNode(
    _In_ PPH_MEMSTRINGS_CONTEXT Context,
    _In_ PPH_MEMSTRINGS_NODE Entry
    )
{
    PhInitializeTreeNewNode(&Entry->Node);

    memset(Entry->TextCache, 0, sizeof(PH_STRINGREF) * PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM);
    Entry->Node.TextCache = Entry->TextCache;
    Entry->Node.TextCacheSize = PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM;

    PhAddItemList(Context->NodeList, Entry);

    if (Context->FilterSupport.NodeList)
    {
        Entry->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->FilterSupport, &Entry->Node);
    }
}

VOID PhpAddPendingMemoryStringsTreeNodes(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    ULONG i;
    BOOLEAN needsFullUpdate = FALSE;

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAcquireQueuedLockExclusive(&Context->SearchResultsLock);

    for (i = Context->SearchResultsAddIndex; i < Context->SearchResults->Count; i++)
    {
        PhpMemoryStringsTreeAddTreeNode(Context, Context->SearchResults->Items[i]);
        needsFullUpdate = TRUE;
    }
    Context->SearchResultsAddIndex = i;

    PhReleaseQueuedLockExclusive(&Context->SearchResultsLock);

    if (needsFullUpdate)
        TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);
}

VOID PhpDeleteMemoryStringsTreeNodeList(
    _In_ PPH_LIST NodeList
    )
{
    for (ULONG i = 0; i < NodeList->Count; i++)
    {
        PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)NodeList->Items[i];

        PhClearReference(&node->String);
        PhFree(node);
    }
}

VOID PhpDeleteMemoryStringsTreeTree(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    Context->StopSearch = TRUE;
    if (Context->SearchThreadHandle)
    {
        NtWaitForSingleObject(Context->SearchThreadHandle, FALSE, NULL);
        NtClose(Context->SearchThreadHandle);
        Context->SearchThreadHandle = NULL;
    }
    Context->StopSearch = FALSE;

    PhpAddPendingMemoryStringsTreeNodes(Context);

    PhpDeleteMemoryStringsTreeNodeList(Context->NodeList);

    PhClearReference(&Context->NodeList);
    PhClearReference(&Context->SearchResults);
    Context->SearchResultsAddIndex = 0;
    Context->StringsCount = 0;

}

VOID PhpSearchMemoryStringsTree(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhpDeleteMemoryStringsTreeTree(Context);

    Context->NodeList = PhCreateList(100);
    Context->SearchResults = PhCreateList(100);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, Context->TreeNewHandle, Context->NodeList);

    TreeNew_SetEmptyText(Context->TreeNewHandle, &LoadingStringsText, 0);
    TreeNew_NodesStructured(Context->TreeNewHandle);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    Context->State = PH_MEMSEARCH_STATE_SEARCHING;
    EnableWindow(Context->FilterHandle, FALSE);

    PhCreateThreadEx(&Context->SearchThreadHandle, PhpMemorySearchStringsTreeThread, Context);
}

VOID PhpLoadSettingsMemoryStringsTree(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"MemStringsTreeListColumns");
    sortSettings = PhGetStringSetting(L"MemStringsTreeListSort");
    Context->Settings.Flags = PhGetIntegerSetting(L"MemStringsTreeListFlags");
    Context->Settings.MinimumLength = PhGetIntegerSetting(L"MemStringsMinimumLength");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhpSaveSettingsMemoryStringsTree(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(L"MemStringsMinimumLength", Context->Settings.MinimumLength);
    PhSetIntegerSetting(L"MemStringsTreeListFlags", Context->Settings.Flags);
    PhSetStringSetting2(L"MemStringsTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"MemStringsTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhpInvalidateMemoryStringsTreeAddresses(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)Context->NodeList->Items[i];

        if (Context->Settings.ZeroPadAddresses)
        {
            PhPrintPointerPadZeros(node->BaseAddressString, node->BaseAddress);
            PhPrintPointerPadZeros(node->AddressString, node->Address);
        }
        else
        {
            PhPrintPointer(node->BaseAddressString, node->BaseAddress);
            PhPrintPointer(node->AddressString, node->Address);
        }

        memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM);
    }

    InvalidateRect(Context->TreeNewHandle, NULL, FALSE);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN PhpGetSelectedMemoryStringsTreeNodes(
    _In_ PPH_MEMSTRINGS_CONTEXT Context,
    _Out_ PPH_MEMSTRINGS_NODE** Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
            PhAddItemList(list, node);
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    *Nodes = NULL;
    *NumberOfNodes = 0;

    PhDereferenceObject(list);
    return FALSE;
}

VOID PhpCopyFilteredMemoryStringsTreeNodes(
    _In_ PPH_MEMSTRINGS_CONTEXT Context,
    _Out_ PPH_LIST* NodeList
    )
{
    PPH_LIST list = PhCreateList(10);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)Context->NodeList->Items[i];

        if (node->Node.Visible)
        {
            PPH_MEMSTRINGS_NODE cloned;

            cloned = PhAllocateCopy(node, sizeof(PH_MEMSTRINGS_NODE));
            PhReferenceObject(cloned->String);
            PhAddItemList(list, cloned);
        }
    }

    *NodeList = list;
}

BOOLEAN PhpMemoryStringsTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMSTRINGS_CONTEXT context = Context;
    PPH_MEMSTRINGS_NODE node = (PPH_MEMSTRINGS_NODE)Node;

    assert(Context);

    if (!context->Settings.Ansi && !node->Unicode)
        return FALSE;
    if (!context->Settings.Unicode && node->Unicode)
        return FALSE;

    if (!context->SearchMatchHandle)
        return TRUE;

    return PhSearchControlMatch(context->SearchMatchHandle, &node->String->sr);
}

VOID NTAPI PvpStringsTreeSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_MEMSTRINGS_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    PhApplyTreeNewFilters(&context->FilterSupport);
}

#define SORT_FUNCTION(Column) PvpMemoryStringsTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PvpMemoryStringsTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_MEMSTRINGS_NODE node1 = *(PPH_MEMSTRINGS_NODE *)_elem1; \
    PPH_MEMSTRINGS_NODE node2 = *(PPH_MEMSTRINGS_NODE *)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintcmp(node1->Index, node2->Index); \
    \
    return PhModifySort(sortResult, ((PPH_MEMSTRINGS_CONTEXT)_context)->TreeNewSortOrder); \
}

LONG PhpMemoryStringsTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPH_MEMSTRINGS_NODE)Node1)->Index, (ULONG_PTR)((PPH_MEMSTRINGS_NODE)Node2)->Index);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(Index)
{
    NOTHING;
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(BaseAddress)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->BaseAddress, (ULONG_PTR)node2->BaseAddress);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Address)
{
    sortResult = uintptrcmp((ULONG_PTR)node1->Address, (ULONG_PTR)node2->Address);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintcmp(node1->Unicode, node2->Unicode);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Length)
{
    sortResult = uintptrcmp(node1->String->Length, node2->String->Length);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(String)
{
    sortResult = PhCompareString(node1->String, node2->String, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Protection)
{
    sortResult = uintcmp(node1->Protection, node2->Protection);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(MemoryType)
{
    sortResult = uintcmp(node1->MemoryType, node2->MemoryType);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpMemoryStringsTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_MEMSTRINGS_CONTEXT context = Context;
    PPH_MEMSTRINGS_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_MEMSTRINGS_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Index),
                    SORT_FUNCTION(BaseAddress),
                    SORT_FUNCTION(Address),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Length),
                    SORT_FUNCTION(String),
                    SORT_FUNCTION(Protection),
                    SORT_FUNCTION(MemoryType),
                };
                int (__cdecl *sortFunction)(void *, const void *, const void *);

                static_assert(RTL_NUMBER_OF(sortFunctions) == PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM, "SortFunctions must equal maximum.");

                if (context->TreeNewSortColumn < PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;
            node = (PPH_MEMSTRINGS_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPH_MEMSTRINGS_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX:
                PhInitializeStringRefLongHint(&getCellText->Text, node->IndexString);
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_BASE_ADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->BaseAddressString);
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_ADDRESS:
                PhInitializeStringRefLongHint(&getCellText->Text, node->AddressString);
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_PROTECTION:
                PhInitializeStringRefLongHint(&getCellText->Text, node->ProtectionText);
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_MEMORY_TYPE:
                getCellText->Text = *PhGetMemoryTypeString(node->MemoryType);
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_TYPE:
                PhInitializeStringRef(&getCellText->Text, node->Unicode ? L"Unicode" : L"ANSI");
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_LENGTH:
                PhInitializeStringRefLongHint(&getCellText->Text, node->LengthString);
                break;
            case PH_MEMSTRINGS_TREE_COLUMN_ITEM_STRING:
                getCellText->Text = node->String->sr;
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PPH_MEMSTRINGS_NODE)getNodeColor->Node;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                {
                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(hwnd, 0);
                        PhSetClipboardString(hwnd, &text->sr);
                        PhDereferenceObject(text);
                    }
                }
                break;
            }
        }
        return TRUE;
    case TreeNewNodeExpanding:
    case TreeNewLeftDoubleClick:
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            SendMessage(context->WindowHandle, WM_PH_MEMSEARCH_SHOWMENU, 0, (LPARAM)contextMenu);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhpInitializeMemoryStringsTree(
    _In_ PPH_MEMSTRINGS_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ HWND TreeNewHandle
    )
{
    BOOLEAN enableMonospaceFont = !!PhGetIntegerSetting(L"EnableMonospaceFont");

    Context->WindowHandle = WindowHandle;
    Context->TreeNewHandle = TreeNewHandle;

    Context->NodeList = PhCreateList(1);
    Context->SearchResults = PhCreateList(1);

    PhInitializeTreeNewFilterSupport(&Context->FilterSupport, TreeNewHandle, Context->NodeList);

    PhSetControlTheme(TreeNewHandle, L"explorer");

    TreeNew_SetCallback(TreeNewHandle, PhpMemoryStringsTreeNewCallback, Context);
    TreeNew_SetRedraw(TreeNewHandle, FALSE);

    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX, TRUE, L"#", 40, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_BASE_ADDRESS, TRUE, L"Base address", 80, PH_ALIGN_LEFT | (enableMonospaceFont ? PH_ALIGN_MONOSPACE_FONT : 0), PH_MEMSTRINGS_TREE_COLUMN_ITEM_BASE_ADDRESS, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_ADDRESS, TRUE, L"Address", 80, PH_ALIGN_LEFT | (enableMonospaceFont ? PH_ALIGN_MONOSPACE_FONT : 0), PH_MEMSTRINGS_TREE_COLUMN_ITEM_ADDRESS, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_TYPE, TRUE, L"Type", 80, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_TYPE, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_LENGTH, TRUE, L"Length", 80, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_LENGTH, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_STRING, TRUE, L"String", 600, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_STRING, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_PROTECTION, FALSE, L"Protection", 80, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_PROTECTION, 0, 0);
    PhAddTreeNewColumnEx2(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_MEMORY_TYPE, FALSE, L"Memory type", 80, PH_ALIGN_LEFT, PH_MEMSTRINGS_TREE_COLUMN_ITEM_MEMORY_TYPE, 0, 0);

    TreeNew_SetRedraw(TreeNewHandle, TRUE);
    TreeNew_SetSort(TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_INDEX, AscendingSortOrder);

    PhCmInitializeManager(&Context->Cm, TreeNewHandle, PH_MEMSTRINGS_TREE_COLUMN_ITEM_MAXIMUM, PhpMemoryStringsTreeNewPostSortFunction);

    PhpLoadSettingsMemoryStringsTree(Context);
}

INT_PTR CALLBACK PhpMemoryStringsTreeMinimumLengthDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PULONG length;

    if (uMsg == WM_INITDIALOG)
    {
        length = (PULONG)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, length);
    }
    else
    {
        length = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!length)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WCHAR lengthString[PH_INT32_STR_LEN_1];

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhPrintUInt32(lengthString, *length);

            PhSetDialogItemText(hwndDlg, IDC_MINIMUMLENGTH, lengthString);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    ULONG64 minimumLength;

                    PhStringToInteger64(&PhaGetDlgItemText(hwndDlg, IDC_MINIMUMLENGTH)->sr, 0, &minimumLength);

                    if (!minimumLength || minimumLength > MAXULONG32)
                    {
                        PhShowError(hwndDlg, L"%s", L"Invalid minimum length");
                        break;
                    }

                    *length = (ULONG)minimumLength;
                    EndDialog(hwndDlg, IDOK);
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

ULONG PhpMemoryStringsTreeMinimumLengthDialog(
    _In_ HWND WindowHandle,
    _In_ ULONG CurrentMinimumLength
    )
{
    ULONG length;

    length = CurrentMinimumLength;

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMSTRINGSMINLEN),
        WindowHandle,
        PhpMemoryStringsTreeMinimumLengthDlgProc,
        &length
        );

    return length;
}

VOID PhpMemoryStringsTreeSetWindowTitle(
    _In_ PPH_MEMSTRINGS_CONTEXT Context
    )
{
    HICON icon;
    PPH_STRING title;
    CLIENT_ID clientId;

    if (Context->ProcessItem->IconEntry && Context->ProcessItem->IconEntry->SmallIconIndex)
    {
        icon = PhGetImageListIcon(Context->ProcessItem->IconEntry->SmallIconIndex, FALSE);
        if (icon)
            PhSetWindowIcon(Context->WindowHandle, icon, NULL, TRUE);
    }

    clientId.UniqueProcess = Context->ProcessItem->ProcessId;
    clientId.UniqueThread = NULL;
    title = PhaConcatStrings2(PhGetStringOrEmpty(PH_AUTO(PhGetClientIdName(&clientId))), L" Strings");
    SetWindowTextW(Context->WindowHandle, title->Buffer);
}

INT_PTR CALLBACK PhpMemoryStringsTreeDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_MEMSTRINGS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_MEMSTRINGS_CONTEXT)lParam;

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
            context->MessageHandle = GetDlgItem(hwndDlg, IDC_MESSAGE);
            context->SearchHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            context->FilterHandle = GetDlgItem(hwndDlg, IDC_FILTER);

            PhpMemoryStringsTreeSetWindowTitle(context);
            PhRegisterDialog(hwndDlg);

            PhCreateSearchControl(
                hwndDlg,
                context->SearchHandle,
                L"Search Strings (Ctrl+K)",
                PvpStringsTreeSearchControlCallback,
                context
                );

            PhpInitializeMemoryStringsTree(context, hwndDlg, GetDlgItem(hwndDlg, IDC_TREELIST));
            PhAddTreeNewFilter(&context->FilterSupport, PhpMemoryStringsTreeFilterCallback, context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->MessageHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 300;
            context->MinimumSize.bottom = 100;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            if (PhGetIntegerPairSetting(L"MemStringsWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"MemStringsWindowPosition", L"MemStringsWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhSetTimer(hwndDlg, PH_WINDOW_TIMER_DEFAULT, 200, NULL);

            if (context->PrevNodeList)
            {
                PhMoveReference(&context->SearchResults, context->PrevNodeList);
                context->StringsCount = context->PrevNodeList->Count;
                context->State = PH_MEMSEARCH_STATE_FINISHED;
                EnableWindow(context->FilterHandle, FALSE);
            }
            else
            {
                PhpSearchMemoryStringsTree(context);
            }

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhpSaveSettingsMemoryStringsTree(context);
            PhpDeleteMemoryStringsTreeTree(context);

            PhSaveWindowPlacementToSetting(L"MemStringsWindowPosition", L"MemStringsWindowSize", hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDereferenceObject(context->ProcessItem);
            NtClose(context->ProcessHandle);
            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_PH_MEMSEARCH_FINISHED:
        {
            PhpAddPendingMemoryStringsTreeNodes(context);

            context->State = PH_MEMSEARCH_STATE_FINISHED;

            TreeNew_SetEmptyText(context->TreeNewHandle, &EmptyStringsText, 0);

            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_TIMER:
        {
            PPH_STRING message;
            PH_FORMAT format[3];
            ULONG count = 0;

            if (context->State == PH_MEMSEARCH_STATE_STOPPED)
                break;

            PhpAddPendingMemoryStringsTreeNodes(context);

            if (context->State == PH_MEMSEARCH_STATE_SEARCHING)
                PhInitFormatS(&format[count++], L"Searching... ");

            PhInitFormatU(&format[count++], context->StringsCount);
            PhInitFormatS(&format[count++], L" strings");

            message = PhFormat(format, count, 80);

            SetWindowText(context->MessageHandle, message->Buffer);

            PhDereferenceObject(message);

            if (context->State == PH_MEMSEARCH_STATE_FINISHED)
            {
                context->State = PH_MEMSEARCH_STATE_STOPPED;
                EnableWindow(context->FilterHandle, TRUE);
            }
        }
        break;
    case WM_PH_MEMSEARCH_SHOWMENU:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;
            PPH_MEMSTRINGS_NODE* stringsNodes = NULL;
            ULONG numberOfNodes = 0;

            if (!PhpGetSelectedMemoryStringsTreeNodes(context, &stringsNodes, &numberOfNodes))
                break;

            if (numberOfNodes != 0)
            {
                PPH_EMENU_ITEM readWrite;

                menu = PhCreateEMenu();

                readWrite = PhCreateEMenuItem(0, 1, L"Read/Write memory", NULL, NULL);

                PhInsertEMenuItem(menu, readWrite, ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 100, L"Copy", NULL, NULL), ULONG_MAX);
                PhInsertCopyCellEMenuItem(menu, 100, context->TreeNewHandle, contextMenuEvent->Column);

                if (numberOfNodes != 1)
                    readWrite->Flags |= PH_EMENU_DISABLED;

                selectedItem = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    contextMenuEvent->Location.x,
                    contextMenuEvent->Location.y
                    );

                if (selectedItem && selectedItem->Id != ULONG_MAX && !PhHandleCopyCellEMenuItem(selectedItem))
                {
                    if (selectedItem->Id == 1)
                    {
                        NTSTATUS status;
                        MEMORY_BASIC_INFORMATION basicInfo;
                        PPH_SHOW_MEMORY_EDITOR showMemoryEditor;
                        PVOID address;
                        SIZE_T length;

                        assert(numberOfNodes == 1);

                        address = stringsNodes[0]->Address;
                        if (stringsNodes[0]->Unicode)
                            length = stringsNodes[0]->String->Length;
                        else
                            length = stringsNodes[0]->String->Length / 2;

                        if (NT_SUCCESS(status = NtQueryVirtualMemory(
                            context->ProcessHandle,
                            address,
                            MemoryBasicInformation,
                            &basicInfo,
                            sizeof(basicInfo),
                            NULL
                            )))
                        {
                            showMemoryEditor = PhAllocateZero(sizeof(PH_SHOW_MEMORY_EDITOR));
                            showMemoryEditor->ProcessId = context->ProcessItem->ProcessId;
                            showMemoryEditor->BaseAddress = basicInfo.BaseAddress;
                            showMemoryEditor->RegionSize = basicInfo.RegionSize;
                            showMemoryEditor->SelectOffset = (ULONG)((ULONG_PTR)address - (ULONG_PTR)basicInfo.BaseAddress);
                            showMemoryEditor->SelectLength = (ULONG)length;

                            SystemInformer_ShowMemoryEditor(showMemoryEditor);
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, L"Unable to edit memory", status, 0);
                        }
                    }
                    else if (selectedItem->Id == 100)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(context->TreeNewHandle, 0);
                        PhSetClipboardString(context->TreeNewHandle, &text->sr);
                        PhDereferenceObject(text);
                    }
                }

                PhDestroyEMenu(menu);
                PhFree(stringsNodes);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                break;
            case IDC_SETTINGS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM selectedItem;
                    PPH_EMENU_ITEM ansi;
                    PPH_EMENU_ITEM unicode;
                    PPH_EMENU_ITEM extendedUnicode;
                    PPH_EMENU_ITEM private;
                    PPH_EMENU_ITEM image;
                    PPH_EMENU_ITEM mapped;
                    PPH_EMENU_ITEM minimumLength;
                    PPH_EMENU_ITEM zeroPad;
                    PPH_EMENU_ITEM refresh;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_SETTINGS), &rect);

                    ansi = PhCreateEMenuItem(0, 1, L"ANSI", NULL, NULL);
                    unicode = PhCreateEMenuItem(0, 2, L"Unicode", NULL, NULL);
                    extendedUnicode = PhCreateEMenuItem(0, 3, L"Extended character set", NULL, NULL);
                    private = PhCreateEMenuItem(0, 4, L"Private", NULL, NULL);
                    image = PhCreateEMenuItem(0, 5, L"Image", NULL, NULL);
                    mapped = PhCreateEMenuItem(0, 6, L"Mapped", NULL, NULL);
                    minimumLength = PhCreateEMenuItem(0, 7, L"Minimum length...", NULL, NULL);
                    zeroPad = PhCreateEMenuItem(0, 8, L"Zero pad addresses", NULL, NULL);
                    refresh = PhCreateEMenuItem(0, 9, L"Refresh", NULL, NULL);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, ansi, ULONG_MAX);
                    PhInsertEMenuItem(menu, unicode, ULONG_MAX);
                    PhInsertEMenuItem(menu, extendedUnicode, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, private, ULONG_MAX);
                    PhInsertEMenuItem(menu, image, ULONG_MAX);
                    PhInsertEMenuItem(menu, mapped, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, minimumLength, ULONG_MAX);
                    PhInsertEMenuItem(menu, zeroPad, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, refresh, ULONG_MAX);

                    if (context->Settings.Ansi)
                        ansi->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Unicode)
                        unicode->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.ExtendedCharSet)
                        extendedUnicode->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Private)
                        private->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Image)
                        image->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.Mapped)
                        mapped->Flags |= PH_EMENU_CHECKED;
                    if (context->Settings.ZeroPadAddresses)
                        zeroPad->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id != ULONG_MAX)
                    {
                        if (selectedItem == ansi)
                        {
                            context->Settings.Ansi = !context->Settings.Ansi;
                            PhpSaveSettingsMemoryStringsTree(context);
                            PhApplyTreeNewFilters(&context->FilterSupport);
                        }
                        else if (selectedItem == unicode)
                        {
                            context->Settings.Unicode = !context->Settings.Unicode;
                            PhpSaveSettingsMemoryStringsTree(context);
                            PhApplyTreeNewFilters(&context->FilterSupport);
                        }
                        else if (selectedItem == extendedUnicode)
                        {
                            context->Settings.ExtendedCharSet = !context->Settings.ExtendedCharSet;
                            PhpSaveSettingsMemoryStringsTree(context);
                            PhpSearchMemoryStringsTree(context);
                        }
                        else if (selectedItem == private)
                        {
                            context->Settings.Private = !context->Settings.Private;
                            PhpSaveSettingsMemoryStringsTree(context);
                            PhpSearchMemoryStringsTree(context);
                        }
                        else if (selectedItem == image)
                        {
                            context->Settings.Image = !context->Settings.Image;
                            PhpSaveSettingsMemoryStringsTree(context);
                            PhpSearchMemoryStringsTree(context);
                        }
                        else if (selectedItem == mapped)
                        {
                            context->Settings.Mapped = !context->Settings.Mapped;
                            PhpSaveSettingsMemoryStringsTree(context);
                            PhpSearchMemoryStringsTree(context);
                        }
                        else if (selectedItem == minimumLength)
                        {
                            ULONG length = PhpMemoryStringsTreeMinimumLengthDialog(hwndDlg, context->Settings.MinimumLength);
                            if (length != context->Settings.MinimumLength)
                            {
                                context->Settings.MinimumLength = length;
                                PhpSaveSettingsMemoryStringsTree(context);
                                PhpSearchMemoryStringsTree(context);
                            }
                        }
                        else if (selectedItem == zeroPad)
                        {
                            context->Settings.ZeroPadAddresses = !context->Settings.ZeroPadAddresses;
                            PhpSaveSettingsMemoryStringsTree(context);
                            PhpInvalidateMemoryStringsTreeAddresses(context);
                        }
                        else if (selectedItem == refresh)
                        {
                            PhpSearchMemoryStringsTree(context);
                        }
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case IDC_FILTER:
                {
                    PPH_LIST nodeList;

                    PhpCopyFilteredMemoryStringsTreeNodes(context, &nodeList);

                    if (!PhpShowMemoryStringTreeDialog(hwndDlg, context->ProcessItem, nodeList))
                    {
                        PhpDeleteMemoryStringsTreeNodeList(nodeList);
                        PhDereferenceObject(nodeList);
                    }
                }
                break;
            }
        }
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

NTSTATUS NTAPI PhpShowMemoryStringTreeDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    HWND windowHandle;
    BOOL result;
    MSG message;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMSTRINGS),
        NULL,
        PhpMemoryStringsTreeDlgProc,
        Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

BOOLEAN PhpShowMemoryStringTreeDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_opt_ PPH_LIST PrevNodeList
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PPH_MEMSTRINGS_CONTEXT context;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessItem->ProcessId
        )))
    {
        PhShowStatus(ParentWindowHandle, L"Unable to open the process", status, 0);
        return FALSE;
    }

    context = PhAllocateZero(sizeof(PH_MEMSTRINGS_CONTEXT));
    context->ProcessItem = PhReferenceObject(ProcessItem);
    context->ProcessHandle = processHandle;
    context->PrevNodeList = PrevNodeList;

    if (!NT_SUCCESS(PhCreateThread2(PhpShowMemoryStringTreeDialogThreadStart, context)))
    {
        PhShowError(ParentWindowHandle, L"%s", L"Unable to create the window.");
        PhDereferenceObject(context->ProcessItem);
        NtClose(context->ProcessHandle);
        PhFree(context);
        return FALSE;
    }

    return TRUE;
}

VOID PhShowMemoryStringTreeDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PhpShowMemoryStringTreeDialog(ParentWindowHandle, ProcessItem, NULL);
}
