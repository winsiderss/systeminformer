#include <setup.h>
#include <appsup.h>

#include <Shlobj.h>
#include <Netlistmgr.h>

HBITMAP LoadPngImageFromResources(
    _In_ PCWSTR Name
    )
{
    UINT width = 0;
    UINT height = 0;
    UINT frameCount = 0;
    BOOLEAN isSuccess = FALSE;
    ULONG resourceLength = 0;
    HGLOBAL resourceHandle = NULL;
    HRSRC resourceHandleSource = NULL;
    WICInProcPointer resourceBuffer = NULL;

    BITMAPINFO bitmapInfo = { 0 };
    HBITMAP bitmapHandle = NULL;
    PBYTE bitmapBuffer = NULL;

    IWICStream* wicStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapScaler* wicScaler = NULL;
    WICPixelFormatGUID pixelFormat;

    WICRect rect = { 0, 0, 164, 164 };

    __try
    {
        // Create the ImagingFactory
        if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &wicFactory)))
            __leave;

        // Find the resource
        if ((resourceHandleSource = FindResource(PhLibImageBase, Name, L"PNG")) == NULL)
            __leave;

        // Get the resource length
        resourceLength = SizeofResource(PhLibImageBase, resourceHandleSource);

        // Load the resource
        if ((resourceHandle = LoadResource(PhLibImageBase, resourceHandleSource)) == NULL)
            __leave;

        if ((resourceBuffer = (WICInProcPointer)LockResource(resourceHandle)) == NULL)
            __leave;

        // Create the Stream
        if (FAILED(IWICImagingFactory_CreateStream(wicFactory, &wicStream)))
            __leave;

        // Initialize the Stream from Memory
        if (FAILED(IWICStream_InitializeFromMemory(wicStream, resourceBuffer, resourceLength)))
            __leave;

        if (FAILED(IWICImagingFactory_CreateDecoder(wicFactory, &GUID_ContainerFormatPng, NULL, &wicDecoder)))
            __leave;

        if (FAILED(IWICBitmapDecoder_Initialize(wicDecoder, (IStream*)wicStream, WICDecodeMetadataCacheOnLoad)))
            __leave;

        // Get the Frame count
        if (FAILED(IWICBitmapDecoder_GetFrameCount(wicDecoder, &frameCount)) || frameCount < 1)
            __leave;

        // Get the Frame
        if (FAILED(IWICBitmapDecoder_GetFrame(wicDecoder, 0, &wicFrame)))
            __leave;

        // Get the WicFrame image format
        if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicFrame, &pixelFormat)))
            __leave;

        // Check if the image format is supported:
        if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppRGBA)) //  GUID_WICPixelFormat32bppPBGRA
        {
            wicBitmapSource = (IWICBitmapSource*)wicFrame;
        }
        else
        {
            IWICFormatConverter* wicFormatConverter = NULL;

            if (FAILED(IWICImagingFactory_CreateFormatConverter(wicFactory, &wicFormatConverter)))
                __leave;

            if (FAILED(IWICFormatConverter_Initialize(
                wicFormatConverter,
                (IWICBitmapSource*)wicFrame,
                &GUID_WICPixelFormat32bppBGRA,
                WICBitmapDitherTypeNone,
                NULL,
                0.0,
                WICBitmapPaletteTypeCustom
                )))
            {
                IWICFormatConverter_Release(wicFormatConverter);
                __leave;
            }

            // Convert the image to the correct format:
            IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource);
            IWICFormatConverter_Release(wicFormatConverter);
            IWICBitmapFrameDecode_Release(wicFrame);
        }

        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = rect.Width;
        bitmapInfo.bmiHeader.biHeight = -((LONG)rect.Height);
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;

        HDC hdc = CreateCompatibleDC(NULL);
        bitmapHandle = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, (PVOID*)&bitmapBuffer, NULL, 0);
        ReleaseDC(NULL, hdc);

        // Check if it's the same rect as the requested size.
        //if (width != rect.Width || height != rect.Height)
        if (FAILED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicScaler)))
            __leave;
        if (FAILED(IWICBitmapScaler_Initialize(wicScaler, wicBitmapSource, rect.Width, rect.Height, WICBitmapInterpolationModeFant)))
            __leave;
        if (FAILED(IWICBitmapScaler_CopyPixels(wicScaler, &rect, rect.Width * 4, rect.Width * rect.Height * 4, bitmapBuffer)))
            __leave;

        isSuccess = TRUE;
    }
    __finally
    {
        if (wicScaler)
        {
            IWICBitmapScaler_Release(wicScaler);
        }

        if (wicBitmapSource)
        {
            IWICBitmapSource_Release(wicBitmapSource);
        }

        if (wicStream)
        {
            IWICStream_Release(wicStream);
        }

        if (wicDecoder)
        {
            IWICBitmapDecoder_Release(wicDecoder);
        }

        if (wicFactory)
        {
            IWICImagingFactory_Release(wicFactory);
        }

        if (resourceHandle)
        {
            FreeResource(resourceHandle);
        }
    }

    return bitmapHandle;
}

