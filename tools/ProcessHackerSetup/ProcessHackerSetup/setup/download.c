#include <setup.h>
#include <appsup.h>
#include <netio.h>

PWSTR Version = NULL;
PWSTR DownloadURL = NULL;

BOOLEAN QueryBuildServerThread(
    _Inout_ P_HTTP_SESSION HttpSocket
    )
{
    BOOLEAN isSuccess = FALSE;
    PPH_STRING xmlStringData = NULL;

    __try
    {
        STATUS_MSG(L"Connecting to build server...\n");
        Sleep(500);

        if (!HttpConnect(HttpSocket, L"wj32.org", INTERNET_DEFAULT_HTTPS_PORT))
        {
            STATUS_MSG(L"HttpConnect: %u\n", GetLastError());
            __leave;
        }

        STATUS_MSG(L"Connected to build server...\n");
        Sleep(500);

        if (!HttpBeginRequest(HttpSocket, NULL, L"/processhacker/update.php", WINHTTP_FLAG_SECURE))
        {
            STATUS_MSG(L"HttpBeginRequest: %u\n", GetLastError());
            __leave;
        }

        if (!HttpAddRequestHeaders(HttpSocket, L"ProcessHacker-OsBuild: 0x0D06F00D"))
        {
            STATUS_MSG(TEXT("HttpAddRequestHeaders: %u\n"), GetLastError());
            __leave;
        }

        if (!HttpSendRequest(HttpSocket, 0))
        {
            STATUS_MSG(L"HttpSendRequest: %u\n", GetLastError());
            __leave;
        }

        STATUS_MSG(L"Querying build server...");
        Sleep(500);

        if (!HttpEndRequest(HttpSocket))
        {
            STATUS_MSG(L"HttpEndRequest: %u\n", GetLastError());
            __leave;
        }

        if (!(xmlStringData = HttpDownloadString(HttpSocket)))
        {
            STATUS_MSG(L"HttpDownloadString: %u\n", GetLastError());
            __leave;
        }

        Version = PhFormatString(
            L"%s.%s",
            XmlParseToken(xmlStringData->Buffer, L"ver"),
            XmlParseToken(xmlStringData->Buffer, L"rev")
            )->Buffer;

        DownloadURL = PhFormatString(
            L"https://github.com/processhacker2/processhacker2/releases/download/v%s/processhacker-%s-bin.zip",
            XmlParseToken(xmlStringData->Buffer, L"ver"),
            XmlParseToken(xmlStringData->Buffer, L"ver")
            )->Buffer;

        STATUS_MSG(L"Found build: %s\n", Version);

        isSuccess = TRUE;
    }
    __finally
    {
        if (xmlStringData)
        {
            PhDereferenceObject(xmlStringData);
        }
    }

    return isSuccess;
}




