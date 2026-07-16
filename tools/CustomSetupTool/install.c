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
 * Installs System Informer.
 *
 * \param Context The setup context.
 * \return Successful or errant status.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS CALLBACK SetupProgressThread(
    _In_ PVOID Context
    )
{
    PPH_SETUP_CONTEXT context = (PPH_SETUP_CONTEXT)Context;
    NTSTATUS status;
    BOOLEAN updateDesktopShortcut = TRUE;
    BOOLEAN desktopShortcutExists = FALSE;
    BOOLEAN removeStartMenuFolder = FALSE;
    PPH_STRING previousInstallPath;
    PPH_STRING currentInstallPath;
    PPH_STRING desktopShortcutPath;

    context->SetupProgressActive = TRUE;

    previousInstallPath = GetApplicationInstallPath();
    currentInstallPath = SetupCreateFullPath(context->SetupInstallPath, L"");

    if (!PhIsNullOrEmptyString(previousInstallPath) &&
        !PhIsNullOrEmptyString(currentInstallPath) &&
        PhEqualStringRef(&previousInstallPath->sr, &currentInstallPath->sr, TRUE))
    {
        if (desktopShortcutPath = PhGetKnownFolderPathZ(&FOLDERID_PublicDesktop, L"\\System Informer.lnk"))
        {
            desktopShortcutExists = PhDoesFileExistWin32(PhGetString(desktopShortcutPath));
            PhDereferenceObject(desktopShortcutPath);
        }

        updateDesktopShortcut = context->SetupCreateDesktopShortcut != desktopShortcutExists;
    }

    PhClearReference(&previousInstallPath);
    PhClearReference(&currentInstallPath);

    if (!PhIsNullOrEmptyString(context->SetupPreviousStartMenuFolderName) && (!context->SetupCreateStartMenuShortcuts ||
        !PhEqualStringRef(&context->SetupPreviousStartMenuFolderName->sr, &context->SetupStartMenuFolderName->sr, TRUE)))
    {
        removeStartMenuFolder = TRUE;
    }

    //
    // Create the folder.
    //

    SetupSetProgressMarquee(context, TRUE);
    SetupSetProgressText(context, L"Creating the installation directory...", NULL);

    if (!NT_SUCCESS(status = PhCreateDirectoryWin32(&context->SetupInstallPath->sr)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

#ifndef FORCE_TEST_UPDATE_LOCAL_INSTALL

    //
    // Stop the application.
    //

    SetupSetProgressText(context, L"Stopping System Informer...", NULL);

    if (!NT_SUCCESS(status = SetupShutdownApplication(context)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Stop the kernel driver.
    //

    SetupSetProgressText(context, L"Stopping the kernel driver...", NULL);

    if (!NT_SUCCESS(status = SetupUninstallDriver(context)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Upgrade the settings file.
    //

    SetupSetProgressText(context, L"Updating settings...", NULL);
    SetupUpgradeSettingsFile();

    //
    // Convert the settings file.
    //

    SetupSetProgressText(context, L"Converting settings...", NULL);
    SetupConvertSettingsFile();

    // Remove the previous installation.
    //if (Context->SetupResetSettings)
    //    PhDeleteDirectory(Context->SetupInstallPath);

    // Perform Windows Options cleanup (registry)
    SetupSetProgressText(context, L"Removing previous Windows integration...", NULL);
    SetupDeleteWindowsOptions(Context);

    // Delete all shortcuts for cleanup

    SetupSetProgressText(context, L"Removing previous shortcuts...", NULL);
    SetupDeleteShortcuts(Context, updateDesktopShortcut, removeStartMenuFolder);

    //
    // Create the uninstaller.
    //

    SetupSetProgressText(context, L"Creating the uninstaller...", NULL);

    if (!NT_SUCCESS(status = SetupCreateUninstallFile(context)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    //
    // Create the ARP uninstall entries.
    //

    SetupSetProgressText(context, L"Creating uninstall registration...", NULL);
    SetupCreateUninstallKey(Context);

    //
    // Create Windows Error Reporting LocalDumps key.
    //

    SetupSetProgressText(context, L"Creating LocalDumps configuration...", NULL);
    SetupCreateLocalDumpsKey();

    //
    // Create autorun.
    //

    SetupSetProgressText(context, L"Creating Windows integration...", NULL);
    SetupCreateWindowsOptions(Context);

    //
    // Create shortcuts.
    //

    SetupSetProgressText(context, L"Creating shortcuts...", NULL);
    SetupCreateShortcuts(Context, updateDesktopShortcut);

    // Set the default image execution options.
    //
    //SetupCreateImageFileExecutionOptions();

#endif

    //
    // Extract the updated files.
    //
    SetupSetProgressText(context, L"Extracting files...", NULL);

    if (!NT_SUCCESS(status = SetupExtractBuild(Context)))
    {
        context->LastStatus = status;
        goto CleanupExit;
    }

    SetupSetProgressText(context, L"Installation complete.", NULL);
    SetupSetProgressValue(context, 100);
    context->SetupProgressActive = FALSE;
    context->SetupCompleted = TRUE;
    PostMessage(context->DialogHandle, SETUP_SHOWFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:
    SetupSetProgressText(context, L"Installation failed.", NULL);
    context->SetupProgressActive = FALSE;
    PostMessage(context->DialogHandle, SETUP_SHOWERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}
