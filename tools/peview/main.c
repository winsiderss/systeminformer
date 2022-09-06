/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2021
 *
 */

#include <peview.h>

PPH_STRING PvFileName = NULL;

BOOLEAN PvInitializeExceptionPolicy(
    VOID
    );

BOOLEAN NTAPI PvpCommandLineCallback(
    _In_opt_ PPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    if (!Option)
        PhSwapReference(&PvFileName, Value);

    return TRUE;
}

INT WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR lpCmdLine,
    _In_ INT nCmdShow
    )
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { 0, L"h", NoArgumentType }
    };
    PH_STRINGREF commandLine;

    if (!NT_SUCCESS(PhInitializePhLib(L"PE Viewer", hInstance)))
        return 1;
    if (!PvInitializeExceptionPolicy())
        return 1;

    // Create a mutant for the installer.
    {
        HANDLE mutantHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING objectName;
        PH_STRINGREF objectNameSr;
        SIZE_T returnLength;
        WCHAR formatBuffer[PH_INT64_STR_LEN_1];
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"SiViewerMutant_");
        PhInitFormatU(&format[1], HandleToUlong(NtCurrentProcessId()));

        if (!PhFormatToBuffer(
            format,
            RTL_NUMBER_OF(format),
            formatBuffer,
            sizeof(formatBuffer),
            &returnLength
            ))
        {
            return FALSE;
        }

        objectNameSr.Length = returnLength - sizeof(UNICODE_NULL);
        objectNameSr.Buffer = formatBuffer;

        if (!PhStringRefToUnicodeString(&objectNameSr, &objectName))
            return FALSE;

        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_CASE_INSENSITIVE,
            PhGetNamespaceHandle(),
            NULL
            );

        NtCreateMutant(
            &mutantHandle,
            MUTANT_QUERY_STATE,
            &objectAttributes,
            TRUE
            );
    }

#ifndef DEBUG
    if (PhIsExecutingInWow64())
    {
        PhShowWarning(
            NULL,
            L"%s",
            L"You are attempting to run the 32-bit version of PE Viewer on 64-bit Windows. "
            L"Most features will not work correctly.\n\n"
            L"Please run the 64-bit version of PE Viewer instead."
            );
        PhExitApplication(STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT);
    }
#endif

    PhGuiSupportInitialization();
    PhSettingsInitialization();
    PvInitializeSettings();
    PvPropInitialization();
    PhTreeNewInitialization();

    if (!NT_SUCCESS(PhGetProcessCommandLineStringRef(&commandLine)))
        return 1;

    PhParseCommandLine(
        &commandLine,
        options,
        RTL_NUMBER_OF(options),
        PH_COMMAND_LINE_IGNORE_FIRST_PART,
        PvpCommandLineCallback,
        NULL
        );

    if (!PvFileName)
    {
        static PH_FILETYPE_FILTER filters[] =
        {
            { L"Supported files (*.exe;*.dll;*.com;*.ocx;*.sys;*.scr;*.cpl;*.ax;*.acm;*.lib;*.winmd;*.mui;*.mun;*.efi;*.pdb)", L"*.exe;*.dll;*.com;*.ocx;*.sys;*.scr;*.cpl;*.ax;*.acm;*.lib;*.winmd;*.mui;*.mun;*.efi;*.pdb" },
            { L"All files (*.*)", L"*.*" }
        };
        PVOID fileDialog;

        if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
            return 1;

        fileDialog = PhCreateOpenFileDialog();
        PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_SHOWHIDDEN | PH_FILEDIALOG_NOPATHVALIDATE);
        PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));

        if (PhShowFileDialog(NULL, fileDialog))
        {
            if (PvFileName = PhGetFileDialogFileName(fileDialog))
            {
#ifndef DEBUG
                PPH_STRING applicationFileName;

                if (applicationFileName = PhGetApplicationFileNameWin32())
                {
                    PhMoveReference(&PvFileName, PhConcatStrings(3, L"\"", PvFileName->Buffer, L"\""));

                    AllowSetForegroundWindow(ASFW_ANY);

                    if (PhShellExecuteEx(
                        NULL,
                        PhGetString(applicationFileName),
                        PvFileName->Buffer,
                        SW_SHOWNORMAL,
                        PH_SHELL_EXECUTE_NOZONECHECKS,
                        0,
                        NULL
                        ))
                    {
                        PhExitApplication(STATUS_SUCCESS);
                    }

                    PhDereferenceObject(applicationFileName);
                }
#endif
            }
        }

        PhFreeFileDialog(fileDialog);
    }

    if (PhIsNullOrEmptyString(PvFileName))
        return 1;

