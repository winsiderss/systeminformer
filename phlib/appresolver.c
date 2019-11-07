/*
 * Process Hacker -
 *   Appmodel support functions
 *
 * Copyright (C) 2017-2018 dmex
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

#include <ph.h>
#include <lsasup.h>

#include <minappmodel.h>
#include <appmodel.h>
#include <combaseapi.h>
#include <propsys.h>
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
            CoCreateInstance(&CLSID_StartMenuCacheAndAppResolver_I, NULL, CLSCTX_INPROC_SERVER, &IID_IApplicationResolver61_I, &resolverInterface);
        else
            CoCreateInstance(&CLSID_StartMenuCacheAndAppResolver_I, NULL, CLSCTX_INPROC_SERVER, &IID_IApplicationResolver62_I, &resolverInterface);

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
            CoCreateInstance(&CLSID_StartMenuCacheAndAppResolver_I, NULL, CLSCTX_INPROC_SERVER, &IID_IStartMenuAppItems61_I, &startMenuInterface);
        else
            CoCreateInstance(&CLSID_StartMenuCacheAndAppResolver_I, NULL, CLSCTX_INPROC_SERVER, &IID_IStartMenuAppItems62_I, &startMenuInterface);

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
            PVOID kernelBaseModuleHandle;

            if (kernelBaseModuleHandle = LoadLibrary(L"kernelbase.dll")) // kernel.appcore.dll
            {
                AppContainerDeriveSidFromMoniker_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppContainerDeriveSidFromMoniker", 0);
                AppContainerLookupMoniker_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppContainerLookupMoniker", 0);
                AppContainerFreeMemory_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppContainerFreeMemory", 0);
                AppContainerRegisterSid_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppContainerRegisterSid", 0);
                AppContainerUnregisterSid_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppContainerUnregisterSid", 0);
                AppPolicyGetWindowingModel_I = PhGetDllBaseProcedureAddress(kernelBaseModuleHandle, "AppPolicyGetWindowingModel", 0);
            }

            if (
                AppContainerDeriveSidFromMoniker_I &&
                AppContainerLookupMoniker_I && 
                AppContainerFreeMemory_I && 
                AppContainerRegisterSid_I && 
                AppContainerUnregisterSid_I
                )
            {
                kernelAppCoreInitialized = TRUE;
            }
        }

        PhEndInitOnce(&initOnce);
    }

    return kernelAppCoreInitialized;
}

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
        *ApplicationUserModelId = PhCreateString(appIdText);
        return TRUE;
    }

    return FALSE;
}

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
        *ApplicationUserModelId = PhCreateString(appIdText);
        return TRUE;
    }

    return FALSE;
}

HRESULT PhAppResolverActivateAppId(
    _In_ PPH_STRING AppUserModelId,
    _In_opt_ PWSTR CommandLine,
    _Out_opt_ HANDLE *ProcessId
    )
{
    HRESULT status;
    ULONG processId = 0;
    IApplicationActivationManager* applicationActivationManager;

    status = CoCreateInstance(
        &CLSID_ApplicationActivationManager,
        NULL,
        CLSCTX_LOCAL_SERVER,
        &IID_IApplicationActivationManager,
        &applicationActivationManager
        );

    if (SUCCEEDED(status))
    {
        CoAllowSetForegroundWindow((IUnknown*)applicationActivationManager, NULL);

        status = IApplicationActivationManager_ActivateApplication(
            applicationActivationManager,
            PhGetString(AppUserModelId),
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

PPH_LIST PhAppResolverEnumeratePackageBackgroundTasks(
    _In_ PPH_STRING PackageFullName
    )
{
    HRESULT status;
    PPH_LIST packageTasks = NULL;
    IPackageDebugSettings* packageDebugSettings;

    status = CoCreateInstance(
        &CLSID_PackageDebugSettings,
        NULL,
        CLSCTX_INPROC_SERVER,
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
            for (ULONG i = 0; i < taskCount; i++)
            {
                PPH_PACKAGE_TASK_ENTRY entry;

                entry = PhAllocateZero(sizeof(PH_PACKAGE_TASK_ENTRY));
                entry->TaskGuid = taskIds[i];
                entry->TaskName = PhCreateString(taskNames[i]);

                if (!packageTasks)
                    packageTasks = PhCreateList(taskCount);

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
    PhDereferenceObject(sidString);

    return packageName;
}

PPH_STRING PhGetPackagePath(
    _In_ PPH_STRING PackageFullName
    )
{
    static PH_STRINGREF storeAppPackages = PH_STRINGREF_INIT(L"Software\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\AppModel\\Repository\\Packages\\");
    HANDLE keyHandle;
    PPH_STRING keyPath;
    PPH_STRING packagePath = NULL;

    keyPath = PhConcatStringRef2(&storeAppPackages, &PackageFullName->sr);

    // rev from GetPackagePath
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
    static UNICODE_STRING attributeNameUs = RTL_CONSTANT_STRING(L"WIN://SYSAPPID");
    static PH_STRINGREF appdataPackages = PH_STRINGREF_INIT(L"%APPDATALOCAL%\\Packages\\");
    HANDLE tokenHandle;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    PPH_STRING packageAppDataPath = NULL;

    if (NT_SUCCESS(PhOpenProcessToken(
        ProcessHandle,
        TOKEN_QUERY,
        &tokenHandle
        )))
    {
        if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, TokenSecurityAttributes, &info)))
        {
            for (ULONG i = 0; i < info->AttributeCount; i++)
            {
                PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = &info->Attribute.pAttributeV1[i];

                if (attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING)
                {
                    if (RtlEqualUnicodeString(&attribute->Name, &attributeNameUs, FALSE))
                    {
                        PPH_STRING attributeValue;
                        PPH_STRING attributePath;

                        attributeValue = PhCreateStringFromUnicodeString(&attribute->Values.pString[2]);

                        if (attributePath = PhExpandEnvironmentStrings(&appdataPackages))
                        {
                            packageAppDataPath = PhConcatStringRef2(&attributePath->sr, &attributeValue->sr);

                            PhDereferenceObject(attributePath);
                            PhDereferenceObject(attributeValue);
                            break;
                        }

                        PhDereferenceObject(attributeValue);
                    }
                }
            }

            PhFree(info);
        }

        NtClose(tokenHandle);
    }

    return packageAppDataPath;
}

PPH_STRING PhGetProcessPackageFullName(
    _In_ HANDLE ProcessHandle
    )
{
    static UNICODE_STRING attributeNameUs = RTL_CONSTANT_STRING(L"WIN://SYSAPPID");
    HANDLE tokenHandle;
    PTOKEN_SECURITY_ATTRIBUTES_INFORMATION info;
    PPH_STRING packageName = NULL;

    if (NT_SUCCESS(PhOpenProcessToken(
        ProcessHandle,
        TOKEN_QUERY,
        &tokenHandle
        )))
    {
        // rev from PackageIdFromFullName
        if (NT_SUCCESS(PhQueryTokenVariableSize(tokenHandle, TokenSecurityAttributes, &info)))
        {
            for (ULONG i = 0; i < info->AttributeCount; i++)
            {
                PTOKEN_SECURITY_ATTRIBUTE_V1 attribute = &info->Attribute.pAttributeV1[i];

                if (attribute->ValueType == TOKEN_SECURITY_ATTRIBUTE_TYPE_STRING)
                {
                    if (RtlEqualUnicodeString(&attribute->Name, &attributeNameUs, FALSE))
                    {
                        packageName = PhCreateStringFromUnicodeString(&attribute->Values.pString[0]);
                        break;
                    }
                }
            }

            PhFree(info);
        }

        NtClose(tokenHandle);
    }

    return packageName;
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

BOOLEAN PhGetAppWindowingModel(
    _In_ HANDLE ProcessTokenHandle,
    _Out_ AppPolicyWindowingModel *ProcessWindowingModelPolicy
    )
{
    if (!PhpKernelAppCoreInitialized() && !AppPolicyGetWindowingModel_I)
        return FALSE;

    return SUCCEEDED(AppPolicyGetWindowingModel_I(ProcessTokenHandle, ProcessWindowingModelPolicy));
}

PPH_LIST PhGetPackageAssetsFromResourceFile(
    _In_ PWSTR FilePath
    )
{
    IMrtResourceManager* resourceManager = NULL;
    IResourceMap* resourceMap = NULL;
    PPH_LIST resourceList = NULL;
    ULONG resourceCount = 0;

    if (FAILED(CoCreateInstance(
        &CLSID_MrtResourceManager_I,
        NULL,
        CLSCTX_INPROC_SERVER,
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

// TODO: FIXME
//HICON PhAppResolverGetPackageIcon(
//    _In_ HANDLE ProcessId,
//    _In_ PPH_STRING PackageFullName
//    )
//{
//    PVOID startMenuInterface;
//    PPH_STRING applicationUserModelId;
//    IPropertyStore* propStoreInterface;
//    HICON packageIcon = NULL;
//
//    if (!(startMenuInterface = PhpQueryStartMenuCacheInterface()))
//        return NULL;
//
//    if (!PhAppResolverGetAppIdForProcess(ProcessId, &applicationUserModelId))
//        return NULL;
//
//    if (WindowsVersion < WINDOWS_8)
//    {
//        IStartMenuAppItems_GetItem(
//            (IStartMenuAppItems61*)startMenuInterface,
//            SMAIF_DEFAULT,
//            applicationUserModelId->Buffer,
//            &IID_IPropertyStore,
//            &propStoreInterface
//            );
//    }
//    else
//    {
//        IStartMenuAppItems2_GetItem(
//            (IStartMenuAppItems62*)startMenuInterface,
//            SMAIF_DEFAULT,
//            applicationUserModelId->Buffer,
//            &IID_IPropertyStore,
//            &propStoreInterface
//            );
//    }
//
//    if (propStoreInterface)
//    {
//        IMrtResourceManager* mrtResourceManager;
//        IResourceMap* resourceMap;
//        PROPVARIANT propVar;
//        PWSTR filePath;
//
//        IPropertyStore_GetValue(propStoreInterface, &PKEY_Tile_Background, &propVar);
//        IPropertyStore_GetValue(propStoreInterface, &PKEY_Tile_SmallLogoPath, &propVar);
//
//        CoCreateInstance(
//            &CLSID_MrtResourceManager_I,
//            NULL,
//            CLSCTX_INPROC_SERVER,
//            &IID_IMrtResourceManager_I,
//            &mrtResourceManager
//            );
//
//        IMrtResourceManager_InitializeForPackage(mrtResourceManager, PackageFullName->Buffer);
//        IMrtResourceManager_GetMainResourceMap(mrtResourceManager, &IID_IResourceMap_I, &resourceMap);
//        IResourceMap_GetFilePath(resourceMap, propVar.pwszVal, &filePath);
//
//        //HBITMAP bitmap = PhLoadImageFromFile(filePath, 32, 32);
//        //packageIcon = PhBitmapToIcon(bitmap, 32, 32);
//
//        IResourceMap_Release(resourceMap);
//        IMrtResourceManager_Release(mrtResourceManager);
//        PropVariantClear(&propVar);
//
//        IPropertyStore_Release(propStoreInterface);
//    }
//
//    PhDereferenceObject(applicationUserModelId);
//
//    return packageIcon;
//}