VOID InitializeFont(
    _In_ HWND ControlHandle,
    _In_ LONG Height,
    _In_ LONG Weight
    )
{
    LOGFONT fontAttributes = { 0 };
    fontAttributes.lfHeight = Height;
    fontAttributes.lfWeight = Weight;
    fontAttributes.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;

    wcscpy_s(
        fontAttributes.lfFaceName,
        ARRAYSIZE(fontAttributes.lfFaceName),
        WindowsVersion > WINDOWS_VISTA ? L"Segoe UI" : L"MS Shell Dlg 2"
        );

    // Verdana
    // Tahoma
    // MS Sans Serif
    //wcscpy_s(
    //    fontAttributes.lfFaceName,
    //    ARRAYSIZE(fontAttributes.lfFaceName),
    //    L"Marlett"
    //    );

    HFONT fontHandle = CreateFontIndirect(&fontAttributes);

    SendMessage(ControlHandle, WM_SETFONT, (WPARAM)fontHandle, FALSE);
}


PPH_STRING GetSystemTemp(VOID)
{
    PPH_STRING dirPath;
    ULONG dirPathLength;

    // Query the directory length...
    dirPathLength = GetTempPath(0, NULL);

    // Allocate the string...
    dirPath = PhCreateStringEx(NULL, dirPathLength * sizeof(TCHAR));

    // Query the directory path...
    if (!GetTempPath(dirPath->Length, dirPath->Buffer))
    {
        PhDereferenceObject(dirPath);
        return NULL;
    }

    return dirPath;
}


PPH_STRING PhGetUrlBaseName(
    _In_ PPH_STRING FileName
    )
{
    PH_STRINGREF pathPart;
    PH_STRINGREF baseNamePart;

    if (!PhSplitStringRefAtLastChar(&FileName->sr, '/', &pathPart, &baseNamePart))
        return NULL;

    return PhCreateString2(&baseNamePart);
}


PPH_STRING BrowseForFolder(
    _In_opt_ HWND DialogHandle,
    _In_opt_ PCWSTR Title
    )
{
    PPH_STRING folderPath = NULL;

    if (WINDOWS_HAS_IFILEDIALOG)
    {
        PVOID fileDialog;

        fileDialog = PhCreateOpenFileDialog();

        PhSetFileDialogOptions(fileDialog, PH_FILEDIALOG_PICKFOLDERS);

        if (PhShowFileDialog(DialogHandle, fileDialog))
        {
            PPH_STRING folderPath;
            PPH_STRING fileDialogFolderPath = PhGetFileDialogFileName(fileDialog);

            folderPath = PhCreateStringEx(fileDialogFolderPath->Buffer, fileDialogFolderPath->Length * 2);

            // Ensure the folder path ends with a slash
            // We must make sure the install path ends with a backslash since
            // this string is wcscat' with our zip extraction paths.
            PathAddBackslash(folderPath->Buffer);

            PhTrimToNullTerminatorString(folderPath);

            PhFreeFileDialog(fileDialog);

            return folderPath;
        }
    }
    else
    {
        PIDLIST_ABSOLUTE shellItemId;
        BROWSEINFO browseInformation;

        memset(&browseInformation, 0, sizeof(BROWSEINFO));

        browseInformation.hwndOwner = DialogHandle;
        browseInformation.lpszTitle = Title;
        browseInformation.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;  // Caller needs to call OleInitialize() before using BIF_NEWDIALOGSTYLE?

        if (shellItemId = SHBrowseForFolder(&browseInformation))
        {
            folderPath = PhCreateStringEx(NULL, MAX_PATH * 2);

            if (SHGetPathFromIDList(shellItemId, folderPath->Buffer))
            {
                // Ensure the folder path ends with a slash
                // We must make sure the install path ends with a backslash since
                // this string is wcscat' with our zip extraction paths.
                PathAddBackslash(folderPath->Buffer);

                PhTrimToNullTerminatorString(folderPath);
            }
            else
            {
                PhClearReference(&folderPath);
            }

            CoTaskMemFree(shellItemId);
        }
    }

    return folderPath;
}

