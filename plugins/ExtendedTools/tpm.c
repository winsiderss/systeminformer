/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s    2023
 *
 */

#include "exttools.h"
#include "secedit.h"

const TPM_RH TpmRHOwner = TPM_RH_OWNER;
const TPM_RH TpmRHNull = TPM_RH_NULL;
const TPM_RH TpmRSPassword = TPM_RS_PW;

CONST PH_ACCESS_ENTRY TpmAttributeEntries[31] =
{
    { L"TPMA_NV_PPWRITE", TPMA_NV_PPWRITE, FALSE, FALSE, L"Platform write" },
    { L"TPMA_NV_OWNERWRITE", TPMA_NV_OWNERWRITE, FALSE, FALSE, L"Owner write" },
    { L"TPMA_NV_AUTHWRITE", TPMA_NV_AUTHWRITE, FALSE, FALSE, L"Auth write" },
    { L"TPMA_NV_POLICYWRITE", TPMA_NV_POLICYWRITE, FALSE, FALSE, L"Policy write" },
    { L"TPMA_NV_COUNTER", TPMA_NV_COUNTER, FALSE, FALSE, L"Counter" },
    { L"TPMA_NV_BITS", TPMA_NV_BITS, FALSE, FALSE, L"Bits" },
    { L"TPMA_NV_EXTEND", TPMA_NV_EXTEND, FALSE, FALSE, L"Extend" },
    { L"TPMA_NV_RESERVED_TYPE_1", TPMA_NV_RESERVED_TYPE_1, FALSE, FALSE, L"Reserved type 1" },
    { L"TPMA_NV_RESERVED_TYPE_2", TPMA_NV_RESERVED_TYPE_2, FALSE, FALSE, L"Reserved type 2" },
    { L"TPMA_NV_RESERVED_TYPE_3", TPMA_NV_RESERVED_TYPE_3, FALSE, FALSE, L"Reserved type 4" },
    { L"TPMA_NV_POLICY_DELETE", TPMA_NV_POLICY_DELETE, FALSE, FALSE, L"Policy delete" },
    { L"TPMA_NV_WRITELOCKED", TPMA_NV_WRITELOCKED, FALSE, FALSE, L"Write locked" },
    { L"TPMA_NV_WRITEALL", TPMA_NV_WRITEALL, FALSE, FALSE, L"Write all" },
    { L"TPMA_NV_WRITEDEFINE", TPMA_NV_WRITEDEFINE, FALSE, FALSE, L"Write define" },
    { L"TPMA_NV_WRITE_STCLEAR", TPMA_NV_WRITE_STCLEAR, FALSE, FALSE, L"Write lockable" },
    { L"TPMA_NV_GLOBALLOCK", TPMA_NV_GLOBALLOCK, FALSE, FALSE, L"Global lock" },
    { L"TPMA_NV_PPREAD", TPMA_NV_PPREAD, FALSE, FALSE, L"Platform read" },
    { L"TPMA_NV_OWNERREAD", TPMA_NV_OWNERREAD, FALSE, FALSE, L"Owner read" },
    { L"TPMA_NV_AUTHREAD", TPMA_NV_AUTHREAD, FALSE, FALSE, L"Auth read" },
    { L"TPMA_NV_POLICYREAD", TPMA_NV_POLICYREAD, FALSE, FALSE, L"Policy read" },
    { L"TPMA_NV_RESERVED_FLAG_1", TPMA_NV_RESERVED_FLAG_1, FALSE, FALSE, L"Reserved flag 1" },
    { L"TPMA_NV_RESERVED_FLAG_2", TPMA_NV_RESERVED_FLAG_2, FALSE, FALSE, L"Reserved flag 2" },
    { L"TPMA_NV_RESERVED_FLAG_3", TPMA_NV_RESERVED_FLAG_3, FALSE, FALSE, L"Reserved flag 3" },
    { L"TPMA_NV_RESERVED_FLAG_4", TPMA_NV_RESERVED_FLAG_4, FALSE, FALSE, L"Reserved flag 4" },
    { L"TPMA_NV_RESERVED_FLAG_5", TPMA_NV_RESERVED_FLAG_5, FALSE, FALSE, L"Reserved flag 5" },
    { L"TPMA_NV_NO_DA", TPMA_NV_NO_DA, FALSE, FALSE, L"No dictionary attack protections" },
    { L"TPMA_NV_ORDERLY", TPMA_NV_ORDERLY, FALSE, FALSE, L"Orderly" },
    { L"TPMA_NV_CLEAR_STCLEAR", TPMA_NV_CLEAR_STCLEAR, FALSE, FALSE, L"Clear lockable" },
    { L"TPMA_NV_READLOCKED", TPMA_NV_READLOCKED, FALSE, FALSE, L"Read locked" },
    { L"TPMA_NV_WRITTEN", TPMA_NV_WRITTEN, FALSE, FALSE, L"Written" },
    { L"TPMA_NV_READ_STCLEAR", TPMA_NV_READ_STCLEAR, FALSE, FALSE, L"Read lockable" },
};

