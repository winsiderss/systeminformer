/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2022
 *
 */

#include <ph.h>
#include <lsasup.h>
#include <guisup.h>

#include <shobjidl.h>

#include <appresolverp.h>
#include <appresolver.h>

static PVOID PhpQueryAppResolverInterface(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID resolverInterface = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion < WINDOWS_8)
            PhGetClassObject(L"appresolver.dll", &CLSID_StartMenuCacheAndAppResolver_I, &IID_IApplicationResolver61_I, &resolverInterface);
        else
            PhGetClassObject(L"appresolver.dll", &CLSID_StartMenuCacheAndAppResolver_I, &IID_IApplicationResolver62_I, &resolverInterface);

        PhEndInitOnce(&initOnce);
    }

    return resolverInterface;
}

static PVOID PhpQueryStartMenuCacheInterface(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID startMenuInterface = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion < WINDOWS_8)
            PhGetClassObject(L"appresolver.dll", &CLSID_StartMenuCacheAndAppResolver_I, &IID_IStartMenuAppItems61_I, &startMenuInterface);
        else
            PhGetClassObject(L"appresolver.dll", &CLSID_StartMenuCacheAndAppResolver_I, &IID_IStartMenuAppItems62_I, &startMenuInterface);

        PhEndInitOnce(&initOnce);
    }

    return startMenuInterface;
}

//static LPMALLOC PhpQueryStartMenuMallocInterface(
//    VOID
//    )
//{
//    static PH_INITONCE initOnce = PH_INITONCE_INIT;
//    static LPMALLOC allocInterface = NULL;
//
//    if (PhBeginInitOnce(&initOnce))
//    {
//        CoGetMalloc(MEMCTX_TASK, &allocInterface);
//
//        PhEndInitOnce(&initOnce);
//    }
//
//    return allocInterface;
//}

static BOOLEAN PhpKernelAppCoreInitialized(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN kernelAppCoreInitialized = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_8)
        {
            PVOID kernelBaseModuleHandle;

            if (kernelBaseModuleHandle = PhLoadLibrary(L"kernelbase.dll")) // kernel.appcore.dll
            {
                AppContainerDeriveSidFromMoniker_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppContainerDeriveSidFromMoniker", 0);
                AppContainerLookupMoniker_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppContainerLookupMoniker", 0);
                AppContainerFreeMemory_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppContainerFreeMemory", 0);
            }

            if (
                AppContainerDeriveSidFromMoniker_I &&
                AppContainerLookupMoniker_I &&
                AppContainerFreeMemory_I
                )
            {
                kernelAppCoreInitialized = TRUE;
            }
        }

        PhEndInitOnce(&initOnce);
    }

    return kernelAppCoreInitialized;
}

