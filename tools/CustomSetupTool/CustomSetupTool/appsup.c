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
#include <io.h>
#include <Netlistmgr.h>

VOID ExtractResourceToFile(
    _In_ PWSTR Resource, 
    _In_ PWSTR FileName
    )
{
    HANDLE fileHandle = NULL;
    ULONG resourceLength;
    HRSRC resourceHandle = NULL;
    HGLOBAL resourceData;
    PVOID resourceBuffer;
    IO_STATUS_BLOCK isb;

    if (!(resourceHandle = FindResource(PhLibImageBase, Resource, RT_RCDATA)))
        goto CleanupExit;

    resourceLength = SizeofResource(PhLibImageBase, resourceHandle);

    if (!(resourceData = LoadResource(PhLibImageBase, resourceHandle)))
        goto CleanupExit;

    if (!(resourceBuffer = LockResource(resourceData)))
        goto CleanupExit;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        FileName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OVERWRITE_IF,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        goto CleanupExit;
    }

    if (!NT_SUCCESS(NtWriteFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        resourceBuffer,
        resourceLength,
        NULL,
        NULL
        )))
    {
        goto CleanupExit;
    }

    if (isb.Information != resourceLength)
        goto CleanupExit;

CleanupExit:

    if (fileHandle)
        NtClose(fileHandle);

    if (resourceHandle)
        FreeResource(resourceHandle);
}

PVOID ExtractResourceToBuffer(
    _In_ PWSTR Resource
    )
{
    ULONG resourceLength;
    HRSRC resourceHandle = NULL;
    HGLOBAL resourceData;
    PVOID resourceBuffer;
    PVOID buffer = NULL;

    if (!(resourceHandle = FindResource(PhLibImageBase, Resource, RT_RCDATA)))
        goto CleanupExit;

    resourceLength = SizeofResource(PhLibImageBase, resourceHandle);

    if (!(resourceData = LoadResource(PhLibImageBase, resourceHandle)))
        goto CleanupExit;

    if (!(resourceBuffer = LockResource(resourceData)))
        goto CleanupExit;

    if (!(buffer = PhAllocate(resourceLength)))
        goto CleanupExit;

    memcpy(buffer, resourceBuffer, resourceLength);

CleanupExit:

    if (resourceHandle)
        FreeResource(resourceHandle);

    return buffer;
}

