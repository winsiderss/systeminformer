/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022-2023
 *
 */

#include "extsrv.h"

typedef struct _PACKAGE_SERVICE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PPH_SERVICE_ITEM ServiceItem;
    HIMAGELIST ImageList;
} PACKAGE_SERVICE_CONTEXT, *PPACKAGE_SERVICE_CONTEXT;

PPH_STRING EspGetServiceAppUserModelId(
    _In_ PPH_STRINGREF ServiceName
    )
{
    static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
    PPH_STRING serviceAppUserModelId = NULL;
    PPH_STRING serviceKeyName;
    HANDLE keyHandle;

    serviceKeyName = PhConcatStringRef2(&servicesKeyName, ServiceName);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &serviceKeyName->sr,
        0
        )))
    {
        serviceAppUserModelId = PhQueryRegistryStringZ(keyHandle, L"AppUserModelId");
        NtClose(keyHandle);
    }

    PhDereferenceObject(serviceKeyName);

    return serviceAppUserModelId;
}

PPH_STRING EspGetServicePackageFullName(
    _In_ PPH_STRINGREF ServiceName
    )
{
    static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
    PPH_STRING servicePackageName = NULL;
    PPH_STRING serviceKeyName;
    HANDLE keyHandle;

    serviceKeyName = PhConcatStringRef2(&servicesKeyName, ServiceName);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &serviceKeyName->sr,
        0
        )))
    {
        servicePackageName = PhQueryRegistryStringZ(keyHandle, L"PackageFullName");
        NtClose(keyHandle);
    }

    PhDereferenceObject(serviceKeyName);

    return servicePackageName;
}

// https://learn.microsoft.com/en-us/windows-hardware/drivers/develop/driver-isolation
// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-iogetdriverdirectory
typedef enum _PH_SERVICE_DIRECTORY_TYPE
{
    // The requested directory is a general-purpose directory in which the driver stores
    // files that contain state information specific to the internals of the driver
    // and only used by the driver itself.
    SERVICE_DIRECTORY_TYPE_DATA,
    // The requested directory is a general-purpose directory in which the driver stores
    // or reads files that contain state information that is meant to be shared between
    // the driver and other components. Other components can access this directory using GetSharedServiceDirectory.
    SERVICE_DIRECTORY_TYPE_SHAREDDATA,
} PH_SERVICE_DIRECTORY_TYPE;

// rev from sechost.dll!GetServiceDirectory (dmex)
PPH_STRING PhGetServiceDirectory(
    _In_ PPH_STRINGREF ServiceName
    )
{
    static PH_STRINGREF serviceStateName = PH_STRINGREF_INIT(L"\\ServiceState\\");
    PPH_STRING serviceDirectory;
    PH_STRINGREF systemRoot;

    PhGetSystemRoot(&systemRoot);
    serviceDirectory = PhConcatStringRef3(&systemRoot, &serviceStateName, ServiceName);

    return serviceDirectory;
}

