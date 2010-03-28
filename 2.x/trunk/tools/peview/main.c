/*
 * Process Hacker - 
 *   PE viewer
 * 
 * Copyright (C) 2010 wj32
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

#define MAIN_PRIVATE
#include <peview.h>

PPH_STRING PvFileName = NULL;

static BOOLEAN NTAPI PvCommandLineCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in PVOID Context
    )
{
    if (!Option)
        PhSwapReference(&PvFileName, Value);

    return TRUE;
}

INT WINAPI WinMain(
    __in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in LPSTR lpCmdLine,
    __in INT nCmdShow
    )
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { 0, L"h", NoArgumentType }
    };
    PH_STRINGREF commandLine;

    if (!NT_SUCCESS(PhInitializePhLib()))
        return 1;

    PhGuiSupportInitialization();

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    PhApplicationName = L"PE Viewer";

    commandLine.us = NtCurrentPeb()->ProcessParameters->CommandLine;

    PhParseCommandLine(
        &commandLine,
        options,
        sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
        PH_COMMAND_LINE_IGNORE_FIRST_PART,
        PvCommandLineCallback,
        NULL
        );

    if (!PvFileName)
    {
        static PH_FILETYPE_FILTER filters[] =
        {
            { L"Supported files (*.exe;*.dll;*.ocx;*.sys;*.scr;*.cpl;*.lib)", L"*.exe;*.dll;*.ocx;*.sys;*.scr;*.cpl;*.lib" },
            { L"All files (*.*)", L"*.*" }
        };
        PVOID fileDialog;

        fileDialog = PhCreateOpenFileDialog();
        PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

        if (PhShowFileDialog(NULL, fileDialog))
        {
            PvFileName = PhGetFileDialogFileName(fileDialog);
        }

        PhFreeFileDialog(fileDialog);
    }

    if (!PvFileName)
        return 1;

    if (!PhStringEndsWith2(PvFileName, L".lib", TRUE))
        PvPeProperties();
    else
        PvLibProperties();

    return 0;
}
