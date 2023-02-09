/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <cpysave.h>
#include <emenu.h>
#include <secwmi.h>
#include <settings.h>
#include <mapldr.h>

#include <phsettings.h>
#include <procprp.h>
#include <procprpp.h>
#include <procprv.h>

#include <wbemidl.h>

typedef struct _PH_PROCESS_WMI_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchWindowHandle;

    PPH_PROCESS_ITEM ProcessItem;
    PPH_STRING DefaultNamespace;
    PPH_STRING SearchboxText;
    PPH_STRING StatusMessage;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG HideDefaultNamespace : 1;
            ULONG HighlightDefaultNamespace : 1;
            ULONG Spare : 30;
        };
    };

    PPH_LIST NodeList;
    PPH_HASHTABLE NodeHashtable;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;
    ULONG TreeNewSortColumn;
    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_CM_MANAGER Cm;
} PH_PROCESS_WMI_CONTEXT, *PPH_PROCESS_WMI_CONTEXT;

typedef struct _PH_WMI_ENTRY
{
    //PPH_STRING InstancePath;
    PPH_STRING RelativePath;
    PPH_STRING ProviderName;
    PPH_STRING ProviderNamespace;
    PPH_STRING FileName;
    PPH_STRING UserName;
} PH_WMI_ENTRY, *PPH_WMI_ENTRY;

typedef enum _PROCESS_WMI_TREE_MENU_ITEM
{
    PROCESS_WMI_TREE_MENU_ITEM_HIDE_DEFAULT_NAMESPACE = 1,
    PROCESS_WMI_TREE_MENU_ITEM_HIGHLIGHT_DEFAULT_NAMESPACE,
    PROCESS_WMI_TREE_MENU_ITEM_MAXIMUM
} WMI_TREE_MENU_ITEM;

typedef enum _PROCESS_WMI_TREE_COLUMN_ITEM
{
    PROCESS_WMI_COLUMN_ITEM_PROVIDER,
    PROCESS_WMI_COLUMN_ITEM_NAMESPACE,
    PROCESS_WMI_COLUMN_ITEM_FILENAME,
    PROCESS_WMI_COLUMN_ITEM_USER,
    PROCESS_WMI_COLUMN_ITEM_MAXIMUM
} WMI_TREE_COLUMN_ITEM;

typedef struct _PHP_PROCESS_WMI_TREENODE
{
    PH_TREENEW_NODE Node;

    ULONG Id;
    PPH_WMI_ENTRY Provider;

    PH_STRINGREF TextCache[PROCESS_WMI_COLUMN_ITEM_MAXIMUM];
} PHP_PROCESS_WMI_TREENODE, *PPHP_PROCESS_WMI_TREENODE;

PPHP_PROCESS_WMI_TREENODE PhpAddWmiProviderNode(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ PPH_WMI_ENTRY Entry
    );

PPHP_PROCESS_WMI_TREENODE PhpFindWmiProviderNode(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ PWSTR KeyPath
    );

VOID PhpClearWmiProviderTree(
    _In_ PPH_PROCESS_WMI_CONTEXT Context
    );

PPHP_PROCESS_WMI_TREENODE PhpGetSelectedWmiProviderNode(
    _In_ PPH_PROCESS_WMI_CONTEXT Context
    );

_Success_(return)
BOOLEAN PhpGetSelectedWmiProviderNodes(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _Out_ PPHP_PROCESS_WMI_TREENODE * *Nodes,
    _Out_ PULONG NumberOfNodes
    );

