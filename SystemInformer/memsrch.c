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
    PVOID CurrentReadAddress;
    PVOID NextReadAddress;
    SIZE_T ReadRemaning;
    PBYTE Buffer;
    SIZE_T BufferSize;
} MEMORY_STRING_SEARCH_CONTEXT, *PMEMORY_STRING_SEARCH_CONTEXT;

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
        const ULONG defaultHeapCompatibilityMode = HEAP_COMPATIBILITY_MODE_LFH;
        RtlSetHeapInformation(
            PhMemorySearchHeap,
            HeapCompatibilityInformation,
            &defaultHeapCompatibilityMode,
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
            context->CurrentReadAddress = context->NextReadAddress;
            length = min(context->ReadRemaning, context->BufferSize);

            if (NT_SUCCESS(status = NtReadVirtualMemory(
                context->ProcessHandle,
                context->CurrentReadAddress,
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

    result = PhCreateMemoryResult(PTR_ADD_OFFSET(context->CurrentReadAddress, PTR_SUB_OFFSET(Result->Address, context->Buffer)), context->BasicInfo.BaseAddress, Result->Length);

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
                        PhShowError2(hwndDlg, L"Unable to search for strings.", L"%s", L"The minimum length must be at least 4.");
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

_Function_class_(USER_THREAD_START_ROUTINE)
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
            PhSetWindowProcedure(hwndDlg, oldWndProc);
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
            context->DefaultWindowProc = PhGetWindowProcedure(hwndDlg);
            PhSetWindowContext(hwndDlg, 0xF, context);
            PhSetWindowProcedure(hwndDlg, PhpMemoryStringTaskDialogSubclassProc);

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