VOID StopwatchInitialize(
    __out PSTOPWATCH Stopwatch
    )
{
    Stopwatch->StartCounter.QuadPart = 0;
    Stopwatch->EndCounter.QuadPart = 0;
}

VOID StopwatchStart(
    _Inout_ PSTOPWATCH Stopwatch
    )
{
    QueryPerformanceCounter(&Stopwatch->StartCounter);
    QueryPerformanceFrequency(&Stopwatch->Frequency);
}

ULONG StopwatchGetMilliseconds(
    _In_ PSTOPWATCH Stopwatch
    )
{
#define CLOCKS_PER_SEC  1000

    LARGE_INTEGER countsPerMs;

    countsPerMs = Stopwatch->Frequency;
    countsPerMs.QuadPart /= CLOCKS_PER_SEC;

    QueryPerformanceCounter(&Stopwatch->EndCounter);

    return (ULONG)((Stopwatch->EndCounter.QuadPart - Stopwatch->StartCounter.QuadPart) / countsPerMs.QuadPart);
}

BOOLEAN ConnectionAvailable(VOID)
{
    if (WindowsVersion > WINDOWS_VISTA)
    {
        INetworkListManager* pNetworkListManager = NULL;

        // Create an instance of the INetworkListManger COM object.
        if (SUCCEEDED(CoCreateInstance(&CLSID_NetworkListManager, NULL, CLSCTX_ALL, &IID_INetworkListManager, &pNetworkListManager)))
        {
            VARIANT_BOOL isConnected = VARIANT_FALSE;
            VARIANT_BOOL isConnectedInternet = VARIANT_FALSE;

            // Query the relevant properties.
            INetworkListManager_get_IsConnected(pNetworkListManager, &isConnected);
            INetworkListManager_get_IsConnectedToInternet(pNetworkListManager, &isConnectedInternet);

            // Cleanup the INetworkListManger COM objects.
            INetworkListManager_Release(pNetworkListManager);

            // Check if Windows is connected to a network and it's connected to the internet.
            if (isConnected == VARIANT_TRUE && isConnectedInternet == VARIANT_TRUE)
                return TRUE;

            // We're not connected to anything
            //return FALSE;
        }
    }

    typedef BOOL(WINAPI *_InternetGetConnectedState)(
        __out PULONG lpdwFlags,
        __reserved ULONG dwReserved
        );

    BOOLEAN isSuccess = FALSE;
    ULONG wininetState = 0;
    _InternetGetConnectedState InternetGetConnectedState_I = NULL;
    HMODULE inetHandle = NULL;

    __try
    {
        if (!(inetHandle = LoadLibrary(L"wininet.dll")))
            __leave;

        InternetGetConnectedState_I = (_InternetGetConnectedState)GetProcAddress(inetHandle, "InternetGetConnectedState");
        if (!InternetGetConnectedState_I)
            __leave;

        if (InternetGetConnectedState_I(&wininetState, 0))
            isSuccess = TRUE;
    }
    __finally
    {
        if (inetHandle)
        {
            FreeLibrary(inetHandle);
        }
    }

    return isSuccess;
}

