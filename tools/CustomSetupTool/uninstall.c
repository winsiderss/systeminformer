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

/**
 * Callback for scheduling directory entries for deletion at the next reboot.
 *
 * \param RootDirectory Handle to the parent directory.
 * \param Information Pointer to a FILE_DIRECTORY_INFORMATION structure for the current entry.
 * \param Context The parent directory path (with a trailing separator).
 * \return TRUE to continue enumeration, FALSE to stop.
 */
_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN SetupDeleteDirectoryOnRebootCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PPH_STRING parentPath = Context;
    PH_STRINGREF fileName;
    PPH_STRING fullPath;

    fileName.Buffer = Information->FileName;
    fileName.Length = Information->FileNameLength;

    if (PATH_IS_WIN32_RELATIVE_PREFIX(&fileName))
        return TRUE;

    fullPath = PhConcatStringRef2(&parentPath->sr, &fileName);

    if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        // Skip reparse points so we never walk outside the install directory.
        if (!FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT))
        {
            HANDLE directoryHandle;
            PPH_STRING childPath;

            childPath = PhConcatStringRef2(&fullPath->sr, &PhNtPathSeparatorString);

            if (NT_SUCCESS(PhCreateFileWin32(
                &directoryHandle,
                PhGetString(fullPath),
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                FILE_ATTRIBUTE_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                )))
            {
                PhEnumDirectoryFile(directoryHandle, NULL, SetupDeleteDirectoryOnRebootCallback, childPath);
                NtClose(directoryHandle);
            }

            PhDereferenceObject(childPath);
        }
    }

    // Deletes are processed in registration order: children are registered before their parent.
    MoveFileEx(PhGetString(fullPath), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

    PhDereferenceObject(fullPath);

    return TRUE;
}

/**
 * Callback for moving files out of a directory into the temporary directory.
 * Files that are in use can't be deleted but can usually be renamed.
 *
 * \param RootDirectory Handle to the parent directory.
 * \param Information Pointer to a FILE_DIRECTORY_INFORMATION structure for the current entry.
 * \param Context The parent directory path (with a trailing separator).
 * \return TRUE to continue enumeration, FALSE to stop.
 */
_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN SetupMoveDirectoryToTempCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PPH_STRING parentPath = Context;
    PH_STRINGREF fileName;
    PPH_STRING fullPath;

    fileName.Buffer = Information->FileName;
    fileName.Length = Information->FileNameLength;

    if (PATH_IS_WIN32_RELATIVE_PREFIX(&fileName))
        return TRUE;

    fullPath = PhConcatStringRef2(&parentPath->sr, &fileName);

    if (FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        if (!FlagOn(Information->FileAttributes, FILE_ATTRIBUTE_REPARSE_POINT))
        {
            HANDLE directoryHandle;
            PPH_STRING childPath;

            childPath = PhConcatStringRef2(&fullPath->sr, &PhNtPathSeparatorString);

            if (NT_SUCCESS(PhCreateFileWin32(
                &directoryHandle,
                PhGetString(fullPath),
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                FILE_ATTRIBUTE_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                )))
            {
                PhEnumDirectoryFile(directoryHandle, NULL, SetupMoveDirectoryToTempCallback, childPath);
                NtClose(directoryHandle);
            }

            PhDereferenceObject(childPath);
        }
    }
    else
    {
        PPH_STRING tempFileName = PhGetTemporaryDirectoryRandomAlphaFileName();

        if (NT_SUCCESS(PhMoveFileWin32(PhGetString(fullPath), PhGetString(tempFileName), FALSE)))
        {
            // Remove the moved file at the next reboot if it is still in use.
            MoveFileEx(PhGetString(tempFileName), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }

        PhDereferenceObject(tempFileName);
    }

    PhDereferenceObject(fullPath);

    return TRUE;
}

/**
 * Moves all files in a directory tree to the temporary directory.
 *
 * \param DirectoryPath The directory path.
 */
