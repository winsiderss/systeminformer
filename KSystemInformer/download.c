/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <kph.h>

#include <trace.h>

PAGED_FILE();

typedef struct _KPH_DOWNLOAD_CONTEXT
{
    KPH_SOCKET_HANDLE Socket;
    KPH_TLS_HANDLE Tls;
} KPH_DOWNLOAD_CONTEXT, *PKPH_DOWNLOAD_CONTEXT;

static UNICODE_STRING KphpDownloadPort = RTL_CONSTANT_STRING(L"443");
static ANSI_STRING KphpDownloadHttpMethod = RTL_CONSTANT_STRING("GET");
static ANSI_STRING KphpDownloadHeaderLocation = RTL_CONSTANT_STRING("Location");
static KPH_HTTP_HEADER_ITEM KphpDownloadHeaders[] =
{
{ RTL_CONSTANT_STRING("Accept"),     RTL_CONSTANT_STRING("application/octet-stream") },
{ RTL_CONSTANT_STRING("Connection"), RTL_CONSTANT_STRING("close") },
};

/**
 * \brief Closes the download context, shutting down TLS and closing the socket
 * and TLS handle.
 *
 * \param[in] Context The download context to close.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpDownloadContextClose(
    _In_ PKPH_DOWNLOAD_CONTEXT Context
    )
{
    PAGED_CODE();

    if (Context->Socket)
    {
        if (Context->Tls)
        {
            KphSocketTlsShutdown(Context->Socket, Context->Tls);
            KphSocketTlsClose(Context->Tls);
        }

        KphSocketClose(Context->Socket);
    }
}

/**
 * \brief Internal routine that establishes the a binary download connection
 * and receives the initial response.
 *
 * \param[in] UrlInfo The URL information to connect to.
 * \param[in] Timeout Optional timeout to use for network operations.
 * \param[out] Buffer The buffer to receive the response.
 * \param[in,out] Length On input, the length of the buffer. On output, the
 * length of the response.
 * \param[out] Response The parsed response structure, must outlive the buffer.
 * \param[out] Socket Populated with the socket handle.
 * \param[out] Tls Populated with the TLS handle.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpDownloadBinary(
    _In_ PKPH_URL_INFORMATION UrlInfo,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_writes_bytes_to_(*Length, *Length) PVOID Buffer,
    _Inout_ PULONG Length,
    _Outptr_allocatesMem_ PKPH_HTTP_RESPONSE* Response,
    _Out_ PKPH_SOCKET_HANDLE Socket,
    _Out_ PKPH_TLS_HANDLE Tls
    )
{
    NTSTATUS status;
    UNICODE_STRING hostName;
    BYTE portBuffer[12];
    UNICODE_STRING port;
    ADDRINFOEXW hints;
    PADDRINFOEXW remoteAddress;
    PVOID requestBuffer;
    ULONG requestLength;
    SOCKADDR_IN localAddress;
    KPH_SOCKET_HANDLE socket;
    KPH_TLS_HANDLE tls;

    PAGED_CODE();

    *Response = NULL;
    *Socket = NULL;
    *Tls = NULL;

    RtlZeroMemory(&hostName, sizeof(hostName));
    socket = NULL;
    tls = NULL;
    remoteAddress = NULL;
    requestBuffer = NULL;

    if (!UrlInfo->DomainName.Length || !UrlInfo->Authority.Length)
    {
        KphTracePrint(TRACE_LEVEL_ERROR, GENERAL, "Invalid URL information.");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = RtlAnsiStringToUnicodeString(&hostName,
                                          &UrlInfo->DomainName,
                                          TRUE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "RtlAnsiStringToUnicodeString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (UrlInfo->Port.Length)
    {
        port.Buffer = (PWCH)portBuffer;
        port.Length = 0;
        port.MaximumLength = sizeof(portBuffer);

        status = RtlAnsiStringToUnicodeString(&port, &UrlInfo->Port, FALSE);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "RtlAnsiStringToUnicodeString failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }
    else
    {
        port = KphpDownloadPort;
    }

    RtlZeroMemory(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_socktype = IPPROTO_TCP;
    hints.ai_family = AF_INET;

    status = KphGetAddressInfo(&hostName,
                               &port,
                               &hints,
                               Timeout,
                               &remoteAddress);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphGetAddressInfo failed: %!STATUS!",
                      status);

        goto Exit;
    }

    requestLength = 0;
    status = KphHttpBuildRequest(&KphpDownloadHttpMethod,
                                 &UrlInfo->Authority,
                                 &UrlInfo->Path,
                                 &UrlInfo->Parameters,
                                 KphpDownloadHeaders,
                                 ARRAYSIZE(KphpDownloadHeaders),
                                 NULL,
                                 0,
                                 NULL,
                                 &requestLength);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        NT_ASSERT(!NT_SUCCESS(status));

        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphHttpBuildRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

    requestBuffer = KphAllocatePaged(requestLength, KPH_TAG_DOWNLOAD_REQUEST);
    if (!requestBuffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphHttpBuildRequest(&KphpDownloadHttpMethod,
                                 &UrlInfo->Authority,
                                 &UrlInfo->Path,
                                 &UrlInfo->Parameters,
                                 KphpDownloadHeaders,
                                 ARRAYSIZE(KphpDownloadHeaders),
                                 NULL,
                                 0,
                                 requestBuffer,
                                 &requestLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphHttpBuildRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

    localAddress.sin_family = AF_INET;
    localAddress.sin_addr.s_addr = INADDR_ANY;
    localAddress.sin_port = 0;

    status = KphSocketConnect(SOCK_STREAM,
                              IPPROTO_TCP,
                              (PSOCKADDR)&localAddress,
                              remoteAddress->ai_addr,
                              Timeout,
                              &socket);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphSocketConnect failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphSocketTlsCreate(&tls);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphSocketTlsCreate failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphSocketTlsHandshake(socket, Timeout, tls, &hostName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphSocketTlsHandshake failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphSocketTlsSend(socket,
                              Timeout,
                              tls,
                              requestBuffer,
                              requestLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphSocketTlsSend failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphSocketTlsRecv(socket, Timeout, tls, Buffer, Length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphSocketTlsRecv failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphHttpParseResponse(Buffer, *Length, Response);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphHttpParseResponse failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *Socket = socket;
    socket = NULL;
    *Tls = tls;
    tls = NULL;

Exit:

    if (socket)
    {
        if (tls)
        {
            KphSocketTlsShutdown(socket, tls);
            KphSocketTlsClose(tls);
        }

        KphSocketClose(socket);
    }

    if (requestBuffer)
    {
        KphFree(requestBuffer, KPH_TAG_DOWNLOAD_REQUEST);
    }

    if (remoteAddress)
    {
        KphFreeAddressInfo(remoteAddress);
    }

    RtlFreeUnicodeString(&hostName);

    return status;
}

/**
 * \brief Downloads a binary from the specified URL.
 *
 * \details This routine requires that the buffer be sufficient to receive the
 * initial response headers. The buffer will be filled only with the response
 * body on a successful return. The caller should call KphDownlodBinaryContinue
 * to continue to receive the rest of the data. The caller should check the
 * successful length output of KphDownloadBinaryContinue against zero to know
 * that all the content has been received.
 *
 * \param[in] Url The URL to download from.
 * \param[in] Timeout Optional timeout to use for network operations. Note that
 * this timeout is for any one network operation, not the entire download.
 * \param[in] MaxRedirects The maximum number of redirects to follow.
 * \param[out] Buffer The buffer to receive the binary content.
 * \param[in,out] Length On input, the length of the buffer. On output, the
 * number of bytes written to the buffer.
 * \param[out] Handle Populated with download handle, must be eventually closed
 * with KphDownloadClose.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphDownloadBinary(
    _In_ PANSI_STRING Url,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ ULONG MaxRedirects,
    _Out_writes_bytes_to_(*Length, *Length) PVOID Buffer,
    _Inout_ PULONG Length,
    _Outptr_allocatesMem_ PKPH_DOWNLOAD_HANDLE Handle
    )
{
    NTSTATUS status;
    PKPH_DOWNLOAD_CONTEXT context;
    ULONG length;
    KPH_URL_INFORMATION urlInfo;
    PKPH_HTTP_RESPONSE response;

    PAGED_CODE();

    *Handle = NULL;

    response = NULL;

    context = KphAllocatePaged(sizeof(KPH_DOWNLOAD_CONTEXT),
                               KPH_TAG_DOWNLOAD_CONTEXT);
    if (!context)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to allocate download context.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphParseUrlInformation(Url, &urlInfo);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphParseUrlInformation failed: %!STATUS!",
                      status);

        goto Exit;
    }

    length = *Length;
    status = KphpDownloadBinary(&urlInfo,
                                Timeout,
                                Buffer,
                                &length,
                                &response,
                                &context->Socket,
                                &context->Tls);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpDownloadBinary failed: %!STATUS!",
                      status);

        goto Exit;
    }

    while (MaxRedirects)
    {
        ANSI_STRING location;
        KPH_URL_INFORMATION locationInfo;

        if ((response->StatusCode < 300) || (response->StatusCode > 399))
        {
            break;
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Received redirect response: %hu %Z",
                      response->StatusCode,
                      &response->StatusMessage);

        MaxRedirects--;

        location.Length = 0;

        for (ULONG i = 0; i < response->HeaderItemCount; i++)
        {
            PKPH_HTTP_HEADER_ITEM item;

            item = &response->HeaderItems[i];

            if (RtlEqualString(&item->Key, &KphpDownloadHeaderLocation, TRUE))
            {
                location = item->Value;
                break;
            }
        }

        if (!location.Length)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "Failed to find Location header item.");

            status = STATUS_UNEXPECTED_NETWORK_ERROR;
            goto Exit;
        }

        //
        // TODO(jxy-s) track the previous URL and log multiple redirects better
        //
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "%Z -> %Z",
                      Url,
                      &location);

        status = KphParseUrlInformation(&location, &locationInfo);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "KphParseUrlInformation failed (%Z): %!STATUS!",
                          &location,
                          status);

            goto Exit;
        }

        if (!locationInfo.Authority.Length)
        {
            //
            // TODO(jxy-s) handle relative redirects
            //
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "Relative redirect not supported.");

            status = STATUS_NOT_SUPPORTED;
            goto Exit;
        }

        KphHttpFreeResponse(response);
        response = NULL;

        KphpDownloadContextClose(context);
        context->Socket = NULL;
        context->Tls = NULL;

        length = *Length;
        status = KphpDownloadBinary(&locationInfo,
                                    Timeout,
                                    Buffer,
                                    &length,
                                    &response,
                                    &context->Socket,
                                    &context->Tls);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "KphpDownloadBinary failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    if (response->StatusCode != 200)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Received non-200 response: %hu %Z",
                      response->StatusCode,
                      &response->StatusMessage);

        status = STATUS_UNEXPECTED_NETWORK_ERROR;
        goto Exit;
    }

    RtlMoveMemory(Buffer, response->Body, response->BodyLength);
    *Length = response->BodyLength;

    *Handle = context;
    context = NULL;

    status = STATUS_SUCCESS;

Exit:

    if (context)
    {
        KphpDownloadContextClose(context);
        KphFree(context, KPH_TAG_DOWNLOAD_CONTEXT);
    }

    if (response)
    {
        KphHttpFreeResponse(response);
    }

    return status;
}

/**
 * \brief Continues to download from a previous call to KphDownloadBinary.
 *
 * \details The caller should generally always call this on a successful call
 * to KphDownloadBinary. The caller should check the successful length output
 * for zero to know that all the content has been received.
 *
 * \param[in] Handle The download handle.
 * \param[in] Timeout Optional timeout to use for network operations. Note that
 * this timeout is for any one network operation, not the entire download.
 * \param[out] Buffer The buffer to receive the binary content.
 * \param[in,out] Length On input, the length of the buffer. On output, the
 * number of bytes written to the buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphDownloadBinaryContinue(
    _In_ KPH_DOWNLOAD_HANDLE Handle,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_writes_bytes_to_(*Length, *Length) PVOID Buffer,
    _Inout_ PULONG Length
    )
{
    PKPH_DOWNLOAD_CONTEXT context;

    PAGED_CODE();

    context = Handle;

    return KphSocketTlsRecv(context->Socket,
                            Timeout,
                            context->Tls,
                            Buffer,
                            Length);
}

/**
 * \brief Closes a download handle.
 *
 * \param[in] Handle The download handle to close.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphDownloadBinaryClose(
    _In_ KPH_DOWNLOAD_HANDLE Handle
    )
{
    PKPH_DOWNLOAD_CONTEXT context;

    PAGED_CODE();

    context = Handle;

    KphpDownloadContextClose(context);
    KphFree(context, KPH_TAG_DOWNLOAD_CONTEXT);
}