PVOID PhpGetWmiUtilsDllBase(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING systemDirectory;
        PPH_STRING systemFileName;

        if (systemDirectory = PhGetSystemDirectory())
        {
            if (systemFileName = PhConcatStringRefZ(&systemDirectory->sr, L"\\wbem\\wmiutils.dll"))
            {
                if (!(imageBaseAddress = PhGetLoaderEntryDllBase(&systemFileName->sr, NULL)))
                    imageBaseAddress = PhLoadLibrary(PhGetString(systemFileName));

                PhDereferenceObject(systemFileName);
            }

            PhDereferenceObject(systemDirectory);
        }

        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

PPH_STRING PhGetWbemClassObjectString(
    _In_ IWbemClassObject* WbemClassObject,
    _In_ PCWSTR Name
    )
{
    PPH_STRING string = NULL;
    VARIANT variant = { 0 };

    if (SUCCEEDED(IWbemClassObject_Get(WbemClassObject, Name, 0, &variant, NULL, 0)))
    {
        if (V_BSTR(&variant)) // Can be null (dmex)
        {
            string = PhCreateString(V_BSTR(&variant));
        }

        VariantClear(&variant);
    }

    return string;
}

HRESULT PhpWmiProviderExecMethod(
    _In_ PPH_STRINGREF Method,
    _In_ PWSTR ProcessIdString,
    _In_ PPH_WMI_ENTRY Entry
    )
{
    static PH_STRINGREF wbemResource = PH_STRINGREF_INIT(L"Root\\CIMV2");
    static PH_STRINGREF wbemLanguage = PH_STRINGREF_INIT(L"WQL");
    HRESULT status;
    PVOID imageBaseAddress;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemLanguageString = NULL;
    BSTR wbemQueryString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IEnumWbemClassObject* wbemEnumerator = NULL;
    IWbemClassObject* wbemClassObject;

    if (!(imageBaseAddress = PhGetWbemProxImageBaseAddress()))
        return ERROR_MOD_NOT_FOUND;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocStringLen(wbemResource.Buffer, (UINT32)wbemResource.Length / sizeof(WCHAR));
    status = IWbemLocator_ConnectServer(
        wbemLocator,
        wbemResourceString,
        NULL,
        NULL,
        NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT,
        NULL,
        NULL,
        &wbemServices
        );

    if (FAILED(status))
        goto CleanupExit;

    querySelectString = PhConcatStrings2(
        L"SELECT Namespace,Provider,User,__RELPATH FROM Msft_Providers WHERE HostProcessIdentifier = ",
        ProcessIdString
        );

    wbemLanguageString = SysAllocStringLen(wbemLanguage.Buffer, (UINT32)wbemLanguage.Length / sizeof(WCHAR));
    wbemQueryString = SysAllocStringLen(PhGetString(querySelectString), (UINT32)querySelectString->Length / sizeof(WCHAR));

    if (FAILED(status = IWbemServices_ExecQuery(
        wbemServices,
        wbemLanguageString,
        wbemQueryString,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &wbemEnumerator
        )))
    {
        goto CleanupExit;
    }

    while (TRUE)
    {
        PPH_STRING namespacePath = NULL;
        PPH_STRING providerName = NULL;
        PPH_STRING userName = NULL;
        PPH_STRING relativePath = NULL;
        ULONG count = 0;

        if (FAILED(IEnumWbemClassObject_Next(wbemEnumerator, WBEM_INFINITE, 1, &wbemClassObject, &count)))
            break;
        if (count == 0)
            break;

        namespacePath = PhGetWbemClassObjectString(wbemClassObject, L"Namespace");
        providerName = PhGetWbemClassObjectString(wbemClassObject, L"Provider");
        userName = PhGetWbemClassObjectString(wbemClassObject, L"User");
        relativePath = PhGetWbemClassObjectString(wbemClassObject, L"__RELPATH");

        if (namespacePath && providerName && userName && relativePath)
        {
            if (
                PhEqualString(Entry->ProviderNamespace, namespacePath, FALSE) &&
                PhEqualString(Entry->ProviderName, providerName, FALSE) &&
                PhEqualString(Entry->UserName, userName, FALSE)
                )
            {
                BSTR wbemPathString = SysAllocStringLen(PhGetString(relativePath), (UINT32)relativePath->Length / sizeof(WCHAR));
                BSTR wbemMethodString = SysAllocStringLen(Method->Buffer, (UINT32)Method->Length / sizeof(WCHAR));

                status = IWbemServices_ExecMethod(
                    wbemServices,
                    wbemPathString,
                    wbemMethodString,
                    0,
                    NULL,
                    wbemClassObject,
                    NULL,
                    NULL
                    );

                SysFreeString(wbemMethodString);
                SysFreeString(wbemPathString);
            }
        }

        if (relativePath)
            PhDereferenceObject(relativePath);
        if (userName)
            PhDereferenceObject(userName);
        if (providerName)
            PhDereferenceObject(providerName);
        if (namespacePath)
            PhDereferenceObject(namespacePath);

        IWbemClassObject_Release(wbemClassObject);
    }

CleanupExit:
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemLanguageString)
        SysFreeString(wbemLanguageString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);
    if (wbemEnumerator)
        IEnumWbemClassObject_Release(wbemEnumerator);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    return status;
}

HRESULT PhpQueryWmiProviderFileName(
    _In_ PPH_STRING ProviderNameSpace,
    _In_ PPH_STRING ProviderName,
    _Out_ PPH_STRING *FileName
    )
{
    static PH_STRINGREF wbemLanguage = PH_STRINGREF_INIT(L"WQL");
    HRESULT status;
    PVOID imageBaseAddress;
    PPH_STRING fileName = NULL;
    PPH_STRING clsidString = NULL;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemLanguageString = NULL;
    BSTR wbemQueryString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IEnumWbemClassObject* wbemEnumerator = NULL;
    IWbemClassObject *wbemClassObject = NULL;
    ULONG count = 0;

    if (!(imageBaseAddress = PhGetWbemProxImageBaseAddress()))
        return ERROR_MOD_NOT_FOUND;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocStringLen(PhGetString(ProviderNameSpace), (UINT32)ProviderNameSpace->Length / sizeof(WCHAR));
    status = IWbemLocator_ConnectServer(
        wbemLocator,
        wbemResourceString,
        NULL,
        NULL,
        NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT,
        NULL,
        NULL,
        &wbemServices
        );

    if (FAILED(status))
        goto CleanupExit;

    querySelectString = PhFormatString(
        L"SELECT clsid FROM __Win32Provider WHERE Name = '%s'",
        PhGetString(ProviderName)
        );

    wbemLanguageString = SysAllocStringLen(wbemLanguage.Buffer, (UINT32)wbemLanguage.Length / sizeof(WCHAR));
    wbemQueryString = SysAllocStringLen(PhGetString(querySelectString), (UINT32)querySelectString->Length / sizeof(WCHAR));

    if (FAILED(status = IWbemServices_ExecQuery(
        wbemServices,
        wbemLanguageString,
        wbemQueryString,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &wbemEnumerator
        )))
    {
        goto CleanupExit;
    }

    if (SUCCEEDED(status = IEnumWbemClassObject_Next(
        wbemEnumerator,
        WBEM_INFINITE,
        1,
        &wbemClassObject,
        &count
        )))
    {
        clsidString = PhGetWbemClassObjectString(wbemClassObject, L"CLSID");
        IWbemClassObject_Release(wbemClassObject);
    }

    // Lookup the GUID in the registry to determine the name and file name.

    if (clsidString)
    {
        HANDLE keyHandle;
        PPH_STRING keyPath;

        keyPath = PhConcatStrings(
            4,
            L"CLSID\\",
            PhGetString(clsidString),
            L"\\",
            L"InprocServer32"
            );

        if (SUCCEEDED(status = HRESULT_FROM_NT(PhOpenKey(
            &keyHandle,
            KEY_QUERY_VALUE,
            PH_KEY_CLASSES_ROOT,
            &keyPath->sr,
            0
            ))))
        {
            if (fileName = PhQueryRegistryString(keyHandle, NULL))
            {
                PPH_STRING expandedString;

                if (expandedString = PhExpandEnvironmentStrings(&fileName->sr))
                {
                    PhMoveReference(&fileName, expandedString);
                }
            }

            NtClose(keyHandle);
        }

        PhDereferenceObject(keyPath);
    }

CleanupExit:
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemLanguageString)
        SysFreeString(wbemLanguageString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (clsidString)
        PhDereferenceObject(clsidString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);
    if (wbemEnumerator)
        IEnumWbemClassObject_Release(wbemEnumerator);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    if (SUCCEEDED(status))
    {
        *FileName = fileName;
    }

    return status;
}