VOID SetupMoveDirectoryToTemp(
    _In_ PPH_STRING DirectoryPath
    )
{
    HANDLE directoryHandle;
    PPH_STRING rootPath;

    if (PhEndsWithStringRef(&DirectoryPath->sr, &PhNtPathSeparatorString, FALSE))
        rootPath = PhReferenceObject(DirectoryPath);
    else
        rootPath = PhConcatStringRef2(&DirectoryPath->sr, &PhNtPathSeparatorString);

    // Don't follow the root if it's a junction/symlink (e.g. Scoop's "current" link),
    // matching PhDeleteDirectoryWin32 which only removes the link itself.
    if (NT_SUCCESS(PhCreateFileWin32(
        &directoryHandle,
        PhGetString(rootPath),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        )))
    {
        PhEnumDirectoryFile(directoryHandle, NULL, SetupMoveDirectoryToTempCallback, rootPath);
        NtClose(directoryHandle);
    }

    PhDereferenceObject(rootPath);
}

/**
 * Schedules a directory tree for deletion at the next reboot.
 *
 * \param DirectoryPath The directory path.
 */
VOID SetupDeleteDirectoryOnReboot(
    _In_ PPH_STRING DirectoryPath
    )
{
    HANDLE directoryHandle;
    PPH_STRING rootPath;

    if (PhEndsWithStringRef(&DirectoryPath->sr, &PhNtPathSeparatorString, FALSE))
        rootPath = PhReferenceObject(DirectoryPath);
    else
        rootPath = PhConcatStringRef2(&DirectoryPath->sr, &PhNtPathSeparatorString);

    if (NT_SUCCESS(PhCreateFileWin32(
        &directoryHandle,
        PhGetString(rootPath),
        FILE_LIST_DIRECTORY | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT
        )))
    {
        PhEnumDirectoryFile(directoryHandle, NULL, SetupDeleteDirectoryOnRebootCallback, rootPath);
        NtClose(directoryHandle);
    }

    // The root directory goes last, after everything inside it.
    MoveFileEx(PhGetString(DirectoryPath), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

    PhDereferenceObject(rootPath);
}

/**
 * Resolves the install directory through any junctions or symbolic links
 * (e.g. Scoop's "current" link) to the real directory on disk.
 *
 * \param DirectoryPath The install directory path.
 * \return The resolved directory path, or NULL if it couldn't be resolved or
 * resolves to a volume root.
 */
PPH_STRING SetupResolveInstallPath(
    _In_ PPH_STRING DirectoryPath
    )
{
    static CONST PH_STRINGREF win32PrefixSr = PH_STRINGREF_INIT(L"\\\\?\\");
    static CONST PH_STRINGREF uncPrefixSr = PH_STRINGREF_INIT(L"\\\\?\\UNC\\");
    HANDLE directoryHandle;
    PPH_STRING finalPath = NULL;
    ULONG length;

    // Open without FILE_OPEN_REPARSE_POINT so any links are followed to the target.
    if (!NT_SUCCESS(PhCreateFileWin32(
        &directoryHandle,
        PhGetString(DirectoryPath),
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
        )))
    {
        return NULL;
    }

    length = GetFinalPathNameByHandle(directoryHandle, NULL, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);

    if (length)
    {
        finalPath = PhCreateStringEx(NULL, length * sizeof(WCHAR));

        length = GetFinalPathNameByHandle(directoryHandle, finalPath->Buffer, length, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);

        if (length && length * sizeof(WCHAR) < finalPath->Length)
        {
            finalPath->Length = length * sizeof(WCHAR);
            finalPath->Buffer[length] = UNICODE_NULL;
        }
        else
        {
            PhClearReference(&finalPath);
        }
    }

    NtClose(directoryHandle);

    if (!finalPath)
        return NULL;

    // Strip the \\?\ prefix for local paths (keep \\?\UNC\ as-is).
    if (PhStartsWithStringRef(&finalPath->sr, &win32PrefixSr, FALSE) &&
        !PhStartsWithStringRef(&finalPath->sr, &uncPrefixSr, TRUE))
    {
        PhMoveReference(&finalPath, PhCreateStringEx(
            finalPath->Buffer + win32PrefixSr.Length / sizeof(WCHAR),
            finalPath->Length - win32PrefixSr.Length
            ));
    }

    if (PhEndsWithStringRef(&finalPath->sr, &PhNtPathSeparatorString, FALSE))
    {
        finalPath->Length -= sizeof(WCHAR);
        finalPath->Buffer[finalPath->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    // Never delete a volume root (e.g. a link that points at C:\).
    if (finalPath->Length <= 2 * sizeof(WCHAR))
    {
        PhDereferenceObject(finalPath);
        return NULL;
    }

    return finalPath;
}

/**
 * Removes a directory tree, falling back to the temporary directory and
 * delete-on-reboot for files that are in use.
 *
 * \param Context The setup context.
 * \param DirectoryPath The directory path.
 */
static VOID SetupRemoveInstallDirectory(
    _In_ PPH_SETUP_CONTEXT Context,
    _In_ PPH_STRING DirectoryPath
    )
{
    if (!NT_SUCCESS(PhDeleteDirectoryWin32(&DirectoryPath->sr)))
    {
        //
        // Files that are still in use (e.g. ksi.dll) can't be deleted but can be renamed.
        // Move everything to the temporary directory and retry removing the directory.
        // If that also fails, schedule the remainder for deletion at the next reboot.
        // See SetupUninstallBuild for the full sequence.
        //

        SetupMoveDirectoryToTemp(DirectoryPath);

        if (!NT_SUCCESS(PhDeleteDirectoryWin32(&DirectoryPath->sr)))
        {
            // Anything that couldn't be moved (e.g. the temp directory is on another volume)
            // is removed at the next reboot, followed by the directory itself.
            SetupDeleteDirectoryOnReboot(DirectoryPath);

            Context->NeedsReboot = TRUE;
        }
    }
}

/**
 * Uninstalls System Informer.
 *
 * If the install directory is a junction or symbolic link (e.g. Scoop's
 * "current" link), the real target directory is removed first and then the
 * link itself.
 *
 * The install directory is removed with PhDeleteDirectoryWin32. If that fails:
 * 1. SetupMoveDirectoryToTemp renames every file in the install tree into the
 *    temporary directory under a random name (reparse points are skipped). Files
 *    that are in use can usually be renamed even when they can't be deleted.
 * 2. PhDeleteDirectoryWin32 is retried, which normally succeeds after the move.
 * 3. Each moved file is also scheduled for deletion at the next reboot in case
 *    a locked handle keeps it alive.
 * 4. If the retry still fails (e.g. the temporary directory is on another
 *    volume), SetupDeleteDirectoryOnReboot schedules the remaining files and
 *    directories (children first, root last) for deletion at the next reboot
 *    and NeedsReboot is set.
 *
 * \param Context The setup context.
 * \return Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS CALLBACK SetupUninstallBuild(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;

    //
    // Roll back an interrupted update so the uninstall doesn't leave .bak/.new files behind.
    //

    SetupRecoverIncompleteUpdate(Context);

    //
    // Stop the application.
    //

    if (!NT_SUCCESS(status = SetupShutdownApplication(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Stop the kernel driver.
    //

    if (!NT_SUCCESS(status = SetupUninstallDriver(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Remove all Windows Options (registry cleanup)
    //

    SetupDeleteWindowsOptions(Context);

    //
    // Remove Windows Error Reporting.
    //

    SetupDeleteLocalDumpsKey();

    //
    // Remove all shortcuts.
    //
    SetupDeleteShortcuts(Context, TRUE, TRUE);

    //
    // Remove the uninstaller.
    //

    SetupDeleteUninstallFile(Context);

    //
    // Remove the ARP uninstall entry.
    //

    SetupDeleteUninstallKey();

    //
    // Remove the previous installation.
    //

    {
        PPH_STRING realInstallPath;

        if (realInstallPath = SetupResolveInstallPath(Context->SetupInstallPath))
        {
            PH_STRINGREF installPath = Context->SetupInstallPath->sr;

            if (PhEndsWithStringRef(&installPath, &PhNtPathSeparatorString, FALSE))
                installPath.Length -= sizeof(WCHAR);

            // The install path is a link: remove the real directory it points to.
            if (!PhEqualStringRef(&realInstallPath->sr, &installPath, TRUE))
                SetupRemoveInstallDirectory(Context, realInstallPath);

            PhDereferenceObject(realInstallPath);
        }

        // Remove the install directory (or the now-dangling link itself).
        SetupRemoveInstallDirectory(Context, Context->SetupInstallPath);
    }

    //
    // Remove the application data.
    //

    if (Context->SetupRemoveAppData)
    {
        SetupDeleteAppdataDirectory(Context);
    }

    //
    // Show the uninstall completed page.
    //

    SetupSetProgressText(Context, L"Uninstall complete.", NULL);
    SetupSetProgressValue(Context, 100);
    Context->SetupProgressActive = FALSE;
    PostMessage(Context->DialogHandle, SETUP_SHOWUNINSTALLFINAL, 0, 0);
    return STATUS_SUCCESS;

    //
    // Show the uninstall failed page.
    //

CleanupExit:
    SetupSetProgressText(Context, L"Uninstall failed.", NULL);
    Context->SetupProgressActive = FALSE;
    PostMessage(Context->DialogHandle, SETUP_SHOWUNINSTALLERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

/**
 * Callback for the uninstall confirmation task dialog.
 *
 * \param hwndDlg The task dialog window handle.
 * \param uMsg The notification message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \param dwRefData The setup context.
 * \return S_OK to continue, otherwise an HRESULT value.
 */
HRESULT CALLBACK TaskDialogUninstallConfirmCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            PhCenterWindow(hwndDlg, NULL);

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                SendMessage(hwndDlg, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, TRUE);
            }
        }
        break;
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDYES)
            {
#ifndef FORCE_TEST_UPDATE_LOCAL_INSTALL
                if (PhGetOwnTokenAttributes().Elevated)
                {
                    ShowUninstallingPageDialog(context);
                    return S_FALSE;
                }
                else
                {
                    NTSTATUS status;
                    PPH_STRING applicationFileName;
                    PH_STRINGREF applicationCommandLine;

                    if (!NT_SUCCESS(status = PhGetProcessCommandLineStringRef(&applicationCommandLine)))
                    {
                        context->LastStatus = status;
                        return S_FALSE;
                    }
                    if (!(applicationFileName = PhGetApplicationFileNameWin32()))
                    {
                        context->LastStatus = STATUS_NO_MEMORY;
                        return S_FALSE;
                    }

                    if (NT_SUCCESS(status = PhShellExecuteEx(
                        hwndDlg,
                        PhGetString(applicationFileName),
                        PhGetStringRefZ(&applicationCommandLine),
                        NULL,
                        SW_SHOW,
                        PH_SHELL_EXECUTE_ADMIN,
                        0,
                        &context->SubProcessHandle
                        )))
                    {
                        ShowWindow(hwndDlg, SW_HIDE);
                    }
                    else
                    {
                        context->LastStatus = status;
                        PhDereferenceObject(applicationFileName);
                        return S_FALSE;
                    }
                }
#else
                ShowUninstallingPageDialog(context);
                return S_FALSE;
#endif
            }
        }
        break;
    case TDN_VERIFICATION_CLICKED:
        {
            context->SetupRemoveAppData = !!(BOOL)wParam;
        }
        break;
    }

    return S_OK;
}

