/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kphlibbase.h>
#include <kphmsgdyn.h>

#ifndef _KERNEL_MODE
#include "../tools/thirdparty/winsdk/ntintsafe.h"
#endif

#include <pshpack1.h>
typedef struct _KPH_DYN_DATA_BUFFER
{
    USHORT Size;
    CHAR Buffer[ANYSIZE_ARRAY];
} KPH_DYN_DATA_BUFFER, *PKPH_DYN_DATA_BUFFER;
#include <poppack.h>

/**
 * \brief Finds a dynamic data entry in the table.
 *
 * \param[in] Message Message to get the table entry from.
 * \param[in] FieldId Field identifier to look for.
 *
 * \return Pointer to table entry on success, null if not found.
 */
_Must_inspect_result_
PCKPH_MESSAGE_DYNAMIC_TABLE_ENTRY KphpMsgDynFindEntry(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId
    )
{
    for (USHORT i = 0; i < Message->_Dyn.Count; i++)
    {
        if (Message->_Dyn.Entries[i].FieldId == FieldId)
        {
            return &Message->_Dyn.Entries[i];
        }
    }

    return NULL;
}

/**
 * \brief Claims some data in the dynamic data buffer.
 *
 * \param[in] Message Message to claim dynamic data in.
 * \param[in] FieldId Field identifier of the data to be populated.
 * \param[in] TypeId Type identifier of the data to be populated.
 * \param[in] RequiredSize Required size of the dynamic data.
 * \param[out] DynData Set to pointer to claimed data in buffer on success.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpMsgDynClaimDynData(
    _In_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ KPH_MESSAGE_TYPE_ID TypeId,
    _In_ ULONG RequiredSize,
    _Outptr_result_nullonfailure_ PVOID* DynData
    )
{
    NTSTATUS status;
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY claimed;
    ULONG offset;

    *DynData = NULL;

    if ((FieldId <= InvalidKphMsgField) || (FieldId >= MaxKphMsgField))
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if ((TypeId <= InvalidKphMsgType) || (TypeId >= MaxKphMsgType))
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    status = KphMsgValidate(Message);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if ((Message->_Dyn.Count >= ARRAYSIZE(Message->_Dyn.Entries)) ||
        (RequiredSize >= ARRAYSIZE(Message->_Dyn.Buffer)))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (KphpMsgDynFindEntry(Message, FieldId) != NULL)
    {
        return STATUS_ALREADY_COMMITTED;
    }

    if (Message->_Dyn.Count == 0)
    {
        offset = 0;
        claimed = &Message->_Dyn.Entries[0];
    }
    else
    {
        ULONG endOffset;

        status = RtlULongAdd(
            Message->_Dyn.Entries[Message->_Dyn.Count - 1].Offset,
            Message->_Dyn.Entries[Message->_Dyn.Count - 1].Size,
            &offset);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = RtlULongAdd(offset, RequiredSize, &endOffset);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (endOffset >= ARRAYSIZE(Message->_Dyn.Buffer))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        claimed = &Message->_Dyn.Entries[Message->_Dyn.Count];
    }

    claimed->FieldId = FieldId;
    claimed->TypeId = TypeId;
    claimed->Offset = offset;
    claimed->Size = RequiredSize;
    Message->_Dyn.Count++;
    Message->Header.Size += RequiredSize;
    *DynData = &Message->_Dyn.Buffer[offset];
    return STATUS_SUCCESS;
}

/**
 * \brief Looks up a dynamic data entry.
 *
 * \param[in] Message Message to look up dynamic data in.
 * \param[in] FieldId Field identifier to look up.
 * \param[in] TypeId Type identifier to look up.
 * \param[out] DynData Set to point to dynamic data in buffer on success.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpMsgDynLookupDynData(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ KPH_MESSAGE_TYPE_ID TypeId,
    _Outptr_result_nullonfailure_ const VOID** DynData
    )
{
    NTSTATUS status;
    PCKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;
    ULONG offset;

    *DynData = NULL;

    status = KphMsgValidate(Message);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    entry = KphpMsgDynFindEntry(Message, FieldId);
    if (!entry)
    {
        return STATUS_NOT_FOUND;
    }

    if (entry->TypeId != TypeId)
    {
        return STATUS_CONTEXT_MISMATCH;
    }

    status = RtlULongAdd(entry->Offset, entry->Size, &offset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (offset >= ARRAYSIZE(Message->_Dyn.Buffer))
    {
        return STATUS_HEAP_CORRUPTION;
    }

    *DynData = &Message->_Dyn.Buffer[entry->Offset];
    return STATUS_SUCCESS;
}

/**
 * \brief Clears the dynamic data table, effectively "freeing" the dynamic data
 * buffer to be populated with other information.
 *
 * \param Message The message to clear the dynamic data table of.
 */
VOID KphMsgDynClear(
    _Inout_ PKPH_MESSAGE Message
    )
{
    Message->Header.Size = KPH_MESSAGE_MIN_SIZE;
    Message->_Dyn.Count = 0;
    RtlZeroMemory(&Message->_Dyn.Entries, sizeof(Message->_Dyn.Entries));
}

