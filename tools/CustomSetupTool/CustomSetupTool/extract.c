/*
 * Process Hacker Toolchain -
 *   project setup
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

 //#define DEBUG_EXTRACT
#include <setup.h>
#include <appsup.h>
#include "miniz\miniz.h"

SETUP_REMOVE_FILE SetupRemoveFiles[] =
{
    { L"plugins\\NetAdapters.dll" },
};

SETUP_EXTRACT_FILE SetupFiles_X32[] =
{
    { "CHANGELOG.txt",                          L"CHANGELOG.txt" },
    { "COPYRIGHT.txt",                          L"COPYRIGHT.txt" },
    { "LICENSE.txt",                            L"LICENSE.txt" },
    { "README.txt",                             L"README.txt" },
    { "x32/peview.exe",                         L"peview.exe" },
    { "x32/ProcessHacker.exe",                  L"ProcessHacker.exe" },
    { "x32/ProcessHacker.sig",                  L"ProcessHacker.sig" },
    { "x32/kprocesshacker.sys",                 L"kprocesshacker.sys" },
    { "x32/plugins/CommonUtil.dll",             L"plugins\\CommonUtil.dll" },
    { "x32/plugins/DotNetTools.dll",            L"plugins\\DotNetTools.dll" },
    { "x32/plugins/ExtendedNotifications.dll",  L"plugins\\ExtendedNotifications.dll" },
    { "x32/plugins/ExtendedServices.dll",       L"plugins\\ExtendedServices.dll" },
    { "x32/plugins/ExtendedTools.dll",          L"plugins\\ExtendedTools.dll" },
    { "x32/plugins/ExtraPlugins.dll",           L"plugins\\ExtraPlugins.dll" },
    { "x32/plugins/HardwareDevices.dll",        L"plugins\\HardwareDevices.dll" },
    { "x32/plugins/NetworkTools.dll",           L"plugins\\NetworkTools.dll" },
    { "x32/plugins/OnlineChecks.dll",           L"plugins\\OnlineChecks.dll" },
    { "x32/plugins/SbieSupport.dll",            L"plugins\\SbieSupport.dll" },
    { "x32/plugins/ToolStatus.dll",             L"plugins\\ToolStatus.dll" },
    { "x32/plugins/Updater.dll",                L"plugins\\Updater.dll" },
    { "x32/plugins/UserNotes.dll",              L"plugins\\UserNotes.dll" },
    { "x32/plugins/WindowExplorer.dll",         L"plugins\\WindowExplorer.dll" },
};

SETUP_EXTRACT_FILE SetupFiles_X64[] =
{
    { "CHANGELOG.txt",                          L"CHANGELOG.txt" },
    { "COPYRIGHT.txt",                          L"COPYRIGHT.txt" },
    { "LICENSE.txt",                            L"LICENSE.txt" },
    { "README.txt",                             L"README.txt" },
    { "x64/peview.exe",                         L"peview.exe" },
    { "x64/ProcessHacker.exe",                  L"ProcessHacker.exe" },
    { "x64/ProcessHacker.sig",                  L"ProcessHacker.sig" },
    { "x64/kprocesshacker.sys",                 L"kprocesshacker.sys" },
    { "x64/plugins/CommonUtil.dll",             L"plugins\\CommonUtil.dll" },
    { "x64/plugins/DotNetTools.dll",            L"plugins\\DotNetTools.dll" },
    { "x64/plugins/ExtendedNotifications.dll",  L"plugins\\ExtendedNotifications.dll" },
    { "x64/plugins/ExtendedServices.dll",       L"plugins\\ExtendedServices.dll" },
    { "x64/plugins/ExtendedTools.dll",          L"plugins\\ExtendedTools.dll" },
    { "x64/plugins/ExtraPlugins.dll",           L"plugins\\ExtraPlugins.dll" },
    { "x64/plugins/HardwareDevices.dll",        L"plugins\\HardwareDevices.dll" },
    { "x64/plugins/NetworkTools.dll",           L"plugins\\NetworkTools.dll" },
    { "x64/plugins/OnlineChecks.dll",           L"plugins\\OnlineChecks.dll" },
    { "x64/plugins/SbieSupport.dll",            L"plugins\\SbieSupport.dll" },
    { "x64/plugins/ToolStatus.dll",             L"plugins\\ToolStatus.dll" },
    { "x64/plugins/Updater.dll",                L"plugins\\Updater.dll" },
    { "x64/plugins/UserNotes.dll",              L"plugins\\UserNotes.dll" },
    { "x64/plugins/WindowExplorer.dll",         L"plugins\\WindowExplorer.dll" },

    { "x32/ProcessHacker.exe",                  L"x86\\ProcessHacker.exe" },
    { "x32/plugins/DotNetTools.dll",            L"x86\\plugins\\DotNetTools.dll" },
};

PVOID GetZipResourceData(
    _In_ PULONG resourceLength
    )
{
    HRSRC resourceHandle = NULL;
    HGLOBAL resourceData;
    PVOID resourceBuffer = NULL;

    if (!(resourceHandle = FindResource(PhLibImageBase, MAKEINTRESOURCE(IDR_BIN_DATA), RT_RCDATA)))
        goto CleanupExit;

    *resourceLength = SizeofResource(PhLibImageBase, resourceHandle);

    if (!(resourceData = LoadResource(PhLibImageBase, resourceHandle)))
        goto CleanupExit;

    if (!(resourceBuffer = LockResource(resourceData)))
        goto CleanupExit;

CleanupExit:

    if (resourceHandle)
        FreeResource(resourceHandle);

    return resourceBuffer;
}

PVOID GetExtractData(
    _In_ USHORT ProcessorArchitecture
    )
{
    if (ProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        return SetupFiles_X64;
    else
        return SetupFiles_X32;
}

ULONG GetExtractDataLength(
    _In_ USHORT ProcessorArchitecture
    )
{    
    if (ProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        return ARRAYSIZE(SetupFiles_X64);
    else
        return ARRAYSIZE(SetupFiles_X32);
}

BOOLEAN SetupExtractBuild(
    _In_ PVOID Arguments
    )
{
    mz_bool status = MZ_FALSE;
    ULONG64 totalLength = 0;
    ULONG64 currentLength = 0;
    mz_zip_archive zip_archive = { 0 };
    ULONG extractDataLength;
    PSETUP_EXTRACT_FILE extractData;
    PVOID resourceBuffer;
    ULONG resourceLength;
    SYSTEM_INFO info;

    GetNativeSystemInfo(&info);

    if (!(resourceBuffer = GetZipResourceData(&resourceLength)))
        goto CleanupExit;

    if (!(status = mz_zip_reader_init_mem(&zip_archive, resourceBuffer, resourceLength, 0)))
        goto CleanupExit;

    extractData = GetExtractData(info.wProcessorArchitecture);
    extractDataLength = GetExtractDataLength(info.wProcessorArchitecture);

    for (ULONG i = 0; i < ARRAYSIZE(SetupRemoveFiles); i++)
    {
        SetupDeleteDirectoryFile(SetupRemoveFiles[i].FileName);
    }

    if (!CreateDirectoryPath(PhGetString(SetupInstallPath)))
        goto CleanupExit;

    for (ULONG i = 0; i < extractDataLength; i++)
    {
        mz_uint zipFileIndex;
        mz_zip_archive_file_stat zipFileStat;

        if ((zipFileIndex = mz_zip_reader_locate_file(
            &zip_archive,
            extractData[i].FileName,
            NULL,
            0
            )) == -1)
        {
            goto CleanupExit;
        }

        if (!mz_zip_reader_file_stat(&zip_archive, zipFileIndex, &zipFileStat))
            goto CleanupExit;

        totalLength += zipFileStat.m_uncomp_size;
        InterlockedExchange64(&ExtractTotalLength, totalLength);
    }

    SendMessage(Arguments, WM_START_SETUP, 0, 0);

#ifdef DEBUG_EXTRACT
    Sleep(1000);
#endif

    for (ULONG i = 0; i < extractDataLength; i++)
    {
        IO_STATUS_BLOCK isb;
        HANDLE fileHandle;
        ULONG indexOfFileName = -1;
        PPH_STRING fullSetupPath;
        PPH_STRING extractPath;
        PVOID buffer;
        mz_ulong zipFileCrc32 = MZ_CRC32_INIT;
        mz_uint zipFileIndex;
        mz_zip_archive_file_stat zipFileStat;
        ULONG bufferLength = 0;

        if ((zipFileIndex = mz_zip_reader_locate_file(
            &zip_archive,
            extractData[i].FileName,
            NULL,
            0
            )) == -1)
        {
            goto CleanupExit;
        }

        if (!mz_zip_reader_file_stat(&zip_archive, zipFileIndex, &zipFileStat))
            goto CleanupExit;

        if (!(buffer = mz_zip_reader_extract_to_heap(
            &zip_archive,
            zipFileIndex,
            &bufferLength,
            0
            )))
        {
            goto CleanupExit;
        }

        if ((zipFileCrc32 = mz_crc32(zipFileCrc32, buffer, bufferLength)) != zipFileStat.m_crc32)
            goto CleanupExit;

        extractPath = PhConcatStrings(
            2, 
            PhGetString(SetupInstallPath), 
            extractData[i].ExtractFileName
            );

        // Create the directory if it does not exist.
        if (fullSetupPath = PhGetFullPath(extractPath->Buffer, &indexOfFileName))
        {
            PPH_STRING directoryPath;

            if (indexOfFileName == -1)
                goto CleanupExit;

            if (directoryPath = PhSubstring(fullSetupPath, 0, indexOfFileName))
            {
                if (!CreateDirectoryPath(PhGetString(directoryPath)))
                {
                    PhDereferenceObject(directoryPath);
                    PhDereferenceObject(fullSetupPath);
                    goto CleanupExit;
                }

                PhDereferenceObject(directoryPath);
            }

            PhDereferenceObject(fullSetupPath);
        }

        // TODO: Rename existing folder items and restore if the setup fails.

        if (!NT_SUCCESS(PhCreateFileWin32(
            &fileHandle,
            PhGetString(extractPath),
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            goto CleanupExit;
        }

        if (!NT_SUCCESS(NtWriteFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            bufferLength,
            NULL,
            NULL
            )))
        {
            goto CleanupExit;
        }

        if (isb.Information != bufferLength)
            goto CleanupExit;

        currentLength += bufferLength;
        InterlockedExchange64(&ExtractCurrentLength, currentLength);

        SendMessage(Arguments, WM_UPDATE_SETUP, 0, (LPARAM)extractData[i].ExtractFileName);

        NtClose(fileHandle);
        mz_free(buffer);

#ifdef DEBUG_EXTRACT
        Sleep(200);
#endif
    }

CleanupExit:
    mz_zip_reader_end(&zip_archive);

#ifdef DEBUG_EXTRACT
    Sleep(1000);
#endif

    return TRUE;
}

