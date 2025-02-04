/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#include <kphlibbase.h>
#include <kphmsgdyn.h>

#ifndef _KERNEL_MODE
#include <ntintsafe.h>
#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif
#endif

/**
 * \brief Finds a dynamic message entry in the table.
 *
 * \param[in] Message The message to get the table entry from.
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
 * \brief Claims some data in the dynamic message buffer.
 *
 * \param[in] Message The message to claim dynamic message in.
 * \param[in] FieldId Field identifier of the data to be populated.
 * \param[in] TypeId Type identifier of the data to be populated.
 * \param[in] Length The length to claim.
 * \param[out] Entry Set to the claimed entry.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpMsgDynClaimEntry(
    _In_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ KPH_MESSAGE_TYPE_ID TypeId,
    _In_ USHORT Length,
    _Outptr_ PCKPH_MESSAGE_DYNAMIC_TABLE_ENTRY* Entry
    )
{
    NTSTATUS status;
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY claimed;
    USHORT offset;

    *Entry = NULL;

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

    if (Message->_Dyn.Count >= ARRAYSIZE(Message->_Dyn.Entries))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (KphpMsgDynFindEntry(Message, FieldId))
    {
        return STATUS_ALREADY_COMMITTED;
    }

    offset = Message->Header.Size;

    status = RtlUShortAdd(offset, Length, &offset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (offset > sizeof(KPH_MESSAGE))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    offset = Message->Header.Size;

    claimed = &Message->_Dyn.Entries[Message->_Dyn.Count++];
    claimed->FieldId = FieldId;
    claimed->TypeId = TypeId;
    claimed->Offset = offset;
    claimed->Length = Length;

    Message->Header.Size += Length;

    *Entry = claimed;

    return STATUS_SUCCESS;
}

/**
 * \brief Looks up a dynamic message entry.
 *
 * \param[in] Message The message to look up dynamic message entry in.
 * \param[in] FieldId Field identifier to look up.
 * \param[in] TypeId Type identifier to look up.
 * \param[out] DynData Set to point to dynamic message entry.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpMsgDynLookupEntry(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ KPH_MESSAGE_TYPE_ID TypeId,
    _Outptr_ PCKPH_MESSAGE_DYNAMIC_TABLE_ENTRY* Entry
    )
{
    NTSTATUS status;
    PCKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    *Entry = NULL;

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

    if (entry->Offset >= Message->Header.Size)
    {
        return STATUS_HEAP_CORRUPTION;
    }

    *Entry = entry;

    return STATUS_SUCCESS;
}

/**
 * \brief Clears the dynamic message table, effectively "freeing" the dynamic
 * message buffer to be populated with other information.
 *
 * \param[in,out] Message The message to clear the dynamic message table of.
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
 * \brief Clears that last added dynamic message entry.
 *
 * \param[in,out] Message The message to clear the last dynamic message entry.
 */
VOID KphMsgDynClearLast(
    _Inout_ PKPH_MESSAGE Message
    )
{
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    if (Message->_Dyn.Count)
    {
        return;
    }

    Message->_Dyn.Count--;
    entry = &Message->_Dyn.Entries[Message->_Dyn.Count];
    Message->Header.Size -= entry->Length;
    RtlZeroMemory(entry, sizeof(KPH_MESSAGE_DYNAMIC_TABLE_ENTRY));
}

/**
 * \brief Retrieves the remaining size of the dynamic message buffer.
 *
 * \param[in] Message The message to retrieve the remaining size of.
 *
 * \return Remaining size of the dynamic message buffer.
 */
USHORT KphMsgDynRemaining(
    _In_ PCKPH_MESSAGE Message
    )
{
    NTSTATUS status;
    USHORT remaining;

    remaining = sizeof(KPH_MESSAGE);

    status = RtlUShortSub(remaining, Message->Header.Size, &remaining);
    if (!NT_SUCCESS(status))
    {
        return 0;
    }

    return remaining;
}

