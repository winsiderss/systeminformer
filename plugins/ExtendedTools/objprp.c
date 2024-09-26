/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
 */

#include "exttools.h"
#include <symprv.h>
#include <secedit.h>
#include <hndlinfo.h>
#include <kphuser.h>

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_HANDLE_ITEM HandleItem;
    HANDLE ProcessId;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

typedef struct _HANDLES_PAGE_CONTEXT
{
    PPH_HANDLE_ITEM HandleItem;
    HWND WindowHandle;
    HWND ListViewHandle;
} HANDLES_PAGE_CONTEXT, * PHANDLES_PAGE_CONTEXT;

typedef struct _ET_HANDLE_ENTRY
{
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
    COLORREF Color;
} HANDLE_ENTRY, * PHANDLE_ENTRY;

HPROPSHEETPAGE EtpCommonCreatePage(
    _In_ PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    );

HPROPSHEETPAGE EtpCreateHandlePage(
    _In_ PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    );

INT CALLBACK EtpCommonPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK EtpTpWorkerFactoryPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EtpObjHandlesPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PPH_STRING EtGetAccessString(
    _In_ PPH_STRING TypeName,
    _In_ ACCESS_MASK access
    );

extern HWND EtObjectManagerDialogHandle;
extern LARGE_INTEGER EtObjectManagerTimeCached;
extern PPH_LIST EtObjectManagerOwnHandles;
extern HICON EtObjectManagerPropWndIcon;

VOID EtHandlePropertiesInitializing(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT context = objectProperties->Parameter;

    if (PhIsNullOrEmptyString(context->HandleItem->TypeName))
        return;

    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        HPROPSHEETPAGE page = NULL;

        // Object Manager
        if (EtObjectManagerDialogHandle &&
            PhEqualStringRef2(&context->OwnerPluginName, PLUGIN_NAME, TRUE))
        {
            page = EtpCreateHandlePage(
                context,
                MAKEINTRESOURCE(IDD_OBJHANDLES),
                EtpObjHandlesPageDlgProc
            );

            // Insert our page into the second slot.
            if (page)
            {
                if (objectProperties->NumberOfPages > 1)
                {
                    memmove(&objectProperties->Pages[2], &objectProperties->Pages[1],
                        (objectProperties->NumberOfPages - 1) * sizeof(HPROPSHEETPAGE));
                }

                objectProperties->Pages[1] = page;
                objectProperties->NumberOfPages++;
            }
        }
        // TpWorkerFactory unnamed, so it won't interact with Object Manager
        else if (PhEqualString2(context->HandleItem->TypeName, L"TpWorkerFactory", TRUE))
        {
            page = EtpCommonCreatePage(
                context,
                MAKEINTRESOURCE(IDD_OBJTPWORKERFACTORY),
                EtpTpWorkerFactoryPageDlgProc
            );

            // Insert our page into the second slot.
            if (page)
            {
                if (objectProperties->NumberOfPages > 1)
                {
                    memmove(&objectProperties->Pages[2], &objectProperties->Pages[1],
                        (objectProperties->NumberOfPages - 1) * sizeof(HPROPSHEETPAGE));
                }

                objectProperties->Pages[1] = page;
                objectProperties->NumberOfPages++;
            }
        }
    }
}

typedef enum _ET_OBJECT_POOLTYPE {
    PagedPool = 1,
    NonPagedPool = 0,
    NonPagedPoolNx = 0x200,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32,
    PagedPoolSessionNx = NonPagedPoolNx + 33
} ET_OBJECT_POOLTYPE;

