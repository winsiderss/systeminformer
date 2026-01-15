/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2024
 *     dmex    2022-2026
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

#include <ksisup.h>

typedef struct _KSI_SUPPORT_DATA
{
    USHORT Class;
    USHORT Machine;
    ULONG TimeDateStamp;
    ULONG SizeOfImage;
} KSI_SUPPORT_DATA, *PKSI_SUPPORT_DATA;

#if defined(KSI_ONLINE_PLATFORM_SUPPORT)
typedef struct _KSI_KERNEL_SUPPORT_CHECK_CONTEXT
{
    HWND WindowHandle;
    HWND TaskDialogHandle;
    KSI_SUPPORT_DATA SupportData;
    BOOLEAN IsCanaryChannel;
    volatile BOOLEAN CheckComplete;
    volatile BOOLEAN IsSupported;
} KSI_KERNEL_SUPPORT_CHECK_CONTEXT, *PKSI_KERNEL_SUPPORT_CHECK_CONTEXT;

VOID KsiShowKernelSupportCheckDialog(
    _In_opt_ HWND WindowHandle
    );
#endif

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

/**
 * Get the kernel file name for the running system.
 * \return PPH_STRING The kernel file name, or NULL on failure.
 */
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

/**
 * Maps the WindowsVersion enum to a human readable string.
 * \return PCWSTR The human readable string for the Windows version.
 */
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
    case WINDOWS_11_26H1:
        return L"Windows 11 26H1";
    case WINDOWS_NEW:
        return L"Windows Insider Preview";
    }

    static_assert(WINDOWS_MAX == WINDOWS_11_26H1, "KsiGetWindowsVersionString must include all versions");

    return L"Windows";
}

/**
 * Return a formatted Windows build string derived from PhOsVersion.
 * \return PPH_STRING The formatted Windows build string "Major.Minor.Build".
 */
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

/**
 * Get the kernel file version string cached from the kernel image.
 *
 * This uses an init-once pattern to load and cache the `FileVersion` field
 * from the kernel image's version resource. The returned `PPH_STRING` is a
 * referenced object and must be dereferenced by the caller.
 * \return Referenced PPH_STRING containing the kernel file version, or NULL.
 */
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

/**
 * Populate `SupportData` with kernel image attributes useful for dynamic configuration lookup.
 *
 * This function caches the support data on first call and returns the cached
 * structure on subsequent calls. It uses the kernel filename to inspect the
 * mapped image headers and derives the class, machine, timestamp and size of
 * image used for compatibility lookups.
 * \param[out] SupportData Pointer to KSI_SUPPORT_DATA that receives the result.
 */
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

/**
 * Get a formatted support string that uniquely identifies the kernel image for dynamic data matching.
 *
 * The returned object is a referenced `PPH_STRING` and must be dereferenced by
 * the caller. The string format is "%02x-%04x-%08x-%08x".
 * \return Referenced `PPH_STRING` containing the kernel support string.
 */
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

/**
 * Show the user a one-time warning if the process does not have full
 * expected kernel protection capabilities.
 *
 * This function checks global settings and the current process state to
 * determine whether to show a descriptive message. If the user dismisses and
 * checks "don't show again" the global warning flag is disabled and persisted.
 */
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

/**
 * Build a long-form diagnostic string containing application and kernel
 * state and an optional formatted message.
 *
 * This helper assembles version, kernel, and process state information along
 * with the provided format and arguments. The returned `PPH_STRING` is a
 * referenced object that the caller must free via PhDereferenceObject().
 *
 * \param[in] Status NTSTATUS associated with the message (0 if none).
 * \param[in] Force If TRUE the message will be shown even if warnings are
 *                  disabled; used to control UI behaviour in callers.
 * \param[in] Format Printf-style format string for the primary message body.
 * \param[in] ArgPtr va_list of arguments for the Format.
 * \return Referenced `PPH_STRING` containing the assembled message.
 */
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

#if defined(KSI_ONLINE_PLATFORM_SUPPORT)
PPH_STRING PhpGetKsiMessage2(
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

    PhAppendStringBuilder2(&stringBuilder, L"\r\n\r\n");
    PhAppendFormatStringBuilder_V(&stringBuilder, Format, ArgPtr);

    messageString = PhFinalStringBuilderString(&stringBuilder);

    PhClearReference(&errorMessage);
    PhClearReference(&kernelVersion);
    PhClearReference(&versionString);
    PhClearReference(&buildString);

    return messageString;
}
#endif

