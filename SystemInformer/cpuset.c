/*
 * Copyright (c) 2025 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer ("SI").
 *
 * Description:
 *
 *     This module contains code to support CPU Sets in SI.
 *
 * Authors:
 *
 *     wnstngs    2025
 *
 */

//
// The current version only supports modifying CPU Sets at the Process Default
// level. TODO: Support the Thread Selected level.
//

//
// ------------------------------------------------------------------- Includes
//

#include <phapp.h>
#include <procprv.h>

#include <cpuset.h>

//
// ---------------------------------------------------------------- Definitions
//

//
// Each CPU Set ID is encoded as follows:
//   Bits [31:16]: Processor group index
//   Bits [15:8]: Processor index within the group
//   Bits [7:0]: CPUSET_BIAS
//
#define CPUSET_BIAS 0x100
#define CPUSET_GROUP_SHIFT 16
#define CPUSET_ID_MASK 0xFF

/**
 * Encodes a CPU Set ID from a group index and a logical processor index.
 *
 * @param GroupIndex
 *     The index of the processor group.
 *
 * @param CpuIndex
 *     The index of the logical processor within the group.
 *
 * @return The encoded CPU Set ID.
 */
#define ENCODE_CPU_SET_ID(GroupIndex, CpuIndex) \
    (((GroupIndex) << CPUSET_GROUP_SHIFT) | \
     ((CpuIndex) & CPUSET_ID_MASK) | \
     CPUSET_BIAS)

/**
 * Decodes a CPU Set ID to extract the processor group index.
 *
 * @param CpuSetId
 *     The encoded CPU Set ID.
 *
 * @return The processor group index.
 */
#define DECODE_CPU_SET_GROUP(CpuSetId) \
    (((CpuSetId) - CPUSET_BIAS) >> CPUSET_GROUP_SHIFT)

/**
 * A helper which iterates through a list of CPU Set information structures
 * and calls the specified callback for each CPU Set.
 *
 * @param InfoStart
 *     Pointer to the start of the CPU Set information buffer.
 *
 * @param Context
 *     Optional context pointer to pass to the callback function.
 *
 * @param Callback
 *     Callback function to invoke. If it fails, the enumeration stops.
 *     TBD: Add a parameter to control this behavior (break vs continue).
 */
#define FOREACH_CPU_SET_INFO(InfoStart, Context, Callback) \
    do { \
        PSYSTEM_CPU_SET_INFORMATION _info = (InfoStart); \
        while (_info->Size != 0 && \
               RTL_CONTAINS_FIELD(_info, _info->Size, CpuSet)) { \
            if (_info->Type == CpuSetInformation) { \
                if (!Callback(_info, (PVOID)(Context))) { \
                    break; \
                } \
            } \
            _info = PTR_ADD_OFFSET(_info, _info->Size); \
        } \
    } while (0)

typedef enum PH_CPU_SET_TYPE {
    PhCpuSetTypeSystem,
    PhCpuSetTypeProcessDefault,
    PhCpuSetTypeThreadSelected
} PH_CPU_SET_TYPE;

typedef struct PHP_CPU_SETS_DLG_CTX {
    HWND WindowHandle;
    HWND ListViewHandle;
    PPH_PROCESS_ITEM ProcessItem;
    PH_CPU_SET_TYPE CpuSetType;
    PULONG CpuSetIds;
    ULONG CpuSetIdCount;
    ULONG Index;
} PHP_CPU_SETS_DLG_CTX, *PPHP_CPU_SETS_DLG_CTX;

//
// ----------------------------------------------------------------- Prototypes
//

INT_PTR
CALLBACK
PhpProcessCpuSetsDlgProc(
    _In_ HWND DlgHandle,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam
    );

//
// ------------------------------------------------------------------ Functions
//

//
// CPU Set ID Helpers
//

/**
 * Converts an array of CPU Set IDs into processor group affinity masks.
 *
 * @param CpuSetIds
 *     An array of CPU Set IDs to convert.
 *
 * @param CpuSetIdCount
 *     The number of CPU Set IDs in the array.
 *
 * @param CpuSetMasks
 *     An output buffer for processor group affinity masks.
 *
 * @param GroupCount
 *     The number of processor groups.
 *
 * @return TRUE if successful, FALSE otherwise.
 */
