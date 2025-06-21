/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2024
 *     dmex    2022-2023
 *
 */

#include <phapp.h>
#include <kphuser.h>
#include <kphcomms.h>
#include <kphdyndata.h>
#include <settings.h>
#include <phsettings.h>
#include <json.h>
#include <mapimg.h>
#include <phappres.h>
#include <sistatus.h>
#include <informer.h>
#include <phsvccl.h>
#include <svcsup.h>

#include <ksisup.h>

typedef struct _KSI_SUPPORT_DATA
{
    USHORT Class;
    USHORT Machine;
    ULONG TimeDateStamp;
    ULONG SizeOfImage;
} KSI_SUPPORT_DATA, *PKSI_SUPPORT_DATA;

static CONST PH_STRINGREF DriverExtension = PH_STRINGREF_INIT(L".sys");
static PPH_STRING KsiKernelVersion = NULL;
static KSI_SUPPORT_DATA KsiSupportData = { MAXWORD, 0, 0, 0 };
static PPH_STRING KsiSupportString = NULL;
// N.B. These are necessary for consistency between load and unload.
static BOOLEAN KsiEnableLoadNative = FALSE;
static BOOLEAN KsiEnableLoadFilter = FALSE;
static PPH_STRING KsiServiceName = NULL;
static PPH_STRING KsiFileName = NULL;
static PPH_STRING KsiObjectName = NULL;

#ifdef DEBUG
//#define KSI_DEBUG_DELAY_SPLASHSCREEN 1

extern // ksidbg.c
VOID KsiDebugLogInitialize(
    VOID
    );

extern // ksidbg.c
VOID KsiDebugLogFinalize(
    VOID
    );
#endif

PPH_STRING KsiGetKernelFileName(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_STRING KsiKernelFileName = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        KsiKernelFileName = PhGetKernelFileName();
        PhEndInitOnce(&initOnce);
    }

    if (KsiKernelFileName)
        return PhReferenceObject(KsiKernelFileName);

    return NULL;
}

PCWSTR KsiGetWindowsVersionString(
    VOID
    )
{
    switch (WindowsVersion)
    {
    case WINDOWS_7:
        return L"Windows 7";
    case WINDOWS_8:
        return L"Windows 8";
    case WINDOWS_8_1:
        return L"Windows 8.1";
    case WINDOWS_10:
        return L"Windows 10 RTM";
    case WINDOWS_10_TH2:
        return L"Windows 10 TH2";
    case WINDOWS_10_RS1:
        return L"Windows 10 RS1";
    case WINDOWS_10_RS2:
        return L"Windows 10 RS2";
    case WINDOWS_10_RS3:
        return L"Windows 10 RS3";
    case WINDOWS_10_RS4:
        return L"Windows 10 RS4";
    case WINDOWS_10_RS5:
        return L"Windows 10 RS5";
    case WINDOWS_10_19H1:
        return L"Windows 10 19H1";
    case WINDOWS_10_19H2:
        return L"Windows 10 19H2";
    case WINDOWS_10_20H1:
        return L"Windows 10 20H1";
    case WINDOWS_10_20H2:
        return L"Windows 10 20H2";
    case WINDOWS_10_21H1:
        return L"Windows 10 21H1";
    case WINDOWS_10_21H2:
        return L"Windows 10 21H2";
    case WINDOWS_10_22H2:
        return L"Windows 10 22H2";
    case WINDOWS_11:
        return L"Windows 11";
    case WINDOWS_11_22H2:
        return L"Windows 11 22H2";
    case WINDOWS_11_23H2:
        return L"Windows 11 23H2";
    case WINDOWS_11_24H2:
        return L"Windows 11 24H2";
    case WINDOWS_11_25H2:
        return L"Windows 11 25H2";
    case WINDOWS_NEW:
        return L"Windows Insider Preview";
    }

    static_assert(WINDOWS_MAX == WINDOWS_11_25H2, "KsiGetWindowsVersionString must include all versions");

    return L"Windows";
}

PPH_STRING KsiGetWindowsBuildString(
    VOID
    )
{
    PH_FORMAT format[5];

    PhInitFormatU(&format[0], PhOsVersion.MajorVersion);
    PhInitFormatC(&format[1], L'.');
    PhInitFormatU(&format[2], PhOsVersion.MinorVersion);
    PhInitFormatC(&format[3], L'.');
    PhInitFormatU(&format[4], PhOsVersion.BuildNumber);

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

PPH_STRING KsiGetKernelVersionString(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING fileName;
        PH_IMAGE_VERSION_INFO versionInfo;

        if (fileName = KsiGetKernelFileName())
        {
            if (NT_SUCCESS(PhInitializeImageVersionInfoEx(&versionInfo, &fileName->sr, FALSE)))
            {
                KsiKernelVersion = versionInfo.FileVersion;

                versionInfo.FileVersion = NULL;
                PhDeleteImageVersionInfo(&versionInfo);
            }

            PhDereferenceObject(fileName);
        }

        PhEndInitOnce(&initOnce);
    }

    if (KsiKernelVersion)
        return PhReferenceObject(KsiKernelVersion);

    return NULL;
}

VOID KsiGetKernelSupportData(
    _Out_ PKSI_SUPPORT_DATA SupportData
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING fileName;
        PH_MAPPED_IMAGE mappedImage;

        if (fileName = KsiGetKernelFileName())
        {
            if (PhEndsWithString2(fileName, L"ntoskrnl.exe", TRUE))
                KsiSupportData.Class = KPH_DYN_CLASS_NTOSKRNL;
            else if (PhEndsWithString2(fileName, L"ntkrla57.exe", TRUE))
                KsiSupportData.Class = KPH_DYN_CLASS_NTKRLA57;

            if (NT_SUCCESS(PhLoadMappedImageHeaderPageSize(&fileName->sr, NULL, &mappedImage)))
            {
                KsiSupportData.Machine = mappedImage.NtHeaders->FileHeader.Machine;
                KsiSupportData.TimeDateStamp = mappedImage.NtHeaders->FileHeader.TimeDateStamp;
                KsiSupportData.SizeOfImage = mappedImage.NtHeaders->OptionalHeader.SizeOfImage;

                PhUnloadMappedImage(&mappedImage);
            }

            PhDereferenceObject(fileName);
        }

        PhEndInitOnce(&initOnce);
    }

    *SupportData = KsiSupportData;
}

