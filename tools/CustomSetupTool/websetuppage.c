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

#include <setup.h>

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