VOID EtHandlePropertiesWindowPreOpen(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PHANDLE_PROPERTIES_CONTEXT context = objectProperties->Parameter;

    if (EtObjectManagerDialogHandle &&
        PhEqualStringRef2(&context->OwnerPluginName, PLUGIN_NAME, TRUE))
    {
        INT index;

        // HACK
        if (PhGetIntegerPairSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION).X != 0)
            PhLoadWindowPlacementFromSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION, NULL, context->ParentWindow);
        else
            PhCenterWindow(context->ParentWindow, GetParent(context->ParentWindow)); // HACK

        PhSetWindowIcon(context->ParentWindow, EtObjectManagerPropWndIcon, NULL, 0);

        // Show real handles count
        WCHAR string[PH_INT64_STR_LEN_1];
        PPH_STRING count = PH_AUTO(PhGetListViewItemText(context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_HANDLES, 1));
        ULONG real_count = wcstoul(count->Buffer, NULL, 10);

        if (real_count > 0) {
            PhPrintUInt32(string, real_count - 1);
            PhSetListViewSubItem(context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_HANDLES, 1, string);
        }

        PhRemoveListViewItem(context->ListViewHandle, PH_HANDLE_GENERAL_INDEX_ACCESSMASK);

        index = PhAddListViewGroupItem(
            context->ListViewHandle,
            PH_HANDLE_GENERAL_CATEGORY_BASICINFO,
            PH_HANDLE_GENERAL_INDEX_OBJECT + 1,
            L"Object attributes",
            NULL
        );

        // Show object attributes
        if (context->HandleItem->Attributes & OBJ_PERMANENT || context->HandleItem->Attributes & OBJ_EXCLUSIVE)
        {
            PPH_STRING attributes = NULL;
            
            switch (context->HandleItem->Attributes & (OBJ_PERMANENT | OBJ_EXCLUSIVE))
            {
            case OBJ_PERMANENT:
                attributes = PH_AUTO(PhCreateString(L"Permanent"));
                break;
            case OBJ_EXCLUSIVE:
                attributes = PH_AUTO(PhCreateString(L"Exclusive"));
                break;
            case OBJ_PERMANENT | OBJ_EXCLUSIVE:
                attributes = PH_AUTO(PhCreateString(L"Permanent, Exclusive"));
                break;
            }

            PhSetListViewSubItem(context->ListViewHandle, index, 1, PhGetString(attributes));
        }

        // Show creation time
        if (EtObjectManagerTimeCached.QuadPart != 0)
        {
            PPH_STRING startTimeString;
            SYSTEMTIME startTimeFields;

            index = PhAddListViewGroupItem(
                context->ListViewHandle,
                PH_HANDLE_GENERAL_CATEGORY_BASICINFO,
                PH_HANDLE_GENERAL_INDEX_OBJECT + 2,
                L"Creation time",
                NULL
            );

            PhLargeIntegerToLocalSystemTime(&startTimeFields, &EtObjectManagerTimeCached);
            startTimeString = PhaFormatDateTime(&startTimeFields);

            PhSetListViewSubItem(context->ListViewHandle, index, 1, startTimeString->Buffer);
        }

        // Show Device drivers information
        if (PhEqualString2(context->HandleItem->TypeName, L"Device", TRUE))
        {
            HANDLE objectHandle;
            HANDLE DriverHandle;
            PPH_STRING driverName;

            PhAddListViewGroup(context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_FILE, L"Device driver information");

            PhAddListViewGroupItem(context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_FILE, 1, L"Lower-edge driver", NULL);

            PhAddListViewGroupItem(context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_FILE, 2, L"Lower-edge driver image", NULL);

            PhAddListViewGroupItem(context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_FILE, 3, L"Upper-edge driver", NULL);

            PhAddListViewGroupItem(context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_FILE, 4, L"Upper-edge driver Image", NULL);

            if (NT_SUCCESS(PhOpenDevice(&objectHandle, &DriverHandle, READ_CONTROL, &context->HandleItem->BestObjectName->sr, TRUE)))
            {
                if (NT_SUCCESS(PhGetDriverName(DriverHandle, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, 1, 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                if (NT_SUCCESS(PhGetDriverImageFileName(DriverHandle, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, 2, 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                NtClose(DriverHandle);
                NtClose(objectHandle);
            }

            if (NT_SUCCESS(PhOpenDevice(&objectHandle, &DriverHandle, READ_CONTROL, &context->HandleItem->BestObjectName->sr, FALSE)))
            {
                if (NT_SUCCESS(PhGetDriverName(DriverHandle, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, 3, 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                if (NT_SUCCESS(PhGetDriverImageFileName(DriverHandle, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, 4, 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                NtClose(DriverHandle);
                NtClose(objectHandle);
            }
        }
        // Show Driver image information
        else if (PhEqualString2(context->HandleItem->TypeName, L"Driver", TRUE))
        {
            PPH_STRING driverName;

            PhAddListViewGroup(context->ListViewHandle, PH_HANDLE_GENERAL_CATEGORY_FILE, L"Driver information");

            index = PhAddListViewGroupItem(
                context->ListViewHandle,
                PH_HANDLE_GENERAL_CATEGORY_FILE,
                PH_HANDLE_GENERAL_INDEX_FILEDRIVERIMAGE,
                L"Driver Image",
                NULL
            );

            if (NT_SUCCESS(PhGetDriverImageFileName(context->HandleItem->Handle, &driverName)))
            {
                PhSetListViewSubItem(context->ListViewHandle, index, 1, PhGetString(driverName));
                PhDereferenceObject(driverName);
            }
        }
        else if (PhEqualString2(context->HandleItem->TypeName, L"Type", TRUE))
        {
            POBJECT_TYPES_INFORMATION objectTypes;
            POBJECT_TYPE_INFORMATION objectType;
            ULONG typeIndex;
            PPH_STRING accessString;
            PH_STRING_BUILDER stringBuilder;

            if (NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
            {
                typeIndex = PhGetObjectTypeNumber(&context->HandleItem->ObjectName->sr);
                objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

                for (ULONG i = 0; i < objectTypes->NumberOfTypes; i++)
                {
                    if (objectType->TypeIndex == typeIndex)
                    {
                        index = PH_HANDLE_GENERAL_CATEGORY_QUOTA;

                        ListView_RemoveGroup(context->ListViewHandle, index);

                        PhAddListViewGroup(context->ListViewHandle, index, L"Type information");

                        PhAddListViewGroupItem(context->ListViewHandle, index, 1, L"Index", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 2, L"Objects", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 3, L"Handles", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 4, L"Peak Objects", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 5, L"Peak Handles", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 6, L"Pool Type", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 7, L"Default Paged Charge", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 8, L"Default NP Charge", NULL);

                        PhPrintUInt32(string, objectType->TypeIndex);
                        PhSetListViewSubItem(context->ListViewHandle, 1, 1, string);
                        PhPrintUInt32(string, objectType->TotalNumberOfObjects);
                        PhSetListViewSubItem(context->ListViewHandle, 2, 1, string);
                        PhPrintUInt32(string, objectType->TotalNumberOfHandles);
                        PhSetListViewSubItem(context->ListViewHandle, 3, 1, string);
                        PhPrintUInt32(string, objectType->HighWaterNumberOfObjects);
                        PhSetListViewSubItem(context->ListViewHandle, 4, 1, string);
                        PhPrintUInt32(string, objectType->HighWaterNumberOfHandles);
                        PhSetListViewSubItem(context->ListViewHandle, 5, 1, string);
                        PhPrintUInt32(string, objectType->DefaultPagedPoolCharge);

                        switch (objectType->PoolType) {
                        case NonPagedPool:
                            PhSetListViewSubItem(context->ListViewHandle, 6, 1, L"Non Paged");
                            break;
                        case PagedPool:
                            PhSetListViewSubItem(context->ListViewHandle, 6, 1, L"Paged");
                            break;
                        case NonPagedPoolNx:
                            PhSetListViewSubItem(context->ListViewHandle, 6, 1, L"Non Paged NX");
                            break;
                        case PagedPoolSessionNx:
                            PhSetListViewSubItem(context->ListViewHandle, 6, 1, L"Paged Session NX");
                            break;
                        }

                        PhSetListViewSubItem(context->ListViewHandle, 7, 1, string);
                        PhPrintUInt32(string, objectType->DefaultNonPagedPoolCharge);
                        PhSetListViewSubItem(context->ListViewHandle, 8, 1, string);

                        index++;
                        PhAddListViewGroup(context->ListViewHandle, index, L"Type access information");

                        PhAddListViewGroupItem(context->ListViewHandle, index, 1, L"Valid Access Mask", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 2, L"Generic Read", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 3, L"Generic Write", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 4, L"Generic Execute", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 5, L"Generic All", NULL);
                        PhAddListViewGroupItem(context->ListViewHandle, index, 6, L"Invalid Attributes", NULL);

                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->ValidAccessMask));
                        PhSetListViewSubItem(context->ListViewHandle, 1, 1, PhGetString(accessString));
                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->GenericMapping.GenericRead));
                        PhSetListViewSubItem(context->ListViewHandle, 2, 1, PhGetString(accessString));
                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->GenericMapping.GenericWrite));
                        PhSetListViewSubItem(context->ListViewHandle, 3, 1, PhGetString(accessString));
                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->GenericMapping.GenericExecute));
                        PhSetListViewSubItem(context->ListViewHandle, 4, 1, PhGetString(accessString));
                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->GenericMapping.GenericAll));
                        PhSetListViewSubItem(context->ListViewHandle, 5, 1, PhGetString(accessString));

                        PhInitializeStringBuilder(&stringBuilder, 10);
                        if (objectType->InvalidAttributes & OBJ_KERNEL_HANDLE)
                            PhAppendStringBuilder2(&stringBuilder, L"KERNEL_HANDLE, ");
                        if (objectType->InvalidAttributes & OBJ_OPENLINK)
                            PhAppendStringBuilder2(&stringBuilder, L"OPEN_LINK, ");
                        if (objectType->InvalidAttributes & OBJ_OPENIF)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_OPENIF, ");
                        if (objectType->InvalidAttributes & OBJ_PERMANENT)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_PERMANENT, ");
                        if (objectType->InvalidAttributes & OBJ_EXCLUSIVE)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_EXCLUSIVE, ");
                        if (objectType->InvalidAttributes & OBJ_INHERIT)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_INHERIT, ");
                        if (objectType->InvalidAttributes & OBJ_CASE_INSENSITIVE)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_CASE_INSENSITIVE, ");
                        if (objectType->InvalidAttributes & OBJ_FORCE_ACCESS_CHECK)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_FORCE_ACCESS_CHECK, ");
                        if (objectType->InvalidAttributes & OBJ_IGNORE_IMPERSONATED_DEVICEMAP)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_IGNORE_IMPERSONATED_DEVICEMAP, ");
                        if (objectType->InvalidAttributes & OBJ_DONT_REPARSE)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_DONT_REPARSE, ");

                        // Remove the trailing ", ".
                        if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                            PhRemoveEndStringBuilder(&stringBuilder, 2);

                        accessString = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
                        PhSetListViewSubItem(context->ListViewHandle, 6, 1, PhGetString(accessString));

                        break;
                    }

                    objectType = PH_NEXT_OBJECT_TYPE(objectType);
                }

                PhFree(objectTypes);
            }
        }

        // Remove irrelevant information if we couldn't open real object
        if (!context->HandleItem->Object)
        {
            if (PhEqualString2(context->HandleItem->TypeName, L"ALPC Port", TRUE))
            {
                PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_FLAGS], 1, NULL);
                PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_SEQUENCENUMBER], 1, NULL);
                PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_PORTCONTEXT], 1, NULL);
                PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ALPCSERVER], 1, NULL);
                PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_HANDLE_GENERAL_INDEX_ALPCCLIENT], 1, NULL);
            }
        }
    }
}