BOOLEAN SetupDownloadBuild(
    _In_ PVOID Arguments
    )
{
    ULONG contentLength = 0;
    BOOLEAN isDownloadSuccess = FALSE;
    P_HTTP_SESSION httpSocket = NULL;
    HTTP_PARSED_URL urlComponents = NULL;
    HANDLE tempFileHandle = NULL;
    ULONG writeLength = 0;
    ULONG readLength = 0;
    ULONG totalLength = 0;
    ULONG bytesDownloaded = 0;
    ULONG downloadedBytes = 0;
    ULONG contentLengthSize = sizeof(ULONG);
    PH_HASH_CONTEXT hashContext;
    IO_STATUS_BLOCK isb;
    BYTE buffer[PAGE_SIZE];

    StartProgress();
    SendDlgItemMessage(Arguments, IDC_PROGRESS1, PBM_SETSTATE, PBST_NORMAL, 0);

    httpSocket = HttpSocketCreate();

    __try
    {
        if (!QueryBuildServerThread(httpSocket))
            __leave;

        if (!HttpParseURL(httpSocket, DownloadURL, &urlComponents))
        {
            STATUS_MSG(L"HttpParseURL: %u\n", GetLastError());
            __leave;
        }

        STATUS_MSG(L"Connecting to download server...\n");

        if (!HttpConnect(httpSocket, urlComponents->HttpServer, INTERNET_DEFAULT_HTTPS_PORT))
        {
            STATUS_MSG(L"HttpConnect: %u\n", GetLastError());
            __leave;
        }

        STATUS_MSG(L"Connected to download server...\n");

        if (!HttpBeginRequest(httpSocket, NULL, urlComponents->HttpPath, WINHTTP_FLAG_SECURE))
        {
            STATUS_MSG(L"HttpBeginRequest: %u\n", GetLastError());
            __leave;
        }

        if (!HttpAddRequestHeaders(httpSocket, L"User-Agent: 0x0D06F00D"))
        {
            STATUS_MSG(L"HttpAddRequestHeaders: %u\n", GetLastError());
            __leave;
        }

        if (!HttpSendRequest(httpSocket, 0))
        {
            STATUS_MSG(L"HttpSendRequest: %u\n", GetLastError());
            __leave;
        }

        STATUS_MSG(L"Querying download server...");

        if (!HttpEndRequest(httpSocket))
        {
            STATUS_MSG(L"HttpEndRequest: %u\n", GetLastError());
            __leave;
        }

        contentLength = HttpGetRequestHeaderDword(httpSocket, WINHTTP_QUERY_CONTENT_LENGTH);

        if (contentLength == 0)
        {
            STATUS_MSG(L"HttpGetRequestHeaderDword: %u\n", GetLastError());
            __leave;
        }

        STATUS_MSG(L"Downloading latest build %s...\n", Version);

        // Create output file
        if (!NT_SUCCESS(PhCreateFileWin32(
            &tempFileHandle,
            L"processhacker-2.38-bin.zip",
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_TEMPORARY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
            )))
        {
            __leave;
        }

        // Initialize hash algorithm.
        PhInitializeHash(&hashContext, Sha1HashAlgorithm);

        // Zero the buffer.
        memset(buffer, 0, PAGE_SIZE);

        // Download the data.
        while (WinHttpReadData(httpSocket->RequestHandle, buffer, PAGE_SIZE, &bytesDownloaded))
        {
            // If we get zero bytes, the file was uploaded or there was an error
            if (bytesDownloaded == 0)
                break;

            // Update the hash of bytes we downloaded.
            PhUpdateHash(&hashContext, buffer, bytesDownloaded);

            // Write the downloaded bytes to disk.
            if (!NT_SUCCESS(NtWriteFile(
                tempFileHandle,
                NULL,
                NULL,
                NULL,
                &isb,
                buffer,
                bytesDownloaded,
                NULL,
                NULL
                )))
            {
                __leave;
            }

            downloadedBytes += (DWORD)isb.Information;

            // Check the number of bytes written are the same we downloaded.
            if (bytesDownloaded != isb.Information)
                __leave;

            // Update the GUI progress.
            // TODO: Update on GUI thread.
            //{
            //    //int percent = MulDiv(100, downloadedBytes, contentLength);
            //    FLOAT percent = ((FLOAT)downloadedBytes / contentLength * 100);
            //    PPH_STRING totalDownloaded = PhFormatSize(downloadedBytes, -1);
            //    PPH_STRING totalLength = PhFormatSize(contentLength, -1);
            //
            //    PPH_STRING dlLengthString = PhFormatString(
            //        L"%s of %s (%.0f%%)",
            //        totalDownloaded->Buffer,
            //        totalLength->Buffer,
            //        percent
            //        );
            //
            //    // Update the progress bar position
            //    SendMessage(context->ProgressHandle, PBM_SETPOS, (ULONG)percent, 0);
            //    Static_SetText(context->StatusHandle, dlLengthString->Buffer);
            //
            //    PhDereferenceObject(dlLengthString);
            //    PhDereferenceObject(totalDownloaded);
            //    PhDereferenceObject(totalLength);
            //}

            SetProgress(downloadedBytes, contentLength);
        }

        // Compute hash result (will fail if file not downloaded correctly).
        //if (PhFinalHash(&hashContext, &hashBuffer, 20, NULL))
        //{
        //    // Allocate our hash string, hex the final hash result in our hashBuffer.
        //    PPH_STRING hexString = PhBufferToHexString(hashBuffer, 20);
        //
        //    if (PhEqualString(hexString, context->Hash, TRUE))
        //    {
        //        //hashSuccess = TRUE;
        //    }
        //
        //    PhDereferenceObject(hexString);
        //}
        //
        //    //if (_tcsstr(hashETag->Buffer, finalHexString->Buffer))
        //    {         
        //        //DEBUG_MSG(TEXT("Hash success: %s (%s)\n"), SetupFileName->Buffer, finalHexString->Buffer);
        //    }
        //    //else
        //    //{
        //    //    SendDlgItemMessage(_hwndProgress, IDC_PROGRESS1, PBM_SETSTATE, PBST_ERROR, 0);
        //    //    SetDlgItemText(_hwndProgress, IDC_MAINHEADER, TEXT("Retrying download... Hash error."));
        //    //    DEBUG_MSG(TEXT("Hash error (retrying...): %s\n"), SetupFileName->Buffer);
        //    //}


        isDownloadSuccess = TRUE;
    }
    __finally
    {
        if (tempFileHandle)
        {
            NtClose(tempFileHandle);
        }

        if (urlComponents)
        {
            PhFree(urlComponents);
        }
    }

    return TRUE;
}