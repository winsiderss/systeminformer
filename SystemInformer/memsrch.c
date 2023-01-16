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
#include <mainwnd.h>
#include <memsrch.h>
#include <procprv.h>
#include <phsettings.h>

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

INT_PTR CALLBACK PhpMemoryStringDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

BOOLEAN PhpShowMemoryStringProgressDialog(
    _In_ PMEMORY_STRING_CONTEXT Context
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

VOID PhSearchMemoryString(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_MEMORY_STRING_OPTIONS Options
    )
{
    ULONG minimumLength;
    BOOLEAN detectUnicode;
    BOOLEAN extendedUnicode;
    ULONG memoryTypeMask;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;
    PUCHAR buffer;
    SIZE_T bufferSize;
    PWSTR displayBuffer;
    SIZE_T displayBufferCount;

    minimumLength = Options->MinimumLength;
    memoryTypeMask = Options->MemoryTypeMask;
    detectUnicode = Options->DetectUnicode;
    extendedUnicode = Options->ExtendedUnicode;

    if (minimumLength < 4)
        return;

    baseAddress = (PVOID)0;

    bufferSize = PAGE_SIZE * 64;
    buffer = PhAllocatePage(bufferSize, NULL);

    if (!buffer)
        return;

    displayBufferCount = PH_DISPLAY_BUFFER_COUNT;
    displayBuffer = PhAllocatePage((displayBufferCount + 1) * sizeof(WCHAR), NULL);

    if (!displayBuffer)
    {
        PhFreePage(buffer);
        return;
    }

    while (NT_SUCCESS(NtQueryVirtualMemory(
        ProcessHandle,
        baseAddress,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        ULONG_PTR offset;
        SIZE_T readSize;

        if (Options->Header.Cancel)
            break;
        if (basicInfo.State != MEM_COMMIT)
            goto ContinueLoop;
        if ((basicInfo.Type & memoryTypeMask) == 0)
            goto ContinueLoop;
        if (basicInfo.Protect == PAGE_NOACCESS)
            goto ContinueLoop;
        if (basicInfo.Protect & PAGE_GUARD)
            goto ContinueLoop;

        readSize = basicInfo.RegionSize;

        if (basicInfo.RegionSize > bufferSize)
        {
            // Don't allocate a huge buffer though.
            if (basicInfo.RegionSize <= 16 * 1024 * 1024) // 16 MB
            {
                PhFreePage(buffer);
                bufferSize = basicInfo.RegionSize;
                buffer = PhAllocatePage(bufferSize, NULL);

                if (!buffer)
                    break;
            }
            else
            {
                readSize = bufferSize;
            }
        }

        for (offset = 0; offset < basicInfo.RegionSize; offset += readSize)
        {
            ULONG_PTR i;
            UCHAR byte; // current byte
            UCHAR byte1; // previous byte
            UCHAR byte2; // byte before previous byte
            BOOLEAN printable;
            BOOLEAN printable1;
            BOOLEAN printable2;
            ULONG length;

            if (!NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(baseAddress, offset),
                buffer,
                readSize,
                NULL
                )))
                continue;

            byte1 = 0;
            byte2 = 0;
            printable1 = FALSE;
            printable2 = FALSE;
            length = 0;

            for (i = 0; i < readSize; i++)
            {
                byte = buffer[i];

                // dmex: We don't want to enable extra bits in the PhCharIsPrintable array by default
                // or we'll get higher amounts of false positive search results. If the user selects the
                // ExtendedUnicode option then we'll use iswprint (GetStringTypeW) which does check
                // every available character by default.
                if (detectUnicode && extendedUnicode && !iswascii(byte))
                    printable = !!iswprint(byte);
                else
                    printable = PhCharIsPrintable[byte];

                // To find strings Process Hacker uses a state table.
                // * byte2 - byte before previous byte
                // * byte1 - previous byte
                // * byte - current byte
                // * length - length of current string run
                //
                // The states are described below.
                //
                //    [byte2] [byte1] [byte] ...
                //    [char] means printable, [oth] means non-printable.
                //
                // 1. [char] [char] [char] ...
                //      (we're in a non-wide sequence)
                //      -> append char.
                // 2. [char] [char] [oth] ...
                //      (we reached the end of a non-wide sequence, or we need to start a wide sequence)
                //      -> if current string is big enough, create result (non-wide).
                //         otherwise if byte = null, reset to new string with byte1 as first character.
                //         otherwise if byte != null, reset to new string.
                // 3. [char] [oth] [char] ...
                //      (we're in a wide sequence)
                //      -> (byte1 should = null) append char.
                // 4. [char] [oth] [oth] ...
                //      (we reached the end of a wide sequence)
                //      -> (byte1 should = null) if the current string is big enough, create result (wide).
                //         otherwise, reset to new string.
                // 5. [oth] [char] [char] ...
                //      (we reached the end of a wide sequence, or we need to start a non-wide sequence)
                //      -> (excluding byte1) if the current string is big enough, create result (wide).
                //         otherwise, reset to new string with byte1 as first character and byte as
                //         second character.
                // 6. [oth] [char] [oth] ...
                //      (we're in a wide sequence)
                //      -> (byte2 and byte should = null) do nothing.
                // 7. [oth] [oth] [char] ...
                //      (we're starting a sequence, but we don't know if it's a wide or non-wide sequence)
                //      -> append char.
                // 8. [oth] [oth] [oth] ...
                //      (nothing)
                //      -> do nothing.

                if (printable2 && printable1 && printable)
                {
                    if (length < displayBufferCount)
                        displayBuffer[length] = byte;

                    length++;
                }
                else if (printable2 && printable1 && !printable)
                {
                    if (length >= minimumLength)
                    {
                        goto CreateResult;
                    }
                    else if (byte == 0)
                    {
                        length = 1;
                        displayBuffer[0] = byte1;
                    }
                    else
                    {
                        length = 0;
                    }
                }
                else if (printable2 && !printable1 && printable)
                {
                    if (byte1 == 0)
                    {
                        if (length < displayBufferCount)
                            displayBuffer[length] = byte;

                        length++;
                    }
                }
                else if (printable2 && !printable1 && !printable)
                {
                    if (length >= minimumLength)
                    {
                        goto CreateResult;
                    }
                    else
                    {
                        length = 0;
                    }
                }
                else if (!printable2 && printable1 && printable)
                {
                    if (length >= minimumLength + 1) // length - 1 >= minimumLength but avoiding underflow
                    {
                        length--; // exclude byte1
                        goto CreateResult;
                    }
                    else
                    {
                        length = 2;
                        displayBuffer[0] = byte1;
                        displayBuffer[1] = byte;
                    }
                }
                else if (!printable2 && printable1 && !printable)
                {
                    // Nothing
                }
                else if (!printable2 && !printable1 && printable)
                {
                    if (length < displayBufferCount)
                        displayBuffer[length] = byte;

                    length++;
                }
                else if (!printable2 && !printable1 && !printable)
                {
                    // Nothing
                }

                goto AfterCreateResult;

CreateResult:
                {
                    PPH_MEMORY_RESULT result;
                    ULONG lengthInBytes;
                    ULONG bias;
                    BOOLEAN isWide;
                    ULONG displayLength;

                    lengthInBytes = length;
                    bias = 0;
                    isWide = FALSE;

                    if (printable1 == printable) // determine if string was wide (refer to state table, 4 and 5)
                    {
                        isWide = TRUE;
                        lengthInBytes *= 2;
                    }

                    if (printable) // byte1 excluded (refer to state table, 5)
                    {
                        bias = 1;
                    }

                    if (!(isWide && !detectUnicode) && (result = PhCreateMemoryResult(
                        PTR_ADD_OFFSET(baseAddress, i - bias - lengthInBytes),
                        baseAddress,
                        lengthInBytes
                        )))
                    {
                        displayLength = (ULONG)(min(length, displayBufferCount) * sizeof(WCHAR));

                        if (result->Display.Buffer = PhAllocateForMemorySearch(displayLength + sizeof(WCHAR)))
                        {
                            memcpy(result->Display.Buffer, displayBuffer, displayLength);
                            result->Display.Buffer[displayLength / sizeof(WCHAR)] = UNICODE_NULL;
                            result->Display.Length = displayLength;
                        }

                        Options->Header.Callback(
                            result,
                            Options->Header.Context
                            );
                    }

                    length = 0;
                }
AfterCreateResult:

                byte2 = byte1;
                byte1 = byte;
                printable2 = printable1;
                printable1 = printable;
            }
        }

ContinueLoop:
        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
    }

    if (displayBuffer)
        PhFreePage(displayBuffer);

    if (buffer)
        PhFreePage(buffer);
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
        ProcessHacker_ShowMemoryResults(showMemoryResults);
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