PPH_STRING KsiGetKernelSupportString(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        KSI_SUPPORT_DATA supportData;

        KsiGetKernelSupportData(&supportData);

        KsiSupportString = PhFormatString(
            L"%02x-%04x-%08x-%08x",
            (BYTE)supportData.Class,
            supportData.Machine,
            supportData.TimeDateStamp,
            supportData.SizeOfImage
            );

        PhEndInitOnce(&initOnce);
    }

    return PhReferenceObject(KsiSupportString);
}

VOID PhShowKsiStatus(
    VOID
    )
{
    KPH_PROCESS_STATE processState;

    if (!PhEnableKsiWarnings || PhStartupParameters.PhSvc)
        return;

    processState = KphGetCurrentProcessState();

    if ((processState != 0) &&
        (processState & KPH_PROCESS_STATE_MAXIMUM) != KPH_PROCESS_STATE_MAXIMUM)
    {
        PPH_STRING infoString;
        PH_STRING_BUILDER stringBuilder;

        PhInitializeStringBuilder(&stringBuilder, 100);

        if (!BooleanFlagOn(processState, KPH_PROCESS_SECURELY_CREATED))
        {
            PhAppendStringBuilder2(&stringBuilder, L"    - not securely created\r\n");
        }
        if (!BooleanFlagOn(processState, KPH_PROCESS_VERIFIED_PROCESS))
        {
            PhAppendStringBuilder2(&stringBuilder, L"    - unverified primary image\r\n");
        }
        if (!BooleanFlagOn(processState, KPH_PROCESS_PROTECTED_PROCESS))
        {
            PhAppendStringBuilder2(&stringBuilder, L"    - inactive protections\r\n");
        }
        if (!BooleanFlagOn(processState, KPH_PROCESS_NO_UNTRUSTED_IMAGES))
        {
            PhAppendStringBuilder2(&stringBuilder, L"    - unsigned images (likely an unsigned plugin)\r\n");
        }
        if (!BooleanFlagOn(processState, KPH_PROCESS_NOT_BEING_DEBUGGED))
        {
            PhAppendStringBuilder2(&stringBuilder, L"    - process is being debugged\r\n");
        }
        if ((processState & KPH_PROCESS_STATE_MINIMUM) != KPH_PROCESS_STATE_MINIMUM)
        {
            PhAppendStringBuilder2(&stringBuilder, L"    - tampered primary image\r\n");
        }

        if (PhEndsWithString2(stringBuilder.String, L"\r\n", FALSE))
            PhRemoveEndStringBuilder(&stringBuilder, 2);
        PhAppendStringBuilder2(&stringBuilder, L"\r\n\r\n"); // String interning optimization (dmex)
        PhAppendStringBuilder2(&stringBuilder, L"You will be unable to use more advanced features, view details about system processes or terminate malicious software.");
        infoString = PhFinalStringBuilderString(&stringBuilder);

        if (PhShowMessageOneTime(
            NULL,
            TD_OK_BUTTON,
            TD_SHIELD_ERROR_ICON,
            L"Access to the kernel driver is restricted.",
            L"%s",
            PhGetString(infoString)
            ))
        {
            PhEnableKsiWarnings = FALSE;
            PhSetIntegerSetting(SETTING_KSI_ENABLE_WARNINGS, FALSE);
        }

        PhDereferenceObject(infoString);
    }
}