HRESULT PhpQueryWmiProviderHostProcess(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ LONG Timeout,
    _Out_ PPH_LIST* ProviderList
    )
{
    static PH_STRINGREF wbemResource = PH_STRINGREF_INIT(L"Root\\CIMV2");
    static PH_STRINGREF wbemLanguage = PH_STRINGREF_INIT(L"WQL");
    HRESULT status;
    PVOID imageBaseAddress;
    PPH_LIST providerList = NULL;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemLanguageString = NULL;
    BSTR wbemQueryString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IEnumWbemClassObject* wbemEnumerator = NULL;
    IWbemClassObject *wbemClassObject;

    if (!(imageBaseAddress = PhGetWbemProxImageBaseAddress()))
        return HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocStringLen(wbemResource.Buffer, (UINT32)wbemResource.Length / sizeof(WCHAR));
    status = IWbemLocator_ConnectServer(
        wbemLocator,
        wbemResourceString,
        NULL,
        NULL,
        NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT,
        NULL,
        NULL,
        &wbemServices
        );

    if (FAILED(status))
        goto CleanupExit;

    querySelectString = PhConcatStrings2(
        L"SELECT Namespace,Provider,User,__RELPATH,__RELPATH FROM Msft_Providers WHERE HostProcessIdentifier = ",
        ProcessItem->ProcessIdString
        );

    wbemLanguageString = SysAllocStringLen(wbemLanguage.Buffer, (UINT32)wbemLanguage.Length / sizeof(WCHAR));
    wbemQueryString = SysAllocStringLen(PhGetString(querySelectString), (UINT32)querySelectString->Length / sizeof(WCHAR));

    if (FAILED(status = IWbemServices_ExecQuery(
        wbemServices,
        wbemLanguageString,
        wbemQueryString,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &wbemEnumerator
        )))
    {
        goto CleanupExit;
    }

    providerList = PhCreateList(1);

    while (TRUE)
    {
        ULONG count = 0;
        PPH_WMI_ENTRY entry;

        if (FAILED(IEnumWbemClassObject_Next(wbemEnumerator, Timeout, 1, &wbemClassObject, &count)))
            break;
        if (count == 0)
            break;

        entry = PhAllocateZero(sizeof(PH_WMI_ENTRY));
        entry->ProviderNamespace = PhGetWbemClassObjectString(wbemClassObject, L"Namespace");
        entry->ProviderName = PhGetWbemClassObjectString(wbemClassObject, L"Provider");
        entry->UserName = PhGetWbemClassObjectString(wbemClassObject, L"User");
        //entry->InstancePath = PhGetWbemClassObjectString(wbemClassObject, L"__PATH");
        entry->RelativePath = PhGetWbemClassObjectString(wbemClassObject, L"__RELPATH");
        IWbemClassObject_Release(wbemClassObject);

        if (entry->ProviderNamespace && entry->ProviderName)
        {
            PPH_STRING fileName = NULL;

            if (SUCCEEDED(PhpQueryWmiProviderFileName(entry->ProviderNamespace, entry->ProviderName, &fileName)))
            {
                entry->FileName = fileName;
            }
        }

        PhAddItemList(providerList, entry);
    }

CleanupExit:
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemLanguageString)
        SysFreeString(wbemLanguageString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);
    if (wbemEnumerator)
        IEnumWbemClassObject_Release(wbemEnumerator);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    if (SUCCEEDED(status))
    {
        *ProviderList = providerList;
    }

    return status;
}

