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

#include <ph.h>
#include <lsasup.h>
#include <mapldr.h>
#include <guisup.h>

#include <shobjidl.h>
#include <shlobj_core.h>

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
            PVOID baseAddress;

            if (baseAddress = PhLoadLibrary(L"kernelbase.dll")) // kernel.appcore.dll
            {
                AppContainerDeriveSidFromMoniker_I = PhGetDllBaseProcedureAddress(baseAddress, "AppContainerDeriveSidFromMoniker", 0);
                AppContainerLookupMoniker_I = PhGetDllBaseProcedureAddress(baseAddress, "AppContainerLookupMoniker", 0);
                AppContainerFreeMemory_I = PhGetDllBaseProcedureAddress(baseAddress, "AppContainerFreeMemory", 0);
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

HRESULT PhAppResolverGetAppIdForProcess(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *ApplicationUserModelId
    )
{
    HRESULT status;
    PVOID resolverInterface;
    PWSTR appIdText;

    if (!(resolverInterface = PhpQueryAppResolverInterface()))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    if (WindowsVersion < WINDOWS_8)
    {
        status = IApplicationResolver_GetAppIDForProcess(
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
        status = IApplicationResolver2_GetAppIDForProcess(
            (IApplicationResolver62*)resolverInterface,
            HandleToUlong(ProcessId),
            &appIdText,
            NULL,
            NULL,
            NULL
            );
    }

    if (HR_SUCCESS(status))
    {
        SIZE_T appIdTextLength;

        appIdTextLength = RtlSizeHeap(
            RtlProcessHeap(),
            0,
            appIdText
            );

        if (appIdTextLength > sizeof(UNICODE_NULL))
        {
            *ApplicationUserModelId = PhCreateStringEx(
                appIdText,
                appIdTextLength - sizeof(UNICODE_NULL)
                );
        }
        else
        {
            status = E_UNEXPECTED;
        }

        CoTaskMemFree(appIdText);
    }

    return status;
}

HRESULT PhAppResolverGetAppIdForWindow(
    _In_ HWND WindowHandle,
    _Out_ PPH_STRING *ApplicationUserModelId
    )
{
    HRESULT status;
    PVOID resolverInterface;
    PWSTR appIdText;

    if (!(resolverInterface = PhpQueryAppResolverInterface()))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    if (WindowsVersion < WINDOWS_8)
    {
        status = IApplicationResolver_GetAppIDForWindow(
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
        status = IApplicationResolver_GetAppIDForWindow(
            (IApplicationResolver62*)resolverInterface,
            WindowHandle,
            &appIdText,
            NULL,
            NULL,
            NULL
            );
    }

    if (HR_SUCCESS(status))
    {
        SIZE_T appIdTextLength;

        appIdTextLength = RtlSizeHeap(
            RtlProcessHeap(),
            0,
            appIdText
            );

        if (appIdTextLength > sizeof(UNICODE_NULL))
        {
            *ApplicationUserModelId = PhCreateStringEx(
                appIdText,
                appIdTextLength - sizeof(UNICODE_NULL)
                );
        }
        else
        {
            status = E_UNEXPECTED;
        }

        CoTaskMemFree(appIdText);
    }

    return status;
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
        CoAllowSetForegroundWindow((IUnknown*)applicationActivationManager, NULL);

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

HRESULT PhAppResolverPackageTerminateProcess(
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
        status = IPackageDebugSettings_TerminateAllProcesses(
            packageDebugSettings,
            PhGetString(PackageFullName)
            );

        IPackageDebugSettings_Release(packageDebugSettings);
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

    if (HR_SUCCESS(result))
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
            PhMoveReference(&appContainerName, PhQueryRegistryStringZ(keyHandle, L"Moniker"));
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

    if (HR_SUCCESS(AppContainerDeriveSidFromMoniker_I(
        AppContainerName,
        &appContainerSid
        )))
    {
        packageSidString = PhSidToStringSid(appContainerSid);

        PhFreeSid(appContainerSid);
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

    if (PhEqualSid(Sid, PhSeInternetExplorerSid())) // S-1-15-3-4096 (dmex)
    {
        return PhCreateString(L"InternetExplorer");
    }

    if (!(sidString = PhSidToStringSid(Sid)))
        return NULL;

    keyPath = PhConcatStringRef2(&appcontainerMappings, &sidString->sr);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &keyPath->sr,
        0
        )))
    {
        PhMoveReference(&packageName, PhQueryRegistryStringZ(keyHandle, L"Moniker"));
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
            PhMoveReference(&packageName, PhQueryRegistryStringZ(keyHandle, L"Moniker"));
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
        packagePath = PhQueryRegistryStringZ(keyHandle, L"PackageRootFolder");
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
    PPH_STRING packageAppDataPath = NULL;
    PPH_STRING localAppDataPath;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    HANDLE tokenHandle;

    localAppDataPath = PhGetKnownLocationZ(PH_FOLDERID_LocalAppData, L"\\Packages\\", FALSE);

    if (PhIsNullOrEmptyString(localAppDataPath))
        return NULL;

    if (NT_SUCCESS(PhOpenProcessToken(ProcessHandle, TOKEN_QUERY, &tokenHandle)))
    {
        if (NT_SUCCESS(PhGetTokenSecurityAttribute(tokenHandle, &attributeName, &info)))
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
            *PhSubAuthoritySid(AppContainerSid, i) !=
            *PhSubAuthoritySid(Sid, i)
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

    if (HR_SUCCESS(status))
    {
        status = IOSTaskCompletion_BeginTask(
            taskCompletion,
            HandleToUlong(ProcessId),
            PT_TC_CRASHDUMP
            );
    }

    if (HR_SUCCESS(status))
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

    if (HR_SUCCESS(status))
    {
        status = IOSTaskCompletion_BeginTaskByHandle(
            taskCompletion,
            ProcessHandle,
            PT_TC_CRASHDUMP
            );
    }

    if (HR_SUCCESS(status))
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
            PVOID baseAddress;

            if (baseAddress = PhLoadLibrary(L"edputil.dll"))
            {
                EdpGetContextForWindow_I = PhGetDllBaseProcedureAddress(baseAddress, "EdpGetContextForWindow", 0);
                EdpFreeContext_I = PhGetDllBaseProcedureAddress(baseAddress, "EdpFreeContext", 0);
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
            PVOID baseAddress;

            if (baseAddress = PhLoadLibrary(L"edputil.dll"))
            {
                EdpGetContextForProcess_I = PhGetDllBaseProcedureAddress(baseAddress, "EdpGetContextForProcess", 0);
                EdpFreeContext_I = PhGetDllBaseProcedureAddress(baseAddress, "EdpFreeContext", 0);
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

// Note: IStartMenuAppItems_EnumItems doesn't return immersive items on Win11 (dmex)
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

DEFINE_GUID(FOLDERID_AppsFolder, 0x1e87508d, 0x89c2, 0x42f0, 0x8a, 0x7e, 0x64, 0x5a, 0x0f, 0x50, 0xca, 0x58);
DEFINE_GUID(BHID_EnumItems, 0x94f60519, 0x2850, 0x4924, 0xaa, 0x5a, 0xd1, 0x5e, 0x84, 0x86, 0x80, 0x39);
DEFINE_GUID(BHID_PropertyStore, 0x0384e1a4, 0x1523, 0x439c, 0xa4, 0xc8, 0xab, 0x91, 0x10, 0x52, 0xf5, 0x86);

static BOOLEAN PhParseStartMenuAppShellItem(
    _In_ IShellItem2* ShellItem,
    _In_ PPH_LIST List
    )
{
    PROPVARIANT packageHostEnvironment;
    PWSTR packageAppUserModelID = NULL;
    PWSTR packageInstallPath = NULL;
    PWSTR packageFullName = NULL;
    PWSTR packageSmallLogoPath = NULL;
    PWSTR packageLongDisplayName = NULL;

    PropVariantInit(&packageHostEnvironment);

    if (HR_FAILED(IShellItem2_GetProperty(ShellItem, &PKEY_AppUserModel_HostEnvironment, &packageHostEnvironment)))
        return FALSE;
    if (!(V_VT(&packageHostEnvironment) == VT_UI4 && V_UI4(&packageHostEnvironment)))
        return FALSE;

    IShellItem2_GetString(ShellItem, &PKEY_AppUserModel_ID, &packageAppUserModelID);
    IShellItem2_GetString(ShellItem, &PKEY_AppUserModel_PackageInstallPath, &packageInstallPath);
    IShellItem2_GetString(ShellItem, &PKEY_AppUserModel_PackageFullName, &packageFullName);
    IShellItem2_GetString(ShellItem, &PKEY_Tile_SmallLogoPath, &packageSmallLogoPath);
    IShellItem2_GetString(ShellItem, &PKEY_Tile_LongDisplayName, &packageLongDisplayName);

    if (packageAppUserModelID &&
        packageInstallPath &&
        packageFullName &&
        packageSmallLogoPath &&
        packageLongDisplayName)
    {
        PPH_APPUSERMODELID_ENUM_ENTRY entry;
        PWSTR imagePath = NULL;

        entry = PhAllocateZero(sizeof(PH_APPUSERMODELID_ENUM_ENTRY));
        entry->AppUserModelId = PhCreateString(packageAppUserModelID);
        entry->PackageDisplayName = PhCreateString(packageLongDisplayName);
        entry->PackageInstallPath = PhCreateString(packageInstallPath);
        entry->PackageFullName = PhCreateString(packageFullName);

        if (HR_SUCCESS(PhAppResolverGetPackageResourceFilePath(packageFullName, packageSmallLogoPath, &imagePath)))
        {
            entry->SmallLogoPath = PhCreateString(imagePath);
            CoTaskMemFree(imagePath);
        }

        PhAddItemList(List, entry);

        return TRUE;
    }

    if (packageAppUserModelID)
        CoTaskMemFree(packageAppUserModelID);
    if (packageInstallPath)
        CoTaskMemFree(packageInstallPath);
    if (packageFullName)
        CoTaskMemFree(packageFullName);
    if (packageSmallLogoPath)
        CoTaskMemFree(packageSmallLogoPath);
    if (packageLongDisplayName)
        CoTaskMemFree(packageLongDisplayName);
    return FALSE;
}

PPH_LIST PhEnumApplicationUserModelIds(
    VOID
    )
{
    HRESULT status;
    PPH_LIST list = NULL;
    IShellItem2* shellKnownFolderItem = NULL;
    IEnumShellItems* shellEnumFolderItem = NULL;

    status = PhShellGetKnownFolderItem(
        &FOLDERID_AppsFolder,
        KF_FLAG_DONT_VERIFY,
        NULL,
        &IID_IShellItem2,
        &shellKnownFolderItem
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = IShellItem2_BindToHandler(
        shellKnownFolderItem,
        NULL,
        &BHID_EnumItems,
        &IID_IEnumShellItems,
        &shellEnumFolderItem
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    list = PhCreateList(10);

    while (TRUE)
    {
        ULONG count = 0;
        IShellItem* itemlist[10];

        if (HR_FAILED(IEnumShellItems_Next(shellEnumFolderItem, 10, itemlist, &count)))
            break;
        if (count == 0)
            break;

        for (ULONG i = 0; i < count; i++)
        {
            IShellItem2* item;

            if (HR_SUCCESS(IShellItem_QueryInterface(itemlist[i], &IID_IShellItem2, &item)))
            {
                PhParseStartMenuAppShellItem(item, list);

                IShellItem2_Release(item);
            }

            IShellItem_Release(itemlist[i]);
        }
    }

CleanupExit:
    if (shellEnumFolderItem)
    {
        IEnumShellItems_Release(shellEnumFolderItem);
    }
    if (shellKnownFolderItem)
    {
        IShellItem2_Release(shellKnownFolderItem);
    }

    return list;
}

HRESULT PhAppResolverGetPackageResourceFilePath(
    _In_ PCWSTR PackageFullName,
    _In_ PCWSTR Key,
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

    if (HR_FAILED(status))
        goto CleanupExit;

    status = IMrtResourceManager_InitializeForPackage(resourceManager, PackageFullName);

    if (HR_FAILED(status))
        goto CleanupExit;

    status = IMrtResourceManager_GetMainResourceMap(resourceManager, &IID_IResourceMap_I, &resourceMap);

    if (HR_FAILED(status))
        goto CleanupExit;

    status = IResourceMap_GetFilePath(resourceMap, Key, &filePath);

CleanupExit:
    if (resourceMap)
        IResourceMap_Release(resourceMap);
    if (resourceManager)
        IMrtResourceManager_Release(resourceManager);

    if (HR_SUCCESS(status))
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
    PPH_STRING applicationUserModelId;
    IPropertyStore* propertyStore;

    if (!(startMenuInterface = PhpQueryStartMenuCacheInterface()))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    status = PhAppResolverGetAppIdForProcess(ProcessId, &applicationUserModelId);

    if (!HR_SUCCESS(status))
        return status;

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

    if (HR_SUCCESS(status))
    {
        *PropertyStore = propertyStore;
    }

    PhDereferenceObject(applicationUserModelId);

    return status;
}

_Success_(return)
BOOLEAN PhAppResolverGetPackageIcon(
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING PackageFullName,
    _Out_opt_ HICON* IconLarge,
    _Out_opt_ HICON* IconSmall,
    _In_ LONG SystemDpi
    )
{
    IPropertyStore* propertyStore = NULL;
    PROPVARIANT propertyPathValue;
    PROPVARIANT propertyColorValue;
    PWSTR imagePath = NULL;
    HICON iconLarge = NULL;
    HICON iconSmall = NULL;

    PropVariantInit(&propertyPathValue);
    PropVariantInit(&propertyColorValue);

    if (HR_FAILED(PhAppResolverGetPackageStartMenuPropertyStore(ProcessId, &propertyStore)))
        goto CleanupExit;
    if (HR_FAILED(IPropertyStore_GetValue(propertyStore, &PKEY_Tile_SmallLogoPath, &propertyPathValue)))
        goto CleanupExit;
    if (HR_FAILED(IPropertyStore_GetValue(propertyStore, &PKEY_Tile_Background, &propertyColorValue)))
        goto CleanupExit;
    if (HR_FAILED(PhAppResolverGetPackageResourceFilePath(PhGetString(PackageFullName), V_BSTR(&propertyPathValue), &imagePath)))
        goto CleanupExit;

    if (IconLarge)
    {
        HBITMAP bitmap;
        LONG width;
        LONG height;

        width = PhGetSystemMetrics(SM_CXICON, SystemDpi);
        height = PhGetSystemMetrics(SM_CYICON, SystemDpi);

        if (bitmap = PhLoadImageFromFile(imagePath, width, height))
        {
            iconLarge = PhGdiplusConvertBitmapToIcon(bitmap, width, height, V_UI4(&propertyColorValue));
            DeleteBitmap(bitmap);
        }
    }

    if (IconSmall)
    {
        HBITMAP bitmap;
        LONG width;
        LONG height;

        width = PhGetSystemMetrics(SM_CXSMICON, SystemDpi);
        height = PhGetSystemMetrics(SM_CYSMICON, SystemDpi);

        if (bitmap = PhLoadImageFromFile(imagePath, width, height))
        {
            iconSmall = PhGdiplusConvertBitmapToIcon(bitmap, width, height, V_UI4(&propertyColorValue));
            DeleteBitmap(bitmap);
        }
    }

CleanupExit:
    if (imagePath)
        CoTaskMemFree(imagePath);
    if (V_BSTR(&propertyPathValue))
        CoTaskMemFree(V_BSTR(&propertyPathValue));
    if (propertyStore)
        IPropertyStore_Release(propertyStore);

    if (IconLarge && IconSmall)
    {
        if (iconLarge && iconSmall)
        {
            *IconLarge = iconLarge;
            *IconSmall = iconSmall;
            return TRUE;
        }

        if (iconLarge)
            DestroyIcon(iconLarge);
        if (iconSmall)
            DestroyIcon(iconSmall);

        return FALSE;
    }

    if (IconLarge && iconLarge)
    {
        *IconLarge = iconLarge;
        return TRUE;
    }

    if (IconSmall && iconSmall)
    {
        *IconSmall = iconSmall;
        return TRUE;
    }

    if (iconLarge)
        DestroyIcon(iconLarge);
    if (iconSmall)
        DestroyIcon(iconSmall);

    return FALSE;
}

// rev from Invoke-CommandInDesktopPackage (dmex)
/**
 * Creates a new process in the context of the supplied PackageFamilyName and AppId.
 * \li \c The created process will have the identity of the provided AppId and will have access to its virtualized file system and registry (if any).
 * \li \c The new process will have a token that's similar to, but not identical to, a real AppId process.
 * \li \c The primary use-case of this command is to invoke debugging or troubleshooting tools in the context of the packaged app to access its virtualized resources.
 * \li \c For example, you can run the Registry Editor to see virtualized registry keys, or Notepad to read virtualized files.
 * \li \c See the important note that follows on using tools such as the Registry Editor that require elevation.
 * \li \c No guarantees are made about the behavior of the created process, other than it having the package identity and access to the package's virtualized resources.
 * \li \c In particular, the new process will not be created in an AppContainer even if an AppId process would normally be created in an AppContainer.
 * \li \c Features such as Privacy Controls or other App Settings may or may not apply to the new process.
 * \li \c You shouldn't rely on any specific side-effects of using this command, as they're undefined and subject to change.
 *
 * \param ApplicationUserModelId The Application ID from the target package's manifest.
 * \param Executable An executable to invoke.
 * \param Arguments Optional arguments to be passed to the new process.
 * \param PreventBreakaway Causes all child processes of the invoked process to also be created in the context of the AppId. By default, child processes are created without any context. This switch is useful for running cmd.exe so that you can launch multiple other tools in the package context.
 * \param ParentProcessId A process to use instead of the calling process as the parent for the process being created.
 * \param ProcessHandle A handle to a process.
 *
 * \return Successful or errant status.
 *
 * \remarks https://learn.microsoft.com/en-us/powershell/module/appx/invoke-commandindesktoppackage
 */
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

    if (WindowsVersion < WINDOWS_11)
    {
        status = PhGetClassObject(
            L"twinui.dll",
            &CLSID_IDesktopAppXActivator_I,
            &IID_IDesktopAppXActivator1_I,
            &desktopAppXActivator
            );
    }
    else
    {
        status = PhGetClassObject(
            L"twinui.appcore.dll",
            &CLSID_IDesktopAppXActivator_I,
            &IID_IDesktopAppXActivator2_I,
            &desktopAppXActivator
            );
    }

    if (HR_SUCCESS(status))
    {
        ULONG options = DAXAO_CHECK_FOR_APPINSTALLER_UPDATES | DAXAO_CENTENNIAL_PROCESS;
        SetFlag(options, PreventBreakaway ? DAXAO_NONPACKAGED_EXE_PROCESS_TREE : DAXAO_NONPACKAGED_EXE);

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