PPH_STRING PhpGetKsiMessage(
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PCWSTR Format,
    _In_ va_list ArgPtr
    )
{
    PPH_STRING buildString;
    PPH_STRING versionString;
    PPH_STRING kernelVersion;
    PPH_STRING errorMessage;
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING supportString;
    PPH_STRING messageString;
    ULONG processState;

    buildString = KsiGetWindowsBuildString();
    versionString = PhGetApplicationVersionString(FALSE);
    kernelVersion = KsiGetKernelVersionString();
    errorMessage = NULL;

    PhInitializeStringBuilder(&stringBuilder, 100);
    PhAppendFormatStringBuilder_V(&stringBuilder, Format, ArgPtr);
    PhAppendStringBuilder2(&stringBuilder, L"\r\n\r\n");

    if (Status != 0)
    {
        if (!(errorMessage = PhGetStatusMessage(Status, 0)))
            errorMessage = PhGetStatusMessage(0, Status);

        if (errorMessage)
        {
            PH_STRINGREF firstPart;
            PH_STRINGREF secondPart;

            // sanitize format specifiers

            secondPart = errorMessage->sr;
            while (PhSplitStringRefAtChar(&secondPart, L'%', &firstPart, &secondPart))
            {
                PhAppendStringBuilder(&stringBuilder, &firstPart);
                PhAppendStringBuilder2(&stringBuilder, L"%%");
            }

            PhAppendStringBuilder(&stringBuilder, &firstPart);
        }
        else
        {
            PhAppendStringBuilder2(&stringBuilder, L"Unknown error.");
        }

        PhAppendFormatStringBuilder(&stringBuilder, L" (0x%08x)", Status);
        PhAppendStringBuilder2(&stringBuilder, L"\r\n\r\n");
    }

    PhAppendFormatStringBuilder(
        &stringBuilder,
        L"%ls %ls\r\n",
        KsiGetWindowsVersionString(),
        PhGetString(buildString)
        );

    PhAppendStringBuilder2(&stringBuilder, L"Windows Kernel ");
    PhAppendStringBuilder2(&stringBuilder, PhGetStringOrDefault(kernelVersion, L"Unknown"));
    PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    PhAppendStringBuilder(&stringBuilder, &versionString->sr);
    PhAppendStringBuilder2(&stringBuilder, L"\r\n");

    processState = KphGetCurrentProcessState();
    if (processState != 0)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Process State ");
        PhAppendFormatStringBuilder(&stringBuilder, L"0x%08x", processState);
        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    if (!PhEnableKsiWarnings)
    {
        PhAppendStringBuilder2(&stringBuilder, L"Driver warnings are disabled.");
        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    supportString = KsiGetKernelSupportString();
    PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    PhAppendStringBuilder(&stringBuilder, &supportString->sr);
    PhDereferenceObject(supportString);

    messageString = PhFinalStringBuilderString(&stringBuilder);

    PhClearReference(&errorMessage);
    PhClearReference(&kernelVersion);
    PhClearReference(&versionString);
    PhClearReference(&buildString);

    return messageString;
}

LONG PhpShowKsiMessage(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PCWSTR Title,
    _In_ PCWSTR Format,
    _In_ va_list ArgPtr
    )
{
    LONG result;
    PPH_STRING errorMessage;

    if (!Force && !PhEnableKsiWarnings || PhStartupParameters.PhSvc)
        return INT_ERROR;

    errorMessage = PhpGetKsiMessage(Status, Force, Format, ArgPtr);

    if (Force)
    {
        result = PhShowMessage2(
            WindowHandle,
            Buttons,
            Icon,
            Title,
            PhGetString(errorMessage)
            );
    }
    else
    {
        BOOLEAN checked;
        LONG bufferLength;
        WCHAR buffer[0x100];

        bufferLength = _snwprintf(
            buffer,
            ARRAYSIZE(buffer) - sizeof(UNICODE_NULL),
            L"%s (0x%08x)",
            Title,
            Status
            );
        buffer[bufferLength] = UNICODE_NULL;

        result = PhShowMessageOneTime2(
            WindowHandle,
            Buttons,
            Icon,
            buffer,
            &checked,
            PhGetString(errorMessage)
            );

        if (checked)
        {
            PhEnableKsiWarnings = FALSE;
            PhSetIntegerSetting(SETTING_KSI_ENABLE_WARNINGS, FALSE);
        }
    }

    PhClearReference(&errorMessage);

    return result;
}

PPH_STRING PhGetKsiMessage(
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PCWSTR Format,
    ...
    )
{
    PPH_STRING message;
    va_list argptr;

    va_start(argptr, Format);
    message = PhpGetKsiMessage(Status, Force, Format, argptr);
    va_end(argptr);

    return message;
}

LONG PhShowKsiMessage2(
    _In_opt_ HWND WindowHandle,
    _In_ ULONG Buttons,
    _In_opt_ PCWSTR Icon,
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    )
{
    LONG result;
    va_list argptr;

    va_start(argptr, Format);
    result = PhpShowKsiMessage(WindowHandle, Buttons, Icon, Status, Force, Title, Format, argptr);
    va_end(argptr);

    return result;
 }

VOID PhShowKsiMessageEx(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR Icon,
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhpShowKsiMessage(WindowHandle, TD_OK_BUTTON, Icon, Status, Force, Title, Format, argptr);
    va_end(argptr);
}

VOID PhShowKsiMessage(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PCWSTR Icon,
    _In_ PCWSTR Title,
    _In_ PCWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhpShowKsiMessage(WindowHandle, TD_OK_BUTTON, Icon, 0, TRUE, Title, Format, argptr);
    va_end(argptr);
}

VOID KsiCreateRandomizedName(
    _In_ PCWSTR Prefix,
    _Out_ PPH_STRING* Name
    )
{
    ULONG length;
    WCHAR buffer[13];

    length = (PhGenerateRandomNumber64() % 6) + 8; // 8-12 characters

    PhGenerateRandomAlphaString(buffer, length);

    *Name = PhConcatStrings2(Prefix, buffer);
}

NTSTATUS KsiSvcConnectToServer(
    VOID
    )
{
    NTSTATUS status;
    PPH_STRING serviceName;
    PPH_STRING fileName;
    PPH_STRING commandLine;
    SC_HANDLE serviceHandle;
    PPH_STRING portName;
    UNICODE_STRING portNameUs;
    ULONG attempts;

    if (!(fileName = PhGetApplicationFileNameWin32()))
        return STATUS_UNSUCCESSFUL;

    KsiCreateRandomizedName(L"SystemInformer_", &serviceName);

    commandLine = PhFormatString(
        L"\"%s\" -ras \"%s\"",
        fileName->Buffer,
        PhGetString(serviceName)
        );

    status = PhCreateService(
        &serviceHandle,
        PhGetString(serviceName),
        PhGetString(serviceName),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        PhGetString(commandLine),
        L"LocalSystem",
        L""
        );

    PhDereferenceObject(commandLine);
    PhDereferenceObject(fileName);

    if (!NT_SUCCESS(status))
        return status;

    PhStartService(serviceHandle, 0, NULL);
    PhDeleteService(serviceHandle);

    portName = PhConcatStrings2(L"\\BaseNamedObjects\\", PhGetString(serviceName));
    PhStringRefToUnicodeString(&portName->sr, &portNameUs);
    attempts = 50;

    // Try to connect several times because the server may take
    // a while to initialize.
    do
    {
        status = PhSvcConnectToServer(&portNameUs, 0);

        if (NT_SUCCESS(status))
            break;

        PhDelayExecution(100);

    } while (--attempts != 0);

    PhDereferenceObject(portName);

    PhCloseServiceHandle(serviceHandle);

    return status;
}

NTSTATUS PhRestartSelf(
    _In_ PCPH_STRINGREF AdditionalCommandLine
    )
{
    static ULONG64 mitigationFlags[] =
    {
#ifdef DEBUG
        0, 0
#else
        (PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_PREFER_SYSTEM32_ALWAYS_ON),
        (PROCESS_CREATION_MITIGATION_POLICY2_LOADER_INTEGRITY_CONTINUITY_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY2_STRICT_CONTROL_FLOW_GUARD_ALWAYS_ON |
         // PROCESS_CREATION_MITIGATION_POLICY2_BLOCK_NON_CET_BINARIES_ALWAYS_ON |
         // PROCESS_CREATION_MITIGATION_POLICY2_XTENDED_CONTROL_FLOW_GUARD_ALWAYS_ON |
         PROCESS_CREATION_MITIGATION_POLICY2_MODULE_TAMPERING_PROTECTION_ALWAYS_ON)
#endif
    };
    NTSTATUS status;
    PPROC_THREAD_ATTRIBUTE_LIST attributeList = NULL;
    PH_STRINGREF commandlineSr;
    PPH_STRING commandline;
    STARTUPINFOEX startupInfo;

    status = PhGetProcessCommandLineStringRef(&commandlineSr);

    if (!NT_SUCCESS(status))
        return status;

    commandline = PhConcatStringRef2(
        &commandlineSr,
        AdditionalCommandLine
        );

    status = KsiSvcConnectToServer();
    if (NT_SUCCESS(status))
    {
        status = PhSvcCallCreateProcessForKsi(PhGetString(commandline), mitigationFlags);
        PhSvcDisconnectFromServer();
    }

    if (NT_SUCCESS(status))
    {
        PhExitApplication(STATUS_SUCCESS);
    }

#ifndef DEBUG
    status = PhInitializeProcThreadAttributeList(&attributeList, 1);

    if (!NT_SUCCESS(status))
        return status;

    if (WindowsVersion >= WINDOWS_10_22H2)
    {
        status = PhUpdateProcThreadAttribute(
            attributeList,
            PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY,
            mitigationFlags,
            sizeof(ULONG64) * 2
            );
    }
    else
    {
        status = PhUpdateProcThreadAttribute(
            attributeList,
            PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY,
            mitigationFlags,
            sizeof(ULONG64) * 1
            );
    }
#endif

    if (!NT_SUCCESS(status))
        return status;

    memset(&startupInfo, 0, sizeof(STARTUPINFOEX));
    startupInfo.StartupInfo.cb = sizeof(STARTUPINFOEX);
    startupInfo.lpAttributeList = attributeList;

    status = PhCreateProcessWin32Ex(
        NULL,
        PhGetString(commandline),
        NULL,
        NULL,
        &startupInfo,
        PH_CREATE_PROCESS_DEFAULT_ERROR_MODE | PH_CREATE_PROCESS_EXTENDED_STARTUPINFO,
        NULL,
        NULL,
        NULL,
        NULL
        );

    if (NT_SUCCESS(status))
    {
        PhExitApplication(STATUS_SUCCESS);
    }

    if (attributeList)
        PhDeleteProcThreadAttributeList(attributeList);

    PhDereferenceObject(commandline);

    return status;
}

BOOLEAN PhDoesOldKsiExist(
    VOID
    )
{
    static CONST PH_STRINGREF ksiOld = PH_STRINGREF_INIT(L"ksi.dll-old");
    BOOLEAN result = FALSE;
    PPH_STRING applicationDirectory;
    PPH_STRING fileName;

    if (!(applicationDirectory = PhGetApplicationDirectory()))
        return FALSE;

    if (fileName = PhConcatStringRef2(&applicationDirectory->sr, &ksiOld))
    {
        if (result = PhDoesFileExist(&fileName->sr))
        {
            // If the file exists try to delete it. If we can't a reboot is
            // still required since it's likely still mapped into the kernel.
            if (NT_SUCCESS(PhDeleteFile(&fileName->sr)))
                result = FALSE;
        }

        PhDereferenceObject(fileName);
    }

    PhDereferenceObject(applicationDirectory);
    return result;
}

PPH_STRING PhGetKsiServiceName(
    VOID
    )
{
    PPH_STRING string;

    string = PhGetStringSetting(SETTING_KSI_SERVICE_NAME);

    if (PhIsNullOrEmptyString(string))
    {
        PhClearReference(&string);
        string = PhCreateString(KPH_SERVICE_NAME);
    }

    return string;
}

_Function_class_(KPH_COMMS_CALLBACK)
BOOLEAN KsiCommsCallback(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    )
{
    if (Message->Header.MessageId == KphMsgRequiredStateFailure &&
        Message->Kernel.RequiredStateFailure.ClientId.UniqueProcess == NtCurrentProcessId())
    {
        // force the cached value to be updated
        KphLevelEx(FALSE);
    }

    return PhInformerDispatch(ReplyToken, Message);
}

NTSTATUS KsiReadConfiguration(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PBYTE* Data,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    PPH_STRING fileName;
    HANDLE fileHandle;

    *Data = NULL;
    *Length = 0;

    if (fileName = PhGetApplicationDirectoryFileName(FileName, TRUE))
    {
        status = PhCreateFile(
            &fileHandle,
            &fileName->sr,
            FILE_GENERIC_READ,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (NT_SUCCESS(status))
        {
            status = PhGetFileData(fileHandle, Data, Length);
            NtClose(fileHandle);
        }

        PhDereferenceObject(fileName);
    }
    else
    {
        status = STATUS_NO_SUCH_FILE;
    }

    return status;
}

NTSTATUS KsiValidateDynamicConfiguration(
    _In_ PBYTE DynData,
    _In_ ULONG DynDataLength
    )
{
    NTSTATUS status;
    PPH_STRING fileName;
    KSI_SUPPORT_DATA supportData;

    if (fileName = KsiGetKernelFileName())
    {
        KsiGetKernelSupportData(&supportData);

        status = KphDynDataLookup(
            (PKPH_DYN_CONFIG)DynData,
            DynDataLength,
            supportData.Class,
            supportData.Machine,
            supportData.TimeDateStamp,
            supportData.SizeOfImage,
            NULL,
            NULL
            );

        PhDereferenceObject(fileName);
    }
    else
    {
        status = STATUS_NO_SUCH_FILE;
    }

    return status;
}

NTSTATUS KsiGetDynData(
    _Out_ PBYTE* DynData,
    _Out_ PULONG DynDataLength,
    _Out_ PBYTE* Signature,
    _Out_ PULONG SignatureLength
    )
{
    static CONST PH_STRINGREF dataFileName = PH_STRINGREF_INIT(L"ksidyn.bin");
    static CONST PH_STRINGREF sigFileName = PH_STRINGREF_INIT(L"ksidyn.sig");
    NTSTATUS status;
    PBYTE data = NULL;
    ULONG dataLength;
    PBYTE sig = NULL;
    ULONG sigLength;

    // TODO download dynamic data from server

    *DynData = NULL;
    *DynDataLength = 0;
    *Signature = NULL;
    *SignatureLength = 0;

    status = KsiReadConfiguration(&dataFileName, &data, &dataLength);
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = KsiValidateDynamicConfiguration(data, dataLength);
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = KsiReadConfiguration(&sigFileName, &sig, &sigLength);
    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!sigLength)
    {
        status = STATUS_SI_DYNDATA_INVALID_SIGNATURE;
        goto CleanupExit;
    }

    *DynDataLength = dataLength;
    *DynData = data;
    data = NULL;

    *SignatureLength = sigLength;
    *Signature = sig;
    sig = NULL;

    status = STATUS_SUCCESS;

CleanupExit:
    if (data)
        PhFree(data);
    if (sig)
        PhFree(sig);

    return status;
}

PPH_STRING PhGetKsiFileName(
    VOID
    )
{
    PPH_STRING applicationDirectory;
    PPH_STRING fileName;

#if defined(PH_BUILD_MSIX)
    PH_FORMAT format[4];
    PhInitFormatS(&format[0], L"\\Drivers");
    PhInitFormatS(&format[1], L"\\SystemInformer");
    PhInitFormatS(&format[2], L"\\SystemInformer");
    PhInitFormatSR(&format[3], DriverExtension);
    fileName = PhFormat(format, RTL_NUMBER_OF(format), MAX_PATH);
    applicationDirectory = PhGetSystemDirectoryWin32(&fileName->sr);
    PhDereferenceObject(fileName);
#else
    fileName = PhGetApplicationFileNameWin32();
    applicationDirectory = PhGetBaseNameChangeExtension(&fileName->sr, &DriverExtension);
    PhDereferenceObject(fileName);
#endif

    return applicationDirectory;
}

PPH_STRING PhGetTemporaryKsiDirectory(
    VOID
    )
{
    PPH_STRING applicationDirectory;

#if defined(PH_BUILD_MSIX)
    applicationDirectory = PhGetLocalAppDataDirectoryZ(L"temp-drivers\\", FALSE);
#else
    applicationDirectory = PhGetApplicationDirectoryFileNameZ(L"temp-drivers\\", FALSE);
#endif

    return applicationDirectory;
}

NTSTATUS KsiCreateTemporaryDriverFile(
    _In_ PPH_STRING FileName,
    _In_ PPH_STRING TempDirectory,
    _Out_ PPH_STRING* TempFileName
    )
{
    NTSTATUS status;
    PH_FORMAT format[4];
    PPH_STRING tempFileName;
    LARGE_INTEGER timeStamp;

    *TempFileName = NULL;

    PhQuerySystemTime(&timeStamp);

    PhInitFormatSR(&format[0], TempDirectory->sr);
    PhInitFormatS(&format[1], L"SystemInformer");
    PhInitFormatI64X(&format[2], timeStamp.QuadPart);
    PhInitFormatSR(&format[3], DriverExtension);

    tempFileName = PhFormat(format, RTL_NUMBER_OF(format), MAX_PATH);

    if (NT_SUCCESS(status = PhCreateDirectoryWin32(&TempDirectory->sr)))
    {
        if (NT_SUCCESS(status = PhCopyFileWin32(PhGetString(FileName), PhGetString(tempFileName), TRUE)))
        {
            *TempFileName = tempFileName;
            tempFileName = NULL;
        }
    }

    PhClearReference(&tempFileName);

    return status;
}

VOID KsiConnect(
    _In_opt_ HWND WindowHandle
    )
{
    NTSTATUS status;
    PBYTE dynData = NULL;
    ULONG dynDataLength;
    PBYTE signature = NULL;
    ULONG signatureLength;
    PPH_STRING ksiFileName = NULL;
    KPH_CONFIG_PARAMETERS config = { 0 };
    PPH_STRING objectName = NULL;
    PPH_STRING portName = NULL;
    PPH_STRING altitude = NULL;
    PPH_STRING tempDriverDir = NULL;
    KPH_LEVEL level;

    if (tempDriverDir = PhGetTemporaryKsiDirectory())
    {
        PhDeleteDirectoryWin32(&tempDriverDir->sr);
    }

    status = KsiGetDynData(
        &dynData,
        &dynDataLength,
        &signature,
        &signatureLength
        );

    if (status == STATUS_SI_DYNDATA_UNSUPPORTED_KERNEL)
    {
        if (PhGetPhReleaseChannel() < PhCanaryChannel)
        {
            PhShowKsiMessageEx(
                WindowHandle,
                TD_SHIELD_ERROR_ICON,
                0,
                FALSE,
                L"Unable to load kernel driver",
                L"The kernel driver is not yet supported on this kernel "
                L"version. For the latest kernel support switch to the Canary "
                L"update channel (Help > Check for updates > Canary > Check)."
                );
        }
        else
        {
            PhShowKsiMessageEx(
                WindowHandle,
                TD_SHIELD_ERROR_ICON,
                0,
                FALSE,
                L"Unable to load kernel driver",
                L"The kernel driver is not yet supported on this kernel "
                L"version. Request support by submitting a GitHub issue with "
                L"the Windows Kernel version."
                );
        }
        goto CleanupExit;
    }

    if (status == STATUS_NO_SUCH_FILE)
    {
        PhShowKsiMessageEx(
            WindowHandle,
            TD_SHIELD_ERROR_ICON,
            0,
            FALSE,
            L"Unable to load kernel driver",
            L"The dynamic configuration was not found."
            );
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status))
    {
        PhShowKsiMessageEx(
            WindowHandle,
            TD_SHIELD_ERROR_ICON,
            status,
            FALSE,
            L"Unable to load kernel driver",
            L"Failed to access the dynamic configuration."
            );

        goto CleanupExit;
    }

    if (!PhDoesFileExistWin32(PhGetString(KsiFileName)))
    {
        PhShowKsiMessageEx(
            WindowHandle,
            TD_SHIELD_ERROR_ICON,
            0,
            FALSE,
            L"Unable to load kernel driver",
            L"The kernel driver was not found."
            );
        goto CleanupExit;
    }

    if (PhIsNullOrEmptyString(portName = PhGetStringSetting(SETTING_KSI_PORT_NAME)))
        PhClearReference(&portName);
    if (PhIsNullOrEmptyString(altitude = PhGetStringSetting(SETTING_KSI_ALTITUDE)))
        PhClearReference(&altitude);

    config.FileName = &KsiFileName->sr;
    config.ServiceName = &KsiServiceName->sr;
    config.ObjectName = &KsiObjectName->sr;
    config.PortName = (portName ? &portName->sr : NULL);
    config.Altitude = (altitude ? &altitude->sr : NULL);
    config.FsSupportedFeatures = 0;
    if (!!PhGetIntegerSetting(SETTING_KSI_ENABLE_FS_FEATURE_OFFLOAD_READ))
        SetFlag(config.FsSupportedFeatures, SUPPORTED_FS_FEATURES_OFFLOAD_READ);
    if (!!PhGetIntegerSetting(SETTING_KSI_ENABLE_FS_FEATURE_OFFLOAD_WRITE))
        SetFlag(config.FsSupportedFeatures, SUPPORTED_FS_FEATURES_OFFLOAD_WRITE);
    if (!!PhGetIntegerSetting(SETTING_KSI_ENABLE_FS_FEATURE_QUERY_OPEN))
        SetFlag(config.FsSupportedFeatures, SUPPORTED_FS_FEATURES_QUERY_OPEN);
    if (!!PhGetIntegerSetting(SETTING_KSI_ENABLE_FS_FEATURE_BYPASS_IO))
        SetFlag(config.FsSupportedFeatures, SUPPORTED_FS_FEATURES_BYPASS_IO);
    config.Flags.Flags = 0;
    config.Flags.DisableImageLoadProtection = !!PhGetIntegerSetting(SETTING_KSI_DISABLE_IMAGE_LOAD_PROTECTION);
    config.Flags.RandomizedPoolTag = !!PhGetIntegerSetting(SETTING_KSI_RANDOMIZED_POOL_TAG);
    config.Flags.DynDataNoEmbedded = !!PhGetIntegerSetting(SETTING_KSI_DYN_DATA_NO_EMBEDDED);
    config.Flags.DisableSystemProcess = !!PhGetIntegerSetting(SETTING_KSI_DISABLE_SYSTEM_PROCESS);
    config.Flags.DisableThreadNames = !!PhGetIntegerSetting(SETTING_KSI_DISABLE_THREAD_NAMES);
    config.EnableNativeLoad = KsiEnableLoadNative;
    config.EnableFilterLoad = KsiEnableLoadFilter;
    config.RingBufferLength = PhGetIntegerSetting(SETTING_KSI_RING_BUFFER_LENGTH);
    config.Callback = KsiCommsCallback;

    status = KphConnect(&config);

    if (status == STATUS_NO_SUCH_FILE)
    {
        PPH_STRING tempFileName;

        //
        // N.B. We know the driver file exists from the check above. This
        // error indicates that the kernel driver is still mapped but
        // unloaded.
        //
        // The chain of calls and resulting return status is this:
        // IopLoadDriver
        // -> MmLoadSystemImage -> STATUS_IMAGE_ALREADY_LOADED
        // -> ObOpenObjectByName -> STATUS_OBJECT_NAME_NOT_FOUND
        // -> STATUS_DRIVER_FAILED_PRIOR_UNLOAD -> STATUS_NO_SUCH_FILE
        //
        // The driver object has been made temporary and the underlying
        // section will be unmapped by the kernel when it is safe to do so.
        // This generally happens when the pinned module is holding the
        // driver in memory until some code finishes executing. This can
        // also happen if another misbehaving driver is leaking or holding
        // a reference to the driver object.
        //
        // To continue to provide a good user experience, we will attempt copy
        // the driver to another file and load it again. First, try to use an
        // existing temporary driver file, if one exists.
        //

        tempFileName = PhGetStringSetting(SETTING_KSI_PREVIOUS_TEMPORARY_DRIVER_FILE);
        if (tempFileName)
        {
            if (PhDoesFileExistWin32(PhGetString(tempFileName)))
            {
                config.FileName = &tempFileName->sr;

                status = KphConnect(&config);
            }

            PhMoveReference(&KsiFileName, tempFileName);
        }

        if (!NT_SUCCESS(status) && tempDriverDir)
        {
            if (NT_SUCCESS(status = KsiCreateTemporaryDriverFile(ksiFileName, tempDriverDir, &tempFileName)))
            {
                PhSetStringSetting(SETTING_KSI_PREVIOUS_TEMPORARY_DRIVER_FILE, PhGetString(tempFileName));

                config.FileName = &tempFileName->sr;

                status = KphConnect(&config);

                PhMoveReference(&KsiFileName, tempFileName);
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        PPH_STRING randomServiceName;
        PPH_STRING randomPortName;
        PPH_STRING randomObjectName;

        //
        // Malware might be blocking the driver load. Offer the user a chance
        // to try again with a different method.
        //
        if (PhShowKsiMessage2(
            WindowHandle,
            TD_YES_BUTTON | TD_NO_BUTTON,
            TD_SHIELD_ERROR_ICON,
            status,
            FALSE,
            L"Unable to load kernel driver",
            L"Try again with alternate driver load method?"
            ) != IDYES)
        {
            goto CleanupExit;
        }

        KsiCreateRandomizedName(L"", &randomServiceName);
        KsiCreateRandomizedName(L"\\", &randomPortName);
        KsiCreateRandomizedName(L"\\Driver\\", &randomObjectName);

        config.EnableNativeLoad = TRUE;
        config.EnableFilterLoad = FALSE;
        config.ServiceName = &randomServiceName->sr;
        config.PortName = &randomPortName->sr;
        config.ObjectName = &randomObjectName->sr;

        PhMoveReference(&KsiServiceName, PhReferenceObject(randomServiceName));
        PhMoveReference(&KsiObjectName, PhReferenceObject(randomObjectName));
        KsiEnableLoadNative = TRUE;
        KsiEnableLoadFilter = FALSE;

        status = KphConnect(&config);

        if (NT_SUCCESS(status))
        {
            //
            // N.B. These settings **must** be persisted to allow subsequent
            // clients to successfully connect or unload the natively loaded
            // driver.
            //
            PhSetIntegerSetting(SETTING_KSI_ENABLE_LOAD_NATIVE, TRUE);
            PhSetIntegerSetting(SETTING_KSI_ENABLE_LOAD_FILTER, FALSE);
            PhSetStringSetting2(SETTING_KSI_SERVICE_NAME, &randomServiceName->sr);
            PhSetStringSetting2(SETTING_KSI_PORT_NAME, &randomPortName->sr);
            PhSetStringSetting2(SETTING_KSI_OBJECT_NAME, &randomObjectName->sr);

            PhSaveSettings2(PhSettingsFileName);

            PhShowKsiMessage(
                WindowHandle,
                TD_INFORMATION_ICON,
                L"Kernel driver loaded",
                L"The kernel driver was successfully loaded using an alternate "
                L"method. The settings used to load the driver have been saved. "
                L"You can revert these settings in the advanced options."
                );
        }

        PhDereferenceObject(randomServiceName);
        PhDereferenceObject(randomPortName);
        PhDereferenceObject(randomObjectName);
    }

    if (!NT_SUCCESS(status))
    {
        PhShowKsiMessageEx(
            WindowHandle,
            TD_SHIELD_ERROR_ICON,
            status,
            FALSE,
            L"Unable to load kernel driver",
            L"Unable to load the kernel driver service."
            );
        goto CleanupExit;
    }

    KphActivateDynData(dynData, dynDataLength, signature, signatureLength);

    level = KphLevelEx(FALSE);

    if (!NtCurrentPeb()->BeingDebugged && (level != KphLevelMax))
    {
        if ((level == KphLevelHigh) &&
            !PhStartupParameters.KphStartupMax)
        {
            static CONST PH_STRINGREF commandline = PH_STRINGREF_INIT(L" -kx");
            status = PhRestartSelf(&commandline);
        }

        if ((level < KphLevelHigh) &&
            !PhStartupParameters.KphStartupMax &&
            !PhStartupParameters.KphStartupHigh)
        {
            static CONST PH_STRINGREF commandline = PH_STRINGREF_INIT(L" -kh");
            status = PhRestartSelf(&commandline);
        }

        if (!NT_SUCCESS(status))
        {
            PhShowKsiMessageEx(
                WindowHandle,
                TD_ERROR_ICON,
                status,
                FALSE,
                L"Unable to load kernel driver",
                L"Unable to restart."
                );
            goto CleanupExit;
        }
    }

    if (level == KphLevelMax)
    {
        ACCESS_MASK process = 0;
        ACCESS_MASK thread = 0;

        if (PhGetIntegerSetting(SETTING_KSI_ENABLE_UNLOAD_PROTECTION))
            KphAcquireDriverUnloadProtection(NULL, NULL);

        switch (PhGetIntegerSetting(SETTING_KSI_CLIENT_PROCESS_PROTECTION_LEVEL))
        {
        case 2:
            process |= (PROCESS_VM_READ | PROCESS_QUERY_INFORMATION);
            thread |= (THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION);
            __fallthrough;
        case 1:
            process |= (PROCESS_TERMINATE | PROCESS_SUSPEND_RESUME);
            thread |= (THREAD_TERMINATE | THREAD_SUSPEND_RESUME | THREAD_RESUME);
            __fallthrough;
        case 0:
        default:
            break;
        }

        if (process != 0 || thread != 0)
            KphStripProtectedProcessMasks(NtCurrentProcess(), process, thread);
    }

    if (PhEnableProcessMonitor)
    {
        KPH_INFORMER_SETTINGS settings;

        settings.Flags = MAXULONG64;
        settings.Flags2 = MAXULONG64;
        settings.Flags3 = MAXULONG64;

        //
        // Conservative settings for now.
        //

        settings.EnableStackTraces = FALSE;

        settings.DebugPrint = FALSE;

        settings.HandlePreCreateProcess = FALSE;
        settings.HandlePostCreateProcess = FALSE;
        settings.HandlePreDuplicateProcess = FALSE;
        settings.HandlePostDuplicateProcess = FALSE;
        settings.HandlePreCreateThread = FALSE;
        settings.HandlePostCreateThread = FALSE;
        settings.HandlePreDuplicateThread = FALSE;
        settings.HandlePostDuplicateThread = FALSE;
        settings.HandlePreCreateDesktop = FALSE;
        settings.HandlePostCreateDesktop = FALSE;
        settings.HandlePreDuplicateDesktop = FALSE;
        settings.HandlePostDuplicateDesktop = FALSE;

        settings.EnableProcessCreateReply = FALSE;
        settings.FileEnablePreCreateReply = FALSE;
        settings.FileEnablePostCreateReply = FALSE;

        settings.FileEnablePostFileNames = FALSE;
        settings.FileEnableIoControlBuffers = FALSE;
        settings.FileEnableFsControlBuffers = FALSE;
        settings.FileEnableDirControlBuffers = FALSE;

        settings.RegEnablePostObjectNames = FALSE;
        settings.RegEnablePostValueNames = FALSE;
        settings.RegEnableValueBuffers = FALSE;

        KphSetInformerSettings(&settings);
    }

CleanupExit:

    PhClearReference(&objectName);
    PhClearReference(&portName);
    PhClearReference(&altitude);
    PhClearReference(&ksiFileName);
    PhClearReference(&tempDriverDir);

    if (signature)
        PhFree(signature);
    if (dynData)
        PhFree(dynData);

#ifdef DEBUG
    KsiDebugLogInitialize();
#endif
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS KsiInitializeCallbackThread(
    _In_opt_ PVOID CallbackContext
    )
{
#ifdef KSI_DEBUG_DELAY_SPLASHSCREEN
    if (CallbackContext) PhDelayExecution(1000);
#endif

    KsiConnect(CallbackContext);

    if (CallbackContext)
    {
        SendMessage(CallbackContext, TDM_CLICK_BUTTON, IDOK, 0);
    }

    return STATUS_SUCCESS;
}

static HRESULT CALLBACK KsiSplashScreenDialogCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT Notification,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR Context
    )
{
    switch (Notification)
    {
    case TDN_CREATED:
        {
            NTSTATUS status;
            HANDLE threadHandle;

            SendMessage(WindowHandle, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(WindowHandle, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            if (NT_SUCCESS(status = PhCreateThreadEx(
                &threadHandle,
                KsiInitializeCallbackThread,
                WindowHandle
                )))
            {
                NtClose(threadHandle);
            }
            else
            {
                PhShowStatus(WindowHandle, L"Unable to create the window.", status, 0);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            INT buttonId = (INT)wParam;

            if (buttonId == IDCANCEL)
            {
                return S_FALSE;
            }
        }
        break;
    case TDN_TIMER:
        {
            ULONG ticks = (ULONG)wParam;
            PPH_STRING timeSpan = PhFormatUInt64(ticks, TRUE);
            PhMoveReference(&timeSpan, PhConcatStringRefZ(&timeSpan->sr, L" ms..."));
            SendMessage(WindowHandle, TDM_SET_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)timeSpan->Buffer);
            PhDereferenceObject(timeSpan);
        }
        break;
    }

    return S_OK;
}

VOID KsiShowInitializingSplashScreen(
    VOID
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CAN_BE_MINIMIZED | TDF_CALLBACK_TIMER;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = PhGetApplicationIcon(FALSE);
    config.pfCallback = KsiSplashScreenDialogCallbackProc;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Initializing System Informer kernel driver...";
    config.pszContent = L"0 ms...";
    config.cxWidth = 200;

    TaskDialogIndirect(&config, NULL, NULL, NULL);
}

VOID PhInitializeKsi(
    VOID
    )
{
    if (!PhGetOwnTokenAttributes().Elevated)
        return;

    if (WindowsVersion < WINDOWS_10)
    {
        PhShowKsiMessageEx(
            NULL,
            TD_SHIELD_ERROR_ICON,
            0,
            FALSE,
            L"Unable to load kernel driver",
            L"The kernel driver is not supported on this Windows version, the "
            L"minimum supported version is Windows 10."
            );
        return;
    }

    if (PhDoesOldKsiExist())
    {
        PhShowKsiMessageEx(
            NULL,
            TD_SHIELD_ERROR_ICON,
            STATUS_PENDING,
            FALSE,
            L"Unable to load kernel driver",
            L"The last System Informer update requires a reboot."
            );
        return;
    }

    if (PhIsExecutingInWow64())
    {
        PhShowKsiMessageEx(
            NULL,
            TD_SHIELD_ERROR_ICON,
            0,
            FALSE,
            L"Unable to load kernel driver",
            L"The kernel driver is not supported under Wow64, use the native "
            "binary instead."
            );
        return;
    }

    if (USER_SHARED_DATA->NativeProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64 &&
        USER_SHARED_DATA->NativeProcessorArchitecture != PROCESSOR_ARCHITECTURE_ARM64)
    {
        PhShowKsiMessageEx(
            NULL,
            TD_SHIELD_ERROR_ICON,
            0,
            FALSE,
            L"Unable to load kernel driver",
            L"The kernel driver is not supported on this architecture."
            );
        return;
    }

    KphInitialize();
    PhInformerInitialize();

    KsiFileName = PhGetKsiFileName();
    KsiServiceName = PhGetKsiServiceName();
    if (PhIsNullOrEmptyString(KsiObjectName = PhGetStringSetting(SETTING_KSI_OBJECT_NAME)))
        PhMoveReference(&KsiObjectName, PhCreateString(KPH_OBJECT_NAME));
    KsiEnableLoadNative = !!PhGetIntegerSetting(SETTING_KSI_ENABLE_LOAD_NATIVE);
    KsiEnableLoadFilter = !!PhGetIntegerSetting(SETTING_KSI_ENABLE_LOAD_FILTER);

    if (PhGetIntegerSetting(SETTING_KSI_ENABLE_SPLASH_SCREEN))
        KsiShowInitializingSplashScreen();
    else
        KsiInitializeCallbackThread(NULL);
}

NTSTATUS PhCleanupKsi(
    VOID
    )
{
    NTSTATUS status;
    KPH_CONFIG_PARAMETERS config = { 0 };
    BOOLEAN shouldUnload;

    if (!KphCommsIsConnected())
        return STATUS_SUCCESS;

    if (PhGetIntegerSetting(SETTING_KSI_ENABLE_UNLOAD_PROTECTION))
        KphReleaseDriverUnloadProtection(NULL, NULL);

    if (PhGetIntegerSetting(SETTING_KSI_UNLOAD_ON_EXIT))
    {
        ULONG clientCount;

        if (!NT_SUCCESS(status = KphGetConnectedClientCount(&clientCount)))
            return status;

        shouldUnload = (clientCount == 1);
    }
    else
    {
        shouldUnload = FALSE;
    }

    KphCommsStop();
#ifdef DEBUG
    KsiDebugLogFinalize();
#endif

    if (!shouldUnload)
        return STATUS_SUCCESS;

    config.FileName = &KsiFileName->sr;
    config.ServiceName = &KsiServiceName->sr;
    config.ObjectName = &KsiObjectName->sr;
    config.EnableNativeLoad = KsiEnableLoadNative;
    config.EnableFilterLoad = KsiEnableLoadFilter;
    status = KphServiceStop(&config);

    return status;
}

_Success_(return)
BOOLEAN PhParseKsiSettingsBlob(
    _In_ PPH_STRING KsiSettingsBlob,
    _Out_ PPH_STRING* Directory,
    _Out_ PPH_STRING* ServiceName
    )
{
    PPH_STRING directory = NULL;
    PPH_STRING serviceName = NULL;
    PSTR string;
    ULONG stringLength;
    PPH_BYTES value;
    PVOID object;

    stringLength = (ULONG)KsiSettingsBlob->Length / sizeof(WCHAR) / 2;
    string = PhAllocateZero(stringLength + sizeof(UNICODE_NULL));

    if (PhHexStringToBufferEx(&KsiSettingsBlob->sr, stringLength, string))
    {
        value = PhCreateBytesEx(string, stringLength);

        if (NT_SUCCESS(PhCreateJsonParserEx(&object, value, FALSE)))
        {
            directory = PhGetJsonValueAsString(object, "KsiDirectory");
            serviceName = PhGetJsonValueAsString(object, "KsiServiceName");
            PhFreeJsonObject(object);
        }

        PhDereferenceObject(value);
    }

    PhFree(string);

    if (!PhIsNullOrEmptyString(directory) &&
        !PhIsNullOrEmptyString(serviceName))
    {
        *Directory = directory;
        *ServiceName = serviceName;
        return TRUE;
    }

    PhClearReference(&serviceName);
    PhClearReference(&directory);
    return FALSE;
}

// Note: Exported from appsup.h (dmex)
PVOID PhCreateKsiSettingsBlob(
    VOID
    )
{
    PVOID object;
    PPH_BYTES value;
    PPH_STRING string;
    PPH_STRING directory;
    PPH_STRING serviceName;

    object = PhCreateJsonObject();

#if defined(PH_BUILD_MSIX)
    PPH_STRING fileName;
    PH_FORMAT format[4];
    PhInitFormatS(&format[0], L"\\Drivers");
    PhInitFormatS(&format[1], L"\\SystemInformer");
    fileName = PhFormat(format, RTL_NUMBER_OF(format), MAX_PATH);
    directory = PhGetSystemDirectoryWin32(&fileName->sr);
    PhDereferenceObject(fileName);
#else
    directory = PhGetApplicationDirectoryWin32();
#endif
    value = PhConvertStringToUtf8(directory);
    PhAddJsonObject2(object, "KsiDirectory", value->Buffer, value->Length);
    PhDereferenceObject(value);
    PhDereferenceObject(directory);

    serviceName = PhGetKsiServiceName();
    value = PhConvertStringToUtf8(serviceName);
    PhAddJsonObject2(object, "KsiServiceName", value->Buffer, value->Length);
    PhDereferenceObject(value);
    PhDereferenceObject(serviceName);

    value = PhGetJsonArrayString(object, FALSE);
    string = PhBufferToHexString((PUCHAR)value->Buffer, (ULONG)value->Length);
    PhDereferenceObject(value);

    PhFreeJsonObject(object);

    return string;
}

NTSTATUS PhQueryKphCounters(
    _Out_ PULONG64 Duration,
    _Out_ PULONG64 DurationDown,
    _Out_ PULONG64 DurationUp
    )
{
    NTSTATUS status;
    LARGE_INTEGER performanceCounterStart;
    LARGE_INTEGER performanceCounterStop;
    LARGE_INTEGER performanceCounter;
    ULONG64 performanceCounterDuration;
    ULONG64 performanceCounterDown;
    ULONG64 performanceCounterUp;

    if (KsiLevel() < KphLevelLow)
    {
        *Duration = 0;
        *DurationDown = 0;
        *DurationUp = 0;
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        PhQueryPerformanceCounter(&performanceCounterStart);
        status = KphQueryPerformanceCounter(&performanceCounter, NULL);
        PhQueryPerformanceCounter(&performanceCounterStop);

        performanceCounterDuration = performanceCounterStop.QuadPart - performanceCounterStart.QuadPart;
        performanceCounterDown = performanceCounter.QuadPart - performanceCounterStart.QuadPart;
        performanceCounterUp = performanceCounterStop.QuadPart - performanceCounter.QuadPart;

        *Duration = performanceCounterDuration;
        *DurationDown = performanceCounterDown;
        *DurationUp = performanceCounterUp;
    }

    return status;
}