/**
 * Callback for the uninstall error task dialog.
 *
 * \param hwndDlg The task dialog window handle.
 * \param uMsg The notification message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \param dwRefData The setup context.
 * \return S_OK to continue, otherwise an HRESULT value.
 */
HRESULT CALLBACK TaskDialogUninstallErrorCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
)
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_BUTTON_CLICKED:
        {
            if ((INT)wParam == IDRETRY)
            {
                ShowUninstallingPageDialog(context);
                return S_FALSE;
            }
        }
        break;
    }

    return S_OK;
}

/**
 * Callback for the uninstall progress task dialog.
 *
 * \param hwndDlg The task dialog window handle.
 * \param uMsg The notification message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \param dwRefData The setup context.
 * \return S_OK to continue, otherwise an HRESULT value.
 */
HRESULT CALLBACK TaskDialogUninstallCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        {
            SendMessage(hwndDlg, TDM_SET_MARQUEE_PROGRESS_BAR, TRUE, 0);
            SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_MARQUEE, TRUE, 1);

            PhCreateThread2(SetupUninstallBuild, context);
        }
        break;
    }

    return S_OK;
}

/**
 * Callback for the uninstall completed task dialog.
 *
 * \param hwndDlg The task dialog window handle.
 * \param uMsg The notification message.
 * \param wParam Additional message information.
 * \param lParam Additional message information.
 * \param dwRefData The setup context.
 * \return S_OK to continue, otherwise an HRESULT value.
 */