/**
 * Show the composed kernel message to the user either one-time or
 * forced, depending on parameters.
 *
 * This wrapper uses PhpGetKsiMessage to build a detailed message string and
 * displays it using the appropriate PhShowMessage* helper. If the user opts to
 * suppress future warnings the global setting is cleared and persisted.
 *
 * \param[in] WindowHandle Parent window handle for UI.
 * \param[in] Buttons Task dialog button flags (e.g., TD_OK_BUTTON).
 * \param[in] Icon Icon resource id/name for the task dialog.
 * \param[in] Status Optional NTSTATUS to include in the message.
 * \param[in] Force Show a modal message; otherwise one-time message.
 * \param[in] Title Title to display in the dialog.
 * \param[in] Format Printf-style format string for the message.
 * \param[in] ArgPtr va_list of arguments for Format.
 * \return LONG result of the dialog interaction (e.g., IDOK, IDYES, IDNO).
 */
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

/**
 * Public wrapper to build a message string via va-args.
 *
 * Convenience wrapper around PhpGetKsiMessage that accepts variable arguments.
 * Caller receives a referenced `PPH_STRING` and must dereference it.
 *
 * \param[in] Status Optional NTSTATUS.
 * \param[in] Force See PhpGetKsiMessage.
 * \param[in] Format Printf-style format string.
 * \return Referenced `PPH_STRING` with the assembled message.
 */
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

/**
 * Public wrapper to display a message to the user with va-args.
 *
 * Calls PhpShowKsiMessage with variable argument list and returns dialog result.
 */
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

/**
 * Convenience function to show an informational message (OK) with va-args.
 *
 * This variant always uses TD_OK_BUTTON and forwards to PhpShowKsiMessage.
 */
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

/**
 * Convenience function to show a forced OK message (no status, forced).
 *
 * Calls PhpShowKsiMessage forcing the message to be shown and using TD_OK_BUTTON.
 */
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

/**
 * Restart the current process with optional additional command-line parameters
 * and process mitigation attributes set.
 *
 * The function obtains the current process command-line, appends the supplied
 * `AdditionalCommandLine` (if any), and creates a new process using
 * PhCreateProcessWin32Ex with hardened mitigation attributes where supported.
 *
 * \param[in] AdditionalCommandLine Optional additional commandline to append.
 * \return NTSTATUS of the attempted create; on success the current process
 *         exits via PhExitApplication(STATUS_SUCCESS) after successful spawn.
 */