// rev from sechost.dll!GetSharedServiceDirectory (dmex)
PPH_STRING PhGetServiceSharedDirectory(
    _In_ PPH_STRINGREF ServiceName,
    _In_ PH_SERVICE_DIRECTORY_TYPE DirectoryType
    )
{
    static PH_STRINGREF serviceStateName = PH_STRINGREF_INIT(L"\\ServiceState\\");
    static PH_STRINGREF serviceSharedName = PH_STRINGREF_INIT(L"\\SharedData");
    static PH_STRINGREF serviceDataName = PH_STRINGREF_INIT(L"\\Data");
    PPH_STRING serviceDirectory;
    PH_STRINGREF systemRoot;

    PhGetSystemRoot(&systemRoot);
    serviceDirectory = PhConcatStringRef3(&systemRoot, &serviceStateName, ServiceName);

    switch (DirectoryType)
    {
    case SERVICE_DIRECTORY_TYPE_DATA:
        PhMoveReference(&serviceDirectory, PhConcatStringRef2(&serviceDirectory->sr, &serviceDataName));
        break;
    case SERVICE_DIRECTORY_TYPE_SHAREDDATA:
        PhMoveReference(&serviceDirectory, PhConcatStringRef2(&serviceDirectory->sr, &serviceSharedName));
        break;
    }

    //if (!PhDoesFileExistWin32(PhGetString(serviceDirectory)))
    //{
    //    PhClearReference(&serviceDirectory);
    //}

    return serviceDirectory;

    //static ULONG (WINAPI *GetSharedServiceDirectory_I)(
    //    _In_ SC_HANDLE ServiceHandle,
    //    _In_ SERVICE_SHARED_DIRECTORY_TYPE DirectoryType,
    //    _Out_writes_to_opt_(PathBufferLength, *RequiredBufferLength) PWCHAR PathBuffer,
    //    _In_ ULONG PathBufferLength,
    //    _Out_ PULONG RequiredBufferLength
    //    ) = NULL;
    //PPH_STRING buffer;
    //ULONG bufferLength = 0;
    //
    //if (!GetSharedServiceDirectory_I)
    //{
    //    GetSharedServiceDirectory_I = PhGetDllProcedureAddress(L"sechost.dll", "GetSharedServiceDirectory", 0);
    //
    //    if (!GetSharedServiceDirectory_I)
    //        return NULL;
    //}
    //
    //if (GetSharedServiceDirectory_I(
    //    ServiceHandle,
    //    ServiceSharedDirectoryPersistentState,
    //    NULL,
    //    0,
    //    &bufferLength
    //    ) != ERROR_INSUFFICIENT_BUFFER)
    //{
    //    return NULL;
    //}
    //
    //buffer = PhCreateStringEx(NULL, (bufferLength + 1) * sizeof(WCHAR));
    //
    //if (GetSharedServiceDirectory_I( // Note: This function creates the directory before returning (dmex)
    //    ServiceHandle,
    //    ServiceSharedDirectoryPersistentState,
    //    buffer->Buffer,
    //    (ULONG)buffer->Length,
    //    &bufferLength
    //    ) != ERROR_SUCCESS)
    //{
    //    PhDereferenceObject(buffer);
    //    return NULL;
    //}
    //
    //return buffer;
}

// rev from sechost.dll!GetServiceRegistryStateKey / sechost.dll!GetSharedServiceRegistryStateKey / IoOpenDriverRegistryKey (dmex)
PPH_STRING PhGetServiceSharedRegistryKey(
    _In_ PPH_STRINGREF ServiceName,
    _In_ PH_SERVICE_DIRECTORY_TYPE DirectoryType
    )
{
    static PH_STRINGREF servicesBaseName = PH_STRINGREF_INIT(L"HKLM\\System\\CurrentControlSet\\Services\\");
    static PH_STRINGREF serviceSharedName = PH_STRINGREF_INIT(L"\\SharedState");
    static PH_STRINGREF serviceDataName = PH_STRINGREF_INIT(L"\\State");
    PPH_STRING serviceStateKey;

    serviceStateKey = PhConcatStringRef2(&servicesBaseName, ServiceName);

    switch (DirectoryType)
    {
    case SERVICE_DIRECTORY_TYPE_DATA:
        PhMoveReference(&serviceStateKey, PhConcatStringRef2(&serviceStateKey->sr, &serviceDataName));
        break;
    case SERVICE_DIRECTORY_TYPE_SHAREDDATA:
        PhMoveReference(&serviceStateKey, PhConcatStringRef2(&serviceStateKey->sr, &serviceSharedName));
        break;
    }

    //{
    //    NTSTATUS status;
    //    HANDLE keyHandle = NULL;
    //    PH_STRINGREF keyName = serviceStateKey->sr;
    //    PhSkipStringRef(&keyName, sizeof(L"HKLM"));
    //
    //    status = PhOpenKey(&keyHandle, KEY_READ, PH_KEY_LOCAL_MACHINE, &keyName, 0);
    //
    //    if (!(
    //        NT_SUCCESS(status) ||
    //        status == STATUS_SHARING_VIOLATION ||
    //        status == STATUS_ACCESS_DENIED
    //        ))
    //    {
    //        PhClearReference(&serviceStateKey);
    //    }
    //
    //    if (keyHandle)
    //    {
    //        NtClose(keyHandle);
    //    }
    //}

    return serviceStateKey;

    //static ULONG (WINAPI *GetSharedServiceRegistryStateKey_I)(
    //    _In_ SC_HANDLE ServiceHandle,
    //    _In_ SERVICE_SHARED_REGISTRY_STATE_TYPE StateType,
    //    _In_ ULONG AccessMask,
    //    _Out_ PHANDLE ServiceStateKey
    //    ) = NULL;
    //PPH_STRING keyPath = NULL;
    //HANDLE keyHandle;
    //
    //if (!GetSharedServiceRegistryStateKey_I)
    //{
    //    GetSharedServiceRegistryStateKey_I = PhGetDllProcedureAddress(L"sechost.dll", "GetSharedServiceRegistryStateKey", 0);
    //
    //    if (!GetSharedServiceRegistryStateKey_I)
    //        return NULL;
    //}
    //
    //if (GetSharedServiceRegistryStateKey_I( // Note: This function creates the registry key before returning (dmex)
    //    ServiceHandle,
    //    ServiceSharedRegistryPersistentState,
    //    KEY_READ,
    //    &keyHandle
    //    ) != ERROR_SUCCESS)
    //{
    //    return NULL;
    //}
    //
    //PhGetHandleInformation(
    //    NtCurrentProcess(),
    //    keyHandle,
    //    ULONG_MAX,
    //    NULL,
    //    NULL,
    //    NULL,
    //    &keyPath
    //    );
    //
    //NtClose(keyHandle);
    //
    //return keyPath;
}

