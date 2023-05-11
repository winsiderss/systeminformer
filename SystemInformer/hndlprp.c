/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2018-2023
 *
 */

#include <phapp.h>
#include <hndlprv.h>
#include <phplug.h>
#include <phsettings.h>
#include <kphuser.h>

#include <emenu.h>
#include <settings.h>
#include <hndlinfo.h>
#include <procprv.h>
#include <secedit.h>

typedef enum _PHP_HANDLE_GENERAL_CATEGORY
{
    // common
    PH_HANDLE_GENERAL_CATEGORY_BASICINFO,
    PH_HANDLE_GENERAL_CATEGORY_REFERENCES,
    PH_HANDLE_GENERAL_CATEGORY_QUOTA,
    // extra
    PH_HANDLE_GENERAL_CATEGORY_ALPC,
    PH_HANDLE_GENERAL_CATEGORY_FILE,
    PH_HANDLE_GENERAL_CATEGORY_SECTION,
    PH_HANDLE_GENERAL_CATEGORY_MUTANT,
    PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
    PH_HANDLE_GENERAL_CATEGORY_ETW,

    PH_HANDLE_GENERAL_CATEGORY_MAXIMUM
} PHP_HANDLE_GENERAL_CATEGORY;

typedef enum _PHP_HANDLE_GENERAL_INDEX
{
    PH_HANDLE_GENERAL_INDEX_NAME,
    PH_HANDLE_GENERAL_INDEX_TYPE,
    PH_HANDLE_GENERAL_INDEX_OBJECT,
    PH_HANDLE_GENERAL_INDEX_ACCESSMASK,

    PH_HANDLE_GENERAL_INDEX_REFERENCES,
    PH_HANDLE_GENERAL_INDEX_HANDLES,

    PH_HANDLE_GENERAL_INDEX_PAGED,
    PH_HANDLE_GENERAL_INDEX_NONPAGED,

    PH_HANDLE_GENERAL_INDEX_FLAGS,
    PH_HANDLE_GENERAL_INDEX_SEQUENCENUMBER,
    PH_HANDLE_GENERAL_INDEX_PORTCONTEXT,

    PH_HANDLE_GENERAL_INDEX_FILETYPE,
    PH_HANDLE_GENERAL_INDEX_FILEMODE,
    PH_HANDLE_GENERAL_INDEX_FILEPOSITION,
    PH_HANDLE_GENERAL_INDEX_FILESIZE,
    PH_HANDLE_GENERAL_INDEX_FILEDRIVER,
    PH_HANDLE_GENERAL_INDEX_FILEDRIVERIMAGE,

    PH_HANDLE_GENERAL_INDEX_SECTIONTYPE,
    PH_HANDLE_GENERAL_INDEX_SECTIONFILE,
    PH_HANDLE_GENERAL_INDEX_SECTIONSIZE,

    PH_HANDLE_GENERAL_INDEX_MUTANTCOUNT,
    PH_HANDLE_GENERAL_INDEX_MUTANTABANDONED,
    PH_HANDLE_GENERAL_INDEX_MUTANTOWNER,

    PH_HANDLE_GENERAL_INDEX_ALPCCONNECTION,
    PH_HANDLE_GENERAL_INDEX_ALPCSERVER,
    PH_HANDLE_GENERAL_INDEX_ALPCCLIENT,

    PH_HANDLE_GENERAL_INDEX_PROCESSTHREADNAME,
    PH_HANDLE_GENERAL_INDEX_PROCESSTHREADCREATETIME,
    PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITTIME,
    PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITCODE,

    PH_HANDLE_GENERAL_INDEX_ETWORIGINALNAME,
    PH_HANDLE_GENERAL_INDEX_ETWGROUPNAME,

    PH_HANDLE_GENERAL_INDEX_MAXIMUM
} PHP_HANDLE_GENERAL_INDEX;

typedef struct _HANDLE_PROPERTIES_CONTEXT
{
    HWND ListViewHandle;
    HWND ParentWindow;
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
    PH_LAYOUT_MANAGER LayoutManager;
    INT ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MAXIMUM];
} HANDLE_PROPERTIES_CONTEXT, *PHANDLE_PROPERTIES_CONTEXT;

#define PH_FILEMODE_ASYNC 0x01000000
#define PhFileModeUpdAsyncFlag(mode) \
    ((mode) & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT) ? (mode) &~ PH_FILEMODE_ASYNC: (mode) | PH_FILEMODE_ASYNC)

PH_ACCESS_ENTRY FileModeAccessEntries[6] =
{
    { L"FILE_FLAG_OVERLAPPED", PH_FILEMODE_ASYNC, FALSE, FALSE, L"Asynchronous" },
    { L"FILE_FLAG_WRITE_THROUGH", FILE_WRITE_THROUGH, FALSE, FALSE, L"Write through" },
    { L"FILE_FLAG_SEQUENTIAL_SCAN", FILE_SEQUENTIAL_ONLY, FALSE, FALSE, L"Sequential" },
    { L"FILE_FLAG_NO_BUFFERING", FILE_NO_INTERMEDIATE_BUFFERING, FALSE, FALSE, L"No buffering" },
    { L"FILE_SYNCHRONOUS_IO_ALERT", FILE_SYNCHRONOUS_IO_ALERT, FALSE, FALSE, L"Synchronous alert" },
    { L"FILE_SYNCHRONOUS_IO_NONALERT", FILE_SYNCHRONOUS_IO_NONALERT, FALSE, FALSE, L"Synchronous non-alert" },
};

INT_PTR CALLBACK PhpHandleGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static NTSTATUS PhpDuplicateHandleFromProcess(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Context
    )
{
    PHANDLE_PROPERTIES_CONTEXT context = Context;
    NTSTATUS status;
    HANDLE processHandle;

    *Handle = NULL;

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        context->ProcessId
        )))
    {
        status = NtDuplicateObject(
            processHandle,
            context->HandleItem->Handle,
            NtCurrentProcess(),
            Handle,
            DesiredAccess,
            0,
            0
            );
        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status) && KphLevel() >= KphLevelMax)
    {
        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            context->ProcessId
            )))
        {
            status = KphDuplicateObject(
                processHandle,
                context->HandleItem->Handle,
                DesiredAccess,
                Handle
                );
            NtClose(processHandle);
        }
    }

    return status;
}

typedef struct _HANDLE_PROPERTIES_THREAD_CONTEXT
{
    HWND ParentWindowHandle;
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
} HANDLE_PROPERTIES_THREAD_CONTEXT, *PHANDLE_PROPERTIES_THREAD_CONTEXT;

