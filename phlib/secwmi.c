/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2024
 *
 */

#include <ph.h>
#include <guisup.h>
#include <mapldr.h>
#include <wbemidl.h>
#include <wtsapi32.h>
#include <powrprof.h>
#include <powersetting.h>
#include <secwmi.h>

#include <mi.h>

DEFINE_GUID(CLSID_WbemLocator, 0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);
DEFINE_GUID(IID_IWbemLocator, 0xdc12a687, 0x737f, 0x11cf, 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);

typeof(&PowerGetActiveScheme) PowerGetActiveScheme_I = NULL;
typeof(&PowerSetActiveScheme) PowerSetActiveScheme_I = NULL;
typeof(&PowerRestoreDefaultPowerSchemes) PowerRestoreDefaultPowerSchemes_I = NULL;
_PowerReadSecurityDescriptor PowerReadSecurityDescriptor_I = NULL;
_PowerWriteSecurityDescriptor PowerWriteSecurityDescriptor_I = NULL;
typeof(&WTSGetListenerSecurityW) WTSGetListenerSecurity_I = NULL;
typeof(&WTSSetListenerSecurityW) WTSSetListenerSecurity_I = NULL;
typeof(&UpdateDCOMSettings) UpdateDCOMSettings_I = NULL;
typeof(&MI_Application_Initialize) MI_Application_Initialize_I = NULL;

HRESULT PhGetWbemLocatorClass(
    _Out_ struct IWbemLocator** WbemLocatorClass
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;
    HRESULT status;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING systemFileName;

        if (systemFileName = PhGetSystemDirectoryWin32Z(L"\\wbem\\wbemprox.dll"))
        {
            imageBaseAddress = PhLoadLibrary(PhGetString(systemFileName));
            PhDereferenceObject(systemFileName);
        }

        {
            typedef void (WINAPI* _SetOaNoCache)(void);
            _SetOaNoCache SetOaNoCache_I;

            if (SetOaNoCache_I = PhGetModuleProcAddress(L"oleaut32.dll", "SetOaNoCache"))
            {
                SetOaNoCache_I();
            }
        }

        PhEndInitOnce(&initOnce);
    }

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        WbemLocatorClass
        );

    return status;
}

