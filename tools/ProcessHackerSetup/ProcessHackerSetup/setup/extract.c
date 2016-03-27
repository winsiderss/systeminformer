#include <setup.h>
#include <appsup.h>
#include "..\lib\miniz\miniz.h"

struct
{
    PSTR FileName;
    PWSTR ExtractFileName;
} SetupFiles_X64[] =
{
    { "CHANGELOG.txt",                          L"CHANGELOG.txt" },
    { "COPYRIGHT.txt",                          L"COPYRIGHT.txt" },
    { "LICENSE.txt",                            L"LICENSE.txt" },
    { "README.txt",                             L"README.txt" },

    { "x64/peview.exe",                         L"peview.exe" },
    { "x64/ProcessHacker.exe",                  L"ProcessHacker.exe" },
    { "x64/kprocesshacker.sys",                 L"kprocesshacker.sys" },

    { "x64/plugins/DotNetTools.dll",            L"plugins\\DotNetTools.dll" },
    { "x64/plugins/ExtendedNotifications.dll",  L"plugins\\ExtendedNotifications.dll" },
    { "x64/plugins/ExtendedServices.dll",       L"plugins\\ExtendedServices.dll" },
    { "x64/plugins/ExtendedTools.dll",          L"plugins\\ExtendedTools.dll" },
    { "x64/plugins/HardwareDevices.dll",        L"plugins\\HardwareDevices.dll" },
    { "x64/plugins/NetworkTools.dll",           L"plugins\\NetworkTools.dll" },
    { "x64/plugins/OnlineChecks.dll",           L"plugins\\OnlineChecks.dll" },
    { "x64/plugins/SbieSupport.dll",            L"plugins\\SbieSupport.dll" },
    { "x64/plugins/ToolStatus.dll",             L"plugins\\ToolStatus.dll" },
    { "x64/plugins/Updater.dll",                L"plugins\\Updater.dll" },
    { "x64/plugins/UserNotes.dll",              L"plugins\\UserNotes.dll" },
    { "x64/plugins/WindowExplorer.dll",         L"plugins\\WindowExplorer.dll" },

    { "x86/ProcessHacker.exe",                  L"x86\\ProcessHacker.exe" },
    { "x86/plugins/DotNetTools.dll",            L"x86\\plugins\\DotNetTools.dll" },
};


BOOLEAN SetupExtractBuild(
    _In_ PVOID Arguments
    )
{
    BOOLEAN isInstallSuccess = FALSE;
    mz_uint64 totalLength = 0;
    mz_uint64 total_size = 0;
    mz_uint64 downloadedBytes = 0;

    PPH_STRING totalSizeStr = NULL;

    mz_bool status = MZ_FALSE;
    mz_zip_archive zip_archive;

    memset(&zip_archive, 0, sizeof(zip_archive));

    STATUS_MSG(L"Extracting update %s...", Version);
    Sleep(1000);

    StartProgress();

    __try
    {
        if (!CreateDirectoryPath(SetupInstallPath))
        {
            __leave;
        }

        // TODO: Move existing folder items.

        if (!(status = mz_zip_reader_init_file(&zip_archive, "processhacker-2.38-bin.zip", 0)))
        {
            __leave;
        }

        // Get filesize information for extract progress
        for (mz_uint i = 0; i < ARRAYSIZE(SetupFiles_X64); i++)
        {
            mz_uint index = mz_zip_reader_locate_file(
                &zip_archive,
                SetupFiles_X64[i].FileName,
                NULL,
                0
                );

            mz_zip_archive_file_stat file_stat;

            if (!mz_zip_reader_file_stat(&zip_archive, index, &file_stat))
            {
                //mz_zip_reader_end(&zip_archive);
                //break;
            }

            total_size += file_stat.m_uncomp_size;

            DEBUG_MSG(L"Filename: \"%hs\", Uncompressed size: %I64u, Compressed size: %I64u\r\n",
                file_stat.m_filename,
                file_stat.m_uncomp_size,
                file_stat.m_comp_size
                );
        }

        totalSizeStr = PhFormatSize(total_size, -1);
        DEBUG_MSG(L"Opened Zip: %hs (%s)\r\n", "processhacker-2.38-bin.zip", totalSizeStr->Buffer);

        for (ULONG i = 0; i < ARRAYSIZE(SetupFiles_X64); i++)
        {
            mz_ulong file_crc32 = MZ_CRC32_INIT;
            IO_STATUS_BLOCK isb;
            HANDLE fileHandle;
            ULONG indexOfFileName = -1;
            PPH_STRING fullSetupPath;
            PPH_STRING extractPath;
            mz_uint file_index;
            mz_zip_archive_file_stat file_stat;
            size_t bufferLength = 0;

            extractPath = PhConcatStrings(2,
                SetupInstallPath->Buffer,
                SetupFiles_X64[i].ExtractFileName
                );

            // Create the directory if it does not exist.
            if (fullSetupPath = PhGetFullPath(extractPath->Buffer, &indexOfFileName))
            {
                PPH_STRING directoryPath;

                if (indexOfFileName == -1)
                    __leave;

                if (directoryPath = PhSubstring(fullSetupPath, 0, indexOfFileName))
                {
                    if (!CreateDirectoryPath(directoryPath))
                    {
                        PhDereferenceObject(directoryPath);
                        PhDereferenceObject(fullSetupPath);
                        __leave;
                    }

                    PhDereferenceObject(directoryPath);
                }

                PhDereferenceObject(fullSetupPath);
            }

            STATUS_MSG(L"Extracting: %s\r\n", SetupFiles_X64[i].ExtractFileName);
            Sleep(100);

            // Create output file
            if (!NT_SUCCESS(PhCreateFileWin32(
                &fileHandle,
                extractPath->Buffer,
                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OVERWRITE_IF,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                )))
            {
                DEBUG_MSG(L"PhCreateFileWin32 Failed\r\n");
                __leave;
            }

            file_index = mz_zip_reader_locate_file(
                &zip_archive,
                SetupFiles_X64[i].FileName,
                NULL,
                0
                );

            if (file_index == -1)
            {
                __leave;
            }

            if (!mz_zip_reader_file_stat(&zip_archive, file_index, &file_stat))
            {
                __leave;
            }

            PVOID buffer = mz_zip_reader_extract_to_heap(
                &zip_archive,
                file_index,
                &bufferLength,
                0
                );

            // Update the hash
            file_crc32 = mz_crc32(file_crc32, buffer, bufferLength);

            // Write the downloaded bytes to disk.
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
                __leave;
            }

            if (isb.Information != bufferLength && file_crc32 != file_stat.m_crc32)
            {
                // Corrupted file.
                __leave;
            }

            totalLength += (mz_uint64)bufferLength;
            SetProgress((LONG)totalLength, (LONG)total_size);

            NtClose(fileHandle);
            mz_free(buffer);
        }

        isInstallSuccess = TRUE;
    }
    __finally
    {
        // Close the archive
        mz_zip_reader_end(&zip_archive);
    }

    Sleep(3000);

    return TRUE;
}