HRESULT CALLBACK TaskDialogUninstallCompleteCallbackProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR dwRefData
    )
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)dwRefData;

    switch (uMsg)
    {
    case TDN_NAVIGATED:
        SetupApplyDarkModeToPage(hwndDlg);
        SendMessage(hwndDlg, TDM_SET_PROGRESS_BAR_POS, 100, 0);
        break;
    }

    return S_OK;
}

/**
 * Shows the uninstall completed page.
 *
 * \param Context The setup context.
 */
VOID ShowUninstallCompletedPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallCompleteCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"System Informer has been uninstalled.";
    if (Context->NeedsReboot)
        config.pszContent = L"A reboot is required to complete the uninstall.";
    else
        config.pszContent = L"Click close to exit setup.";

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * Shows the uninstall progress page.
 *
 * \param Context The setup context.
 */
VOID ShowUninstallingPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED | TDF_SHOW_MARQUEE_PROGRESS_BAR;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = L"Uninstalling System Informer...";

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * Shows the uninstall error page.
 *
 * \param Context The setup context.
 */
VOID ShowUninstallErrorPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.dwCommonButtons = TDCBF_RETRY_BUTTON | TDCBF_CLOSE_BUTTON;
    config.hMainIcon = Context->IconLargeHandle;
    config.pfCallback = TaskDialogUninstallErrorCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    PPH_STRING statusMessage = NULL;

    if (Context->LastStatus)
        statusMessage = PhGetStatusMessage(Context->LastStatus, 0);
    if (statusMessage)
        config.pszMainInstruction = statusMessage->Buffer;
    else
        config.pszMainInstruction = L"Uninstall failed with an error.";
    config.pszContent = L"Click retry to try again or close to exit setup.";

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