VOID EtHandlePropertiesWindowUninitializing(
    _In_ PVOID Parameter
)
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PHANDLE_PROPERTIES_CONTEXT context = objectProperties->Parameter;

    if (context->HandleItem->Handle)
    {
        NtClose(context->HandleItem->Handle);

        PhRemoveItemList(EtObjectManagerOwnHandles, PhFindItemList(EtObjectManagerOwnHandles, context->HandleItem->Handle));
        PhDereferenceObject(EtObjectManagerOwnHandles);
    }

    PhSaveWindowPlacementToSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION, NULL, context->ParentWindow);

    PhDereferenceObject(context->HandleItem);
}

static HPROPSHEETPAGE EtpCommonCreatePage(
    _In_ PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhCreateAlloc(sizeof(COMMON_PAGE_CONTEXT));
    memset(pageContext, 0, sizeof(COMMON_PAGE_CONTEXT));
    pageContext->HandleItem = Context->HandleItem;
    pageContext->ProcessId = Context->ProcessId;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = Template;
    propSheetPage.pfnDlgProc = DlgProc;
    propSheetPage.lParam = (LPARAM)pageContext;
    propSheetPage.pfnCallback = EtpCommonPropPageProc;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);
    PhDereferenceObject(pageContext); // already got a ref from above call

    return propSheetPageHandle;
}