_Success_(return != FALSE)
static
BOOLEAN
PhpCpuSetIdsToMasks(
    _In_reads_opt_(CpuSetIdCount) CONST ULONG *CpuSetIds,
    _In_ ULONG CpuSetIdCount,
    _Out_writes_(GroupCount) PKAFFINITY CpuSetMasks,
    _In_ USHORT GroupCount
    )
{
    ULONG i;
    USHORT group;
    UCHAR cpuIndex;

    assert(CpuSetMasks != NULL);
    assert(GroupCount > 0);

    memset(CpuSetMasks, 0, GroupCount * sizeof(KAFFINITY));

    for (i = 0; i < CpuSetIdCount; i += 1) {

        //
        // Decode group and processor index
        //
        cpuIndex = (UCHAR)CpuSetIds[i];
        group = DECODE_CPU_SET_GROUP(CpuSetIds[i]);

        assert(group < GroupCount);
        assert(cpuIndex < MAXIMUM_PROC_PER_GROUP);

        if (group >= GroupCount || cpuIndex >= MAXIMUM_PROC_PER_GROUP) {
            return FALSE;
        }

        //
        // Set the corresponding bit
        //
        CpuSetMasks[group] |= ((KAFFINITY)1 << cpuIndex);
    }

    return TRUE;
}

/**
 * Converts processor group affinity masks to CPU Set IDs.
 *
 * @param CpuSetMasks
 *     An array of processor group affinity masks to be converted.
 *
 * @param GroupCount
 *     The number of processor groups.
 *
 * @param IdList
 *     An optional output array where CPU Set IDs will be stored if provided.
 *
 * @param IdListSize
 *     The size of the IdList array.
 *
 * @param RequiredCount
 *     A pointer to a variable that will receive the number of IDs required to
 *     fully represent the input affinity masks.
 *
 * @return TRUE if successful, FALSE otherwise.
 */
_Success_(return != FALSE)
static
BOOLEAN
PhpCpuSetMasksToIdList(
    _In_reads_(GroupCount) CONST KAFFINITY *CpuSetMasks,
    _In_ USHORT GroupCount,
    _Out_writes_to_opt_(IdListSize, *RequiredCount) PULONG IdList,
    _In_ ULONG IdListSize,
    _Out_ PULONG RequiredCount
    )
{
    ULONG total = 0;
    ULONG bit;
    USHORT group;
    KAFFINITY mask;

    assert(CpuSetMasks != NULL);
    assert(GroupCount > 0);
    assert(RequiredCount != NULL);

    for (group = 0; group < GroupCount; group += 1) {
        mask = CpuSetMasks[group];

        while (mask) {
            //
            // Find the next set bit (logical processor)
            //
            BitScanForward64(&bit, mask);

            //
            // Clear the bit
            //
            mask &= ~(1ULL << bit);

            //
            // Encode the CPU Set ID if there's room
            //
            if (IdList && total < IdListSize) {
                IdList[total] = ENCODE_CPU_SET_ID(group, (UCHAR)bit);
            }

            total += 1;
        }
    }

    *RequiredCount = total;

    if (total <= IdListSize) {
        return TRUE;
    }

    return FALSE;
}

//
// System CPU Sets
//

/**
 * Internal helper function to query the available CPU Sets on the system.
 *
 * @param Information
 *     A pointer to a SYSTEM_CPU_SET_INFORMATION structure that receives
 *     the CPU Set data.
 *
 * @return An NTSTATUS error code.
 */
