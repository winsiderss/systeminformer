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

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_HANDLE_ITEM HandleItem;
    HANDLE ProcessId;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

HPROPSHEETPAGE EtpCommonCreatePage(
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

        if (PhEqualString2(context->HandleItem->TypeName, L"TpWorkerFactory", TRUE))
        {
            page = EtpCommonCreatePage(
                context,
                MAKEINTRESOURCE(IDD_OBJTPWORKERFACTORY),
                EtpTpWorkerFactoryPageDlgProc
                );
        }

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
    _In_opt_ PVOID Context
    )
{
    if (!Context)
        return TRUE;

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

                    if (symbolProvider = PhCreateSymbolProvider(basicInfo.ProcessId))
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
            }
        }
        break;
    }

    return FALSE;
}