INT CALLBACK EtpCommonPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = (PCOMMON_PAGE_CONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
        PhReferenceObject(pageContext);
    else if (uMsg == PSPCB_RELEASE)
        PhDereferenceObject(pageContext);

    return 1;
}

static HPROPSHEETPAGE EtpCreateHandlePage(
    _In_ PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PHANDLES_PAGE_CONTEXT handlesPageContext;

    handlesPageContext = PhCreateAlloc(sizeof(HANDLES_PAGE_CONTEXT));
    memset(handlesPageContext, 0, sizeof(HANDLES_PAGE_CONTEXT));
    handlesPageContext->HandleItem = Context->HandleItem;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = Template;
    propSheetPage.pfnDlgProc = DlgProc;
    propSheetPage.lParam = (LPARAM)handlesPageContext;
    propSheetPage.pfnCallback = EtpCommonPropPageProc;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);
    PhDereferenceObject(handlesPageContext); // already got a ref from above call

    return propSheetPageHandle;
}

static NTSTATUS EtpDuplicateHandleFromProcess(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOMMON_PAGE_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        Context->ProcessId
        )))
        return status;

    status = NtDuplicateObject(
        processHandle,
        Context->HandleItem->Handle,
        NtCurrentProcess(),
        Handle,
        DesiredAccess,
        0,
        0
        );
    NtClose(processHandle);

    return status;
}

