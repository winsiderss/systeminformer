/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PAGED_FILE();

/**
 * \brief Captures a Unicode string from user mode.
 *
 * \param[in] UnicodeString Unicode string to capture from user mode.
 * \param[out] CapturedUnicodeString Receives the captured Unicode string, the
 * captured buffer must be freed using KphReleaseUnicodeString.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureUnicodeString(
    _In_ PUNICODE_STRING UnicodeString,
    _Out_ PUNICODE_STRING* CapturedUnicodeString
    )
{
    NTSTATUS status;
    UNICODE_STRING inputString;
    PUNICODE_STRING outputString;

    KPH_PAGED_CODE_PASSIVE();

    outputString = NULL;

    __try
    {
        CopyFromUser(&inputString, UnicodeString, sizeof(UNICODE_STRING));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    outputString = KphAllocatePaged(sizeof(UNICODE_STRING) + inputString.Length,
                                    KPH_TAG_CAPTURED_UNICODE_STRING);
    if (!outputString)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    outputString->Buffer = Add2Ptr(outputString, sizeof(UNICODE_STRING));
    outputString->Length = 0;
    outputString->MaximumLength = inputString.Length;

    __try
    {
        CopyFromUser(outputString->Buffer,
                     inputString.Buffer,
                     inputString.Length);
        outputString->Length = inputString.Length;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    *CapturedUnicodeString = outputString;
    outputString = NULL;

    status = STATUS_SUCCESS;

Exit:

    if (outputString)
    {
        KphFree(outputString, KPH_TAG_CAPTURED_UNICODE_STRING);
    }

    return status;
}

/**
 * \brief Releases a previously captured Unicode string.
 *
 * \param[in] UnicodeString Unicode string to release.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphReleaseUnicodeString(
    _In_ PUNICODE_STRING CaputredUnicodeString
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphFree(CaputredUnicodeString, KPH_TAG_CAPTURED_UNICODE_STRING);
}

/**
 * \brief Zeros memory in the specified mode.
 *
 * \param[out] Destination Address to zero memory at.
 * \param[in] Length The length of the memory to zero, in bytes.
 * \param[in] AccessMode The access mode to use for the zeroing.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphZeroModeMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        ZeroModeMemory(Destination, Length, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Copies memory to the specified mode.
 *
 * \param[out] Destination Address to copy memory to.
 * \param[in] Source Address to copy memory from.
 * \param[in] Length The length of the memory to copy, in bytes.
 * \param[in] AccessMode The access mode to use for the copy.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCopyToMode(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) PVOID Source,
    _In_ SIZE_T Length,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        CopyToMode(Destination, Source, Length, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Copies memory from the specified mode.
 *
 * \param[out] Destination Address to copy memory to.
 * \param[in] Source Address to copy memory from.
 * \param[in] Length The length of the memory to copy, in bytes.
 * \param[in] AccessMode The access mode to use for the copy.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphCopyFromMode(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) PVOID Source,
    _In_ SIZE_T Length,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        CopyFromMode(Destination, Source, Length, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Writes an unsigned 8-bit value to the specified mode.
 *
 * \param[out] Destination Address to write the unsigned 8-bit value to.
 * \param[in] Source The unsigned 8-bit value to write.
 * \param[in] AccessMode The access mode to use for the write.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteUCharToMode(
    _Out_ PUCHAR Destination,
    _In_ UCHAR Source,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        WriteUCharToMode(Destination, Source, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Writes an unsigned 32-bit value to the specified mode.
 *
 * \param[out] Destination Address to write the unsigned 32-bit value to.
 * \param[in] Source The unsigned 32-bit value to write.
 * \param[in] AccessMode The access mode to use for the write.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteULongToMode(
    _Out_ PULONG Destination,
    _In_ ULONG Source,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        WriteULongToMode(Destination, Source, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Writes an unsigned 64-bit value to the specified mode.
 *
 * \param[out] Destination Address to write the unsigned 64-bit value to.
 * \param[in] Source The unsigned 64-bit value to write.
 * \param[in] AccessMode The access mode to use for the write.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteULong64ToMode(
    _Out_ PULONG64 Destination,
    _In_ ULONG64 Source,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        WriteULong64ToMode(Destination, Source, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Writes an signed 64-bit value to the specified mode.
 *
 * \param[out] Destination Address to write the signed 64-bit value to.
 * \param[in] Source The signed 64-bit value to write.
 * \param[in] AccessMode The access mode to use for the write.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteLong64ToMode(
    _Out_ PLONG64 Destination,
    _In_ LONG64 Source,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        WriteLong64ToMode(Destination, Source, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Writes an unsigned size width value to the specified mode.
 *
 * \param[out] Destination Address to write the unsigned sized width value to.
 * \param[in] Source The unsigned sized width value to write.
 * \param[in] AccessMode The access mode to use for the write.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteSizeTToMode(
    _Out_ PSIZE_T Destination,
    _In_ SIZE_T Source,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        WriteSizeTToMode(Destination, Source, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Writes a pointer to the specified mode.
 *
 * \param[out] Destination Address to write the pointer to.
 * \param[in] Source The pointer to write.
 * \param[in] AccessMode The access mode to use for the write.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWritePointerToMode(
    _Out_ PVOID* Destination,
    _In_ PVOID Source,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        WritePointerToMode(Destination, Source, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Writes a handle to the specified mode.
 *
 * \details If the write fails then the handle is closed oh behalf of the caller
 * to prevent a handle leak.
 *
 * \param[out] Destination Address to write the handle to.
 * \param[in] Source The handle to write.
 * \param[in] AccessMode The access mode to use for the write.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphWriteHandleToMode(
    _Out_ PHANDLE Destination,
    _In_ _Post_ptr_invalid_ HANDLE Source,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        WriteHandleToMode(Destination, Source, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        ObCloseHandle(Source, AccessMode);
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Copies a Unicode string to the specified mode.
 *
 * \details The Unicode string is constructed at the destination address and the
 * string content immediately follows the string structure.
 *
 * \param[out] Destination Optional address to copy the Unicode string to.
 * \param[in] Length The length of the destination buffer, in bytes.
 * \param[in] String Optional Unicode string to copy.
 * \param[out] ReturnLength Number of bytes copied to the destination buffer,
 * including the string structure and following content.
 * \param[in] AccessMode The access mode to use for the copy.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCopyUnicodeStringToMode(
    _Out_writes_bytes_opt_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_opt_ PCUNICODE_STRING String,
    _Out_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    PUNICODE_STRING destination;
    USHORT maximumLength;

    KPH_PAGED_CODE();

    if (!String)
    {
        *ReturnLength = sizeof(UNICODE_STRING);

        if (!Destination || (Length < sizeof(UNICODE_STRING)))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        destination = Destination;

        return KphZeroModeMemory(destination,
                                 sizeof(UNICODE_STRING),
                                 AccessMode);
    }

    *ReturnLength = (sizeof(UNICODE_STRING) + String->Length);

    if (!Destination || (Length < (sizeof(UNICODE_STRING) + String->Length)))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    destination = Destination;
    maximumLength = (USHORT)(Length - sizeof(UNICODE_STRING));

    if (AccessMode != KernelMode)
    {
        __try
        {
            WriteUShortToUser(&destination->Length, String->Length);
            WriteUShortToUser(&destination->MaximumLength, maximumLength);
            WritePointerToUser(&destination->Buffer,
                               Add2Ptr(destination, sizeof(UNICODE_STRING)));

            NT_ASSERT(destination->Buffer);

            CopyToUser(destination->Buffer, String->Buffer, String->Length);

            if ((String->Length + sizeof(UNICODE_NULL)) <= maximumLength)
            {
                PWCHAR end;

                end = &destination->Buffer[String->Length / sizeof(WCHAR)];

                WriteUShortToUser(end, UNICODE_NULL);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    else
    {
        destination->Length = 0;
        destination->MaximumLength = maximumLength;
        destination->Buffer = Add2Ptr(destination, sizeof(UNICODE_STRING));
        RtlCopyUnicodeString(destination, String);
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Reads a LARGE_INTEGER from the specified mode.
 *
 * \param[out] Destination Address to read the LARGE_INTEGER to.
 * \param[in] Source The LARGE_INTEGER to read.
 * \param[in] AccessMode The access mode to use for the read.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphReadLargeIntegerFromMode(
    _Out_ PLARGE_INTEGER Destination,
    _In_ PLARGE_INTEGER Source,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    KPH_PAGED_CODE();

    __try
    {
        *Destination = ReadLargeIntegerFromMode(Source, AccessMode);
    }
    __except (UmaExceptionFilter(AccessMode))
    {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}
