/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2018-2025
 *
 */

#include "exttools.h"
#include <subprocesstag.h>

typedef struct _MODULE_SERVICES_CONTEXT
{
    HWND ParentWindowHandle;
    HWND ServiceListHandle;
    HANDLE ProcessId;
    PPH_STRING ModuleName;
    PH_LAYOUT_MANAGER LayoutManager;
    ULONG ServiceItemCount;
    PPH_SERVICE_ITEM* ServiceItems;
} MODULE_SERVICES_CONTEXT, *PMODULE_SERVICES_CONTEXT;

_Success_(return)
ULONG PhQueryModuleServiceReferences(
    _In_ PMODULE_SERVICES_CONTEXT Context,
    _Out_ PPH_LIST* ServiceList
    );

INT_PTR CALLBACK EtpModuleServicesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtpModuleServicesDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    PMODULE_SERVICES_CONTEXT context = (PMODULE_SERVICES_CONTEXT)Parameter;
    BOOL result;
    MSG message;
    HWND windowHandle;
    ULONG status;
    PPH_LIST serviceList;
    PH_AUTO_POOL autoPool;

    status = PhQueryModuleServiceReferences(context, &serviceList);

    if (status != ERROR_SUCCESS)
    {
        PhShowStatus(context->ParentWindowHandle, L"Unable to query module references.", 0, status);
        return STATUS_SUCCESS;
    }

    if (serviceList->Count == 0)
    {
        PhShowInformation2(context->ParentWindowHandle, L"Unable to query module references.", L"%s", L"This module was not referenced by a service.");
        PhDereferenceObject(serviceList);
        PhFree(context);
        return STATUS_SUCCESS;
    }
    else
    {
        context->ServiceItemCount = serviceList->Count;
        context->ServiceItems = PhAllocateCopy(
            serviceList->Items,
            serviceList->Count * sizeof(PPH_SERVICE_ITEM)
            );
        PhDereferenceObject(serviceList);
    }

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_MODSERVICES),
        NULL,
        EtpModuleServicesDlgProc,
        Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID EtShowModuleServicesDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING ModuleName
    )
{
    PMODULE_SERVICES_CONTEXT context;

    context = PhAllocateZero(sizeof(MODULE_SERVICES_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;
    context->ProcessId = ProcessId;
    context->ModuleName = PhReferenceObject(ModuleName);

    PhCreateThread2(EtpModuleServicesDialogThreadStart, context);
}

_Success_(return)
ULONG PhQueryModuleServiceReferences(
    _In_ PMODULE_SERVICES_CONTEXT Context,
    _Out_ PPH_LIST *ServiceList
    )
{
    static PQUERY_TAG_INFORMATION I_QueryTagInformation = NULL;
    ULONG win32Result;
    TAG_INFO_NAMES_REFERENCING_MODULE namesReferencingModule;
    PPH_LIST serviceList;

    if (!I_QueryTagInformation)
    {
        if (!(I_QueryTagInformation = PhGetModuleProcAddress(L"sechost.dll", "I_QueryTagInformation")))
        {
            if (!(I_QueryTagInformation = PhGetModuleProcAddress(L"advapi32.dll", "I_QueryTagInformation")))
            {
                return ERROR_PROC_NOT_FOUND;
            }
        }
    }

    if (!I_QueryTagInformation)
        return ERROR_PROC_NOT_FOUND;

    memset(&namesReferencingModule, 0, sizeof(TAG_INFO_NAMES_REFERENCING_MODULE));
    namesReferencingModule.InParams.ProcessId = HandleToUlong(Context->ProcessId);
    namesReferencingModule.InParams.ModuleName = PhGetString(Context->ModuleName);

    win32Result = I_QueryTagInformation(NULL, eTagInfoLevelNamesReferencingModule, &namesReferencingModule);

    if (win32Result == ERROR_NO_MORE_ITEMS)
        win32Result = ERROR_SUCCESS;

    if (win32Result != ERROR_SUCCESS)
        return win32Result;

    serviceList = PhCreateList(16);

    if (namesReferencingModule.OutParams.Names)
    {
        PCWSTR serviceName;
        PPH_SERVICE_ITEM serviceItem;

        for (serviceName = namesReferencingModule.OutParams.Names; *serviceName; serviceName += PhCountStringZ(serviceName) + 1)
        {
            if (serviceItem = PhReferenceServiceItemZ(serviceName))
                PhAddItemList(serviceList, serviceItem);
        }

        LocalFree((HLOCAL)namesReferencingModule.OutParams.Names);
    }

    *ServiceList = serviceList;
    return ERROR_SUCCESS;
}

INT_PTR CALLBACK EtpModuleServicesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PMODULE_SERVICES_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PMODULE_SERVICES_CONTEXT)lParam;
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
            RECT rect;

            PhSetApplicationWindowIcon(hwndDlg);

            context->ServiceListHandle = PhCreateServiceListControl(hwndDlg, context->ServiceItems, context->ServiceItemCount);
            SendMessage(context->ServiceListHandle, WM_PH_SET_LIST_VIEW_SETTINGS, 0, (LPARAM)SETTING_NAME_MODULE_SERVICES_COLUMNS);

            {
                PPH_PROCESS_ITEM processItem;
                PPH_STRING message;

                if (processItem = PhReferenceProcessItem(context->ProcessId))
                {
                    message = PhFormatString(
                        L"Services referencing %s in %s (%lu):",
                        PhGetString(context->ModuleName),
                        PhGetStringOrEmpty(processItem->ProcessName),
                        HandleToUlong(processItem->ProcessId)
                        );
                    PhDereferenceObject(processItem);
                }
                else
                {
                    message = PhFormatString(
                        L"Services referencing %s:",
                        PhGetString(context->ModuleName)
                        );
                }

                PhSetDialogItemText(hwndDlg, IDC_MESSAGE, PhGetString(message));
                PhDereferenceObject(message);
            }

            // Position the control.
            if (PhGetWindowRect(GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), &rect))
            {
                MapWindowRect(NULL, hwndDlg, &rect);
                MoveWindow(context->ServiceListHandle, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);
                ShowWindow(context->ServiceListHandle, SW_SHOW);
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->ServiceListHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_MODULE_SERVICES_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_MODULE_SERVICES_WINDOW_POSITION, SETTING_NAME_MODULE_SERVICES_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_MODULE_SERVICES_WINDOW_POSITION, SETTING_NAME_MODULE_SERVICES_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhDereferenceObjects(context->ServiceItems, context->ServiceItemCount);

            PhDereferenceObject(context->ModuleName);

            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    DestroyWindow(hwndDlg);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
