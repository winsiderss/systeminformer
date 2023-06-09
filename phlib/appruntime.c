/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2023
 *
 */

#include <ph.h>
#include <mapldr.h>
#include <appresolver.h>

#if (PH_NATIVE_WINDOWS_RUNTIME_STRING)
#pragma comment(lib, "runtimeobject.lib")
#include <roapi.h>
#include <winstring.h>
#endif

#ifdef __hstring_h__
C_ASSERT(sizeof(HSTRING_REFERENCE) == sizeof(HSTRING_HEADER));
#else
C_ASSERT(sizeof(HSTRING_REFERENCE) == sizeof(WSTRING_HEADER));
#endif

PPH_STRING PhCreateStringFromWindowsRuntimeString(
    _In_ HSTRING String
    )
{
#if (PH_NATIVE_WINDOWS_RUNTIME_STRING)
    UINT32 stringLength;
    PCWSTR string;

    if (string = PhGetWindowsRuntimeStringBuffer(String, &stringLength))
    {
        if (stringLength >= sizeof(UNICODE_NULL))
        {
            return PhCreateStringEx((PWCHAR)string, stringLength * sizeof(WCHAR));
        }
    }
#else
    HSTRING_INSTANCE* string = (HSTRING_INSTANCE*)String;

    if (string && string->Length >= sizeof(UNICODE_NULL))
    {
        return PhCreateStringEx((PWCHAR)string->Buffer, string->Length * sizeof(WCHAR));
    }
#endif

    return PhReferenceEmptyString();
}

// rev from WindowsCreateStringReference (dmex)
/**
 * \brief Creates a new string reference based on the specified string.
 *
 * \param SourceString A null-terminated string to use as the source for the new string.
 * \param String A pointer to the newly created string.
 *
 * \return Successful or errant status.
 */
HRESULT PhCreateWindowsRuntimeStringReference(
    _In_ PCWSTR SourceString,
    _Out_ PVOID String
    )
{
#if (PH_NATIVE_WINDOWS_RUNTIME_STRING)
    HSTRING stringHandle;

    return WindowsCreateStringReference(
        SourceString,
        (UINT32)PhCountStringZ((PWSTR)SourceString),
        String,
        &stringHandle
        );
#else
    HSTRING_REFERENCE* string = (HSTRING_REFERENCE*)String;

    string->Flags = HSTRING_REFERENCE_FLAG;
    string->Length = (UINT32)PhCountStringZ((PWSTR)SourceString);
    string->Buffer = SourceString;

    return S_OK;
#endif
}

// rev from WindowsCreateString (dmex)
/**
 * \brief Creates a new string based on the specified string.
 *
 * \param SourceString A null-terminated string to use as the source for the new string.
 * \param String A pointer to the newly created string.
 *
 * \return Successful or errant status.
 */
HRESULT PhCreateWindowsRuntimeString(
    _In_ PCWSTR SourceString,
    _Out_ HSTRING* String
    )
{
#if (PH_NATIVE_WINDOWS_RUNTIME_STRING)
    return WindowsCreateString(
        SourceString,
        (UINT32)PhCountStringZ((PWSTR)SourceString),
        String
        );
#else
    SIZE_T stringLength;
    SIZE_T bufferLength;
    HSTRING_INSTANCE* string;

    stringLength = PhCountStringZ((PWSTR)SourceString) * sizeof(WCHAR);
    bufferLength = sizeof(HSTRING_INSTANCE) + stringLength + sizeof(UNICODE_NULL);

    if (bufferLength > UINT_MAX)
        return MEM_E_INVALID_SIZE;

    string = RtlAllocateHeap(RtlProcessHeap(), 0, bufferLength);

    if (!string)
        return E_OUTOFMEMORY;

    assert(!(stringLength & 1));

    string->Flags = 0;
    string->ReferenceCount = 1;
    string->Buffer = string->Data;
    string->Length = (UINT32)stringLength / sizeof(WCHAR);
    memcpy(string->Data, SourceString, stringLength);
    string->Data[string->Length] = UNICODE_NULL;

    *String = (HSTRING)string;
    return S_OK;
#endif
}

// rev from WindowsDeleteString (dmex)
/**
 * \brief Decrements the reference count of a string.
 *
 * \param String The string to be deleted.
 */
VOID PhDeleteWindowsRuntimeString(
    _In_opt_ HSTRING String
    )
{
#if (PH_NATIVE_WINDOWS_RUNTIME_STRING)
    WindowsDeleteString(String);
#else
    HSTRING_INSTANCE* string = (HSTRING_INSTANCE*)String;
    LONG newRefCount;

    if (!string || BooleanFlagOn(string->Flags, HSTRING_REFERENCE_FLAG))
        return;

    newRefCount = InterlockedDecrement(&string->ReferenceCount);
    ASSUME_ASSERT(newRefCount >= 0);
    ASSUME_ASSERT(!(newRefCount < 0));

    if (newRefCount == 0)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, string);
    }
#endif
}

// rev from WindowsGetStringLen (dmex)
/**
 * \brief Gets the length, in Unicode characters, of the specified string.
 *
 * \param String The string to be counted.
 *
 * \return Successful or errant status.
 */
UINT32 PhGetWindowsRuntimeStringLength(
    _In_opt_ HSTRING String
    )
{
#if (PH_NATIVE_WINDOWS_RUNTIME_STRING)
    return WindowsGetStringLen(String);
#else
    HSTRING_INSTANCE* string = (HSTRING_INSTANCE*)String;

    if (string)
    {
        return string->Length;
    }

    return 0;
#endif
}

