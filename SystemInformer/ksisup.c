/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *     dmex    2022-2023
 *
 */

#include <phapp.h>
#include <kphuser.h>
#include <kphcomms.h>
#include <settings.h>
#include <json.h>

#include <ksisup.h>

#ifdef DEBUG
//#define KSI_DEBUG_DELAY_SPLASHSCREEN 1
#endif

BOOLEAN KsiEnableLoadNative = FALSE;
BOOLEAN KsiEnableLoadFilter = FALSE;
PPH_STRING KsiKernelVersion = NULL;

PPH_STRING PhpGetKernelVersionString(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING fileName;
        PH_IMAGE_VERSION_INFO versionInfo;

        if (fileName = PhGetKernelFileName2())
        {
            if (PhInitializeImageVersionInfoEx(&versionInfo, &fileName->sr, FALSE))
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

VOID PhShowKsiStatus(
    VOID
    )
{
    KPH_PROCESS_STATE processState;

    if (!PhGetIntegerSetting(L"EnableKphWarnings") || PhStartupParameters.PhSvc)
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
            TD_ERROR_ICON,
            L"Access to the kernel driver is restricted.",
            L"%s",
            PhGetString(infoString)
            ))
        {
            PhSetIntegerSetting(L"EnableKphWarnings", FALSE);
        }

        PhDereferenceObject(infoString);
    }
}

