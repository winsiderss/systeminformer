/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2023
 *
 */

#include "setup.h"
#include "..\thirdparty\miniz\miniz.h"

VOID SetupSetProgressMarquee(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ BOOLEAN Enable
    )
{
    HWND progressHandle;

    if (!Context->DialogHandle)
        return;

    if (progressHandle = GetDlgItem(Context->DialogHandle, IDC_PROGRESS))
    {
        SendMessage(progressHandle, PBM_SETMARQUEE, Enable, 0);
    }
    else
    {
        SendMessage(Context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, Enable, 0);

        if (Enable)
            SendMessage(Context->DialogHandle, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);
    }
}

VOID SetupSetProgressText(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_opt_ PCWSTR MainInstruction,
    _In_opt_ PCWSTR Content
    )
{
    HWND statusHandle;

    if (!Context->DialogHandle)
        return;

    if (statusHandle = GetDlgItem(Context->DialogHandle, IDC_STATUS))
    {
        if (MainInstruction)
            SetWindowText(statusHandle, MainInstruction);
    }
    else
    {
        if (MainInstruction)
            SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_MAIN_INSTRUCTION, (LPARAM)MainInstruction);
        if (Content)
            SendMessage(Context->DialogHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)Content);
    }
}

VOID SetupSetProgressValue(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ ULONG Value
    )
{
    HWND progressHandle;

    if (!Context->DialogHandle)
        return;

    if (progressHandle = GetDlgItem(Context->DialogHandle, IDC_PROGRESS))
    {
        SendMessage(progressHandle, PBM_SETMARQUEE, FALSE, 0);
        SendMessage(progressHandle, PBM_SETPOS, Value, 0);
    }
    else
    {
        SendMessage(Context->DialogHandle, TDM_SET_MARQUEE_PROGRESS_BAR, FALSE, 0);
        SendMessage(Context->DialogHandle, TDM_SET_PROGRESS_BAR_POS, (WPARAM)Value, 0);
    }
}

/**
 * Checks if the existing KSystem Informer driver file matches the specified buffer.
 *
 * \param FileName The file name.
 * \param Buffer The buffer to compare.
 * \param BufferLength The length of the buffer.
 * \return Successful or errant status.
 */
NTSTATUS SetupUseExistingKsi(
    _In_ PPH_STRING FileName,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;
    LARGE_INTEGER fileSize;
    PVOID fileBuffer;
    ULONG fileLength;

    if (!PhDoesFileExistWin32(PhGetString(FileName)))
        return STATUS_NO_SUCH_FILE;

    status = PhCreateFileWin32Ex(
        &fileHandle,
        PhGetString(FileName),
        FILE_GENERIC_READ,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)))
    {
        if (fileSize.QuadPart > ULONG_MAX)
        {
            status = STATUS_FILE_TOO_LARGE;
            goto CleanupExit;
        }

        fileLength = fileSize.LowPart;

        if (fileLength == BufferLength)
        {
            fileBuffer = PhAllocate(fileLength);

            if (NT_SUCCESS(status = NtReadFile(fileHandle, NULL, NULL, NULL, &isb, fileBuffer, fileLength, NULL, NULL)))
            {
                if (isb.Information != fileLength)
                    status = STATUS_UNSUCCESSFUL;
            }

            if (NT_SUCCESS(status))
            {
                if (RtlEqualMemory(fileBuffer, Buffer, fileLength))
                    status = STATUS_SUCCESS;
                else
                    status = STATUS_UNSUCCESSFUL;
            }

            PhFree(fileBuffer);
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }
    }

CleanupExit:

    NtClose(fileHandle);

    return status;
}

/**
 * Updates the KSystem Informer driver file.
 *
 * \param Context The setup context.
 * \param FileName The file name.
 * \param Buffer The buffer containing the file data.
 * \param BufferLength The length of the buffer.
 * \return Successful or errant status.
 */
NTSTATUS SetupUpdateKsi(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ PPH_STRING FileName,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    NTSTATUS status;
    PPH_STRING oldFileName;

    oldFileName = PhConcatStrings(2, PhGetString(FileName), L".old");

    if (PhDoesFileExistWin32(PhGetString(oldFileName)))
    {
        if (!DeleteFile(PhGetString(oldFileName)))
        {
            status = PhGetLastWin32ErrorAsNtStatus();
            PhDereferenceObject(oldFileName);
            return status;
        }
    }

    if (MoveFile(PhGetString(FileName), PhGetString(oldFileName)))
    {
        status = SetupOverwriteFile(FileName, Buffer, BufferLength);

        if (NT_SUCCESS(status))
        {
            Context->NeedsReboot = TRUE;
        }
        else
        {
            MoveFile(PhGetString(oldFileName), PhGetString(FileName));
        }
    }
    else
    {
        status = PhGetLastWin32ErrorAsNtStatus();
    }

    PhDereferenceObject(oldFileName);

    return status;
}