PPH_STRING PhpQueryWmiProviderStatistics(
    _In_ PPH_WMI_ENTRY Entry
    )
{
    static PH_STRINGREF wbemResource = PH_STRINGREF_INIT(L"Root\\CIMV2");
    static PH_STRINGREF wbemLanguage = PH_STRINGREF_INIT(L"WQL");
    HRESULT status;
    PVOID imageBaseAddress;
    PPH_STRING wbemProviderString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemQueryString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IEnumWbemClassObject* wbemEnumerator = NULL;
    IWbemClassObject *wbemClassObject;

    if (!(imageBaseAddress = PhGetWbemProxImageBaseAddress()))
        return NULL;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocStringLen(wbemResource.Buffer, (UINT32)wbemResource.Length / sizeof(WCHAR));
    status = IWbemLocator_ConnectServer(
        wbemLocator,
        wbemResourceString,
        NULL,
        NULL,
        NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT,
        NULL,
        NULL,
        &wbemServices
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IWbemServices_GetObject(
        wbemServices,
        PhGetString(Entry->RelativePath),
        WBEM_FLAG_RETURN_WBEM_COMPLETE,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (SUCCEEDED(status))
    {
        PPH_STRING string;
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 0x100);
        PhAppendFormatStringBuilder(&stringBuilder, L"Statistics for %s: \r\n\r\n", PhGetString(Entry->ProviderName));

        // Note: Strings optimized for string pooling (dmex)
        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_AccessCheck"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_AccessCheck", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_CancelQuery"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_CancelQuery", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_CreateClassEnumAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_CreateClassEnumAsync", L" \t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_CreateInstanceEnumAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_CreateInstanceEnumAsync", L" \t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_CreateRefreshableEnum"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_CreateRefreshableEnum", L" \t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_CreateRefreshableObject"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_CreateRefreshableObject", L" \t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_CreateRefresher"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_CreateRefresher", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_DeleteClassAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_DeleteClassAsync", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_DeleteInstanceAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_DeleteInstanceAsync", L" \t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_ExecMethodAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_ExecMethodAsync", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_ExecQueryAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_ExecQueryAsync", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_FindConsumer"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_FindConsumer", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_GetObjectAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_GetObjectAsync", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_GetObjects"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_GetObjects", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_GetProperty"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_GetProperty", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_NewQuery"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_NewQuery", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_ProvideEvents"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_ProvideEvents", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_PutClassAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_PutClassAsync", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_PutInstanceAsync"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_PutInstanceAsync", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_PutProperty"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_PutProperty", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_QueryInstances"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_QueryInstances", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_SetRegistrationObject"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_SetRegistrationObject", L" \t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_StopRefreshing"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_StopRefreshing", L" \t\t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        if (string = PhGetWbemClassObjectString(wbemClassObject, L"ProviderOperation_ValidateSubscription"))
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"%s:%s", L"ProviderOperation_ValidateSubscription", L" \t");
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L"\r\n");
            PhDereferenceObject(string);
        }

        IWbemClassObject_Release(wbemClassObject);

        wbemProviderString = PhFinalStringBuilderString(&stringBuilder);
    }

CleanupExit:
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (wbemEnumerator)
        IEnumWbemClassObject_Release(wbemEnumerator);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    return wbemProviderString;
}

PPH_STRING PhpQueryWmiDefaultNamespace(
    VOID
    )
{
    static PH_STRINGREF wbemKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Wbem\\Scripting");
    PPH_STRING defaultNameSpace = NULL;
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_QUERY_VALUE,
        PH_KEY_LOCAL_MACHINE,
        &wbemKeyName,
        0
        )))
    {
        defaultNameSpace = PhQueryRegistryString(keyHandle, L"Default Namespace");
        NtClose(keyHandle);
    }

    return defaultNameSpace;
}

VOID PhQueryWmiHostProcessString(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _Inout_ PPH_STRING_BUILDER Providers
    )
{
    PPH_LIST providerList;

    // Note: We use a timeout value of 1 to avoid deadlocks when
    // the wmiprvse.exe process is suspended. Fixes GH#713 (dmex)

    if (SUCCEEDED(PhpQueryWmiProviderHostProcess(ProcessItem, 1, &providerList)))
    {
        for (ULONG i = 0; i < providerList->Count; i++)
        {
            PPH_WMI_ENTRY entry = providerList->Items[i];

            PhAppendFormatStringBuilder(
                Providers,
                L"    %s (%s)\n",
                PhGetStringOrEmpty(entry->ProviderName),
                PhGetStringOrEmpty(entry->FileName)
                );

            //if (entry->InstancePath)
            //    PhDereferenceObject(entry->InstancePath);
            if (entry->RelativePath)
                PhDereferenceObject(entry->RelativePath);
            if (entry->ProviderName)
                PhDereferenceObject(entry->ProviderName);
            if (entry->ProviderNamespace)
                PhDereferenceObject(entry->ProviderNamespace);
            if (entry->FileName)
                PhDereferenceObject(entry->FileName);
            if (entry->UserName)
                PhDereferenceObject(entry->UserName);

            PhFree(entry);
        }

        PhDereferenceObject(providerList);
    }
}

VOID PhpSetWmiProviderListStatusMessage(
    _Inout_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ HRESULT Status
    )
{
    PPH_STRING statusMessage;

    statusMessage = PhGetStatusMessage(0, HRESULT_CODE(Status)); // HACK
    PhMoveReference(&Context->StatusMessage, PhConcatStrings2(
        L"Unable to query provider information:\n",
        PhGetStringOrDefault(statusMessage, L"Unknown error.")
        ));
    TreeNew_SetEmptyText(Context->TreeNewHandle, &Context->StatusMessage->sr, 0);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
    PhClearReference(&statusMessage);
}

