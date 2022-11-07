/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *     dmex    2022
 *
 */

#include <phapp.h>
#include <kphuser.h>
#include <kphcomms.h>
#include <settings.h>

#include <ksisup.h>
#include <phsettings.h>

VOID PhShowKsiStatus(
    VOID
    )
{
    KPH_PROCESS_STATE processState;

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
            TDCBF_OK_BUTTON,
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

VOID PhShowKsiError(
    _In_ PWSTR Message,
    _In_opt_ NTSTATUS Status
    )
{
    if (Status == STATUS_NO_SUCH_FILE)
    {
        if (PhShowMessageOneTime(
            NULL,
            TDCBF_OK_BUTTON,
            TD_ERROR_ICON,
            Message,
            L"%s",
            L"You will be unable to use more advanced features, view details about system processes or terminate malicious software."
            ))
        {
            PhSetIntegerSetting(L"EnableKphWarnings", FALSE);
        }
    }
    else
    {
        PPH_STRING errorMessage;
        PPH_STRING statusMessage;

        if (!(errorMessage = PhGetStatusMessage(Status, 0)))
            errorMessage = PhGetStatusMessage(0, Status);

        if (errorMessage)
        {
            statusMessage = PhConcatStrings(
                3,
                PhGetString(errorMessage),
                L"\r\n\r\n",
                L"You will be unable to use more advanced features, view details about system processes or terminate malicious software."
                );

            if (PhShowMessageOneTime(
                NULL,
                TDCBF_OK_BUTTON,
                TD_ERROR_ICON,
                Message,
                L"%s",
                PhGetString(statusMessage)
                ))
            {
                PhSetIntegerSetting(L"EnableKphWarnings", FALSE);
            }

            PhDereferenceObject(statusMessage);
            PhDereferenceObject(errorMessage);
        }
        else
        {
            if (PhShowMessageOneTime(
                NULL,
                TDCBF_OK_BUTTON,
                TD_ERROR_ICON,
                Message,
                L"%s",
                L"You will be unable to use more advanced features, view details about system processes or terminate malicious software."
                ))
            {
                PhSetIntegerSetting(L"EnableKphWarnings", FALSE);
            }
        }
    }
}

static VOID NTAPI KsiCommsCallback(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_FREE_LIST freelist;
    PKPH_MESSAGE msg;

    if (Message->Header.MessageId != KphMsgProcessCreate)
    {
        return;
    }

    freelist = KphGetMessageFreeList();

    msg = PhAllocateFromFreeList(freelist);
    KphMsgInit(msg, KphMsgProcessCreate);
    msg->Reply.ProcessCreate.CreationStatus = STATUS_SUCCESS;
    KphCommsReplyMessage(ReplyToken, msg);

    PhFreeToFreeList(freelist, msg);
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

BOOLEAN PhDoesNewKsiExist(
    VOID
    )
{
    static PH_STRINGREF ksiNew = PH_STRINGREF_INIT(L"ksi.dll-new");
    BOOLEAN result = FALSE;
    PPH_STRING applicationDirectory;
    PPH_STRING fileName;

    if (!(applicationDirectory = PhGetApplicationDirectory()))
        return FALSE;

    if (fileName = PhConcatStringRef2(&applicationDirectory->sr, &ksiNew))
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

VOID PhInitializeKsi(
    VOID
    )
{
    static PH_STRINGREF driverExtension = PH_STRINGREF_INIT(L".sys");
    NTSTATUS status;
    PPH_STRING fileName = NULL;
    PPH_STRING ksiFileName = NULL;
    PPH_STRING ksiServiceName = NULL;

    if (WindowsVersion < WINDOWS_10 || WindowsVersion == WINDOWS_NEW)
        return;

    if (PhDoesNewKsiExist())
    {
        if (PhGetIntegerSetting(L"EnableKphWarnings") && !PhStartupParameters.PhSvc)
        {
            PhShowKsiError(
                L"Unable to load kernel driver, the last System Informer update requires a reboot.",
                STATUS_PENDING 
                );
        }
        return;
    }

    if (!(fileName = PhGetApplicationFileNameWin32()))
        goto CleanupExit;
    if (!(ksiFileName = PhGetBaseNamePathWithExtension(fileName, &driverExtension)))
        goto CleanupExit;

    ksiServiceName = PhGetKsiServiceName();

    // Reset driver parameters after a Windows build update. (dmex)
    {
        ULONG latestBuildNumber = PhGetIntegerSetting(L"KphBuildNumber");

        if (latestBuildNumber == 0)
        {
            PhSetIntegerSetting(L"KphBuildNumber", PhOsVersion.dwBuildNumber);
        }
        else
        {
            if (latestBuildNumber != PhOsVersion.dwBuildNumber)
            {
                if (KphParametersExists(PhGetString(ksiServiceName)))
                {
                    if (PhShowMessage(
                        NULL,
                        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2,
                        L"System Informer needs to reset the driver configuration\r\n\r\n%s",
                        L"Do you want to continue?"
                        ) != IDYES)
                    {
                        return;
                    }

                    if (NT_SUCCESS(KphResetParameters(PhGetString(ksiServiceName))))
                    {
                        PhSetIntegerSetting(L"KphBuildNumber", PhOsVersion.dwBuildNumber);
                    }
                }
                else
                {
                    PhSetIntegerSetting(L"KphBuildNumber", PhOsVersion.dwBuildNumber);
                }
            }
        }
    }

    if (ksiServiceName && PhDoesFileExistWin32(PhGetString(ksiFileName)))
    {
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

        status = KphConnect(
            &ksiServiceName->sr,
            &objectName->sr,
            &portName->sr,
            &ksiFileName->sr,
            &altitudeName->sr,
            disableImageLoadProtection,
            KsiCommsCallback
            );

        if (NT_SUCCESS(status))
        {
            KPH_LEVEL level = KphLevel();

            if (!NtCurrentPeb()->BeingDebugged && (level != KphLevelMax))
            {
                if ((level == KphLevelHigh) &&
                    !PhStartupParameters.KphStartupMax)
                {
                    PH_STRINGREF commandline = PH_STRINGREF_INIT(L"-kx");
                    PhRestartSelf(&commandline);
                }

                if ((level < KphLevelHigh) &&
                    !PhStartupParameters.KphStartupMax &&
                    !PhStartupParameters.KphStartupHigh)
                {
                    PH_STRINGREF commandline = PH_STRINGREF_INIT(L"-kh");
                    PhRestartSelf(&commandline);
                }
            }
        }
        else
        {
            if (PhGetIntegerSetting(L"EnableKphWarnings") && PhGetOwnTokenAttributes().Elevated && !PhStartupParameters.PhSvc)
                PhShowKsiError(L"Unable to load the kernel driver service.", status);
        }

        PhClearReference(&objectName);
    }
    else
    {
        if (PhGetIntegerSetting(L"EnableKphWarnings") && !PhStartupParameters.PhSvc)
            PhShowKsiError(L"The kernel driver was not found.", STATUS_NO_SUCH_FILE);
    }

CleanupExit:
    if (ksiServiceName)
        PhDereferenceObject(ksiServiceName);
    if (ksiFileName)
        PhDereferenceObject(ksiFileName);
    if (fileName)
        PhDereferenceObject(fileName);
}

VOID PhDestroyKsi(
    VOID
    )
{
    KphCommsStop();
}