PVOID PhpInitializePowerPolicyApi(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        imageBaseAddress = PhLoadLibrary(L"powrprof.dll");
        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

PVOID PhpInitializeRemoteDesktopServiceApi(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        imageBaseAddress = PhLoadLibrary(L"wtsapi32.dll");
        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

PVOID PhpInitializeComSecurityApi(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        imageBaseAddress = PhLoadLibrary(L"ole32.dll");
        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

PVOID PhpInitializeManagementInfrastructureApi(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        imageBaseAddress = PhLoadLibrary(L"mi.dll");
        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

NTSTATUS PhMiResultToNtStatus(
    _In_ MI_Result Result
    )
{
    switch (Result)
    {
    case MI_RESULT_OK:
        return STATUS_SUCCESS;
    case MI_RESULT_ACCESS_DENIED:
        return STATUS_ACCESS_DENIED;
    case MI_RESULT_INVALID_NAMESPACE:
        return STATUS_OBJECT_PATH_NOT_FOUND;
    case MI_RESULT_INVALID_PARAMETER:
        return STATUS_INVALID_PARAMETER;
    case MI_RESULT_INVALID_CLASS:
    case MI_RESULT_NOT_FOUND:
    case MI_RESULT_METHOD_NOT_FOUND:
        return STATUS_OBJECT_NAME_NOT_FOUND;
    case MI_RESULT_NOT_SUPPORTED:
    case MI_RESULT_METHOD_NOT_AVAILABLE:
    case MI_RESULT_QUERY_LANGUAGE_NOT_SUPPORTED:
    case MI_RESULT_FILTERED_ENUMERATION_NOT_SUPPORTED:
    case MI_RESULT_CONTINUATION_ON_ERROR_NOT_SUPPORTED:
        return STATUS_NOT_SUPPORTED;
    case MI_RESULT_ALREADY_EXISTS:
        return STATUS_OBJECT_NAME_COLLISION;
    case MI_RESULT_NO_SUCH_PROPERTY:
        return STATUS_NOT_FOUND;
    case MI_RESULT_TYPE_MISMATCH:
        return STATUS_OBJECT_TYPE_MISMATCH;
    case MI_RESULT_INVALID_QUERY:
        return STATUS_INVALID_INFO_CLASS;
    case MI_RESULT_INVALID_ENUMERATION_CONTEXT:
        return STATUS_INVALID_HANDLE;
    case MI_RESULT_INVALID_OPERATION_TIMEOUT:
        return STATUS_IO_TIMEOUT;
    case MI_RESULT_SERVER_LIMITS_EXCEEDED:
        return STATUS_QUOTA_EXCEEDED;
    case MI_RESULT_SERVER_IS_SHUTTING_DOWN:
        return STATUS_SERVER_DISABLED;
    case MI_RESULT_FAILED:
    case MI_RESULT_CLASS_HAS_CHILDREN:
    case MI_RESULT_CLASS_HAS_INSTANCES:
    case MI_RESULT_INVALID_SUPERCLASS:
    case MI_RESULT_NAMESPACE_NOT_EMPTY:
    case MI_RESULT_PULL_HAS_BEEN_ABANDONED:
    case MI_RESULT_PULL_CANNOT_BE_ABANDONED:
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

PVOID PhGetMiClassObjectUlongPtr(
    _In_ const MI_Instance* MiClassObject,
    _In_ PCWSTR Name
    )
{
    MI_Value value;
    MI_Type type;

    if (MI_Instance_GetElement(MiClassObject, Name, &value, &type, NULL, NULL) == MI_RESULT_OK)
    {
        if (type == MI_UINT64)
            return (PVOID)(ULONG_PTR)value.uint64;
        if (type == MI_UINT32)
            return (PVOID)(ULONG_PTR)value.uint32;
    }

    return NULL;
}

PPH_STRING PhGetMiClassObjectString(
    _In_ const MI_Instance* MiClassObject,
    _In_ PCWSTR Name
    )
{
    MI_Value value;
    MI_Type type;

    if (MI_Instance_GetElement(MiClassObject, Name, &value, &type, NULL, NULL) == MI_RESULT_OK)
    {
        if (type == MI_STRING && value.string)
            return PhCreateString(value.string);
    }

    return NULL;
}

HRESULT PhCoSetProxyBlanket(
    _In_ IUnknown* InterfacePtr
    )
{
    HRESULT status;
    IClientSecurity* clientSecurity;

    status = IUnknown_QueryInterface(
        InterfacePtr,
        &IID_IClientSecurity,
        &clientSecurity
        );

    if (SUCCEEDED(status))
    {
        status = IClientSecurity_SetBlanket(
            clientSecurity,
            InterfacePtr,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE
            );
        IClientSecurity_Release(clientSecurity);
    }

    return status;
}

HRESULT PhGetWbemClassObjectDependency(
    _Out_ PVOID* WbemClassObjectDependency,
    _In_ IWbemClassObject* WbemClassObject,
    _In_ IWbemServices* WbemServices,
    _In_ PCWSTR Name
    )
{
    HRESULT status;
    IWbemClassObject* dependency;
    VARIANT variant = { 0 };

    status = IWbemClassObject_Get(
        WbemClassObject,
        Name,
        0,
        &variant,
        NULL,
        NULL
        );

    if (SUCCEEDED(status))
    {
        status = IWbemServices_GetObject(
            WbemServices,
            V_BSTR(&variant),
            WBEM_FLAG_RETURN_WBEM_COMPLETE,
            NULL,
            &dependency,
            NULL
            );

        if (SUCCEEDED(status))
        {
            *WbemClassObjectDependency = dependency;
        }

        VariantClear(&variant);
    }

    return status;
}

PPH_STRING PhGetWbemClassObjectString(
    _In_ PVOID WbemClassObject,
    _In_ PCWSTR Name
    )
{
    PPH_STRING string = NULL;
    VARIANT variant = { 0 };

    if (SUCCEEDED(IWbemClassObject_Get((IWbemClassObject*)WbemClassObject, Name, 0, &variant, NULL, 0)))
    {
        if (V_BSTR(&variant)) // Can be null (dmex)
        {
            string = PhCreateString(V_BSTR(&variant));
        }

        VariantClear(&variant);
    }

    return string;
}

ULONG64 PhGetWbemClassObjectUlong64(
    _In_ PVOID WbemClassObject,
    _In_ PCWSTR Name
    )
{
    VARIANT variant;

    RtlZeroMemory(&variant, sizeof(VARIANT));

    if (SUCCEEDED(IWbemClassObject_Get((IWbemClassObject*)WbemClassObject, Name, 0, &variant, NULL, 0)))
    {
        return V_UI8(&variant);
    }

    return ULONG64_MAX;
}

PVOID PhGetWbemClassObjectUlongPtr(
    _In_ PVOID WbemClassObject,
    _In_ PCWSTR Name
    )
{
    VARIANT variant;

    RtlZeroMemory(&variant, sizeof(VARIANT));

    if (SUCCEEDED(IWbemClassObject_Get((IWbemClassObject*)WbemClassObject, Name, 0, &variant, NULL, 0)))
    {
        return (PVOID)V_UINT_PTR(&variant);
    }

    return NULL;
}

// Power policy security descriptors

NTSTATUS PhpGetPowerPolicySecurityDescriptor(
    _Out_ PPH_STRING* StringSecurityDescriptor
    )
{
    ULONG status;
    PWSTR stringSecurityDescriptor;
    PGUID policyGuid;

    if (!(PowerGetActiveScheme_I && PowerReadSecurityDescriptor_I))
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpInitializePowerPolicyApi())
        {
            PowerGetActiveScheme_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerGetActiveScheme", 0);
            PowerReadSecurityDescriptor_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerReadSecurityDescriptor", 0);
        }
        else
        {
            return STATUS_DLL_NOT_FOUND;
        }
    }

    if (!(PowerGetActiveScheme_I && PowerReadSecurityDescriptor_I))
        return STATUS_PROCEDURE_NOT_FOUND;

    // We can use GUIDs for schemes, policies or other values but we only query the default scheme. (dmex)
    status = PowerGetActiveScheme_I(
        NULL,
        &policyGuid
        );

    if (status != ERROR_SUCCESS)
        return PhDosErrorToNtStatus(status);

    // PowerReadSecurityDescriptor/PowerWriteSecurityDescriptor both use the same SDDL
    // format as registry keys and are RPC wrappers for this registry key:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Power\SecurityDescriptors

    status = PowerReadSecurityDescriptor_I(
        ACCESS_DEFAULT_SECURITY_DESCRIPTOR, // ACCESS_ACTIVE_SCHEME?
        policyGuid,
        &stringSecurityDescriptor
        );

    if (status == ERROR_SUCCESS)
    {
        if (StringSecurityDescriptor)
        {
            *StringSecurityDescriptor = PhCreateString(stringSecurityDescriptor);
        }

        LocalFree(stringSecurityDescriptor);
    }

    LocalFree(policyGuid);

    return PhDosErrorToNtStatus(status);
}

NTSTATUS PhpSetPowerPolicySecurityDescriptor(
    _In_ PPH_STRING StringSecurityDescriptor
    )
{
    ULONG status;
    PGUID policyGuid;

    if (!(PowerGetActiveScheme_I && PowerWriteSecurityDescriptor_I))
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpInitializePowerPolicyApi())
        {
            PowerGetActiveScheme_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerGetActiveScheme", 0);
            PowerWriteSecurityDescriptor_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerWriteSecurityDescriptor", 0);
        }
        else
        {
            return STATUS_DLL_NOT_FOUND;
        }
    }

    if (!(PowerGetActiveScheme_I && PowerWriteSecurityDescriptor_I))
        return STATUS_PROCEDURE_NOT_FOUND;

    status = PowerGetActiveScheme_I(
        NULL,
        &policyGuid
        );

    if (status != ERROR_SUCCESS)
        return PhDosErrorToNtStatus(status);

    status = PowerWriteSecurityDescriptor_I(
        ACCESS_DEFAULT_SECURITY_DESCRIPTOR,
        policyGuid, // Todo: Pass the GUID from the original query.
        PhGetString(StringSecurityDescriptor)
        );

    //if (status == ERROR_SUCCESS)
    //{
    //    // refresh/reapply the current scheme.
    //    status = PowerSetActiveScheme_I(NULL, policyGuid);
    //}

    LocalFree(policyGuid);

    return PhDosErrorToNtStatus(status);
}

// Terminal server security descriptors

NTSTATUS PhpGetRemoteDesktopSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    BOOL status;
    ULONG securityDescriptorLength = 0;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;

    if (!WTSGetListenerSecurity_I)
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpInitializeRemoteDesktopServiceApi())
            WTSGetListenerSecurity_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "WTSGetListenerSecurityW", 0);
        else
            return STATUS_DLL_NOT_FOUND;
    }

    if (!WTSGetListenerSecurity_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    // Todo: Add support for SI_RESET using the default security descriptor:
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Terminal Server\DefaultSecurity

    status = WTSGetListenerSecurity_I(
        WTS_CURRENT_SERVER_HANDLE,
        NULL,
        0,
        L"Rdp-Tcp", // or "Console"
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
        securityDescriptor,
        0,
        &securityDescriptorLength
        );

    if (!(!status && securityDescriptorLength))
        return STATUS_INVALID_SECURITY_DESCR;

    securityDescriptor = PhAllocateZero(securityDescriptorLength);

    status = WTSGetListenerSecurity_I(
        WTS_CURRENT_SERVER_HANDLE,
        NULL,
        0,
        L"Rdp-Tcp", // or "Console"
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
        securityDescriptor,
        securityDescriptorLength,
        &securityDescriptorLength
        );

    if (status)
    {
        if (SecurityDescriptor)
        {
            *SecurityDescriptor = securityDescriptor;
            return STATUS_SUCCESS;
        }
    }

    if (securityDescriptor)
        PhFree(securityDescriptor);

    return STATUS_INVALID_SECURITY_DESCR;
}

NTSTATUS PhpSetRemoteDesktopSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    if (!WTSSetListenerSecurity_I)
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpInitializeRemoteDesktopServiceApi())
            WTSSetListenerSecurity_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "WTSSetListenerSecurityW", 0);
        else
            return STATUS_DLL_NOT_FOUND;
    }

    if (!WTSSetListenerSecurity_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    if (WTSSetListenerSecurity_I(
        WTS_CURRENT_SERVER_HANDLE,
        NULL,
        0,
        L"RDP-Tcp",
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
        SecurityDescriptor
        ))
    {
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_SECURITY_DESCR;
}

// WBEM security descriptors

#if defined(PHLIB_WBEM_DEPRECATED)
NTSTATUS PhGetWmiNamespaceSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    HRESULT status;
    PVOID securityDescriptor = NULL;
    PVOID securityDescriptorData = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemObjectString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemGetSDClassObject = 0;
    VARIANT variantArrayValue = { 0 };
    VARIANT variantReturnValue = { 0 };

    status = PhGetWbemLocatorClass(
        &wbemLocator
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemResourceString = PhStringZToBSTR(L"Root");
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

    if (HR_FAILED(status))
        goto CleanupExit;

    status = PhCoSetProxyBlanket(
        (IUnknown*)wbemServices
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemObjectString = PhStringZToBSTR(L"__SystemSecurity");
    status = IWbemServices_GetObject(
        wbemServices,
        wbemObjectString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemMethodString = PhStringZToBSTR(L"GetSD");
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemObjectString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemGetSDClassObject,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemGetSDClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    if (V_UI4(&variantReturnValue) != ERROR_SUCCESS)
    {
        status = HRESULT_FROM_WIN32(V_UI4(&variantReturnValue));
        goto CleanupExit;
    }

    status = IWbemClassObject_Get(
        wbemGetSDClassObject,
        L"SD",
        0,
        &variantArrayValue,
        NULL,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = SafeArrayAccessData(V_ARRAY(&variantArrayValue), &securityDescriptorData);

    if (HR_FAILED(status))
        goto CleanupExit;

    // The Wbem security descriptor is relative but the ACUI
    // dialog automatically converts the descriptor for us
    // so we don't have to convert just validate. (dmex)

    if (RtlValidSecurityDescriptor(securityDescriptorData))
    {
        securityDescriptor = PhAllocateCopy(
            securityDescriptorData,
            PhLengthSecurityDescriptor(securityDescriptorData)
            );
    }

    SafeArrayUnaccessData(V_ARRAY(&variantArrayValue));

CleanupExit:
    if (wbemGetSDClassObject)
        IWbemClassObject_Release(wbemGetSDClassObject);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemClassObject)
        IWbemClassObject_Release(wbemClassObject);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    VariantClear(&variantArrayValue);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemObjectString)
        SysFreeString(wbemObjectString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);

    if (HR_SUCCESS(status) && securityDescriptor)
    {
        *SecurityDescriptor = securityDescriptor;
        return STATUS_SUCCESS;
    }

    if (securityDescriptor)
    {
        PhFree(securityDescriptor);
    }

    if (status == WBEM_E_ACCESS_DENIED)
        return STATUS_ACCESS_DENIED;
    if (status == WBEM_E_INVALID_PARAMETER)
        return STATUS_INVALID_PARAMETER;

    return STATUS_INVALID_SECURITY_DESCR;
}
#else
NTSTATUS PhGetWmiNamespaceSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    MI_Result miResult;
    MI_Application application;
    MI_Session session;
    MI_Operation operation = MI_OPERATION_NULL;
    MI_Instance* instance = NULL;
    const MI_Instance* completionDetails = NULL;
    const MI_Char* errorMessage = NULL;
    MI_Boolean moreResults = MI_FALSE;
    MI_Result operationResult = MI_RESULT_OK;
    MI_Value value;
    MI_Type type = 0;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    NTSTATUS status = STATUS_SUCCESS;

    if (!MI_Application_Initialize_I)
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpInitializeManagementInfrastructureApi())
        {
            MI_Application_Initialize_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "MI_Application_InitializeV1", 0);
        }
    }

    if (!MI_Application_Initialize_I)
        return STATUS_DLL_NOT_FOUND;

    miResult = MI_Application_Initialize_I(
        0,
        NULL,
        NULL,
        &application
        );

    if (miResult != MI_RESULT_OK)
        return PhMiResultToNtStatus(miResult);

    miResult = MI_Application_NewSession(
        &application,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &session
        );

    if (miResult != MI_RESULT_OK)
    {
        MI_Application_Close(&application);
        return PhMiResultToNtStatus(miResult);
    }

    MI_Session_Invoke(
        &session,
        0,
        NULL,
        L"root",
        L"__SystemSecurity",
        L"GetSD",
        NULL,
        NULL,
        NULL,
        &operation
        );

    for (;;)
    {
        miResult = MI_Operation_GetInstance(
            &operation,
            &instance,
            &moreResults,
            &operationResult,
            &errorMessage,
            &completionDetails
            );

        status = PhMiResultToNtStatus(operationResult);

        if (miResult != MI_RESULT_OK)
        {
            status = PhMiResultToNtStatus(miResult);
            break;
        }

        if (!instance)
            break;

        if (MI_Instance_GetElement(
            instance,
            L"ReturnValue",
            &value,
            &type,
            NULL,
            NULL
            ) == MI_RESULT_OK && type == MI_UINT32)
        {
            if (value.uint32 != ERROR_SUCCESS)
            {
                status = PhDosErrorToNtStatus(value.uint32);
            }
        }

        if (NT_SUCCESS(status) && MI_Instance_GetElement(
            instance,
            L"SD",
            &value,
            &type,
            NULL,
            NULL
            ) == MI_RESULT_OK && type == MI_UINT8A && value.uint8a.data && value.uint8a.size)
        {
            if (RtlValidSecurityDescriptor(value.uint8a.data))
            {
                securityDescriptor = PhAllocateCopy(
                    value.uint8a.data,
                    value.uint8a.size
                    );
            }
        }

        if (!moreResults)
            break;
    }

    MI_Operation_Close(&operation);
    MI_Session_Close(&session, NULL, NULL);
    MI_Application_Close(&application);

    if (NT_SUCCESS(status) && securityDescriptor)
    {
        *SecurityDescriptor = securityDescriptor;
        return STATUS_SUCCESS;
    }

    if (securityDescriptor)
        PhFree(securityDescriptor);

    if (!NT_SUCCESS(status))
        return status;

    return STATUS_INVALID_SECURITY_DESCR;
}
#endif

NTSTATUS PhSetWmiNamespaceSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    HRESULT status;
    BSTR wbemResourceString = NULL;
    BSTR wbemObjectString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemSetSDClassObject = NULL;
    PVOID safeArrayData = NULL;
    LPSAFEARRAY safeArray = NULL;
    SAFEARRAYBOUND safeArrayBounds;
    PSECURITY_DESCRIPTOR relativeSecurityDescriptor = NULL;
    ULONG relativeSecurityDescriptorLength = 0;
    BOOLEAN freeSecurityDescriptor = FALSE;
    VARIANT variantArrayValue = { 0 };
    VARIANT variantReturnValue = { 0 };
    PSID administratorsSid;

    // kludge the descriptor into the correct format required by wmimgmt (dmex)
    // 1) The owner must always be the built-in domain administrator.
    // 2) The group must always be the built-in domain administrator.

    administratorsSid = PhSeAdministratorsSid();
    PhSetOwnerSecurityDescriptor(SecurityDescriptor, administratorsSid, TRUE);
    PhSetGroupSecurityDescriptor(SecurityDescriptor, administratorsSid, TRUE);

    status = PhGetWbemLocatorClass(
        &wbemLocator
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemResourceString = PhStringZToBSTR(L"Root");
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

    if (HR_FAILED(status))
        goto CleanupExit;

    status = PhCoSetProxyBlanket(
        (IUnknown*)wbemServices
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemObjectString = PhStringZToBSTR(L"__SystemSecurity");
    status = IWbemServices_GetObject(
        wbemServices,
        wbemObjectString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    if (RtlValidRelativeSecurityDescriptor(
        SecurityDescriptor,
        PhLengthSecurityDescriptor(SecurityDescriptor),
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION
        ))
    {
        relativeSecurityDescriptor = SecurityDescriptor;
        relativeSecurityDescriptorLength = PhLengthSecurityDescriptor(SecurityDescriptor);
    }
    else
    {
        NTSTATUS ntstatus;

        ntstatus = RtlAbsoluteToSelfRelativeSD(
            SecurityDescriptor,
            NULL,
            &relativeSecurityDescriptorLength
            );

        if (ntstatus != STATUS_BUFFER_TOO_SMALL)
        {
            // Note: HR>WIN32>NT required for correct WMI error messages (dmex)
            status = HRESULT_FROM_WIN32(PhNtStatusToDosError(ntstatus));
            goto CleanupExit;
        }

        relativeSecurityDescriptor = PhAllocate(relativeSecurityDescriptorLength);
        ntstatus = RtlAbsoluteToSelfRelativeSD(
            SecurityDescriptor,
            relativeSecurityDescriptor,
            &relativeSecurityDescriptorLength
            );

        if (!NT_SUCCESS(ntstatus))
        {
            PhFree(relativeSecurityDescriptor);
            // Note: HR>WIN32>NT required for correct WMI error messages (dmex)
            status = HRESULT_FROM_WIN32(PhNtStatusToDosError(ntstatus));
            goto CleanupExit;
        }

        freeSecurityDescriptor = TRUE;
    }

    safeArrayBounds.lLbound = 0;
    safeArrayBounds.cElements = relativeSecurityDescriptorLength;

    if (!(safeArray = SafeArrayCreate(VT_UI1, 1, &safeArrayBounds)))
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    V_VT(&variantArrayValue) = VT_ARRAY | VT_UI1;
    V_ARRAY(&variantArrayValue) = safeArray;

    status = SafeArrayAccessData(safeArray, &safeArrayData);

    if (HR_FAILED(status))
        goto CleanupExit;

    memcpy(
        safeArrayData,
        relativeSecurityDescriptor,
        relativeSecurityDescriptorLength
        );

    status = SafeArrayUnaccessData(safeArray);

    if (HR_FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Put(
        wbemClassObject,
        L"SD",
        0,
        &variantArrayValue,
        CIM_EMPTY
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemMethodString = PhStringZToBSTR(L"SetSD");
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemObjectString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemSetSDClassObject,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemSetSDClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    if (V_UI4(&variantReturnValue) != ERROR_SUCCESS)
    {
        status = HRESULT_FROM_WIN32(V_UI4(&variantReturnValue));
    }

CleanupExit:
    if (wbemSetSDClassObject)
        IWbemClassObject_Release(wbemSetSDClassObject);
    if (wbemClassObject)
        IWbemClassObject_Release(wbemClassObject);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    if (freeSecurityDescriptor && relativeSecurityDescriptor)
        PhFree(relativeSecurityDescriptor);

    VariantClear(&variantArrayValue);
    //if (safeArray) SafeArrayDestroy(safeArray);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemObjectString)
        SysFreeString(wbemObjectString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);

    if (HR_SUCCESS(status))
    {
        return STATUS_SUCCESS;
    }

    if (status == WBEM_E_ACCESS_DENIED)
        return STATUS_ACCESS_DENIED;
    if (status == WBEM_E_INVALID_PARAMETER)
        return STATUS_INVALID_PARAMETER;

    return STATUS_INVALID_SECURITY_DESCR;
}

static PH_STRINGREF OleKeyName = PH_STRINGREF_INIT(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Ole");
static PH_STRINGREF OlePermissionValueName[] =
{
    PH_STRINGREF_INIT(L"DefaultLaunchPermission"),
    PH_STRINGREF_INIT(L"DefaultAccessPermission"),
    PH_STRINGREF_INIT(L"MachineLaunchRestriction"),
    PH_STRINGREF_INIT(L"MachineAccessRestriction"),
};

/**
 * Read a COM access/launch policy security descriptor override from the registry.
 *
 * \param ComSDType The type of a security descriptor to read.
 * \param SecurityDescriptor A security descriptor buffer.
 */
NTSTATUS PhGetComSecurityDescriptorOverride(
    _In_ COMSD ComSDType,
    _Outptr_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    NTSTATUS status;
    HANDLE keyHandle = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION value = NULL;

    if (ComSDType >= RTL_NUMBER_OF(OlePermissionValueName))
        return STATUS_INVALID_PARAMETER;

    status = PhOpenKey(
        &keyHandle,
        KEY_QUERY_VALUE,
        NULL,
        &OleKeyName,
        OBJ_CASE_INSENSITIVE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhQueryValueKey(
        keyHandle,
        &OlePermissionValueName[ComSDType],
        KeyValuePartialInformation,
        &value
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!RtlValidRelativeSecurityDescriptor(
        (PSECURITY_DESCRIPTOR)value->Data,
        value->DataLength,
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION
        ))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        goto CleanupExit;
    }

    *SecurityDescriptor = PhAllocateCopy(value->Data, value->DataLength);

CleanupExit:

    if (keyHandle)
        NtClose(keyHandle);

    if (value)
        PhFree(value);

    return status;
}

/**
 * Query a COM access/launch policy security descriptor.
 *
 * \param ComSDType The type of a security descriptor to query.
 * \param SecurityDescriptor A security descriptor buffer.
 */
NTSTATUS PhGetComSecurityDescriptor(
    _In_ COMSD ComSDType,
    _Outptr_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    NTSTATUS status;
    HRESULT hresult;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;

    status = PhGetComSecurityDescriptorOverride(ComSDType, SecurityDescriptor);

    if (NT_SUCCESS(status) || (status != STATUS_OBJECT_NAME_NOT_FOUND))
        return status;

    hresult = CoGetSystemSecurityPermissions(ComSDType, &securityDescriptor);

    if (HR_FAILED(hresult))
    {
        switch (hresult)
        {
        case E_ACCESSDENIED:
            return STATUS_ACCESS_DENIED;
        case E_OUTOFMEMORY:
            return STATUS_NO_MEMORY;
        case E_INVALIDARG:
            return STATUS_INVALID_PARAMETER;
        }

        return STATUS_UNSUCCESSFUL;
    }

    if (!RtlValidSecurityDescriptor(securityDescriptor))
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        goto CleanupExit;
    }

    *SecurityDescriptor = PhAllocateCopy(
        securityDescriptor,
        PhLengthSecurityDescriptor(securityDescriptor)
        );

CleanupExit:
    if (securityDescriptor)
        LocalFree(securityDescriptor);

    return STATUS_SUCCESS;
}

/**
 * Adjust a COM access/launch policy security descriptor.
 *
 * \param ComSDType The type of a security descriptor to read.
 * \param SecurityInformation The security information to change.
 * \param SecurityDescriptor A security descriptor buffer.
 */
NTSTATUS PhSetComSecurityDescriptor(
    _In_ COMSD ComSDType,
    _In_ ULONG SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS status;
    HANDLE keyHandle = NULL;
    PSECURITY_DESCRIPTOR originalSecurityDescriptor = NULL;
    PSECURITY_DESCRIPTOR mergedSecurityDescriptor = NULL;

    if (ComSDType >= RTL_NUMBER_OF(OlePermissionValueName))
        return STATUS_INVALID_PARAMETER;

    status = PhOpenKey(
        &keyHandle,
        KEY_SET_VALUE,
        NULL,
        &OleKeyName,
        OBJ_CASE_INSENSITIVE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetComSecurityDescriptor(ComSDType, &originalSecurityDescriptor);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhMergeSecurityDescriptors(
        originalSecurityDescriptor,
        SecurityDescriptor,
        SecurityInformation,
        &mergedSecurityDescriptor
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhSetValueKey(
        keyHandle,
        &OlePermissionValueName[ComSDType],
        REG_BINARY,
        mergedSecurityDescriptor,
        PhLengthSecurityDescriptor(mergedSecurityDescriptor)
        );

    if (NT_SUCCESS(status))
    {
        // Notify RPCSS about the change
        if (!UpdateDCOMSettings_I)
        {
            PVOID imageBaseAddress;

            if (imageBaseAddress = PhpInitializeComSecurityApi())
            {
                UpdateDCOMSettings_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "UpdateDCOMSettings", 0);
            }
        }

        if (UpdateDCOMSettings_I)
            UpdateDCOMSettings_I();
    }

CleanupExit:
    if (keyHandle)
        NtClose(keyHandle);

    if (originalSecurityDescriptor)
        PhFree(originalSecurityDescriptor);

    if (mergedSecurityDescriptor)
        PhFree(mergedSecurityDescriptor);

    return status;
}

#if defined(PHLIB_WBEM_DEPRECATED)
HRESULT PhRestartDefenderOfflineScan(
    VOID
    )
{
    HRESULT status;
    BSTR wbemResourceString = NULL;
    BSTR wbemObjectString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemStartClassObject = NULL;
    VARIANT variantReturnValue = { 0 };

    status = PhGetWbemLocatorClass(
        &wbemLocator
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemResourceString = PhStringZToBSTR(L"Root\\Microsoft\\Windows\\Defender");
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

    if (HR_FAILED(status))
        goto CleanupExit;

    status = PhCoSetProxyBlanket(
        (IUnknown*)wbemServices
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemObjectString = PhStringZToBSTR(L"MSFT_MpWDOScan");
    status = IWbemServices_GetObject(
        wbemServices,
        wbemObjectString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemMethodString = PhStringZToBSTR(L"Start");
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemObjectString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemStartClassObject,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemStartClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    if (V_UI4(&variantReturnValue) != ERROR_SUCCESS)
    {
        status = HRESULT_FROM_WIN32(V_UI4(&variantReturnValue));
        goto CleanupExit;
    }

CleanupExit:
    if (wbemStartClassObject)
        IWbemClassObject_Release(wbemStartClassObject);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemClassObject)
        IWbemClassObject_Release(wbemClassObject);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemObjectString)
        SysFreeString(wbemObjectString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);

    return status;
}
#else
HRESULT PhRestartDefenderOfflineScan(
    VOID
    )
{
    MI_Result miResult;
    MI_Application application;
    MI_Session session;
    MI_Operation operation = MI_OPERATION_NULL;
    MI_Instance* instance = NULL;
    const MI_Instance* completionDetails = NULL;
    const MI_Char* errorMessage = NULL;
    MI_Boolean moreResults = MI_FALSE;
    MI_Result operationResult = MI_RESULT_OK;
    MI_Value value;

    HRESULT status = S_OK;
    NTSTATUS subStatus;

    //
    // Initialize the MI session
    //
    if (!MI_Application_Initialize_I)
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpInitializeManagementInfrastructureApi())
        {
            MI_Application_Initialize_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "MI_Application_InitializeV1", 0);
        }
    }

    if (!MI_Application_Initialize_I)
        return E_FAIL;

    //
    // Initialize the MI session
    //
    miResult = MI_Application_Initialize_I(
        0,
        NULL,
        NULL,
        &application
        );

    if (miResult != MI_RESULT_OK)
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(PhMiResultToNtStatus(miResult)));

    //
    // Connect to local CIM session
    //
    miResult = MI_Application_NewSession(
        &application,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &session
        );

    if (miResult != MI_RESULT_OK)
    {
        MI_Application_Close(&application);
        return HRESULT_FROM_WIN32(PhNtStatusToDosError(PhMiResultToNtStatus(miResult)));
    }

    //
    // Invoke static method: MSFT_MpWDOScan.Start
    //
    MI_Session_Invoke(
        &session,
        0,
        NULL,
        L"root\\Microsoft\\Windows\\Defender",
        L"MSFT_MpWDOScan",
        L"Start",
        NULL,
        NULL,
        NULL,
        &operation
        );

    if (miResult != MI_RESULT_OK)
    {
        status = E_FAIL;
        goto Cleanup;
    }

    //
    // Pull results
    //
    for (;;)
    {
        miResult = MI_Operation_GetInstance(
            &operation,
            &instance,
            &moreResults,
            &operationResult,
            &errorMessage,
            &completionDetails
            );

        //
        // Transport or framework failure
        //
        if (miResult != MI_RESULT_OK)
        {
            subStatus = PhMiResultToNtStatus(miResult);
            status = HRESULT_FROM_WIN32(PhNtStatusToDosError(subStatus));
            break;
        }

        //
        // Completion reached (no more instances)
        //
        if (!instance)
            break;

        //
        // Extract ReturnValue
        //
        if (MI_Instance_GetElement(
            instance,
            L"ReturnValue",
            &value,
            NULL,
            NULL,
            NULL
            ) == MI_RESULT_OK)
        {
            if (value.uint32 != ERROR_SUCCESS)
            {
                status = HRESULT_FROM_WIN32(value.uint32);
            }
        }

        if (!moreResults)
            break;
    }

    //
    // Provider-level failure without instance
    //
    if (operationResult != MI_RESULT_OK && status == S_OK)
    {
        subStatus = PhMiResultToNtStatus(operationResult);
        status = HRESULT_FROM_WIN32(PhNtStatusToDosError(subStatus));
    }

Cleanup:
    MI_Operation_Close(&operation);
    MI_Session_Close(&session, NULL, NULL);
    MI_Application_Close(&application);

    return status;
}
#endif

#if defined(PHLIB_WBEM_DEPRECATED)
NTSTATUS PhWbemProcessExecutableRundown(
    _In_opt_ HANDLE ProcessId,
    _In_ PPH_ENUM_PROCESS_MODULES_RUNDOWN_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    HRESULT status;
    BSTR wbemResourceString = NULL;
    BSTR wbemObjectString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IEnumWbemClassObject* wbemEnumClassObject = NULL;

    status = PhGetWbemLocatorClass(
        &wbemLocator
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemResourceString = PhStringZToBSTR(L"Root\\CIMV2");
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

    if (HR_FAILED(status))
        goto CleanupExit;

    status = PhCoSetProxyBlanket(
        (IUnknown*)wbemServices
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    wbemObjectString = PhStringZToBSTR(L"CIM_ProcessExecutable"); // KernelRundownGuid EVENT_TRACE_TYPE_FILENAME_RUNDOWN events
    status = IWbemServices_CreateInstanceEnum(
        wbemServices,
        wbemObjectString,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &wbemEnumClassObject
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    while (TRUE)
    {
        ULONG wbemClassObjectCount = 0;
        IWbemClassObject* wbemClassObjectArray[50];

        status = IEnumWbemClassObject_Next(
            wbemEnumClassObject,
            WBEM_INFINITE,
            RTL_NUMBER_OF(wbemClassObjectArray),
            wbemClassObjectArray,
            &wbemClassObjectCount
            );

        if (wbemClassObjectCount == 0)
            break;

        for (ULONG i = 0; i < wbemClassObjectCount; i++)
        {
            PVOID baseAddress;
            PPH_STRING fileName = NULL;
            HANDLE processId = NULL;
            IWbemClassObject* wbemAntecedentObject;
            IWbemClassObject* wbemDependentObject;

            if (baseAddress = PhGetWbemClassObjectUlongPtr(wbemClassObjectArray[i], L"BaseAddress"))
            {
                if (HR_SUCCESS(PhGetWbemClassObjectDependency(
                    &wbemAntecedentObject,
                    wbemClassObjectArray[i],
                    wbemServices,
                    L"Antecedent" // returns CIM_DataFile & CIM_LogicalFile
                    )))
                {
                    fileName = PhGetWbemClassObjectString(wbemAntecedentObject, L"Name");
                    IWbemClassObject_Release(wbemAntecedentObject);
                }

                if (HR_SUCCESS(PhGetWbemClassObjectDependency(
                    &wbemDependentObject,
                    wbemClassObjectArray[i],
                    wbemServices,
                    L"Dependent" // returns Win32_Process
                    )))
                {
                    processId = PhGetWbemClassObjectUlongPtr(wbemDependentObject, L"ProcessId");
                    IWbemClassObject_Release(wbemDependentObject);
                }

                BOOLEAN stop = FALSE;

                if (ProcessId)
                {
                    if (ProcessId == processId)
                    {
                        if (!Callback(baseAddress, 0, fileName, Context))
                            stop = TRUE;
                    }
                }
                else
                {
                    if (!Callback(baseAddress, 0, fileName, Context))
                        stop = TRUE;
                }

                PhClearReference(&fileName);

                if (stop)
                {
                    for (ULONG j = i; j < wbemClassObjectCount; j++)
                    {
                        IWbemClassObject_Release(wbemClassObjectArray[j]);
                    }
                    goto CleanupExit;
                }
            }

            IWbemClassObject_Release(wbemClassObjectArray[i]);
        }
    }

CleanupExit:
    if (wbemEnumClassObject)
        IEnumWbemClassObject_Release(wbemEnumClassObject);
    if (wbemServices)
        IWbemServices_Release(wbemServices);
    if (wbemLocator)
        IWbemLocator_Release(wbemLocator);

    if (wbemObjectString)
        SysFreeString(wbemObjectString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);

    if (status == WBEM_E_ACCESS_DENIED)
        return STATUS_ACCESS_DENIED;
    if (status == WBEM_E_INVALID_PARAMETER)
        return STATUS_INVALID_PARAMETER;
    if (status != WBEM_S_NO_ERROR)
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}
#else
NTSTATUS PhWbemProcessExecutableRundown(
    _In_opt_ HANDLE ProcessId,
    _In_ PPH_ENUM_PROCESS_MODULES_RUNDOWN_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    MI_Result miResult;
    MI_Application application;
    MI_Session session;
    MI_Operation operation = MI_OPERATION_NULL;
    MI_Instance* instance = NULL;
    const MI_Instance* completionDetails = NULL;
    const MI_Char* errorMessage = NULL;
    MI_Boolean moreResults = MI_FALSE;
    MI_Result operationResult = MI_RESULT_OK;
    NTSTATUS status = STATUS_SUCCESS;

    if (!MI_Application_Initialize_I)
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpInitializeManagementInfrastructureApi())
            MI_Application_Initialize_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "MI_Application_InitializeV1", 0);
    }

    if (!MI_Application_Initialize_I)
        return STATUS_DLL_NOT_FOUND;

    miResult = MI_Application_Initialize_I(
        0,
        NULL,
        NULL,
        &application
        );

    if (miResult != MI_RESULT_OK)
        return PhMiResultToNtStatus(miResult);

    miResult = MI_Application_NewSession(
        &application,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &session
        );

    if (miResult != MI_RESULT_OK)
    {
        MI_Application_Close(&application);
        return PhMiResultToNtStatus(miResult);
    }

    MI_Session_QueryInstances(
        &session,
        0,
        NULL,
        L"root\\CIMV2",
        L"WQL",
        L"SELECT * FROM CIM_ProcessExecutable",
        NULL,
        &operation
        );

    for (;;)
    {
        miResult = MI_Operation_GetInstance(
            &operation,
            &instance,
            &moreResults,
            &operationResult,
            &errorMessage,
            &completionDetails
            );

        if (miResult != MI_RESULT_OK)
        {
            status = PhMiResultToNtStatus(miResult);
            break;
        }

        if (!instance)
            break;

        {
            PVOID baseAddress;
            PPH_STRING fileName = NULL;
            HANDLE processId = NULL;
            MI_Value value;
            MI_Type type;

            baseAddress = PhGetMiClassObjectUlongPtr(instance, L"BaseAddress");

            if (baseAddress &&
                MI_Instance_GetElement(
                instance,
                L"Antecedent",
                &value,
                &type,
                NULL,
                NULL
                ) == MI_RESULT_OK &&
                type == MI_REFERENCE &&
                value.reference)
            {
                fileName = PhGetMiClassObjectString(value.reference, L"Name");
            }

            if (baseAddress &&
                MI_Instance_GetElement(
                instance,
                L"Dependent",
                &value,
                &type,
                NULL,
                NULL
                ) == MI_RESULT_OK &&
                type == MI_REFERENCE &&
                value.reference)
            {
                processId = (HANDLE)PhGetMiClassObjectUlongPtr(value.reference, L"ProcessId");
            }

            if (baseAddress)
            {
                if (!ProcessId || ProcessId == processId)
                    status = Callback(baseAddress, 0, fileName, Context);
            }

            PhClearReference(&fileName);
        }

        if (!NT_SUCCESS(status) || !moreResults)
            break;
    }

    if (operationResult != MI_RESULT_OK && NT_SUCCESS(status))
        status = PhMiResultToNtStatus(operationResult);

    MI_Operation_Close(&operation);
    MI_Session_Close(&session, NULL, NULL);
    MI_Application_Close(&application);

    return status;
}

#endif