#ifdef DEBUG
    if (!PhDoesFileExistWin32(PhGetString(PvFileName)))
    {
        PPH_STRING fileName;

        fileName = PhGetBaseName(PvFileName);
        PhMoveReference(&fileName, PhSearchFilePath(PhGetString(fileName), NULL));

        if (!PhIsNullOrEmptyString(fileName))
        {
            PhMoveReference(&PvFileName, fileName);
        }
    }
#endif

    if (PhEndsWithString2(PvFileName, L".lnk", TRUE))
    {
        PPH_STRING targetFileName;

        targetFileName = PvResolveShortcutTarget(PvFileName);

        if (targetFileName)
            PhMoveReference(&PvFileName, targetFileName);
    }

    if (PhEndsWithString2(PvFileName, L".lib", TRUE))
        PvLibProperties();
    else if (PhEndsWithString2(PvFileName, L".pdb", TRUE))
        PvPdbProperties();
    else
    {
        NTSTATUS status;
        HANDLE fileHandle;

        status = PhCreateFileWin32(
            &fileHandle,
            PhGetString(PvFileName),
            FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            );

        if (status == STATUS_IO_REPARSE_TAG_NOT_HANDLED)
        {
            PPH_STRING targetFileName;

            if (targetFileName = PvResolveReparsePointTarget(PvFileName))
            {
                PhMoveReference(&PvFileName, targetFileName);
            }

            status = PhCreateFileWin32(
                &fileHandle,
                PhGetString(PvFileName),
                FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
        }

        if (NT_SUCCESS(status))
        {
            status = PhLoadMappedImageEx(
                NULL,
                fileHandle,
                &PvMappedImage
                );
            NtClose(fileHandle);

            if (NT_SUCCESS(status))
            {
                switch (PvMappedImage.Signature)
                {
                case IMAGE_DOS_SIGNATURE:
                    {
                        if (PhGetIntegerSetting(L"EnableLegacyPropertiesDialog"))
                            PvPeProperties();
                        else
                            PvShowPePropertiesWindow();
                    }
                    break;
                case IMAGE_ELF_SIGNATURE:
                    PvExlfProperties();
                    break;
                default:
                    status = STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT;
                    break;
                }
            }

            if (NT_SUCCESS(status))
                PhUnloadMappedImage(&PvMappedImage);
        }

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT)
                PhShowError2(NULL, L"Unable to load the file.", L"%s", L"PE Viewer does not support this image type.");
            else
                PhShowStatus(NULL, L"Unable to load the file.", status, 0);
        }
    }

    PvSaveSettings();

    return 0;
}

#ifndef DEBUG
ULONG CALLBACK PvUnhandledExceptionCallback(
    _In_ PEXCEPTION_POINTERS ExceptionInfo
    )
{
    PPH_STRING errorMessage;
    PPH_STRING message;

    if (NT_NTWIN32(ExceptionInfo->ExceptionRecord->ExceptionCode))
        errorMessage = PhGetStatusMessage(0, WIN32_FROM_NTSTATUS(ExceptionInfo->ExceptionRecord->ExceptionCode));
    else
        errorMessage = PhGetStatusMessage(ExceptionInfo->ExceptionRecord->ExceptionCode, 0);

    message = PhFormatString(
        L"0x%08X (%s)",
        ExceptionInfo->ExceptionRecord->ExceptionCode,
        PhGetStringOrEmpty(errorMessage)
        );

    PhShowMessage(
        NULL,
        MB_OK | MB_ICONWARNING,
        L"PE Viewer has crashed :(\r\n\r\n%s",
        PhGetStringOrEmpty(message)
        );

    PhExitApplication(ExceptionInfo->ExceptionRecord->ExceptionCode);

    PhDereferenceObject(message);
    PhDereferenceObject(errorMessage);

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

BOOLEAN PvInitializeExceptionPolicy(
    VOID
    )
{
#ifndef DEBUG
    ULONG errorMode;
    
    if (NT_SUCCESS(PhGetProcessErrorMode(NtCurrentProcess(), &errorMode)))
    {
        errorMode &= ~(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
        PhSetProcessErrorMode(NtCurrentProcess(), errorMode);
    }

    RtlSetUnhandledExceptionFilter(PvUnhandledExceptionCallback);
#endif
    return TRUE;
}
