/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2023
 *
 */

#include <ph.h>
#include <mapldr.h>

#include <wbemidl.h>
#include <wtsapi32.h>
#include <powrprof.h>

#include <secwmi.h>

DEFINE_GUID(CLSID_WbemLocator, 0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);
DEFINE_GUID(IID_IWbemLocator, 0xdc12a687, 0x737f, 0x11cf, 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);

_PowerGetActiveScheme PowerGetActiveScheme_I = NULL;
_PowerSetActiveScheme PowerSetActiveScheme_I = NULL;
_PowerRestoreDefaultPowerSchemes PowerRestoreDefaultPowerSchemes_I = NULL;
_PowerReadSecurityDescriptor PowerReadSecurityDescriptor_I = NULL;
_PowerWriteSecurityDescriptor PowerWriteSecurityDescriptor_I = NULL;
_WTSGetListenerSecurity WTSGetListenerSecurity_I = NULL;
_WTSSetListenerSecurity WTSSetListenerSecurity_I = NULL;

PVOID PhGetWbemProxImageBaseAddress(
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
            if (systemFileName = PhConcatStringRefZ(&systemDirectory->sr, L"\\wbem\\wbemprox.dll"))
            {
                imageBaseAddress = PhLoadLibrary(PhGetString(systemFileName));
                PhDereferenceObject(systemFileName);
            }

            PhDereferenceObject(systemDirectory);
        }

        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
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

// Power policy security descriptors

NTSTATUS PhpGetPowerPolicySecurityDescriptor(
    _Out_ PPH_STRING* StringSecurityDescriptor
    )
{
    ULONG status;
    PVOID imageBaseAddress;
    PWSTR stringSecurityDescriptor;
    PGUID policyGuid;

    if (!(imageBaseAddress = PhpInitializePowerPolicyApi()))
        return STATUS_DLL_NOT_FOUND;

    if (!PowerGetActiveScheme_I)
        PowerGetActiveScheme_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerGetActiveScheme", 0);
    if (!PowerReadSecurityDescriptor_I)
        PowerReadSecurityDescriptor_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerReadSecurityDescriptor", 0);

    if (!(PowerGetActiveScheme_I && PowerReadSecurityDescriptor_I))
        return STATUS_PROCEDURE_NOT_FOUND;

    // We can use GUIDs for schemes, policies or other values but we only query the default scheme. (dmex)
    status = PowerGetActiveScheme_I(
        NULL,
        &policyGuid
        );

    if (status != ERROR_SUCCESS)
        return NTSTATUS_FROM_WIN32(status);

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

    return NTSTATUS_FROM_WIN32(status);
}

NTSTATUS PhpSetPowerPolicySecurityDescriptor(
    _In_ PPH_STRING StringSecurityDescriptor
    )
{
    ULONG status;
    PVOID imageBaseAddress;
    PGUID policyGuid;

    if (!(imageBaseAddress = PhpInitializePowerPolicyApi()))
        return STATUS_DLL_NOT_FOUND;

    if (!PowerGetActiveScheme_I)
        PowerGetActiveScheme_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerGetActiveScheme", 0);
    if (!PowerWriteSecurityDescriptor_I)
        PowerWriteSecurityDescriptor_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "PowerWriteSecurityDescriptor", 0);

    if (!(PowerGetActiveScheme_I && PowerWriteSecurityDescriptor_I))
        return STATUS_PROCEDURE_NOT_FOUND;

    status = PowerGetActiveScheme_I(
        NULL,
        &policyGuid
        );

    if (status != ERROR_SUCCESS)
        return NTSTATUS_FROM_WIN32(status);

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

    return NTSTATUS_FROM_WIN32(status);
}

// Terminal server security descriptors

NTSTATUS PhpGetRemoteDesktopSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    BOOL status;
    PVOID imageBaseAddress;
    ULONG securityDescriptorLength = 0;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;

    if (!(imageBaseAddress = PhpInitializeRemoteDesktopServiceApi()))
        return STATUS_DLL_NOT_FOUND;

    if (!WTSGetListenerSecurity_I)
        WTSGetListenerSecurity_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "WTSGetListenerSecurityW", 0);

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

    return STATUS_INVALID_SECURITY_DESCR;
}

NTSTATUS PhpSetRemoteDesktopSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PVOID imageBaseAddress;

    if (!(imageBaseAddress = PhpInitializeRemoteDesktopServiceApi()))
        return STATUS_DLL_NOT_FOUND;

    if (!WTSSetListenerSecurity_I)
        WTSSetListenerSecurity_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "WTSSetListenerSecurityW", 0);

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

NTSTATUS PhGetWmiNamespaceSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    HRESULT status;
    PVOID imageBaseAddress;
    PVOID securityDescriptor = NULL;
    PVOID securityDescriptorData = NULL;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemQueryString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemGetSDClassObject = 0;
    VARIANT variantReturnValue = { VT_EMPTY };
    VARIANT variantArrayValue = { VT_EMPTY };

    if (!(imageBaseAddress = PhGetWbemProxImageBaseAddress()))
        return STATUS_UNSUCCESSFUL;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocStringLen(L"Root", 4);
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

    wbemQueryString = SysAllocStringLen(L"__SystemSecurity", 16);
    status = IWbemServices_GetObject(
        wbemServices,
        wbemQueryString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemMethodString = SysAllocStringLen(L"GetSD", 5);
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemQueryString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemGetSDClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemGetSDClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (FAILED(status))
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

    if (FAILED(status))
        goto CleanupExit;

    status = SafeArrayAccessData(V_ARRAY(&variantArrayValue), &securityDescriptorData);

    if (FAILED(status))
        goto CleanupExit;

    // The Wbem security descriptor is relative but the ACUI
    // dialog automatically converts the descriptor for us
    // so we don't have to convert just validate. (dmex)

    if (RtlValidSecurityDescriptor(securityDescriptorData))
    {
        securityDescriptor = PhAllocateCopy(
            securityDescriptorData,
            RtlLengthSecurityDescriptor(securityDescriptorData)
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
    VariantClear(&variantReturnValue);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);

    if (SUCCEEDED(status) && securityDescriptor)
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

NTSTATUS PhSetWmiNamespaceSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    HRESULT status;
    PVOID imageBaseAddress;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemQueryString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemSetSDClassObject = NULL;
    PVOID safeArrayData = NULL;
    LPSAFEARRAY safeArray = NULL;
    SAFEARRAYBOUND safeArrayBounds;
    PSECURITY_DESCRIPTOR relativeSecurityDescriptor = 0;
    ULONG relativeSecurityDescriptorLength = 0;
    BOOLEAN freeSecurityDescriptor = FALSE;
    VARIANT variantArrayValue = { VT_EMPTY };
    VARIANT variantReturnValue = { VT_EMPTY };

    if (!(imageBaseAddress = PhGetWbemProxImageBaseAddress()))
        return STATUS_UNSUCCESSFUL;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocStringLen(L"Root", 4);
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

    wbemQueryString = SysAllocStringLen(L"__SystemSecurity", 16);
    status = IWbemServices_GetObject(
        wbemServices,
        wbemQueryString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    if (RtlValidRelativeSecurityDescriptor(
        SecurityDescriptor,
        RtlLengthSecurityDescriptor(SecurityDescriptor),
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION
        ))
    {
        relativeSecurityDescriptor = SecurityDescriptor;
        relativeSecurityDescriptorLength = RtlLengthSecurityDescriptor(SecurityDescriptor);
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
            status = HRESULT_FROM_NT(ntstatus);
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
            status = HRESULT_FROM_NT(ntstatus);
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

    status = SafeArrayAccessData(safeArray, &safeArrayData);

    if (FAILED(status))
        goto CleanupExit;

    memcpy(
        safeArrayData,
        relativeSecurityDescriptor,
        relativeSecurityDescriptorLength
        );

    status = SafeArrayUnaccessData(safeArray);

    if (FAILED(status))
        goto CleanupExit;

    V_VT(&variantArrayValue) = VT_ARRAY | VT_UI1;
    V_ARRAY(&variantArrayValue) = safeArray;

    status = IWbemClassObject_Put(
        wbemClassObject,
        L"SD",
        0,
        &variantArrayValue,
        CIM_EMPTY
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemMethodString = SysAllocStringLen(L"SetSD", 5);
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemQueryString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemSetSDClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemSetSDClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (FAILED(status))
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

    VariantClear(&variantReturnValue);
    VariantClear(&variantArrayValue);
    //if (safeArray) SafeArrayDestroy(safeArray);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);

    if (SUCCEEDED(status))
    {
        return STATUS_SUCCESS;
    }

    if (status == WBEM_E_ACCESS_DENIED)
        return STATUS_ACCESS_DENIED;
    if (status == WBEM_E_INVALID_PARAMETER)
        return STATUS_INVALID_PARAMETER;

    return STATUS_INVALID_SECURITY_DESCR;
}

HRESULT PhRestartDefenderOfflineScan(
    VOID
    )
{
    HRESULT status;
    PVOID imageBaseAddress;
    PPH_STRING querySelectString = NULL;
    BSTR wbemResourceString = NULL;
    BSTR wbemQueryString = NULL;
    BSTR wbemMethodString = NULL;
    IWbemLocator* wbemLocator = NULL;
    IWbemServices* wbemServices = NULL;
    IWbemClassObject* wbemClassObject = NULL;
    IWbemClassObject* wbemStartClassObject = 0;
    VARIANT variantReturnValue = { VT_EMPTY };

    if (!(imageBaseAddress = PhGetWbemProxImageBaseAddress()))
        return STATUS_UNSUCCESSFUL;

    status = PhGetClassObjectDllBase(
        imageBaseAddress,
        &CLSID_WbemLocator,
        &IID_IWbemLocator,
        &wbemLocator
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemResourceString = SysAllocStringLen(L"Root\\Microsoft\\Windows\\Defender", 31);
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

    status = CoSetProxyBlanket(
        (IUnknown*)wbemServices,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemQueryString = SysAllocStringLen(L"MSFT_MpWDOScan", 14);
    status = IWbemServices_GetObject(
        wbemServices,
        wbemQueryString,
        0,
        NULL,
        &wbemClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    wbemMethodString = SysAllocStringLen(L"Start", 5);
    status = IWbemServices_ExecMethod(
        wbemServices,
        wbemQueryString,
        wbemMethodString,
        0,
        NULL,
        wbemClassObject,
        &wbemStartClassObject,
        NULL
        );

    if (FAILED(status))
        goto CleanupExit;

    status = IWbemClassObject_Get(
        wbemStartClassObject,
        L"ReturnValue",
        0,
        &variantReturnValue,
        NULL,
        NULL
        );

    if (FAILED(status))
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

    VariantClear(&variantReturnValue);

    if (wbemMethodString)
        SysFreeString(wbemMethodString);
    if (wbemQueryString)
        SysFreeString(wbemQueryString);
    if (wbemResourceString)
        SysFreeString(wbemResourceString);
    if (querySelectString)
        PhDereferenceObject(querySelectString);

    return status;
}
