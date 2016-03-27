#include <setup.h>
#include <appsup.h>

BOOLEAN SetupResetCurrentInstall(
    _In_ PVOID Arguments
    )
{
    STATUS_MSG(L"Setting up for first time use...");

    ProcessHackerShutdown();

    ULONG err = ERROR_SERVICE_DOES_NOT_EXIST;

    do
    {
        Sleep(1000);

        err = KphUninstall();

    } while (err != ERROR_SERVICE_DOES_NOT_EXIST);

    RemoveAppCompatEntries();

    //PPH_STRING clientPath = GetProcessHackerInstallPath();
    //PPH_STRING startmenu = GetKnownLocation(CSIDL_COMMON_PROGRAMS, TEXT("\\ProcessHacker2"));
    //PPH_STRING desktopShortcutString = GetKnownLocation(CSIDL_DESKTOPDIRECTORY, TEXT("\\Process Hacker 2.lnk"));
    //PPH_STRING startmenuShortcutString = GetKnownLocation(CSIDL_COMMON_PROGRAMS, TEXT("\\Process Hacker 2.lnk"));
    //PPH_STRING startmenuStartupShortcutString = GetKnownLocation(CSIDL_COMMON_PROGRAMS, TEXT("\\Startup\\Process Hacker 2.lnk"));


    STATUS_MSG(L"Cleaning up...\n");
    Sleep(1000);

    //PPH_STRING clientPathString = StringFormat(TEXT("%s\\ProcessHacker.exe"), SetupInstallPath->Buffer);
    //PPH_STRING uninstallPath = StringFormat(TEXT("%s\\ProcessHacker-Setup.exe"), SetupInstallPath->Buffer);
    //PPH_STRING clientPathArgsString = StringFormat(TEXT("\"%s\\ProcessHacker.exe\""), SetupInstallPath->Buffer);
    //PPH_STRING clientPathExeString = StringFormat(TEXT("\"%s\" \"%%1\""), clientPathString->Buffer);

    // Create the Uninstall key...

    //HKEY keyHandle = RegistryCreateKey(
    //    HKEY_LOCAL_MACHINE, 
    //    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ProcessHacker2"), 
    //    KEY_ALL_ACCESS | KEY_WOW64_32KEY
    //    );
    //
    //if (keyHandle)
    //{
    //    RegistrySetString(keyHandle, TEXT("DisplayName"), TEXT("Process Hacker"));
    //    RegistrySetString(keyHandle, TEXT("InstallLocation"), SetupInstallPath->Buffer);
    //    RegistrySetString(keyHandle, TEXT("UninstallString"), uninstallPath->Buffer);
    //    RegistrySetString(keyHandle, TEXT("DisplayIcon"), uninstallPath->Buffer);
    //    RegistrySetString(keyHandle, TEXT("Publisher"), TEXT("Electronic Arts, Inc."));
    //    RegistrySetString(keyHandle, TEXT("URLInfoAbout"), TEXT("http://wj32.org/ProcessHacker"));
    //    RegistrySetString(keyHandle, TEXT("HelpLink"), TEXT("http://wj32.org/ProcessHacker"));
    //    RegistrySetDword(keyHandle, TEXT("NoModify"), 1);
    //    RegistrySetDword(keyHandle, TEXT("NoRepair"), 1);
    //    RegCloseKey(keyHandle);
    //}

    //if (IsCreateDesktopSelected)
    //{
    //    PPH_STRING desktopFolderString = GetKnownLocation(
    //        CSIDL_DESKTOPDIRECTORY,
    //        TEXT("\\ProcessHacker.lnk")
    //        );

    //    //if (!CreateLink(
    //    //    desktopFolderString->Buffer, clientPathString->Buffer, SetupInstallPath->Buffer,
    //    //    TEXT("")
    //    //    ))
    //    //{
    //    //    DEBUG_MSG(TEXT("ERROR CreateDesktopLink: %u\n"), GetLastError());
    //    //}

    //    PhFree(desktopFolderString);
    //}

    //if (IsCreateStartSelected)
    //{
    //    PPH_STRING startmenuFolderString = GetKnownLocation(
    //        CSIDL_COMMON_PROGRAMS,
    //        TEXT("\\ProcessHacker.lnk")
    //        );

    //    //if (!CreateLink(
    //    //    startmenuFolderString->Buffer, clientPathString->Buffer, SetupInstallPath->Buffer,
    //    //    TEXT("")
    //    //    ))
    //    //{
    //    //    DEBUG_MSG(TEXT("ERROR CreateStartLink: %u\n"), GetLastError());
    //    //}

    //    PhFree(startmenuFolderString);
    //}

    //if (IsCreateRunStartupSelected)
    //{
    //    PPH_STRING startmenuStartupString = GetKnownLocation(
    //        CSIDL_COMMON_STARTUP,
    //        TEXT("\\ProcessHacker.lnk")
    //        );

    //    //if (!CreateLink(
    //    //    startmenuStartupString->Buffer, clientPathString->Buffer, SetupInstallPath->Buffer,
    //    //    TEXT("")
    //    //    ))
    //    //{
    //    //    DEBUG_MSG(TEXT("ERROR CreateLinkStartup: %u\n"), GetLastError());
    //    //}

    //    PhFree(startmenuStartupString);
    //}

    // GetCachedImageIndex Test
    //{
    //    SHFOLDERCUSTOMSETTINGS fcs = { sizeof(SHFOLDERCUSTOMSETTINGS) };
    //    fcs.dwMask = FCSM_ICONFILE;
    //    fcs.pszIconFile = TEXT("ProcessHacker.exe");
    //    fcs.cchIconFile = _tcslen(TEXT("ProcessHacker.exe")) * sizeof(TCHAR);
    //    fcs.iIconIndex = 0;

    //    if (SUCCEEDED(SHGetSetFolderCustomSettings(&fcs, SetupInstallPath->Buffer, FCS_FORCEWRITE)))
    //    {
    //        SHFILEINFO shellFolderInfo = { 0 };

    //        if (SHGetFileInfo(SetupInstallPath->Buffer, 0, &shellFolderInfo, sizeof(SHFILEINFO), SHGFI_ICONLOCATION))
    //        {
    //            PPH_STRING fileName = FileGetBaseName(shellFolderInfo.szDisplayName);


    //            INT shellIconIndex = Shell_GetCachedImageIndex(
    //                fileName->Buffer,
    //                shellFolderInfo.iIcon,
    //                0
    //                );

    //            SHUpdateImage(
    //                fileName->Buffer,
    //                shellFolderInfo.iIcon,
    //                0,
    //                shellIconIndex
    //                );

    //            SHChangeNotify(SHCNE_UPDATEIMAGE, SHCNF_DWORD, NULL, (PVOID)shellIconIndex);

    //            PhFree(fileName);
    //        }
    //    }
    //}

    //_tspawnl(_P_DETACH, clientPathString->Buffer, clientPathArgsString->Buffer, NULL);

    //if (clientPathExeString) 
    //    PhFree(clientPathExeString);

    //if (clientPathArgsString) 
    //    PhFree(clientPathArgsString);

    //if (uninstallPath)
    //    PhFree(uninstallPath);

    //if (clientPathString) 
    //    PhFree(clientPathString);


    return TRUE;
}