_Success_(return)
BOOLEAN PhAppResolverGetAppIdForProcess(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *ApplicationUserModelId
    )
{
    PVOID resolverInterface;
    PWSTR appIdText = NULL;

    if (!(resolverInterface = PhpQueryAppResolverInterface()))
        return FALSE;

    if (WindowsVersion < WINDOWS_8)
    {
        IApplicationResolver_GetAppIDForProcess(
            (IApplicationResolver61*)resolverInterface,
            HandleToUlong(ProcessId),
            &appIdText,
            NULL,
            NULL,
            NULL
            );
    }
    else
    {
        IApplicationResolver2_GetAppIDForProcess(
            (IApplicationResolver62*)resolverInterface,
            HandleToUlong(ProcessId),
            &appIdText,
            NULL,
            NULL,
            NULL
            );
    }

    if (appIdText)
    {
        //SIZE_T appIdTextLength = IMalloc_GetSize(PhpQueryStartMenuMallocInterface(), appIdText);
        //*ApplicationUserModelId = PhCreateStringEx(appIdText, appIdTextLength - sizeof(UNICODE_NULL));
        //IMalloc_Free(PhpQueryStartMenuMallocInterface(), appIdText);

        *ApplicationUserModelId = PhCreateString(appIdText);
        CoTaskMemFree(appIdText);
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhAppResolverGetAppIdForWindow(
    _In_ HWND WindowHandle,
    _Out_ PPH_STRING *ApplicationUserModelId
    )
{
    PVOID resolverInterface;
    PWSTR appIdText = NULL;

    if (!(resolverInterface = PhpQueryAppResolverInterface()))
        return FALSE;

    if (WindowsVersion < WINDOWS_8)
    {
        IApplicationResolver_GetAppIDForWindow(
            (IApplicationResolver61*)resolverInterface,
            WindowHandle,
            &appIdText,
            NULL,
            NULL,
            NULL
            );
    }
    else
    {
        IApplicationResolver_GetAppIDForWindow(
            (IApplicationResolver62*)resolverInterface,
            WindowHandle,
            &appIdText,
            NULL,
            NULL,
            NULL
            );
    }

    if (appIdText)
    {
        //SIZE_T appIdTextLength = IMalloc_GetSize(PhpQueryStartMenuMallocInterface(), appIdText);
        //*ApplicationUserModelId = PhCreateStringEx(appIdText, appIdTextLength - sizeof(UNICODE_NULL));
        //IMalloc_Free(PhpQueryStartMenuMallocInterface(), appIdText);

        *ApplicationUserModelId = PhCreateString(appIdText);
        CoTaskMemFree(appIdText);
        return TRUE;
    }

    return FALSE;
}

HRESULT PhAppResolverActivateAppId(
    _In_ PPH_STRING ApplicationUserModelId,
    _In_opt_ PWSTR CommandLine,
    _Out_opt_ HANDLE *ProcessId
    )
{
    HRESULT status;
    ULONG processId = ULONG_MAX;
    IApplicationActivationManager* applicationActivationManager;

    status = PhGetClassObject(
        L"twinui.appcore.dll",
        &CLSID_ApplicationActivationManager,
        &IID_IApplicationActivationManager,
        &applicationActivationManager
        );

    if (SUCCEEDED(status))
    {
        //CoAllowSetForegroundWindow((IUnknown*)applicationActivationManager, NULL);

        status = IApplicationActivationManager_ActivateApplication(
            applicationActivationManager,
            PhGetString(ApplicationUserModelId),
            CommandLine,
            AO_NONE,
            &processId
            );

        IApplicationActivationManager_Release(applicationActivationManager);
    }

    if (SUCCEEDED(status))
    {
        if (ProcessId) *ProcessId = UlongToHandle(processId);
    }

    return status;
}

HRESULT PhAppResolverEnablePackageDebug(
    _In_ PPH_STRING PackageFullName
    )
{
    HRESULT status;
    IPackageDebugSettings* packageDebugSettings;

    status = PhGetClassObject(
        L"twinapi.appcore.dll",
        &CLSID_PackageDebugSettings,
        &IID_IPackageDebugSettings,
        &packageDebugSettings
        );

    if (SUCCEEDED(status))
    {
        status = IPackageDebugSettings_EnableDebugging(
            packageDebugSettings,
            PhGetString(PackageFullName),
            NULL,
            NULL
            );

        IPackageDebugSettings_Release(packageDebugSettings);
    }

    return status;
}

HRESULT PhAppResolverDisablePackageDebug(
    _In_ PPH_STRING PackageFullName
    )
{
    HRESULT status;
    IPackageDebugSettings* packageDebugSettings;

    status = PhGetClassObject(
        L"twinapi.appcore.dll",
        &CLSID_PackageDebugSettings,
        &IID_IPackageDebugSettings,
        &packageDebugSettings
        );

    if (SUCCEEDED(status))
    {
        status = IPackageDebugSettings_DisableDebugging(
            packageDebugSettings,
            PhGetString(PackageFullName)
            );

        IPackageDebugSettings_Release(packageDebugSettings);
    }

    return status;
}

HRESULT PhAppResolverPackageSuspend(
    _In_ PPH_STRING PackageFullName
    )
{
    HRESULT status;
    IPackageDebugSettings* packageDebugSettings;

    status = PhGetClassObject(
        L"twinapi.appcore.dll",
        &CLSID_PackageDebugSettings,
        &IID_IPackageDebugSettings,
        &packageDebugSettings
        );

    if (SUCCEEDED(status))
    {
        status = IPackageDebugSettings_Suspend(
            packageDebugSettings,
            PhGetString(PackageFullName)
            );

        IPackageDebugSettings_Release(packageDebugSettings);
    }

    return status;
}

HRESULT PhAppResolverPackageResume(
    _In_ PPH_STRING PackageFullName
    )
{
    HRESULT status;
    IPackageDebugSettings* packageDebugSettings;

    status = PhGetClassObject(
        L"twinapi.appcore.dll",
        &CLSID_PackageDebugSettings,
        &IID_IPackageDebugSettings,
        &packageDebugSettings
        );

    if (SUCCEEDED(status))
    {
        status = IPackageDebugSettings_Resume(
            packageDebugSettings,
            PhGetString(PackageFullName)
            );

        IPackageDebugSettings_Release(packageDebugSettings);
    }

    return status;
}

PPH_LIST PhAppResolverEnumeratePackageBackgroundTasks(
    _In_ PPH_STRING PackageFullName
    )
{
    HRESULT status;
    PPH_LIST packageTasks = NULL;
    IPackageDebugSettings* packageDebugSettings;

    status = PhGetClassObject(
        L"twinapi.appcore.dll",
        &CLSID_PackageDebugSettings,
        &IID_IPackageDebugSettings,
        &packageDebugSettings
        );

    if (SUCCEEDED(status))
    {
        ULONG taskCount = 0;
        PGUID taskIds = NULL;
        PWSTR* taskNames = NULL;

        status = IPackageDebugSettings_EnumerateBackgroundTasks(
            packageDebugSettings,
            PhGetString(PackageFullName),
            &taskCount,
            &taskIds,
            &taskNames
            );

        if (SUCCEEDED(status))
        {
            if (!packageTasks && taskCount > 0)
                packageTasks = PhCreateList(taskCount);

            for (ULONG i = 0; i < taskCount; i++)
            {
                PPH_PACKAGE_TASK_ENTRY entry;

                if (!packageTasks)
                    break;

                entry = PhAllocateZero(sizeof(PH_PACKAGE_TASK_ENTRY));
                entry->TaskGuid = taskIds[i];
                entry->TaskName = PhCreateString(taskNames[i]);

                PhAddItemList(packageTasks, entry);
            }
        }

        IPackageDebugSettings_Release(packageDebugSettings);
    }

    if (packageTasks)
        return packageTasks;
    else
        return NULL;
}

HRESULT PhAppResolverPackageStopSessionRedirection(
    _In_ PPH_STRING PackageFullName
    )
{
    HRESULT status;
    IPackageDebugSettings* packageDebugSettings;

    status = PhGetClassObject(
        L"twinapi.appcore.dll",
        &CLSID_PackageDebugSettings,
        &IID_IPackageDebugSettings,
        &packageDebugSettings
        );

    if (SUCCEEDED(status))
    {
        status = IPackageDebugSettings_StopSessionRedirection(
            packageDebugSettings,
            PhGetString(PackageFullName)
            );

        IPackageDebugSettings_Release(packageDebugSettings);
    }

    return status;
}

PPH_STRING PhGetAppContainerName(
    _In_ PSID AppContainerSid
    )
{
    HRESULT result;
    PPH_STRING appContainerName = NULL;
    PWSTR packageMonikerName;

    if (!PhpKernelAppCoreInitialized())
        return NULL;

    result = AppContainerLookupMoniker_I(
        AppContainerSid,
        &packageMonikerName
        );

    if (SUCCEEDED(result))
    {
        appContainerName = PhCreateString(packageMonikerName);
        AppContainerFreeMemory_I(packageMonikerName);
    }
    else // Check the local system account appcontainer mappings. (dmex)
    {
        static PH_STRINGREF appcontainerMappings = PH_STRINGREF_INIT(L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Mappings\\");
        static PH_STRINGREF appcontainerDefaultMappings = PH_STRINGREF_INIT(L".DEFAULT\\");
        HANDLE keyHandle;
        PPH_STRING sidString;
        PPH_STRING keyPath;

        sidString = PhSidToStringSid(AppContainerSid);
        keyPath = PhConcatStringRef3(&appcontainerDefaultMappings, &appcontainerMappings, &sidString->sr);

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_READ,
            PH_KEY_USERS,
            &keyPath->sr,
            0
            )))
        {
            PhMoveReference(&appContainerName, PhQueryRegistryString(keyHandle, L"Moniker"));
            NtClose(keyHandle);
        }

        PhDereferenceObject(keyPath);
        PhDereferenceObject(sidString);
    }

    return appContainerName;
}