typedef struct _TPM_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND ParentWindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} TPM_WINDOW_CONTEXT, *PTPM_WINDOW_CONTEXT;

NTSTATUS EtTpmReturnCodeToStatus(
    _In_ ULONG ReturnCode
    )
{
#define ET_TPM_20_EC_MAP(n) case TPM_RC_##n: return STATUS_TPM_20_E_##n

    switch (_byteswap_ulong(ReturnCode))
    {
    case TPM_RC_SUCCESS:
        return STATUS_SUCCESS;
    case TPM_RC_BAD_TAG:
        return STATUS_TPM_BADTAG;
    ET_TPM_20_EC_MAP(INITIALIZE);
    ET_TPM_20_EC_MAP(FAILURE);
    ET_TPM_20_EC_MAP(SEQUENCE);
    ET_TPM_20_EC_MAP(PRIVATE);
    ET_TPM_20_EC_MAP(HMAC);
    ET_TPM_20_EC_MAP(DISABLED);
    ET_TPM_20_EC_MAP(EXCLUSIVE);
    ET_TPM_20_EC_MAP(AUTH_TYPE);
    ET_TPM_20_EC_MAP(POLICY);
    ET_TPM_20_EC_MAP(PCR);
    ET_TPM_20_EC_MAP(PCR_CHANGED);
    ET_TPM_20_EC_MAP(UPGRADE);
    ET_TPM_20_EC_MAP(TOO_MANY_CONTEXTS);
    ET_TPM_20_EC_MAP(AUTH_UNAVAILABLE);
    ET_TPM_20_EC_MAP(REBOOT);
    ET_TPM_20_EC_MAP(UNBALANCED);
    ET_TPM_20_EC_MAP(COMMAND_SIZE);
    ET_TPM_20_EC_MAP(COMMAND_CODE);
    ET_TPM_20_EC_MAP(AUTHSIZE);
    ET_TPM_20_EC_MAP(AUTH_CONTEXT);
    ET_TPM_20_EC_MAP(NV_RANGE);
    ET_TPM_20_EC_MAP(NV_SIZE);
    ET_TPM_20_EC_MAP(NV_AUTHORIZATION);
    ET_TPM_20_EC_MAP(NV_UNINITIALIZED);
    ET_TPM_20_EC_MAP(NV_SPACE);
    ET_TPM_20_EC_MAP(NV_DEFINED);
    ET_TPM_20_EC_MAP(BAD_CONTEXT);
    ET_TPM_20_EC_MAP(CPHASH);
    ET_TPM_20_EC_MAP(PARENT);
    ET_TPM_20_EC_MAP(NEEDS_TEST);
    ET_TPM_20_EC_MAP(NO_RESULT);
    ET_TPM_20_EC_MAP(SENSITIVE);
    ET_TPM_20_EC_MAP(ASYMMETRIC);
    ET_TPM_20_EC_MAP(ATTRIBUTES);
    ET_TPM_20_EC_MAP(HASH);
    ET_TPM_20_EC_MAP(VALUE);
    ET_TPM_20_EC_MAP(HIERARCHY);
    ET_TPM_20_EC_MAP(KEY_SIZE);
    ET_TPM_20_EC_MAP(MGF);
    ET_TPM_20_EC_MAP(MODE);
    ET_TPM_20_EC_MAP(TYPE);
    ET_TPM_20_EC_MAP(HANDLE);
    ET_TPM_20_EC_MAP(KDF);
    ET_TPM_20_EC_MAP(RANGE);
    ET_TPM_20_EC_MAP(AUTH_FAIL);
    ET_TPM_20_EC_MAP(NONCE);
    ET_TPM_20_EC_MAP(PP);
    ET_TPM_20_EC_MAP(SCHEME);
    ET_TPM_20_EC_MAP(SIZE);
    ET_TPM_20_EC_MAP(SYMMETRIC);
    ET_TPM_20_EC_MAP(TAG);
    ET_TPM_20_EC_MAP(SELECTOR);
    ET_TPM_20_EC_MAP(INSUFFICIENT);
    ET_TPM_20_EC_MAP(SIGNATURE);
    ET_TPM_20_EC_MAP(KEY);
    ET_TPM_20_EC_MAP(POLICY_FAIL);
    ET_TPM_20_EC_MAP(INTEGRITY);
    ET_TPM_20_EC_MAP(TICKET);
    ET_TPM_20_EC_MAP(RESERVED_BITS);
    ET_TPM_20_EC_MAP(BAD_AUTH);
    ET_TPM_20_EC_MAP(EXPIRED);
    ET_TPM_20_EC_MAP(POLICY_CC);
    ET_TPM_20_EC_MAP(BINDING);
    ET_TPM_20_EC_MAP(CURVE);
    ET_TPM_20_EC_MAP(ECC_POINT);
    case TPM_RC_CONTEXT_GAP:
        return STATUS_TPM_CONTEXT_GAP;
    case TPM_RC_OBJECT_MEMORY:
    case TPM_RC_SESSION_MEMORY:
    case TPM_RC_MEMORY:
        return STATUS_TPM_NOSPACE;
    case TPM_RC_SESSION_HANDLES:
    case TPM_RC_OBJECT_HANDLES:
        return STATUS_TPM_BAD_HANDLE;
    case TPM_RC_LOCALITY:
        return STATUS_TPM_BAD_LOCALITY;
    case TPM_RC_YIELDED:
    case TPM_RC_CANCELED:
        return STATUS_TPM_COMMAND_CANCELED;
    case TPM_RC_TESTING:
        return STATUS_TPM_DOING_SELFTEST;
    case TPM_RC_REFERENCE_H0:
    case TPM_RC_REFERENCE_H1:
    case TPM_RC_REFERENCE_H2:
    case TPM_RC_REFERENCE_H3:
    case TPM_RC_REFERENCE_H4:
    case TPM_RC_REFERENCE_H5:
    case TPM_RC_REFERENCE_H6:
    case TPM_RC_REFERENCE_S0:
    case TPM_RC_REFERENCE_S1:
    case TPM_RC_REFERENCE_S2:
    case TPM_RC_REFERENCE_S3:
    case TPM_RC_REFERENCE_S4:
    case TPM_RC_REFERENCE_S5:
    case TPM_RC_REFERENCE_S6:
        return STATUS_TPM_BAD_HANDLE;
    case TPM_RC_NV_RATE:
        return STATUS_TPM_RETRY;
    case TPM_RC_LOCKOUT:
        return STATUS_TPM_AREA_LOCKED;
    case TPM_RC_RETRY:
        return STATUS_TPM_RETRY;
    case TPM_RC_NV_UNAVAILABLE:
        return STATUS_NOT_FOUND;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

_Must_inspect_result_
NTSTATUS EtTpmOpen(
    _Out_ PTBS_HCONTEXT TbsContextHandle
    )
{
    TBS_CONTEXT_PARAMS2 params;

    params.asUINT32 = 0;
    params.version = TBS_CONTEXT_VERSION_TWO;
    params.includeTpm20 = 1;

    if (Tbsi_Context_Create(
        (PTBS_CONTEXT_PARAMS)&params,
        TbsContextHandle
        ) != TBS_SUCCESS)
    {
        return STATUS_TPM_20_E_FAILURE;
    }

    return STATUS_SUCCESS;
}

NTSTATUS EtTpmClose(
    _In_ TBS_HCONTEXT TbsContextHandle
    )
{
    if (Tbsip_Context_Close(TbsContextHandle) != TBS_SUCCESS)
    {
        return STATUS_TPM_20_E_HANDLE;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS EtTpmQueryIndices(
    _In_ TBS_HCONTEXT TbsContextHandle,
    _Inout_ PULONG IndexCount,
    _Out_writes_all_opt_(*IndexCount) TPM_NV_INDEX* Indices
    )
{
    TPM_GET_CAPABILITY_CMD_HEADER command;
    TPM_GET_CAPABILITY_REPLY reply;
    ULONG indexCount;
    ULONG resultLength;

    command.Header.SessionTag = _byteswap_ushort(TPM_ST_NO_SESSIONS);
    command.Header.CommandCode = _byteswap_ulong(TPM_CC_GetCapability);
    command.Header.Size = _byteswap_ulong(sizeof(command));

    indexCount = RTL_NUMBER_OF(reply.Data.Data.Handles.Handle);
    command.Capability = _byteswap_ulong(TPM_CAP_HANDLES);
    command.Property = _byteswap_ulong(TPM_HR_NV_INDEX);
    command.PropertyCount = _byteswap_ulong(indexCount);

    resultLength = sizeof(reply);
    if (Tbsip_Submit_Command(
        TbsContextHandle,
        TBS_COMMAND_LOCALITY_ZERO,
        TBS_COMMAND_PRIORITY_NORMAL,
        (PCBYTE)&command,
        sizeof(command),
        (PBYTE)&reply,
        &resultLength
        ) != TBS_SUCCESS)
    {
        return STATUS_TPM_20_E_FAILURE;
    }

    if (reply.Header.ResponseCode != TPM_RC_SUCCESS)
    {
        *IndexCount = 0;
        return EtTpmReturnCodeToStatus(reply.Header.ResponseCode);
    }

    indexCount = _byteswap_ulong(reply.Data.Data.Handles.Count);

    if (!Indices || *IndexCount < indexCount)
    {
        *IndexCount = indexCount;
        return STATUS_BUFFER_TOO_SMALL;
    }

    for (ULONG i = 0; i < indexCount; i++)
    {
        Indices[i].Value = _byteswap_ulong(reply.Data.Data.Handles.Handle[i].Value);
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS EtTpmReadPublic(
    _In_ TBS_HCONTEXT TbsContextHandle,
    _In_ TPM_NV_INDEX Index,
    _Out_ TPMA_NV* Attributes,
    _Out_ PUSHORT DataSize
    )
{
    TPM_NV_READ_PUBLIC_CMD_HEADER command;
    TPM_NV_READ_PUBLIC_REPLY reply;
    ULONG resultLength;

    *Attributes = 0;
    *DataSize = 0;

    command.Header.SessionTag = _byteswap_ushort(TPM_ST_NO_SESSIONS);
    command.Header.CommandCode = _byteswap_ulong(TPM_CC_NV_ReadPublic);
    command.Header.Size = _byteswap_ulong(sizeof(command));

    command.NvIndex.Value = _byteswap_ulong(Index.Value);

    resultLength = sizeof(reply);

    if (Tbsip_Submit_Command(
        TbsContextHandle,
        TBS_COMMAND_LOCALITY_ZERO,
        TBS_COMMAND_PRIORITY_NORMAL,
        (PCBYTE)&command,
        sizeof(command),
        (PBYTE)&reply,
        &resultLength
        ) != TBS_SUCCESS)
    {
        return STATUS_TPM_20_E_FAILURE;
    }

    if (reply.Header.ResponseCode != TPM_RC_SUCCESS)
        return EtTpmReturnCodeToStatus(reply.Header.ResponseCode);

    *Attributes = _byteswap_ulong(reply.NvPublic.Attributes);
    *DataSize = _byteswap_ushort(reply.NvPublic.DataSize);

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS EtTpmReadOffset(
    _In_ TBS_HCONTEXT TbsContextHandle,
    _In_ TPM_NV_INDEX Index,
    _In_ USHORT Offset,
    _Out_writes_bytes_all_(DataSize) PBYTE Data,
    _In_ USHORT DataSize
    )
{
    NTSTATUS status;
    BYTE buffer[FIELD_OFFSET(TPM_NV_READ_CMD_HEADER, AuthSession.Password) + sizeof(TPM_NV_READ_CMD_FOOTER)];
    ULONG size;
    PTPM_NV_READ_CMD_HEADER command;
    PTPM_NV_READ_CMD_FOOTER footer;
    PTPM_NV_READ_REPLY reply;
    ULONG resultLength;

    command = (TPM_NV_READ_CMD_HEADER*)buffer;

    command->Header.SessionTag = _byteswap_ushort(TPM_ST_SESSIONS);
    command->Header.CommandCode = _byteswap_ulong(TPM_CC_NV_Read);
    command->Header.Size = _byteswap_ulong(sizeof(buffer));

    command->NvIndex.Value = _byteswap_ulong(Index.Value);

    command->AuthHandle.Value = _byteswap_ulong(TpmRHOwner.Value);

    size = FIELD_OFFSET(TPMS_AUTH_COMMAND_NO_NONCE, Password);
    size -= RTL_FIELD_SIZE(TPMS_AUTH_COMMAND_NO_NONCE, SessionSize);
    command->AuthSession.SessionSize = _byteswap_ulong(size);
    command->AuthSession.SessionHandle.Value = _byteswap_ulong(TpmRSPassword.Value);
    command->AuthSession.NonceSize = 0;
    command->AuthSession.SessionAttributes.Value = 0;
    command->AuthSession.PasswordSize = 0;
    footer = (TPM_NV_READ_CMD_FOOTER*)&command->AuthSession.Password[0];

    footer->Offset = _byteswap_ushort(Offset);
    footer->Size = _byteswap_ushort(DataSize);

    size = FIELD_OFFSET(TPM_NV_READ_REPLY, Data);
    size += DataSize;
    size += sizeof(TPMS_AUTH_RESPONSE_NO_NONCE);
    reply = PhAllocateZero(size);
    resultLength = size;

    if (Tbsip_Submit_Command(
        TbsContextHandle,
        TBS_COMMAND_LOCALITY_ZERO,
        TBS_COMMAND_PRIORITY_NORMAL,
        (PCBYTE)command,
        sizeof(buffer),
        (PBYTE)reply,
        &resultLength
        ) != TBS_SUCCESS)
    {
        status = STATUS_TPM_20_E_FAILURE;
        goto CleanupExit;
    }

    if (reply->Header.ResponseCode != TPM_RC_SUCCESS)
    {
        status = EtTpmReturnCodeToStatus(reply->Header.ResponseCode);
        goto CleanupExit;
    }

    for (ULONG i = 0; i < DataSize; i++)
    {
        Data[i] = reply->Data[i];
    }

    status = STATUS_SUCCESS;

CleanupExit:

    PhFree(reply);

    return status;
}

_Must_inspect_result_
NTSTATUS EtTpmRead(
    _In_ TBS_HCONTEXT TbsContextHandle,
    _In_ TPM_NV_INDEX Index,
    _Out_writes_bytes_all_(DataSize) PBYTE Data,
    _In_ USHORT DataSize
    )
{
    NTSTATUS status;
    USHORT remaining;
    USHORT offset;

    RtlZeroMemory(Data, DataSize);

    remaining = DataSize;
    offset = 0;

    while (remaining)
    {
        USHORT readSize;

        //
        // Certain TPMs do not support reads over a certain sizes that is
        // smaller than the specification recommends.
        //
        readSize = min(remaining, 512);

        status = EtTpmReadOffset(TbsContextHandle,
                                 Index,
                                 offset,
                                 PTR_ADD_OFFSET(Data, offset),
                                 readSize);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        offset += readSize;
        remaining -= readSize;
    }

    return STATUS_SUCCESS;
}

NTSTATUS TpmOpen(
    _Out_ PHANDLE FileHandle
    )
{
    static PH_STRINGREF deviceName = PH_STRINGREF_INIT(L"\\??\\TPM");
    NTSTATUS status;

    status = PhCreateFile(
        FileHandle,
        &deviceName,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        );

    return status;
}

NTSTATUS TpmDeviceIoControl(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG IoControlCode,
    _In_opt_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _In_ PVOID OutputBuffer,
    _Inout_opt_ PULONG OutputBufferLength
    )
{
    NTSTATUS status;
    ULONG outputBufferLength = 0;
    HANDLE eventHandle;
    IO_STATUS_BLOCK ioStatusBlock;

    if (OutputBufferLength)
        outputBufferLength = *OutputBufferLength;

    status = PhCreateEvent(
        &eventHandle,
        EVENT_ALL_ACCESS,
        NotificationEvent,
        FALSE
        );

    if (NT_SUCCESS(status))
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            eventHandle,
            NULL,
            NULL,
            &ioStatusBlock,
            IoControlCode,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            outputBufferLength
            );

        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject(DeviceHandle, FALSE, NULL);

            if (NT_SUCCESS(status))
            {
                status = ioStatusBlock.Status;
            }
        }

        if (NT_SUCCESS(status))
        {
            if (OutputBufferLength)
                *OutputBufferLength = (ULONG)ioStatusBlock.Information;
        }

        NtClose(eventHandle);
    }

    return status;
}

NTSTATUS TpmGetDeviceInfo(
    _Out_ PTPM_DEVICE_INFO Info
    )
{
    NTSTATUS status;
    HANDLE deviceHandle;
    ULONG deviceInfoLength;
    TPM_DEVICE_INFO deviceInfo;

    deviceInfoLength = sizeof(TPM_DEVICE_INFO);
    memset(&deviceInfo, 0, sizeof(TPM_DEVICE_INFO));

    status = TpmOpen(&deviceHandle);

    if (NT_SUCCESS(status))
    {
        status = TpmDeviceIoControl(
            deviceHandle,
            0x22C01C,
            NULL,
            0,
            &deviceInfo,
            &deviceInfoLength
            );

        NtClose(deviceHandle);
    }

    if (NT_SUCCESS(status))
    {
        memcpy(Info, &deviceInfo, sizeof(TPM_DEVICE_INFO));
    }

    return status;
}

BOOLEAN EtTpmIsReady(
    VOID
    )
{
    NTSTATUS status;
    TPM_DEVICE_INFO info;

    status = TpmGetDeviceInfo(&info);

    if (status == STATUS_SUCCESS)
        return TRUE;
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        return Tbsi_GetDeviceInfo(sizeof(info), &info) == TBS_SUCCESS;

    return FALSE;
}

PPH_STRING EtMakeTpmAttributesString(
    _In_ TPMA_NV Attributes
    )
{
    PH_FORMAT format[4];
    PPH_STRING attributesString;
    PPH_STRING string;

    attributesString = PhGetAccessString(Attributes, (PVOID)TpmAttributeEntries, RTL_NUMBER_OF(TpmAttributeEntries));
    PhInitFormatSR(&format[0], attributesString->sr);
    PhInitFormatS(&format[1], L" (0x");
    PhInitFormatX(&format[2], Attributes);
    PhInitFormatS(&format[3], L")");
    string = PhFormat(format, RTL_NUMBER_OF(format), 1);
    PhDereferenceObject(attributesString);

    return string;
}

NTSTATUS EtEnumerateTpmEntries(
    _In_ PTPM_WINDOW_CONTEXT Context
    )
{
    NTSTATUS status;
    TBS_HCONTEXT tbsContextHandle;
    ULONG indexCount;
    TPM_NV_INDEX* indices;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    indices = NULL;

    status = EtTpmOpen(&tbsContextHandle);
    if (!NT_SUCCESS(status))
    {
        tbsContextHandle = NULL;
        goto Exit;
    }

    indexCount = 0;
    status = EtTpmQueryIndices(tbsContextHandle, &indexCount, NULL);
    if (status != STATUS_BUFFER_TOO_SMALL)
        goto Exit;

    indices = PhAllocateZero(indexCount * sizeof(TPM_NV_INDEX));

    status = EtTpmQueryIndices(tbsContextHandle, &indexCount, indices);
    if (!NT_SUCCESS(status))
        goto Exit;

    for (ULONG i = 0; i < indexCount; i++)
    {
        INT lvItemIndex;
        USHORT dataSize;
        PPH_STRING string;
        TPMA_NV attributes;

        string = PhFormatString(L"0x%08lx", indices[i].Value);
        lvItemIndex = PhAddListViewItem(Context->ListViewHandle, MAXINT, string->Buffer, ULongToPtr(indices[i].Value));
        PhDereferenceObject(string);

        if (!NT_SUCCESS(EtTpmReadPublic(
            tbsContextHandle,
            indices[i],
            &attributes,
            &dataSize
            )))
            continue;

        string = PhFormatSize(dataSize, ULONG_MAX);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, string->Buffer);
        PhDereferenceObject(string);

        if ((attributes & (TPMA_NV_OWNERREAD | TPMA_NV_OWNERWRITE)) == (TPMA_NV_OWNERREAD | TPMA_NV_OWNERWRITE))
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, L"Read/Write");
        else if ((attributes & TPMA_NV_OWNERREAD) == TPMA_NV_OWNERREAD)
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, L"Read");
        else if ((attributes & TPMA_NV_OWNERWRITE) == TPMA_NV_OWNERWRITE)
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, L"Write");
        else
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, L"None");

        if ((attributes & (TPMA_NV_AUTHREAD | TPMA_NV_AUTHWRITE)) == (TPMA_NV_AUTHREAD | TPMA_NV_AUTHWRITE))
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, L"Read/Write");
        else if ((attributes & TPMA_NV_AUTHREAD) == TPMA_NV_AUTHREAD)
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, L"Read");
        else if ((attributes & TPMA_NV_AUTHWRITE) == TPMA_NV_AUTHWRITE)
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, L"Write");
        else
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, L"None");

        if ((attributes & (TPMA_NV_PPREAD | TPMA_NV_PPWRITE)) == (TPMA_NV_PPREAD | TPMA_NV_PPWRITE))
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, L"Read/Write");
        else if ((attributes & TPMA_NV_PPREAD) == TPMA_NV_PPREAD)
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, L"Read");
        else if ((attributes & TPMA_NV_PPWRITE) == TPMA_NV_PPWRITE)
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, L"Write");
        else
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 4, L"None");

        // clear the bits we've broken out into their own columns
        attributes &= ~(
            TPMA_NV_OWNERREAD |
            TPMA_NV_OWNERWRITE |
            TPMA_NV_AUTHREAD |
            TPMA_NV_AUTHWRITE |
            TPMA_NV_PPWRITE |
            TPMA_NV_PPREAD
            );

        string = EtMakeTpmAttributesString(attributes);
        PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 5, string->Buffer);
        PhDereferenceObject(string);
    }

    ExtendedListView_SortItems(Context->ListViewHandle);
    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);