BOOLEAN CreateLink(
    _In_ PWSTR DestFilePath,
    _In_ PWSTR FilePath,
    _In_ PWSTR FileParentDir,
    _In_ PWSTR FileComment
    )
{
    IShellLink* shellLinkPtr = NULL;
    IPersistFile* persistFilePtr = NULL;
    BOOLEAN isSuccess = FALSE;

    __try
    {
        if (FAILED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, &shellLinkPtr)))
            __leave;
        if (FAILED(IShellLinkW_QueryInterface(shellLinkPtr, &IID_IPersistFile, &persistFilePtr)))
            __leave;

        // Load existing shell item if it exists...
        //if (SUCCEEDED(IPersistFile_Load(persistFilePtr, DestFilePath, STGM_READ)))
        //{
        //    IShellLinkW_Resolve(shellLinkPtr, NULL, 0);
        //}

        IShellLinkW_SetDescription(shellLinkPtr, FileComment);
        IShellLinkW_SetWorkingDirectory(shellLinkPtr, FileParentDir);
        IShellLinkW_SetIconLocation(shellLinkPtr, FilePath, 0);

        // Set the shortcut target path...
        if (FAILED(IShellLinkW_SetPath(shellLinkPtr, FilePath)))
             __leave;

        // Save the shortcut to the file system...
        if (FAILED(IPersistFile_Save(persistFilePtr, DestFilePath, TRUE)))
            __leave;

        isSuccess = TRUE;
    }
    __finally
    {
        if (persistFilePtr)
        {
            IPersistFile_Release(persistFilePtr);
        }

        if (shellLinkPtr)
        {
            IShellLinkW_Release(shellLinkPtr);
        }
    }

    return isSuccess;
}

PWSTR XmlParseToken(
    _In_ PWSTR XmlString,
    _In_ PWSTR XmlTokenName
    )
{
    PWSTR xmlTokenNext = NULL;
    PWSTR xmlStringData;
    PWSTR xmlTokenString;

    xmlStringData = PhDuplicateStringZ(XmlString);

    xmlTokenString = wcstok_s(
        xmlStringData,
        L"<>",
        &xmlTokenNext
        );

    while (xmlTokenString)
    {
        if (PhEqualStringZ(xmlTokenString, XmlTokenName, TRUE))
        {
            // We found the value.
            xmlTokenString = wcstok_s(NULL, L"<>", &xmlTokenNext);
            break;
        }

        xmlTokenString = wcstok_s(NULL, L"<>", &xmlTokenNext);
    }

    if (xmlTokenString)
    {
        PWSTR xmlStringDup = PhDuplicateStringZ(xmlTokenString);

        PhFree(xmlStringData);

        return xmlStringDup;
    }

    PhFree(xmlStringData);
    return NULL;
}


BOOLEAN DialogPromptExit(
    _In_ HWND hwndDlg
    )
{
    if (WindowsVersion > WINDOWS_VISTA && TaskDialogIndirect)
    {
        INT nButtonPressed = 0;

        TASKDIALOGCONFIG tdConfig = { sizeof(TASKDIALOGCONFIG) };
        tdConfig.hwndParent = hwndDlg;
        tdConfig.hInstance = PhLibImageBase;
        tdConfig.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
        tdConfig.nDefaultButton = IDNO;
        tdConfig.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
        tdConfig.pszMainIcon = TD_WARNING_ICON;
        tdConfig.pszMainInstruction = L"Exit Setup";
        tdConfig.pszWindowTitle = PhApplicationName;
        tdConfig.pszContent = L"Are you sure you want to cancel the Setup?";

        TaskDialogIndirect(&tdConfig, &nButtonPressed, NULL, NULL);

        return nButtonPressed == IDNO;
    }
    else
    {
        MSGBOXPARAMS mbParams = { sizeof(MSGBOXPARAMS) };
        mbParams.hwndOwner = hwndDlg;
        mbParams.hInstance = PhLibImageBase;
        mbParams.lpszText = L"Are you sure you want to cancel the Setup?";
        mbParams.lpszCaption = PhApplicationName;
        mbParams.dwStyle = MB_YESNO | MB_ICONEXCLAMATION;
        // | MB_USERICON;
        //params.lpszIcon = MAKEINTRESOURCE(IDI_ICON1);

        return MessageBoxIndirect(&mbParams) == IDNO;
    }
}

