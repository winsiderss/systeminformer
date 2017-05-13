/*
 * Process Hacker -
 *   Custom Bootstrap Tool
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

#include <ph.h>
#include <userenv.h>

static ULONG64 PhMitigationPolicy =
    //PROCESS_CREATION_MITIGATION_POLICY_FORCE_RELOCATE_IMAGES_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON |
    //PROCESS_CREATION_MITIGATION_POLICY_STRICT_HANDLE_CHECKS_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_FONT_DISABLE_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON |
    PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON;

PPH_STRING GetProcessHackerInstallPath(VOID)
{
    static PH_STRINGREF UninstallKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ProcessHacker");
    HANDLE keyHandle;
    PPH_STRING installPath = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &UninstallKeyName,
        0
        )))
    {
        installPath = PhQueryRegistryString(keyHandle, L"InstallLocation");
        NtClose(keyHandle);
    }

    return installPath;
}

PPH_STRING GetProcessHackerPath(VOID)
{
    PPH_STRING installPath;
    PPH_STRING path = NULL;

    installPath = GetProcessHackerInstallPath();

    if (!PhIsNullOrEmptyString(installPath))
    {
        path = PhConcatStrings2(PhGetString(installPath), L"\\ProcessHacker.exe");
    }

    PhClearReference(&installPath);

    return path;
}

BOOLEAN CheckProcessHackerInstalled(VOID)
{
    BOOLEAN installed = FALSE;
    PPH_STRING installPath;
    PPH_STRING path;

    installPath = GetProcessHackerInstallPath();

    if (!PhIsNullOrEmptyString(installPath))
    {
        path = PhConcatStrings2(PhGetString(installPath), L"\\ProcessHacker.exe");
        installed = GetFileAttributes(path->Buffer) != INVALID_FILE_ATTRIBUTES;
        PhClearReference(&path);
    }

    PhClearReference(&installPath);
    
    return installed;
}

INT WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ PWSTR lpCmdLine,
    _In_ INT nCmdShow
    )
{
    SIZE_T attributeListLength = 0;
    PROCESS_INFORMATION processInfo = { 0 };
    STARTUPINFOEX info = { sizeof(STARTUPINFOEX) };
    PPH_STRING fileName;
    PPH_STRING currentDirectory;

    if (!NT_SUCCESS(PhInitializePhLibEx(0, 0, 0)))
        return 1;

    if (!InitializeProcThreadAttributeList(NULL, 1, 0, &attributeListLength) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return 1;

    info.lpAttributeList = PhAllocate(attributeListLength);
   
    if (!InitializeProcThreadAttributeList(info.lpAttributeList, 1, 0, &attributeListLength))
        return 1;

    if (!UpdateProcThreadAttribute(info.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY, &PhMitigationPolicy, sizeof(ULONG64), NULL, NULL))
        return 1;

    currentDirectory = PhGetApplicationDirectory();
    fileName = PhConcatStrings2(currentDirectory->Buffer, L"\\ProcessHacker.exe");
    PhMoveReference(&fileName, PhGetFullPath(fileName->Buffer, NULL));

    if (!RtlDoesFileExists_U(fileName->Buffer) && CheckProcessHackerInstalled())
        PhMoveReference(&fileName, GetProcessHackerPath());

    CreateProcess(
        NULL,
        fileName->Buffer,
        NULL,
        NULL,
        FALSE,
        EXTENDED_STARTUPINFO_PRESENT,
        NULL,
        NULL,
        &info.StartupInfo,
        &processInfo
        );

    PhDereferenceObject(fileName);

    if (processInfo.hProcess)
        NtClose(processInfo.hProcess);

    if (processInfo.hThread)
        NtClose(processInfo.hThread);

    DeleteProcThreadAttributeList(info.lpAttributeList);

    return 0;
}