BOOLEAN EspUpdatePackageProperties(
    _In_ PPACKAGE_SERVICE_CONTEXT Context
    )
{
    PPH_STRING string;

    string = PhGetServiceSharedDirectory(&Context->ServiceItem->Name->sr, SERVICE_DIRECTORY_TYPE_DATA);
    PhSetDialogItemText(Context->WindowHandle, IDC_ISOLATEDSTATE, PhGetStringOrDefault(string, L"N/A"));
    PhClearReference(&string);

    string = PhGetServiceSharedDirectory(&Context->ServiceItem->Name->sr, SERVICE_DIRECTORY_TYPE_SHAREDDATA);
    PhSetDialogItemText(Context->WindowHandle, IDC_SHAREDSTATE, PhGetStringOrDefault(string, L"N/A"));
    PhClearReference(&string);

    string = PhGetServiceSharedRegistryKey(&Context->ServiceItem->Name->sr, SERVICE_DIRECTORY_TYPE_DATA);
    PhSetDialogItemText(Context->WindowHandle, IDC_REGISOLATED, PhGetStringOrDefault(string, L"N/A"));
    PhClearReference(&string);

    string = PhGetServiceSharedRegistryKey(&Context->ServiceItem->Name->sr, SERVICE_DIRECTORY_TYPE_SHAREDDATA);
    PhSetDialogItemText(Context->WindowHandle, IDC_REGSHARED, PhGetStringOrDefault(string, L"N/A"));
    PhClearReference(&string);

    string = EspGetServiceAppUserModelId(&Context->ServiceItem->Name->sr);
    PhSetDialogItemText(Context->WindowHandle, IDC_AUMID, PhGetStringOrDefault(string, L"N/A"));
    PhClearReference(&string);

    string = EspGetServicePackageFullName(&Context->ServiceItem->Name->sr);
    PhSetDialogItemText(Context->WindowHandle, IDC_PACKAGENAME, PhGetStringOrDefault(string, L"N/A"));
    PhClearReference(&string);

    return TRUE;
}

INT_PTR CALLBACK EspPackageServiceDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPACKAGE_SERVICE_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
        PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;

        context = PhAllocateZero(sizeof(PACKAGE_SERVICE_CONTEXT));
        context->ServiceItem = serviceItem;

        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;

            EspUpdatePackageProperties(context);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            PhSetDialogFocus(WindowHandle, GetDlgItem(GetParent(WindowHandle), IDCANCEL));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, (LPARAM)GetDlgItem(WindowHandle, IDC_CANCELBTN));
                return TRUE;
            }
        }
        break;
    //case WM_CTLCOLORBTN:
    //    return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    //case WM_CTLCOLORDLG:
    //    return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    //case WM_CTLCOLORSTATIC:
    //    return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