/**
 * Gets the native processor architecture.
 *
 * \return The processor architecture.
 */
static USHORT SetupGetCurrentArchitecture(
    VOID
    )
{
    static typeof(&IsWow64Process2) IsWow64Process2_I = NULL;
    USHORT processMachine;
    USHORT nativeMachine;
    SYSTEM_INFO info;

    if (!IsWow64Process2_I)
        IsWow64Process2_I = PhGetModuleProcAddress(L"kernel32.dll", "IsWow64Process2");

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

/**
 * Gets the total size of files that will be extracted from the build archive.
 *
 * \param ZipFileArchive The ZIP archive.
 * \param NativeArchitecture The native processor architecture.
 * \param TotalLength Receives the total uncompressed length.
 * \return Successful or errant status.
 */
static NTSTATUS SetupExtractBuildGetTotalLength(
    _Inout_ mz_zip_archive* ZipFileArchive,
    _In_ USHORT NativeArchitecture,
    _Out_ PULONG64 TotalLength
    )
{
    *TotalLength = 0;

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(ZipFileArchive); i++)
    {
        mz_zip_archive_file_stat zipFileStat;
        PPH_STRING fileName;

        if (!mz_zip_reader_file_stat(ZipFileArchive, i, &zipFileStat))
            continue;

        fileName = PhConvertUtf8ToUtf16(zipFileStat.m_filename);

        if (PhFindStringInString(fileName, 0, L"SystemInformer.exe.settings.xml") != SIZE_MAX)
        {
            PhDereferenceObject(fileName);
            continue;
        }
        if (PhFindStringInString(fileName, 0, L"usernotesdb.xml") != SIZE_MAX)
        {
            PhDereferenceObject(fileName);
            continue;
        }

        if (NativeArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            if (PhStartsWithString2(fileName, L"i386", TRUE) ||
                PhStartsWithString2(fileName, L"arm64", TRUE))
            {
                PhDereferenceObject(fileName);
                continue;
            }
        }
        else if (NativeArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
        {
            if (PhStartsWithString2(fileName, L"i386", TRUE) ||
                PhStartsWithString2(fileName, L"amd64", TRUE))
            {
                PhDereferenceObject(fileName);
                continue;
            }
        }
        else
        {
            if (PhStartsWithString2(fileName, L"amd64", TRUE) ||
                PhStartsWithString2(fileName, L"arm64", TRUE))
            {
                PhDereferenceObject(fileName);
                continue;
            }
        }

        *TotalLength += zipFileStat.m_uncomp_size;
        PhDereferenceObject(fileName);
    }

    return STATUS_SUCCESS;
}

/**
 * Extracts files from the build archive and writes staged files.
 *
 * \param Context The setup context.
 * \param ZipFileArchive The ZIP archive.
 * \param NativeArchitecture The native processor architecture.
 * \param TotalLength The total uncompressed length.
 * \param StagedFiles The staged files list.
 * \return Successful or errant status.
 */
static NTSTATUS SetupExtractBuildStageFiles(
    _In_ PPH_SETUP_CONTEXT Context,
    _Inout_ mz_zip_archive* ZipFileArchive,
    _In_ USHORT NativeArchitecture,
    _In_ ULONG64 TotalLength,
    _Inout_ PPH_LIST StagedFiles
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG64 currentLength = 0;
    PPH_STRING extractPath = NULL;

    for (mz_uint i = 0; i < mz_zip_reader_get_num_files(ZipFileArchive); i++)
    {
        PVOID buffer = NULL;
        SIZE_T zipFileBufferLength = 0;
        PPH_STRING fileName = NULL;
        mz_ulong zipFileCrc32 = 0;
        mz_zip_archive_file_stat zipFileStat;
        PPH_STRING altPathName;

        if (!mz_zip_reader_file_stat(ZipFileArchive, i, &zipFileStat))
            continue;

        fileName = PhConvertUtf8ToUtf16(zipFileStat.m_filename);

        if (PhFindStringInString(fileName, 0, L"SystemInformer.exe.settings.xml") != SIZE_MAX)
        {
            PhDereferenceObject(fileName);
            continue;
        }
        if (PhFindStringInString(fileName, 0, L"usernotesdb.xml") != SIZE_MAX)
        {
            PhDereferenceObject(fileName);
            continue;
        }

        if (NativeArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
            if (PhStartsWithString2(fileName, L"i386", TRUE) ||
                PhStartsWithString2(fileName, L"arm64", TRUE))
            {
                PhDereferenceObject(fileName);
                continue;
            }

            if (PhStartsWithString2(fileName, L"amd64", TRUE))
            {
                PhMoveReference(&fileName, PhSubstring(fileName, 6, (fileName->Length / sizeof(WCHAR)) - 6));
            }
        }
        else if (NativeArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
        {
            if (PhStartsWithString2(fileName, L"i386", TRUE) ||
                PhStartsWithString2(fileName, L"amd64", TRUE))
            {
                PhDereferenceObject(fileName);
                continue;
            }

            if (PhStartsWithString2(fileName, L"arm64", TRUE))
            {
                PhMoveReference(&fileName, PhSubstring(fileName, 6, (fileName->Length / sizeof(WCHAR)) - 6));
            }
        }
        else
        {
            if (PhStartsWithString2(fileName, L"amd64", TRUE) ||
                PhStartsWithString2(fileName, L"arm64", TRUE))
            {
                PhDereferenceObject(fileName);
                continue;
            }

            if (PhStartsWithString2(fileName, L"i386", TRUE))
            {
                PhMoveReference(&fileName, PhSubstring(fileName, 5, (fileName->Length / sizeof(WCHAR)) - 5));
            }
        }

        if (!(buffer = mz_zip_reader_extract_to_heap(ZipFileArchive, zipFileStat.m_file_index, &zipFileBufferLength, 0)))
        {
            PhDereferenceObject(fileName);
            status = STATUS_NO_MEMORY;
            goto CleanupExit;
        }

        if ((zipFileCrc32 = mz_crc32(zipFileCrc32, buffer, (mz_uint)zipFileBufferLength)) != zipFileStat.m_crc32)
        {
            mz_free(buffer);
            PhDereferenceObject(fileName);
            status = STATUS_CRC_ERROR;
            goto CleanupExit;
        }

        {
            if (altPathName = PhConvertAltSeperatorToNtPathSeperator(fileName))
            {
                PhSwapReference(&fileName, altPathName);
            }

            PhClearReference(&extractPath);
            extractPath = SetupCreateFullPath(Context->SetupInstallPath, PhGetString(fileName));

            if (PhIsNullOrEmptyString(extractPath))
            {
                extractPath = PhConcatStringRef3(
                    &Context->SetupInstallPath->sr,
                    &PhNtPathSeparatorString,
                    &fileName->sr
                    );
            }

            PhDereferenceObject(fileName);
        }

        if (!NT_SUCCESS(status = PhCreateDirectoryFullPathWin32(&extractPath->sr)))
        {
            mz_free(buffer);
            goto CleanupExit;
        }

        if (PhEndsWithString2(extractPath, L"\\ksi.dll", FALSE))
        {
            ULONG attempts = 5;

            do
            {
                if (NT_SUCCESS(status = SetupUseExistingKsi(extractPath, buffer, zipFileBufferLength)))
                    break;
                if (NT_SUCCESS(status = SetupOverwriteFile(extractPath, buffer, zipFileBufferLength)))
                    break;
                if (NT_SUCCESS(status = SetupUpdateKsi(Context, extractPath, buffer, zipFileBufferLength)))
                    break;

                PhDelayExecution(1000);
            } while (--attempts);
        }
        else
        {
            if (NT_SUCCESS(status = SetupWriteFileAtomic(Context, extractPath, buffer, zipFileBufferLength)))
            {
                PhAddItemList(StagedFiles, PhReferenceObject(extractPath));
            }
        }

        mz_free(buffer);

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        currentLength += zipFileBufferLength;

        {
            ULONG64 percent = 50 * currentLength / TotalLength;
            PH_FORMAT format[7];
            WCHAR string[MAX_PATH];
            PPH_STRING baseName = PhGetBaseName(extractPath);

            PhInitFormatS(&format[0], L"Extracting: ");
            PhInitFormatS(&format[1], PhGetStringOrEmpty(baseName));

            if (PhFormatToBuffer(format, 2, string, sizeof(string), NULL))
            {
                SetupSetProgressText(Context, string, NULL);
            }

            PhInitFormatS(&format[0], L"Progress: ");
            PhInitFormatSize(&format[1], currentLength);
            PhInitFormatS(&format[2], L" of ");
            PhInitFormatSize(&format[3], TotalLength);
            PhInitFormatS(&format[4], L" (");
            PhInitFormatI64U(&format[5], percent);
            PhInitFormatS(&format[6], L"%)");

            if (PhFormatToBuffer(format, ARRAYSIZE(format), string, sizeof(string), NULL))
            {
                SetupSetProgressText(Context, NULL, string);
            }

            SetupSetProgressValue(Context, (ULONG)percent);

            if (baseName)
                PhDereferenceObject(baseName);
        }
    }

CleanupExit:

    if (extractPath)
        PhDereferenceObject(extractPath);

    return status;
}

/**
 * Commits staged files to their final file names.
 *
 * \param Context The setup context.
 * \param StagedFiles The staged files list.
 * \return Successful or errant status.
 */
static NTSTATUS SetupExtractBuildCommitFiles(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ PPH_LIST StagedFiles
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    for (ULONG i = 0; i < StagedFiles->Count; i++)
    {
        PPH_STRING file = StagedFiles->Items[i];
        ULONG64 percent = StagedFiles->Count ? 50 + (50 * (i + 1) / StagedFiles->Count) : 100;
        PH_FORMAT format[2];
        WCHAR string[MAX_PATH];
        PPH_STRING baseName = PhGetBaseName(file);

        if (!NT_SUCCESS(status = SetupCommitFile(Context, file)))
        {
            if (baseName) PhDereferenceObject(baseName);
            return status;
        }

        PhInitFormatS(&format[0], L"Finalizing: ");
        PhInitFormatS(&format[1], PhGetStringOrEmpty(baseName));

        if (PhFormatToBuffer(format, 2, string, sizeof(string), NULL))
        {
            SetupSetProgressText(Context, string, NULL);
        }

        SetupSetProgressValue(Context, (ULONG)percent);

        if (baseName)
            PhDereferenceObject(baseName);
    }

    return status;
}

/**
 * Extracts the embedded build archive to the installation directory.
 *
 * \param Context The setup context.
 * \return Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS CALLBACK SetupExtractBuild(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG resourceLength;
    PVOID resourceBuffer = NULL;
    ULONG64 totalLength = 0;
    mz_zip_archive zipFileArchive = { 0 };
    USHORT nativeArchitecture;
    PPH_LIST stagedFiles = NULL;

    status = PhLoadResource(
        NtCurrentImageBase(),
        MAKEINTRESOURCE(IDR_BIN_DATA),
        RT_RCDATA,
        &resourceLength,
        &resourceBuffer
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!mz_zip_reader_init_mem(&zipFileArchive, resourceBuffer, resourceLength, 0))
    {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    nativeArchitecture = SetupGetCurrentArchitecture();
    stagedFiles = PhCreateList(100);

    if (!NT_SUCCESS(status = SetupExtractBuildGetTotalLength(
        &zipFileArchive,
        nativeArchitecture,
        &totalLength
        )))
    {
        goto CleanupExit;
    }

    SetupSetProgressMarquee(Context, FALSE);

    if (!NT_SUCCESS(status = SetupExtractBuildStageFiles(
        Context,
        &zipFileArchive,
        nativeArchitecture,
        totalLength,
        stagedFiles
        )))
    {
        goto CleanupExit;
    }

    status = SetupExtractBuildCommitFiles(Context, stagedFiles);

CleanupExit:

    if (stagedFiles)
    {
        if (NT_SUCCESS(status))
        {
            for (ULONG i = 0; i < stagedFiles->Count; i++)
                SetupFinalizeFile(Context, stagedFiles->Items[i]);
        }
        else
        {
            for (ULONG i = 0; i < stagedFiles->Count; i++)
                SetupRollbackFile(Context, stagedFiles->Items[i]);
        }

        for (ULONG i = 0; i < stagedFiles->Count; i++)
            PhDereferenceObject(stagedFiles->Items[i]);
        PhDereferenceObject(stagedFiles);
    }

    mz_zip_reader_end(&zipFileArchive);

    return status;
}