NTSTATUS PhpShowHandlePropertiesThread(
    _In_ PVOID Parameter
    )
{
    PHANDLE_PROPERTIES_THREAD_CONTEXT handleContext = Parameter;
    PROPSHEETHEADER propSheetHeader = { sizeof(PROPSHEETHEADER) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[16];
    HANDLE_PROPERTIES_CONTEXT context;
    PH_AUTO_POOL autoPool;

    context.ProcessId = handleContext->ProcessId;
    context.HandleItem = handleContext->HandleItem;

    PhInitializeAutoPool(&autoPool);

    propSheetHeader.dwFlags =
        PSH_MODELESS |
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE;
    propSheetHeader.hInstance = PhInstanceHandle;
    propSheetHeader.hwndParent = handleContext->ParentWindowHandle;
    propSheetHeader.pszCaption = L"Handle";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // General page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_HNDLGENERAL);
    propSheetPage.hInstance = PhInstanceHandle;
    propSheetPage.pfnDlgProc = PhpHandleGeneralDlgProc;
    propSheetPage.lParam = (LPARAM)&context;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Object-specific page
    if (PhIsNullOrEmptyString(handleContext->HandleItem->TypeName))
    {
        NOTHING;
    }
    else if (PhEqualString2(handleContext->HandleItem->TypeName, L"Event", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateEventPage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(handleContext->HandleItem->TypeName, L"EventPair", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateEventPairPage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(handleContext->HandleItem->TypeName, L"Job", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateJobPage(
            PhpDuplicateHandleFromProcess,
            &context,
            NULL
            );
    }
    else if (PhEqualString2(handleContext->HandleItem->TypeName, L"Semaphore", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateSemaphorePage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(handleContext->HandleItem->TypeName, L"Timer", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateTimerPage(
            PhpDuplicateHandleFromProcess,
            &context
            );
    }
    else if (PhEqualString2(handleContext->HandleItem->TypeName, L"Token", TRUE))
    {
        pages[propSheetHeader.nPages++] = PhCreateTokenPage(
            PhpDuplicateHandleFromProcess,
            context.ProcessId,
            &context,
            NULL
            );
    }
    //else if (PhEqualString2(handleContext->HandleItem->TypeName, L"Section", TRUE))
    //{
    //    pages[propSheetHeader.nPages++] = PhCreateMappingsPage(
    //        handleContext->ProcessId,
    //        handleContext->HandleItem->Handle
    //        );
    //}

    // Security page
    pages[propSheetHeader.nPages++] = PhCreateSecurityPage(
        PhGetStringOrEmpty(handleContext->HandleItem->BestObjectName),
        PhGetStringOrEmpty(handleContext->HandleItem->TypeName),
        PhpDuplicateHandleFromProcess,
        NULL,
        &context
        );

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_OBJECT_PROPERTIES objectProperties;
        PH_PLUGIN_HANDLE_PROPERTIES_CONTEXT propertiesContext;

        propertiesContext.ProcessId = handleContext->ProcessId;
        propertiesContext.HandleItem = handleContext->HandleItem;

        objectProperties.Parameter = &propertiesContext;
        objectProperties.NumberOfPages = propSheetHeader.nPages;
        objectProperties.MaximumNumberOfPages = RTL_NUMBER_OF(pages);
        objectProperties.Pages = pages;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackHandlePropertiesInitializing), &objectProperties);

        propSheetHeader.nPages = objectProperties.NumberOfPages;
    }

    PhModalPropertySheet(&propSheetHeader);

    PhDeleteAutoPool(&autoPool);

    PhDereferenceObject(handleContext->HandleItem);
    PhFree(handleContext);

    return STATUS_SUCCESS;
}

VOID PhShowHandleProperties(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_HANDLE_ITEM HandleItem
    )
{
    PHANDLE_PROPERTIES_THREAD_CONTEXT context;

    context = PhAllocate(sizeof(HANDLE_PROPERTIES_THREAD_CONTEXT));
    context->ParentWindowHandle = PhCsForceNoParent ? NULL : ParentWindowHandle;
    context->ProcessId = ProcessId;
    context->HandleItem = HandleItem;
    PhReferenceObject(HandleItem);

    PhCreateThread2(PhpShowHandlePropertiesThread, context);
}

VOID PhpUpdateHandleGeneralListViewGroups(
    _In_ PHANDLE_PROPERTIES_CONTEXT Context
    )
{
    ListView_EnableGroupView(Context->ListViewHandle, TRUE);
    PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_BASICINFO, L"Basic information");
    PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_REFERENCES, L"References");
    PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_QUOTA, L"Quota charges");

    Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_NAME] = PhAddListViewGroupItem(
        Context->ListViewHandle,
        PH_HANDLE_GENERAL_CATEGORY_BASICINFO,
        PH_HANDLE_GENERAL_INDEX_NAME,
        L"Name",
        NULL
        );
    Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_TYPE] = PhAddListViewGroupItem(
        Context->ListViewHandle,
        PH_HANDLE_GENERAL_CATEGORY_BASICINFO,
        PH_HANDLE_GENERAL_INDEX_TYPE,
        L"Type",
        NULL
        );
    Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_OBJECT] = PhAddListViewGroupItem(
        Context->ListViewHandle,
        PH_HANDLE_GENERAL_CATEGORY_BASICINFO,
        PH_HANDLE_GENERAL_INDEX_OBJECT,
        L"Object address",
        NULL
        );
    Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ACCESSMASK] = PhAddListViewGroupItem(
        Context->ListViewHandle,
        PH_HANDLE_GENERAL_CATEGORY_BASICINFO,
        PH_HANDLE_GENERAL_INDEX_ACCESSMASK,
        L"Granted access",
        NULL
        );
    Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_REFERENCES] = PhAddListViewGroupItem(
        Context->ListViewHandle,
        PH_HANDLE_GENERAL_CATEGORY_REFERENCES,
        PH_HANDLE_GENERAL_INDEX_REFERENCES,
        L"References",
        NULL
        );
    Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_HANDLES] = PhAddListViewGroupItem(
        Context->ListViewHandle,
        PH_HANDLE_GENERAL_CATEGORY_REFERENCES,
        PH_HANDLE_GENERAL_INDEX_HANDLES,
        L"Handles",
        NULL
        );
    Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PAGED] = PhAddListViewGroupItem(
        Context->ListViewHandle,
        PH_HANDLE_GENERAL_CATEGORY_QUOTA,
        PH_HANDLE_GENERAL_INDEX_PAGED,
        L"Paged",
        NULL
        );
    Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_NONPAGED] = PhAddListViewGroupItem(
        Context->ListViewHandle,
        PH_HANDLE_GENERAL_CATEGORY_QUOTA,
        PH_HANDLE_GENERAL_INDEX_NONPAGED,
        L"Virtual size",
        NULL
        );

    if (PhIsNullOrEmptyString(Context->HandleItem->TypeName))
    {
        NOTHING;
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"ALPC Port", TRUE))
    {
        PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_ALPC, L"ALPC Port");
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FLAGS] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_ALPC,
            PH_HANDLE_GENERAL_INDEX_FLAGS,
            L"Flags",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SEQUENCENUMBER] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_ALPC,
            PH_HANDLE_GENERAL_INDEX_SEQUENCENUMBER,
            L"Sequence Number",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PORTCONTEXT] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_ALPC,
            PH_HANDLE_GENERAL_INDEX_PORTCONTEXT,
            L"Port Context",
            NULL
            );

        if (WindowsVersion >= WINDOWS_10_19H2)
        {
            Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MUTANTOWNER] = PhAddListViewGroupItem(
                Context->ListViewHandle,
                PH_HANDLE_GENERAL_CATEGORY_ALPC,
                PH_HANDLE_GENERAL_INDEX_MUTANTOWNER,
                L"Owner",
                NULL
                );
        }

        if (KphLevel() >= KphLevelMed)
        {
            Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ALPCCONNECTION] = PhAddListViewGroupItem(
                Context->ListViewHandle,
                PH_HANDLE_GENERAL_CATEGORY_ALPC,
                PH_HANDLE_GENERAL_INDEX_ALPCCONNECTION,
                L"Connection",
                NULL
                );

            Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ALPCSERVER] = PhAddListViewGroupItem(
                Context->ListViewHandle,
                PH_HANDLE_GENERAL_CATEGORY_ALPC,
                PH_HANDLE_GENERAL_INDEX_ALPCSERVER,
                L"Server",
                NULL
                );

            Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ALPCCLIENT] = PhAddListViewGroupItem(
                Context->ListViewHandle,
                PH_HANDLE_GENERAL_CATEGORY_ALPC,
                PH_HANDLE_GENERAL_INDEX_ALPCCLIENT,
                L"Client",
                NULL
                );
        }
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"EtwRegistration", TRUE))
    {
        PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_ETW, L"Event trace information");
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ETWORIGINALNAME] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_ETW,
            PH_HANDLE_GENERAL_INDEX_ETWORIGINALNAME,
            L"GUID",
            NULL
            );
        //Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ETWGROUPNAME] = PhAddListViewGroupItem(
        //    Context->ListViewHandle,
        //    PH_HANDLE_GENERAL_CATEGORY_ETW,
        //    PH_HANDLE_GENERAL_INDEX_ETWGROUPNAME,
        //    L"Group GUID",
        //    NULL
        //    );
    }
    else if (PhEqualStringRef2(&Context->HandleItem->TypeName->sr, L"File", TRUE))
    {
        PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_FILE, L"File information");

        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_FILE,
            PH_HANDLE_GENERAL_INDEX_FILETYPE,
            L"Type",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEMODE] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_FILE,
            PH_HANDLE_GENERAL_INDEX_FILEMODE,
            L"Mode",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEPOSITION] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_FILE,
            PH_HANDLE_GENERAL_INDEX_FILEPOSITION,
            L"Position",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILESIZE] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_FILE,
            PH_HANDLE_GENERAL_INDEX_FILESIZE,
            L"Size",
            NULL
            );

        if (KphLevel() >= KphLevelMed)
        {
            Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEDRIVER] = PhAddListViewGroupItem(
                Context->ListViewHandle,
                PH_HANDLE_GENERAL_CATEGORY_FILE,
                PH_HANDLE_GENERAL_INDEX_FILEDRIVER,
                L"Driver",
                NULL
                );

            Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEDRIVERIMAGE] = PhAddListViewGroupItem(
                Context->ListViewHandle,
                PH_HANDLE_GENERAL_CATEGORY_FILE,
                PH_HANDLE_GENERAL_INDEX_FILEDRIVERIMAGE,
                L"Driver Image",
                NULL
                );
        }
    }
    else if (PhEqualStringRef2(&Context->HandleItem->TypeName->sr, L"Section", TRUE))
    {
        PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_SECTION, L"Section information");

        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SECTIONTYPE] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_SECTION,
            PH_HANDLE_GENERAL_INDEX_SECTIONTYPE,
            L"Type",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SECTIONFILE] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_SECTION,
            PH_HANDLE_GENERAL_INDEX_SECTIONFILE,
            L"File",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SECTIONSIZE] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_SECTION,
            PH_HANDLE_GENERAL_INDEX_SECTIONSIZE,
            L"Size",
            NULL
            );
    }
    else if (PhEqualStringRef2(&Context->HandleItem->TypeName->sr, L"Mutant", TRUE))
    {
        PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_MUTANT, L"Mutant information");

        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MUTANTCOUNT] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_MUTANT,
            PH_HANDLE_GENERAL_INDEX_MUTANTCOUNT,
            L"Count",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MUTANTABANDONED] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_MUTANT,
            PH_HANDLE_GENERAL_INDEX_MUTANTABANDONED,
            L"Abandoned",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MUTANTOWNER] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_MUTANT,
            PH_HANDLE_GENERAL_INDEX_MUTANTOWNER,
            L"Owner",
            NULL
            );
    }
    else if (PhEqualStringRef2(&Context->HandleItem->TypeName->sr, L"Process", TRUE))
    {
        PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD, L"Process information");

        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADNAME] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
            PH_HANDLE_GENERAL_INDEX_PROCESSTHREADNAME,
            L"Name",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADCREATETIME] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
            PH_HANDLE_GENERAL_INDEX_PROCESSTHREADCREATETIME,
            L"Created",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITTIME] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
            PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITTIME,
            L"Exited",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITCODE] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
            PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITCODE,
            L"Exit status",
            NULL
            );
    }
    else if (PhEqualStringRef2(&Context->HandleItem->TypeName->sr, L"Thread", TRUE))
    {
        PhAddListViewGroup(Context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD, L"Thread information");

        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADNAME] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
            PH_HANDLE_GENERAL_INDEX_PROCESSTHREADNAME,
            L"Name",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADCREATETIME] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
            PH_HANDLE_GENERAL_INDEX_PROCESSTHREADCREATETIME,
            L"Created",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITTIME] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
            PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITTIME,
            L"Exited",
            NULL
            );
        Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITCODE] = PhAddListViewGroupItem(
            Context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_PROCESSTHREAD,
            PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITCODE,
            L"Exit status",
            NULL
            );
    }
}