PPH_STRING PhGetAppContainerSidFromName(
    _In_ PWSTR AppContainerName
    )
{
    PSID appContainerSid;
    PPH_STRING packageSidString = NULL;

    if (!PhpKernelAppCoreInitialized())
        return NULL;

    if (SUCCEEDED(AppContainerDeriveSidFromMoniker_I(
        AppContainerName,
        &appContainerSid
        )))
    {
        packageSidString = PhSidToStringSid(appContainerSid);

        RtlFreeSid(appContainerSid);
    }

    return packageSidString;
}

PPH_STRING PhGetAppContainerPackageName(
    _In_ PSID Sid
    )
{
    static PH_STRINGREF appcontainerMappings = PH_STRINGREF_INIT(L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppContainer\\Mappings\\");
    static PH_STRINGREF appcontainerDefaultMappings = PH_STRINGREF_INIT(L".DEFAULT\\");
    HANDLE keyHandle;
    PPH_STRING sidString;
    PPH_STRING keyPath;
    PPH_STRING packageName = NULL;

    sidString = PhSidToStringSid(Sid);

    if (PhEqualString2(sidString, L"S-1-15-3-4096", FALSE)) // HACK
    {
        PhDereferenceObject(sidString);
        return PhCreateString(L"InternetExplorer");
    }

    keyPath = PhConcatStringRef2(&appcontainerMappings, &sidString->sr);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyPath->sr,
        0
        )))
    {
        PhMoveReference(&packageName, PhQueryRegistryString(keyHandle, L"Moniker"));
        NtClose(keyHandle);
    }

    PhDereferenceObject(keyPath);

    // Check the local system account appcontainer mappings. (dmex)
    if (PhIsNullOrEmptyString(packageName))
    {
        keyPath = PhConcatStringRef3(&appcontainerDefaultMappings, &appcontainerMappings, &sidString->sr);

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_READ,
            PH_KEY_USERS,
            &keyPath->sr,
            0
            )))
        {
            PhMoveReference(&packageName, PhQueryRegistryString(keyHandle, L"Moniker"));
            NtClose(keyHandle);
        }

        PhDereferenceObject(keyPath);
    }

    PhDereferenceObject(sidString);

    return packageName;
}