static BOOLEAN NTAPI EnumGenericModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    if (
        Module->Type == PH_MODULE_TYPE_MODULE ||
        Module->Type == PH_MODULE_TYPE_WOW64_MODULE
        )
    {
        PhLoadModuleSymbolProvider(
            Context,
            Module->FileName,
            (ULONG64)Module->BaseAddress,
            Module->Size
            );
    }

    return TRUE;
}

INT_PTR CALLBACK EtpTpWorkerFactoryPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PCOMMON_PAGE_CONTEXT context = (PCOMMON_PAGE_CONTEXT)propSheetPage->lParam;
            HANDLE workerFactoryHandle;

            if (NT_SUCCESS(EtpDuplicateHandleFromProcess(&workerFactoryHandle, WORKER_FACTORY_QUERY_INFORMATION, context)))
            {
                WORKER_FACTORY_BASIC_INFORMATION basicInfo;

                if (NT_SUCCESS(NtQueryInformationWorkerFactory(
                    workerFactoryHandle,
                    WorkerFactoryBasicInformation,
                    &basicInfo,
                    sizeof(WORKER_FACTORY_BASIC_INFORMATION),
                    NULL
                    )))
                {
                    PPH_SYMBOL_PROVIDER symbolProvider;
                    PPH_STRING symbol = NULL;
                    WCHAR value[PH_PTR_STR_LEN_1];

                    if (symbolProvider = PhCreateSymbolProvider(NULL))
                    {
                        PhLoadSymbolProviderOptions(symbolProvider);

                        PhEnumGenericModules(
                            basicInfo.ProcessId,
                            NULL,
                            0,
                            EnumGenericModulesCallback,
                            symbolProvider
                            );

                        symbol = PhGetSymbolFromAddress(
                            symbolProvider,
                            (ULONG64)basicInfo.StartRoutine,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                            );

                        PhDereferenceObject(symbolProvider);
                    }

                    if (symbol)
                    {
                        PhSetDialogItemText(
                            hwndDlg,
                            IDC_WORKERTHREADSTART,
                            PhaFormatString(L"Worker Thread Start: %s", symbol->Buffer)->Buffer
                            );
                        PhDereferenceObject(symbol);
                    }
                    else
                    {
                        PhPrintPointer(value, basicInfo.StartRoutine);
                        PhSetDialogItemText(
                            hwndDlg,
                            IDC_WORKERTHREADSTART,
                            PhaFormatString(L"Worker Thread Start: %s", value)->Buffer
                            );
                    }

                    PhPrintPointer(value, basicInfo.StartParameter);
                    PhSetDialogItemText(
                        hwndDlg,
                        IDC_WORKERTHREADCONTEXT,
                        PhaFormatString(L"Worker Thread Context: %s", value)->Buffer
                        );
                }

                NtClose(workerFactoryHandle);

                PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    }

    return FALSE;
}