/**
 * \brief Adds a Unicode string to the dynamic message buffer.
 *
 * \param[in,out] Message The message to populate dynamic message buffer of.
 * \param[in] FieldId Field identifier for the data.
 * \param[in] String Unicode string to copy into the dynamic message buffer.
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
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    status = KphpMsgDynClaimEntry(Message,
                                  FieldId,
                                  KphMsgTypeUnicodeString,
                                  String->Length,
                                  &entry);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    RtlCopyMemory(Add2Ptr(Message, entry->Offset),
                  String->Buffer,
                  String->Length);

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves a Unicode string from the dynamic message buffer.
 *
 * \param[in] Message The message to retrieve the Unicode string from.
 * \param[in] FieldId Field identifier for the data.
 * \param[out] String If found populated with a reference to the string data in
 * the dynamic message buffer.
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
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    status = KphpMsgDynLookupEntry(Message,
                                   FieldId,
                                   KphMsgTypeUnicodeString,
                                   &entry);
    if (!NT_SUCCESS(status))
    {
        String->Length = 0;
        String->MaximumLength = 0;
        String->Buffer = NULL;

        return status;
    }

    String->Length = entry->Length;
    String->MaximumLength = String->Length;
    String->Buffer = Add2Ptr(Message, entry->Offset);

    return STATUS_SUCCESS;
}

/**
 * \brief Adds an ANSI string to the dynamic message buffer.
 *
 * \param[in,out] Message The message to add the string to.
 * \param[in] FieldId Field identifier for the string.
 * \param[in] String ANSI string to copy into the dynamic message buffer.
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
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    status = KphpMsgDynClaimEntry(Message,
                                  FieldId,
                                  KphMsgTypeAnsiString,
                                  String->Length,
                                  &entry);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    RtlCopyMemory(Add2Ptr(Message, entry->Offset),
                  String->Buffer,
                  String->Length);

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves an ANSI string from the message.
 *
 * \param[in] Message The message to retrieve the string from.
 * \param[in] FieldId Field identifier of the string.
 * \param[out] String If found populated with a reference to the string data in
 * the dynamic message buffer.
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
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    status = KphpMsgDynLookupEntry(Message,
                                   FieldId,
                                   KphMsgTypeAnsiString,
                                   &entry);
    if (!NT_SUCCESS(status))
    {
        String->Length = 0;
        String->MaximumLength = 0;
        String->Buffer = NULL;

        return status;
    }

    String->Length = entry->Length;
    String->MaximumLength = String->Length;
    String->Buffer = Add2Ptr(Message, entry->Offset);

    return STATUS_SUCCESS;
}

/**
 * \brief Adds a stack trace to the dynamic message buffer.
 *
 * \param[in,out] Message The message to add the stack trace to.
 * \param[in] FieldId Field identifier for the stack trace.
 * \param[in] StackTrace Stack trace to copy into the dynamic message buffer.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphMsgDynAddStackTrace(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PKPHM_STACK_TRACE StackTrace
    )
{
    NTSTATUS status;
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    status = KphpMsgDynClaimEntry(Message,
                                  FieldId,
                                  KphMsgTypeStackTrace,
                                  (StackTrace->Count * sizeof(PVOID)),
                                  &entry);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    RtlCopyMemory(Add2Ptr(Message, entry->Offset),
                  StackTrace->Frames,
                  (StackTrace->Count * sizeof(PVOID)));

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves a stack trace from the message.
 *
 * \param[in] Message The message to retrieve the stack trace from.
 * \param[in] FieldId Field identifier of the stack trace.
 * \param[out] StackTrace If found populated with a reference to the stack
 * trace data in the dynamic message buffer.
 *
 * \return Successful or errant status.
 */
NTSTATUS KphMsgDynGetStackTrace(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _Out_ PKPHM_STACK_TRACE StackTrace
    )
{
    NTSTATUS status;
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    status = KphpMsgDynLookupEntry(Message,
                                   FieldId,
                                   KphMsgTypeStackTrace,
                                   &entry);
    if (!NT_SUCCESS(status))
    {
        StackTrace->Count = 0;
        StackTrace->Frames = NULL;

        return status;
    }

    StackTrace->Count = (entry->Length / sizeof(PVOID));
    StackTrace->Frames = Add2Ptr(Message, entry->Offset);

    return STATUS_SUCCESS;
}

/**
 * \brief Adds a sized buffer to the dynamic message buffer.
 *
 * \param[in,out] Message The message to add the sized buffer to.
 * \param[in] FieldId Field identifier for the sized buffer.
 * \param[in] SizedBuffer Sized buffer to copy into the dynamic message buffer.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphMsgDynAddSizedBuffer(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PKPHM_SIZED_BUFFER SizedBuffer
    )
{
    NTSTATUS status;
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    status = KphpMsgDynClaimEntry(Message,
                                  FieldId,
                                  KphMsgTypeSizedBuffer,
                                  SizedBuffer->Size,
                                  &entry);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    RtlCopyMemory(Add2Ptr(Message, entry->Offset),
                  SizedBuffer->Buffer,
                  SizedBuffer->Size);

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves a sized buffer from the message.
 *
 * \param[in] Message The message to retrieve the sized buffer from.
 * \param[in] FieldId Field identifier of the sized buffer.
 * \param[out] SizedBuffer If found populated with a reference to the sized
 * buffer data in the dynamic message buffer.
 *
 * \return Successful or errant status.
 */
NTSTATUS KphMsgDynGetSizedBuffer(
    _In_ PCKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _Out_ PKPHM_SIZED_BUFFER SizedBuffer
    )
{
    NTSTATUS status;
    PKPH_MESSAGE_DYNAMIC_TABLE_ENTRY entry;

    status = KphpMsgDynLookupEntry(Message,
                                    FieldId,
                                    KphMsgTypeSizedBuffer,
                                    &entry);
    if (!NT_SUCCESS(status))
    {
        SizedBuffer->Size = 0;
        SizedBuffer->Buffer = NULL;
        return status;
    }

    SizedBuffer->Size = entry->Length;
    SizedBuffer->Buffer = Add2Ptr(Message, entry->Offset);

    return STATUS_SUCCESS;
}