// rev from GetPackagePath (dmex)
PPH_STRING PhGetPackagePath(
    _In_ PPH_STRING PackageFullName
    )
{
    static PH_STRINGREF storeAppPackages = PH_STRINGREF_INIT(L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages\\");
    HANDLE keyHandle;
    PPH_STRING keyPath;
    PPH_STRING packagePath = NULL;

    keyPath = PhConcatStringRef2(&storeAppPackages, &PackageFullName->sr);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyPath->sr,
        0
        )))
    {
        packagePath = PhQueryRegistryString(keyHandle, L"PackageRootFolder");
        NtClose(keyHandle);
    }

    PhDereferenceObject(keyPath);

    return packagePath;
}

PPH_STRING PhGetPackageAppDataPath(
    _In_ HANDLE ProcessHandle
    )
{
    static PH_STRINGREF attributeName = PH_STRINGREF_INIT(L"WIN://SYSAPPID");
    static PH_STRINGREF localAppDataPackages = PH_STRINGREF_INIT(L"%APPDATALOCAL%\\Packages\\");
    PPH_STRING packageAppDataPath = NULL;
    PPH_STRING localAppDataPath;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    HANDLE tokenHandle;

    localAppDataPath = PhExpandEnvironmentStrings(&localAppDataPackages);

    if (PhIsNullOrEmptyString(localAppDataPath))
        return NULL;

    if (NT_SUCCESS(PhOpenProcessToken(ProcessHandle, TOKEN_QUERY, &tokenHandle)))
    {
        if (NT_SUCCESS(PhGetTokenSecurityAttributes(tokenHandle, &info)))
        {
            for (ULONG i = 0; i < info->AttributeCount; i++)
            {
                PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = &info->Attribute.pAttributeV1[i];
        
                if (attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING)
                {
                    PH_STRINGREF valueAttributeName;
        
                    PhUnicodeStringToStringRef(&attribute->Name, &valueAttributeName);
        
                    if (PhEqualStringRef(&valueAttributeName, &attributeName, FALSE))
                    {
                        PPH_STRING attributeValue;
        
                        attributeValue = PhCreateStringFromUnicodeString(&attribute->Values.pString[2]);
                        packageAppDataPath = PhConcatStringRef2(&localAppDataPath->sr, &attributeValue->sr);
        
                        PhDereferenceObject(attributeValue);
                        break;
                    }
                }
            }
        
            PhFree(info);
        }

        NtClose(tokenHandle);
    }

    PhDereferenceObject(localAppDataPath);

    return packageAppDataPath;
}

