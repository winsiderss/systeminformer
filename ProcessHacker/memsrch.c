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

    displayBufferCount = PAGE_SIZE * 2 - 1;
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
            BOOLEAN isWide;
            UCHAR byte1; // previous byte
            UCHAR byte2; // byte before previous byte
            ULONG length;

            if (!NT_SUCCESS(PhReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(baseAddress, offset),
                buffer,
                readSize,
                NULL
                )))
                continue;

            isWide = FALSE;
            byte1 = 0;
            byte2 = 0;
            length = 0;

            for (i = 0; i < readSize; i++)
            {
                UCHAR byte;
                BOOLEAN printable;

                byte = buffer[i];
                printable = PhCharIsPrintable[byte];

                if (detectUnicode && isWide && printable && byte1 != 0)
                {
                    // Two printable characters in a row. This can't be 
                    // a wide ASCII string anymore, but we thought it was 
                    // going to be a wide string.

                    isWide = FALSE;

                    if (length >= 2)
                    {
                        // ... [char] [null] [char] *[char]* ...
                        //                          ^ we are here
                        // Reset.
                        length = 1;
                        displayBuffer[0] = byte1;
                    }

                    if (length < displayBufferCount)
                        displayBuffer[length] = byte;

                    length++;
                }
                else if (printable)
                {
                    if (length < displayBufferCount)
                        displayBuffer[length] = byte;

                    length++;
                }
                else if (detectUnicode && byte == 0 && PhCharIsPrintable[byte1] && !PhCharIsPrintable[byte2])
                {
                    // We got a non printable byte, a printable byte, and now a null. 
                    // This is probably going to be a wide string (or we're already in one).
                    isWide = TRUE;
                }
                else if (isWide && byte == 0 && PhCharIsPrintable[byte1] && PhCharIsPrintable[byte2] &&
                    length < minimumLength)
                {
                    // ... [char] [char] *[null]* ([char] [null] [char] [null]) ...
                    //                   ^ we are here
                    // We got a sequence of printable characters but now we have a null. 
                    // That sequence was too short to be considered a string, so now we'll 
                    // switch to wide string mode.

                    isWide = TRUE;

                    // Reset.
                    length = 1;
                    displayBuffer[0] = byte1;
                }
                else
                {
                    if (length >= minimumLength)
                    {
                        PPH_MEMORY_RESULT result;
                        ULONG lengthInBytes;
                        ULONG displayLength;

                        lengthInBytes = length;

                        if (isWide)
                            lengthInBytes *= 2;

                        if (result = PhCreateMemoryResult(
                            PTR_ADD_OFFSET(baseAddress, i - lengthInBytes),
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
                    }

                    // Reset.
                    isWide = FALSE;
                    length = 0;
                }

                byte2 = byte1;
                byte1 = byte;
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