VOID PhpRefreshWmiProvidersList(
    _Inout_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    HRESULT status;
    PPH_LIST providerList;

    PhpClearWmiProviderTree(Context);

    // Note: We should use a timeout value of 1 to avoid deadlocks when the
    // wmiprvse.exe process is suspended but infinite guarantees a result. (dmex)

    status = PhpQueryWmiProviderHostProcess(
        ProcessItem,
        WBEM_INFINITE,
        &providerList
        );

    if (SUCCEEDED(status))
    {
        for (ULONG i = 0; i < providerList->Count; i++)
        {
            PhpAddWmiProviderNode(Context, providerList->Items[i]);
        }

        PhDereferenceObject(providerList);
    }
    else
    {
        PhpSetWmiProviderListStatusMessage(Context, status);
    }

    PhApplyTreeNewFilters(&Context->TreeFilterSupport);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhpShowWmiProviderStatus(
    _In_opt_ HWND hWnd,
    _In_opt_ PWSTR Message,
    _In_ HRESULT Win32Result
    )
{
    PPH_STRING statusMessage;

    statusMessage = PhGetMessage(
        PhpGetWmiUtilsDllBase(), 
        0xb, 
        PhGetUserDefaultLangID(), 
        Win32Result
        );

    if (PhIsNullOrEmptyString(statusMessage))
    {
        PhMoveReference(&statusMessage, PhGetStatusMessage(0, HRESULT_CODE(Win32Result)));
    }

    if (statusMessage)
    {
        if (Message)
        {
            PhShowError2(hWnd, Message, L"%s", statusMessage->Buffer);
        }
        else
        {
            PhShowError(hWnd, L"%s", statusMessage->Buffer);
        }

        PhDereferenceObject(statusMessage);
    }
    else
    {
        if (Message)
        {
            PhShowError(hWnd, L"%s", Message);
        }
        else
        {
            PhShowError(hWnd, L"%s", L"Unable to perform the operation.");
        }
    }
}

VOID PhpShowWmiProviderNodeContextMenu(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenuEvent
    )
{
    PPHP_PROCESS_WMI_TREENODE* nodes;
    ULONG numberOfNodes;
    PPH_EMENU menu;
    PPH_EMENU_ITEM selectedItem;

    if (!PhpGetSelectedWmiProviderNodes(Context, &nodes, &numberOfNodes))
        return;
    if (numberOfNodes == 0)
        return;

    menu = PhCreateEMenu();

    if (PhGetIntegerSetting(L"WmiProviderEnableHiddenMenu"))
    {
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Suspend", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Res&ume", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"Un&load", NULL, NULL), ULONG_MAX);
        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    }

    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 4, L"&Inspect", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 5, L"S&tatistics", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 6, L"Open &file location", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
    PhInsertCopyCellEMenuItem(menu, IDC_COPY, Context->TreeNewHandle, ContextMenuEvent->Column);

    selectedItem = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        ContextMenuEvent->Location.x,
        ContextMenuEvent->Location.y
        );

    if (selectedItem && selectedItem->Id != ULONG_MAX)
    {
        if (!PhHandleCopyCellEMenuItem(selectedItem))
        {
            switch (selectedItem->Id)
            {
            case 1:
                {
                    HRESULT status;

                    status = PhpWmiProviderExecMethod(&(PH_STRINGREF)PH_STRINGREF_INIT(L"Suspend"), Context->ProcessItem->ProcessIdString, nodes[0]->Provider);

                    if (FAILED(status))
                    {
                        PhpShowWmiProviderStatus(Context->WindowHandle, L"Unable to perform the operation.", status);
                    }
                }
                break;
            case 2:
                {
                    HRESULT status;

                    status = PhpWmiProviderExecMethod(&(PH_STRINGREF)PH_STRINGREF_INIT(L"Resume"), Context->ProcessItem->ProcessIdString, nodes[0]->Provider);

                    if (FAILED(status))
                    {
                        PhpShowWmiProviderStatus(Context->WindowHandle, L"Unable to perform the operation.", status);
                    }
                }
                break;
            case 3:
                {
                    HRESULT status;

                    status = PhpWmiProviderExecMethod(&(PH_STRINGREF)PH_STRINGREF_INIT(L"Unload"), Context->ProcessItem->ProcessIdString, nodes[0]->Provider);

                    if (FAILED(status))
                    {
                        PhpShowWmiProviderStatus(Context->WindowHandle, L"Unable to perform the operation.", status);
                    }
                }
                break;
            case 4:
                {
                    if (!PhIsNullOrEmptyString(nodes[0]->Provider->FileName) && PhDoesFileExistWin32(PhGetString(nodes[0]->Provider->FileName)))
                    {
                        PhShellExecuteUserString(
                            Context->WindowHandle,
                            L"ProgramInspectExecutables",
                            PhGetString(nodes[0]->Provider->FileName),
                            FALSE,
                            L"Make sure the PE Viewer executable file is present."
                            );
                    }
                }
                break;
            case 5:
                {
                    PPH_STRING string;

                    if (string = PhpQueryWmiProviderStatistics(nodes[0]->Provider))
                    {
                        PhShowInformationDialog(Context->WindowHandle, PhGetString(string), 0);
                        PhDereferenceObject(string);
                    }
                }
                break;
            case 6:
                {
                    if (!PhIsNullOrEmptyString(nodes[0]->Provider->FileName) && PhDoesFileExistWin32(PhGetString(nodes[0]->Provider->FileName)))
                    {
                        PhShellExecuteUserString(
                            Context->WindowHandle,
                            L"FileBrowseExecutable",
                            PhGetString(nodes[0]->Provider->FileName),
                            FALSE,
                            L"Make sure the Explorer executable file is present."
                            );
                    }
                }
                break;
            case IDC_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(Context->TreeNewHandle, 0);
                    PhSetClipboardString(Context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
    }

    PhDestroyEMenu(menu);
}

VOID PhLoadSettingsWmiProviderList(
    _Inout_ PPH_PROCESS_WMI_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhGetStringSetting(L"WmiProviderTreeListColumns");
    sortSettings = PhGetStringSetting(L"WmiProviderTreeListSort");
    Context->Flags = PhGetIntegerSetting(L"WmiProviderTreeListFlags");

    PhCmLoadSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &settings->sr, &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSaveSettingsWmiProviderList(
    _Inout_ PPH_PROCESS_WMI_CONTEXT Context
    )
{
    PPH_STRING settings;
    PPH_STRING sortSettings;

    settings = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, &sortSettings);

    PhSetIntegerSetting(L"WmiProviderTreeListFlags", Context->Flags);
    PhSetStringSetting2(L"WmiProviderTreeListColumns", &settings->sr);
    PhSetStringSetting2(L"WmiProviderTreeListSort", &sortSettings->sr);

    PhDereferenceObject(settings);
    PhDereferenceObject(sortSettings);
}

VOID PhSetOptionsWmiProviderList(
    _Inout_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ ULONG Options
    )
{
    switch (Options)
    {
    case PROCESS_WMI_TREE_MENU_ITEM_HIDE_DEFAULT_NAMESPACE:
        Context->HideDefaultNamespace = !Context->HideDefaultNamespace;
        break;
    case PROCESS_WMI_TREE_MENU_ITEM_HIGHLIGHT_DEFAULT_NAMESPACE:
        Context->HighlightDefaultNamespace = !Context->HighlightDefaultNamespace;
        break;
    }
}

BOOLEAN PhpWmiProviderNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPHP_PROCESS_WMI_TREENODE node1 = *(PPHP_PROCESS_WMI_TREENODE*)Entry1;
    PPHP_PROCESS_WMI_TREENODE node2 = *(PPHP_PROCESS_WMI_TREENODE*)Entry2;

    return PhEqualStringRef(&node1->Provider->RelativePath->sr, &node2->Provider->RelativePath->sr, TRUE);
}

ULONG PhpWmiProviderNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRefEx(&(*(PPHP_PROCESS_WMI_TREENODE*)Entry)->Provider->RelativePath->sr, TRUE, PH_STRING_HASH_X65599);
}