// rev from WindowsGetStringRawBuffer (dmex)
/**
 * \brief Retrieves the backing buffer for the specified string.
 *
 * \param String An optional string for which the backing buffer is to be retrieved. Can be NULL.
 * \param Length The number of Unicode characters in the backing buffer for String (including embedded null characters, but excluding the terminating null).
 *
 * \return A pointer to the buffer that provides the backing store for string, or the empty string if string is NULL or the empty string.
 */
PCWSTR PhGetWindowsRuntimeStringBuffer(
    _In_opt_ HSTRING String,
    _Out_opt_ PUINT32 Length
    )
{
#if (PH_NATIVE_WINDOWS_RUNTIME_STRING)
    return WindowsGetStringRawBuffer(String, Length);
#else
    HSTRING_INSTANCE* string = (HSTRING_INSTANCE*)String;

    if (string)
    {
        if (Length)
            *Length = string->Length;

        return string->Buffer;
    }
    else
    {
        if (Length)
            *Length = 0;

        return L"";
    }
#endif
}

#pragma region Cryptographic Buffer

#include <windows.security.cryptography.h>

// 320b7e22-3cb0-4cdf-8663-1d28910065eb
DEFINE_GUID(IID_ICryptographicBufferStatics, 0x320b7e22, 0x3cb0, 0x4cdf, 0x86, 0x63, 0x1d, 0x28, 0x91, 0x00, 0x65, 0xeb);

PPH_STRING PhCryptographicBufferToHexString(
    _In_ __x_ABI_CWindows_CStorage_CStreams_CIBuffer* Buffer
    )
{
    PPH_STRING string = NULL;
    __x_ABI_CWindows_CSecurity_CCryptography_CICryptographicBufferStatics* cryptographicBufferStatics;
    HSTRING cryptographicBufferHandle;

    if (SUCCEEDED(PhGetActivationFactory(
        L"CryptoWinRT.dll",
        RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer,
        &IID_ICryptographicBufferStatics,
        &cryptographicBufferStatics
        )))
    {
        if (SUCCEEDED(__x_ABI_CWindows_CSecurity_CCryptography_CICryptographicBufferStatics_EncodeToHexString(
            cryptographicBufferStatics,
            Buffer,
            &cryptographicBufferHandle
            )))
        {
            string = PhCreateStringFromWindowsRuntimeString(cryptographicBufferHandle);
            PhDeleteWindowsRuntimeString(cryptographicBufferHandle);
        }

        __x_ABI_CWindows_CSecurity_CCryptography_CICryptographicBufferStatics_Release(cryptographicBufferStatics);
    }

    return string;
}

#pragma endregion

#pragma region Data Reader

// 11fcbfc8-f93a-471b-b121-f379e349313c
DEFINE_GUID(IID_IDataReaderStatics, 0x11fcbfc8, 0xf93a, 0x471b, 0xb1, 0x21, 0xf3, 0x79, 0xe3, 0x49, 0x31, 0x3c);

PPH_STRING PhDataReaderBufferToHexString(
    _In_ __x_ABI_CWindows_CStorage_CStreams_CIBuffer* Buffer
    )
{
    PPH_STRING string = NULL;
    __x_ABI_CWindows_CStorage_CStreams_CIDataReaderStatics* dataReaderStatics;
    __x_ABI_CWindows_CStorage_CStreams_CIDataReader* dataReader;
    UINT32 dataBufferLength = 0;
    UCHAR dataBuffer[128] = { 0 };

    if (SUCCEEDED(__x_ABI_CWindows_CStorage_CStreams_CIBuffer_get_Length(Buffer, &dataBufferLength)) && dataBufferLength < sizeof(dataBuffer))
    {
        if (SUCCEEDED(PhGetActivationFactory(
            L"WinTypes.dll",
            RuntimeClass_Windows_Storage_Streams_DataReader,
            &IID_IDataReaderStatics,
            &dataReaderStatics
            )))
        {
            if (SUCCEEDED(__x_ABI_CWindows_CStorage_CStreams_CIDataReaderStatics_FromBuffer(dataReaderStatics, Buffer, &dataReader)))
            {
                if (SUCCEEDED(__x_ABI_CWindows_CStorage_CStreams_CIDataReader_ReadBytes(dataReader, dataBufferLength, dataBuffer)))
                {
                    string = PhBufferToHexString(dataBuffer, dataBufferLength);
                }

                __x_ABI_CWindows_CStorage_CStreams_CIDataReader_Release(dataReader);
            }

            __x_ABI_CWindows_CStorage_CStreams_CIDataReaderStatics_Release(dataReaderStatics);
        }
    }

    return string;
}

#pragma endregion

#pragma region System Identification

#include <windows.system.profile.h>

// 5581f42a-d3df-4d93-a37d-c41a616c6d01
DEFINE_GUID(IID_ISystemIdentificationStatics, 0x5581f42a, 0xd3df, 0x4d93, 0xa3, 0x7d, 0xc4, 0x1a, 0x61, 0x6c, 0x6d, 0x01);

