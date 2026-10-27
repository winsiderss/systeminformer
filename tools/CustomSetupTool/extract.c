/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2026
 *
 */

#include "setup.h"

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
 * Checks if the installed file is identical to the staged file by comparing their SHA-256 hashes.
 *
 * \param InstalledFileName The installed file name.
 * \param StagedFileName The staged file name.
 * \return TRUE if the files are identical, otherwise FALSE.
 */
BOOLEAN SetupIsFileUnchanged(
    _In_ PPH_STRING InstalledFileName,
    _In_ PPH_STRING StagedFileName
    )
{
    BYTE installedHash[256 / 8];
    BYTE stagedHash[256 / 8];

    if (!PhDoesFileExistWin32(PhGetString(InstalledFileName)))
        return FALSE;

    return
        NT_SUCCESS(SetupHashFile(InstalledFileName, installedHash)) &&
        NT_SUCCESS(SetupHashFile(StagedFileName, stagedHash)) &&
        RtlEqualMemory(installedHash, stagedHash, sizeof(installedHash));
}

typedef struct _SETUP_STAGE_CONTEXT
{
    PPH_SETUP_CONTEXT Setup;
    PPH_STRING StagingRoot; // with a trailing separator
    PPH_STRING InstallRoot; // with a trailing separator
    PPH_STRING RelativePath; // empty, or a directory path with a trailing separator
    PPH_LIST StagedFiles;
    NTSTATUS Status;
} SETUP_STAGE_CONTEXT, *PSETUP_STAGE_CONTEXT;

/**
 * Callback for moving the extracted files to their .new names next to the installed files.
 *
 * \param RootDirectory Handle to the parent directory.
 * \param Information Pointer to a FILE_DIRECTORY_INFORMATION structure for the current entry.
 * \param Context The stage context.
 * \return TRUE to continue enumeration, FALSE to stop.
 */
_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN SetupStageFilesCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PSETUP_STAGE_CONTEXT context = Context;
    PH_STRINGREF fileName;
    PPH_STRING relativeName;
    PPH_STRING stagedName;
    PPH_STRING finalName;

    if (!NT_SUCCESS(context->Status))
        return FALSE;

    fileName.Buffer = Information->FileName;
    fileName.Length = Information->FileNameLength;

    if (PATH_IS_WIN32_RELATIVE_PREFIX(&fileName))
        return TRUE;

    relativeName = PhConcatStringRef2(&context->RelativePath->sr, &fileName);
    stagedName = PhConcatStringRef2(&context->StagingRoot->sr, &relativeName->sr);
    finalName = PhConcatStringRef2(&context->InstallRoot->sr, &relativeName->sr);

    if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        if (!FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT))
        {
            SETUP_STAGE_CONTEXT child = *context;
            HANDLE directoryHandle;

            child.RelativePath = PhConcatStringRef2(&relativeName->sr, &PhNtPathSeparatorString);

            if (NT_SUCCESS(context->Status = PhCreateFileWin32(
                &directoryHandle,
                PhGetString(stagedName),
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                FILE_ATTRIBUTE_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                )))
            {
                PhEnumDirectoryFile(directoryHandle, NULL, SetupStageFilesCallback, &child);
                NtClose(directoryHandle);

                context->Status = child.Status;
            }

            PhDereferenceObject(child.RelativePath);
        }
    }
    else
    {
        PPH_STRING sessionId = SetupGetSessionId(context->Setup);
        PPH_STRING newName;

        if (PhIsNullOrEmptyString(sessionId))
        {
            context->Status = STATUS_NO_MEMORY;
        }
        else if (NT_SUCCESS(context->Status = PhCreateDirectoryFullPathWin32(&finalName->sr)))
        {
            if (
                PhEndsWithString2(finalName, L"\\ksi.dll", TRUE) &&
                SetupIsFileUnchanged(finalName, stagedName)
                )
            {
                // Don't replace an identical driver interface library that may still be loaded.
                PhDeleteFileWin32(PhGetString(stagedName));
            }
            else
            {
                newName = PhConcatStrings(4, PhGetString(finalName), L".", PhGetString(sessionId), L".new");

                if (NT_SUCCESS(context->Status = PhMoveFileWin32(PhGetString(stagedName), PhGetString(newName), FALSE)))
                {
                    PhAddItemList(context->StagedFiles, PhReferenceObject(finalName));
                }

                PhDereferenceObject(newName);
            }
        }
    }

    PhDereferenceObject(finalName);
    PhDereferenceObject(stagedName);
    PhDereferenceObject(relativeName);

    return NT_SUCCESS(context->Status);
}


