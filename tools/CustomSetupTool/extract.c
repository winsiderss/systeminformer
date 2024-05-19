/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

#include "setup.h"
#include "..\thirdparty\miniz\miniz.h"

BOOLEAN SetupUseExistingKsi(
    _In_ PPH_STRING FileName,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    BOOLEAN result;
    PPH_STRING oldFile;
    PH_HASH_CONTEXT hashContext;
    BYTE inputHash[256 / 8];
    BYTE oldHash[256/ 8];

    oldFile = PhConcatStringRefZ(&FileName->sr, L"-old");
    if (!PhDoesFileExistWin32(PhGetString(oldFile)))
    {
        result = FALSE;
        goto CleanupExit;
    }

    PhInitializeHash(&hashContext, Sha256HashAlgorithm);
    PhUpdateHash(&hashContext, Buffer, BufferLength);
    if (!PhFinalHash(&hashContext, inputHash, ARRAYSIZE(inputHash), NULL))
    {
        result = FALSE;
        goto CleanupExit;
    }

    if (!SetupHashFile(oldFile, oldHash))
    {
        result = FALSE;
        goto CleanupExit;
    }

    if (!RtlEqualMemory(inputHash, oldHash, ARRAYSIZE(inputHash)))
    {
        result = FALSE;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(PhMoveFileWin32(PhGetString(oldFile), PhGetString(FileName), TRUE)))
    {
        result = FALSE;
        goto CleanupExit;
    }

    result = TRUE;

CleanupExit:

    PhClearReference(&oldFile);

    return result;
}