NTSTATUS PhRestartSelf(
    _In_ PCPH_STRINGREF AdditionalCommandLine
    )
{
#ifndef DEBUG
    static ULONG64 mitigationFlags[] =
    {
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
    };
#endif
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

/**
 * Determine if an old temporary driver file 'ksi.dll-old' exists and try to delete it.
 *
 * This function checks the application directory for the existence of the
 * legacy filename 'ksi.dll-old'. If it exists the function attempts to delete
 * it; if deletion fails the function returns TRUE indicating that a reboot
 * is likely required (the file is still locked).
 * \return TRUE if an old KSI file remains (and may require reboot), FALSE if
 *         no old file exists or deletion succeeded.
 */
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

/**
 * Obtain the configured KSI service name, falling back to default.
 *
 * \return Referenced `PPH_STRING` containing the service name.
 */
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

/**
 * KPH communication callback used by KsiConnect configuration.
 *
 * The callback forwards the message to the informer dispatch and also,
 * when appropriate, forces a KPH cached required-state refresh when a
 * RequiredStateFailure message relates to the current process.
 *
 * \param[in] ReplyToken Opaque reply token from KphComms.
 * \param[in] Message Pointer to the KPH message to process.
 * \return BOOLEAN value expected by KPH_COMMS_CALLBACK (TRUE/ FALSE).
 */
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

/**
 * Read a configuration file located in the application directory.
 *
 * This opens and reads the entire file into a buffer allocated by the Ph
 * memory allocator. The caller receives ownership of `*Data` and must free it
 * via PhFree(). On success `*Length` receives the byte length of the buffer.
 *
 * \param[in] FileName File name relative to application directory.
 * \param[out] Data Receives pointer to allocated buffer with file contents.
 * \param[out] Length Receives buffer length in bytes.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Validate dynamic configuration blob against the current kernel version.
 *
 * Performs a lookup using KphDynDataLookup matching the runtime kernel support
 * data (class, machine, timestamp, size) to ensure the provided dynamic
 * configuration applies to this kernel.
 *
 * \param[in] DynData Pointer to dynamic configuration buffer.
 * \param[in] DynDataLength Buffer length in bytes.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Retrieve the dynamic configuration and its signature.
 *
 * This routine attempts to read a local `ksidyn.bin` and `ksidyn.sig` files,
 * validate the dynamic configuration for the current kernel and return both
 * the data and signature buffers to the caller. Caller owns the returned
 * buffers and must free them using PhFree().
 *
 * \param[out] DynData Receives pointer to allocated dynamic data buffer.
 * \param[out] DynDataLength Receives the length of the dynamic data buffer.
 * \param[out] Signature Receives pointer to allocated signature buffer.
 * \param[out] SignatureLength Receives length of the signature buffer.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Determine the on-disk kernel driver filename for the current application context.
 *
 * \return Referenced `PPH_STRING` containing the full driver filename.
 */
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

/**
 * Get a temporary directory suitable for creating temporary files.
 *
 * Returns a referenced `PPH_STRING` representing the temporary directory
 * for the running application context.
 */
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

/**
 * Create a temporary copy of a driver file in the specified temporary directory.
 *
 * The function generates a filename based on the current system time, ensures
 * the directory exists and copies `FileName` into `TempDirectory`. On success
 * returns a referenced `PPH_STRING` in `*TempFileName` (caller owns reference).
 *
 * \param[in] FileName Existing driver filename to copy.
 * \param[in] TempDirectory Directory to create the temporary file in.
 * \param[out] TempFileName Receives referenced `PPH_STRING` of the created file.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Create a randomized alphanumeric name with the given prefix.
 *
 * Generates an 8-12 character random alpha string and concatenates it to the
 * provided prefix producing a new referenced `PPH_STRING` returned via `Name`.
 *
 * \param[in] Prefix Wide string prefix to prepend.
 * \param[out] Name Receives referenced `PPH_STRING` containing the new name.
 * \return BOOLEAN TRUE on success, FALSE on failure.
 */
BOOLEAN KsiCreateRandomizedName(
    _In_ PCWSTR Prefix,
    _Out_ PPH_STRING* Name
    )
{
    ULONG length;
    WCHAR buffer[13];

    length = (PhGenerateRandomNumber64() % 6) + 8; // 8-12 characters
    PhGenerateRandomAlphaString(buffer, length);

    *Name = PhConcatStrings2(Prefix, buffer);
    return TRUE;
}

/**
 * Connect to the kernel driver using the configured dynamic configuration.
 *
 * This function orchestrates reading dynamic configuration, validating it,
 * ensuring the driver file exists, and attempting connection with multiple
 * fallback strategies (temporary copy, service, NtLoadDriver etc...).
 *
 * \param[in] WindowHandle Optional parent window for UI prompts.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS KsiConnect(
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

#if defined(KSI_ONLINE_PLATFORM_SUPPORT)
    if (status == STATUS_SI_DYNDATA_UNSUPPORTED_KERNEL)
    {
        KsiShowKernelSupportCheckDialog(WindowHandle);
        goto CleanupExit;
    }
#else
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
#endif

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
    config.EnableNativeLoad = KsiEnableLoadNative;
    config.EnableFilterLoad = KsiEnableLoadFilter;
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

    return status;
}

/**
 * Thread callback routine that initializes KSI and notifies the originating window.
 * \param[in] CallbackContext Optional HWND passed as a void*; if provided
 * the routine sends TDM_CLICK_BUTTON to the window when initialization completes.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Dialog callback invoked by the splash screen TaskDialogIndirect.
 *
 * Handles creation notification by spawning the initialization thread and
 * updates marquee/timer UI. Also handles cancel button to abort the dialog.
 *
 * \param[in] WindowHandle HWND of the task dialog.
 * \param[in] Notification Task dialog notification code.
 * \param[in] wParam Notification-specific parameter.
 * \param[in] lParam Notification-specific parameter.
 * \param[in] Context User defined context pointer (unused).
 * \return HRESULT S_OK to continue default processing, S_FALSE to abort dialog.
 */
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

/**
 * Display a splash screen while the kernel driver initializes.
 *
 * This function configures and invokes a TaskDialogIndirect with a marquee
 * progress bar and callback to spawn the initialization thread.
 */
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

/**
 * Initialize the KSI subsystem for the application.
 *
 * Checks preconditions (elevation, OS version, architecture, old leftover
 * files) and then initializes underlying Kph and informer subsystems. If the
 * splash-screen setting is enabled the initialization will be done asynchronously
 * with a UI; otherwise initialization is performed synchronously.
 */
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

/**
 * Cleanup KSI subsystems and optionally unload the driver on exit.
 *
 * If the communications channel is connected this will stop communications and,
 * depending on settings and client count, attempt to stop and unload the
 * driver service. Returns the NTSTATUS of the unload operation when attempted.
 * \return STATUS_SUCCESS when no unload required or unload succeeded, otherwise
 *         an NTSTATUS indicating the error encountered during service stop.
 */
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

/**
 * Parse a hex-encoded JSON settings blob to extract KSI settings.
 *
 * The function decodes the hex string stored in `KsiSettingsBlob`, parses the
 * JSON and extracts "KsiDirectory" and "KsiServiceName" entries. If both are
 * present the function sets `*Directory` and `*ServiceName` to referenced
 * strings that the caller must dereference.
 *
 * \param[in] KsiSettingsBlob Hex-encoded buffer stored as a PPH_STRING.
 * \param[out] Directory Receives referenced `PPH_STRING` of the directory.
 * \param[out] ServiceName Receives referenced `PPH_STRING` of the service name.
 * \return TRUE if both values were found and returned, FALSE otherwise.
 */
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
/**
 * Create a small configuration blob containing the KSI directory and service name.
 *
 * This helper constructs a JSON object containing the current `KsiDirectory`
 * and `KsiServiceName`, converts it to a UTF-8 JSON string, and returns the
 * value encoded as a hex string `PPH_STRING`. The returned string is a
 * referenced object and the caller must free it.
 * \return Referenced `PPH_STRING` containing the hex-encoded JSON settings.
 */
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

/**
 * Measures latency of the KPH driver using its performance counter interface.
 *
 * If the current KSI level is below KphLevelLow, the function sets all output
 * values to zero and returns STATUS_UNSUCCESSFUL. Otherwise it queries the KPH
 * performance counter and measures timestamps before and after the kernel call
 * to compute:
 *
 *  - Duration:     Total round-trip time (local wall clock)
 *  - DurationDown: Time spent entering the kernel (downstream)
 *  - DurationUp:   Time spent returning from the kernel (upstream)
 *
 * \param[out] Duration      Total measured duration.
 * \param[out] DurationDown  Time spent sending the request to the kernel.
 * \param[out] DurationUp    Time spent recieving the response from the kernel.
 * \return NTSTATUS from KphQueryPerformanceCounter, or STATUS_UNSUCCESSFUL.
 * \remarks Use this function when you need a kernel-reported high-resolution
 * timebase to measure or correlate user/kernel timing.
 */
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

#if defined(KSI_ONLINE_PLATFORM_SUPPORT)
PPH_STRING KsiCreateBuildString(
    VOID
    )
{
    static const PH_STRINGREF versionHeader = PH_STRINGREF_INIT(L"KsiBuild: ");
    ULONG majorVersion;
    ULONG minorVersion;
    ULONG buildVersion;
    ULONG revisionVersion;
    SIZE_T returnLength;
    PH_FORMAT format[8];
    WCHAR formatBuffer[260];

    PhGetPhVersionNumbers(&majorVersion, &minorVersion, &buildVersion, &revisionVersion);
    PhInitFormatSR(&format[0], versionHeader);
    PhInitFormatU(&format[1], majorVersion);
    PhInitFormatC(&format[2], L'.');
    PhInitFormatU(&format[3], minorVersion);
    PhInitFormatC(&format[4], L'.');
    PhInitFormatU(&format[5], buildVersion);
    PhInitFormatC(&format[6], L'.');
    PhInitFormatU(&format[7], revisionVersion);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
    {
        PH_STRINGREF stringFormat;

        stringFormat.Buffer = formatBuffer;
        stringFormat.Length = returnLength - sizeof(UNICODE_NULL);

        return PhCreateString2(&stringFormat);
    }
    else
    {
        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
}

NTSTATUS KsiCreatePlatformSupportInformation(
    _In_ PCPH_STRINGREF FileName,
    _Out_ PUSHORT ImageMachine,
    _Out_ PULONG TimeDateStamp,
    _Out_ PULONG SizeOfImage,
    _Out_writes_bytes_to_(OutputLength, *ReturnLength) PWSTR OutputBuffer,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    USHORT imageMachine;
    ULONG imageDateStamp;
    ULONG imageSizeOfImage;
    PH_MAPPED_IMAGE mappedImage;
    LARGE_INTEGER fileSize;
    PH_HASH_CONTEXT hashContext;
    ULONG64 bytesRemaining;
    ULONG numberOfBytesRead;
    ULONG bufferLength;
    PBYTE buffer;
    UCHAR hash[PH_HASH_SHA256_LENGTH];

    status = PhCreateFile(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetFileSize(fileHandle, &fileSize);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhInitializeHash(&hashContext, Sha256HashAlgorithm);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    bufferLength = PAGE_SIZE * 2;
    buffer = PhAllocateSafe(bufferLength);

    if (!buffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    bytesRemaining = (ULONG64)fileSize.QuadPart;

    while (bytesRemaining)
    {
        status = PhReadFile(
            fileHandle,
            buffer,
            bufferLength,
            NULL,
            &numberOfBytesRead
            );

        if (!NT_SUCCESS(status))
            break;

        status = PhUpdateHash(
            &hashContext,
            buffer,
            numberOfBytesRead
            );

        if (!NT_SUCCESS(status))
            break;

        bytesRemaining -= numberOfBytesRead;
    }

    PhFree(buffer);

    status = PhFinalHash(
        &hashContext,
        hash,
        sizeof(hash),
        NULL
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!PhBufferToHexStringBuffer(
        hash,
        sizeof(hash),
        FALSE,
        OutputBuffer,
        OutputLength,
        ReturnLength
        ))
    {
        status = STATUS_FAIL_CHECK;
        goto CleanupExit;
    }

    status = PhLoadMappedImageHeaderPageSize(
        NULL,
        fileHandle,
        &mappedImage
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    __try
    {
        imageMachine = mappedImage.NtHeaders->FileHeader.Machine;
        imageDateStamp = mappedImage.NtHeaders->FileHeader.TimeDateStamp;
        imageSizeOfImage = mappedImage.NtHeaders->OptionalHeader.SizeOfImage;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    PhUnloadMappedImage(&mappedImage);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (NT_SUCCESS(status))
    {
        *ImageMachine = imageMachine;
        *TimeDateStamp = imageDateStamp;
        *SizeOfImage = imageSizeOfImage;

        NtClose(fileHandle);
        return STATUS_SUCCESS;
    }

CleanupExit:
    NtClose(fileHandle);
    return status;
}

typedef struct _KSI_PLATFORM_BUILD_ENTRY
{
    USHORT Class;
    PH_STRINGREF FileName;
} KSI_PLATFORM_BUILD_ENTRY, *PKSI_PLATFORM_BUILD_ENTRY;

PPH_STRING KsiCreatePlatformBuildString(
    VOID
    )
{
    static CONST PH_STRINGREF platformHeader = PH_STRINGREF_INIT(L"KsiPlatformSupport: ");
    static CONST KSI_PLATFORM_BUILD_ENTRY platformFiles[] =
    {
        { KPH_DYN_CLASS_NTOSKRNL, PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntoskrnl.exe") },
        { KPH_DYN_CLASS_NTKRLA57, PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\ntkrla57.exe") },
        { KPH_DYN_CLASS_LXCORE,   PH_STRINGREF_INIT(L"\\SystemRoot\\System32\\drivers\\lxcore.sys") },
    };

    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 30);
    PhAppendStringBuilder(&stringBuilder, &platformHeader);
    PhAppendStringBuilder2(&stringBuilder, L"{\"version\":1,");
    PhAppendStringBuilder2(&stringBuilder, L"\"files\":[");

    for (ULONG i = 0; i < RTL_NUMBER_OF(platformFiles); i++)
    {
        USHORT imageMachine;
        ULONG timeDateStamp;
        ULONG sizeOfImage;
        SIZE_T outputLength = 0;
        WCHAR outputBuffer[PH_HASH_SHA256_LENGTH * 2 + 1];

        if (NT_SUCCESS(KsiCreatePlatformSupportInformation(
            &platformFiles[i].FileName,
            &imageMachine,
            &timeDateStamp,
            &sizeOfImage,
            outputBuffer,
            sizeof(outputBuffer),
            &outputLength
            )))
        {
            PPH_STRING string;
            PH_STRINGREF hashString;
            PH_FORMAT format[11];

            hashString.Buffer = outputBuffer;
            hashString.Length = outputLength;

            PhInitFormatS(&format[0], L"{\"hash\":\"");
            PhInitFormatSR(&format[1], hashString);
            PhInitFormatS(&format[2], L"\",\"file\":");
            PhInitFormatU(&format[3], platformFiles[i].Class);
            PhInitFormatS(&format[4], L",\"machine\":");
            PhInitFormatU(&format[5], imageMachine);
            PhInitFormatS(&format[6], L",\"timestamp\":");
            PhInitFormatU(&format[7], timeDateStamp);
            PhInitFormatS(&format[8], L",\"size\":");
            PhInitFormatU(&format[9], sizeOfImage);
            PhInitFormatS(&format[10], L"},");

            string = PhFormat(format, RTL_NUMBER_OF(format), 10);
            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhDereferenceObject(string);
        }
    }

    if (PhEndsWithString2(stringBuilder.String, L",", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    PhAppendStringBuilder2(&stringBuilder, L"]}");

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING KsiCreateUpdateWindowsString(
    VOID
    )
{
    PPH_STRING buildString = NULL;
    PPH_STRING fileName;
    PVOID imageBase;
    ULONG imageSize;
    PVOID versionInfo;

    if (NT_SUCCESS(PhGetKernelFileNameEx(&fileName, &imageBase, &imageSize)))
    {
        if (NT_SUCCESS(PhGetFileVersionInfoEx(&fileName->sr, &versionInfo)))
        {
            VS_FIXEDFILEINFO* rootBlock;

            if (rootBlock = PhGetFileVersionFixedInfo(versionInfo))
            {
                PH_FORMAT format[5];

                PhInitFormatS(&format[0], L"KsiOsBuild: ");
                PhInitFormatU(&format[1], HIWORD(rootBlock->dwFileVersionLS));
                PhInitFormatC(&format[2], '.');
                PhInitFormatU(&format[3], LOWORD(rootBlock->dwFileVersionLS));
                PhInitFormatS(&format[4], PhIsExecutingInWow64() ? L"_64" : L"_32");

                buildString = PhFormat(format, RTL_NUMBER_OF(format), 0);
            }

            PhFree(versionInfo);
        }

        PhDereferenceObject(fileName);
    }

    return buildString;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS KsiCheckKernelSupportThread(
    _In_ PVOID Parameter
    )
{
    PKSI_KERNEL_SUPPORT_CHECK_CONTEXT context = Parameter;
    NTSTATUS status;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING xmlData = NULL;
    PPH_STRING searchString = NULL;

    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, L"systeminformer.io", PH_HTTP_DEFAULT_HTTPS_PORT)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(httpContext, L"POST", L"/ksiver", PH_HTTP_FLAG_SECURE)))
        goto CleanupExit;

    {
        PPH_STRING buildStringHeader;
        PPH_STRING updateStringHeader;
        PPH_STRING platformStringHeader;

        if (buildStringHeader = KsiCreateBuildString())
        {
            PhHttpAddRequestHeadersSR(httpContext, &buildStringHeader->sr);
            PhDereferenceObject(buildStringHeader);
        }

        if (updateStringHeader = KsiCreateUpdateWindowsString())
        {
            PhHttpAddRequestHeadersSR(httpContext, &updateStringHeader->sr);
            PhDereferenceObject(updateStringHeader);
        }

        if (platformStringHeader = KsiCreatePlatformBuildString())
        {
            PhHttpAddRequestHeadersSR(httpContext, &platformStringHeader->sr);
            PhDereferenceObject(platformStringHeader);
        }
    }

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, NULL, 0, NULL, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryResponseStatus(httpContext)))
        goto CleanupExit;

    WriteBooleanRelease(&context->IsSupported, TRUE);

CleanupExit:
    if (searchString)
        PhDereferenceObject(searchString);
    if (xmlData)
        PhDereferenceObject(xmlData);
    if (httpContext)
        PhHttpDestroy(httpContext);

    WriteBooleanRelease(&context->CheckComplete, TRUE);

    return STATUS_SUCCESS;
}

HRESULT CALLBACK KsiKernelSupportCheckDialogCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PKSI_KERNEL_SUPPORT_CHECK_CONTEXT context = (PKSI_KERNEL_SUPPORT_CHECK_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_CREATED:
        {
            context->TaskDialogHandle = hwndDlg;

            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhCreateThread2(KsiCheckKernelSupportThread, context);
        }
        break;
    case TDN_DESTROYED:
        {
            context->TaskDialogHandle = NULL;
        }
        break;
    case TDN_TIMER:
        {
            if (ReadBooleanAcquire(&context->CheckComplete))
            {
                WriteBooleanRelease(&context->CheckComplete, TRUE);

                TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
                config.hwndParent = context->WindowHandle;
                config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
                config.dwCommonButtons = TDCBF_OK_BUTTON;
                config.pszWindowTitle = PhApplicationName;
                config.pszMainIcon = TD_SHIELD_WARNING_ICON;
                config.cxWidth = 200;

                if (ReadBooleanAcquire(&context->IsSupported))
                {
                    config.pszMainInstruction = L"Platform support pending review.";
                    config.pszContent = L"Your kernel version is pending review on the development branch. "
                        L"Your kernel will be supported in the next build!";
                }
                else
                {
                    if (context->IsCanaryChannel)
                    {
                        config.pszMainInstruction = L"Kernel version not supported";
                        config.pszContent = L"This kernel version is not yet supported. "
                            L"Your kernel version is pending review review on the development branch.";
                    }
                    else
                    {
                        config.pszMainInstruction = L"Kernel version not supported";
                        config.pszContent = L"This kernel version is not yet supported. "
                            L"For the latest kernel support switch to the Canary update channel "
                            L"(Help > Check for updates > Canary > Check).";
                    }
                }

                PhTaskDialogNavigatePage(hwndDlg, &config);
            }
        }
        break;
    }

    return S_OK;
}

VOID KsiShowKernelSupportCheckDialog(
    _In_opt_ HWND WindowHandle
    )
{
    KSI_KERNEL_SUPPORT_CHECK_CONTEXT context = { 0 };
    PPH_STRING statusMessage;

    statusMessage = PhpGetKsiMessage2(
        STATUS_SI_DYNDATA_UNSUPPORTED_KERNEL,
        FALSE,
        L"Checking for pending platform update...",
        NULL
        );

    context.WindowHandle = WindowHandle;
    context.IsCanaryChannel = (PhGetPhReleaseChannel() >= PhCanaryChannel);
    KsiGetKernelSupportData(&context.SupportData);

    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    config.hwndParent = WindowHandle;
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_SHOW_PROGRESS_BAR | TDF_POSITION_RELATIVE_TO_WINDOW | TDF_CALLBACK_TIMER;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainIcon = TD_SHIELD_WARNING_ICON;
    config.pszMainInstruction = L"Checking for pending platform update...";
    config.pszContent = PhGetString(statusMessage);
    config.lpCallbackData = (LONG_PTR)&context;
    config.pfCallback = KsiKernelSupportCheckDialogCallbackProc;
    config.cxWidth = 200;

    PhShowTaskDialog(&config, NULL, NULL, NULL);
}
#endif