Exit:

    if (indices)
        PhFree(indices);

    if (tbsContextHandle)
        EtTpmClose(tbsContextHandle);

    return status;
}

INT_PTR CALLBACK EtTpmDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PTPM_WINDOW_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(TPM_WINDOW_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            TPM_DEVICE_INFO info;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_TPM_LIST);
            context->ParentWindowHandle = (HWND)lParam;

            PhSetApplicationWindowIcon(hwndDlg);

            if (Tbsi_GetDeviceInfo(sizeof(info), &info) == TBS_SUCCESS)
            {
                PPH_STRING newText;
                PPH_STRING windowText = PhGetWindowText(hwndDlg);

                if (info.tpmVersion == TPM_VERSION_12)
                    newText = PhConcatStringRefZ(&windowText->sr, L" 1.2");
                else if (info.tpmVersion == TPM_VERSION_20)
                    newText = PhConcatStringRefZ(&windowText->sr, L" 2.0");
                else
                    newText = PhReferenceObject(windowText);

                PhSetWindowText(hwndDlg, newText->Buffer);
                PhDereferenceObject(newText);
                PhDereferenceObject(windowText);
            }

            // TODO(jxy-s)
            // - incorporate Tbsi_Get_TCG_Log(_Ex)
            // - incorporate Tbsi_Get_OwnerAuth
            // - fully support TMP 1.2

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 80, L"Index");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Data size");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 85, L"Owner rights");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 85, L"Auth rights");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 85, L"Platform rights");
            PhAddListViewColumn(context->ListViewHandle, 5, 5, 5, LVCFMT_LEFT, 400, L"Attributes");
            PhSetExtendedListView(context->ListViewHandle);

            PhLoadListViewColumnsFromSetting(SETTING_NAME_TPM_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_TPM_LIST_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            if (PhGetIntegerPairSetting(SETTING_NAME_TPM_WINDOW_POSITION).X != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_TPM_WINDOW_POSITION, SETTING_NAME_TPM_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, context->ParentWindowHandle);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            EtEnumerateTpmEntries(context);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_TPM_LISTVIEW_COLUMNS, context->ListViewHandle);
            PhSaveWindowPlacementToSetting(SETTING_NAME_TPM_WINDOW_POSITION, SETTING_NAME_TPM_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_TPM_LIST_REFRESH:
                {
                    EtEnumerateTpmEntries(context);
                }
                break;
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            switch (hdr->code)
            {
            case NM_DBLCLK:
                {
                    if (hdr->hwndFrom == context->ListViewHandle)
                    {
                        PVOID entry;

                        if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                        {
                            TPM_NV_INDEX index;
                            index.Value = PtrToUlong(entry);
                            EtShowTpmEditDialog(hwndDlg, index);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"&Edit", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case 1:
                                TPM_NV_INDEX index;
                                index.Value = PtrToUlong(listviewItems[0]);
                                EtShowTpmEditDialog(hwndDlg, index);
                                break;
                            case PHAPP_IDC_COPY:
                                PhCopyListView(context->ListViewHandle);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID EtShowTpmDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (!EtTpmIsReady())
    {
        PhShowStatus(ParentWindowHandle, L"Unable to query the TPM", STATUS_TPM_FAIL, 0);
        return;
    }

    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_TPM),
        NULL,
        EtTpmDlgProc,
        ParentWindowHandle
        );
}