VOID PhpDestroyWmiProviderNode(
    _In_ PPHP_PROCESS_WMI_TREENODE Node
    )
{
    if (Node->Provider)
    {
        if (Node->Provider->RelativePath)
            PhDereferenceObject(Node->Provider->RelativePath);
        if (Node->Provider->ProviderName)
            PhDereferenceObject(Node->Provider->ProviderName);
        if (Node->Provider->ProviderNamespace)
            PhDereferenceObject(Node->Provider->ProviderNamespace);
        if (Node->Provider->FileName)
            PhDereferenceObject(Node->Provider->FileName);
        if (Node->Provider->UserName)
            PhDereferenceObject(Node->Provider->UserName);

        PhFree(Node->Provider);
    }

    PhFree(Node);
}

PPHP_PROCESS_WMI_TREENODE PhpAddWmiProviderNode(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ PPH_WMI_ENTRY Entry
    )
{
    PPHP_PROCESS_WMI_TREENODE node;

    node = PhAllocateZero(sizeof(PHP_PROCESS_WMI_TREENODE));
    PhInitializeTreeNewNode(&node->Node);

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PROCESS_WMI_COLUMN_ITEM_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = PROCESS_WMI_COLUMN_ITEM_MAXIMUM;
    node->Provider = Entry;

    PhAddEntryHashtable(Context->NodeHashtable, &node);
    PhAddItemList(Context->NodeList, node);

    if (Context->TreeFilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &node->Node);

    return node;
}

PPHP_PROCESS_WMI_TREENODE PhpFindWmiProviderNode(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ PWSTR RelativePath
    )
{
    PHP_PROCESS_WMI_TREENODE lookupNode;
    PPHP_PROCESS_WMI_TREENODE lookupNodePtr = &lookupNode;
    PPHP_PROCESS_WMI_TREENODE *node;

    PhInitializeStringRefLongHint(&lookupNode.Provider->RelativePath->sr, RelativePath);

    node = (PPHP_PROCESS_WMI_TREENODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupNodePtr
        );

    if (node)
        return *node;
    else
        return NULL;
}

VOID PhpRemoveWmiProviderNode(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ PPHP_PROCESS_WMI_TREENODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    PhpDestroyWmiProviderNode(Node);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhpUpdateWmiProviderNode(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ PPHP_PROCESS_WMI_TREENODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * PROCESS_WMI_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID PhpExpandAllWmiProviderNodes(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _In_ BOOLEAN Expand
    )
{
    ULONG i;
    BOOLEAN needsRestructure = FALSE;

    for (i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_MODULE_NODE node = Context->NodeList->Items[i];

        if (node->Node.Expanded != Expand)
        {
            node->Node.Expanded = Expand;
            needsRestructure = TRUE;
        }
    }

    if (needsRestructure)
        TreeNew_NodesStructured(Context->TreeNewHandle);
}

#define SORT_FUNCTION(Column) PhpWmiProviderTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl PhpWmiProviderTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPHP_PROCESS_WMI_TREENODE node1 = *(PPHP_PROCESS_WMI_TREENODE*)_elem1; \
    PPHP_PROCESS_WMI_TREENODE node2 = *(PPHP_PROCESS_WMI_TREENODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
         sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPH_PROCESS_WMI_CONTEXT)_context)->TreeNewSortOrder); \
}

LONG PhpWmiProviderTreeNewPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
        Result = uintptrcmp((ULONG_PTR)((PPHP_PROCESS_WMI_TREENODE)Node1)->Node.Index, (ULONG_PTR)((PPHP_PROCESS_WMI_TREENODE)Node2)->Node.Index);

    return PhModifySort(Result, SortOrder);
}