PPH_STRING PhSystemIdentificationToString(
    _In_ __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationInfo* IdentificationInfo
    )
{
    __x_ABI_CWindows_CSystem_CProfile_CSystemIdentificationSource source;
    __x_ABI_CWindows_CStorage_CStreams_CIBuffer* buffer;
    PPH_STRING identifier = NULL;
    PCWSTR string = L"Unknown";

    if (SUCCEEDED(__x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationInfo_get_Id(IdentificationInfo, &buffer)))
    {
        identifier = PhCryptographicBufferToHexString(buffer);

        if (PhIsNullOrEmptyString(identifier))
        {
            identifier = PhDataReaderBufferToHexString(buffer);
        }

        __x_ABI_CWindows_CStorage_CStreams_CIBuffer_Release(buffer);
    }

    if (SUCCEEDED(__x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationInfo_get_Source(IdentificationInfo, &source)))
    {
        switch (source)
        {
        case SystemIdentificationSource_Tpm:
            string = L"TPM";
            break;
        case SystemIdentificationSource_Uefi:
            string = L"UEFI";
            break;
        case SystemIdentificationSource_Registry:
            string = L"Registry";
            break;
        }
    }

    PhMoveReference(&identifier, PhFormatString(
        L"%s (%s)",
        PhGetStringOrDefault(identifier, L"Unknown"),
        string
        ));

    return identifier;
}

typedef struct _PH_APPHWID_QUERY_CONTEXT
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
} PH_APPHWID_QUERY_CONTEXT, *PPH_APPHWID_QUERY_CONTEXT;

static PVOID PhDetoursPackageSystemIdentificationContext(
    _In_opt_ PPH_APPHWID_QUERY_CONTEXT Buffer,
    _In_ BOOLEAN Initialize
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static ULONG index = 0;

    if (PhBeginInitOnce(&initOnce))
    {
        index = TlsAlloc();
        PhEndInitOnce(&initOnce);
    }

    if (index != TLS_OUT_OF_INDEXES)
    {
        if (Initialize)
        {
            if (TlsSetValue(index, Buffer))
                return Buffer;
        }
        else
        {
            return TlsGetValue(index);
        }
    }

    return NULL;
}

static HANDLE WINAPI PhDetoursPackageSystemIdentificationCurrentProcess(
    VOID
    )
{
    PPH_APPHWID_QUERY_CONTEXT context = PhDetoursPackageSystemIdentificationContext(NULL, FALSE);

    if (context)
        return context->ProcessHandle;
    else
        return NtCurrentProcess();
}

static HANDLE WINAPI PhDetoursPackageSystemIdentificationCurrentThread(
    VOID
    )
{
    PPH_APPHWID_QUERY_CONTEXT context = PhDetoursPackageSystemIdentificationContext(NULL, FALSE);

    if (context)
        return context->ThreadHandle;
    else
        return NtCurrentThread();
}

static ULONG WINAPI PhDetoursPackageSystemIdentificationCurrentProcessId(
    VOID
    )
{
    PPH_APPHWID_QUERY_CONTEXT context = PhDetoursPackageSystemIdentificationContext(NULL, FALSE);

    if (context)
        return HandleToUlong(context->ProcessId);
    else
        return HandleToUlong(NtCurrentProcessId());
}

static ULONG WINAPI PhDetoursPackageSystemIdentificationCurrentThreadId(
    VOID
    )
{
    PPH_APPHWID_QUERY_CONTEXT context = PhDetoursPackageSystemIdentificationContext(NULL, FALSE);

    if (context)
        return HandleToUlong(context->ThreadId);
    else
        return HandleToUlong(NtCurrentThreadId());
}

static BOOL WINAPI PhDetoursPackageSystemIdentificationCloseHandle(
    _In_ _Post_ptr_invalid_ HANDLE hObject
    )
{
    return TRUE;
}