VOID DialogPromptsProcessHackerIsRunning(
    _In_ HWND hwndDlg
    )
{
    if (WindowsVersion > WINDOWS_VISTA && TaskDialogIndirect) // We're on Vista or above.
    {
        INT nButtonPressed = 0;

        TASKDIALOGCONFIG tdConfig = { sizeof(TASKDIALOGCONFIG) };
        tdConfig.hwndParent = hwndDlg;
        tdConfig.hInstance = PhLibImageBase;
        tdConfig.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
        tdConfig.nDefaultButton = IDOK;
        tdConfig.dwCommonButtons = TDCBF_OK_BUTTON;
        tdConfig.pszMainIcon = TD_INFORMATION_ICON;
        tdConfig.pszMainInstruction = L"Please close Process Hacker before continuing.";
        tdConfig.pszWindowTitle = PhApplicationName;
        //tdConfig.pszContent = L"Please close Process Hacker before continuing.";

        TaskDialogIndirect(&tdConfig, &nButtonPressed, NULL, NULL);
    }
    else
    {
        MSGBOXPARAMS mbParams = { sizeof(MSGBOXPARAMS) };
        mbParams.hwndOwner = hwndDlg;
        mbParams.hInstance = PhLibImageBase;
        mbParams.lpszText = L"Please close Process Hacker before continuing.";
        mbParams.lpszCaption = PhApplicationName;
        mbParams.dwStyle = MB_OK; // | MB_USERICON;
        //params.lpszIcon = MAKEINTRESOURCE(IDI_ICON1);

        MessageBoxIndirect(&mbParams);
    }
}

_Check_return_
BOOLEAN IsProcessHackerRunning(
    VOID
    )
{
    HANDLE mutantHandle;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING mutantName;

    RtlInitUnicodeString(&mutantName, L"\\BaseNamedObjects\\ProcessHacker2Mutant");
    InitializeObjectAttributes(
        &oa,
        &mutantName,
        0,
        NULL,
        NULL
        );

    if (NT_SUCCESS(NtOpenMutant(
        &mutantHandle,
        MUTANT_QUERY_STATE,
        &oa
        )))
    {
        NtClose(mutantHandle);
        return TRUE;
    }

    return FALSE;
}

_Check_return_
BOOLEAN IsProcessHackerInstalledUsingSetup(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");

    HANDLE keyHandle;

    // Check uninstall entries for the 'Process_Hacker2_is1' registry key.
    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        NtClose(keyHandle);
        return TRUE;
    }

    return FALSE;
}

_Check_return_
BOOLEAN IsProcessHackerInstalled(VOID)
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");
    BOOLEAN keySuccess = FALSE;
    HANDLE keyHandle;
    PPH_STRING installPath = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | KEY_WOW64_64KEY, // 64bit key
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        installPath = PhQueryRegistryString(keyHandle, L"InstallLocation");
        NtClose(keyHandle);
    }

    if (!PhEndsWithString2(installPath, L"ProcessHacker.exe", TRUE))
    {

    }

    // Check if KeyData value maps to valid file path.
    if (GetFileAttributes(installPath->Buffer) == INVALID_FILE_ATTRIBUTES)
    {

    }

    keySuccess = TRUE;

    return keySuccess;
}

_Maybenull_
PPH_STRING GetProcessHackerInstallPath(
    VOID
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Process_Hacker2_is1");

    HANDLE keyHandle;
    PPH_STRING installPath = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        )))
    {
        installPath = PhQueryRegistryString(keyHandle, L"InstallLocation");
        NtClose(keyHandle);
    }

    return installPath;
}