HBITMAP LoadPngImageFromResources(
    _In_ PCWSTR Name
    )
{
    BOOLEAN success = FALSE;
    UINT frameCount = 0;
    ULONG resourceLength = 0;
    HGLOBAL resourceHandle = NULL;
    HRSRC resourceHandleSource = NULL;
    WICInProcPointer resourceBuffer = NULL;
    HDC screenHdc = NULL;
    HDC bufferDc = NULL;
    BITMAPINFO bitmapInfo = { 0 };
    HBITMAP bitmapHandle = NULL;
    PVOID bitmapBuffer = NULL;
    IWICStream* wicStream = NULL;
    IWICBitmapSource* wicBitmapSource = NULL;
    IWICBitmapDecoder* wicDecoder = NULL;
    IWICBitmapFrameDecode* wicFrame = NULL;
    IWICImagingFactory* wicFactory = NULL;
    IWICBitmapScaler* wicScaler = NULL;
    WICPixelFormatGUID pixelFormat;
    WICRect rect = { 0, 0, 164, 164 };

    // Create the ImagingFactory
    if (FAILED(CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &wicFactory)))
        goto CleanupExit;

    // Find the resource
    if ((resourceHandleSource = FindResource(PhLibImageBase, Name, L"PNG")) == NULL)
        goto CleanupExit;

    // Get the resource length
    resourceLength = SizeofResource(PhLibImageBase, resourceHandleSource);

    // Load the resource
    if ((resourceHandle = LoadResource(PhLibImageBase, resourceHandleSource)) == NULL)
        goto CleanupExit;

    if ((resourceBuffer = (WICInProcPointer)LockResource(resourceHandle)) == NULL)
        goto CleanupExit;

    // Create the Stream
    if (FAILED(IWICImagingFactory_CreateStream(wicFactory, &wicStream)))
        goto CleanupExit;

    // Initialize the Stream from Memory
    if (FAILED(IWICStream_InitializeFromMemory(wicStream, resourceBuffer, resourceLength)))
        goto CleanupExit;

    if (FAILED(IWICImagingFactory_CreateDecoder(wicFactory, &GUID_ContainerFormatPng, NULL, &wicDecoder)))
        goto CleanupExit;

    if (FAILED(IWICBitmapDecoder_Initialize(wicDecoder, (IStream*)wicStream, WICDecodeMetadataCacheOnLoad)))
        goto CleanupExit;

    // Get the Frame count
    if (FAILED(IWICBitmapDecoder_GetFrameCount(wicDecoder, &frameCount)) || frameCount < 1)
        goto CleanupExit;

    // Get the Frame
    if (FAILED(IWICBitmapDecoder_GetFrame(wicDecoder, 0, &wicFrame)))
        goto CleanupExit;

    // Get the WicFrame image format
    if (FAILED(IWICBitmapFrameDecode_GetPixelFormat(wicFrame, &pixelFormat)))
        goto CleanupExit;

    // Check if the image format is supported:
    if (IsEqualGUID(&pixelFormat, &GUID_WICPixelFormat32bppPRGBA))
    {
        wicBitmapSource = (IWICBitmapSource*)wicFrame;
    }
    else
    {
        IWICFormatConverter* wicFormatConverter = NULL;

        if (FAILED(IWICImagingFactory_CreateFormatConverter(wicFactory, &wicFormatConverter)))
            goto CleanupExit;

        if (FAILED(IWICFormatConverter_Initialize(
            wicFormatConverter,
            (IWICBitmapSource*)wicFrame,
            &GUID_WICPixelFormat32bppPRGBA,
            WICBitmapDitherTypeNone,
            NULL,
            0.0,
            WICBitmapPaletteTypeCustom
            )))
        {
            IWICFormatConverter_Release(wicFormatConverter);
            goto CleanupExit;
        }

        // Convert the image to the correct format:
        IWICFormatConverter_QueryInterface(wicFormatConverter, &IID_IWICBitmapSource, &wicBitmapSource);

        // Cleanup the converter.
        IWICFormatConverter_Release(wicFormatConverter);

        // Dispose the old frame now that the converted frame is in wicBitmapSource.
        IWICBitmapFrameDecode_Release(wicFrame);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = rect.Width;
    bitmapInfo.bmiHeader.biHeight = -((LONG)rect.Height);
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    screenHdc = CreateIC(L"DISPLAY", NULL, NULL, NULL);
    bufferDc = CreateCompatibleDC(screenHdc);
    bitmapHandle = CreateDIBSection(screenHdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBuffer, NULL, 0);

    // Check if it's the same rect as the requested size.
    //if (width != rect.Width || height != rect.Height)
    if (FAILED(IWICImagingFactory_CreateBitmapScaler(wicFactory, &wicScaler)))
        goto CleanupExit;
    if (FAILED(IWICBitmapScaler_Initialize(wicScaler, wicBitmapSource, rect.Width, rect.Height, WICBitmapInterpolationModeFant)))
        goto CleanupExit;
    if (FAILED(IWICBitmapScaler_CopyPixels(wicScaler, &rect, rect.Width * 4, rect.Width * rect.Height * 4, (PBYTE)bitmapBuffer)))
        goto CleanupExit;

    success = TRUE;

CleanupExit:

    if (wicScaler)
        IWICBitmapScaler_Release(wicScaler);

    if (bufferDc)
        DeleteDC(bufferDc);

    if (screenHdc)
        DeleteDC(screenHdc);

    if (wicBitmapSource)
        IWICBitmapSource_Release(wicBitmapSource);

    if (wicStream)
        IWICStream_Release(wicStream);

    if (wicDecoder)
        IWICBitmapDecoder_Release(wicDecoder);

    if (wicFactory)
        IWICImagingFactory_Release(wicFactory);

    if (resourceHandle)
        FreeResource(resourceHandle);

    if (success)
    {
        return bitmapHandle;
    }

    DeleteObject(bitmapHandle);
    return NULL;
}

VOID SetupLoadImage(
    _In_ HWND WindowHandle, 
    _In_ PWSTR Name
    )
{
    HBITMAP imageBitmap;
    
    if (imageBitmap = LoadPngImageFromResources(Name))
    {
        // Remove the frame and apply the bitmap
        PhSetWindowStyle(WindowHandle, SS_BITMAP | SS_BLACKFRAME, SS_BITMAP);

        SendMessage(WindowHandle, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)imageBitmap);

        DeleteObject(imageBitmap);
    }
}

VOID SetupInitializeFont(
    _In_ HWND ControlHandle, 
    _In_ LONG Height, 
    _In_ LONG Weight
    )
{
    NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);

    metrics.lfMessageFont.lfHeight = -PhMultiplyDivideSigned(Height, PhGlobalDpi, 72);
    metrics.lfMessageFont.lfWeight = Weight;
    //metrics.lfMessageFont.lfQuality = CLEARTYPE_QUALITY;

    HFONT fontHandle = CreateFontIndirect(&metrics.lfMessageFont);
    SendMessage(ControlHandle, WM_SETFONT, (WPARAM)fontHandle, FALSE);
    //DeleteFont(fontHandle);
}