/**
 * Commits staged files to their final file names.
 *
 * \param Context The setup context.
 * \param StagedFiles The staged files list.
 * \return Successful or errant status.
 */
NTSTATUS SetupExtractBuildCommitFiles(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ PPH_LIST StagedFiles
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    for (ULONG i = 0; i < StagedFiles->Count; i++)
    {
        PPH_STRING file = StagedFiles->Items[i];
        ULONG64 percent = 100;
        PH_FORMAT format[2];
        WCHAR string[MAX_PATH];
        PPH_STRING baseName = PhGetBaseName(file);

        if (StagedFiles->Count)
        {
            percent = 0;
            PhMultiplyDivide64(i + 1, 100, StagedFiles->Count, &percent);
        }

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
 * Finalizes the files committed by SetupExtractBuild (removes their .bak backups).
 *
 * \param Context The setup context.
 */
VOID SetupFinalizeStagedFiles(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_LIST stagedFiles = Context->SetupStagedFiles;

    if (!stagedFiles)
        return;

    Context->SetupStagedFiles = NULL;

    for (ULONG i = 0; i < stagedFiles->Count; i++)
    {
        SetupFinalizeFile(Context, stagedFiles->Items[i]);
        PhDereferenceObject(stagedFiles->Items[i]);
    }

    PhDereferenceObject(stagedFiles);
}

/**
 * Rolls back the files committed by SetupExtractBuild (restores their .bak backups).
 *
 * \param Context The setup context.
 */
VOID SetupRollbackStagedFiles(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    PPH_LIST stagedFiles = Context->SetupStagedFiles;

    if (!stagedFiles)
        return;

    Context->SetupStagedFiles = NULL;

    for (ULONG i = 0; i < stagedFiles->Count; i++)
    {
        SetupRollbackFile(Context, stagedFiles->Items[i]);
        PhDereferenceObject(stagedFiles->Items[i]);
    }

    PhDereferenceObject(stagedFiles);
}

/**
 * Converts a 7-Zip wrapper result into an NTSTATUS.
 *
 * \param Result The SI7Z_RESULT returned by the wrapper.
 * \return The corresponding status.
 */
static NTSTATUS SetupStatusFromSevenZipResult(
    _In_ SI7Z_RESULT Result
    )
{
    switch (Result)
    {
    case SI7Z_OK:
        return STATUS_SUCCESS;
    case SI7Z_ERROR_INVALID_ARGUMENT:
        return STATUS_INVALID_PARAMETER;
    case SI7Z_ERROR_UNSUPPORTED_FORMAT:
    case SI7Z_ERROR_OPEN_FAILED:
        return STATUS_INVALID_IMAGE_FORMAT;
    case SI7Z_ERROR_UNSUPPORTED_METHOD:
    case SI7Z_ERROR_NOT_IMPLEMENTED:
        return STATUS_NOT_SUPPORTED;
    case SI7Z_ERROR_OUT_OF_MEMORY:
        return STATUS_NO_MEMORY;
    case SI7Z_ERROR_ITEM_NOT_FOUND:
        return STATUS_NOT_FOUND;
    case SI7Z_ERROR_DATA_ERROR:
        return STATUS_DATA_ERROR;
    case SI7Z_ERROR_UNSAFE_PATH:
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    case SI7Z_ERROR_WRITE_FAILED:
        return STATUS_CANNOT_MAKE;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

/**
 * Extracts the embedded build archive to the installation directory.
 *
 * The archive is extracted to a staging directory, each file is moved to
 * "file.<session>.new" next to its final name and then committed with
 * SetupCommitFile (the installed file is renamed to "file.<session>.bak"). A failure rolls
 * every committed file back. On success the backups are finalized, unless
 * SetupDeferFinalize is set, in which case the caller must call SetupFinalizeStagedFiles or
 * SetupRollbackStagedFiles.
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
    PVOID zipBuffer = NULL;
    SIZE_T zipBufferLength = 0;
    PPH_STRING installRoot = NULL;
    PPH_STRING stagingRoot = NULL;
    PPH_STRING sessionId;
    SETUP_STAGE_CONTEXT stageContext;
    HANDLE directoryHandle;

    if (Context->SetupBuildZipPath)
    {
        HANDLE zipFileHandle = NULL;
        LARGE_INTEGER zipFileSize;
        ULONG bytesRead;

        status = PhCreateFileWin32Ex(
            &zipFileHandle,
            PhGetString(Context->SetupBuildZipPath),
            FILE_GENERIC_READ,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            status = PhGetFileSize(zipFileHandle, &zipFileSize);
        }

        if (NT_SUCCESS(status) && (zipFileSize.QuadPart <= 0 || zipFileSize.QuadPart > SETUP_MAX_DOWNLOAD_SIZE))
        {
            status = STATUS_FILE_TOO_LARGE;
        }

        if (NT_SUCCESS(status))
        {
            zipBufferLength = (SIZE_T)zipFileSize.QuadPart;
            status = PhAllocateHeap(zipBufferLength, &zipBuffer);
            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhReadFile(
                zipFileHandle,
                zipBuffer,
                (ULONG)zipBufferLength,
                NULL,
                &bytesRead
                );

            if (NT_SUCCESS(status) && bytesRead != zipBufferLength)
            {
                status = STATUS_END_OF_FILE;
            }
        }

        if (zipFileHandle)
            NtClose(zipFileHandle);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }
    else
    {
        status = PhLoadResource(
            NtCurrentImageBase(),
            MAKEINTRESOURCE(IDR_BIN_DATA),
            RT_RCDATA,
            &resourceLength,
            &resourceBuffer
            );

        if (!NT_SUCCESS(status))
            return status;

        zipBufferLength = resourceLength;
        zipBuffer = PhAllocateSafe(zipBufferLength);
        if (!zipBuffer) return STATUS_NO_MEMORY;

        if (!PhBase64Decode((PCSTR)resourceBuffer, resourceLength, zipBuffer, zipBufferLength, &zipBufferLength))
        {
            status = STATUS_INVALID_BUFFER_SIZE;
            goto CleanupExit;
        }
    }

    if (!(sessionId = SetupGetSessionId(Context)))
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    if (PhEndsWithStringRef(&Context->SetupInstallPath->sr, &PhNtPathSeparatorString, FALSE))
        installRoot = PhReferenceObject(Context->SetupInstallPath);
    else
        installRoot = PhConcatStringRef2(&Context->SetupInstallPath->sr, &PhNtPathSeparatorString);

    stagingRoot = PhFormatString(L"%supdate.%s.staging\\", PhGetString(installRoot), PhGetString(sessionId));

    // Remove a stale staging directory from a previous attempt.
    PhDeleteDirectoryWin32(&stagingRoot->sr);

    if (!NT_SUCCESS(status = PhCreateDirectoryWin32(&stagingRoot->sr)))
        goto CleanupExit;

    SetupSetProgressMarquee(Context, TRUE);

    // Use the shared 7-Zip wrapper for extraction. It validates every entry path
    // before writing (no "..", device names or writes through reparse points) and
    // reads the embedded buffer directly, avoiding a second archive implementation.
    if (!NT_SUCCESS(status = SetupStatusFromSevenZipResult(Si7zExtractBufferToDirectory(
        zipBuffer,
        zipBufferLength,
        NULL,
        PhGetString(stagingRoot)
        ))))
    {
        goto CleanupExit;
    }

    // Move the extracted files next to their final names.
    Context->SetupStagedFiles = PhCreateList(100);

    memset(&stageContext, 0, sizeof(SETUP_STAGE_CONTEXT));
    stageContext.Setup = Context;
    stageContext.StagingRoot = stagingRoot;
    stageContext.InstallRoot = installRoot;
    stageContext.RelativePath = PhReferenceEmptyString();
    stageContext.StagedFiles = Context->SetupStagedFiles;
    stageContext.Status = STATUS_SUCCESS;

    if (NT_SUCCESS(status = PhCreateFileWin32(
        &directoryHandle,
        PhGetString(stagingRoot),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
        )))
    {
        status = PhEnumDirectoryFile(directoryHandle, NULL, SetupStageFilesCallback, &stageContext);
        NtClose(directoryHandle);

        if (!NT_SUCCESS(stageContext.Status))
            status = stageContext.Status;
    }

    PhDereferenceObject(stageContext.RelativePath);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    SetupSetProgressMarquee(Context, FALSE);
    status = SetupExtractBuildCommitFiles(Context, Context->SetupStagedFiles);

CleanupExit:

    if (stagingRoot)
    {
        PhDeleteDirectoryWin32(&stagingRoot->sr);
        PhDereferenceObject(stagingRoot);
    }

    if (installRoot)
        PhDereferenceObject(installRoot);

    if (!NT_SUCCESS(status))
    {
        SetupRollbackStagedFiles(Context);
    }
    else if (!Context->SetupDeferFinalize)
    {
        SetupFinalizeStagedFiles(Context);
    }

    if (zipBuffer)
        PhFree(zipBuffer);

    return status;
}