VOID PhpShowKsiMessage(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PWSTR Icon,
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PWSTR Title,
    _In_ PWSTR Format,
    _In_ va_list ArgPtr
    )
{
    PPH_STRING kernelVersion;
    PPH_STRING errorMessage;
    PH_STRING_BUILDER stringBuilder;
    PPH_STRING messageString;

    if (!Force && !PhGetIntegerSetting(L"EnableKphWarnings") || PhStartupParameters.PhSvc)
        return;

    errorMessage = NULL;
    kernelVersion = PhpGetKernelVersionString();

    PhInitializeStringBuilder(&stringBuilder, 100);

    PhAppendFormatStringBuilder_V(&stringBuilder, Format, ArgPtr);
    PhAppendStringBuilder2(&stringBuilder, L"\r\n\r\n");

    if (Status != 0)
    {
        if (!(errorMessage = PhGetStatusMessage(Status, 0)))
            errorMessage = PhGetStatusMessage(0, Status);

        PhAppendStringBuilder2(&stringBuilder, PhGetStringOrDefault(errorMessage, L"Unknown error."));
        PhAppendStringBuilder2(&stringBuilder, L"\r\n\r\n");
    }

    PhAppendStringBuilder2(&stringBuilder, L"Kernel version: ");
    PhAppendStringBuilder2(&stringBuilder, PhGetStringOrDefault(kernelVersion, L"Unknown"));
    PhAppendStringBuilder2(&stringBuilder, L"\r\n");

    PhAppendStringBuilder2(&stringBuilder, L"State mask: ");
    PhAppendFormatStringBuilder(&stringBuilder, L"0x%08x", KphGetCurrentProcessState());
    PhAppendStringBuilder2(&stringBuilder, L"\r\n");

    if (Force && !PhGetIntegerSetting(L"EnableKphWarnings"))
    {
        PhAppendStringBuilder2(&stringBuilder, L"Driver warnings are disabled.");
        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    if (PhEndsWithString2(stringBuilder.String, L"\r\n", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    messageString = PhFinalStringBuilderString(&stringBuilder);

    if (Force)
    {
        PhShowMessage2(
            WindowHandle,
            TD_OK_BUTTON,
            Icon,
            Title,
            PhGetString(messageString)
            );
    }
    else
    {
        if (PhShowMessageOneTime(
            WindowHandle,
            TD_OK_BUTTON,
            Icon,
            Title,
            PhGetString(messageString)
            ))
        {
            PhSetIntegerSetting(L"EnableKphWarnings", FALSE);
        }
    }

    if (errorMessage)
        PhDereferenceObject(errorMessage);

    if (kernelVersion)
        PhDereferenceObject(kernelVersion);
}

VOID PhShowKsiMessageEx(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PWSTR Icon,
    _In_opt_ NTSTATUS Status,
    _In_ BOOLEAN Force,
    _In_ PWSTR Title,
    _In_ PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhpShowKsiMessage(WindowHandle, Icon, Status, Force, Title, Format, argptr);
    va_end(argptr);
}

VOID PhShowKsiMessage(
    _In_opt_ HWND WindowHandle,
    _In_opt_ PWSTR Icon,
    _In_ PWSTR Title,
    _In_ PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhpShowKsiMessage(WindowHandle, Icon, 0, TRUE, Title, Format, argptr);
    va_end(argptr);
}

NTSTATUS PhRestartSelf(
    _In_ PPH_STRINGREF AdditionalCommandLine
    )
{
    NTSTATUS status;
    PH_STRINGREF commandlineSr;
    PPH_STRING commandline;

    status = PhGetProcessCommandLineStringRef(&commandlineSr);

    if (!NT_SUCCESS(status))
        return status;

    commandline = PhConcatStringRef2(
        &commandlineSr,
        AdditionalCommandLine
        );

    status = PhCreateProcessWin32(
        NULL,
        PhGetString(commandline),
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        NULL
        );

    PhDereferenceObject(commandline);

    if (NT_SUCCESS(status))
    {
        PhExitApplication(STATUS_SUCCESS);
    }

    return status;
}

BOOLEAN PhDoesOldKsiExist(
    VOID
    )
{
    static PH_STRINGREF ksiOld = PH_STRINGREF_INIT(L"ksi.dll-old");
    BOOLEAN result = FALSE;
    PPH_STRING applicationDirectory;
    PPH_STRING fileName;

    if (!(applicationDirectory = PhGetApplicationDirectory()))
        return FALSE;

    if (fileName = PhConcatStringRef2(&applicationDirectory->sr, &ksiOld))
    {
        result = PhDoesFileExist(&fileName->sr);
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

    string = PhGetStringSetting(L"KphServiceName");

    if (PhIsNullOrEmptyString(string))
    {
        PhClearReference(&string);
        string = PhCreateString(KPH_SERVICE_NAME);
    }

    return string;
}

NTSTATUS KsiInitializeCallbackThread(
    _In_opt_ PVOID CallbackContext
    )
{
    static PH_STRINGREF driverExtension = PH_STRINGREF_INIT(L".sys");
    NTSTATUS status;
    PPH_STRING fileName = NULL;
    PPH_STRING ksiFileName = NULL;
    PPH_STRING ksiServiceName = NULL;

#ifdef KSI_DEBUG_DELAY_SPLASHSCREEN
    if (CallbackContext) PhDelayExecution(1000);
#endif

    if (!(ksiServiceName = PhGetKsiServiceName()))
        goto CleanupExit;
    if (!(fileName = PhGetApplicationFileNameWin32()))
        goto CleanupExit;
    if (!(ksiFileName = PhGetBaseNameChangeExtension(&fileName->sr, &driverExtension)))
        goto CleanupExit;

    if (PhDoesFileExistWin32(PhGetString(ksiFileName)))
    {
        KPH_CONFIG_PARAMETERS config = { 0 };
        PPH_STRING objectName = NULL;
        PPH_STRING portName = NULL;
        PPH_STRING altitudeName = NULL;
        BOOLEAN disableImageLoadProtection = FALSE;

        if (PhIsNullOrEmptyString(objectName = PhGetStringSetting(L"KphObjectName")))
            PhMoveReference(&objectName, PhCreateString(KPH_OBJECT_NAME));
        if (PhIsNullOrEmptyString(portName = PhGetStringSetting(L"KphPortName")))
            PhMoveReference(&portName, PhCreateString(KPH_PORT_NAME));
        if (PhIsNullOrEmptyString(altitudeName = PhGetStringSetting(L"KphAltitude")))
            PhMoveReference(&altitudeName, PhCreateString(KPH_ALTITUDE_NAME));
        disableImageLoadProtection = !!PhGetIntegerSetting(L"KphDisableImageLoadProtection");

        config.FileName = &ksiFileName->sr;
        config.ServiceName = &ksiServiceName->sr;
        config.ObjectName = &objectName->sr;
        config.PortName = &portName->sr;
        config.Altitude = &altitudeName->sr;
        config.EnableNativeLoad = KsiEnableLoadNative;
        config.EnableFilterLoad = KsiEnableLoadFilter;
        config.DisableImageLoadProtection = disableImageLoadProtection;
        config.Callback = NULL;
        status = KphConnect(&config);

        if (NT_SUCCESS(status))
        {
            KPH_LEVEL level = KphLevel();

            if (!NtCurrentPeb()->BeingDebugged && (level != KphLevelMax))
            {
                if ((level == KphLevelHigh) &&
                    !PhStartupParameters.KphStartupMax)
                {
                    PH_STRINGREF commandline = PH_STRINGREF_INIT(L" -kx");
                    PhRestartSelf(&commandline);
                }

                if ((level < KphLevelHigh) &&
                    !PhStartupParameters.KphStartupMax &&
                    !PhStartupParameters.KphStartupHigh)
                {
                    PH_STRINGREF commandline = PH_STRINGREF_INIT(L" -kh");
                    PhRestartSelf(&commandline);
                }
            }
        }
        else
        {
            PhShowKsiMessageEx(
                CallbackContext,
                TD_ERROR_ICON,
                status,
                FALSE,
                L"Unable to load kernel driver",
                L"Unable to load the kernel driver service."
                );
        }

        PhClearReference(&objectName);
    }
    else
    {
        PhShowKsiMessageEx(
            CallbackContext,
            TD_ERROR_ICON,
            STATUS_NO_SUCH_FILE,
            FALSE,
            L"Unable to load kernel driver",
            L"The kernel driver was not found."
            );
    }

CleanupExit:
    if (ksiServiceName)
        PhDereferenceObject(ksiServiceName);
    if (ksiFileName)
        PhDereferenceObject(ksiFileName);
    if (fileName)
        PhDereferenceObject(fileName);

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
            TD_ERROR_ICON,
            0,
            FALSE,
            L"Unable to load kernel driver",
            L"The kernel driver is not supported on this Windows version, the "
            L"minimum supported version is Windows 10."
            );
        return;
    }
    if (WindowsVersion < WINDOWS_10_20H1) // Temporary workaround for +3 month Microsoft delay (dmex)
    {
        PhShowKsiMessageEx(
            NULL,
            TD_ERROR_ICON,
            0,
            FALSE,
            L"Unable to load kernel driver",
            L"The kernel driver is temporarily disabled on this Windows version."
            );
        return;
    }
    if (WindowsVersion == WINDOWS_NEW)
    {
        PhShowKsiMessageEx(
            NULL,
            TD_ERROR_ICON,
            0,
            FALSE,
            L"Unable to load kernel driver",
            L"The kernel driver is not supported on preview builds. If this is "
            L"not a preview build, request support by submitting a GitHub issue "
            L"with the Windows Kernel version."
            );
        return;
    }

    if (PhDoesOldKsiExist())
    {
        PhShowKsiMessageEx(
            NULL,
            TD_ERROR_ICON,
            STATUS_PENDING,
            FALSE,
            L"Unable to load kernel driver",
            L"The last System Informer update requires a reboot."
            );
        return;
    }

    KsiEnableLoadNative = !!PhGetIntegerSetting(L"KsiEnableLoadNative");
    KsiEnableLoadFilter = !!PhGetIntegerSetting(L"KsiEnableLoadFilter");

    if (PhGetIntegerSetting(L"KsiEnableSplashScreen"))
        KsiShowInitializingSplashScreen();
    else
        KsiInitializeCallbackThread(NULL);
}

NTSTATUS PhDestroyKsi(
    VOID
    )
{
    NTSTATUS status;
    PPH_STRING ksiServiceName;
    KPH_CONFIG_PARAMETERS config = { 0 };

    if (!KphCommsIsConnected())
        return STATUS_SUCCESS;
    if (!(ksiServiceName = PhGetKsiServiceName()))
        return STATUS_UNSUCCESSFUL;

    config.ServiceName = &ksiServiceName->sr;
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

        if (object = PhCreateJsonParserEx(value, FALSE))
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

    directory = PhGetApplicationDirectoryWin32();
    value = PhConvertUtf16ToUtf8Ex(directory->Buffer, directory->Length);
    PhDereferenceObject(directory);

    PhAddJsonObject2(object, "KsiDirectory", value->Buffer, value->Length);
    PhDereferenceObject(value);

    serviceName = PhGetKsiServiceName();
    value = PhConvertUtf16ToUtf8Ex(serviceName->Buffer, serviceName->Length);
    PhDereferenceObject(serviceName);

    PhAddJsonObject2(object, "KsiServiceName", value->Buffer, value->Length);
    PhDereferenceObject(value);

    value = PhGetJsonArrayString(object, FALSE);
    string = PhBufferToHexString((PUCHAR)value->Buffer, (ULONG)value->Length);
    PhDereferenceObject(value);

    PhFreeJsonObject(object);

    return string;
}
