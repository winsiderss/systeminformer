/*
 * Process Hacker Toolchain -
 *   project setup
 *
 * Copyright (C) 2017 dmex
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

#ifndef _APPSUP_H
#define _APPSUP_H

extern PH_STRINGREF UninstallKeyName;

typedef struct _SETUP_EXTRACT_FILE
{
    PSTR FileName;
    PWSTR ExtractFileName;
} SETUP_EXTRACT_FILE, *PSETUP_EXTRACT_FILE;

typedef struct _SETUP_REMOVE_FILE
{
    PWSTR FileName;
} SETUP_REMOVE_FILE, *PSETUP_REMOVE_FILE;

PPH_STRING SetupFindInstallDirectory(
    VOID
    );

PVOID ExtractResourceToBuffer(
    _In_ PWSTR Resource
    );

VOID SetupDeleteDirectoryFile(
    _In_ PWSTR FileName
    );

VOID SetupInitializeFont(
    _In_ HWND ControlHandle,
    _In_ LONG Height,
    _In_ LONG Weight
    );

VOID SetupCreateLink(
    _In_ PWSTR DestFilePath,
    _In_ PWSTR FilePath,
    _In_ PWSTR FileParentDir
    );

BOOLEAN DialogPromptExit(
    _In_ HWND hwndDlg
    );

HBITMAP LoadPngImageFromResources(
    _In_ PCWSTR Name
    );

BOOLEAN CheckProcessHackerInstalled(
    VOID
    );

PPH_STRING GetProcessHackerInstallPath(
    VOID
    );

BOOLEAN ShutdownProcessHacker(
    VOID
    );

BOOLEAN CreateDirectoryPath(
    _In_ PWSTR DirectoryPath
    );

BOOLEAN RemoveDirectoryPath(
    _In_ PWSTR DirPath
    );


#endif