BEGIN_SORT_FUNCTION(ProviderName)
{
    sortResult = PhCompareStringWithNull(node1->Provider->ProviderName, node2->Provider->ProviderName, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(ProviderNamespace)
{
    sortResult = PhCompareStringWithNull(node1->Provider->ProviderNamespace, node2->Provider->ProviderNamespace, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(FileName)
{
    sortResult = PhCompareStringWithNull(node1->Provider->FileName, node2->Provider->FileName, FALSE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(UserName)
{
    sortResult = PhCompareStringWithNull(node1->Provider->UserName, node2->Provider->UserName, FALSE);
}
END_SORT_FUNCTION

BOOLEAN NTAPI PhpWmiProviderTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_WMI_CONTEXT context = Context;
    PPHP_PROCESS_WMI_TREENODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPHP_PROCESS_WMI_TREENODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(ProviderName),
                    SORT_FUNCTION(ProviderNamespace),
                    SORT_FUNCTION(FileName),
                    SORT_FUNCTION(UserName),
                };
                int (__cdecl* sortFunction)(void*, const void*, const void*);

                if (context->TreeNewSortColumn < PROCESS_WMI_COLUMN_ITEM_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE*)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PPHP_PROCESS_WMI_TREENODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPHP_PROCESS_WMI_TREENODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PROCESS_WMI_COLUMN_ITEM_PROVIDER:
                getCellText->Text = PhGetStringRef(node->Provider->ProviderName);
                break;
            case PROCESS_WMI_COLUMN_ITEM_NAMESPACE:
                getCellText->Text = PhGetStringRef(node->Provider->ProviderNamespace);
                break;
            case PROCESS_WMI_COLUMN_ITEM_FILENAME:
                getCellText->Text = PhGetStringRef(node->Provider->FileName);
                break;
            case PROCESS_WMI_COLUMN_ITEM_USER:
                getCellText->Text = PhGetStringRef(node->Provider->UserName);
                break;
            default:
                return FALSE;
            }

            getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = (PPH_TREENEW_GET_NODE_COLOR)Parameter1;
            node = (PPHP_PROCESS_WMI_TREENODE)getNodeColor->Node;

            if (
                context->HighlightDefaultNamespace &&
                context->DefaultNamespace &&
                node->Provider->ProviderNamespace &&
                PhEqualString(context->DefaultNamespace, node->Provider->ProviderNamespace, TRUE)
                )
            {
                getNodeColor->BackColor = PhCsColorElevatedProcesses;
            }

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);

            // HACK
            if (context->TreeFilterSupport.FilterList)
                PhApplyTreeNewFilters(&context->TreeFilterSupport);

            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->WindowHandle, WM_COMMAND, ID_SHOWCONTEXTMENU, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->WindowHandle, WM_COMMAND, IDC_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            }
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(
                data.Menu,
                hwnd,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                data.MouseEvent->ScreenLocation.x,
                data.MouseEvent->ScreenLocation.y
                );

            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    case TreeNewGetDialogCode:
        {
            PULONG code = Parameter2;

            if (PtrToUlong(Parameter1) == VK_RETURN)
            {
                *code = DLGC_WANTMESSAGE;
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

VOID PhpClearWmiProviderTree(
    _In_ PPH_PROCESS_WMI_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhpDestroyWmiProviderNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);

    //TreeNew_NodesStructured(Context->TreeNewHandle);
}

PPHP_PROCESS_WMI_TREENODE PhpGetSelectedWmiProviderNode(
    _In_ PPH_PROCESS_WMI_CONTEXT Context
    )
{
    PPHP_PROCESS_WMI_TREENODE node = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        node = Context->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

_Success_(return)
BOOLEAN PhpGetSelectedWmiProviderNodes(
    _In_ PPH_PROCESS_WMI_CONTEXT Context,
    _Out_ PPHP_PROCESS_WMI_TREENODE **Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPHP_PROCESS_WMI_TREENODE node = (PPHP_PROCESS_WMI_TREENODE)Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

VOID PhpInitializeWmiProviderTree(
    _Inout_ PPH_PROCESS_WMI_CONTEXT Context
    )
{
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(Context->WindowHandle);

    Context->NodeList = PhCreateList(10);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPHP_PROCESS_WMI_TREENODE),
        PhpWmiProviderNodeHashtableEqualFunction,
        PhpWmiProviderNodeHashtableHashFunction,
        10
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, PhpWmiProviderTreeNewCallback, Context);

    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_WMI_COLUMN_ITEM_PROVIDER, TRUE, L"Provider", 140, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_WMI_COLUMN_ITEM_NAMESPACE, TRUE, L"Namespace", 180, PH_ALIGN_LEFT, 1, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_WMI_COLUMN_ITEM_FILENAME, TRUE, L"File name", 260, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PROCESS_WMI_COLUMN_ITEM_USER, TRUE, L"User", 80, PH_ALIGN_LEFT, 3, 0);

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetSort(Context->TreeNewHandle, PROCESS_WMI_COLUMN_ITEM_PROVIDER, NoSortOrder);

    TreeNew_SetRowHeight(Context->TreeNewHandle, PhGetDpi(22, dpiValue));

    PhCmInitializeManager(&Context->Cm, Context->TreeNewHandle, PHMOTLC_MAXIMUM, PhpWmiProviderTreeNewPostSortFunction);
    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);
}

VOID PhpDeleteWmiProviderTree(
    _In_ PPH_PROCESS_WMI_CONTEXT Context
    )
{
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PhpDestroyWmiProviderNode(Context->NodeList->Items[i]);
    }

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

BOOLEAN PhpProcessWmiProviderTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_PROCESS_WMI_CONTEXT context = Context;
    PPHP_PROCESS_WMI_TREENODE node = (PPHP_PROCESS_WMI_TREENODE)Node;

    if (!context)
        return FALSE;

    if (
        context->HideDefaultNamespace &&
        context->DefaultNamespace &&
        node->Provider->ProviderNamespace &&
        PhEqualString(context->DefaultNamespace, node->Provider->ProviderNamespace, TRUE)
        )
    {
        return FALSE;
    }

    if (PhIsNullOrEmptyString(context->SearchboxText))
        return TRUE;

    //if (!PhIsNullOrEmptyString(node->Provider->InstancePath))
    //{
    //    if (PhWordMatchStringRef(&context->SearchboxText->sr, &node->Provider->InstancePath->sr))
    //        return TRUE;
    //}

    //if (!PhIsNullOrEmptyString(node->Provider->RelativePath))
    //{
    //    if (PhWordMatchStringRef(&context->SearchboxText->sr, &node->Provider->RelativePath->sr))
    //        return TRUE;
    //}

    if (!PhIsNullOrEmptyString(node->Provider->ProviderName))
    {
        if (PhWordMatchStringRef(&context->SearchboxText->sr, &node->Provider->ProviderName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Provider->ProviderNamespace))
    {
        if (PhWordMatchStringRef(&context->SearchboxText->sr, &node->Provider->ProviderNamespace->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Provider->FileName))
    {
        if (PhWordMatchStringRef(&context->SearchboxText->sr, &node->Provider->FileName->sr))
            return TRUE;
    }

    if (!PhIsNullOrEmptyString(node->Provider->UserName))
    {
        if (PhWordMatchStringRef(&context->SearchboxText->sr, &node->Provider->UserName->sr))
            return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpProcessWmiProvidersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_PROCESS_WMI_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context = propPageContext->Context = PhAllocateZero(sizeof(PH_PROCESS_WMI_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_SEARCH);
            context->SearchboxText = PhReferenceEmptyString();
            context->ProcessItem = processItem;
            context->DefaultNamespace = PhpQueryWmiDefaultNamespace();

            PhCreateSearchControl(hwndDlg, context->SearchWindowHandle, L"Search WMI Providers (Ctrl+K)");
            Edit_SetSel(context->SearchWindowHandle, 0, -1);
            PhpInitializeWmiProviderTree(context);
            context->TreeFilterEntry = PhAddTreeNewFilter(&context->TreeFilterSupport, PhpProcessWmiProviderTreeFilterCallback, context);

            PhMoveReference(&context->StatusMessage, PhCreateString(L"There are no providers to display."));
            TreeNew_SetEmptyText(context->TreeNewHandle, &context->StatusMessage->sr, 0);
            PhLoadSettingsWmiProviderList(context);

            PhpRefreshWmiProvidersList(context, processItem);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveTreeNewFilter(&context->TreeFilterSupport, context->TreeFilterEntry);

            PhSaveSettingsWmiProviderList(context);
            PhpDeleteWmiProviderTree(context);

            if (context->StatusMessage)
                PhDereferenceObject(context->StatusMessage);
            if (context->DefaultNamespace)
                PhDereferenceObject(context->DefaultNamespace);
            if (context->SearchboxText)
                PhDereferenceObject(context->SearchboxText);

            PhFree(context);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
             TreeNew_SetRowHeight(context->TreeNewHandle, PhGetDpi(22, LOWORD(wParam)));
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, context->SearchWindowHandle, dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhAddPropPageLayoutItem(hwndDlg, context->TreeNewHandle, dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != context->SearchWindowHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchWindowHandle));

                    if (!PhEqualString(context->SearchboxText, newSearchboxText, FALSE))
                    {
                        // Cache the current search text for our callback.
                        PhSwapReference(&context->SearchboxText, newSearchboxText);

                        // Expand any hidden nodes to make search results visible.
                        PhpExpandAllWmiProviderNodes(context, TRUE);

                        PhApplyTreeNewFilters(&context->TreeFilterSupport);
                    }
                }
                break;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case ID_SHOWCONTEXTMENU:
                {
                    PhpShowWmiProviderNodeContextMenu(context, (PPH_TREENEW_CONTEXT_MENU)lParam);
                }
                break;
            case IDC_REFRESH:
                {
                    PhpRefreshWmiProvidersList(context, processItem);
                }
                break;
            case IDC_OPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM namespaceMenuItem;
                    PPH_EMENU_ITEM highlightNamespaceMenuItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_OPTIONS), &rect);

                    namespaceMenuItem = PhCreateEMenuItem(0, PROCESS_WMI_TREE_MENU_ITEM_HIDE_DEFAULT_NAMESPACE, L"Hide default namespace", NULL, NULL);
                    highlightNamespaceMenuItem = PhCreateEMenuItem(0, PROCESS_WMI_TREE_MENU_ITEM_HIGHLIGHT_DEFAULT_NAMESPACE, L"Highlight default namespace", NULL, NULL);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, namespaceMenuItem, ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightNamespaceMenuItem, ULONG_MAX);

                    if (context->HideDefaultNamespace)
                        namespaceMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightDefaultNamespace)
                        highlightNamespaceMenuItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        PhSetOptionsWmiProviderList(context, selectedItem->Id);
                        PhSaveSettingsWmiProviderList(context);
                        PhApplyTreeNewFilters(&context->TreeFilterSupport);
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)context->TreeNewHandle);
                return TRUE;
            }
        }
        break;
    case WM_KEYDOWN:
        {
            if (LOWORD(wParam) == 'K')
            {
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    SetFocus(context->SearchWindowHandle);
                    return TRUE;
                }
            }
        }
        break;
    }

    return FALSE;
}
