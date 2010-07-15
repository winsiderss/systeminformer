/*
 * Process Hacker - 
 *   memory searchers
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phapp.h>
#include <memsrch.h>
#include <windowsx.h>

#define WM_PH_MEMORY_STATUS_UPDATE (WM_APP + 301)

#define PH_SEARCH_UPDATE 1
#define PH_SEARCH_COMPLETED 2

typedef struct _MEMORY_STRING_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ProcessHandle;
    ULONG MinimumLength;
    BOOLEAN DetectUnicode;
    BOOLEAN Private;
    BOOLEAN Image;
    BOOLEAN Mapped;

    HWND WindowHandle;
    HANDLE ThreadHandle;
    PH_MEMORY_STRING_OPTIONS Options;
    PPH_LIST Results;
} MEMORY_STRING_CONTEXT, *PMEMORY_STRING_CONTEXT;

INT_PTR CALLBACK PhpMemoryStringDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpMemoryStringProgressDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

PVOID PhMemorySearchHeap = NULL;
LONG PhMemorySearchHeapRefCount = 0;
PH_QUEUED_LOCK PhMemorySearchHeapLock;

PVOID PhAllocateForMemorySearch(
    __in SIZE_T Size
    )
{
    PVOID memory;

    PhAcquireQueuedLockExclusive(&PhMemorySearchHeapLock);

    if (!PhMemorySearchHeap)
    {
        assert(PhMemorySearchHeapRefCount == 0);
        PhMemorySearchHeap = RtlCreateHeap(
            HEAP_GROWABLE | HEAP_CLASS_1 | HEAP_NO_SERIALIZE,
            NULL,
            8192 * 1024, // 8 MB
            2048 * 1024, // 2 MB
            NULL,
            NULL
            );
    }

    if (PhMemorySearchHeap)
    {
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
    __in __post_invalid PVOID Memory
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
    __in PVOID Address,
    __in SIZE_T Length
    )
{
    PPH_MEMORY_RESULT result;

    result = PhAllocateForMemorySearch(sizeof(PH_MEMORY_RESULT));

    if (!result)
        return NULL;

    result->RefCount = 1;
    result->Address = Address;
    result->Length = Length;
    result->Display.Length = 0;
    result->Display.Buffer = NULL;

    return result;
}

VOID PhReferenceMemoryResult(
    __in PPH_MEMORY_RESULT Result
    )
{
    _InterlockedIncrement(&Result->RefCount);
}

VOID PhDereferenceMemoryResult(
    __in PPH_MEMORY_RESULT Result
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
    __in_ecount(NumberOfResults) PPH_MEMORY_RESULT *Results,
    __in ULONG NumberOfResults
    )
{
    ULONG i;

    for (i = 0; i < NumberOfResults; i++)
        PhDereferenceMemoryResult(Results[i]);
}

VOID PhSearchMemoryString(
    __in HANDLE ProcessHandle,
    __in PPH_MEMORY_STRING_OPTIONS Options
    )
{
    ULONG minimumLength;
    BOOLEAN detectUnicode;
    ULONG memoryTypeMask;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;
    PUCHAR buffer;
    SIZE_T bufferSize;
    PWSTR displayBuffer;
    SIZE_T displayBufferCount;

    minimumLength = Options->MinimumLength;
    detectUnicode = Options->DetectUnicode;
    memoryTypeMask = Options->MemoryTypeMask;

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
            UCHAR byte1; // previous byte
            UCHAR byte2; // byte before previous byte
            BOOLEAN printable1;
            BOOLEAN printable2;
            ULONG length;

            if (!NT_SUCCESS(PhReadVirtualMemory(
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
                UCHAR byte;
                BOOLEAN printable;

                byte = buffer[i];
                printable = PhCharIsPrintable[byte];

                // To find strings Process Hacker uses a small state machine.
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
                    if (length < displayBufferCount)
                        displayBuffer[length] = byte;

                    length++;
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
                    ULONG displayLength;

                    lengthInBytes = length;
                    bias = 0;

                    if (printable1 == printable) // determine if string was wide (refer to state table, 4 and 5)
                        lengthInBytes *= 2;
                    if (printable) // byte1 excluded (refer to state table, 5)
                        bias = 1;

                    if (result = PhCreateMemoryResult(
                        PTR_ADD_OFFSET(baseAddress, i - bias - lengthInBytes),
                        lengthInBytes
                        ))
                    {
                        displayLength = (ULONG)(min(length, displayBufferCount) * sizeof(WCHAR));

                        if (result->Display.Buffer = PhAllocateForMemorySearch(displayLength + sizeof(WCHAR)))
                        {
                            memcpy(result->Display.Buffer, displayBuffer, displayLength);
                            result->Display.Buffer[displayLength / sizeof(WCHAR)] = 0;
                            result->Display.Length = (USHORT)displayLength;
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

    if (buffer)
        PhFreePage(buffer);
}

VOID PhShowMemoryStringDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    MEMORY_STRING_CONTEXT context;
    PPH_SHOWMEMORYRESULTS showMemoryResults;

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
    context.ProcessId = ProcessItem->ProcessId;
    context.ProcessHandle = processHandle;

    if (DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_MEMSTRING),
        ParentWindowHandle,
        PhpMemoryStringDlgProc,
        (LPARAM)&context
        ) != IDOK)
    {
        NtClose(processHandle);
        return;
    }

    context.Results = PhCreateList(1024);

    if (DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PROGRESS),
        ParentWindowHandle,
        PhpMemoryStringProgressDlgProc,
        (LPARAM)&context
        ) == IDOK)
    {
        showMemoryResults = PhAllocate(sizeof(PH_SHOWMEMORYRESULTS));
        showMemoryResults->ProcessId = ProcessItem->ProcessId;
        showMemoryResults->Results = context.Results;

        PhReferenceObject(context.Results);
        ProcessHacker_ShowMemoryResults(
            PhMainWndHandle,
            showMemoryResults
            );
    }

    PhDereferenceObject(context.Results);
    NtClose(processHandle);
}

INT_PTR CALLBACK PhpMemoryStringDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SetProp(hwndDlg, L"Context", (HANDLE)lParam);
            SetDlgItemText(hwndDlg, IDC_MINIMUMLENGTH, L"10");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_DETECTUNICODE), BST_CHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_PRIVATE), BST_CHECKED);
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"Context");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PMEMORY_STRING_CONTEXT context = (PMEMORY_STRING_CONTEXT)GetProp(hwndDlg, L"Context");
                    ULONG64 minimumLength = 10;

                    PhStringToInteger64(&PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_MINIMUMLENGTH)->sr, 0, &minimumLength);
                    context->MinimumLength = (ULONG)minimumLength;
                    context->DetectUnicode = Button_GetCheck(GetDlgItem(hwndDlg, IDC_DETECTUNICODE)) == BST_CHECKED;
                    context->Private = Button_GetCheck(GetDlgItem(hwndDlg, IDC_PRIVATE)) == BST_CHECKED;
                    context->Image = Button_GetCheck(GetDlgItem(hwndDlg, IDC_IMAGE)) == BST_CHECKED;
                    context->Mapped = Button_GetCheck(GetDlgItem(hwndDlg, IDC_MAPPED)) == BST_CHECKED;

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static BOOL NTAPI PhpMemoryStringResultCallback(
    __in __assumeRefs(1) PPH_MEMORY_RESULT Result,
    __in PVOID Context
    )
{
    PMEMORY_STRING_CONTEXT context = Context;

    PhAddListItem(context->Results, Result);

    return TRUE;
}

NTSTATUS PhpMemoryStringThreadStart(
    __in PVOID Parameter
    )
{
    PMEMORY_STRING_CONTEXT context = Parameter;

    context->Options.Header.Callback = PhpMemoryStringResultCallback;
    context->Options.Header.Context = context;
    context->Options.MinimumLength = context->MinimumLength;
    context->Options.DetectUnicode = context->DetectUnicode;

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

INT_PTR CALLBACK PhpMemoryStringProgressDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PMEMORY_STRING_CONTEXT context = (PMEMORY_STRING_CONTEXT)lParam;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            SetProp(hwndDlg, L"Context", (HANDLE)context);

            SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT, L"Searching...");

            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);

            context->WindowHandle = hwndDlg;
            context->ThreadHandle = PhCreateThread(0, PhpMemoryStringThreadStart, context);

            if (!context->ThreadHandle)
            {
                PhShowStatus(hwndDlg, L"Unable to create the search thread", 0, GetLastError());
                EndDialog(hwndDlg, IDCANCEL);
                return FALSE;
            }

            SetTimer(hwndDlg, 1, 500, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PMEMORY_STRING_CONTEXT context;

            context = (PMEMORY_STRING_CONTEXT)GetProp(hwndDlg, L"Context");

            if (context->ThreadHandle)
                NtClose(context->ThreadHandle);

            RemoveProp(hwndDlg, L"Context");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    PMEMORY_STRING_CONTEXT context =
                        (PMEMORY_STRING_CONTEXT)GetProp(hwndDlg, L"Context");

                    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
                    context->Options.Header.Cancel = TRUE;
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            if (wParam == 1)
            {
                PMEMORY_STRING_CONTEXT context =
                    (PMEMORY_STRING_CONTEXT)GetProp(hwndDlg, L"Context");
                PPH_STRING progressText;

                progressText = PhFormatString(L"%u strings found...", context->Results->Count);
                SetDlgItemText(hwndDlg, IDC_PROGRESSTEXT, progressText->Buffer);
                PhDereferenceObject(progressText);
                InvalidateRect(GetDlgItem(hwndDlg, IDC_PROGRESSTEXT), NULL, FALSE);
            }
        }
        break;
    case WM_PH_MEMORY_STATUS_UPDATE:
        {
            PMEMORY_STRING_CONTEXT context;

            context = (PMEMORY_STRING_CONTEXT)GetProp(hwndDlg, L"Context");

            switch (wParam)
            {
            case PH_SEARCH_COMPLETED:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}