FORCEINLINE PVOID EtpGenericPropertyPageHeader(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ ULONG ContextHash
    )
{
    PVOID context;

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;

        context = (PVOID)propSheetPage->lParam;
        PhSetWindowContext(hwndDlg, ContextHash, context);
    }
    break;
    case WM_NCDESTROY:
    {
        context = PhGetWindowContext(hwndDlg, ContextHash);
        PhRemoveWindowContext(hwndDlg, ContextHash);
    }
    break;
    default:
    {
        context = PhGetWindowContext(hwndDlg, ContextHash);
    }
    break;
    }

    return context;
}

PPH_STRING EtGetAccessString(
    _In_ PPH_STRING TypeName,
    _In_ ACCESS_MASK access
    )
{
    PPH_STRING AccessString = NULL;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    if (PhGetAccessEntries(
        PhGetStringOrEmpty(TypeName),
        &accessEntries,
        &numberOfAccessEntries
    ))
    {
        PPH_STRING accessString;

        accessString = PH_AUTO(PhGetAccessString(
            access,
            accessEntries,
            numberOfAccessEntries
        ));

        if (accessString->Length != 0)
        {
            AccessString = PhFormatString(
                L"0x%x (%s)",
                access,
                accessString->Buffer
            );
        }
        else
        {
            AccessString = PhFormatString(L"0x%x", access);
        }

        PhFree(accessEntries);
    }
    else
    {
        AccessString = PhFormatString(L"0x%x", access);
    }

    return AccessString;
}