/**
 * Shows the uninstall confirmation page.
 *
 * \param Context The setup context.
 */
VOID ShowUninstallPageDialog(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    TASKDIALOG_BUTTON buttonArray[] =
    {
        { IDYES, L"Uninstall" }
    };
    TASKDIALOGCONFIG config;

    memset(&config, 0, sizeof(TASKDIALOGCONFIG));
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.dwFlags = TDF_USE_HICON_MAIN | TDF_ALLOW_DIALOG_CANCELLATION | TDF_CAN_BE_MINIMIZED;
    config.hMainIcon = Context->IconLargeHandle;
    config.pButtons = buttonArray;
    config.cButtons = ARRAYSIZE(buttonArray);
    config.pfCallback = TaskDialogUninstallConfirmCallbackProc;
    config.lpCallbackData = (LONG_PTR)Context;

    config.cxWidth = 200;
    config.pszWindowTitle = PhApplicationName;
    config.pszMainInstruction = PhApplicationName;
    config.pszContent = L"Are you sure you want to uninstall System Informer?";
    if (PhGetOwnTokenAttributes().Elevated)
        config.pszVerificationText = L"Remove application settings";
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.nDefaultButton = IDCANCEL;

    PhTaskDialogNavigatePage(Context->DialogHandle, &config);
}

