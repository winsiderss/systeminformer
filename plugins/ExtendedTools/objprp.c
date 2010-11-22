/*
 * Process Hacker Extended Tools - 
 *   handle properties extensions
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

#include "exttools.h"
#include "resource.h"

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_HANDLE_ITEM HandleItem;
    HANDLE ProcessId;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

HPROPSHEETPAGE EtpCommonCreatePage(
    __in PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT Context,
    __in PWSTR Template,
    __in DLGPROC DlgProc
    );

INT CALLBACK EtpCommonPropPageProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK EtpAlpcPortPageDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK EtpTpWorkerFactoryPageDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtHandlePropertiesInitializing(
    __in PVOID Parameter
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT context = objectProperties->Parameter;

    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        HPROPSHEETPAGE page = NULL;

        if (PhEqualString2(context->HandleItem->TypeName, L"ALPC Port", TRUE))
        {
            page = EtpCommonCreatePage(
                context,
                MAKEINTRESOURCE(IDD_OBJALPCPORT),
                EtpAlpcPortPageDlgProc
                );
        }
        else if (PhEqualString2(context->HandleItem->TypeName, L"TpWorkerFactory", TRUE))
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
    __in PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT Context,
    __in PWSTR Template,
    __in DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PCOMMON_PAGE_CONTEXT pageContext;

    if (!NT_SUCCESS(PhCreateAlloc(&pageContext, sizeof(COMMON_PAGE_CONTEXT))))
        return NULL;

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
    __in HWND hwnd,
    __in UINT uMsg,
    __in LPPROPSHEETPAGE ppsp
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
    __out PHANDLE Handle,
    __in ACCESS_MASK DesiredAccess,
    __in PCOMMON_PAGE_CONTEXT Context
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

    status = PhDuplicateObject(
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

INT_PTR CALLBACK EtpAlpcPortPageDlgProc(
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
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PCOMMON_PAGE_CONTEXT context = (PCOMMON_PAGE_CONTEXT)propSheetPage->lParam;
            HANDLE portHandle;

            if (NT_SUCCESS(EtpDuplicateHandleFromProcess(&portHandle, READ_CONTROL, context)))
            {
                ALPC_PORT_BASIC_INFORMATION basicInfo;

                if (NT_SUCCESS(NtAlpcQueryInformation(
                    portHandle,
                    AlpcPortBasicInformation,
                    &basicInfo,
                    sizeof(ALPC_PORT_BASIC_INFORMATION),
                    NULL
                    )))
                {
                    PH_FORMAT format[2];
                    PPH_STRING string;

                    PhInitFormatS(&format[0], L"Sequence Number: ");
                    PhInitFormatD(&format[1], basicInfo.SequenceNo);
                    format[1].Type |= FormatGroupDigits;

                    string = PhFormat(format, 2, 128);
                    SetDlgItemText(hwndDlg, IDC_SEQUENCENUMBER, string->Buffer);
                    PhDereferenceObject(string);

                    SetDlgItemText(hwndDlg, IDC_PORTCONTEXT,
                        PhaFormatString(L"Port Context: 0x%Ix", basicInfo.PortContext)->Buffer);
                }

                NtClose(portHandle);
            }
        }
        break;
    }

    return FALSE;
}

static BOOLEAN NTAPI EnumGenericModulesCallback(
    __in PPH_MODULE_INFO Module,
    __in_opt PVOID Context
    )
{
    if (Module->Type == PH_MODULE_TYPE_MODULE || Module->Type == PH_MODULE_TYPE_WOW64_MODULE)
    {
        PhLoadModuleSymbolProvider(Context, Module->FileName->Buffer,
            (ULONG64)Module->BaseAddress, Module->Size);
    }

    return TRUE;
}

INT_PTR CALLBACK EtpTpWorkerFactoryPageDlgProc(
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

                    symbolProvider = PhCreateSymbolProvider(basicInfo.UniqueProcessId);
                    PhLoadSymbolProviderOptions(symbolProvider);

                    if (symbolProvider->IsRealHandle)
                    {
                        PhEnumGenericModules(basicInfo.UniqueProcessId, symbolProvider->ProcessHandle,
                            0, EnumGenericModulesCallback, symbolProvider);

                        symbol = PhGetSymbolFromAddress(symbolProvider, (ULONG64)basicInfo.WorkerThreadStart,
                            NULL, NULL, NULL, NULL);
                    }

                    PhDereferenceObject(symbolProvider);

                    if (symbol)
                    {
                        SetDlgItemText(hwndDlg, IDC_WORKERTHREADSTART,
                            PhaFormatString(L"Worker Thread Start: %s", symbol->Buffer)->Buffer);
                        PhDereferenceObject(symbol);
                    }
                    else
                    {
                        SetDlgItemText(hwndDlg, IDC_WORKERTHREADSTART,
                            PhaFormatString(L"Worker Thread Start: 0x%Ix", basicInfo.WorkerThreadStart)->Buffer);
                    }

                    SetDlgItemText(hwndDlg, IDC_WORKERTHREADCONTEXT,
                        PhaFormatString(L"Worker Thread Context: 0x%Ix", basicInfo.WorkerThreadContext)->Buffer);
                }

                NtClose(workerFactoryHandle);
            }
        }
        break;
    }

    return FALSE;
}