_Check_return_
BOOLEAN ProcessHackerShutdown(
    VOID
    )
{
    HWND WindowHandle;

    WindowHandle = FindWindow(L"ProcessHacker", NULL);

    if (WindowHandle)
    {
        HANDLE processHandle;
        ULONG processID = 0;
        ULONG threadID = GetWindowThreadProcessId(WindowHandle, &processID);

        SendMessageTimeout(WindowHandle, WM_QUIT, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 5000, NULL);

        if (NT_SUCCESS(PhOpenProcess(&processHandle, SYNCHRONIZE | PROCESS_TERMINATE, ULongToHandle(processID))))
        {
            //PostMessage(WindowHandle, WM_QUIT, 0, 0);
            // Wait on the process handle, if we timeout, kill it.
            //if (WaitForSingleObject(processHandle, 10000) != WAIT_OBJECT_0)

            NtTerminateProcess(processHandle, 1);
            NtClose(processHandle);
        }
    }

    return FALSE;
}

_Check_return_
ULONG UninstallKph(
    _In_ BOOLEAN Kph2Uninstall
    )
{
    ULONG status = ERROR_SUCCESS;
    SC_HANDLE scmHandle;
    SC_HANDLE serviceHandle;

    if (!(scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT)))
        return PhGetLastWin32ErrorAsNtStatus();

    if (serviceHandle = OpenService(
        scmHandle, 
        Kph2Uninstall ? L"KProcessHacker2" : L"KProcessHacker3", 
        SERVICE_STOP | DELETE
        ))
    {
        SERVICE_STATUS serviceStatus;

        ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus);

        if (!DeleteService(serviceHandle))
        {
            status = GetLastError();
        }

        CloseServiceHandle(serviceHandle);
    }
    else
    {
        status = GetLastError();
    }

    CloseServiceHandle(scmHandle);

    return status;
}


static VOID RemoveAppCompatEntry(
    _In_ HANDLE ParentKey
    )
{
    static PH_STRINGREF keyName = PH_STRINGREF_INIT(L"ProcessHacker.exe");
    ULONG bufferLength;
    KEY_FULL_INFORMATION fullInfo;

    memset(&fullInfo, 0, sizeof(KEY_FULL_INFORMATION));

    if (!NT_SUCCESS(NtQueryKey(
        ParentKey,
        KeyFullInformation,
        &fullInfo,
        sizeof(KEY_FULL_INFORMATION),
        &bufferLength
        )))
    {
        return;
    }

    for (ULONG i = 0; i < fullInfo.Values; i++)
    {
        PPH_STRING value;
        PKEY_VALUE_FULL_INFORMATION buffer;

        bufferLength = sizeof(KEY_VALUE_FULL_INFORMATION);
        buffer = PhAllocate(bufferLength);
        memset(buffer, 0, bufferLength);

        if (NT_SUCCESS(NtEnumerateValueKey(
            ParentKey,
            i,
            KeyValueFullInformation,
            buffer,
            bufferLength,
            &bufferLength
            )))
        {
            PhFree(buffer);
            break;
        }

        //bufferLength = bufferLength;
        buffer = PhReAllocate(buffer, bufferLength);
        memset(buffer, 0, bufferLength);

        if (!NT_SUCCESS(NtEnumerateValueKey(
            ParentKey,
            i,
            KeyValueFullInformation,
            buffer,
            bufferLength,
            &bufferLength
            )))
        {
            PhFree(buffer);
            break;
        }

        if (value = PhCreateStringEx(buffer->Name, buffer->NameLength))
        {
            UNICODE_STRING us;

            PhStringRefToUnicodeString(&value->sr, &us);

            if (PhEndsWithStringRef(&value->sr, &keyName, TRUE))
            {
                NtDeleteValueKey(ParentKey, &us);
            }

            PhDereferenceObject(value);
        }

        PhFree(buffer);
    }
}