VOID PhpUpdateHandleGeneral(
    _In_ PHANDLE_PROPERTIES_CONTEXT Context
    )
{
    HANDLE processHandle;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;
    OBJECT_BASIC_INFORMATION basicInfo;
    WCHAR string[PH_INT64_STR_LEN_1];

    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_NAME], 1, PhGetStringOrEmpty(Context->HandleItem->BestObjectName));
    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_TYPE], 1, PhGetStringOrEmpty(Context->HandleItem->TypeName));

    if (Context->HandleItem->Object)
    {
        PhPrintPointer(string, Context->HandleItem->Object);
        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_OBJECT], 1, string);
    }

    if (PhGetAccessEntries(
        PhGetStringOrEmpty(Context->HandleItem->TypeName),
        &accessEntries,
        &numberOfAccessEntries
        ))
    {
        PPH_STRING accessString;
        PPH_STRING grantedAccessString;

        accessString = PH_AUTO(PhGetAccessString(
            Context->HandleItem->GrantedAccess,
            accessEntries,
            numberOfAccessEntries
            ));

        if (accessString->Length != 0)
        {
            grantedAccessString = PH_AUTO(PhFormatString(
                L"0x%x (%s)",
                Context->HandleItem->GrantedAccess,
                accessString->Buffer
                ));

            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_ACCESSMASK, 1, grantedAccessString->Buffer);
        }
        else
        {
            PhPrintPointer(string, UlongToPtr(Context->HandleItem->GrantedAccess));
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_ACCESSMASK, 1, string);
        }

        PhFree(accessEntries);
    }
    else
    {
        PhPrintPointer(string, UlongToPtr(Context->HandleItem->GrantedAccess));
        PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_ACCESSMASK, 1, string);
    }

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        (KphLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
        Context->ProcessId
        )))
    {
        if (NT_SUCCESS(PhGetHandleInformation(
            processHandle,
            Context->HandleItem->Handle,
            ULONG_MAX,
            &basicInfo,
            NULL,
            NULL,
            NULL
            )))
        {
            PhPrintUInt32(string, basicInfo.PointerCount);
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_REFERENCES, 1, string);

            PhPrintUInt32(string, basicInfo.HandleCount);
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_HANDLES, 1, string);

            PhPrintUInt32(string, basicInfo.PagedPoolCharge);
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_PAGED, 1, string);

            PhPrintUInt32(string, basicInfo.NonPagedPoolCharge);
            PhSetListViewSubItem(Context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_NONPAGED, 1, string);
        }

        NtClose(processHandle);
    }

    if (PhIsNullOrEmptyString(Context->HandleItem->TypeName))
    {
        NOTHING;
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"ALPC Port", TRUE))
    {
        NTSTATUS status;
        HANDLE processHandle = NULL;

        if (KphLevel() >= KphLevelMed && NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                Context->ProcessId
                )))
        {
            //
            // TODO this path doesn't use all the ALPC info returned yet
            // see: KPH_ALPC_BASIC_INFORMATION.State
            //
            KPH_ALPC_BASIC_INFORMATION basicInfo;
            PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION connectionNames;
            KPH_ALPC_COMMUNICATION_INFORMATION connectionInfo;

            if (NT_SUCCESS(KphAlpcQueryInformation(
                processHandle,
                Context->HandleItem->Handle,
                KphAlpcBasicInformation,
                &basicInfo,
                sizeof(basicInfo),
                NULL
                )))
            {
                ULONG remainingFlags = basicInfo.Flags;
                PH_STRING_BUILDER stringBuilder;

                PhInitializeStringBuilder(&stringBuilder, 0x100);

                if (basicInfo.Flags & ALPC_PORFLG_LPC_MODE)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"LPC mode, ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_LPC_MODE);
                }
                if (basicInfo.Flags & ALPC_PORFLG_ALLOW_IMPERSONATION)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"Allow impersonation, ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_ALLOW_IMPERSONATION);
                }
                if (basicInfo.Flags & ALPC_PORFLG_ALLOW_LPC_REQUESTS)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"Allow LPC requests, ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_ALLOW_LPC_REQUESTS);
                }
                if (basicInfo.Flags & ALPC_PORFLG_WAITABLE_PORT)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"Waitable, ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_WAITABLE_PORT);
                }
                if (basicInfo.Flags & ALPC_PORFLG_ALLOW_DUP_OBJECT)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"Allow object duplication, ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_ALLOW_DUP_OBJECT);
                }
                if (basicInfo.Flags & ALPC_PORFLG_SYSTEM_PROCESS)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"System process only, ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_SYSTEM_PROCESS);
                }
                if (basicInfo.Flags & ALPC_PORFLG_WAKE_POLICY1)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"Wake policy (1), ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_WAKE_POLICY1);
                }
                if (basicInfo.Flags & ALPC_PORFLG_WAKE_POLICY2)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"Wake policy (2), ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_WAKE_POLICY2);
                }
                if (basicInfo.Flags & ALPC_PORFLG_WAKE_POLICY3)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"Wake policy (3), ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_WAKE_POLICY3);
                }
                if (basicInfo.Flags & ALPC_PORFLG_DIRECT_MESSAGE)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"No shared section (direct), ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_DIRECT_MESSAGE);
                }
                if (basicInfo.Flags & ALPC_PORFLG_ALLOW_MULTIHANDLE_ATTRIBUTE)
                {
                    PhAppendStringBuilder2(&stringBuilder, L"Allow multi-handle attributes, ");
                    ClearFlag(remainingFlags, ALPC_PORFLG_ALLOW_MULTIHANDLE_ATTRIBUTE);
                }
                if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                    PhRemoveEndStringBuilder(&stringBuilder, 2);

                if (basicInfo.Flags == 0)
                    PhAppendStringBuilder2(&stringBuilder, L"None ");
                else
                {
                    PhPrintPointer(string, UlongToPtr(basicInfo.Flags));
                    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", string);
                }

                if (remainingFlags)
                {
                    PhPrintPointer(string, UlongToPtr(remainingFlags));
                    PhAppendFormatStringBuilder(&stringBuilder, L" (UNKNOWN: %s)", string);
                }

                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FLAGS], 1, PhFinalStringBuilderString(&stringBuilder)->Buffer);
                PhDeleteStringBuilder(&stringBuilder);

                PhPrintUInt32(string, basicInfo.SequenceNo);
                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SEQUENCENUMBER], 1, string);

                PhPrintPointer(string, basicInfo.PortContext);
                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PORTCONTEXT], 1, string);
            }

            if (!NT_SUCCESS(KphAlpcQueryComminicationsNamesInfo(
                processHandle,
                Context->HandleItem->Handle,
                &connectionNames)))
            {
                connectionNames = NULL;
            }

            if (NT_SUCCESS(KphAlpcQueryInformation(
                processHandle,
                Context->HandleItem->Handle,
                KphAlpcCommunicationInformation,
                &connectionInfo,
                sizeof(connectionInfo),
                NULL
                )))
            {
                CLIENT_ID clientId;
                PPH_STRING name;

                if (connectionInfo.ConnectionPort.OwnerProcessId)
                {
                    clientId.UniqueProcess = connectionInfo.ConnectionPort.OwnerProcessId;
                    clientId.UniqueThread = 0;

                    name = PhStdGetClientIdName(&clientId);

                    if (connectionNames && connectionNames->ConnectionPort.Length > 0)
                    {
                        PPH_STRING newName;
                        PH_FORMAT format[3];

                        PhInitFormatSR(&format[0], name->sr);
                        PhInitFormatS(&format[1], L" - ");
                        PhInitFormatUCS(&format[2], &connectionNames->ConnectionPort);

                        newName = PhFormat(format, 3, MAX_PATH);
                        PhDereferenceObject(name);
                        name = newName;
                    }

                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ALPCCONNECTION], 1, name->Buffer);
                    PhDereferenceObject(name);
                }

                if (connectionInfo.ServerCommunicationPort.OwnerProcessId)
                {
                    clientId.UniqueProcess = connectionInfo.ServerCommunicationPort.OwnerProcessId;
                    clientId.UniqueThread = 0;

                    name = PhStdGetClientIdName(&clientId);

                    if (connectionNames && connectionNames->ServerCommunicationPort.Length > 0)
                    {
                        PPH_STRING newName;
                        PH_FORMAT format[3];

                        PhInitFormatSR(&format[0], name->sr);
                        PhInitFormatS(&format[1], L" - ");
                        PhInitFormatUCS(&format[2], &connectionNames->ServerCommunicationPort);

                        newName = PhFormat(format, 3, MAX_PATH);
                        PhDereferenceObject(name);
                        name = newName;
                    }

                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ALPCSERVER], 1, name->Buffer);
                    PhDereferenceObject(name);
                }

                if (connectionInfo.ClientCommunicationPort.OwnerProcessId)
                {
                    clientId.UniqueProcess = connectionInfo.ClientCommunicationPort.OwnerProcessId;
                    clientId.UniqueThread = 0;

                    name = PhStdGetClientIdName(&clientId);

                    if (connectionNames && connectionNames->ClientCommunicationPort.Length > 0)
                    {
                        PPH_STRING newName;
                        PH_FORMAT format[3];

                        PhInitFormatSR(&format[0], name->sr);
                        PhInitFormatS(&format[1], L" - ");
                        PhInitFormatUCS(&format[2], &connectionNames->ClientCommunicationPort);

                        newName = PhFormat(format, 3, MAX_PATH);
                        PhDereferenceObject(name);
                        name = newName;
                    }

                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ALPCCLIENT], 1, name->Buffer);
                    PhDereferenceObject(name);
                }

                if (connectionNames)
                    PhFree(connectionNames);

                NtClose(processHandle);
            }
            else
            {
                HANDLE alpcPortHandle = NULL;

                if (NT_SUCCESS(status = PhOpenProcess(
                    &processHandle,
                    PROCESS_DUP_HANDLE,
                    Context->ProcessId
                    )))
                {
                    status = NtDuplicateObject(
                        processHandle,
                        Context->HandleItem->Handle,
                        NtCurrentProcess(),
                        &alpcPortHandle,
                        READ_CONTROL,
                        0,
                        0
                        );
                    NtClose(processHandle);
                }

                if (NT_SUCCESS(status) && alpcPortHandle)
                {
                    ALPC_BASIC_INFORMATION basicInfo;

                    if (NT_SUCCESS(NtAlpcQueryInformation(
                        alpcPortHandle,
                        AlpcBasicInformation,
                        &basicInfo,
                        sizeof(ALPC_BASIC_INFORMATION),
                        NULL
                        )))
                    {
                        ULONG remainingFlags = basicInfo.Flags;
                        PH_STRING_BUILDER stringBuilder;

                        PhInitializeStringBuilder(&stringBuilder, 0x100);

                        if (basicInfo.Flags & ALPC_PORFLG_LPC_MODE)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"LPC mode, ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_LPC_MODE);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_ALLOW_IMPERSONATION)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Allow impersonation, ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_ALLOW_IMPERSONATION);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_ALLOW_LPC_REQUESTS)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Allow LPC requests, ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_ALLOW_LPC_REQUESTS);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_WAITABLE_PORT)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Waitable, ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_WAITABLE_PORT);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_ALLOW_DUP_OBJECT)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Allow object duplication, ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_ALLOW_DUP_OBJECT);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_SYSTEM_PROCESS)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"System process only, ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_SYSTEM_PROCESS);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_WAKE_POLICY1)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Wake policy (1), ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_WAKE_POLICY1);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_WAKE_POLICY2)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Wake policy (2), ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_WAKE_POLICY2);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_WAKE_POLICY3)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Wake policy (3), ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_WAKE_POLICY3);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_DIRECT_MESSAGE)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"No shared section (direct), ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_DIRECT_MESSAGE);
                        }
                        if (basicInfo.Flags & ALPC_PORFLG_ALLOW_MULTIHANDLE_ATTRIBUTE)
                        {
                            PhAppendStringBuilder2(&stringBuilder, L"Allow multi-handle attributes, ");
                            ClearFlag(remainingFlags, ALPC_PORFLG_ALLOW_MULTIHANDLE_ATTRIBUTE);
                        }
                        if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                            PhRemoveEndStringBuilder(&stringBuilder, 2);

                        if (basicInfo.Flags == 0)
                            PhAppendStringBuilder2(&stringBuilder, L"None ");
                        else
                        {
                            PhPrintPointer(string, UlongToPtr(basicInfo.Flags));
                            PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", string);
                        }

                        if (remainingFlags)
                        {
                            PhPrintPointer(string, UlongToPtr(remainingFlags));
                            PhAppendFormatStringBuilder(&stringBuilder, L" (UNKNOWN: %s)", string);
                        }

                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FLAGS], 1, PhFinalStringBuilderString(&stringBuilder)->Buffer);
                        PhDeleteStringBuilder(&stringBuilder);

                        PhPrintUInt32(string, basicInfo.SequenceNo);
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SEQUENCENUMBER], 1, string);

                        PhPrintPointer(string, basicInfo.PortContext);
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PORTCONTEXT], 1, string);
                    }

                    if (WindowsVersion >= WINDOWS_10_19H2)
                    {
                        ALPC_SERVER_SESSION_INFORMATION serverInfo;

                        if (NT_SUCCESS(NtAlpcQueryInformation(
                            alpcPortHandle,
                            AlpcServerSessionInformation,
                            &serverInfo,
                            sizeof(ALPC_SERVER_SESSION_INFORMATION),
                            NULL
                            )))
                        {
                            CLIENT_ID clientId;
                            PPH_STRING name;

                            clientId.UniqueProcess = UlongToHandle(serverInfo.ProcessId);
                            clientId.UniqueThread = 0;

                            name = PhGetClientIdName(&clientId);
                            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MUTANTOWNER], 1, name->Buffer);
                            PhDereferenceObject(name);
                        }
                    }

                    NtClose(alpcPortHandle);
                }
            }
        }
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"EtwRegistration", TRUE))
    {
        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ETWORIGINALNAME], 1, PhGetString(Context->HandleItem->ObjectName));
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"File", TRUE))
    {
        NTSTATUS status;
        HANDLE processHandle;

        if (KphLevel() >= KphLevelMed && NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                Context->ProcessId
                )))
        {
            BOOLEAN isFileOrDirectory = FALSE;
            BOOLEAN isConsoleHandle = FALSE;
            FILE_FS_DEVICE_INFORMATION fileDeviceInfo;
            FILE_MODE_INFORMATION fileModeInfo;
            FILE_STANDARD_INFORMATION fileStandardInfo;
            FILE_POSITION_INFORMATION filePositionInfo;
            IO_STATUS_BLOCK isb;
            KPH_FILE_OBJECT_DRIVER fileObjectDriver;

            if (NT_SUCCESS(KphQueryVolumeInformationFile(
                processHandle,
                Context->HandleItem->Handle,
                FileFsDeviceInformation,
                &fileDeviceInfo,
                sizeof(FILE_FS_DEVICE_INFORMATION),
                &isb
                )))
            {
                switch (fileDeviceInfo.DeviceType)
                {
                case FILE_DEVICE_NAMED_PIPE:
                    //isPipeHandle = TRUE;
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"Pipe");
                    break;
                case FILE_DEVICE_NETWORK:
                    //isNetworkHandle = TRUE;
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"Network");
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
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"File or directory");
                    break;
                case FILE_DEVICE_CONSOLE:
                    isConsoleHandle = TRUE;
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"Console");
                    break;
                default:
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"Other");
                    break;
                }
            }

            // Note: These devices deadlock without a timeout (dmex)
            // 1) Named pipes
            // 2) \Device\ConDrv\CurrentIn
            // 3) \Device\VolMgrControl

            if (NT_SUCCESS(status = PhCallKphQueryFileInformationWithTimeout(
                processHandle,
                Context->HandleItem->Handle,
                FileModeInformation,
                &fileModeInfo,
                sizeof(FILE_MODE_INFORMATION)
                )))
            {
                PH_FORMAT format[5];
                PPH_STRING fileModeAccessStr;
                WCHAR fileModeString[MAX_PATH];

                // Since FILE_MODE_INFORMATION has no flag for asynchronous I/O we should use our own flag and set
                // it only if none of synchronous flags are present. That's why we need PhFileModeUpdAsyncFlag.
                fileModeAccessStr = PhGetAccessString(
                    PhFileModeUpdAsyncFlag(fileModeInfo.Mode),
                    FileModeAccessEntries,
                    RTL_NUMBER_OF(FileModeAccessEntries)
                    );

                PhInitFormatS(&format[0], L"0x");
                PhInitFormatX(&format[1], fileModeInfo.Mode);
                PhInitFormatS(&format[2], L" (");
                PhInitFormatSR(&format[3], fileModeAccessStr->sr);
                PhInitFormatS(&format[4], L")");

                if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), fileModeString, sizeof(fileModeString), NULL))
                {
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEMODE], 1, fileModeString);
                }

                PhDereferenceObject(fileModeAccessStr);
            }

            if (!isConsoleHandle)
            {
                if (NT_SUCCESS(status = PhCallKphQueryFileInformationWithTimeout(
                    processHandle,
                    Context->HandleItem->Handle,
                    FileStandardInformation,
                    &fileStandardInfo,
                    sizeof(FILE_STANDARD_INFORMATION)
                    )))
                {
                    PH_FORMAT format[1];
                    WCHAR fileSizeString[PH_INT64_STR_LEN];

                    PhInitFormatSize(&format[0], fileStandardInfo.EndOfFile.QuadPart);

                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), fileSizeString, sizeof(fileSizeString), NULL))
                    {
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILESIZE], 1, fileSizeString);
                    }

                    if (isFileOrDirectory)
                    {
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, fileStandardInfo.Directory ? L"Directory" : L"File");
                    }

                    //disableFlushButton |= fileStandardInfo.Directory;
                }

                if (NT_SUCCESS(status = PhCallKphQueryFileInformationWithTimeout(
                    processHandle,
                    Context->HandleItem->Handle,
                    FilePositionInformation,
                    &filePositionInfo,
                    sizeof(FILE_POSITION_INFORMATION)
                    )))
                {
                    if (filePositionInfo.CurrentByteOffset.QuadPart != 0 && fileStandardInfo.EndOfFile.QuadPart != 0)
                    {
                        PH_FORMAT format[4];
                        WCHAR filePositionString[PH_INT64_STR_LEN];

                        PhInitFormatI64UGroupDigits(&format[0], filePositionInfo.CurrentByteOffset.QuadPart);
                        PhInitFormatS(&format[1], L" (");
                        PhInitFormatF(&format[2], (DOUBLE)filePositionInfo.CurrentByteOffset.QuadPart / (DOUBLE)fileStandardInfo.EndOfFile.QuadPart * 100.0, 1);
                        PhInitFormatS(&format[3], L"%)");

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), filePositionString, sizeof(filePositionString), NULL))
                        {
                            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEPOSITION], 1, filePositionString);
                        }
                    }
                    else if (filePositionInfo.CurrentByteOffset.QuadPart != 0)
                    {
                        PH_FORMAT format[1];
                        WCHAR filePositionString[PH_INT64_STR_LEN];

                        PhInitFormatI64UGroupDigits(&format[0], filePositionInfo.CurrentByteOffset.QuadPart);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), filePositionString, sizeof(filePositionString), NULL))
                        {
                            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEPOSITION], 1, filePositionString);
                        }
                    }
                }
            }

            if (NT_SUCCESS(KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectFileObjectDriver,
                &fileObjectDriver,
                sizeof(fileObjectDriver),
                NULL
                )))
            {
                PPH_STRING string;

                if (NT_SUCCESS(PhGetDriverName(fileObjectDriver.DriverHandle, &string)))
                {
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEDRIVER], 1, PhGetString(string));
                    PhDereferenceObject(string);
                }

                if (NT_SUCCESS(PhGetDriverImageFileName(fileObjectDriver.DriverHandle, &string)))
                {
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEDRIVERIMAGE], 1, PhGetString(string));
                    PhDereferenceObject(string);
                }

                NtClose(fileObjectDriver.DriverHandle);
            }

            NtClose(processHandle);
        }
        else
        {
            HANDLE fileHandle = NULL;

            if (NT_SUCCESS(status = PhOpenProcess(
                &processHandle,
                PROCESS_DUP_HANDLE,
                Context->ProcessId
                )))
            {
                status = NtDuplicateObject(
                    processHandle,
                    Context->HandleItem->Handle,
                    NtCurrentProcess(),
                    &fileHandle,
                    MAXIMUM_ALLOWED,
                    0,
                    0
                    );
                NtClose(processHandle);
            }

            if (NT_SUCCESS(status) && fileHandle)
            {
                //BOOLEAN disableFlushButton = FALSE;
                BOOLEAN isFileOrDirectory = FALSE;
                BOOLEAN isConsoleHandle = FALSE;
                //BOOLEAN isPipeHandle = FALSE;
                //BOOLEAN isNetworkHandle = FALSE;
                FILE_FS_DEVICE_INFORMATION fileDeviceInfo;
                FILE_MODE_INFORMATION fileModeInfo;
                FILE_STANDARD_INFORMATION fileStandardInfo;
                FILE_POSITION_INFORMATION filePositionInfo;
                IO_STATUS_BLOCK isb;

                if (NT_SUCCESS(NtQueryVolumeInformationFile(
                    fileHandle,
                    &isb,
                    &fileDeviceInfo,
                    sizeof(FILE_FS_DEVICE_INFORMATION),
                    FileFsDeviceInformation
                    )))
                {
                    switch (fileDeviceInfo.DeviceType)
                    {
                    case FILE_DEVICE_NAMED_PIPE:
                        //isPipeHandle = TRUE;
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"Pipe");
                        break;
                    case FILE_DEVICE_NETWORK:
                        //isNetworkHandle = TRUE;
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"Network");
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
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"File or directory");
                        break;
                    case FILE_DEVICE_CONSOLE:
                        isConsoleHandle = TRUE;
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"Console");
                        break;
                    default:
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, L"Other");
                        break;
                    }
                }

                // Note: These devices deadlock without a timeout (dmex)
                // 1) Named pipes
                // 2) \Device\ConDrv\CurrentIn
                // 3) \Device\VolMgrControl

                if (NT_SUCCESS(status = PhCallNtQueryFileInformationWithTimeout(
                    fileHandle,
                    FileModeInformation,
                    &fileModeInfo,
                    sizeof(FILE_MODE_INFORMATION)
                    )))
                {
                    PH_FORMAT format[5];
                    PPH_STRING fileModeAccessStr;
                    WCHAR fileModeString[MAX_PATH];

                    // Since FILE_MODE_INFORMATION has no flag for asynchronous I/O we should use our own flag and set
                    // it only if none of synchronous flags are present. That's why we need PhFileModeUpdAsyncFlag.
                    fileModeAccessStr = PhGetAccessString(
                        PhFileModeUpdAsyncFlag(fileModeInfo.Mode),
                        FileModeAccessEntries,
                        RTL_NUMBER_OF(FileModeAccessEntries)
                        );

                    PhInitFormatS(&format[0], L"0x");
                    PhInitFormatX(&format[1], fileModeInfo.Mode);
                    PhInitFormatS(&format[2], L" (");
                    PhInitFormatSR(&format[3], fileModeAccessStr->sr);
                    PhInitFormatS(&format[4], L")");

                    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), fileModeString, sizeof(fileModeString), NULL))
                    {
                        PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEMODE], 1, fileModeString);
                    }

                    PhDereferenceObject(fileModeAccessStr);
                }

                if (!isConsoleHandle)
                {
                    if (NT_SUCCESS(status = PhCallNtQueryFileInformationWithTimeout(
                        fileHandle,
                        FileStandardInformation,
                        &fileStandardInfo,
                        sizeof(FILE_STANDARD_INFORMATION)
                        )))
                    {
                        PH_FORMAT format[1];
                        WCHAR fileSizeString[PH_INT64_STR_LEN];

                        PhInitFormatSize(&format[0], fileStandardInfo.EndOfFile.QuadPart);

                        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), fileSizeString, sizeof(fileSizeString), NULL))
                        {
                            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILESIZE], 1, fileSizeString);
                        }

                        if (isFileOrDirectory)
                        {
                            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILETYPE], 1, fileStandardInfo.Directory ? L"Directory" : L"File");
                        }

                        //disableFlushButton |= fileStandardInfo.Directory;
                    }

                    if (NT_SUCCESS(status = PhCallNtQueryFileInformationWithTimeout(
                        fileHandle,
                        FilePositionInformation,
                        &filePositionInfo,
                        sizeof(FILE_POSITION_INFORMATION)
                        )))
                    {
                        if (filePositionInfo.CurrentByteOffset.QuadPart != 0 && fileStandardInfo.EndOfFile.QuadPart != 0)
                        {
                            PH_FORMAT format[4];
                            WCHAR filePositionString[PH_INT64_STR_LEN];

                            PhInitFormatI64UGroupDigits(&format[0], filePositionInfo.CurrentByteOffset.QuadPart);
                            PhInitFormatS(&format[1], L" (");
                            PhInitFormatF(&format[2], (DOUBLE)filePositionInfo.CurrentByteOffset.QuadPart / (DOUBLE)fileStandardInfo.EndOfFile.QuadPart * 100.0, 1);
                            PhInitFormatS(&format[3], L"%)");

                            if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), filePositionString, sizeof(filePositionString), NULL))
                            {
                                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEPOSITION], 1, filePositionString);
                            }
                        }
                        else if (filePositionInfo.CurrentByteOffset.QuadPart != 0)
                        {
                            PH_FORMAT format[1];
                            WCHAR filePositionString[PH_INT64_STR_LEN];

                            PhInitFormatI64UGroupDigits(&format[0], filePositionInfo.CurrentByteOffset.QuadPart);

                            if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), filePositionString, sizeof(filePositionString), NULL))
                            {
                                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FILEPOSITION], 1, filePositionString);
                            }
                        }
                    }
                }

                NtClose(fileHandle);
            }
        }
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"Section", TRUE))
    {
        NTSTATUS status;
        HANDLE processHandle;
        SECTION_BASIC_INFORMATION basicInfo;
        PPH_STRING fileName = NULL;

        if (KphLevel() >= KphLevelMed && NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                Context->ProcessId
                )))
        {
            ULONG bufferSize;
            ULONG returnLength;
            PUNICODE_STRING buffer;
            NTSTATUS status2;

            returnLength = 0;
            bufferSize = 0x100;
            buffer = PhAllocate(bufferSize);

            status = KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectSectionBasicInformation,
                &basicInfo,
                sizeof(basicInfo),
                NULL
                );

            status2 = KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectSectionFileName,
                buffer,
                bufferSize,
                &returnLength
                );
            if (status2 == STATUS_BUFFER_OVERFLOW && returnLength > 0)
            {
                PhFree(buffer);
                bufferSize = returnLength;
                buffer = PhAllocate(returnLength);

                status2 = KphQueryInformationObject(
                    processHandle,
                    Context->HandleItem->Handle,
                    KphObjectSectionFileName,
                    buffer,
                    bufferSize,
                    &returnLength
                    );
            }

            if (NT_SUCCESS(status2))
            {
                fileName = PhCreateStringFromUnicodeString(buffer);
            }

            PhFree(buffer);
            NtClose(processHandle);
        }
        else
        {
            HANDLE sectionHandle = NULL;

            if (NT_SUCCESS(status = PhOpenProcess(
                &processHandle,
                PROCESS_DUP_HANDLE,
                Context->ProcessId
                )))
            {
                status = NtDuplicateObject(
                    processHandle,
                    Context->HandleItem->Handle,
                    NtCurrentProcess(),
                    &sectionHandle,
                    SECTION_QUERY | SECTION_MAP_READ,
                    0,
                    0
                    );

                if (!NT_SUCCESS(status))
                {
                    status = NtDuplicateObject(
                        processHandle,
                        Context->HandleItem->Handle,
                        NtCurrentProcess(),
                        &sectionHandle,
                        SECTION_QUERY,
                        0,
                        0
                        );
                }

                NtClose(processHandle);
            }

            if (NT_SUCCESS(status) && sectionHandle)
            {
                status = PhGetSectionBasicInformation(sectionHandle, &basicInfo);

                if (!NT_SUCCESS(PhGetSectionFileName(sectionHandle, &fileName)))
                {
                    fileName = NULL;
                }

                NtClose(sectionHandle);
            }
        }

        if (NT_SUCCESS(status))
        {
            PWSTR sectionType = L"Unknown";
            PPH_STRING sectionSize = NULL;

            if (basicInfo.AllocationAttributes & SEC_COMMIT)
                sectionType = L"Commit";
            else if (basicInfo.AllocationAttributes & SEC_FILE)
                sectionType = L"File";
            else if (basicInfo.AllocationAttributes & SEC_IMAGE)
                sectionType = L"Image";
            else if (basicInfo.AllocationAttributes & SEC_RESERVE)
                sectionType = L"Reserve";

            sectionSize = PhaFormatSize(basicInfo.MaximumSize.QuadPart, -1);

            if (fileName)
            {
                PPH_STRING newFileName;

                if (newFileName = PhResolveDevicePrefix(&fileName->sr))
                {
                    PhDereferenceObject(fileName);
                    fileName = newFileName;
                }
            }

            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SECTIONFILE], 1, PhGetStringOrDefault(fileName, L"N/A"));
            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SECTIONTYPE], 1, sectionType);
            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SECTIONSIZE], 1, PhGetStringOrDefault(sectionSize, L"Unknown"));
        }
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"Mutant", TRUE))
    {
        NTSTATUS status;
        HANDLE processHandle;
        HANDLE mutantHandle = NULL;

        if (NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_DUP_HANDLE,
            Context->ProcessId
            )))
        {
            status = NtDuplicateObject(
                processHandle,
                Context->HandleItem->Handle,
                NtCurrentProcess(),
                &mutantHandle,
                SEMAPHORE_QUERY_STATE,
                0,
                0
                );
            NtClose(processHandle);
        }

        if (NT_SUCCESS(status) && mutantHandle)
        {
            MUTANT_BASIC_INFORMATION basicInfo;
            MUTANT_OWNER_INFORMATION ownerInfo;

            if (NT_SUCCESS(PhGetMutantBasicInformation(mutantHandle, &basicInfo)))
            {
                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MUTANTCOUNT], 1, PhaFormatUInt64(basicInfo.CurrentCount, TRUE)->Buffer);
                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MUTANTABANDONED], 1, basicInfo.AbandonedState ? L"True" : L"False");
            }

            if (NT_SUCCESS(PhGetMutantOwnerInformation(mutantHandle, &ownerInfo)))
            {
                PPH_STRING name;

                if (ownerInfo.ClientId.UniqueProcess)
                {
                    name = PhGetClientIdName(&ownerInfo.ClientId);
                    PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_MUTANTOWNER], 1, name->Buffer);
                    PhDereferenceObject(name);
                }
            }

            NtClose(mutantHandle);
        }
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"Process", TRUE))
    {
        NTSTATUS status;
        HANDLE processHandle;
        NTSTATUS exitStatus = STATUS_PENDING;
        PPH_STRING fileName = NULL;
        PROCESS_BASIC_INFORMATION basicInfo;
        KERNEL_USER_TIMES times;
        BOOLEAN haveTimes = FALSE;

        if (KphLevel() >= KphLevelMed && NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                Context->ProcessId
                )))
        {
            ULONG bufferSize;
            ULONG returnLength;
            PUNICODE_STRING buffer;
            NTSTATUS status2;

            returnLength = 0;
            bufferSize = 0x100;
            buffer = PhAllocate(bufferSize);

            if (NT_SUCCESS(KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectProcessBasicInformation,
                &basicInfo,
                sizeof(basicInfo),
                NULL
                )))
            {
                exitStatus = basicInfo.ExitStatus;
            }

            haveTimes = NT_SUCCESS(KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectProcessTimes,
                &times,
                sizeof(times),
                NULL
                ));

            status2 = KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectProcessImageFileName,
                buffer,
                bufferSize,
                &returnLength
                );
            if (status2 == STATUS_BUFFER_TOO_SMALL && returnLength > 0)
            {
                PhFree(buffer);
                bufferSize = returnLength;
                buffer = PhAllocate(returnLength);

                status2 = KphQueryInformationObject(
                    processHandle,
                    Context->HandleItem->Handle,
                    KphObjectProcessImageFileName,
                    buffer,
                    bufferSize,
                    &returnLength
                    );
            }

            if (NT_SUCCESS(status2))
            {
                PPH_STRING newName;

                fileName = PhCreateStringFromUnicodeString(buffer);

                newName = PhGetFileName(fileName);
                PhDereferenceObject(fileName);
                fileName = newName;
            }

            NtClose(processHandle);
        }
        else
        {
            HANDLE dupHandle = NULL;

            if (NT_SUCCESS(status = PhOpenProcess(
                &processHandle,
                PROCESS_DUP_HANDLE,
                Context->ProcessId
                )))
            {
                status = NtDuplicateObject(
                    processHandle,
                    Context->HandleItem->Handle,
                    NtCurrentProcess(),
                    &dupHandle,
                    PROCESS_QUERY_LIMITED_INFORMATION,
                    0,
                    0
                    );

                NtClose(processHandle);
            }

            if (NT_SUCCESS(status) && dupHandle)
            {
                if (NT_SUCCESS(PhGetProcessImageFileName(dupHandle, &fileName)))
                {
                    PPH_STRING newName;

                    newName = PhGetFileName(fileName);
                    PhDereferenceObject(fileName);
                    fileName = newName;
                }

                if (NT_SUCCESS(PhGetProcessBasicInformation(dupHandle, &basicInfo)))
                {
                    exitStatus = basicInfo.ExitStatus;
                }

                haveTimes = NT_SUCCESS(PhGetProcessTimes(dupHandle, &times));

                NtClose(dupHandle);
            }
        }

        if (fileName)
        {
            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADNAME], 1, PhGetStringOrEmpty(fileName));
            PhDereferenceObject(fileName);
        }

        if (haveTimes)
        {
            SYSTEMTIME time;

            PhLargeIntegerToLocalSystemTime(&time, &times.CreateTime);
            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADCREATETIME], 1, PhaFormatDateTime(&time)->Buffer);

            if (exitStatus != STATUS_PENDING)
            {
                PhLargeIntegerToLocalSystemTime(&time, &times.ExitTime);
                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITTIME], 1, PhaFormatDateTime(&time)->Buffer);
            }
        }

        if (exitStatus != STATUS_PENDING)
        {
            PPH_STRING status;
            PPH_STRING exitcode;

            status = PhGetStatusMessage(exitStatus, 0);
            exitcode = PhFormatString(
                L"0x%x (%s)",
                exitStatus,
                PhGetStringOrDefault(status, L"Unknown")
                );

            PhSetListViewSubItem(
                Context->ListViewHandle,
                Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITCODE],
                1,
                PhGetStringOrEmpty(exitcode)
                );

            PhDereferenceObject(exitcode);
            PhClearReference(&status);
        }
    }
    else if (PhEqualString2(Context->HandleItem->TypeName, L"Thread", TRUE))
    {
        NTSTATUS status;
        HANDLE processHandle;
        BOOLEAN isTerminated = FALSE;
        PPH_STRING name = NULL;
        KERNEL_USER_TIMES times;
        NTSTATUS exitStatus = STATUS_PENDING;

        if (KphLevel() >= KphLevelMed && NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                Context->ProcessId
                )))
        {
            ULONG bufferSize;
            ULONG returnLength;
            PTHREAD_NAME_INFORMATION buffer;
            NTSTATUS status2;
            ULONG threadIsTerminated;
            THREAD_BASIC_INFORMATION basicInfo;

            returnLength = 0;
            bufferSize = 0x100;
            buffer = PhAllocate(bufferSize);

            status2 = KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectThreadNameInformation,
                buffer,
                bufferSize,
                &returnLength
                );
            if (status2 == STATUS_BUFFER_TOO_SMALL && returnLength > 0)
            {
                PhFree(buffer);
                bufferSize = returnLength;
                buffer = PhAllocate(returnLength);

                status2 = KphQueryInformationObject(
                    processHandle,
                    Context->HandleItem->Handle,
                    KphObjectThreadNameInformation,
                    buffer,
                    bufferSize,
                    &returnLength
                    );
            }

            if (NT_SUCCESS(status2))
            {
                name = PhCreateStringFromUnicodeString(&buffer->ThreadName);
            }

            if (NT_SUCCESS(KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectThreadIsTerminated,
                &threadIsTerminated,
                sizeof(threadIsTerminated),
                NULL
                )))
            {
                isTerminated = !!threadIsTerminated;
            }

            if (isTerminated && NT_SUCCESS(KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectThreadBasicInformation,
                &basicInfo,
                sizeof(basicInfo),
                NULL
                )))
            {
                exitStatus = basicInfo.ExitStatus;
            }

            status = KphQueryInformationObject(
                processHandle,
                Context->HandleItem->Handle,
                KphObjectThreadTimes,
                &times,
                sizeof(times),
                NULL
                );

            PhFree(buffer);
            NtClose(processHandle);
        }
        else
        {
            HANDLE dupHandle = NULL;
            THREAD_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(status = PhOpenProcess(
                &processHandle,
                PROCESS_DUP_HANDLE,
                Context->ProcessId
                )))
            {
                status = NtDuplicateObject(
                    processHandle,
                    Context->HandleItem->Handle,
                    NtCurrentProcess(),
                    &dupHandle,
                    THREAD_QUERY_LIMITED_INFORMATION,
                    0,
                    0
                    );

                NtClose(processHandle);
            }

            if (NT_SUCCESS(status) && dupHandle)
            {

                if (NT_SUCCESS(PhGetThreadName(dupHandle, &name)))
                {
                    PhDereferenceObject(name);
                }

                PhGetThreadIsTerminated(dupHandle, &isTerminated);

                if (isTerminated && NT_SUCCESS(PhGetThreadBasicInformation(dupHandle, &basicInfo)))
                {
                    exitStatus = basicInfo.ExitStatus;
                }

                status = PhGetThreadTimes(dupHandle, &times);

                NtClose(dupHandle);
            }
        }

        if (name)
        {
            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADNAME], 1, PhGetStringOrEmpty(name));
            PhDereferenceObject(name);
        }

        if (isTerminated)
        {
            PPH_STRING status;
            PPH_STRING exitcode;

            status = PhGetStatusMessage(exitStatus, 0);
            exitcode = PhFormatString(
                L"0x%x (%s)",
                exitStatus,
                PhGetStringOrDefault(status, L"Unknown")
                );

            PhSetListViewSubItem(
                Context->ListViewHandle,
                Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITCODE],
                1,
                PhGetStringOrEmpty(exitcode)
                );

            PhDereferenceObject(exitcode);
            PhClearReference(&status);
        }

        if (NT_SUCCESS(status))
        {
            SYSTEMTIME time;

            PhLargeIntegerToLocalSystemTime(&time, &times.CreateTime);
            PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADCREATETIME], 1, PhaFormatDateTime(&time)->Buffer);

            if (isTerminated)
            {
                PhLargeIntegerToLocalSystemTime(&time, &times.ExitTime);
                PhSetListViewSubItem(Context->ListViewHandle, Context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PROCESSTHREADEXITTIME], 1, PhaFormatDateTime(&time)->Buffer);
            }
        }
    }
}