BOOLEAN ConnectionAvailable(VOID)
{
    INetworkListManager* networkListManager = NULL;

    if (SUCCEEDED(CoCreateInstance(
        &CLSID_NetworkListManager, 
        NULL, 
        CLSCTX_ALL, 
        &IID_INetworkListManager, 
        &networkListManager
        )))
    {
        VARIANT_BOOL isConnected = VARIANT_FALSE;
        VARIANT_BOOL isConnectedInternet = VARIANT_FALSE;

        // Query the relevant properties.
        INetworkListManager_get_IsConnected(networkListManager, &isConnected);
        INetworkListManager_get_IsConnectedToInternet(networkListManager, &isConnectedInternet);

        // Cleanup the INetworkListManger COM objects.
        INetworkListManager_Release(networkListManager);

        // Check if Windows is connected to a network and it's connected to the internet.
        if (isConnected == VARIANT_TRUE && isConnectedInternet == VARIANT_TRUE)
            return TRUE;

        // We're not connected to anything
    }

    return FALSE;
}

VOID SetupCreateLink(
    _In_ PWSTR LinkFilePath, 
    _In_ PWSTR FilePath,
    _In_ PWSTR FileParentDir
    )
{
    IShellLink* shellLinkPtr = NULL;
    IPersistFile* persistFilePtr = NULL;

    if (FAILED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLink, &shellLinkPtr)))
        goto CleanupExit;

    if (FAILED(IShellLinkW_QueryInterface(shellLinkPtr, &IID_IPersistFile, &persistFilePtr)))
        goto CleanupExit;

    // Load existing shell item if it exists...
    //IPersistFile_Load(persistFilePtr, LinkFilePath, STGM_READ)
    //IShellLinkW_SetDescription(shellLinkPtr, FileComment);
    IShellLinkW_SetWorkingDirectory(shellLinkPtr, FileParentDir);
    IShellLinkW_SetIconLocation(shellLinkPtr, FilePath, 0);

    // Set the shortcut target path...
    if (FAILED(IShellLinkW_SetPath(shellLinkPtr, FilePath)))
        goto CleanupExit;

    // Save the shortcut to the file system...
    IPersistFile_Save(persistFilePtr, LinkFilePath, TRUE);

CleanupExit:
    if (persistFilePtr)
        IPersistFile_Release(persistFilePtr);
    if (shellLinkPtr)
        IShellLinkW_Release(shellLinkPtr);
}

BOOLEAN DialogPromptExit(
    _In_ HWND hwndDlg
    )
{
    INT buttonPressed = 0;
    TASKDIALOGCONFIG config = { sizeof(TASKDIALOGCONFIG) };
    config.hwndParent = hwndDlg;
    config.hInstance = PhLibImageBase;
    config.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
    config.nDefaultButton = IDNO;
    config.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
    config.pszMainIcon = TD_WARNING_ICON;
    config.pszMainInstruction = L"Exit Setup";
    config.pszWindowTitle = PhApplicationName;
    config.pszContent = L"Are you sure you want to cancel the Setup?";

    TaskDialogIndirect(&config, &buttonPressed, NULL, NULL);

    return buttonPressed == IDNO;
}

BOOLEAN CheckProcessHackerRunning(VOID)
{
    HANDLE mutantHandle;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING mutantName;

    RtlInitUnicodeString(&mutantName, L"\\BaseNamedObjects\\ProcessHackerMutant");
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

BOOLEAN CheckProcessHackerInstalled(VOID)
{
    BOOLEAN installed = FALSE;
    PPH_STRING installPath;
    
    installPath = GetProcessHackerInstallPath();

    if (!PhIsNullOrEmptyString(installPath) && PhEndsWithString2(installPath, L"ProcessHacker.exe", TRUE))
    {
        // Check if the value has a valid file path.
        installed = GetFileAttributes(installPath->Buffer) != INVALID_FILE_ATTRIBUTES;
    }

    PhClearReference(&installPath);

    return installed;
}

PPH_STRING GetProcessHackerInstallPath(VOID)
{
    HANDLE keyHandle;
    PPH_STRING installPath = NULL;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ | KEY_WOW64_64KEY,
        PH_KEY_LOCAL_MACHINE,
        &UninstallKeyName,
        0
        )))
    {
        installPath = PhQueryRegistryString(keyHandle, L"InstallLocation");
        NtClose(keyHandle);
    }

    return installPath;
}