_Check_return_
BOOLEAN RemoveAppCompatEntries(
    VOID
    )
{
    static PH_STRINGREF appCompatLayersName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers");
    static PH_STRINGREF appCompatPersistedName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Compatibility Assistant\\Persisted");
    HANDLE keyHandle;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        PH_KEY_CURRENT_USER,
        &appCompatLayersName,
        0
        )))
    {
        RemoveAppCompatEntry(keyHandle);
        NtClose(keyHandle);
    }

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        PH_KEY_CURRENT_USER,
        &appCompatPersistedName,
        0
        )))
    {
        RemoveAppCompatEntry(keyHandle);
        NtClose(keyHandle);
    }

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &appCompatLayersName,
        0
        )))
    {
        RemoveAppCompatEntry(keyHandle);
        NtClose(keyHandle);
    }

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &appCompatPersistedName,
        0
        )))
    {
        RemoveAppCompatEntry(keyHandle);
        NtClose(keyHandle);
    }

    return TRUE;
}




//PPH_STRING FileGetParentDir(
//    _In_ PWSTR FilePath
//    )
//{
//    ULONG index = 0;
//    PCWSTR offset = _tcsrchr(FilePath, TEXT('\\'));
//
//    if (offset == NULL)
//        offset = _tcsrchr(FilePath, TEXT('/'));
//
//    index = offset - FilePath;
//
//    return PhCreateStringEx(FilePath, index * sizeof(TCHAR));
//}

//BOOLEAN FileMakeDirPathRecurse(
//    _In_ PWSTR DirPath
//    )
//{
//    BOOLEAN isSuccess = FALSE;
//    PPH_STRING dirPathString = NULL;
//    PWSTR dirTokenNext = NULL;
//    PWSTR dirTokenDup = NULL;
//    PWSTR dirTokenString = NULL;
//
//    __try
//    {
//        if ((dirTokenDup = PhDuplicateStringZ(DirPath)) == NULL)
//            __leave;
//
//        // Find the first directory path token...
//        if ((dirTokenString = _tcstok_s(dirTokenDup, TEXT("\\"), &dirTokenNext)) == NULL)
//            __leave;
//
//        while (dirTokenString)
//        {
//            if (dirPathString)
//            {
//                // Copy the new folder path to the previous folder path...
//                PPH_STRING tempPathString = StringFormat(TEXT("%s\\%s"),
//                    dirPathString->Buffer,
//                    dirTokenString
//                    );
//
//                if (!FileExists(tempPathString->Buffer))
//                {
//                    if (_tmkdir(tempPathString->Buffer) != 0)
//                    {
//                        DEBUG_MSG(TEXT("ERROR: _tmkdir (%u)\n"), _doserrno);
//                        PhFree(tempPathString);
//                        __leave;
//                    }
//                    else
//                    {
//                        DEBUG_MSG(TEXT("CreateDir: %s\n"), tempPathString->Buffer);
//                    }
//                }
//
//                PhFree(dirPathString);
//                dirPathString = tempPathString;
//            }
//            else
//            {
//                // Copy the drive letter and root folder...
//                dirPathString  = StringFormat(TEXT("%s"), dirTokenString);
//            }
//
//            // Find the next directory path token...
//            dirTokenString = _tcstok_s(NULL, TEXT("\\"), &dirTokenNext);
//        }
//
//        isSuccess = TRUE;
//    }
//    __finally
//    {
//        if (dirPathString)
//        {
//            PhFree(dirPathString);
//        }
//
//        if (dirTokenDup)
//        {
//            PhFree(dirTokenDup);
//        }
//    }
//
//    return isSuccess;
//}