BOOLEAN SetupUpdateKsi(
    _Inout_ PPH_SETUP_CONTEXT Context,
    _In_ PPH_STRING FileName,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    BOOLEAN result;
    PH_HASH_CONTEXT hashContext;
    BYTE inputHash[256 / 8];
    BYTE existingHash[256/ 8];
    PPH_STRING oldFile = NULL;

    PhInitializeHash(&hashContext, Sha256HashAlgorithm);
    PhUpdateHash(&hashContext, Buffer, BufferLength);
    if (!PhFinalHash(&hashContext, inputHash, ARRAYSIZE(inputHash), NULL))
    {
        result = FALSE;
        goto CleanupExit;
    }

    if (!SetupHashFile(FileName, existingHash))
    {
        result = FALSE;
        goto CleanupExit;
    }

    if (RtlEqualMemory(inputHash, existingHash, ARRAYSIZE(inputHash)))
    {
        result = TRUE;
        goto CleanupExit;
    }

    Context->NeedsReboot = TRUE;

    oldFile = PhConcatStringRefZ(&FileName->sr, L"-old");

    PhDeleteFileWin32(PhGetString(oldFile));
    if (!NT_SUCCESS(PhMoveFileWin32(PhGetString(FileName), PhGetString(oldFile), TRUE)))
    {
        result = FALSE;
        goto CleanupExit;
    }

    MoveFileExW(PhGetString(oldFile), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

    if (!SetupOverwriteFile(FileName, Buffer, BufferLength))
    {
        result = FALSE;
        goto CleanupExit;
    }

    result = TRUE;

CleanupExit:

    PhClearReference(&oldFile);

    return result;
}

USHORT SetupGetCurrentArchitecture(
    VOID
    )
{
    static BOOL (WINAPI* IsWow64Process2_I)(
        _In_ HANDLE ProcessHandle,
        _Out_ PUSHORT ProcessMachine,
        _Out_opt_ PUSHORT NativeMachine
        ) = NULL;
    USHORT processMachine;
    USHORT nativeMachine;
    SYSTEM_INFO info;

    if (!IsWow64Process2_I)
    {
        IsWow64Process2_I = PhGetModuleProcAddress(L"kernel32.dll", "IsWow64Process2");
    }

    if (IsWow64Process2_I && IsWow64Process2_I(NtCurrentProcess(), &processMachine, &nativeMachine))
    {
        switch (nativeMachine)
        {
        case IMAGE_FILE_MACHINE_I386:
            return PROCESSOR_ARCHITECTURE_INTEL;
        case IMAGE_FILE_MACHINE_AMD64:
            return PROCESSOR_ARCHITECTURE_AMD64;
        case IMAGE_FILE_MACHINE_ARM64:
            return PROCESSOR_ARCHITECTURE_ARM64;
        }
    }

    GetNativeSystemInfo(&info);

    return info.wProcessorArchitecture;
}

BOOLEAN SetupExtractBuild(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    mz_bool status = MZ_FALSE;
    ULONG64 totalLength = 0;
    ULONG64 currentLength = 0;
    mz_zip_archive zipFileArchive = { 0 };
    PPH_STRING extractPath = NULL;
    USHORT nativeArchitecture;

    nativeArchitecture = SetupGetCurrentArchitecture();

#ifdef PH_BUILD_API
    ULONG resourceLength;
    PVOID resourceBuffer = NULL;

    if (!PhLoadResource(PhInstanceHandle, MAKEINTRESOURCE(IDR_BIN_DATA), RT_RCDATA, &resourceLength, &resourceBuffer))
        return FALSE;

    if (!(status = mz_zip_reader_init_mem(&zipFileArchive, resourceBuffer, resourceLength, 0)))
    {
        Context->ErrorCode = ERROR_PATH_NOT_FOUND;
        goto CleanupExit;
    }
#else
    if (!(status = mz_zip_reader_init_mem(&zipFileArchive, Context->ZipDownloadBuffer, Context->ZipDownloadBufferLength, 0)))
    {
        Context->ErrorCode = ERROR_PATH_NOT_FOUND;
        goto CleanupExit;
    }
#endif

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zipFileArchive); i++)
    {
        mz_zip_archive_file_stat zipFileStat;
        PPH_STRING fileName;

        if (!mz_zip_reader_file_stat(&zipFileArchive, i, &zipFileStat))
            continue;

        fileName = PhConvertUtf8ToUtf16(zipFileStat.m_filename);

        if (PhFindStringInString(fileName, 0, L"SystemInformer.exe.settings.xml") != SIZE_MAX)
            continue;
        if (PhFindStringInString(fileName, 0, L"usernotesdb.xml") != SIZE_MAX)
            continue;

        if (nativeArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            if (PhStartsWithString2(fileName, L"i386\\", TRUE) ||
                PhStartsWithString2(fileName, L"arm64\\", TRUE))
                continue;
        }
        else if (nativeArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
        {
            if (PhStartsWithString2(fileName, L"i386\\", TRUE) ||
                PhStartsWithString2(fileName, L"amd64\\", TRUE))
                continue;
        }
        else
        {
            if (PhStartsWithString2(fileName, L"amd64\\", TRUE) ||
                PhStartsWithString2(fileName, L"arm64\\", TRUE))
                continue;
        }

        totalLength += zipFileStat.m_uncomp_size;
    }

    SendMessage(Context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(&zipFileArchive); i++)
    {
        PVOID buffer = NULL;
        size_t zipFileBufferLength = 0;
        PPH_STRING fileName = NULL;
        mz_ulong zipFileCrc32 = 0;
        mz_zip_archive_file_stat zipFileStat;

        if (!mz_zip_reader_file_stat(&zipFileArchive, i, &zipFileStat))
            continue;

        fileName = PhConvertUtf8ToUtf16(zipFileStat.m_filename);

        if (PhFindStringInString(fileName, 0, L"SystemInformer.exe.settings.xml") != SIZE_MAX)
            continue;
        if (PhFindStringInString(fileName, 0, L"usernotesdb.xml") != SIZE_MAX)
            continue;

        if (nativeArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            if (PhStartsWithString2(fileName, L"i386\\", TRUE) ||
                PhStartsWithString2(fileName, L"arm64\\", TRUE))
                continue;

            if (PhStartsWithString2(fileName, L"amd64\\", TRUE))
                PhMoveReference(&fileName, PhSubstring(fileName, 6, (fileName->Length / sizeof(WCHAR)) - 6));
        }
        else if (nativeArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
        {
            if (PhStartsWithString2(fileName, L"i386\\", TRUE) ||
                PhStartsWithString2(fileName, L"amd64\\", TRUE))
                continue;

            if (PhStartsWithString2(fileName, L"arm64\\", TRUE))
                PhMoveReference(&fileName, PhSubstring(fileName, 6, (fileName->Length / sizeof(WCHAR)) - 6));
        }
        else
        {
            if (PhStartsWithString2(fileName, L"amd64\\", TRUE) ||
                PhStartsWithString2(fileName, L"arm64\\", TRUE))
                continue;

            if (PhStartsWithString2(fileName, L"i386\\", TRUE))
                PhMoveReference(&fileName, PhSubstring(fileName, 5, (fileName->Length / sizeof(WCHAR)) - 5));
        }

        if (!(buffer = mz_zip_reader_extract_to_heap(&zipFileArchive, zipFileStat.m_file_index, &zipFileBufferLength, 0)))
        {
            Context->ErrorCode = ERROR_FILE_INVALID;
            goto CleanupExit;
        }

        if ((zipFileCrc32 = mz_crc32(zipFileCrc32, buffer, zipFileBufferLength)) != zipFileStat.m_crc32)
        {
            Context->ErrorCode = ERROR_CRC;
            goto CleanupExit;
        }

        extractPath = PhConcatStringRef3(
            &Context->SetupInstallPath->sr,
            &PhNtPathSeperatorString,
            &fileName->sr
            );

        if (!NT_SUCCESS(PhCreateDirectoryFullPathWin32(&extractPath->sr)))
        {
            Context->ErrorCode = ERROR_DIRECTORY_NOT_SUPPORTED;
            goto CleanupExit;
        }

        // TODO: Backup file and restore if the setup fails.
        //{
        //    PPH_STRING backupFilePath;
        //
        //    backupFilePath = PhConcatStrings(
        //        4,
        //        PhGetString(Context->SetupInstallPath),
        //        L"\\",
        //        PhGetString(fileName),
        //        L".bak"
        //        );
        //
        //    MoveFile(extractPath->Buffer, backupFilePath->Buffer);
        //
        //    PhDereferenceObject(backupFilePath);
        //}

        {
            ULONG attempts = 5;
            BOOLEAN updateKsiAttempt;

            do
            {
                updateKsiAttempt = FALSE;

                if (PhEndsWithString2(extractPath, L"\\ksi.dll", FALSE))
                {
                    if (SetupUseExistingKsi(extractPath, buffer, zipFileBufferLength))
                        break;
                    if (SetupOverwriteFile(extractPath, buffer, zipFileBufferLength))
                        break;
                    if (SetupUpdateKsi(Context, extractPath, buffer, zipFileBufferLength))
                        break;

                    updateKsiAttempt = TRUE;
                }
                else
                {
                    if (SetupOverwriteFile(extractPath, buffer, zipFileBufferLength))
                        break;
                }

                PhDelayExecution(1000); // Wait 1 second and try again

            } while (--attempts);

            if (updateKsiAttempt)
            {
                Context->ErrorCode = ERROR_PATH_BUSY;
                goto CleanupExit;
            }
        }

        currentLength += zipFileBufferLength;

        {
            ULONG64 percent = 100 * currentLength / totalLength;
            PH_FORMAT format[7];
            WCHAR string[MAX_PATH];
            PPH_STRING baseName = PhGetBaseName(extractPath);

            PhInitFormatS(&format[0], L"Extracting: ");
            PhInitFormatS(&format[1], PhGetStringOrEmpty(baseName));

            if (PhFormatToBuffer(format, 2, string, sizeof(string), NULL))
            {
                SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)string);
            }

            PhInitFormatS(&format[0], L"Progress: ");
            PhInitFormatSize(&format[1], currentLength);
            PhInitFormatS(&format[2], L" of ");
            PhInitFormatSize(&format[3], totalLength);
            PhInitFormatS(&format[4], L" (");
            PhInitFormatI64U(&format[5], percent);
            PhInitFormatS(&format[6], L"%)");

            if (PhFormatToBuffer(format, ARRAYSIZE(format), string, sizeof(string), NULL))
            {
                SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)string);
            }

            SendMessage(Context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)(INT)percent, 0);

            if (baseName)
                PhDereferenceObject(baseName);

#ifdef FORCE_TEST_UPDATE_LOCAL_INSTALL
            PhDelayExecution(100);
#endif
        }

        mz_free(buffer);
    }

    mz_zip_reader_end(&zipFileArchive);

    if (extractPath)
        PhDereferenceObject(extractPath);

    return TRUE;

CleanupExit:

    mz_zip_reader_end(&zipFileArchive);

    if (extractPath)
        PhDereferenceObject(extractPath);

    return FALSE;
}