VOID EtpEnumObjectHandles(
    _In_ PHANDLES_PAGE_CONTEXT context
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static ULONG FileTypeIndex;

    if (PhBeginInitOnce(&initOnce))
    {
        FileTypeIndex = PhGetObjectTypeNumberZ(L"File");
        PhEndInitOnce(&initOnce);
    }

    COLORREF ColorOwnObject = PhGetIntegerSetting(L"ColorOwnProcesses");
    COLORREF ColorInherit = PhGetIntegerSetting(L"ColorInheritHandles");
    COLORREF ColorProtected = PhGetIntegerSetting(L"ColorProtectedHandles");
    COLORREF ColorProtectedInherit = PhGetIntegerSetting(L"ColorPartiallySuspended");
    COLORREF ColorNormal = !!PhGetIntegerSetting(L"EnableThemeSupport") ? RGB(43, 43, 43) : GetSysColor(COLOR_WINDOW);
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    ULONG i;
    BOOLEAN useKsi = KsiLevel() >= KphLevelMed;
    
    BOOLEAN isDevice = PhEqualString2(context->HandleItem->TypeName, L"Device", TRUE);
    BOOLEAN isAlpcPort = PhEqualString2(context->HandleItem->TypeName, L"ALPC Port", TRUE);
    BOOLEAN isRegKey = PhEqualString2(context->HandleItem->TypeName, L"Key", TRUE);

    ULONG SourceTypeIndex = isDevice ? FileTypeIndex : context->HandleItem->TypeIndex;  // HACK

    if (NT_SUCCESS(PhEnumHandlesEx(&handles)))
    {
        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &handles->Handles[i];
            INT lvItemIndex;
            WCHAR value[PH_INT64_STR_LEN_1];
            BOOLEAN ObjectNameMath;
            HANDLE dupHandle;
            PPH_STRING bestObjectName;
            PHANDLE_ENTRY entry;
            PPH_STRING processName;
            PPH_STRING accessString;
            CLIENT_ID ClientId = { 0 };

            // Skip other types
            if (handleInfo->ObjectTypeIndex != SourceTypeIndex)
                continue;

            ObjectNameMath = FALSE;

            // Lookup for matches in object name to find more handles for ALPC Port, File, Key
            if (isAlpcPort)
            {
                if (NT_SUCCESS(EtpDuplicateHandleFromProcessEx(&dupHandle, READ_CONTROL,
                    (HANDLE)handleInfo->UniqueProcessId, (HANDLE)handleInfo->HandleValue)))
                {
                    if (NT_SUCCESS(PhGetHandleInformation(NtCurrentProcess(), dupHandle,
                        handleInfo->ObjectTypeIndex, NULL, NULL, NULL, &bestObjectName)))
                    {
                        ObjectNameMath = PhFindStringInStringRef(&bestObjectName->sr, &context->HandleItem->ObjectName->sr, TRUE) != MAXULONG_PTR;
                        PhDereferenceObject(bestObjectName);
                    }
                    NtClose(dupHandle);
                }
            }
            // If we're dealing with a file handle we must take special precautions so we don't hang.
            else if (isDevice && useKsi || isRegKey)
            {
                if (NT_SUCCESS(EtpDuplicateHandleFromProcessEx(&dupHandle, READ_CONTROL,
                    (HANDLE)handleInfo->UniqueProcessId, (HANDLE)handleInfo->HandleValue)))
                {
                    if (NT_SUCCESS(PhGetHandleInformation(NtCurrentProcess(), dupHandle,
                        handleInfo->ObjectTypeIndex, NULL, NULL, NULL, &bestObjectName)))
                    {
                        ObjectNameMath = PhStartsWithString(bestObjectName, context->HandleItem->BestObjectName, TRUE);
                        PhDereferenceObject(bestObjectName);
                    }
                    NtClose(dupHandle);
                }
            }

            if (handleInfo->Object == context->HandleItem->Object || ObjectNameMath)
            {
                // Skip Object Manager own handles
                if ((HANDLE)handleInfo->UniqueProcessId == NtCurrentProcessId() &&
                    PhFindItemList(EtObjectManagerOwnHandles, (PVOID)handleInfo->HandleValue) != ULONG_MAX)
                {
                    continue;
                }

                entry = PhAllocateZero(sizeof(HANDLE_ENTRY));
                entry->ProcessId = (HANDLE)handleInfo->UniqueProcessId;
                entry->HandleItem = PhCreateHandleItem(handleInfo);
                entry->Color = ColorNormal;

                ClientId.UniqueProcess = entry->ProcessId;
                processName = PH_AUTO(PhGetClientIdName(&ClientId));
                lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, PhGetString(processName), entry);

                PhPrintPointer(value, (PVOID)handleInfo->HandleValue);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, value);

                accessString = PH_AUTO(EtGetAccessString(context->HandleItem->TypeName, handleInfo->GrantedAccess));
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, PhGetString(accessString));

                switch (handleInfo->HandleAttributes & (OBJ_PROTECT_CLOSE | OBJ_INHERIT))
                {
                case OBJ_PROTECT_CLOSE:
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, L"Protected");
                    entry->Color = ColorProtected;
                    break;
                case OBJ_INHERIT:
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, L"Inherit");
                    entry->Color = ColorInherit;
                    break;
                case OBJ_PROTECT_CLOSE | OBJ_INHERIT:
                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, L"Protected, Inherit");
                    entry->Color = ColorProtectedInherit;
                    break;
                }

                // Highlight own object handles
                if (handleInfo->Object == context->HandleItem->Object)
                    entry->Color = ColorOwnObject;
            }
        }

        PhFree(handles);
    }
}

VOID EtpShowHandleProperties(
    _In_ HWND hwndDlg,
    _In_ PHANDLE_ENTRY entry
    )
{
    HANDLE processHandle;

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
        entry->ProcessId
    )))
    {
        if (NT_SUCCESS(PhGetHandleInformation(
            processHandle,
            entry->HandleItem->Handle,
            entry->HandleItem->TypeIndex,
            NULL,
            &entry->HandleItem->TypeName,
            &entry->HandleItem->ObjectName,
            &entry->HandleItem->BestObjectName
        )))
        {
            PhShowHandlePropertiesPlugin(hwndDlg, entry->ProcessId, entry->HandleItem, NULL, NULL);
        }

        NtClose(processHandle);
    }
}
COLORREF NTAPI EtpColorItemColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
)
{
    PHANDLE_ENTRY entry = Param;

    return entry->Color;
}