//BOOLEAN FileRemoveDirPathRecurse(
//    _In_ PWSTR DirPath
//    )
//{
//    struct _tfinddata_t findData;
//    intptr_t findHandle = (intptr_t)INVALID_HANDLE_VALUE;
//    PPH_STRING dirPath = NULL;
//
//    dirPath = StringFormat(TEXT("%s\\*.*"), DirPath);
//
//    // Find the first file...
//    if ((findHandle = _tfindfirst(dirPath->Buffer, &findData)) == (intptr_t)INVALID_HANDLE_VALUE)
//    {
//        if (errno == ENOENT) // ERROR_FILE_NOT_FOUND
//        {
//            PhFree(dirPath);
//            return TRUE;
//        }
//
//        DEBUG_MSG(TEXT("_tfindfirst: (%u) %s\n"), _doserrno, dirPath->Buffer);
//        PhFree(dirPath);
//        return FALSE;
//    }
//
//    do
//    {
//        // Check for "." and ".."
//        if (!_tcsicmp(findData.name, TEXT(".")) || !_tcsicmp(findData.name, TEXT("..")))
//            continue;
//
//        if (findData.attrib & _A_SUBDIR)
//        {
//            PPH_STRING subDirPath = StringFormat(
//                TEXT("%s\\%s"),
//                DirPath,
//                findData.name
//                );
//
//            // Found a directory...
//            if (!FileRemoveDirPathRecurse(subDirPath->Buffer))
//            {
//                PhFree(subDirPath);
//                _findclose(findHandle);
//                return FALSE;
//            }
//
//            PhFree(subDirPath);
//        }
//        else
//        {
//            PPH_STRING subDirPath = StringFormat(
//                TEXT("%s\\%s"),
//                DirPath,
//                findData.name
//                );
//
//            if (findData.attrib & _A_RDONLY)
//            {
//                if (_tchmod(subDirPath->Buffer, _S_IWRITE) == 0)
//                {
//                    DEBUG_MSG(TEXT("_tchmod: %s\n"), subDirPath->Buffer);
//                }
//                else
//                {
//                    DEBUG_MSG(TEXT("ERROR _tchmod: (%u) %s\n"), _doserrno, subDirPath->Buffer);
//                }
//            }
//
//            if (_tremove(subDirPath->Buffer) == 0)
//            {
//                DEBUG_MSG(TEXT("DeleteFile: %s\n"), subDirPath->Buffer);
//            }
//            else
//            {
//                DEBUG_MSG(TEXT("ERROR DeleteFile: (%u) %s\n"), _doserrno, subDirPath->Buffer);
//            }
//
//            PhFree(subDirPath);
//        }
//
//        // Find the next entry
//    } while (_tfindnext(findHandle, &findData) == 0);
//
//    // Close the file handle
//    _findclose(findHandle);
//
//    // Check for write permission (read-only attribute ignored on directories?)
//    //if (_taccess(DirPath, 2) == -1)
//    //{
//    //    if (_tchmod(DirPath, _S_IWRITE) == 0)
//    //    {
//    //        DEBUG_MSG(TEXT("_tchmod: %s\n"), DirPath);
//    //    }
//    //    else
//    //    {
//    //        DEBUG_MSG(TEXT("ERROR _tchmod: (%u) %s\n"), _doserrno, DirPath);
//    //    }
//    //}
//
//    // Lastly delete the parent directory
//    if (_trmdir(DirPath) == 0)
//    {
//        DEBUG_MSG(TEXT("DeleteDirectory: %s\n"), DirPath);
//    }
//    else
//    {
//        DEBUG_MSG(TEXT("ERROR DeleteDirectory: (%u) %s\n"), _doserrno, DirPath);
//    }
//
//    PhFree(dirPath);
//    return TRUE;
//}


BOOLEAN CreateDirectoryPath(
    _In_ PPH_STRING DirectoryPath
    )
{
    BOOLEAN success = FALSE;
    BOOLEAN directoryExists = FALSE;
    FILE_NETWORK_OPEN_INFORMATION directoryInfo;

    if (NT_SUCCESS(PhQueryFullAttributesFileWin32(DirectoryPath->Buffer, &directoryInfo)))
    {
        if (directoryInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            directoryExists = TRUE;
        }
    }

    if (!directoryExists)
    {
        INT errorCode = SHCreateDirectoryEx(NULL, DirectoryPath->Buffer, NULL);

        if (errorCode == ERROR_SUCCESS)
        {
            DEBUG_MSG(L"Created Directory: %s\r\n", DirectoryPath->Buffer);
            success = TRUE;
        }
        else
        {
            DEBUG_MSG(L"SHCreateDirectoryEx Failed\r\n");
        }
    }
    else
    {
        //DEBUG_MSG(L"Directory Exists: %s\r\n", DirectoryPath->Buffer);
        success = TRUE;
    }

    return success;
}