BOOLEAN PhIsPackageCapabilitySid(
    _In_ PSID AppContainerSid,
    _In_ PSID Sid
    )
{
    BOOLEAN isPackageCapability = TRUE;

    for (ULONG i = 1; i < SECURITY_APP_PACKAGE_RID_COUNT - 1; i++)
    {
        if (
            *RtlSubAuthoritySid(AppContainerSid, i) !=
            *RtlSubAuthoritySid(Sid, i)
            )
        {
            isPackageCapability = FALSE;
            break;
        }
    }

    return isPackageCapability;
}

PPH_LIST PhGetPackageAssetsFromResourceFile(
    _In_ PWSTR FilePath
    )
{
    IMrtResourceManager* resourceManager = NULL;
    IResourceMap* resourceMap = NULL;
    PPH_LIST resourceList = NULL;
    ULONG resourceCount = 0;

    if (FAILED(PhGetClassObject(
        L"mrmcorer.dll",
        &CLSID_MrtResourceManager_I,
        &IID_IMrtResourceManager_I,
        &resourceManager
        )))
    {
        return FALSE;
    }

    if (FAILED(IMrtResourceManager_InitializeForFile(resourceManager, FilePath)))
        goto CleanupExit;

    if (FAILED(IMrtResourceManager_GetMainResourceMap(resourceManager, &IID_IResourceMap_I, &resourceMap)))
        goto CleanupExit;

    if (FAILED(IResourceMap_GetNamedResourceCount(resourceMap, &resourceCount)))
        goto CleanupExit;

    resourceList = PhCreateList(10);

    for (ULONG i = 0; i < resourceCount; i++)
    {
        PWSTR resourceName;

        if (SUCCEEDED(IResourceMap_GetNamedResourceUri(resourceMap, i, &resourceName)))
        {
            PhAddItemList(resourceList, PhCreateString(resourceName));
        }
    }

CleanupExit:

    if (resourceMap)
        IResourceMap_Release(resourceMap);

    if (resourceManager)
        IMrtResourceManager_Release(resourceManager);

    return resourceList;
}

HRESULT PhAppResolverBeginCrashDumpTask(
    _In_ HANDLE ProcessId,
    _Out_ HANDLE *TaskHandle
    )
{
    HRESULT status;
    IOSTaskCompletion* taskCompletion;

    status = PhGetClassObject(
        L"twinapi.appcore.dll",
        &CLSID_OSTaskCompletion_I,
        &IID_IOSTaskCompletion_I,
        &taskCompletion
        );

    if (SUCCEEDED(status))
    {
        status = IOSTaskCompletion_BeginTask(
            taskCompletion,
            HandleToUlong(ProcessId),
            PT_TC_CRASHDUMP
            );
    }

    if (SUCCEEDED(status))
    {
        *TaskHandle = taskCompletion;
    }
    else if (taskCompletion)
    {
        IOSTaskCompletion_Release(taskCompletion);
    }

    return status;
}