BOOLEAN ShutdownProcessHacker(VOID)
{
    HWND windowHandle;
    HANDLE processHandle;
    ULONG processID = 0;

    windowHandle = FindWindow(L"ProcessHacker", NULL);
    if (!windowHandle)
        return TRUE;

    GetWindowThreadProcessId(windowHandle, &processID);

    SendMessageTimeout(
        windowHandle,
        WM_QUIT,
        0,
        0,
        SMTO_ABORTIFHUNG | SMTO_BLOCK,
        5000,
        NULL
        );

    // Check the window handle again
    windowHandle = FindWindow(L"ProcessHacker", NULL);
    if (!windowHandle)
        return TRUE;

    GetWindowThreadProcessId(windowHandle, &processID);

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        SYNCHRONIZE | PROCESS_TERMINATE,
        ULongToHandle(processID)
        )))
    {
        PostMessage(windowHandle, WM_QUIT, 0, 0);

        if (WaitForSingleObject(processHandle, 10 * 1000) != WAIT_OBJECT_0)
        {
            if (!NT_SUCCESS(NtTerminateProcess(processHandle, 1)))
            {
                NtClose(processHandle);
                return FALSE;
            }
        }

        NtClose(processHandle);
    }

    return FALSE;
}

BOOLEAN CreateDirectoryPath(_In_ PWSTR DirPath)
{
    BOOLEAN success = FALSE;
    PPH_STRING dirPathString = NULL;
    PWSTR dirPathDup = NULL;

    if (RtlDoesFileExists_U(DirPath))
        return TRUE;

    if ((dirPathDup = PhDuplicateStringZ(DirPath)) == NULL)
        goto CleanupExit;

    for (PWSTR path = _wcstok(dirPathDup, L"\\"); path; path = _wcstok(NULL, L"\\"))
    {
        if (!dirPathString)
            dirPathString = PhCreateString(path);
        else
        {
            PPH_STRING tempPathString;

            tempPathString = PhConcatStrings(
                3,
                dirPathString->Buffer,
                L"\\",
                path
                );

            if (!RtlDoesFileExists_U(PhGetString(tempPathString)))
            {
                if (!CreateDirectory(PhGetString(tempPathString), NULL))
                {
                    PhDereferenceObject(tempPathString);
                    goto CleanupExit;
                }
            }

            PhSwapReference(&dirPathString, tempPathString);
            PhDereferenceObject(tempPathString);
        }
    }

    success = TRUE;

CleanupExit:

    if (dirPathString)
    {
        PhDereferenceObject(dirPathString);
    }

    if (dirPathDup)
    {
        PhFree(dirPathDup);
    }

    return success;
}

BOOLEAN RemoveDirectoryPath(_In_ PWSTR DirPath)
{
    HANDLE findHandle;
    PPH_STRING findPath;
    WIN32_FIND_DATA data = { 0 };

    findPath = PhConcatStrings2(DirPath, L"\\*");

    if ((findHandle = FindFirstFile(findPath->Buffer, &data)) == INVALID_HANDLE_VALUE)
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            PhDereferenceObject(findPath);
            return TRUE;
        }

        PhDereferenceObject(findPath);
        return FALSE;
    }

    do
    {
        if (PhEqualStringZ(data.cFileName, L".", TRUE) || PhEqualStringZ(data.cFileName, L"..", TRUE))
            continue;

        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            PPH_STRING dirPath = PhConcatStrings(3, DirPath, L"\\", data.cFileName);

            RemoveDirectoryPath(dirPath->Buffer);
            PhDereferenceObject(dirPath);
        }
        else
        {
            PPH_STRING filePath = PhConcatStrings(3, DirPath, L"\\", data.cFileName);

            if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                _wchmod(filePath->Buffer, _S_IWRITE);
            }

            SetupDeleteDirectoryFile(filePath->Buffer);
            PhDereferenceObject(filePath);
        }

    } while (FindNextFile(findHandle, &data));

    FindClose(findHandle);

    // Delete the parent directory
    SetupDeleteDirectoryFile(DirPath);

    PhDereferenceObject(findPath);
    return TRUE;
}

VOID SetupDeleteDirectoryFile(_In_ PWSTR FileName)
{
    HANDLE tempHandle;
    FILE_DISPOSITION_INFORMATION dispositionInfo;
    IO_STATUS_BLOCK isb;

    if (NT_SUCCESS(PhCreateFileWin32(
        &tempHandle,
        FileName,
        FILE_GENERIC_WRITE | DELETE,
        0,
        0,
        FILE_OVERWRITE_IF,
        FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        dispositionInfo.DeleteFile = TRUE;
        NtSetInformationFile(
            tempHandle,
            &isb,
            &dispositionInfo,
            sizeof(FILE_DISPOSITION_INFORMATION),
            FileDispositionInformation
            );
        NtClose(tempHandle);
    }
}