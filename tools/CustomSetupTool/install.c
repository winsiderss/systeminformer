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

NTSTATUS CALLBACK SetupProgressThread(
    _In_ PPH_SETUP_CONTEXT Context
    )
{
    NTSTATUS status;

    // Create the folder.
    if (!NT_SUCCESS(status = PhCreateDirectoryWin32(&Context->SetupInstallPath->sr)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

#ifndef FORCE_TEST_UPDATE_LOCAL_INSTALL

    // Stop the application.
    if (!NT_SUCCESS(status = SetupShutdownApplication(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    // Stop the kernel driver.
    if (!NT_SUCCESS(status = SetupUninstallDriver(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    // Upgrade the settings file.
    SetupUpgradeSettingsFile();
    // Convert the settings file.
    SetupConvertSettingsFile();

    // Remove the previous installation.
    //if (Context->SetupResetSettings)
    //    PhDeleteDirectory(Context->SetupInstallPath);

    // Create the uninstaller.
    if (!NT_SUCCESS(status = SetupCreateUninstallFile(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

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
    if (!NT_SUCCESS(status = SetupExtractBuild(Context)))
    {
        Context->LastStatus = status;
        goto CleanupExit;
    }

    PostMessage(Context->DialogHandle, SETUP_SHOWFINAL, 0, 0);
    return STATUS_SUCCESS;

CleanupExit:
    PostMessage(Context->DialogHandle, SETUP_SHOWERROR, 0, 0);
    return STATUS_UNSUCCESSFUL;
}