HRESULT PhAppResolverBeginCrashDumpTaskByHandle(
    _In_ HANDLE ProcessHandle,
    _Out_ HANDLE *TaskHandle
    )
{
    HRESULT status;
    IOSTaskCompletion* taskCompletion;

    status = PhGetClassObject(
        L"twinapi.appcore.dll",
        &CLSID_OSTaskCompletion_I,
        &IID_IOSTaskCompletion_I,
        &taskCompletion
        );

    if (SUCCEEDED(status))
    {
        status = IOSTaskCompletion_BeginTaskByHandle(
            taskCompletion,
            ProcessHandle,
            PT_TC_CRASHDUMP
            );
    }

    if (SUCCEEDED(status))
    {
        *TaskHandle = taskCompletion;
    }
    else if (taskCompletion)
    {
        IOSTaskCompletion_Release(taskCompletion);
    }

    return status;
}

HRESULT PhAppResolverEndCrashDumpTask(
    _In_ HANDLE TaskHandle
    )
{
    IOSTaskCompletion* taskCompletionManager = TaskHandle;
    HRESULT status;

    status = IOSTaskCompletion_EndTask(taskCompletionManager);
    IOSTaskCompletion_Release(taskCompletionManager);

    return status;
}

HRESULT PhAppResolverGetEdpContextForWindow(
    _In_ HWND WindowHandle,
    _Out_ EDP_CONTEXT_STATES* State
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    HRESULT status;
    PEDP_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_10)
        {
            PVOID edputilModuleHandle;

            if (edputilModuleHandle = PhLoadLibrary(L"edputil.dll"))
            {
                EdpGetContextForWindow_I = PhGetDllBaseProcedureAddress(edputilModuleHandle, "EdpGetContextForWindow", 0);
                EdpFreeContext_I = PhGetDllBaseProcedureAddress(edputilModuleHandle, "EdpFreeContext", 0);
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (!(EdpGetContextForWindow_I && EdpFreeContext_I))
        return E_FAIL;

    status = EdpGetContextForWindow_I(
        WindowHandle,
        &context
        );

    if (SUCCEEDED(status))
    {
        *State = context->contextStates;
        EdpFreeContext_I(context);
    }

    return status;
}

HRESULT PhAppResolverGetEdpContextForProcess(
    _In_ HANDLE ProcessId,
    _Out_ EDP_CONTEXT_STATES* State
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    HRESULT status;
    PEDP_CONTEXT context;

    if (PhBeginInitOnce(&initOnce))
    {
        if (WindowsVersion >= WINDOWS_10)
        {
            PVOID edputilModuleHandle;

            if (edputilModuleHandle = PhLoadLibrary(L"edputil.dll"))
            {
                EdpGetContextForProcess_I = PhGetDllBaseProcedureAddress(edputilModuleHandle, "EdpGetContextForProcess", 0);
                EdpFreeContext_I = PhGetDllBaseProcedureAddress(edputilModuleHandle, "EdpFreeContext", 0);
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (!(EdpGetContextForWindow_I && EdpFreeContext_I))
        return E_FAIL;

    status = EdpGetContextForProcess_I(
        HandleToUlong(ProcessId),
        &context
        );

    if (SUCCEEDED(status))
    {
        *State = context->contextStates;
        EdpFreeContext_I(context);
    }

    return status;
}

//VOID PhEnumerateStartMenuAppUserModelIds(VOID)
//{
//    PVOID startMenuInterface;
//    IEnumObjects* startMenuEnumObjects;
//    ULONG count = 0;
//
//    if (!(startMenuInterface = PhpQueryStartMenuCacheInterface()))
//        return;
//
//    if (WindowsVersion < WINDOWS_8)
//    {
//        IStartMenuAppItems_EnumItems(
//            (IStartMenuAppItems61*)startMenuInterface,
//            SMAIF_DEFAULT,
//            &IID_IEnumObjects,
//            &startMenuEnumObjects
//            );
//    }
//    else
//    {
//        IStartMenuAppItems_EnumItems(
//            (IStartMenuAppItems62*)startMenuInterface,
//            SMAIF_DEFAULT,
//            &IID_IEnumObjects,
//            &startMenuEnumObjects
//            );
//    }
//
//    while (TRUE)
//    {
//        IPropertyStore* propStoreInterface[10];
//
//        if (IEnumObjects_Next(startMenuEnumObjects, 10, &IID_IPropertyStore, propStoreInterface, &count) != S_OK)
//            break;
//        if (count == 0)
//            break;
//
//        for (ULONG i = 0; i < count; i++)
//        {
//            PROPVARIANT propValue;
//
//            if (SUCCEEDED(IPropertyStore_GetValue(propStoreInterface[i], &PKEY_AppUserModel_ID, &propValue)))
//            {
//                PropVariantClear(&propValue);
//            }
//
//            IPropertyStore_Release(propStoreInterface[i]);
//        }
//    }
//
//    IStartMenuAppItems_Release(startMenuEnumObjects);
//}

HRESULT PhAppResolverGetPackageResourceFilePath(
    _In_ PPH_STRING PackageFullName,
    _In_ PWSTR Key,
    _Out_ PWSTR* FilePath
    )
{
    HRESULT status;
    IMrtResourceManager* resourceManager = NULL;
    IResourceMap* resourceMap = NULL;
    PWSTR filePath = NULL;

    status = PhGetClassObject(
        L"mrmcorer.dll",
        &CLSID_MrtResourceManager_I,
        &IID_IMrtResourceManager_I,
        &resourceManager
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IMrtResourceManager_InitializeForPackage(resourceManager, PhGetString(PackageFullName));

    if (FAILED(status))
        goto CleanupExit;

    status = IMrtResourceManager_GetMainResourceMap(resourceManager, &IID_IResourceMap_I, &resourceMap);

    if (FAILED(status))
        goto CleanupExit;

    status = IResourceMap_GetFilePath(resourceMap, Key, &filePath);

CleanupExit:
    if (resourceMap)
        IResourceMap_Release(resourceMap);
    if (resourceManager)
        IMrtResourceManager_Release(resourceManager);

    if (SUCCEEDED(status))
    {
        *FilePath = filePath;
    }

    return status;
}

HRESULT PhAppResolverGetPackageStartMenuPropertyStore(
    _In_ HANDLE ProcessId,
    _Out_ IPropertyStore** PropertyStore
    )
{
    HRESULT status;
    PVOID startMenuInterface;
    PPH_STRING applicationUserModelId = NULL;
    IPropertyStore* propertyStore = NULL;

    if (!(startMenuInterface = PhpQueryStartMenuCacheInterface()))
        return S_FALSE;

    if (!PhAppResolverGetAppIdForProcess(ProcessId, &applicationUserModelId))
        return S_FALSE;

    if (WindowsVersion < WINDOWS_8)
    {
        status = IStartMenuAppItems_GetItem(
            (IStartMenuAppItems61*)startMenuInterface,
            SMAIF_DEFAULT,
            PhGetString(applicationUserModelId),
            &IID_IPropertyStore,
            &propertyStore
            );
    }
    else
    {
        status = IStartMenuAppItems2_GetItem(
            (IStartMenuAppItems62*)startMenuInterface,
            SMAIF_DEFAULT,
            PhGetString(applicationUserModelId),
            &IID_IPropertyStore,
            &propertyStore
            );
    }

    if (SUCCEEDED(status))
    {
        *PropertyStore = propertyStore;
    }

    PhDereferenceObject(applicationUserModelId);

    return status;
}

BOOLEAN PhAppResolverGetPackageIcon(
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING PackageFullName,
    _Out_opt_ HICON* IconLarge,
    _Out_opt_ HICON* IconSmall,
    _In_ LONG SystemDpi
    )
{
    IPropertyStore* propertyStore = NULL;
    PROPVARIANT propertyKeyValue = { 0 };
    PROPVARIANT propertyColorValue = { 0 };
    PWSTR imagePath = NULL;
    HICON largeIcon = NULL;
    HICON smallIcon = NULL;

    if (FAILED(PhAppResolverGetPackageStartMenuPropertyStore(ProcessId, &propertyStore)))
        goto CleanupExit;
    if (FAILED(IPropertyStore_GetValue(propertyStore, &PKEY_Tile_SmallLogoPath, &propertyKeyValue)))
        goto CleanupExit;
    //if (FAILED(IPropertyStore_GetValue(propertyStore, &PKEY_Tile_Background, &propertyColorValue)))
    //    goto CleanupExit;
    if (FAILED(PhAppResolverGetPackageResourceFilePath(PackageFullName, propertyKeyValue.pwszVal, &imagePath)))
        goto CleanupExit;

    {
        HBITMAP bitmap;
        LONG width;
        LONG height;

        width = PhGetSystemMetrics(SM_CXICON, SystemDpi);
        height = PhGetSystemMetrics(SM_CYICON, SystemDpi);

        if (bitmap = PhLoadImageFromFile(imagePath, width, height))
        {
            largeIcon = PhGdiplusConvertBitmapToIcon(bitmap, width, height, propertyColorValue.ulVal);
            DeleteBitmap(bitmap);
        }
    }

    {
        HBITMAP bitmap;
        LONG width;
        LONG height;

        width = PhGetSystemMetrics(SM_CXSMICON, SystemDpi);
        height = PhGetSystemMetrics(SM_CYSMICON, SystemDpi);

        if (bitmap = PhLoadImageFromFile(imagePath, width, height))
        {
            smallIcon = PhGdiplusConvertBitmapToIcon(bitmap, width, height, propertyColorValue.ulVal);
            DeleteBitmap(bitmap);
        }
    }

CleanupExit:
    if (imagePath)
        CoTaskMemFree(imagePath);
    if (propertyKeyValue.pwszVal)
        CoTaskMemFree(propertyKeyValue.pwszVal);
    if (propertyStore)
        IPropertyStore_Release(propertyStore);

    if (largeIcon && smallIcon)
    {
        *IconLarge = largeIcon;
        *IconSmall = smallIcon;
        return TRUE;
    }
    else
    {
        if (smallIcon)
            DestroyIcon(smallIcon);
        if (largeIcon)
            DestroyIcon(largeIcon);
        return FALSE;
    }
}

// rev from Invoke-CommandInDesktopPackage (dmex)
HRESULT PhCreateProcessDesktopPackage(
    _In_ PWSTR ApplicationUserModelId,
    _In_ PWSTR Executable,
    _In_ PWSTR Arguments,
    _In_ BOOLEAN PreventBreakaway,
    _In_opt_ HANDLE ParentProcessId,
    _Out_opt_ PHANDLE ProcessHandle
    )
{
    HRESULT status;
    IDesktopAppXActivator* desktopAppXActivator;

    status = PhGetClassObject(
        L"twinui.appcore.dll",
        &CLSID_IDesktopAppXActivator_I,
        &IID_IDesktopAppXActivator2_I,
        &desktopAppXActivator
        );

    if (SUCCEEDED(status))
    {
        ULONG options = DAXAO_CHECK_FOR_APPINSTALLER_UPDATES | DAXAO_CENTENNIAL_PROCESS;
        options |= (PreventBreakaway ? DAXAO_NONPACKAGED_EXE_PROCESS_TREE : DAXAO_NONPACKAGED_EXE);

        status = IDesktopAppXActivator_ActivateWithOptions(
            desktopAppXActivator,
            ApplicationUserModelId,
            Executable,
            Arguments,
            options,
            HandleToUlong(ParentProcessId),
            ProcessHandle
            );

        IDesktopAppXActivator_Release(desktopAppXActivator);
    }

    return status;
}