INT_PTR CALLBACK EtpObjHandlesPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    PHANDLES_PAGE_CONTEXT context;

    context = EtpGenericPropertyPageHeader(hwndDlg, uMsg, wParam, lParam, 3);

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 118, L"Process");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"Handle");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 120, L"Access");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 60, L"Attributes");
            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetSort(context->ListViewHandle, 0, NoSortOrder);
            ExtendedListView_SetItemColorFunction(context->ListViewHandle, EtpColorItemColorFunction);

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

            PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));

            EtpEnumObjectHandles(context);

            PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
        }
        break;
    case WM_DESTROY:
        {
            INT index = INT_ERROR;

            while ((index = PhFindListViewItemByFlags(
                context->ListViewHandle,
                index,
                LVNI_ALL
            )) != INT_ERROR)
            {
                PHANDLE_ENTRY entry;
                if (PhGetListViewItemParam(context->ListViewHandle, index, &entry))
                {
                    PhClearReference(&entry->HandleItem);
                    PhFree(entry);
                } 
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);

            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
                case NM_DBLCLK:
                {
                    LPNMITEMACTIVATE info = (LPNMITEMACTIVATE)header;
                    PHANDLE_ENTRY entry;

                    if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                    {
                        EtpShowHandleProperties(hwndDlg, entry);
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            PHANDLE_ENTRY* listviewItems;
            ULONG numberOfItems;

            PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
            if (numberOfItems != 0)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU_ITEM item;
                PPH_EMENU_ITEM propMenuItem;
                PPH_EMENU_ITEM gotoMenuItem;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                menu = PhCreateEMenu();

                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_CLOSEHANDLE, L"C&lose", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, propMenuItem = PhCreateEMenuItem(0, IDC_PROPERTIES, L"Prope&rties", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTOPROCESS, L"&Go to process...", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);
                PhSetFlagsEMenuItem(menu, IDC_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);
                if (numberOfItems > 1)
                {
                    PhSetDisabledEMenuItem(propMenuItem);
                    PhSetDisabledEMenuItem(gotoMenuItem);
                }

                item = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    point.x,
                    point.y
                );

                if (item && item->Id != ULONG_MAX && !PhHandleCopyListViewEMenuItem(item))
                {
                    switch (item->Id)
                    {
                    case IDC_CLOSEHANDLE:
                    {
                        HANDLE oldHandle;

                        for (ULONG i = 0; i < numberOfItems; i++)
                        {
                            if (PhUiCloseHandles(hwndDlg, listviewItems[i]->ProcessId, &listviewItems[i]->HandleItem, 1, TRUE))
                            {
                                oldHandle = NULL;
                                if (EtpDuplicateHandleFromProcessEx(&oldHandle, 0, listviewItems[i]->ProcessId, listviewItems[i]->HandleItem->Handle)
                                    == STATUS_INVALID_HANDLE)   // HACK for protected handles
                                {
                                    PhRemoveListViewItem(context->ListViewHandle, PhFindListViewItemByParam(context->ListViewHandle, INT_ERROR, listviewItems[i]));
                                    PhClearReference(&listviewItems[i]->HandleItem);
                                    PhFree(listviewItems[i]);
                                }
                                else if (oldHandle)
                                    NtClose(oldHandle);
                            }
                            else
                                break; 
                        }
                    }
                    break;
                    case IDC_PROPERTIES:
                    {
                        EtpShowHandleProperties(hwndDlg, listviewItems[0]);
                    }
                    break;
                    case IDC_GOTOPROCESS:
                    {
                        PPH_PROCESS_ITEM processItem;

                        if (processItem = PhReferenceProcessItem(listviewItems[0]->ProcessId))
                        {
                            SystemInformer_ShowProcessProperties(processItem);
                            PhDereferenceObject(processItem);
                        }
                    }
                    break;
                    case IDC_COPY:
                    {
                        PhCopyListView(context->ListViewHandle);
                    }
                    break;
                    }

                    PhDestroyEMenu(menu);
                }
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