/**
 * \brief Adds a Unicode string to the dynamic data.
 *
 * \param[in,out] Message The message to populate dynamic data of.
 * \param[in] FieldId Field identifier for the data.
 * \param[in] String Unicode string to copy into the dynamic data.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphMsgDynAddUnicodeString(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PCUNICODE_STRING String
    )
{
    NTSTATUS status;
    PKPH_DYN_DATA_BUFFER data;
    ULONG requiredSize;

    requiredSize = sizeof(KPH_DYN_DATA_BUFFER);
    status = RtlULongAdd(requiredSize, String->Length, &requiredSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlULongAdd(requiredSize, sizeof(WCHAR), &requiredSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = KphpMsgDynClaimDynData(Message,
                                    FieldId,
                                    KphMsgTypeUnicodeString,
                                    requiredSize,
                                    (PVOID*)&data);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    data->Size = String->Length;
    RtlCopyMemory(&data->Buffer[0], String->Buffer, String->Length);
    *((PWCHAR)&data->Buffer[String->Length]) = UNICODE_NULL;
    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves a Unicode string from the dynamic data of a message.
 *
 * \param[in] Message The message to retrieve the Unicode string from.
 * \param[in] FieldId Field identifier for the data.
 * \param[out] String If found populated with a reference to the string data in the
 * dynamic data buffer.
 *
 * \return Successful or errant status.
 */
NTSTATUS KphMsgDynGetUnicodeString(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _Out_ PUNICODE_STRING String
    )
{
    NTSTATUS status;
    PKPH_DYN_DATA_BUFFER data;

    status = KphpMsgDynLookupDynData(Message,
                                     FieldId,
                                     KphMsgTypeUnicodeString,
                                     (PVOID*)&data);
    if (!NT_SUCCESS(status))
    {
        String->Length = 0;
        String->MaximumLength = 0;
        String->Buffer = NULL;
        return status;
    }

    String->Length = data->Size;
    String->MaximumLength = data->Size;
    String->Buffer = (PWCH)&data->Buffer[0];
    return STATUS_SUCCESS;
}

/**
 * \brief Adds an ANSI string to the dynamic data buffer.
 *
 * \param[in,out] Message The message to add the string to.
 * \param[in] FieldId Field identifier for the string.
 * \param[in] String ANSI string to copy into the dynamic data buffer.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphMsgDynAddAnsiString(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PCANSI_STRING String
    )
{
    NTSTATUS status;
    PKPH_DYN_DATA_BUFFER data;
    ULONG requiredSize;

    requiredSize = sizeof(KPH_DYN_DATA_BUFFER);
    status = RtlULongAdd(requiredSize, String->Length, &requiredSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlULongAdd(requiredSize, sizeof(CHAR), &requiredSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = KphpMsgDynClaimDynData(Message,
                                    FieldId,
                                    KphMsgTypeAnsiString,
                                    requiredSize,
                                    (PVOID*)&data);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    data->Size = String->Length;
    RtlCopyMemory(&data->Buffer[0], String->Buffer, String->Length);
    data->Buffer[String->Length] = ANSI_NULL;
    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves an ANSI string from the message.
 *
 * \param[in] Message The message to retrieve the string from.
 * \param[in] FieldId Field identifier of the string.
 * \param[out] String If found populated with a reference to the string data in the
 * dynamic data buffer.
 *
 * \return Successful or errant status.
 */
NTSTATUS KphMsgDynGetAnsiString(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _Out_ PANSI_STRING String
    )
{
    NTSTATUS status;
    PKPH_DYN_DATA_BUFFER data;

    status = KphpMsgDynLookupDynData(Message,
                                     FieldId,
                                     KphMsgTypeAnsiString,
                                     (PVOID*)&data);
    if (!NT_SUCCESS(status))
    {
        String->Length = 0;
        String->MaximumLength = 0;
        String->Buffer = NULL;
        return status;
    }

    String->Length = data->Size;
    String->MaximumLength = data->Size;
    String->Buffer = (PCHAR)&data->Buffer[0];
    return STATUS_SUCCESS;
}

/**
 * \brief Adds a stack trace to the dynamic data buffer.
 *
 * \param[in,out] Message The message to add the stack trace to.
 * \param[in] FieldId Field identifier for the stack trace.
 * \param[in] StackTrace Stack trace to copy into the dynamic data buffer.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphMsgDynAddStackTrace(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PKPH_STACK_TRACE StackTrace
    )
{
    NTSTATUS status;
    PKPH_DYN_DATA_BUFFER data;
    ULONG requiredSize;

    requiredSize = sizeof(KPH_DYN_DATA_BUFFER);
    status = RtlULongAdd(requiredSize,
                         (sizeof(PVOID) * StackTrace->Count),
                         &requiredSize);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = KphpMsgDynClaimDynData(Message,
                                    FieldId,
                                    KphMsgTypeStackTrace,
                                    requiredSize,
                                    (PVOID*)&data);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    data->Size = StackTrace->Count;
    RtlCopyMemory(&data->Buffer[0],
                  StackTrace->Frames,
                  (sizeof(PVOID) * StackTrace->Count));
    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves a stack trace from the message.
 *
 * \param[in] Message The message to retrieve the stack trace from.
 * \param[in] FieldId Field identifier of the stack trace.
 * \param[out] StackTrace If found populated with a reference to the stack
 * trace data in the dynamic data buffer.
 *
 * \return Successful or errant status.
 */
NTSTATUS KphMsgDynGetStackTrace(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _Out_ PKPH_STACK_TRACE StackTrace
    )
{
    NTSTATUS status;
    PKPH_DYN_DATA_BUFFER data;

    status = KphpMsgDynLookupDynData(Message,
                                     FieldId,
                                     KphMsgTypeStackTrace,
                                     (PVOID*)&data);
    if (!NT_SUCCESS(status))
    {
        StackTrace->Frames = NULL;
        StackTrace->Count = 0;
        return status;
    }

    StackTrace->Frames = (PVOID*)&data->Buffer[0];
    StackTrace->Count = data->Size;
    return STATUS_SUCCESS;
}
