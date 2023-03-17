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

NTSTATUS SetupProgressThread(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    // Create the folder.
    if (!NT_SUCCESS(PhCreateDirectoryWin32(&Context->SetupInstallPath->sr)))
    {
        Context->ErrorCode = ERROR_INVALID_DATA;
        goto CleanupExit;
    }

#ifndef FORCE_TEST_UPDATE_LOCAL_INSTALL

    // Stop the application.
    if (!SetupShutdownApplication(Context))
        goto CleanupExit;

    // Stop the kernel driver.
    if (!SetupUninstallDriver(Context))
        goto CleanupExit;

    // Upgrade the legacy settings file.
    SetupUpgradeSettingsFile();

    // Remove the previous installation.
    //if (Context->SetupResetSettings)
    //    PhDeleteDirectory(Context->SetupInstallPath);

    // Create the uninstaller.
    if (!SetupCreateUninstallFile(Context))
        goto CleanupExit;

    // Create the ARP uninstall entries.
    SetupCreateUninstallKey(Context);

    // Create autorun.
    SetupCreateWindowsOptions(Context);

    // Create shortcuts.
    SetupCreateShortcuts(Context);

    // Set the default image execution options.
    //SetupCreateImageFileExecutionOptions();

#endif

    // Extract the updated files.
    if (!SetupExtractBuild(Context))
        goto CleanupExit;

    PostMessage(Context->DialogHandle, SETUP_SHOWFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:
    PostMessage(Context->DialogHandle, SETUP_SHOWERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}
