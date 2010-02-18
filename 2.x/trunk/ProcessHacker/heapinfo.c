#include <phgui.h>

typedef struct _PROCESS_HEAPS_CONTEXT
{
    PPH_PROCESS_ITEM ProcessItem;
    PRTL_PROCESS_HEAPS ProcessHeaps;
    PVOID ProcessHeap;
} PROCESS_HEAPS_CONTEXT, *PPROCESS_HEAPS_CONTEXT;

INT_PTR CALLBACK PhpProcessHeapsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowProcessHeapsDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_ITEM ProcessItem
    )
{
    NTSTATUS status;
    PROCESS_HEAPS_CONTEXT context;
    PRTL_DEBUG_INFORMATION debugBuffer;
    HANDLE processHandle;

    context.ProcessItem = ProcessItem;
    context.ProcessHeap = NULL;

    debugBuffer = RtlCreateQueryDebugBuffer(0, FALSE);

    if (!debugBuffer)
        return;

    status = RtlQueryProcessDebugInformation(
        ProcessItem->ProcessId,
        RTL_QUERY_PROCESS_HEAP_SUMMARY | RTL_QUERY_PROCESS_HEAP_ENTRIES,
        debugBuffer
        );

    if (NT_SUCCESS(status))
    {
        context.ProcessHeaps = debugBuffer->Heaps;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            ProcessQueryAccess | PROCESS_VM_READ,
            ProcessItem->ProcessId
            )))
        {
            PhGetProcessDefaultHeap(processHandle, &context.ProcessHeap);
            CloseHandle(processHandle);
        }

        DialogBoxParam(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_HEAPS),
            ParentWindowHandle,
            PhpProcessHeapsDlgProc,
            (LPARAM)&context
            );
    }
    else
    {
        PhShowStatus(ParentWindowHandle, L"Unable to query heap information", status, 0);
    }

    RtlDestroyQueryDebugBuffer(debugBuffer);
}

INT_PTR CALLBACK PhpProcessHeapsDlgProc(
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
            PPROCESS_HEAPS_CONTEXT context;
            HWND lvHandle;
            ULONG i;

            context = (PPROCESS_HEAPS_CONTEXT)lParam;
            SetProp(hwndDlg, L"Context", (HANDLE)context);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 100, L"Address");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 120, L"Used");
            PhAddListViewColumn(lvHandle, 2, 2, 2, LVCFMT_LEFT, 120, L"Committed");
            PhAddListViewColumn(lvHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Entries");
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");

            PhSetExtendedListView(lvHandle);

            for (i = 0; i < context->ProcessHeaps->NumberOfHeaps; i++)
            {
                PRTL_HEAP_INFORMATION heapInfo = &context->ProcessHeaps->Heaps[i];
                WCHAR addressString[PH_PTR_STR_LEN_1];
                INT lvItemIndex;
                PPH_STRING usedString;
                PPH_STRING committedString;
                PPH_STRING numberOfEntriesString;

                PhPrintPointer(addressString, heapInfo->BaseAddress);
                lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, addressString, heapInfo);

                usedString = PhFormatSize(heapInfo->BytesAllocated, -1);
                committedString = PhFormatSize(heapInfo->BytesCommitted, -1);
                numberOfEntriesString = PhFormatUInt64(heapInfo->NumberOfEntries, TRUE);

                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, usedString->Buffer); 
                PhSetListViewSubItem(lvHandle, lvItemIndex, 2, committedString->Buffer);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 3, numberOfEntriesString->Buffer);

                PhDereferenceObject(usedString);
                PhDereferenceObject(committedString);
                PhDereferenceObject(numberOfEntriesString);
            }
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
            case IDOK:
                EndDialog(hwndDlg, IDOK);
            }
        }
        break;
    }

    return FALSE;
}

NTSTATUS PhGetProcessDefaultHeap(
    __in HANDLE ProcessHandle,
    __out PPVOID Heap
    )
{
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION basicInfo;

    if (!NT_SUCCESS(status = PhGetProcessBasicInformation(
        ProcessHandle,
        &basicInfo
        )))
        return status;

    return status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessHeap)),
        Heap,
        sizeof(PVOID),
        NULL
        );
}