static
NTSTATUS
PhpQueryCpuSetInformation(
    _Out_ PVOID *Information
    )
{
    NTSTATUS status;
    static ULONG initialBufferSize = 0;
    HANDLE processHandle = NULL;
    ULONG returnLength;
    PSYSTEM_CPU_SET_INFORMATION cpuSetInfo;

    *Information = NULL;

    //
    // MSDN: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_cpu_set_information
    // This is a variable-sized structure designed for future expansion. When iterating over
    // this structure, use the size field to determine the offset to the next structure.
    //
    // N.B. The kernel returns the size even in the last element, we over-allocate by the
    // Size offset and check it to minimize instructions (jxy-s).
    //

    if (initialBufferSize) {
        returnLength = initialBufferSize;
        cpuSetInfo = PhAllocateZero(returnLength);

        if (!cpuSetInfo) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else {
        returnLength = 0;
        cpuSetInfo = NULL;
    }

    status = NtQuerySystemInformationEx(SystemCpuSetInformation,
                                        &processHandle,
                                        sizeof(HANDLE),
                                        cpuSetInfo,
                                        returnLength,
                                        &returnLength);

    if (status == STATUS_BUFFER_TOO_SMALL) {
        returnLength += RTL_SIZEOF_THROUGH_FIELD(SYSTEM_CPU_SET_INFORMATION,
                                                 Size);

        PhFree(cpuSetInfo);
        cpuSetInfo = PhAllocateZero(returnLength);

        if (!cpuSetInfo) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = NtQuerySystemInformationEx(SystemCpuSetInformation,
                                            &processHandle,
                                            sizeof(HANDLE),
                                            cpuSetInfo,
                                            returnLength,
                                            NULL);

        if (NT_SUCCESS(status) && initialBufferSize <= 0x100000) {
            initialBufferSize = returnLength;
        }
    }

    if (!NT_SUCCESS(status)) {
        if (cpuSetInfo) {
            PhFree(cpuSetInfo);
            cpuSetInfo = NULL;
        }
    }

    *Information = cpuSetInfo;

    return status;
}

/**
 * Queries the available CPU Sets on the system.
 *
 * @param Information
 *     A pointer to a SYSTEM_CPU_SET_INFORMATION structure that receives
 *     the CPU Set data.
 *
 * @return An NTSTATUS error code.
 */
NTSTATUS
PhQuerySystemCpuSetInformation(
    _Out_ PVOID *Information
    )
{
    return PhpQueryCpuSetInformation(Information);
}

/**
 * Enumerates all available CPU Sets on the system and calls the callback
 * function for each CPU Set found.
 *
 * @param Callback
 *     A pointer to a callback function that is called for each CPU Set.
 *     If the callback returns FALSE, enumeration stops.
 *
 * @param Context
 *     An optional context pointer that is passed to the callback.
 *
 * @return An NTSTATUS error code.
 */
NTSTATUS
PhEnumerateSystemCpuSets(
    _In_ PPH_CPU_SETS_ENUMERATION_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID cpuSetInfo = NULL;

    if (!Callback) {
        return STATUS_INVALID_PARAMETER;
    }

    status = PhpQueryCpuSetInformation(&cpuSetInfo);

    if (NT_SUCCESS(status)) {
        FOREACH_CPU_SET_INFO(cpuSetInfo, Context, Callback);
        PhFree(cpuSetInfo);
    }

    return status;
}

//
// Process Default CPU Sets
//

/**
 * Queries the Process Default CPU Sets assigned to the given process.
 *
 * @param ProcessHandle
 *     A handle to the target process. The handle must have
 *     QUERY_LIMITED_INFORMATION permissions.
 *
 * @param CpuSetIds
 *     A pointer to an array of CPU Set IDs.
 *
 * @param CpuSetIdCount
 *     The number of CPU Set IDs in the array.
 *
 * @return An NTSTATUS error code.
 */
NTSTATUS
PhQueryCpuSetsProcess(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG *CpuSetIds,
    _Out_ PULONG CpuSetIdCount
    )
{
    NTSTATUS status;
    PKAFFINITY masks;
    ULONG bufferSize;
    static USHORT groupCount;
    ULONG requiredCount;

    groupCount = USER_SHARED_DATA->ActiveGroupCount;

    *CpuSetIds = NULL;
    *CpuSetIdCount = 0;

    bufferSize = groupCount * sizeof(KAFFINITY);
    masks = PhAllocateZero(bufferSize);

    if (!masks) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = NtQueryInformationProcess(ProcessHandle,
                                       ProcessDefaultCpuSetsInformation,
                                       masks,
                                       bufferSize,
                                       NULL);

    if (!NT_SUCCESS(status)) {
        PhFree(masks);
        return status;
    }

    PhpCpuSetMasksToIdList(masks,
                           groupCount,
                           NULL,
                           0,
                           &requiredCount);

    if (requiredCount == 0) {
        *CpuSetIds = NULL;
        *CpuSetIdCount = 0;
        PhFree(masks);
        return STATUS_SUCCESS;
    }

    *CpuSetIds = PhAllocate(requiredCount * sizeof(**CpuSetIds));

    if (!*CpuSetIds) {
        PhFree(masks);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = PhpCpuSetMasksToIdList(masks,
                                    groupCount,
                                    *CpuSetIds,
                                    requiredCount,
                                    CpuSetIdCount)
                 ? STATUS_SUCCESS
                 : STATUS_BUFFER_TOO_SMALL;

    if (!NT_SUCCESS(status)) {
        PhFree(*CpuSetIds);
        *CpuSetIds = NULL;
    }

    PhFree(masks);

    return status;
}

/**
 * Sets the Process Default CPU Sets for a process.
 *
 * @param ProcessHandle
 *     A handle to the process. The handle must have
 *     PROCESS_SET_LIMITED_INFORMATION access.
 *
 * @param CpuSetIds
 *     An optional array of CPU Set IDs to assign to the process. If NULL or
 *     CpuSetIdCount is 0, the process will be assigned to all available
 *     CPU Sets.
 *
 * @param CpuSetIdCount
 *     The number of CPU Set IDs in the array.
 *
 * @return An NTSTATUS error code.
 */
NTSTATUS
PhSetCpuSetsProcess(
    _In_ HANDLE ProcessHandle,
    _In_reads_opt_(CpuSetIdCount) const ULONG *CpuSetIds,
    _In_ ULONG CpuSetIdCount
    )
{
    NTSTATUS status;
    static USHORT groupCount;
    PKAFFINITY masks;

    groupCount = USER_SHARED_DATA->ActiveGroupCount;

    masks = PhAllocateZero(groupCount * sizeof(KAFFINITY));
    if (!masks) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!PhpCpuSetIdsToMasks(CpuSetIds, CpuSetIdCount, masks, groupCount)) {
        PhFree(masks);
        return STATUS_CPU_SET_INVALID;
    }

    status = NtSetInformationProcess(ProcessHandle,
                                     ProcessDefaultCpuSetsInformation,
                                     masks,
                                     groupCount * sizeof(KAFFINITY));

    PhFree(masks);

    return status;
}

//
// Thread Selected CPU Sets
// TODO: PhQueryCpuSetsThread()
// TODO: PhSetCpuSetsThread()
//

//
// CPU Sets Dialog
//

/**
 * Shows the CPU Sets dialog for a process.
 *
 * @param ParentWindowHandle
 *     A handle to the parent window.
 *
 * @param ProcessItem
 *     A pointer to a process item. The dialog will display and allow
 *     modification of the CPU Sets for this process.
 */
VOID
PhShowCpuSetsDialogProcess(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPHP_CPU_SETS_DLG_CTX context;

    context = PhAllocateZero(sizeof(PHP_CPU_SETS_DLG_CTX));
    context->ProcessItem = ProcessItem;
    context->CpuSetType = PhCpuSetTypeProcessDefault;

    //
    // We'll query the CPU Sets in WM_INITDIALOG
    //
    context->CpuSetIds = NULL;
    context->CpuSetIdCount = 0;

    PhDialogBox(PhInstanceHandle,
                MAKEINTRESOURCE(IDD_CPUSETS),
                ParentWindowHandle,
                PhpProcessCpuSetsDlgProc,
                context);

    //
    // Context is freed in WM_DESTROY
    //
}

/**
 * Callback function invoked for each System CPU Set from
 * PhpPopulateListViewWithCpuSets().
 * Adds detailed information about CPU Sets to a list view.
 *
 * @param Information
 *     A pointer to a structure representing a CPU Set.
 *
 * @param Context
 *     An optional pointer to a structure that provides the dialog context,
 *     including selected CPU Sets.
 *
 * @return TRUE if the operation was successful, FALSE otherwise.
 */
_Success_(return != FALSE)
static
BOOLEAN
PhpPopulateCpuSetsListViewCallback(
    _In_ PSYSTEM_CPU_SET_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    PPHP_CPU_SETS_DLG_CTX context = (PPHP_CPU_SETS_DLG_CTX)Context;
    INT lvItemIndex;
    WCHAR number[PH_INT32_STR_LEN_1];
    BOOLEAN isChecked = FALSE;
    ULONG i;
    ULONG low;
    ULONG high;
    ULONG mid;

    assert(Information != NULL);

    if (!context || !context->ListViewHandle) {
        return FALSE;
    }

    if (context->CpuSetIds && context->CpuSetIdCount > 0) {
#define BINARY_SEARCH_THRESHOLD 32
        if (context->CpuSetIdCount > BINARY_SEARCH_THRESHOLD) {
            low = 0;
            high = context->CpuSetIdCount - 1;

            while (low <= high) {
                mid = low + (high - low) / 2;

                if (context->CpuSetIds[mid] == Information->CpuSet.Id) {
                    isChecked = TRUE;
                    break;
                }

                if (context->CpuSetIds[mid] < Information->CpuSet.Id) {
                    low = mid + 1;
                }
                else {
                    high = mid - 1;
                }
            }
        }
        else {
            for (i = 0; i < context->CpuSetIdCount; i += 1) {
                if (context->CpuSetIds[i] == Information->CpuSet.Id) {
                    isChecked = TRUE;
                    break;
                }
            }
        }
    }

    PhPrintUInt32(number, Information->CpuSet.Id);

    //
    // Index = MAXINT because:
    // "If this value [Index] is greater than the number of items currently
    // contained by the listview control, the new item will be appended to
    // the end of the list and assigned the correct index"
    //
    lvItemIndex = PhAddListViewItem(context->ListViewHandle,
                                    MAXINT,
                                    number,
                                    UlongToPtr(context->Index++));

    if (isChecked) {
        assert(lvItemIndex >= 0);
        ListView_SetCheckState(context->ListViewHandle, lvItemIndex, TRUE)
    }

    //
    // Node
    //
    PhPrintUInt32(number, Information->CpuSet.NumaNodeIndex);
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, number);

    //
    // Group
    //
    PhPrintUInt32(number, Information->CpuSet.Group);
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, number);

    //
    // Core
    //
    PhPrintUInt32(number, Information->CpuSet.CoreIndex);
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, number);

    //
    // Processor
    //
    PhPrintUInt32(number, Information->CpuSet.LogicalProcessorIndex);
    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 4, number);

    //
    // Allocated
    //
    PhSetListViewSubItem(context->ListViewHandle,
                         lvItemIndex,
                         5,
                         Information->CpuSet.Allocated ? L"Yes" : L"No");

    return TRUE;
}

