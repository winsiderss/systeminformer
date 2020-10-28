/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2010 wj32
 * Copyright (C) 2017-2020 dmex
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

#include <peview.h>

PPH_STRING PvFileName = NULL;

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
    PPH_STRING commandLine;

    if (!NT_SUCCESS(PhInitializePhLibEx(L"PE Viewer", ULONG_MAX, hInstance, 0, 0)))
        return 1;

    // Create a mutant for the installer.
    {
        HANDLE mutantHandle;
        PPH_STRING objectName;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING objectNameUs;
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"PeViewerMutant_");
        PhInitFormatU(&format[1], HandleToUlong(NtCurrentProcessId()));

        objectName = PhFormat(format, 2, 16);
        PhStringRefToUnicodeString(&objectName->sr, &objectNameUs);

        InitializeObjectAttributes(
            &objectAttributes,
            &objectNameUs,
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

        PhDereferenceObject(objectName);
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
    PeInitializeSettings();
    PvPropInitialization();
    PhTreeNewInitialization();

    if (!NT_SUCCESS(PhGetProcessCommandLine(NtCurrentProcess(), &commandLine)))
        return 1;

    PhParseCommandLine(
        &commandLine->sr,
        options,
        RTL_NUMBER_OF(options),
        PH_COMMAND_LINE_IGNORE_FIRST_PART,
        PvpCommandLineCallback,
        NULL
        );
    PhDereferenceObject(commandLine);

    if (!PvFileName)
    {
        static PH_FILETYPE_FILTER filters[] =
        {
            { L"Supported files (*.exe;*.dll;*.com;*.ocx;*.sys;*.scr;*.cpl;*.ax;*.acm;*.lib;*.winmd;*.efi;*.pdb)", L"*.exe;*.dll;*.com;*.ocx;*.sys;*.scr;*.cpl;*.ax;*.acm;*.lib;*.winmd;*.efi;*.pdb" },
            { L"All files (*.*)", L"*.*" }
        };
        PVOID fileDialog;

        if (!SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
            return 1;

        fileDialog = PhCreateOpenFileDialog();
        PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_NOPATHVALIDATE);
        PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));

        if (PhShowFileDialog(NULL, fileDialog))
        {
            if (PvFileName = PhGetFileDialogFileName(fileDialog))
            {
#ifndef DEBUG
                PPH_STRING applicationFileName;

                if (applicationFileName = PhGetApplicationFileName())
                {
                    PhMoveReference(&PvFileName, PhConcatStrings(3, L"\"", PvFileName->Buffer, L"\""));

                    if (PhShellExecuteEx(
                        NULL,
                        PhGetString(applicationFileName),
                        PvFileName->Buffer,
                        SW_SHOWDEFAULT,
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
                PhGetString(PvFileName),
                fileHandle,
                &PvMappedImage
                );
            NtClose(fileHandle);

            if (NT_SUCCESS(status))
            {
                switch (PvMappedImage.Signature)
                {
                case IMAGE_DOS_SIGNATURE:
                    PvPeProperties();
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

    PeSaveSettings();

    return 0;
}
