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

//NTSTATUS SetupDownloadWebSetupThread(
//    _In_ PPH_SETUP_CONTEXT Context
//    )
//{
//    ULONGLONG currentVersion = 0;
//    ULONGLONG latestVersion = 0;
//    PPH_STRING setupFileName;
//    PH_IMAGE_VERSION_INFO versionInfo;
//
//    if (!SetupQueryUpdateData(Context))
//        goto CleanupExit;
//
//    setupFileName = PhGetApplicationFileName();
//
//    if (!PhInitializeImageVersionInfo(&versionInfo, PhGetString(setupFileName)))
//        goto CleanupExit;
//
//    currentVersion = ParseVersionString(versionInfo.FileVersion);
//
//#ifdef FORCE_UPDATE_CHECK
//    latestVersion = MAKE_VERSION_ULONGLONG(
//        9999,
//        9999,
//        9999,
//        0
//        );
//#else
//    latestVersion = ParseVersionString(Context->WebSetupFileVersion);
//#endif
//
//    // Compare the current version against the latest available version
//    if (currentVersion < latestVersion)
//    {
//        if (!UpdateDownloadUpdateData(Context))
//            goto CleanupExit;
//    }
//
//    //PostMessage(Context->DialogHandle, PSM_SETCURSELID, 0, IDD_DIALOG5);
//    return STATUS_SUCCESS;
//
//CleanupExit:
//
//    //PostMessage(Context->DialogHandle, PSM_SETCURSELID, 0, IDD_ERROR);
//    return STATUS_FAIL_CHECK;
//}
//
//INT_PTR CALLBACK SetupWebSetup_WndProc(
//    _In_ HWND hwndDlg,
//    _In_ UINT uMsg,
//    _Inout_ WPARAM wParam,
//    _Inout_ LPARAM lParam
//    )
//{
//    //SetWindowText(context->MainHeaderHandle, L"Checking for newer websetup version...");
//    //SetWindowText(context->StatusHandle, L"Requesting latest version...");
//    //SetWindowText(context->SubStatusHandle, L"");
//    //PhCreateThread2(SetupDownloadWebSetupThread, context);
//}