static HRESULT PhDetoursPackageSystemIdentificationInitialize(
    _In_ PVOID Address,
    _In_ PPH_APPHWID_QUERY_CONTEXT Context
    )
{
    PVOID baseAddress = PhGetLoaderEntryAddressDllBase(Address);

    if (!baseAddress)
        return E_FAIL;

    if (!NT_SUCCESS(PhLoaderEntryDetourImportProcedure(
        baseAddress,
        "api-ms-win-core-processthreads-l1-1-0.dll",
        "GetCurrentProcess",
        PhDetoursPackageSystemIdentificationCurrentProcess,
        NULL
        )))
    {
        return E_FAIL;
    }

    if (!NT_SUCCESS(PhLoaderEntryDetourImportProcedure(
        baseAddress,
        "api-ms-win-core-processthreads-l1-1-0.dll",
        "GetCurrentProcessId",
        PhDetoursPackageSystemIdentificationCurrentProcessId,
        NULL
        )))
    {
        return E_FAIL;
    }

    if (!NT_SUCCESS(PhLoaderEntryDetourImportProcedure(
        baseAddress,
        "api-ms-win-core-processthreads-l1-1-0.dll",
        "GetCurrentThread",
        PhDetoursPackageSystemIdentificationCurrentThread,
        NULL
        )))
    {
        return E_FAIL;
    }

    if (!NT_SUCCESS(PhLoaderEntryDetourImportProcedure(
        baseAddress,
        "api-ms-win-core-processthreads-l1-1-0.dll",
        "GetCurrentThreadId",
        PhDetoursPackageSystemIdentificationCurrentThreadId,
        NULL
        )))
    {
        return E_FAIL;
    }

    if (!NT_SUCCESS(PhLoaderEntryDetourImportProcedure(
        baseAddress,
        "api-ms-win-core-handle-l1-1-0.dll",
        "CloseHandle",
        PhDetoursPackageSystemIdentificationCloseHandle,
        NULL
        )))
    {
        return E_FAIL;
    }

    if (!PhDetoursPackageSystemIdentificationContext(Context, TRUE))
    {
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT PhDetoursPackageSystemIdentificationDelete(
    VOID
    )
{
    if (!PhDetoursPackageSystemIdentificationContext(NULL, TRUE))
    {
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT PhQueryProcessSystemIdentification(
    _In_ PPH_APPHWID_QUERY_CONTEXT Context,
    _Out_ PPH_STRING* SystemIdForPublisher,
    _Out_ PPH_STRING* SystemIdForUser
    )
{
    HRESULT status;
    __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationStatics* systemIdStatics = NULL;
    __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationInfo* systemIdPublisher;
    __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationInfo* systemIdUser;
    BOOLEAN revertImpersonationToken = FALSE;
    HANDLE tokenHandle = NULL;
    HRESULT systemIdForUserStatus = E_FAIL;
    HRESULT systemIdPublisherStatus = E_FAIL;
    PPH_STRING systemIdForPublisherString = NULL;
    PPH_STRING systemIdForUserString = NULL;

    status = PhGetActivationFactory(
        L"Windows.System.Profile.SystemId.dll",
        RuntimeClass_Windows_System_Profile_SystemIdentification,
        &IID_ISystemIdentificationStatics,
        &systemIdStatics
        );

    if (FAILED(status))
        return status;

    if (NT_SUCCESS(PhOpenProcessToken(
        Context->ProcessHandle,
        TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE,
        &tokenHandle
        )))
    {
        revertImpersonationToken = NT_SUCCESS(PhImpersonateToken(NtCurrentThread(), tokenHandle));
    }

    // We've impersonated the package but the SystemIdentification class
    // gets confused and the package manager queries the impersonated token
    // and the WinRT class queries our token... Redirect the handle imports
    // so we can query the correct token. If we can't then unfortunately
    // developers can't verify their SystemID support returns correct values
    // or verify their capabilities/sandboxing/token permissions are setup correctly (dmex)

    status = PhDetoursPackageSystemIdentificationInitialize(
        systemIdStatics->lpVtbl,
        Context
        );

    if (FAILED(status))
        goto CleanupExit;

    if ((systemIdPublisherStatus = __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationStatics_GetSystemIdForPublisher(
        systemIdStatics,
        &systemIdPublisher
        )) == S_OK)
    {
        systemIdForPublisherString = PhSystemIdentificationToString(systemIdPublisher);
        __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationInfo_Release(systemIdPublisher);
    }

    if ((systemIdForUserStatus = __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationStatics_GetSystemIdForUser(
        systemIdStatics,
        NULL,
        &systemIdUser
        )) == S_OK)
    {
        systemIdForUserString = PhSystemIdentificationToString(systemIdUser);
        __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationInfo_Release(systemIdUser);
    }

CleanupExit:
    if (systemIdStatics)
    {
        __x_ABI_CWindows_CSystem_CProfile_CISystemIdentificationStatics_Release(systemIdStatics);
        systemIdStatics = NULL;
    }

    if (tokenHandle)
    {
        if (revertImpersonationToken)
            PhRevertImpersonationToken(NtCurrentThread());

        NtClose(tokenHandle);
        tokenHandle = NULL;
    }

    PhDetoursPackageSystemIdentificationDelete();

    *SystemIdForPublisher = NULL;
    *SystemIdForUser = NULL;

    // Note: Load messages after reverting impersonation otherwise the kernel32.dll message table doesn't get mapped.
    // Errors will be from the process token missing the "userSystemId" capability or limited process token privileges. (dmex)

    if (systemIdPublisherStatus == S_OK)
    {
        *SystemIdForPublisher = systemIdForPublisherString;
    }
    else
    {
        if (HRESULT_FACILITY(systemIdPublisherStatus) == FACILITY_NT_BIT >> NT_FACILITY_SHIFT)
        {
            ClearFlag(systemIdPublisherStatus, FACILITY_NT_BIT); // 0xD0000022 -> 0xC0000022
            *SystemIdForPublisher = PhGetStatusMessage(systemIdPublisherStatus, 0);
        }

        if (PhIsNullOrEmptyString(*SystemIdForPublisher))
        {
            *SystemIdForPublisher = PhGetStatusMessage(0, HRESULT_CODE(systemIdPublisherStatus));
        }
    }

    if (systemIdForUserStatus == S_OK)
    {
        *SystemIdForUser = systemIdForUserString;
    }
    else
    {
        if (HRESULT_FACILITY(systemIdForUserStatus) == FACILITY_NT_BIT >> NT_FACILITY_SHIFT)
        {
            ClearFlag(systemIdForUserStatus, FACILITY_NT_BIT); // 0xD0000022 -> 0xC0000022
            *SystemIdForUser = PhGetStatusMessage(systemIdForUserStatus, 0);
        }

        if (PhIsNullOrEmptyString(*SystemIdForUser))
        {
            *SystemIdForUser = PhGetStatusMessage(0, HRESULT_CODE(systemIdForUserStatus));
        }
    }

    return status;
}

HRESULT PhGetProcessSystemIdentification(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING* SystemIdForPublisher,
    _Out_ PPH_STRING* SystemIdForUser
    )
{
    HRESULT status;
    HANDLE processHandle;
    HANDLE threadHandle = NULL;
    HANDLE threadId = NULL;
    PVOID processes;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        ProcessId
        );

    if (!NT_SUCCESS(status))
    {
        *SystemIdForPublisher = NULL;
        *SystemIdForUser = NULL;

        return HRESULT_FROM_NT(status);
    }

    if (NT_SUCCESS(PhEnumProcesses(&processes)))
    {
        PSYSTEM_PROCESS_INFORMATION process;

        process = PhFindProcessInformation(processes, ProcessId);

        for (ULONG i = 0; i < process->NumberOfThreads; i++)
        {
            HANDLE tempThreadHandle;

            threadId = process->Threads[i].ClientId.UniqueThread;

            if (NT_SUCCESS(PhOpenThread(
                &tempThreadHandle,
                THREAD_QUERY_LIMITED_INFORMATION,
                threadId
                )))
            {
                threadHandle = tempThreadHandle;
                break;
            }
        }

        PhFree(processes);
    }

    if (!threadHandle)
    {
        status = HRESULT_FROM_NT(STATUS_UNSUCCESSFUL);
        goto CleanupExit;
    }

    {
        PH_APPHWID_QUERY_CONTEXT queryContext;

        memset(&queryContext, 0, sizeof(PH_APPHWID_QUERY_CONTEXT));
        queryContext.ProcessId = ProcessId;
        queryContext.ProcessHandle = processHandle;
        queryContext.ThreadId = threadId;
        queryContext.ThreadHandle = threadHandle;

        status = PhQueryProcessSystemIdentification(&queryContext, SystemIdForPublisher, SystemIdForUser);
    }

CleanupExit:
    if (threadHandle)
    {
        NtClose(threadHandle);
    }

    if (processHandle)
    {
        NtClose(processHandle);
    }

    return status;
}

#pragma endregion

#pragma region Display Information

// BED112AE-ADC3-4DC9-AE65-851F4D7D4799
DEFINE_GUID(IID_IDisplayInformation, 0xBED112AE, 0xADC3, 0x4DC9, 0xAE, 0x65, 0x85, 0x1F, 0x4D, 0x7D, 0x47, 0x99);
// C6A02A6C-D452-44DC-BA07-96F3C6ADF9D1
DEFINE_GUID(IID_IDisplayInformationStatics, 0xc6a02a6c, 0xd452, 0x44dc, 0xba, 0x07, 0x96, 0xf3, 0xc6, 0xad, 0xf9, 0xd1);
// 6937ED8D-30EA-4DED-8271-4553FF02F68A
DEFINE_GUID(IID_IDisplayPropertiesStatics, 0x6937ed8d, 0x30ea, 0x4ded, 0x82, 0x71, 0x45, 0x53, 0xff, 0x02, 0xf6, 0x8a);

FLOAT PhGetDisplayLogicalDpi(
    VOID
    )
{
    FLOAT displayLogicalDpi = 0;
    __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayPropertiesStatics* displayProperties;
    HRESULT status;

    status = PhGetActivationFactory(
        L"Windows.Graphics.dll",
        RuntimeClass_Windows_Graphics_Display_DisplayProperties,
        &IID_IDisplayPropertiesStatics,
        &displayProperties
        );

    if (SUCCEEDED(status))
    {
        FLOAT value;

        status = __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayPropertiesStatics_get_LogicalDpi(displayProperties, &value);

        if (SUCCEEDED(status))
        {
            displayLogicalDpi = value;
        }
    }

    // Requires IWindowsXamlManagerStatics_InitializeForCurrentThread (dmex)
    //
    //__x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics* displayInformation;
    //status = PhGetActivationFactory(
    //    L"Windows.Graphics.dll",
    //    RuntimeClass_Windows_Graphics_Display_DisplayInformation,
    //    &IID_IDisplayInformationStatics,
    //    &displayInformation
    //    );
    //
    //if (SUCCEEDED(status))
    //{
    //    __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation* display_information;
    //    __x_ABI_CWindows_CGraphics_CDisplay_CResolutionScale resolution_scale;
    //
    //    if (SUCCEEDED(__x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics_GetForCurrentView(displayInformation, &display_information)))
    //    {
    //        if (SUCCEEDED(__x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation_get_ResolutionScale(display_information, &resolution_scale)))
    //        {
    //            float scale = (float)(resolution_scale / 100.0f);
    //        }
    //    }
    //}

    return displayLogicalDpi;
}

#pragma endregion

#pragma region Package Manager

#include <Windows.Management.Deployment.h>

// 9a7d4b65-5e8f-4fc7-a2e5-7f6925cb8b53
DEFINE_GUID(IID_IPackageManager, 0x9A7D4B65, 0x5E8F, 0x4FC7, 0xA2, 0xE5, 0x7F, 0x69, 0x25, 0xCB, 0x8B, 0x53);
// a6612fb6-7688-4ace-95fb-359538e7aa01
DEFINE_GUID(IID_IPackage2, 0xA6612FB6, 0x7688, 0x4ACE, 0x95, 0xFB, 0x35, 0x95, 0x38, 0xE7, 0xAA, 0x01);
// 2c584f7b-ce2a-4be6-a093-77cfbb2a7ea1
DEFINE_GUID(IID_IPackage8, 0x2c584f7b, 0xce2a, 0x4be6, 0xa0, 0x93, 0x77, 0xcf, 0xbb, 0x2a, 0x7e, 0xa1);
// d0a618ad-bf35-42ac-ac06-86eeeb41d04b
DEFINE_GUID(IID_IAppListEntry2, 0xd0a618ad, 0xbf35, 0x42ac, 0xac, 0x06, 0x86, 0xee, 0xeb, 0x41, 0xd0, 0x4b);
// cf7f59b3-6a09-4de8-a6c0-5792d56880d1
DEFINE_GUID(IID_IAppInfo, 0xcf7f59b3, 0x6a09, 0x4de8, 0xa6, 0xc0, 0x57, 0x92, 0xd5, 0x68, 0x80, 0xd1);
// 4207a996-ca2f-42f7-bde8-8b10457a7f30
DEFINE_GUID(IID_IStorageItem, 0x4207a996, 0xca2f, 0x42f7, 0xbd, 0xe8, 0x8b, 0x10, 0x45, 0x7a, 0x7f, 0x30);

static _OpenPackageInfoByFullNameForUser OpenPackageInfoByFullNameForUser_I = NULL;
static _GetPackageApplicationIds GetPackageApplicationIds_I = NULL;
static _ClosePackageInfo ClosePackageInfo_I = NULL;

static BOOLEAN PhPackageImportsInitialized(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN initialized = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhGetLoaderEntryDllBaseZ(L"kernelbase.dll"))
        {
            OpenPackageInfoByFullNameForUser_I = PhGetDllBaseProcedureAddress(baseAddress, "OpenPackageInfoByFullNameForUser", 0);
            GetPackageApplicationIds_I = PhGetDllBaseProcedureAddress(baseAddress, "GetPackageApplicationIds", 0);
            ClosePackageInfo_I = PhGetDllBaseProcedureAddress(baseAddress, "ClosePackageInfo", 0);
        }

        if (
            OpenPackageInfoByFullNameForUser_I &&
            GetPackageApplicationIds_I &&
            ClosePackageInfo_I
            )
        {
            initialized = TRUE;
        }

        PhEndInitOnce(&initOnce);
    }

    return initialized;
}

_Success_(return)
BOOLEAN PhGetPackageApplicationIds(
    _In_ PCWSTR PackageFullName,
    _Out_ PWSTR** ApplicationUserModelIds,
    _Out_ PULONG ApplicationUserModelIdsCount
    )
{
    BOOLEAN success = FALSE;
    ULONG status;
    HANDLE packageHandle;
    ULONG bufferCount = 0;
    ULONG bufferLength = 0;
    PBYTE buffer = NULL;

    if (!PhPackageImportsInitialized())
        return FALSE;

    status = OpenPackageInfoByFullNameForUser_I(
        NULL,
        PackageFullName,
        0,
        &packageHandle
        );

    if (status != ERROR_SUCCESS)
        return FALSE;

    status = GetPackageApplicationIds_I(
        packageHandle,
        &bufferLength,
        NULL,
        NULL
        );

    if (status != ERROR_INSUFFICIENT_BUFFER)
        goto CleanupExit;

    buffer = PhAllocate(bufferLength);
    memset(buffer, 0, bufferLength);

    status = GetPackageApplicationIds_I(
        packageHandle,
        &bufferLength,
        buffer,
        &bufferCount
        );

    if (status != ERROR_SUCCESS)
        goto CleanupExit;

    success = TRUE;

CleanupExit:

    ClosePackageInfo_I(packageHandle);

    if (success)
    {
        *ApplicationUserModelIds = (PWSTR*)buffer;
        *ApplicationUserModelIdsCount = bufferCount;
    }
    else
    {
        if (buffer)
        {
            PhFree(buffer);
        }
    }

    return success;
}

typedef enum _PH_QUERY_PACKAGE_INFO_TYPE
{
    PH_QUERY_PACKAGE_INFO_NAME = 1,
    PH_QUERY_PACKAGE_INFO_FULLNAME = 2,
    PH_QUERY_PACKAGE_INFO_FAMILYNAME = 4,
    PH_QUERY_PACKAGE_INFO_DISPLAYNAME = 8,
    PH_QUERY_PACKAGE_INFO_VERSION = 16,
    PH_QUERY_PACKAGE_INFO_LOGO = 32,
    PH_QUERY_PACKAGE_INFO_LOCATION = 64,
} PH_QUERY_PACKAGE_INFO_TYPE;

BOOLEAN PhQueryApplicationModelPackageInformation(
    _In_ PPH_LIST PackageList,
    _In_ __x_ABI_CWindows_CApplicationModel_CIPackage* AppPackage,
    _In_ __x_ABI_CWindows_CApplicationModel_CIPackage2* AppPackage2,
    _In_ PH_QUERY_PACKAGE_INFO_TYPE FilterType
    )
{
    __x_ABI_CWindows_CStorage_CIStorageFolder* appPackageFolder;
    __x_ABI_CWindows_CStorage_CIStorageItem* appPackageFolderItem;
    __x_ABI_CWindows_CApplicationModel_CIPackageId* appPackageID;
    __x_ABI_CWindows_CFoundation_CIUriRuntimeClass* appPackageLogo;
    PPH_STRING packageInstallLocationString = NULL;
    PPH_STRING packageNameString = NULL;
    PPH_STRING packageFullNameString = NULL;
    PPH_STRING packageFamilyNameString = NULL;
    PPH_STRING packageDisplayNameString = NULL;
    PPH_STRING packageLogoString = NULL;
    PPH_STRING packageVersionString = NULL;
    HSTRING stringHandle = NULL;

    if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackage_get_Id(AppPackage, &appPackageID)))
    {
        if (FlagOn(FilterType, PH_QUERY_PACKAGE_INFO_NAME))
        {
            if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackageId_get_Name(appPackageID, &stringHandle)))
            {
                if (PhGetWindowsRuntimeStringLength(stringHandle))
                {
                    packageNameString = PhCreateStringFromWindowsRuntimeString(stringHandle);
                }

                PhDeleteWindowsRuntimeString(stringHandle);
            }
        }

        if (FlagOn(FilterType, PH_QUERY_PACKAGE_INFO_FULLNAME))
        {
            if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackageId_get_FullName(appPackageID, &stringHandle)))
            {
                if (PhGetWindowsRuntimeStringLength(stringHandle))
                {
                    packageFullNameString = PhCreateStringFromWindowsRuntimeString(stringHandle);
                }

                PhDeleteWindowsRuntimeString(stringHandle);
            }
        }

        if (FlagOn(FilterType, PH_QUERY_PACKAGE_INFO_FAMILYNAME))
        {
            if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackageId_get_FamilyName(appPackageID, &stringHandle)))
            {
                if (PhGetWindowsRuntimeStringLength(stringHandle))
                {
                    packageFamilyNameString = PhCreateStringFromWindowsRuntimeString(stringHandle);
                }

                PhDeleteWindowsRuntimeString(stringHandle);
            }
        }

        if (FlagOn(FilterType, PH_QUERY_PACKAGE_INFO_VERSION))
        {
            __x_ABI_CWindows_CApplicationModel_CPackageVersion appPackageVersion = { 0 };

            if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackageId_get_Version(appPackageID, &appPackageVersion)))
            {
                PH_FORMAT format[7];

                PhInitFormatU(&format[0], appPackageVersion.Major);
                PhInitFormatC(&format[1], L'.');
                PhInitFormatU(&format[2], appPackageVersion.Minor);
                PhInitFormatC(&format[3], L'.');
                PhInitFormatU(&format[4], appPackageVersion.Revision);
                PhInitFormatC(&format[5], L'.');
                PhInitFormatU(&format[6], appPackageVersion.Build);

                packageVersionString = PhFormat(format,RTL_NUMBER_OF(format), 0);
            }
        }

        __x_ABI_CWindows_CApplicationModel_CIPackageId_Release(appPackageID);
    }

    if (FlagOn(FilterType, PH_QUERY_PACKAGE_INFO_DISPLAYNAME))
    {
        if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackage2_get_DisplayName(AppPackage2, &stringHandle)))
        {
            if (PhGetWindowsRuntimeStringLength(stringHandle))
            {
                packageDisplayNameString = PhCreateStringFromWindowsRuntimeString(stringHandle);
            }

            PhDeleteWindowsRuntimeString(stringHandle);
        }
    }

    if (FlagOn(FilterType, PH_QUERY_PACKAGE_INFO_LOGO))
    {
        if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackage2_get_Logo(AppPackage2, &appPackageLogo)))
        {
            if (SUCCEEDED(__x_ABI_CWindows_CFoundation_CIUriRuntimeClass_get_RawUri(appPackageLogo, &stringHandle)))
            {
                packageLogoString = PhCreateStringFromWindowsRuntimeString(stringHandle);
                PhDeleteWindowsRuntimeString(stringHandle);
            }

            __x_ABI_CWindows_CFoundation_CIUriRuntimeClass_Release(appPackageLogo);
        }
    }

    if (FlagOn(FilterType, PH_QUERY_PACKAGE_INFO_LOCATION))
    {
        if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackage_get_InstalledLocation(AppPackage, &appPackageFolder)))
        {
            if (SUCCEEDED(__x_ABI_CWindows_CStorage_CIStorageFolder_QueryInterface(appPackageFolder, &IID_IStorageItem, &appPackageFolderItem)))
            {
                if (SUCCEEDED(__x_ABI_CWindows_CStorage_CIStorageItem_get_Path(appPackageFolderItem, &stringHandle)))
                {
                    if (PhGetWindowsRuntimeStringLength(stringHandle))
                    {
                        packageInstallLocationString = PhCreateStringFromWindowsRuntimeString(stringHandle);
                    }
                    PhDeleteWindowsRuntimeString(stringHandle);
                }

                __x_ABI_CWindows_CStorage_CIStorageItem_Release(appPackageFolderItem);
            }

            __x_ABI_CWindows_CStorage_CIStorageFolder_Release(appPackageFolder);
        }
    }

    if (packageFullNameString)
    {
        ULONG applicationUserModelIdsCount;
        PWSTR* applicationUserModelIdsBuffer;

        if (PhGetPackageApplicationIds(
            PhGetString(packageFullNameString),
            &applicationUserModelIdsBuffer,
            &applicationUserModelIdsCount
            ))
        {
            for (ULONG i = 0; i < applicationUserModelIdsCount; ++i)
            {
                PPH_APPUSERMODELID_ENUM_ENTRY entry;

                entry = PhAllocateZero(sizeof(PH_APPUSERMODELID_ENUM_ENTRY));
                entry->AppUserModelId = PhCreateString(applicationUserModelIdsBuffer[i]);
                PhSetReference(&entry->PackageName, packageNameString);
                PhSetReference(&entry->PackageDisplayName, packageDisplayNameString);
                PhSetReference(&entry->PackageFamilyName, packageFamilyNameString);
                PhSetReference(&entry->PackageInstallPath, packageInstallLocationString);
                PhSetReference(&entry->PackageFullName, packageFullNameString);
                PhSetReference(&entry->PackageVersion, packageVersionString);
                PhSetReference(&entry->SmallLogoPath, packageLogoString);

                PhAddItemList(PackageList, entry);
            }
        }
    }

    PhClearReference(&packageLogoString);
    PhClearReference(&packageVersionString);
    PhClearReference(&packageFullNameString);
    PhClearReference(&packageInstallLocationString);
    PhClearReference(&packageFamilyNameString);
    PhClearReference(&packageDisplayNameString);
    PhClearReference(&packageNameString);

    return TRUE;
}

