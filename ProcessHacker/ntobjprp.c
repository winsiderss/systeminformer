/*
 * Process Hacker -
 *   properties for NT objects
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

#include <hndlinfo.h>
#include <procprv.h>
#include <secedit.h>

#include <windowsx.h>

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_OPEN_OBJECT OpenObject;
    PVOID Context;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

#define PH_FILEMODE_ASYNC 0x01000000
#define PhFileModeUpdAsyncFlag(mode) mode & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT) ? mode &~ PH_FILEMODE_ASYNC: mode | PH_FILEMODE_ASYNC

PH_ACCESS_ENTRY FileModeAccessEntries[6] = {
    { L"FILE_FLAG_OVERLAPPED", PH_FILEMODE_ASYNC, FALSE, FALSE, L"Asynchronous" },
    { L"FILE_FLAG_WRITE_THROUGH", FILE_WRITE_THROUGH, FALSE, FALSE, L"Write through" },
    { L"FILE_FLAG_SEQUENTIAL_SCAN", FILE_SEQUENTIAL_ONLY, FALSE, FALSE, L"Sequental" },
    { L"FILE_FLAG_NO_BUFFERING", FILE_NO_INTERMEDIATE_BUFFERING, FALSE, FALSE, L"No buffering" },
    { L"FILE_SYNCHRONOUS_IO_ALERT", FILE_SYNCHRONOUS_IO_ALERT, FALSE, FALSE, L"Synchronous alert" },
    { L"FILE_SYNCHRONOUS_IO_NONALERT", FILE_SYNCHRONOUS_IO_NONALERT, FALSE, FALSE, L"Synchronous non-alert" },
};

HPROPSHEETPAGE PhpCommonCreatePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    );

INT CALLBACK PhpCommonPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK PhpEventPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpEventPairPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpFilePageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpMutantPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpSectionPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpSemaphorePageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpTimerPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static HPROPSHEETPAGE PhpCommonCreatePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhCreateAlloc(sizeof(COMMON_PAGE_CONTEXT));
    memset(pageContext, 0, sizeof(COMMON_PAGE_CONTEXT));
    pageContext->OpenObject = OpenObject;
    pageContext->Context = Context;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.pszTemplate = Template;
    propSheetPage.hInstance = PhInstanceHandle;
    propSheetPage.pfnDlgProc = DlgProc;
    propSheetPage.lParam = (LPARAM)pageContext;
    propSheetPage.pfnCallback = PhpCommonPropPageProc;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);
    // CreatePropertySheetPage would have sent PSPCB_ADDREF (below),
    // which would have added a reference.
    PhDereferenceObject(pageContext);

    return propSheetPageHandle;
}

INT CALLBACK PhpCommonPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = (PCOMMON_PAGE_CONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
    {
        PhReferenceObject(pageContext);
    }
    else if (uMsg == PSPCB_RELEASE)
    {
        PhDereferenceObject(pageContext);
    }

    return 1;
}

FORCEINLINE PCOMMON_PAGE_CONTEXT PhpCommonPageHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    return PhpGenericPropertyPageHeader(hwndDlg, uMsg, wParam, lParam, 2);
}

HPROPSHEETPAGE PhCreateEventPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJEVENT),
        PhpEventPageProc
        );
}

static VOID PhpRefreshEventPageInfo(
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
    )
{
    HANDLE eventHandle;

    if (NT_SUCCESS(PageContext->OpenObject(
        &eventHandle,
        EVENT_QUERY_STATE,
        PageContext->Context
        )))
    {
        EVENT_BASIC_INFORMATION basicInfo;
        PWSTR eventType = L"Unknown";
        PWSTR eventState = L"Unknown";

        if (NT_SUCCESS(PhGetEventBasicInformation(eventHandle, &basicInfo)))
        {
            switch (basicInfo.EventType)
            {
            case NotificationEvent:
                eventType = L"Notification";
                break;
            case SynchronizationEvent:
                eventType = L"Synchronization";
                break;
            }

            eventState = basicInfo.EventState > 0 ? L"True" : L"False";
        }

        SetDlgItemText(hwndDlg, IDC_TYPE, eventType);
        SetDlgItemText(hwndDlg, IDC_SIGNALED, eventState);

        NtClose(eventHandle);
    }
}

INT_PTR CALLBACK PhpEventPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpRefreshEventPageInfo(hwndDlg, pageContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_SET:
            case IDC_RESET:
            case IDC_PULSE:
                {
                    NTSTATUS status;
                    HANDLE eventHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &eventHandle,
                        EVENT_MODIFY_STATE,
                        pageContext->Context
                        )))
                    {
                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case IDC_SET:
                            NtSetEvent(eventHandle, NULL);
                            break;
                        case IDC_RESET:
                            NtResetEvent(eventHandle, NULL);
                            break;
                        case IDC_PULSE:
                            NtPulseEvent(eventHandle, NULL);
                            break;
                        }

                        NtClose(eventHandle);
                    }

                    PhpRefreshEventPageInfo(hwndDlg, pageContext);

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"Unable to open the event", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateEventPairPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJEVENTPAIR),
        PhpEventPairPageProc
        );
}

INT_PTR CALLBACK PhpEventPairPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Nothing
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_SETLOW:
            case IDC_SETHIGH:
                {
                    NTSTATUS status;
                    HANDLE eventPairHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &eventPairHandle,
                        EVENT_PAIR_ALL_ACCESS,
                        pageContext->Context
                        )))
                    {
                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case IDC_SETLOW:
                            NtSetLowEventPair(eventPairHandle);
                            break;
                        case IDC_SETHIGH:
                            NtSetHighEventPair(eventPairHandle);
                            break;
                        }

                        NtClose(eventPairHandle);
                    }

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"Unable to open the event pair", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateFilePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
)
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJFILE),
        PhpFilePageProc
    );
}

static VOID PhpRefreshFilePageInfo(
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
)
{
    HANDLE hFile;

    if (NT_SUCCESS(PageContext->OpenObject(
        &hFile,
        MAXIMUM_ALLOWED,
        PageContext->Context
    )))
    {
        IO_STATUS_BLOCK ioStatusBlock;
        FILE_MODE_INFORMATION fileModeInfo;
        FILE_STANDARD_INFORMATION fileStandardInfo;
        FILE_POSITION_INFORMATION filePositionInfo;
        FILE_FS_DEVICE_INFORMATION deviceInformation;
        NTSTATUS statusStandardInfo;
        BOOL disableFlushButton = FALSE;
        BOOL isFileOrDirectory = FALSE;

        // Obtain the file type
        if (NT_SUCCESS(NtQueryVolumeInformationFile(
            hFile,
            &ioStatusBlock,
            &deviceInformation,
            sizeof(deviceInformation),
            FileFsDeviceInformation
        )))
        {
            switch (deviceInformation.DeviceType)
            {
            case FILE_DEVICE_NAMED_PIPE:
                SetDlgItemText(hwndDlg, IDC_FILETYPE, L"Pipe");
                break;
            case FILE_DEVICE_CD_ROM:
            case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
            case FILE_DEVICE_CONTROLLER:
            case FILE_DEVICE_DATALINK:
            case FILE_DEVICE_DFS:
            case FILE_DEVICE_DISK:
            case FILE_DEVICE_DISK_FILE_SYSTEM:
            case FILE_DEVICE_VIRTUAL_DISK:
                isFileOrDirectory = TRUE;
                SetDlgItemText(hwndDlg, IDC_FILETYPE, L"File or directory");
                break;
            default:
                SetDlgItemText(hwndDlg, IDC_FILETYPE, L"Other");
                break;
            }
        }

        // Obtain the file size and distinguish files from directories
        if (NT_SUCCESS(statusStandardInfo = NtQueryInformationFile(
            hFile,
            &ioStatusBlock,
            &fileStandardInfo,
            sizeof(fileStandardInfo),
            FileStandardInformation
        )))
        {
            PPH_STRING fileSizeStr;

            fileSizeStr = PhFormatUInt64(fileStandardInfo.EndOfFile.QuadPart, TRUE);
            SetDlgItemText(hwndDlg, IDC_FILESIZE, fileSizeStr->Buffer);
            disableFlushButton |= fileStandardInfo.Directory;
            PhDereferenceObject(fileSizeStr);

            if (isFileOrDirectory)
            {
                SetDlgItemText(hwndDlg, IDC_FILETYPE, fileStandardInfo.Directory ? L"Directory" : L"File");
            }
        }

        // Obtain current position
        if (NT_SUCCESS(NtQueryInformationFile(
            hFile,
            &ioStatusBlock,
            &filePositionInfo,
            sizeof(filePositionInfo),
            FilePositionInformation
        )))
        {
            PPH_STRING filePosStr;

            // If we also know the size of the file, we can calculate the percentage
            if (NT_SUCCESS(statusStandardInfo) &&
                filePositionInfo.CurrentByteOffset.QuadPart != 0 &&
                fileStandardInfo.EndOfFile.QuadPart != 0
                )
            {
                PH_FORMAT format[4];

                PhInitFormatI64U(&format[0], filePositionInfo.CurrentByteOffset.QuadPart);
                format[0].Type |= FormatGroupDigits;
                PhInitFormatS(&format[1], L" (");
                PhInitFormatF(&format[2], (double)filePositionInfo.CurrentByteOffset.QuadPart / fileStandardInfo.EndOfFile.QuadPart * 100, 1);
                PhInitFormatS(&format[3], L"%)");

                filePosStr = PhFormat(format, 4, 18);
            }
            else // or we can't
            {
                filePosStr = PhFormatUInt64(filePositionInfo.CurrentByteOffset.QuadPart, TRUE);
            }
            
            SetDlgItemText(hwndDlg, IDC_POSITION, filePosStr->Buffer);
            PhDereferenceObject(filePosStr);
        }
        
        // Obtain the file mode
        if (NT_SUCCESS(NtQueryInformationFile(
            hFile,
            &ioStatusBlock,
            &fileModeInfo,
            sizeof(fileModeInfo),
            FileModeInformation
        )))
        {
            PH_FORMAT format[5];
            PPH_STRING fileModeStr, fileModeAccessStr;

            // Since FILE_MODE_INFORMATION has no flag for asynchronous I/O we should use our own flag and set
            // it only if none of synchronous flags are present. That's why we need PhFileModeUpdAsyncFlag.
            fileModeAccessStr = PhGetAccessString(
                PhFileModeUpdAsyncFlag(fileModeInfo.Mode),
                FileModeAccessEntries,
                sizeof(FileModeAccessEntries) / sizeof(PH_ACCESS_ENTRY)
            );

            PhInitFormatS(&format[0], L"0x");
            PhInitFormatX(&format[1], fileModeInfo.Mode);
            PhInitFormatS(&format[2], L" (");
            PhInitFormatSR(&format[3], fileModeAccessStr->sr);
            PhInitFormatS(&format[4], L")");
            fileModeStr = PhFormat(format, 5, 64);
            PhDereferenceObject(fileModeAccessStr);

            SetDlgItemText(hwndDlg, IDC_FILEMODE, fileModeStr->Buffer);
            PhDereferenceObject(fileModeStr);
            disableFlushButton |= fileModeInfo.Mode & (FILE_WRITE_THROUGH | FILE_NO_INTERMEDIATE_BUFFERING);
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_FILEMODE, L"Unknown");
        }

        EnableWindow(GetDlgItem(hwndDlg, IDC_FLUSH), !disableFlushButton);        
        NtClose(hFile);
    }
}

INT_PTR CALLBACK PhpFilePageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        PhpRefreshFilePageInfo(hwndDlg, pageContext);
    }
    break;
    case WM_COMMAND:
    {
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_FLUSH:
        {
            HANDLE fileHandle;
            NTSTATUS status;

            if (NT_SUCCESS(status = pageContext->OpenObject(
                &fileHandle,
                GENERIC_WRITE,
                pageContext->Context
            )))
            {
                IO_STATUS_BLOCK ioStatusBlock;

                status = NtFlushBuffersFile(fileHandle, &ioStatusBlock);
                NtClose(fileHandle);
            }

            if (!NT_SUCCESS(status))
                PhShowStatus(hwndDlg, L"Unable to flush the file buffer", status, 0);
        }
        break;
        }
    }
    break;
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateMutantPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJMUTANT),
        PhpMutantPageProc
        );
}

static VOID PhpRefreshMutantPageInfo(
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
    )
{
    HANDLE mutantHandle;

    if (NT_SUCCESS(PageContext->OpenObject(
        &mutantHandle,
        SEMAPHORE_QUERY_STATE,
        PageContext->Context
        )))
    {
        MUTANT_BASIC_INFORMATION basicInfo;
        MUTANT_OWNER_INFORMATION ownerInfo;

        if (NT_SUCCESS(PhGetMutantBasicInformation(mutantHandle, &basicInfo)))
        {
            SetDlgItemInt(hwndDlg, IDC_COUNT, basicInfo.CurrentCount, TRUE);
            SetDlgItemText(hwndDlg, IDC_ABANDONED, basicInfo.AbandonedState ? L"True" : L"False");
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_COUNT, L"Unknown");
            SetDlgItemText(hwndDlg, IDC_ABANDONED, L"Unknown");
        }

        if (NT_SUCCESS(PhGetMutantOwnerInformation(mutantHandle, &ownerInfo)))
        {
            PPH_STRING name;

            if (ownerInfo.ClientId.UniqueProcess)
            {
                name = PhGetClientIdName(&ownerInfo.ClientId);
                SetDlgItemText(hwndDlg, IDC_OWNER, name->Buffer);
                PhDereferenceObject(name);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_OWNER, L"N/A");
            }
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_OWNER, L"Unknown");
        }

        NtClose(mutantHandle);
    }
}

INT_PTR CALLBACK PhpMutantPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpRefreshMutantPageInfo(hwndDlg, pageContext);
        }
        break;
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateSectionPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJSECTION),
        PhpSectionPageProc
        );
}

static VOID PhpRefreshSectionPageInfo(
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
    )
{
    HANDLE sectionHandle;
    SECTION_BASIC_INFORMATION basicInfo;
    PWSTR sectionType = L"Unknown";
    PPH_STRING sectionSize = NULL;
    PPH_STRING fileName = NULL;

    if (!NT_SUCCESS(PageContext->OpenObject(
        &sectionHandle,
        SECTION_QUERY | SECTION_MAP_READ,
        PageContext->Context
        )))
    {
        if (!NT_SUCCESS(PageContext->OpenObject(
            &sectionHandle,
            SECTION_QUERY | SECTION_MAP_READ,
            PageContext->Context
            )))
        {
            return;
        }
    }

    if (NT_SUCCESS(PhGetSectionBasicInformation(sectionHandle, &basicInfo)))
    {
        if (basicInfo.AllocationAttributes & SEC_COMMIT)
            sectionType = L"Commit";
        else if (basicInfo.AllocationAttributes & SEC_FILE)
            sectionType = L"File";
        else if (basicInfo.AllocationAttributes & SEC_IMAGE)
            sectionType = L"Image";
        else if (basicInfo.AllocationAttributes & SEC_RESERVE)
            sectionType = L"Reserve";

        sectionSize = PhaFormatSize(basicInfo.MaximumSize.QuadPart, -1);
    }

    if (NT_SUCCESS(PhGetSectionFileName(sectionHandle, &fileName)))
    {
        PPH_STRING newFileName;

        PH_AUTO(fileName);

        if (newFileName = PhResolveDevicePrefix(fileName))
            fileName = PH_AUTO(newFileName);
    }

    SetDlgItemText(hwndDlg, IDC_TYPE, sectionType);
    SetDlgItemText(hwndDlg, IDC_SIZE_, PhGetStringOrDefault(sectionSize, L"Unknown"));
    SetDlgItemText(hwndDlg, IDC_FILE, PhGetStringOrDefault(fileName, L"N/A"));

    NtClose(sectionHandle);
}

INT_PTR CALLBACK PhpSectionPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpRefreshSectionPageInfo(hwndDlg, pageContext);
        }
        break;
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateSemaphorePage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJSEMAPHORE),
        PhpSemaphorePageProc
        );
}

static VOID PhpRefreshSemaphorePageInfo(
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
    )
{
    HANDLE semaphoreHandle;

    if (NT_SUCCESS(PageContext->OpenObject(
        &semaphoreHandle,
        SEMAPHORE_QUERY_STATE,
        PageContext->Context
        )))
    {
        SEMAPHORE_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetSemaphoreBasicInformation(semaphoreHandle, &basicInfo)))
        {
            SetDlgItemInt(hwndDlg, IDC_CURRENTCOUNT, basicInfo.CurrentCount, TRUE);
            SetDlgItemInt(hwndDlg, IDC_MAXIMUMCOUNT, basicInfo.MaximumCount, TRUE);
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_CURRENTCOUNT, L"Unknown");
            SetDlgItemText(hwndDlg, IDC_MAXIMUMCOUNT, L"Unknown");
        }

        NtClose(semaphoreHandle);
    }
}

INT_PTR CALLBACK PhpSemaphorePageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpRefreshSemaphorePageInfo(hwndDlg, pageContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_ACQUIRE:
            case IDC_RELEASE:
                {
                    NTSTATUS status;
                    HANDLE semaphoreHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &semaphoreHandle,
                        GET_WM_COMMAND_ID(wParam, lParam) == IDC_ACQUIRE ? SYNCHRONIZE : SEMAPHORE_MODIFY_STATE,
                        pageContext->Context
                        )))
                    {
                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case IDC_ACQUIRE:
                            {
                                LARGE_INTEGER timeout;

                                timeout.QuadPart = 0;
                                NtWaitForSingleObject(semaphoreHandle, FALSE, &timeout);
                            }
                            break;
                        case IDC_RELEASE:
                            NtReleaseSemaphore(semaphoreHandle, 1, NULL);
                            break;
                        }

                        NtClose(semaphoreHandle);
                    }

                    PhpRefreshSemaphorePageInfo(hwndDlg, pageContext);

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"Unable to open the semaphore", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

HPROPSHEETPAGE PhCreateTimerPage(
    _In_ PPH_OPEN_OBJECT OpenObject,
    _In_opt_ PVOID Context
    )
{
    return PhpCommonCreatePage(
        OpenObject,
        Context,
        MAKEINTRESOURCE(IDD_OBJTIMER),
        PhpTimerPageProc
        );
}

static VOID PhpRefreshTimerPageInfo(
    _In_ HWND hwndDlg,
    _In_ PCOMMON_PAGE_CONTEXT PageContext
    )
{
    HANDLE timerHandle;

    if (NT_SUCCESS(PageContext->OpenObject(
        &timerHandle,
        TIMER_QUERY_STATE,
        PageContext->Context
        )))
    {
        TIMER_BASIC_INFORMATION basicInfo;

        if (NT_SUCCESS(PhGetTimerBasicInformation(timerHandle, &basicInfo)))
        {
            SetDlgItemText(hwndDlg, IDC_SIGNALED, basicInfo.TimerState ? L"True" : L"False");
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_SIGNALED, L"Unknown");
        }

        NtClose(timerHandle);
    }
}

INT_PTR CALLBACK PhpTimerPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhpCommonPageHeader(hwndDlg, uMsg, wParam, lParam);

    if (!pageContext)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpRefreshTimerPageInfo(hwndDlg, pageContext);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_CANCEL:
                {
                    NTSTATUS status;
                    HANDLE timerHandle;

                    if (NT_SUCCESS(status = pageContext->OpenObject(
                        &timerHandle,
                        TIMER_MODIFY_STATE,
                        pageContext->Context
                        )))
                    {
                        switch (GET_WM_COMMAND_ID(wParam, lParam))
                        {
                        case IDC_CANCEL:
                            NtCancelTimer(timerHandle, NULL);
                            break;
                        }

                        NtClose(timerHandle);
                    }

                    PhpRefreshTimerPageInfo(hwndDlg, pageContext);

                    if (!NT_SUCCESS(status))
                        PhShowStatus(hwndDlg, L"Unable to open the timer", status, 0);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