INT_PTR CALLBACK PhpHandleGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PHANDLE_PROPERTIES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
        context = (PHANDLE_PROPERTIES_CONTEXT)propSheetPage->lParam;

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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->ParentWindow = GetParent(hwndDlg);

            PhSetApplicationWindowIcon(context->ParentWindow);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 250, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            // HACK
            if (PhGetIntegerPairSetting(L"HandlePropertiesWindowPosition").X != 0)
                PhLoadWindowPlacementFromSetting(L"HandlePropertiesWindowPosition", NULL, context->ParentWindow);
            else
                PhCenterWindow(context->ParentWindow, GetParent(context->ParentWindow)); // HACK

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PhpUpdateHandleGeneralListViewGroups(context);
            PhpUpdateHandleGeneral(context);

            PhRegisterWindowCallback(context->ParentWindow, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            if (PhEnableThemeSupport) // TODO: Required for compat (dmex)
                PhInitializeWindowTheme(context->ParentWindow, PhEnableThemeSupport);
            else
                PhInitializeWindowTheme(hwndDlg, FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterWindowCallback(context->ParentWindow);

            PhSaveWindowPlacementToSetting(L"HandlePropertiesWindowPosition", NULL, context->ParentWindow); // HACK

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->ListViewHandle);
                return TRUE;
            }

            REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();

                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
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