/**
 * Adds CPU Set items to a list view control.
 *
 * @param ListViewHandle
 *     The handle to the list view control.
 *
 * @param DialogContext
 *     A pointer to a dialog context structure.
 *
 * @return STATUS_SUCCESS if no errors, or the specific error status code.
 */
static
NTSTATUS
PhpPopulateListViewWithCpuSets(
    _In_ HWND ListViewHandle,
    _In_ PPHP_CPU_SETS_DLG_CTX DialogContext
    )
{
    if (!DialogContext || !DialogContext->ProcessItem) {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status;
    HANDLE processHandle;
    PULONG cpuSetIds = NULL;
    ULONG cpuSetIdCount = 0;
    PVOID cpuSetsInfo = NULL;

    status = PhOpenProcess(&processHandle,
                           PROCESS_QUERY_LIMITED_INFORMATION,
                           DialogContext->ProcessItem->ProcessId);

    if (!NT_SUCCESS(status)) {
        PhShowStatus(ListViewHandle,
                     L"Unable to query CPU Sets information",
                     status,
                     0);
        return status;
    }

    //
    // Query Process Default CPU Sets
    //
    status = PhQueryCpuSetsProcess(processHandle,
                                   &cpuSetIds,
                                   &cpuSetIdCount);

    NtClose(processHandle);

    //
    // Disable redraw while updating
    //
    ExtendedListView_SetRedraw(ListViewHandle, FALSE);

    if (NT_SUCCESS(status) && cpuSetIdCount > 0) {
        //
        // On success with count > 0, we keep cpuSetIds and check those IDs
        //
        if (DialogContext->CpuSetIds) {
            PhFree(DialogContext->CpuSetIds);
        }
        DialogContext->CpuSetIds = cpuSetIds;
        DialogContext->CpuSetIdCount = cpuSetIdCount;

        //
        // Enumerate all system CPU Sets and check selected
        //
        DialogContext->Index = 0;
        status = PhEnumerateSystemCpuSets(PhpPopulateCpuSetsListViewCallback,
                                          DialogContext);
    }
    else {
        //
        // Otherwise, free the allocated buffer
        //
        if (cpuSetIds) {
            PhFree(cpuSetIds);
        }
        DialogContext->CpuSetIds = NULL;
        DialogContext->CpuSetIdCount = 0;

        //
        // No Process Default CPU Set, show all System CPU Sets as unchecked
        //
        status = PhQuerySystemCpuSetInformation(&cpuSetsInfo);
        if (NT_SUCCESS(status) && cpuSetsInfo) {
            DialogContext->Index = 0;
            FOREACH_CPU_SET_INFO(cpuSetsInfo,
                                 DialogContext,
                                 PhpPopulateCpuSetsListViewCallback);
            PhFree(cpuSetsInfo);
        }

        if (cpuSetIds) {
            PhFree(cpuSetIds);
        }
    }

    //
    // Bring back redrawing
    //
    ExtendedListView_SetRedraw(ListViewHandle, TRUE);

    return status;
}

/**
 * Attempts to parse a CPU Set ID from a null-terminated wide string.
 *
 * @param CpuSetIdText
 *     The input string to parse.
 *
 * @param AsUlong
 *     Receives the parsed unsigned long on success.
 *
 * @returns
 *     TRUE if the entire string is a valid non-negative integer;
 *     FALSE otherwise.
 *
 * @remark
 *     We know that every ListView item's text is exactly PhPrintUInt32(...):
 *     pure digits, no sign, no extra spaces. We use a specialized parser that
 *     simply walks the WCHAR* buffer once and stops on the first non-digit.
 */
static
BOOL
PhpTryParseCpuId(
    _In_ LPCWSTR CpuSetIdText,
    _Out_ PULONG AsUlong
)
{
    if (!CpuSetIdText ||
        *CpuSetIdText < L'0' ||
        *CpuSetIdText > L'9') {
        return FALSE;
    }

    ULONG v = 0;
    WCHAR c;
    for (; *CpuSetIdText; CpuSetIdText += 1) {
        c = *CpuSetIdText;
        if (c < L'0' || c > L'9') {
            return FALSE;
        }
        v = v * 10 + (c - L'0');
    }
    *AsUlong = v;

    return TRUE;
}

/**
 * Retrieves the selected CPU Set IDs from a ListView.
 *
 * @param ListViewHandle
 *     The handle to the ListView control.
 *
 * @param SelectedIds
 *     A pointer to store the dynamically allocated array of selected IDs.
 *     The caller must free this memory when it is no longer needed.
 *
 * @param SelectedCount
 *     Pointer to store the count of selected IDs.
 */
static
NTSTATUS
PhpGetSelectedCpuSetIds(
    _In_ HWND ListViewHandle,
    _Out_ PULONG *SelectedIds,
    _Out_ PULONG SelectedCount
)
{
    INT i;
    INT itemCount = ListView_GetItemCount(ListViewHandle);
    ULONG count;
    WCHAR buffer[PH_INT32_STR_LEN_1];
    LVITEMW lv = {0};
    PULONG shrunk;

    if (itemCount <= 0) {
        *SelectedIds = NULL;
        *SelectedCount = 0;
        return STATUS_SUCCESS;
    }

    ULONG *ids = PhAllocate(sizeof(*ids) * itemCount);
    if (!ids) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    lv.mask = LVIF_TEXT;
    lv.iSubItem = 0;
    lv.pszText = buffer;
    lv.cchTextMax = ARRAYSIZE(buffer);

    count = 0;
    for (i = 0; i < itemCount; i += 1) {
        if (!ListView_GetCheckState(ListViewHandle, i)) {
            continue;
        }

        //
        // Retrieve the item text
        //
        lv.iItem = i;
        if (SendMessageW(ListViewHandle,
                         LVM_GETITEMTEXTW,
                         (WPARAM)i,
                         (LPARAM)&lv) <= 0) {
            continue;
        }

        if (PhpTryParseCpuId(buffer, &ids[count])) {
            count += 1;
        }
    }

    if (count == 0) {
        PhFree(ids);
        *SelectedIds = NULL;
        *SelectedCount = 0;
        return STATUS_SUCCESS;
    }

    if (count < (ULONG)itemCount) {
        shrunk = PhReAllocate(ids, sizeof(*ids) * count);
        if (shrunk) {
            ids = shrunk;
        }
    }

    *SelectedIds = ids;
    *SelectedCount = count;

    return STATUS_SUCCESS;
}

INT_PTR
CALLBACK
PhpProcessCpuSetsDlgProc(
    _In_ HWND DlgHandle,
    _In_ UINT Message,
    _In_ WPARAM WParam,
    _In_ LPARAM LParam
)
{
    PPHP_CPU_SETS_DLG_CTX context;

    if (Message == WM_INITDIALOG) {
        context = (PPHP_CPU_SETS_DLG_CTX)LParam;

        if (!context) {
            return FALSE;
        }

        context->WindowHandle = DlgHandle;

        PhSetWindowContext(DlgHandle,
                           PH_WINDOW_CONTEXT_DEFAULT,
                           context);
    }
    else {
        context = PhGetWindowContext(DlgHandle, PH_WINDOW_CONTEXT_DEFAULT);

        if (!context && Message != WM_DESTROY) {
            return FALSE;
        }
    }

    switch (Message) {
    case WM_INITDIALOG: {
        NTSTATUS status;
        PWSTR titleText;

        context->WindowHandle = DlgHandle;
        context->ListViewHandle = GetDlgItem(DlgHandle, IDC_LIST);
        context->Index = 0;

        PhSetApplicationWindowIcon(DlgHandle);

        //
        // Set dialog title with process name if available
        //
        // @formatter:off
        if (context->ProcessItem) {
            titleText = PhaFormatString(L"CPU Sets - %s (%lu)",
                                        context->ProcessItem->ProcessName->Buffer,
                                        HandleToUlong(context->ProcessItem->ProcessId))->Buffer;

            SetWindowText(DlgHandle, titleText);
        }

        PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 60, L"ID");
        PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 60, L"Node");
        PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 60, L"Group");
        PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 60, L"Core");
        PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 60, L"Processor");
        PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 60, L"Allocated");
        // @formatter:on

        PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);

        PhSetControlTheme(context->ListViewHandle, L"explorer");

        PhSetExtendedListView(context->ListViewHandle);

        ListView_SetExtendedListViewStyleEx(context->ListViewHandle,
                                            LVS_EX_CHECKBOXES,
                                            LVS_EX_CHECKBOXES);

        ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

        ListView_DeleteAllItems(context->ListViewHandle);

        status = PhpPopulateListViewWithCpuSets(context->ListViewHandle,
                                                context);

        ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

        if (!NT_SUCCESS(status)) {
            //
            // If we couldn't query the CPU Sets, close the dialog
            //
            EndDialog(DlgHandle, IDCANCEL);
            break;
        }

        //
        // Enable/disable the OK button based on whether we have a process
        //
        EnableWindow(GetDlgItem(DlgHandle, IDOK), context->ProcessItem != NULL);

        PhSetDialogFocus(DlgHandle, GetDlgItem(DlgHandle, IDOK));

        PhInitializeWindowTheme(DlgHandle, PhEnableThemeSupport);
    }
    break;
    case WM_DESTROY: {
        PPHP_CPU_SETS_DLG_CTX destroyContext =
            PhGetWindowContext(DlgHandle, PH_WINDOW_CONTEXT_DEFAULT);

        if (destroyContext) {
            if (destroyContext->CpuSetIds) {
                PhFree(destroyContext->CpuSetIds);
                destroyContext->CpuSetIds = NULL;
                destroyContext->CpuSetIdCount = 0;
            }

            PhRemoveWindowContext(DlgHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(destroyContext);
        }

        return FALSE;
    }
    case WM_COMMAND: {
        switch (GET_WM_COMMAND_ID(WParam, LParam)) {
        case IDCANCEL:
            EndDialog(DlgHandle, IDCANCEL);
            break;
        case IDC_SELECTALL:
        case IDC_DESELECTALL: {
            BOOL state = (GET_WM_COMMAND_ID(WParam, LParam) == IDC_SELECTALL);
            HWND listView = context->ListViewHandle;
            for (INT i = 0; i < ListView_GetItemCount(listView); i += 1) {
                ListView_SetCheckState(listView, i, state)
            }
            break;
        }
        case IDOK: {
            NTSTATUS status;
            HANDLE processHandle = NULL;
            PULONG selectedIds = NULL;
            ULONG selectedCount = 0;
            BOOLEAN succeeded = FALSE;

            if (!context || !context->ListViewHandle) {
                break;
            }

            //
            // Get selected CPU Sets from the list view
            //
            status = PhpGetSelectedCpuSetIds(context->ListViewHandle,
                                             &selectedIds,
                                             &selectedCount);

            if (!NT_SUCCESS(status)) {
                PhShowStatus(DlgHandle,
                             L"Failed to retrieve selected CPU Set IDs.",
                             status,
                             0);
                break;
            }

            if (!selectedIds && selectedCount > 0) {
                selectedCount = 0;
            }

            //
            // Apply the selected CPU Sets to the process if we have a process
            //
            if (context->ProcessItem && context->ProcessItem->ProcessId) {
                status = PhOpenProcess(&processHandle,
                                       PROCESS_SET_LIMITED_INFORMATION |
                                       PROCESS_QUERY_LIMITED_INFORMATION,
                                       context->ProcessItem->ProcessId);
                if (NT_SUCCESS(status)) {
                    //
                    // Set the process CPU Sets
                    //
                    status = PhSetCpuSetsProcess(processHandle,
                                                 selectedIds,
                                                 selectedCount);

                    if (NT_SUCCESS(status)) {
                        //
                        // Verify the changes were actually applied
                        //
                        PULONG verifyIds = NULL;
                        ULONG verifyCount = 0;

                        //
                        // Query the process again to verify our changes
                        //
                        status = PhQueryCpuSetsProcess(processHandle,
                                                       &verifyIds,
                                                       &verifyCount);

                        if (NT_SUCCESS(status)) {
                            //
                            // If we selected CPU Sets but none were found on
                            // verification, something might have gone wrong
                            //
                            if (selectedCount > 0 && verifyCount == 0) {
                                succeeded = FALSE;
                                status = STATUS_UNSUCCESSFUL;
                            }
                            else {
                                succeeded = TRUE;
                            }

                            if (verifyIds) {
                                PhFree(verifyIds);
                                verifyIds = NULL;
                            }
                        }
                        else {
                            succeeded = FALSE;
                        }
                    }
                    else {
                        succeeded = FALSE;
                    }

                    NtClose(processHandle);
                }

                if (!succeeded) {
                    PhShowStatus(DlgHandle,
                                 L"Unable to set CPU Sets for the process.",
                                 status,
                                 0);

                    if (selectedIds) {
                        PhFree(selectedIds);
                        selectedIds = NULL;
                    }

                    break;
                }
            }
            else {
                //
                // If we don't have a process to set CPU Set for,
                // just consider it successful
                //
                succeeded = TRUE;
            }

            if (succeeded) {
                //
                // Free the previously selected IDs if any
                //
                if (context->CpuSetIds) {
                    PhFree(context->CpuSetIds);
                }

                //
                // Update context with the newly selected IDs
                //
                context->CpuSetIds = selectedIds;
                context->CpuSetIdCount = selectedCount;

                EndDialog(DlgHandle, IDOK);
            }
            else {
                //
                // Clean up on failure
                //
                if (selectedIds) {
                    PhFree(selectedIds);
                    selectedIds = NULL;
                }
            }
        }
        break;
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(DlgHandle,
                                     WParam,
                                     LParam,
                                     PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(DlgHandle,
                                     WParam,
                                     LParam,
                                     PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(DlgHandle,
                                        WParam,
                                        LParam,
                                        PhWindowThemeControlColor);
    }
    }

    return FALSE;
}