PPH_LIST PhEnumPackageApplicationUserModelIds(
    VOID
    )
{
    HRESULT status;
    PPH_LIST list = NULL;
    __x_ABI_CWindows_CManagement_CDeployment_CIPackageManager* packageManager = NULL;
    __FIIterable_1_Windows__CApplicationModel__CPackage* enumPackages = NULL;
    __FIIterator_1_Windows__CApplicationModel__CPackage* enumPackage = NULL;
    __x_ABI_CWindows_CApplicationModel_CIPackage* currentPackage = NULL;
    __x_ABI_CWindows_CApplicationModel_CIPackage2* currentPackage2 = NULL;
    boolean haveCurrentPackage = FALSE;

    status = PhActivateInstance(
        L"AppXDeploymentClient.dll",
        RuntimeClass_Windows_Management_Deployment_PackageManager,
        &IID_IPackageManager,
        &packageManager
        );

    if (FAILED(status))
        return NULL;

    if (PhGetOwnTokenAttributes().Elevated)
        status = __x_ABI_CWindows_CManagement_CDeployment_CIPackageManager_FindPackages(packageManager, &enumPackages);
    else
        status = __x_ABI_CWindows_CManagement_CDeployment_CIPackageManager_FindPackagesByUserSecurityId(packageManager, NULL, &enumPackages);

    if (FAILED(status))
        goto CleanupExit;

    status = __FIIterable_1_Windows__CApplicationModel__CPackage_First(enumPackages, &enumPackage);

    if (FAILED(status))
        goto CleanupExit;

    status = __FIIterator_1_Windows__CApplicationModel__CPackage_get_HasCurrent(enumPackage, &haveCurrentPackage);

    if (FAILED(status))
        goto CleanupExit;

    list = PhCreateList(10);

    while (haveCurrentPackage)
    {
        status = __FIIterator_1_Windows__CApplicationModel__CPackage_get_Current(enumPackage, &currentPackage);

        if (SUCCEEDED(status))
        {
            status = __x_ABI_CWindows_CApplicationModel_CIPackage_QueryInterface(currentPackage, &IID_IPackage2, &currentPackage2);

            if (SUCCEEDED(status))
            {
                PhQueryApplicationModelPackageInformation(
                    list,
                    currentPackage,
                    currentPackage2,
                    PH_QUERY_PACKAGE_INFO_NAME | PH_QUERY_PACKAGE_INFO_FULLNAME | PH_QUERY_PACKAGE_INFO_FAMILYNAME |
                    PH_QUERY_PACKAGE_INFO_DISPLAYNAME | PH_QUERY_PACKAGE_INFO_LOGO
                    );

                // Note: GetPackageApplicationIds returns all identifiers while the IAppListEntry2_get_AppUserModelId interface only returns a single entry (dmex)
                //if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackage_QueryInterface(currentPackage, &IID_IPackage8, &currentPackage8)))
                //if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CIPackage8_GetAppListEntries(currentPackage8, &packageAppListEntryArray)))
                //if (SUCCEEDED(__FIVectorView_1_Windows__CApplicationModel__CCore__CAppListEntry_get_Size(packageAppListEntryArray, &packageAppListEntryCount)))
                //for (UINT32 i = 0; i < packageAppListEntryCount; i++)
                //if (SUCCEEDED(__FIVectorView_1_Windows__CApplicationModel__CCore__CAppListEntry_GetAt(packageAppListEntryArray, i, &packageAppListEntry)))
                //if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CCore_CIAppListEntry_QueryInterface(packageAppListEntry, &IID_IAppListEntry2, &packageAppListEntry2)))
                //if (SUCCEEDED(__x_ABI_CWindows_CApplicationModel_CCore_CIAppListEntry2_get_AppUserModelId(packageAppListEntry2, &packageAppUserModelIdString)))

                __x_ABI_CWindows_CApplicationModel_CIPackage2_Release(currentPackage2);
            }

            __FIIterator_1_Windows__CApplicationModel__CPackage_Release(currentPackage);
        }

        if (FAILED(status))
            break;

        status = __FIIterator_1_Windows__CApplicationModel__CPackage_MoveNext(enumPackage, &haveCurrentPackage);

        if (FAILED(status))
            break;
    }

CleanupExit:

    if (enumPackages)
        __FIIterable_1_Windows__CApplicationModel__CPackage_Release(enumPackages);
    __x_ABI_CWindows_CManagement_CDeployment_CIPackageManager_Release(packageManager);

    return list;
}

VOID PhDestroyEnumPackageApplicationUserModelIds(
    _In_ PPH_LIST PackageList
    )
{
    if (!PackageList)
        return;

    for (ULONG i = 0; i < PackageList->Count; i++)
    {
        PPH_APPUSERMODELID_ENUM_ENTRY entry = PackageList->Items[i];

        PhClearReference(&entry->AppUserModelId);
        PhClearReference(&entry->PackageName);
        PhClearReference(&entry->PackageDisplayName);
        PhClearReference(&entry->PackageFamilyName);
        PhClearReference(&entry->PackageInstallPath);
        PhClearReference(&entry->PackageFullName);
        PhClearReference(&entry->PackageVersion);
        PhClearReference(&entry->SmallLogoPath);

        PhFree(entry);
    }

    PhDereferenceObject(PackageList);
}

#pragma endregion
