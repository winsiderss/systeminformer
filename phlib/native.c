/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2024
 *
 */

#include <ph.h>
#include <apiimport.h>
#include <kphuser.h>
#include <lsasup.h>
#include <mapldr.h>

#define PH_DEVICE_PREFIX_LENGTH 64
#define PH_DEVICE_MUP_PREFIX_MAX_COUNT 16

static PH_INITONCE PhDevicePrefixesInitOnce = PH_INITONCE_INIT;

static UNICODE_STRING PhDevicePrefixes[26];
static PH_QUEUED_LOCK PhDevicePrefixesLock = PH_QUEUED_LOCK_INIT;

static PPH_STRING PhDeviceMupPrefixes[PH_DEVICE_MUP_PREFIX_MAX_COUNT] = { 0 };
static ULONG PhDeviceMupPrefixesCount = 0;
static PH_QUEUED_LOCK PhDeviceMupPrefixesLock = PH_QUEUED_LOCK_INIT;

/**
 * Retrieves a copy of an object's security descriptor.
 *
 * \param Handle A handle to the object whose security descriptor is to be queried.
 * \param SecurityInformation The type of security information to be queried.
 * \param SecurityDescriptor A copy of the specified security descriptor in self-relative format.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor
    )
{
    NTSTATUS status;
    ULONG bufferSize;
    PVOID buffer;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);
    // This is required (especially for File objects) because some drivers don't seem to handle
    // QuerySecurity properly. (wj32)
    memset(buffer, 0, bufferSize);

    status = NtQuerySecurityObject(
        Handle,
        SecurityInformation,
        buffer,
        bufferSize,
        &bufferSize
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);
        memset(buffer, 0, bufferSize);

        status = NtQuerySecurityObject(
            Handle,
            SecurityInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *SecurityDescriptor = (PSECURITY_DESCRIPTOR)buffer;

    return status;
}

/**
 * Sets an object's security descriptor.
 *
 * \param Handle A handle to the object whose security descriptor is to be set.
 * \param SecurityInformation The type of security information to be set.
 * \param SecurityDescriptor The security descriptor in self-relative format.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetObjectSecurity(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    return NtSetSecurityObject(
        Handle,
        SecurityInformation,
        SecurityDescriptor
        );
}

/**
 * Merges two SACLs according to the provided security information.
 * The function preserves ACEs not covered by the change from the lower SACL and replaces other
 * ACEs with the ones from the higher SACL.
 *
 * Example:
 *  - A lower SACL containing two ACEs: a trust label and a mandatory label.
 *  - A higher SACL containing one ACE: a trust label.
 *  - Security information specifies PROCESS_TRUST_LABEL_SECURITY_INFORMATION.
 * Result: a SACL with the trust label from the higher SACL and the mandatory label from the lower SACL.
 *
 * \param LowerSacl An existing SACL with potentially mixed ACE types.
 * \param HigherSacl An overlaying SACL for the specified security information.
 * \param SecurityInformation The type of security information to be set.
 * \param MergedSacl The new merged SACL.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhMergeSystemAcls(
    _In_opt_ PACL LowerSacl,
    _In_opt_ PACL HigherSacl,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Outptr_result_maybenull_ PACL* MergedSacl
    )
{
    NTSTATUS status;
    ULONG aceTypesToReplace = 0;
    ULONG requiredSize = 0;
    PACL mergedSacl;
    PACE_HEADER mergedAce;

    if (!LowerSacl && !HigherSacl)
    {
        // Nothing to merge
        *MergedSacl = NULL;
        return STATUS_SUCCESS;
    }

    if (LowerSacl && (LowerSacl->AclRevision < MIN_ACL_REVISION || LowerSacl->AclRevision > MAX_ACL_REVISION))
        return STATUS_UNKNOWN_REVISION;

    if (HigherSacl && (HigherSacl->AclRevision < MIN_ACL_REVISION || HigherSacl->AclRevision > MAX_ACL_REVISION))
        return STATUS_UNKNOWN_REVISION;

    // Identify which types of ACEs we want to keep from the lower SACL and
    // which to replace with the ones from the higher SACL

    if (FlagOn(SecurityInformation, SACL_SECURITY_INFORMATION))
        aceTypesToReplace |= AUDIT_ALARM_ACE_TYPE_MASK;
    if (FlagOn(SecurityInformation, LABEL_SECURITY_INFORMATION))
        aceTypesToReplace |= MANDATORY_LABEL_ACE_TYPE_MASK;
    if (FlagOn(SecurityInformation, ATTRIBUTE_SECURITY_INFORMATION))
        aceTypesToReplace |= RESOURCE_ATTRIBUTE_ACE_TYPE_MASK;
    if (FlagOn(SecurityInformation, SCOPE_SECURITY_INFORMATION))
        aceTypesToReplace |= SCOPED_POLICY_ACE_TYPE_MASK;
    if (FlagOn(SecurityInformation, PROCESS_TRUST_LABEL_SECURITY_INFORMATION))
        aceTypesToReplace |= PROCESS_TRUST_ACE_TYPE_MASK;
    if (FlagOn(SecurityInformation, ACCESS_FILTER_SECURITY_INFORMATION))
        aceTypesToReplace |= ACCESS_FILTER_ACE_TYPE_MASK;

    // Calculate the size of ACEs to keep from the lower SACL

    if (LowerSacl && !HigherSacl && aceTypesToReplace == 0)
    {
        if (LowerSacl->AclSize > USHORT_MAX)
            return STATUS_INVALID_PARAMETER;

        mergedSacl = (PACL)PhAllocate(LowerSacl->AclSize);
        RtlCopyMemory(mergedSacl, LowerSacl, LowerSacl->AclSize);
        *MergedSacl = mergedSacl;
        return STATUS_SUCCESS;
    }

    // Helper macro to walk the ACL. (dmex)
#define START_FOR_EACH_ACE(_ACL_, _ACE_) \
    if (_ACL_) \
    { \
        PVOID _end = PTR_ADD_OFFSET((_ACL_), (_ACL_)->AclSize); \
        PACE_HEADER _ACE_ = (PACE_HEADER)PTR_ADD_OFFSET((_ACL_), sizeof(ACL)); \
        for (USHORT _i = 0; _i < (_ACL_)->AceCount; _i++, _ACE_ = (PACE_HEADER)PTR_ADD_OFFSET(_ACE_, _ACE_->AceSize)) \
        { \
            if ((ULONG_PTR)_ACE_ >= (ULONG_PTR)_end) \
                return STATUS_UNSUCCESSFUL; /* malformed */ \
            if (_ACE_->AceSize < sizeof(ACE_HEADER) || PTR_ADD_OFFSET(_ACE_, _ACE_->AceSize) > _end) \
                return STATUS_UNSUCCESSFUL; /* malformed */
#define END_FOR_EACH_ACE } }

    // Calculate required payload size (sum of selected ACE sizes).

    if (LowerSacl)
    {
        START_FOR_EACH_ACE(LowerSacl, ace)
        {
            ULONG bit = ace->AceType < 32 ? (1u << ace->AceType) : 0;

            if (!FlagOn(bit, aceTypesToReplace))
            {
                requiredSize += ace->AceSize;
            }
        }
        END_FOR_EACH_ACE
    }

    // Calculate the size of ACEs to use from the higher SACL

    if (HigherSacl)
    {
        START_FOR_EACH_ACE(HigherSacl, ace)
        {
            ULONG bit = ace->AceType < 32 ? (1u << ace->AceType) : 0;

            if (FlagOn(bit, aceTypesToReplace))
            {
                requiredSize += ace->AceSize;
            }
        }
        END_FOR_EACH_ACE
    }

    // Allocate new ACL (header + aligned payload).

    requiredSize = sizeof(ACL) + ALIGN_UP_BY(requiredSize, sizeof(ULONG));

    if (requiredSize > USHORT_MAX)
        return STATUS_INVALID_PARAMETER;

    mergedSacl = (PACL)PhAllocate(requiredSize);
    mergedAce = (PACE_HEADER)PTR_ADD_OFFSET(mergedSacl, sizeof(ACL));

    status = PhCreateAcl(mergedSacl, requiredSize, ACL_REVISION);

    if (!NT_SUCCESS(status))
    {
        PhFree(mergedSacl);
        return status;
    }

    // Add the lower SACL ACEs we keep

    if (LowerSacl)
    {
        START_FOR_EACH_ACE(LowerSacl, ace)
        {
            ULONG bit = ace->AceType < 32 ? (1u << ace->AceType) : 0;
            if (!(bit & aceTypesToReplace))
            {
                RtlCopyMemory(mergedAce, ace, ace->AceSize);
                mergedSacl->AceCount++;
                PhEnsureAclRevision(&mergedSacl->AclRevision, mergedAce->AceType);
                mergedAce = (PACE_HEADER)PTR_ADD_OFFSET(mergedAce, mergedAce->AceSize);
            }
        }
        END_FOR_EACH_ACE
    }

    // Add the higher SACL ACEs

    if (HigherSacl)
    {
        START_FOR_EACH_ACE(HigherSacl, ace)
        {
            ULONG bit = ace->AceType < 32 ? (1u << ace->AceType) : 0;
            if (bit & aceTypesToReplace)
            {
                RtlCopyMemory(mergedAce, ace, ace->AceSize);
                mergedSacl->AceCount++;
                PhEnsureAclRevision(&mergedSacl->AclRevision, mergedAce->AceType);
                mergedAce = (PACE_HEADER)PTR_ADD_OFFSET(mergedAce, mergedAce->AceSize);
            }
        }
        END_FOR_EACH_ACE
    }

#undef START_FOR_EACH_ACE
#undef END_FOR_EACH_ACE

    *MergedSacl = mergedSacl;
    return STATUS_SUCCESS;
}

/**
 * Merges two security descriptors according to the provided security information.
 * The function preserves parts not covered by the change from the lower security descriptor and
 * replaces other parts with the ones from the higher security descriptor.
 *
 * Example:
 *  - A lower security descriptor containing a DACL, an owner, and a SACL with a trust label.
 *  - A higher security descriptor containing a DACL, and a SACL with a mandatory label.
 *  - Security information specifies DACL_SECURITY_INFORMATION and LABEL_SECURITY_INFORMATION.
 * Result: a security descriptor with the higher DACL, the lower owner, and a merged SACL with
 * the lower trust label and the higher mandatory label.
 *
 * \param LowerSecurityDescriptor An existing security descriptor.
 * \param HigherSecurityDescriptor An overlaying security descriptor for the specified security information.
 * \param SecurityInformation The type of security information to be set.
 * \param MergedSecurityDescriptor The new merged security descriptor.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhMergeSecurityDescriptors(
    _In_ PSECURITY_DESCRIPTOR LowerSecurityDescriptor,
    _In_ PSECURITY_DESCRIPTOR HigherSecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Outptr_ PSECURITY_DESCRIPTOR* MergedSecurityDescriptor
    )
{
    NTSTATUS status;
    SECURITY_DESCRIPTOR mergedSecurityDescriptor;
    PACL acl;
    PSID sid;
    BOOLEAN present;
    BOOLEAN defaulted;
    PACL higherSacl;
    PACL lowerSacl;
    PACL mergedSacl = NULL;
    PSECURITY_DESCRIPTOR relativeSecurityDescriptor = NULL;
    ULONG requiredLength = 0;

    status = PhCreateSecurityDescriptor(&mergedSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);

    if (!NT_SUCCESS(status))
        return status;

    // Choose the DACL

    status = PhGetDaclSecurityDescriptor(
        (SecurityInformation & DACL_SECURITY_INFORMATION) ? HigherSecurityDescriptor : LowerSecurityDescriptor,
        &present,
        &acl,
        &defaulted
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhSetDaclSecurityDescriptor(
        &mergedSecurityDescriptor,
        present,
        acl,
        defaulted
        );

    if (!NT_SUCCESS(status))
        return status;

    // Choose the owner

    status = PhGetOwnerSecurityDescriptor(
        (SecurityInformation & OWNER_SECURITY_INFORMATION) ? HigherSecurityDescriptor : LowerSecurityDescriptor,
        &sid,
        &defaulted
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!sid)
        return STATUS_INVALID_OWNER;

    status = PhSetOwnerSecurityDescriptor(
        &mergedSecurityDescriptor,
        sid,
        defaulted
        );

    if (!NT_SUCCESS(status))
        return status;

    // Choose the primary group

    status = PhGetGroupSecurityDescriptor(
        (SecurityInformation & GROUP_SECURITY_INFORMATION) ? HigherSecurityDescriptor : LowerSecurityDescriptor,
        &sid,
        &defaulted
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!sid)
        return STATUS_INVALID_PRIMARY_GROUP;

    status = PhSetGroupSecurityDescriptor(
        &mergedSecurityDescriptor,
        sid,
        defaulted
        );

    if (!NT_SUCCESS(status))
        return status;

    // Collect both SACLs

    status = PhGetSaclSecurityDescriptor(
        LowerSecurityDescriptor,
        &present,
        &lowerSacl,
        &defaulted
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!present)
        lowerSacl = NULL;

    status = PhGetSaclSecurityDescriptor(
        HigherSecurityDescriptor,
        &present,
        &higherSacl,
        &defaulted
        );

    if (!NT_SUCCESS(status))
        return status;

    if (!present)
        higherSacl = NULL;

    // Merge SACLs and apply

    status = PhMergeSystemAcls(
        lowerSacl,
        higherSacl,
        SecurityInformation,
        &mergedSacl
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhSetSaclSecurityDescriptor(
        &mergedSecurityDescriptor,
        !!mergedSacl,
        mergedSacl,
        FALSE
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Make the buffer self-relative

    status = RtlAbsoluteToSelfRelativeSD(
        &mergedSecurityDescriptor,
        NULL,
        &requiredLength
        );

    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        status = STATUS_INVALID_SECURITY_DESCR;
        goto CleanupExit;
    }

    relativeSecurityDescriptor = PhAllocate(requiredLength);

    status = RtlAbsoluteToSelfRelativeSD(
        &mergedSecurityDescriptor,
        relativeSecurityDescriptor,
        &requiredLength
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    *MergedSecurityDescriptor = relativeSecurityDescriptor;
    relativeSecurityDescriptor = NULL;

CleanupExit:
    if (mergedSacl)
        PhFree(mergedSacl);

    if (relativeSecurityDescriptor)
        PhFree(relativeSecurityDescriptor);

    return status;
}

/**
 * Retrieves the next environment variable from a process environment block.
 *
 * \param Environment The environment block.
 * \param EnvironmentLength The length of the environment block, in bytes.
 * \param EnumerationKey A pointer to an index variable used for enumeration.
 * \param Variable A pointer to a structure that receives the variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumProcessEnvironmentVariables(
    _In_ PVOID Environment,
    _In_ ULONG EnvironmentLength,
    _Inout_ PULONG EnumerationKey,
    _Out_ PPH_ENVIRONMENT_VARIABLE Variable
    )
{
    ULONG length;
    ULONG startIndex;
    PWCHAR name;
    ULONG nameLength;
    PWCHAR value;
    ULONG valueLength;
    PWCHAR currentChar;
    ULONG currentIndex;

    length = EnvironmentLength / sizeof(WCHAR);

    currentIndex = *EnumerationKey;
    currentChar = PTR_ADD_OFFSET(Environment, currentIndex * sizeof(WCHAR));
    startIndex = currentIndex;
    name = currentChar;

    // Find the end of the name.
    while (TRUE)
    {
        if (currentIndex >= length)
            return STATUS_UNSUCCESSFUL;
        if (*currentChar == L'=' && startIndex != currentIndex)
            break; // equality sign is considered as a delimiter unless it is the first character (diversenok)
        if (*currentChar == UNICODE_NULL)
            return STATUS_UNSUCCESSFUL; // no more variables

        currentIndex++;
        currentChar++;
    }

    nameLength = currentIndex - startIndex;

    currentIndex++;
    currentChar++;
    startIndex = currentIndex;
    value = currentChar;

    // Find the end of the value.
    while (TRUE)
    {
        if (currentIndex >= length)
            return STATUS_UNSUCCESSFUL;
        if (*currentChar == UNICODE_NULL)
            break;

        currentIndex++;
        currentChar++;
    }

    valueLength = currentIndex - startIndex;

    currentIndex++;
    *EnumerationKey = currentIndex;

    Variable->Name.Buffer = name;
    Variable->Name.Length = nameLength * sizeof(WCHAR);
    Variable->Value.Buffer = value;
    Variable->Value.Length = valueLength * sizeof(WCHAR);

    return STATUS_SUCCESS;
}

/**
 * Queries an environment variable as a string reference.
 *
 * \param Environment The environment block.
 * \param Name The name of the variable.
 * \param Value A pointer to a string reference that receives the value.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryEnvironmentVariableStringRef(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _Inout_opt_ PPH_STRINGREF Value
    )
{
    NTSTATUS status;
    SIZE_T returnLength;

    status = RtlQueryEnvironmentVariable(
        Environment,
        Name->Buffer,
        Name->Length / sizeof(WCHAR),
        Value ? Value->Buffer : NULL,
        Value ? Value->Length / sizeof(WCHAR) : 0,
        &returnLength
        );

    if (Value && NT_SUCCESS(status))
    {
        Value->Length = returnLength * sizeof(WCHAR);
    }

    return status;
}

/**
 * Queries an environment variable.
 *
 * \param Environment The environment block.
 * \param Name The name of the variable.
 * \param Value A pointer to a string that receives the value.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _Out_opt_ PPH_STRING* Value
    )
{
#ifdef PHNT_UNICODESTRING_ENVIRONMENTVARIABLE
    NTSTATUS status;
    UNICODE_STRING variableName;
    UNICODE_STRING variableValue;

    if (!PhStringRefToUnicodeString(Name, &variableName))
        return STATUS_NAME_TOO_LONG;

    if (Value)
    {
        variableValue.Length = 0x100 * sizeof(WCHAR);
        variableValue.MaximumLength = variableValue.Length + sizeof(UNICODE_NULL);
        variableValue.Buffer = PhAllocate(variableValue.MaximumLength);
    }
    else
    {
        RtlInitEmptyUnicodeString(&variableValue, NULL, 0);
    }

    status = RtlQueryEnvironmentVariable_U(
        Environment,
        &variableName,
        &variableValue
        );

    if (Value && status == STATUS_BUFFER_TOO_SMALL)
    {
        if (variableValue.Length + sizeof(UNICODE_NULL) > UNICODE_STRING_MAX_BYTES)
            variableValue.MaximumLength = variableValue.Length;
        else
            variableValue.MaximumLength = variableValue.Length + sizeof(UNICODE_NULL);

        PhFree(variableValue.Buffer);
        variableValue.Buffer = PhAllocate(variableValue.MaximumLength);

        status = RtlQueryEnvironmentVariable_U(
            Environment,
            &variableName,
            &variableValue
            );
    }

    if (Value && NT_SUCCESS(status))
    {
        *Value = PhCreateStringFromUnicodeString(&variableValue);
    }

    if (Value && variableValue.Buffer)
    {
        PhFree(variableValue.Buffer);
    }

    return status;
#else
    NTSTATUS status;
    PPH_STRING buffer;
    SIZE_T returnLength;

    status = RtlQueryEnvironmentVariable(
        Environment,
        Name->Buffer,
        Name->Length / sizeof(WCHAR),
        NULL,
        0,
        &returnLength
        );

    if (Value && status == STATUS_BUFFER_TOO_SMALL)
    {
        buffer = PhCreateStringEx(NULL, returnLength * sizeof(WCHAR));

        status = RtlQueryEnvironmentVariable(
            Environment,
            Name->Buffer,
            Name->Length / sizeof(WCHAR),
            buffer->Buffer,
            buffer->Length / sizeof(WCHAR),
            &returnLength
            );

        if (NT_SUCCESS(status))
        {
            buffer->Length = returnLength * sizeof(WCHAR);
            *Value = buffer;
        }
        else
        {
            PhDereferenceObject(buffer);
        }
    }
    else
    {
        if (Value)
            *Value = NULL;
    }

    return status;
#endif
}

/**
 * Sets an environment variable.
 *
 * \param Environment The environment block.
 * \param Name The name of the variable.
 * \param Value The value to set, or NULL to delete the variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_ PCPH_STRINGREF Name,
    _In_opt_ PCPH_STRINGREF Value
    )
{
    NTSTATUS status;
    UNICODE_STRING variableName;
    UNICODE_STRING variableValue;

    if (!PhStringRefToUnicodeString(Name, &variableName))
        return STATUS_NAME_TOO_LONG;

    if (Value)
    {
        if (!PhStringRefToUnicodeString(Value, &variableValue))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&variableValue, NULL, 0);
    }

    status = RtlSetEnvironmentVariable(
        Environment,
        &variableName,
        &variableValue
        );

    return status;
}

/**
 * Retrieves the file name for a mapped section in a process.
 *
 * \param SectionHandle The section handle.
 * \param ProcessHandle The process handle.
 * \param FileName A pointer to a string that receives the file name.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessSectionFileName(
    _In_ HANDLE SectionHandle,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID baseAddress;
    SIZE_T viewSize;

    baseAddress = NULL;
    viewSize = PAGE_SIZE;

    status = PhMapViewOfSection(
        SectionHandle,
        ProcessHandle,
        &baseAddress,
        0,
        NULL,
        &viewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcessMappedFileName(
        ProcessHandle,
        baseAddress,
        FileName
        );

    PhUnmapViewOfSection(ProcessHandle, baseAddress);

    return status;
}

/**
 * Retrieves the list of unloaded DLLs for a process.
 *
 * \param ProcessId The process ID.
 * \param EventTrace A pointer to the event trace buffer.
 * \param EventTraceSize The size of each event trace element.
 * \param EventTraceCount The number of event trace elements.
 */
NTSTATUS PhGetProcessUnloadedDlls(
    _In_ HANDLE ProcessId,
    _Out_ PVOID *EventTrace,
    _Out_ ULONG *EventTraceSize,
    _Out_ ULONG *EventTraceCount
    )
{
    NTSTATUS status;
    PULONG elementSize;
    PULONG elementCount;
    PVOID eventTrace;
    HANDLE processHandle = NULL;
    SIZE_T eventTraceSize;
    ULONG capturedElementSize = 0;
    ULONG capturedElementCount = 0;
    PVOID capturedEventTracePointer;
    PVOID capturedEventTrace = NULL;

    RtlGetUnloadEventTraceEx(&elementSize, &elementCount, &eventTrace);

    if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_VM_READ, ProcessId)))
        goto CleanupExit;

    // We have the pointers for the unload event trace information.
    // Since ntdll is loaded at the same base address across all processes,
    // we can read the information in.

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        processHandle,
        elementSize,
        &capturedElementSize,
        sizeof(ULONG),
        NULL
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        processHandle,
        elementCount,
        &capturedElementCount,
        sizeof(ULONG),
        NULL
        )))
        goto CleanupExit;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        processHandle,
        eventTrace,
        &capturedEventTracePointer,
        sizeof(PVOID),
        NULL
        )))
        goto CleanupExit;

    if (!capturedEventTracePointer)
    {
        status = STATUS_NOT_FOUND; // no events
        goto CleanupExit;
    }

    if (capturedElementCount > 0x4000)
        capturedElementCount = 0x4000;

    eventTraceSize = capturedElementSize * capturedElementCount;
    capturedEventTrace = PhAllocateSafe(eventTraceSize);

    if (!capturedEventTrace)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        processHandle,
        capturedEventTracePointer,
        capturedEventTrace,
        eventTraceSize,
        NULL
        )))
        goto CleanupExit;

CleanupExit:

    if (processHandle)
        NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        *EventTrace = capturedEventTrace;
        *EventTraceSize = capturedElementSize;
        *EventTraceCount = capturedElementCount;
    }
    else
    {
        if (capturedEventTrace)
            PhFree(capturedEventTrace);
    }

    return status;
}

/**
 * Sends a trace control request.
 *
 * \param TraceInformationClass The trace information class.
 * \param InputBuffer The input buffer.
 * \param InputBufferLength The length of the input buffer.
 * \param OutputBuffer The output buffer.
 * \param OutputBufferLength The length of the output buffer.
 * \param ReturnLength A pointer to a variable that receives the return length.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhTraceControl(
    _In_ ETWTRACECONTROLCODE TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;

    status = NtTraceControl(
        TraceInformationClass,
        InputBuffer,
        InputBufferLength,
        OutputBuffer,
        OutputBufferLength,
        ReturnLength
        );

    return status;
}

/**
 * Sends a trace control request with variable output size.
 *
 * \param TraceInformationClass The trace information class.
 * \param InputBuffer The input buffer.
 * \param InputBufferLength The length of the input buffer.
 * \param OutputBuffer A pointer to a buffer that receives the output.
 * \param OutputBufferLength A pointer to a variable that receives the output buffer length.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhTraceControlVariableSize(
    _In_ ETWTRACECONTROLCODE TraceInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(*OutputBufferLength) PVOID* OutputBuffer,
    _Out_opt_ PULONG OutputBufferLength
    )
{
    NTSTATUS status;
    PVOID buffer = NULL;
    ULONG bufferLength = 0;
    ULONG returnLength = 0;

    status = NtTraceControl(
        TraceInformationClass,
        InputBuffer,
        InputBufferLength,
        NULL,
        0,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        bufferLength = returnLength;
        buffer = PhAllocate(bufferLength);

        status = NtTraceControl(
            TraceInformationClass,
            InputBuffer,
            InputBufferLength,
            buffer,
            bufferLength,
            &bufferLength
            );
    }

    if (NT_SUCCESS(status))
    {
        if (OutputBuffer)
            *OutputBuffer = buffer;
        if (OutputBufferLength)
            *OutputBufferLength = bufferLength;
    }
    else
    {
        if (buffer)
            PhFree(buffer);
    }

    return status;
}

/**
 * Retrieves the client ID associated with the specified window handle.
 *
 * \param WindowHandle The handle to the window whose client ID is to be retrieved.
 * \param ClientId A pointer to a CLIENT_ID structure that receives the process and thread IDs.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetWindowClientId(
    _In_ HWND WindowHandle,
    _Out_ PCLIENT_ID ClientId
    )
{
    ULONG windowProcessId;
    ULONG windowThreadId;

    if (windowThreadId = GetWindowThreadProcessId(WindowHandle, &windowProcessId))
    {
        ClientId->UniqueProcess = UlongToHandle(windowProcessId);
        ClientId->UniqueThread = UlongToHandle(windowThreadId);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}

typedef struct _OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT
{
    NTSTATUS Status;
    PVOID BaseAddress;
    HANDLE DriverHandle;
} OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT, *POPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT;

_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
NTSTATUS NTAPI PhpOpenDriverByBaseAddressCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ POPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE driverHandle;
    UNICODE_STRING driverName;
    OBJECT_ATTRIBUTES objectAttributes;
    KPH_DRIVER_BASIC_INFORMATION basicInfo;

    if (!PhStringRefToUnicodeString(Name, &driverName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &driverName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = KphOpenDriver(
        &driverHandle,
        SYNCHRONIZE,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return status;

    status = KphQueryInformationDriver(
        driverHandle,
        KphDriverBasicInformation,
        &basicInfo,
        sizeof(KPH_DRIVER_BASIC_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (basicInfo.DriverStart == Context->BaseAddress)
        {
            Context->Status = STATUS_SUCCESS;
            Context->DriverHandle = driverHandle;
            return STATUS_SUCCESS;
        }
    }

    NtClose(driverHandle);

    return status;
}

/**
 * Opens a driver object using a base address.
 *
 * \param DriverHandle A variable which receives a handle to the driver object.
 * \param BaseAddress The base address of the driver to open.
 * \return NTSTATUS Successful or errant status.
 * \remarks STATUS_OBJECT_NAME_NOT_FOUND is returned if the driver could not be found.
 */
NTSTATUS PhOpenDriverByBaseAddress(
    _Out_ PHANDLE DriverHandle,
    _In_ PVOID BaseAddress
    )
{
    NTSTATUS status;
    HANDLE directoryHandle;
    UNICODE_STRING objectDirectory;
    OBJECT_ATTRIBUTES objectAttributes;
    OPEN_DRIVER_BY_BASE_ADDRESS_CONTEXT context;

    RtlInitUnicodeString(&objectDirectory, L"\\Driver");
    InitializeObjectAttributes(
        &objectAttributes,
        &objectDirectory,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return status;

    context.Status = STATUS_OBJECT_NAME_NOT_FOUND;
    context.BaseAddress = BaseAddress;

    status = PhEnumDirectoryObjects(
        directoryHandle,
        PhpOpenDriverByBaseAddressCallback,
        &context
        );

    NtClose(directoryHandle);

    if (NT_SUCCESS(status))
    {
        if (NT_SUCCESS(context.Status))
        {
            *DriverHandle = context.DriverHandle;
            return STATUS_SUCCESS;
        }

        return context.Status;
    }

    return status;
}

/**
 * Opens a handle to a driver object.
 *
 * \param DriverHandle Pointer to a variable that receives the handle to the driver object.
 * \param DesiredAccess Specifies the desired access rights to the driver object.
 * \param RootDirectory Optional handle to the root directory for the object name (can be NULL).
 * \param ObjectName Pointer to a string reference that specifies the name of the driver object.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenDriver(
    _Out_ PHANDLE DriverHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
    )
{
    if (KsiLevel() == KphLevelMax)
    {
        UNICODE_STRING objectName;
        OBJECT_ATTRIBUTES objectAttributes;

        if (!PhStringRefToUnicodeString(ObjectName, &objectName))
            return STATUS_NAME_TOO_LONG;

        InitializeObjectAttributes(
            &objectAttributes,
            &objectName,
            OBJ_CASE_INSENSITIVE,
            RootDirectory,
            NULL
            );

        return KphOpenDriver(
            DriverHandle,
            DesiredAccess,
            &objectAttributes
            );
    }
    else
    {
        return STATUS_NOT_IMPLEMENTED;
    }
}

/**
 * Queries variable-sized information for a driver. The function allocates a buffer to contain the
 * information.
 *
 * \param DriverHandle A handle to a driver. The access required depends on the information class specified.
 * \param DriverInformationClass The information class to retrieve.
 * \param Buffer A variable which receives a pointer to a buffer containing the information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhpQueryDriverVariableSize(
    _In_ HANDLE DriverHandle,
    _In_ KPH_DRIVER_INFORMATION_CLASS DriverInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer = NULL;
    ULONG bufferSize;
    ULONG returnLength = 0;

    status = KphQueryInformationDriver(
        DriverHandle,
        DriverInformationClass,
        NULL,
        0,
        &returnLength
        );

    if (NT_SUCCESS(status) && returnLength == 0)
    {
        return STATUS_NOT_FOUND;
    }

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = KphQueryInformationDriver(
            DriverHandle,
            DriverInformationClass,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        if (returnLength == 0)
        {
            status = STATUS_NOT_FOUND;
            PhFree(buffer);
        }
        else
        {
            *Buffer = buffer;
        }
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Gets the object name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param Name A variable which receives a pointer to a string containing the object name.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetDriverName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        KphDriverNameInformation,
        &unicodeString
        )))
        return status;

    *Name = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

/**
 * Gets the object name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param Name A variable which receives a pointer to a string containing the driver image file name.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetDriverImageFileName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *Name
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        KphDriverImageFileNameInformation,
        &unicodeString
        )))
        return status;

    *Name = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

/**
 * Gets the service key name of a driver.
 *
 * \param DriverHandle A handle to a driver.
 * \param ServiceKeyName A string containing the service key name.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetDriverServiceKeyName(
    _In_ HANDLE DriverHandle,
    _Out_ PPH_STRING *ServiceKeyName
    )
{
    NTSTATUS status;
    PUNICODE_STRING unicodeString;

    if (!NT_SUCCESS(status = PhpQueryDriverVariableSize(
        DriverHandle,
        KphDriverServiceKeyNameInformation,
        &unicodeString
        )))
        return status;

    *ServiceKeyName = PhCreateStringFromUnicodeString(unicodeString);
    PhFree(unicodeString);

    return status;
}

static NTSTATUS PhpUnloadDriver(
    _In_ PCPH_STRINGREF ServiceKeyName,
    _In_ PCPH_STRINGREF DriverFileName
    )
{
    static CONST PH_STRINGREF fullServicesKeyName = PH_STRINGREF_INIT(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    NTSTATUS status;
    PPH_STRING fullServiceKeyName;
    UNICODE_STRING fullServiceKeyNameUs;
    HANDLE serviceKeyHandle;
    ULONG disposition;

    fullServiceKeyName = PhConcatStringRef2(&fullServicesKeyName, ServiceKeyName);

    if (!PhStringRefToUnicodeString(&fullServiceKeyName->sr, &fullServiceKeyNameUs))
    {
        PhDereferenceObject(fullServiceKeyName);
        return STATUS_NAME_TOO_LONG;
    }

    if (NT_SUCCESS(status = PhCreateKey(
        &serviceKeyHandle,
        KEY_WRITE | DELETE,
        NULL,
        &fullServiceKeyName->sr,
        0,
        0,
        &disposition
        )))
    {
        if (disposition == REG_CREATED_NEW_KEY)
        {
            ULONG regValue = 0;

            // Set up the required values.
            PhSetValueKeyZ(serviceKeyHandle, L"ErrorControl", REG_DWORD, &regValue, sizeof(ULONG));
            PhSetValueKeyZ(serviceKeyHandle, L"Start", REG_DWORD, &regValue, sizeof(ULONG));
            PhSetValueKeyZ(serviceKeyHandle, L"Type", REG_DWORD, &regValue, sizeof(ULONG));
            PhSetValueKeyZ(serviceKeyHandle, L"ImagePath", REG_SZ, DriverFileName->Buffer, (ULONG)DriverFileName->Length + sizeof(UNICODE_NULL));
        }

        status = NtUnloadDriver(&fullServiceKeyNameUs);

        if (disposition == REG_CREATED_NEW_KEY)
        {
            // We added values, not subkeys, so this function will work correctly.
            NtDeleteKey(serviceKeyHandle);
        }

        NtClose(serviceKeyHandle);
    }

    PhDereferenceObject(fullServiceKeyName);

    return status;
}

/**
 * Unloads a driver.
 *
 * \param BaseAddress The base address of the driver. This parameter can be NULL if a value is
 * specified in \c Name.
 * \param Name The base name of the driver. This parameter can be NULL if a value is specified in
 * \c BaseAddress and KSystemInformer is loaded.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INVALID_PARAMETER_MIX Both \c BaseAddress and \c Name were null, or \c Name was
 * not specified and KSystemInformer is not loaded.
 * \retval STATUS_OBJECT_NAME_NOT_FOUND The driver could not be found.
 */
NTSTATUS PhUnloadDriver(
    _In_opt_ PVOID BaseAddress,
    _In_opt_ PCPH_STRINGREF Name,
    _In_ PCPH_STRINGREF FileName
    )
{
    NTSTATUS status;
    HANDLE driverHandle;
    PPH_STRING serviceKeyName = NULL;
    KPH_LEVEL level;

    level = KsiLevel();

    if (!BaseAddress && !Name)
        return STATUS_INVALID_PARAMETER_MIX;
    if (!Name && (level != KphLevelMax))
        return STATUS_INVALID_PARAMETER_MIX;

    // Try to get the service key name by scanning the Driver directory.

    if ((level == KphLevelMax) && BaseAddress)
    {
        if (PhGetOwnTokenAttributes().Elevated)
        {
            if (NT_SUCCESS(PhOpenDriverByBaseAddress(
                &driverHandle,
                BaseAddress
                )))
            {
                PhGetDriverServiceKeyName(driverHandle, &serviceKeyName);
                NtClose(driverHandle);
            }
        }
    }

    // Use the base name if we didn't get the service key name.

    if (!serviceKeyName && Name)
    {
        PPH_STRING name;

        name = PhCreateString2(Name);

        // Remove the extension if it is present.
        if (PhEndsWithString2(name, L".sys", TRUE))
        {
            serviceKeyName = PhSubstring(name, 0, name->Length / sizeof(WCHAR) - 4);
            PhDereferenceObject(name);
        }
        else
        {
            serviceKeyName = name;
        }
    }

    if (!serviceKeyName)
        return STATUS_OBJECT_NAME_NOT_FOUND;

    status = PhpUnloadDriver(&serviceKeyName->sr, FileName);
    PhDereferenceObject(serviceKeyName);

    return status;
}

/**
 * Enumerates the running processes.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcesses(
    _Out_ PVOID *Processes
    )
{
    return PhEnumProcessesEx(Processes, SystemProcessInformation);
}

/**
 * Enumerates the running processes.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 * \param SystemInformationClass A variable which indicates the kind of system information to be retrieved.
 * \return NTSTATUS Successful or errant status.
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcessesEx(
    _Out_ PVOID *Processes,
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass
    )
{
    static ULONG initialBufferSize[3] = { 0x4000, 0x4000, 0x4000 };
    NTSTATUS status;
    ULONG classIndex;
    PVOID buffer;
    ULONG bufferSize;

    switch (SystemInformationClass)
    {
    case SystemProcessInformation:
        classIndex = 0;
        break;
    case SystemExtendedProcessInformation:
        classIndex = 1;
        break;
    case SystemFullProcessInformation:
        classIndex = 2;
        break;
    default:
        return STATUS_INVALID_INFO_CLASS;
    }

    bufferSize = initialBufferSize[classIndex];
    buffer = PhAllocateSafe(bufferSize);
    if (!buffer) return STATUS_NO_MEMORY;

    while (TRUE)
    {
        status = NtQuerySystemInformation(
            SystemInformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocateSafe(bufferSize);
            if (!buffer) return STATUS_NO_MEMORY;
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize[classIndex] = bufferSize;
    *Processes = buffer;

    return status;
}

/**
 * Enumerates the threads of a running process.
 *
 * \param ProcessId The ID of the process.
 * \param Callback The callback function to be called for each enumerated thread.
 * \param Context An optional context parameter to be passed to the callback function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumProcessThreads(
    _In_ HANDLE ProcessId,
    _In_ PPH_ENUM_PROCESS_THREADS Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;

    status = PhEnumProcesses(&processes);

    if (!NT_SUCCESS(status))
        return status;

    if (process = PhFindProcessInformation(processes, ProcessId))
    {
        status = Callback(
            process->NumberOfThreads,
            process->Threads,
            Context
            );
    }
    else
    {
        status = STATUS_INVALID_CID;
    }

    PhFree(processes);

    return status;
}

/**
 * Enumerates the next process.
 *
 * \param ProcessHandle The handle to the current process. Pass NULL to start enumeration from the beginning.
 * \param DesiredAccess The desired access rights for the process handle.
 * \param Callback The callback function to be called for each enumerated process.
 * \param Context An optional context parameter to be passed to the callback function.
 * \return Returns the status of the enumeration operation.
 *         If the enumeration is successful, it returns STATUS_SUCCESS.
 *         If there are no more processes to enumerate, it returns STATUS_NO_MORE_ENTRIES.
 *         Otherwise, it returns an appropriate NTSTATUS error code.
 */
NTSTATUS PhEnumNextProcess(
    _In_opt_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_ENUM_NEXT_PROCESS Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE newProcessHandle;

    status = NtGetNextProcess(
        ProcessHandle,
        DesiredAccess,
        0,
        0,
        &processHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    while (TRUE)
    {
        status = Callback(processHandle, Context);

        if (status == STATUS_NO_MORE_ENTRIES)
            break;

        if (!NT_SUCCESS(status))
        {
            NtClose(processHandle);
            break;
        }

        status = NtGetNextProcess(
            processHandle,
            DesiredAccess,
            0,
            0,
            &newProcessHandle
            );

        if (NT_SUCCESS(status))
        {
            NtClose(processHandle);
            processHandle = newProcessHandle;
        }
        else
        {
            NtClose(processHandle);
            break;
        }
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Enumerates the next thread.
 *
 * \param ProcessHandle The handle to the process.
 * \param ThreadHandle The handle to the current thread. Pass NULL to start enumeration from the beginning.
 * \param DesiredAccess The desired access rights for the thread handle.
 * \param Callback The callback function to be called for each enumerated thread.
 * \param Context An optional context parameter to be passed to the callback function.
 * \return Returns the status of the enumeration operation.
 *         If the enumeration is successful, it returns STATUS_SUCCESS.
 *         If there are no more threads to enumerate, it returns STATUS_NO_MORE_ENTRIES.
 *         Otherwise, it returns an appropriate NTSTATUS error code.
 */
NTSTATUS PhEnumNextThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PPH_ENUM_NEXT_THREAD Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    HANDLE newThreadHandle;

    status = NtGetNextThread(
        ProcessHandle,
        ThreadHandle,
        DesiredAccess,
        0,
        0,
        &threadHandle
        );

    if (!NT_SUCCESS(status))
        return status;

    while (TRUE)
    {
        status = Callback(threadHandle, Context);

        if (status == STATUS_NO_MORE_ENTRIES)
            break;

        if (!NT_SUCCESS(status))
        {
            NtClose(threadHandle);
            break;
        }

        status = NtGetNextThread(
            ProcessHandle,
            threadHandle,
            DesiredAccess,
            0,
            0,
            &newThreadHandle
            );

        if (NT_SUCCESS(status))
        {
            NtClose(threadHandle);
            threadHandle = newThreadHandle;
        }
        else
        {
            NtClose(threadHandle);
            break;
        }
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Enumerates the running processes for a session.
 *
 * \param Processes A variable which receives a pointer to a buffer containing process information.
 * You must free the buffer using PhFree() when you no longer need it.
 * \param SessionId A session ID.
 * \return NTSTATUS Successful or errant status.
 * \remarks You can use the \ref PH_FIRST_PROCESS and \ref PH_NEXT_PROCESS macros to process the
 * information contained in the buffer.
 */
NTSTATUS PhEnumProcessesForSession(
    _Out_ PVOID *Processes,
    _In_ ULONG SessionId
    )
{
    static ULONG initialBufferSize = 0x4000;
    NTSTATUS status;
    SYSTEM_SESSION_PROCESS_INFORMATION sessionProcessInfo;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    sessionProcessInfo.SessionId = SessionId;

    while (TRUE)
    {
        sessionProcessInfo.BufferSize = bufferSize;
        sessionProcessInfo.Buffer = buffer;

        status = NtQuerySystemInformation(
            SystemSessionProcessInformation,
            &sessionProcessInfo,
            sizeof(SYSTEM_SESSION_PROCESS_INFORMATION),
            &bufferSize // size of the inner buffer gets returned
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize = bufferSize;
    *Processes = buffer;

    return status;
}

/**
 * Finds the process information structure for a specific process.
 *
 * \param Processes A pointer to a buffer returned by PhEnumProcesses().
 * \param ProcessId The ID of the process.
 * \return A pointer to the process information structure for the specified process, or NULL if the
 * structure could not be found.
 */
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformation(
    _In_ PVOID Processes,
    _In_ HANDLE ProcessId
    )
{
    PSYSTEM_PROCESS_INFORMATION process;

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        if (process->UniqueProcessId == ProcessId)
            return process;
    } while (process = PH_NEXT_PROCESS(process));

    return NULL;
}

/**
 * Finds the process information structure for a specific process.
 *
 * \param Processes A pointer to a buffer returned by PhEnumProcesses().
 * \param ImageName The image name to search for.
 * \return A pointer to the process information structure for the specified process, or NULL if the
 * structure could not be found.
 */
PSYSTEM_PROCESS_INFORMATION PhFindProcessInformationByImageName(
    _In_ PVOID Processes,
    _In_ PCPH_STRINGREF ImageName
    )
{
    PSYSTEM_PROCESS_INFORMATION process;
    PH_STRINGREF processImageName;

    process = PH_FIRST_PROCESS(Processes);

    do
    {
        PhUnicodeStringToStringRef(&process->ImageName, &processImageName);

        if (PhEqualStringRef(&processImageName, ImageName, TRUE))
            return process;
    } while (process = PH_NEXT_PROCESS(process));

    return NULL;
}

/**
 * Enumerates all open handles.
 *
 * \param Handles A variable which receives a pointer to a structure containing information about
 * all opened handles. You must free the structure using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 */
__declspec(deprecated("The SystemHandleInformation class is deprecated on 64-bit Windows. Use PhEnumHandlesEx instead."))
NTSTATUS PhEnumHandles(
    _Out_ PSYSTEM_HANDLE_INFORMATION *Handles
    )
{
    static ULONG initialBufferSize = 0x10000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemHandleInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x200000) initialBufferSize = bufferSize;
    *Handles = (PSYSTEM_HANDLE_INFORMATION)buffer;

    return status;
}

/**
 * Enumerates all open handles.
 *
 * \param Handles A variable which receives a pointer to a structure containing information about
 * all opened handles. You must free the structure using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 * \remarks This function is only available starting with Windows XP.
 */
NTSTATUS PhEnumHandlesEx(
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles
    )
{
    static ULONG initialBufferSize = 0x10000;
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength = 0;
    ULONG attempts = 0;

    bufferSize = initialBufferSize;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemExtendedHandleInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 10)
    {
        PhFree(buffer);
        bufferSize = returnLength;
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemExtendedHandleInformation,
            buffer,
            bufferSize,
            &returnLength
            );

        attempts++;
    }

    if (!NT_SUCCESS(status))
    {
        // Fall back to using the previous code that we've used since Windows XP (dmex)
        bufferSize = initialBufferSize;
        buffer = PhAllocate(bufferSize);

        while ((status = NtQuerySystemInformation(
            SystemExtendedHandleInformation,
            buffer,
            bufferSize,
            NULL
            )) == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            bufferSize *= 2;

            // Fail if we're resizing the buffer to something very large.
            if (bufferSize > PH_LARGE_BUFFER_SIZE)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferSize);
        }
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x200000) initialBufferSize = bufferSize;
    *Handles = (PSYSTEM_HANDLE_INFORMATION_EX)buffer;

    return status;
}

/**
 * Enumerates all open handles.
 *
 * \param ProcessHandle A handle to the process. The handle must have PROCESS_QUERY_INFORMATION access.
 * \param Handles A variable which receives a pointer to a structure containing information about
 * handles opened by the process. You must free the structure using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 * \retval STATUS_INSUFFICIENT_RESOURCES The handle information returned by the kernel is too large.
 * \remarks This function is only available starting with Windows 8.
 */
NTSTATUS PhEnumProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_ PPROCESS_HANDLE_SNAPSHOT_INFORMATION *Handles
    )
{
    NTSTATUS status;
    PPROCESS_HANDLE_SNAPSHOT_INFORMATION buffer;
    ULONG bufferSize;
    ULONG returnLength = 0;
    ULONG attempts = 0;

    bufferSize = 0x8000;
    buffer = PhAllocatePage(bufferSize, NULL);
    if (!buffer) return STATUS_NO_MEMORY;
    buffer->NumberOfHandles = 0;

    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessHandleInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFreePage(buffer);
        bufferSize = returnLength;
        buffer = PhAllocatePage(bufferSize, NULL);
        if (!buffer) return STATUS_NO_MEMORY;
        buffer->NumberOfHandles = 0;

        status = NtQueryInformationProcess(
            ProcessHandle,
            ProcessHandleInformation,
            buffer,
            bufferSize,
            &returnLength
            );

        attempts++;
    }

    if (NT_SUCCESS(status))
    {
        // NOTE: This is needed to workaround minimal processes on Windows 10
        // returning STATUS_SUCCESS with invalid handle data. (dmex)
        // NOTE: 21H1 and above no longer set NumberOfHandles to zero before returning
        // STATUS_SUCCESS so we first zero the entire buffer using PhAllocateZero. (dmex)
        if (buffer->NumberOfHandles == 0)
        {
            status = STATUS_UNSUCCESSFUL;
            PhFreePage(buffer);
        }
        else
        {
            *Handles = buffer;
        }
    }
    else
    {
        PhFreePage(buffer);
    }

    return status;
}

/**
 * Enumerates all handles in a process.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle A handle to the process.
 * \param EnableHandleSnapshot TRUE to return a snapshot of the process handles.
 * \param Handles A variable which receives a pointer to a buffer containing
 * information about the handles.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumHandlesGeneric(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN EnableHandleSnapshot,
    _Out_ PSYSTEM_HANDLE_INFORMATION_EX *Handles
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    // There are three ways of enumerating handles:
    // * On Windows 8 and later, NtQueryInformationProcess with ProcessHandleInformation is the most efficient method.
    // * On Windows XP and later, NtQuerySystemInformation with SystemExtendedHandleInformation.
    // * Otherwise, NtQuerySystemInformation with SystemHandleInformation can be used.

    if ((KsiLevel() >= KphLevelMed) && ProcessHandle)
    {
        PKPH_PROCESS_HANDLE_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        // Enumerate handles using KSystemInformer. Unlike with NtQuerySystemInformation,
        // this only enumerates handles for a single process and saves a lot of processing.

        if (NT_SUCCESS(status = KsiEnumerateProcessHandles(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[handles->HandleCount]));
            convertedHandles->NumberOfHandles = handles->HandleCount;

            for (i = 0; i < handles->HandleCount; i++)
            {
                PKPH_PROCESS_HANDLE handle = &handles->Handles[i];

                convertedHandles->Handles[i].Object = handle->Object;
                convertedHandles->Handles[i].UniqueProcessId = ProcessId;
                convertedHandles->Handles[i].HandleValue = handle->Handle;
                convertedHandles->Handles[i].GrantedAccess = handle->GrantedAccess;
                convertedHandles->Handles[i].CreatorBackTraceIndex = 0;
                convertedHandles->Handles[i].ObjectTypeIndex = handle->ObjectTypeIndex;
                convertedHandles->Handles[i].HandleAttributes = handle->HandleAttributes;
            }

            PhFree(handles);

            *Handles = convertedHandles;
        }
    }

    if (!NT_SUCCESS(status) && WindowsVersion >= WINDOWS_8 && ProcessHandle && EnableHandleSnapshot)
    {
        PPROCESS_HANDLE_SNAPSHOT_INFORMATION handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i;

        if (NT_SUCCESS(status = PhEnumProcessHandles(ProcessHandle, &handles)))
        {
            convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[handles->NumberOfHandles]));
            convertedHandles->NumberOfHandles = handles->NumberOfHandles;

            for (i = 0; i < handles->NumberOfHandles; i++)
            {
                PPROCESS_HANDLE_TABLE_ENTRY_INFO handle = &handles->Handles[i];

                convertedHandles->Handles[i].Object = NULL;
                convertedHandles->Handles[i].UniqueProcessId = ProcessId;
                convertedHandles->Handles[i].HandleValue = handle->HandleValue;
                convertedHandles->Handles[i].GrantedAccess = handle->GrantedAccess;
                convertedHandles->Handles[i].CreatorBackTraceIndex = 0;
                convertedHandles->Handles[i].ObjectTypeIndex = (USHORT)handle->ObjectTypeIndex;
                convertedHandles->Handles[i].HandleAttributes = handle->HandleAttributes;
            }

            PhFreePage(handles);

            *Handles = convertedHandles;
        }
    }

    if (!NT_SUCCESS(status))
    {
        PSYSTEM_HANDLE_INFORMATION_EX handles;
        PSYSTEM_HANDLE_INFORMATION_EX convertedHandles;
        ULONG i, numberOfHandles = 0;
        ULONG firstIndex = 0, lastIndex = 0;

        if (NT_SUCCESS(status = PhEnumHandlesEx(&handles)))
        {
            for (i = 0; i < handles->NumberOfHandles; i++)
            {
                PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = &handles->Handles[i];

                if (handle->UniqueProcessId == ProcessId)
                {
                    if (numberOfHandles == 0)
                        firstIndex = i;
                    lastIndex = i;
                    numberOfHandles++;
                }
            }

            if (numberOfHandles)
            {
                convertedHandles = PhAllocate(UFIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handles[numberOfHandles]));
                convertedHandles->NumberOfHandles = numberOfHandles;

                if (lastIndex == firstIndex + numberOfHandles - 1) // consecutive
                {
                    memcpy(
                        &convertedHandles->Handles[0],
                        &handles->Handles[firstIndex],
                        numberOfHandles * sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX)
                        );

                    *Handles = convertedHandles;
                }
                else
                {
                    ULONG j = 0;

                    for (i = firstIndex; i <= lastIndex; i++)
                    {
                        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handle = &handles->Handles[i];

                        if (handle->UniqueProcessId == ProcessId)
                        {
                            memcpy(&convertedHandles->Handles[j++], handle, sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX));
                        }
                    }

                    *Handles = convertedHandles;
                }
            }
            else
            {
                status = STATUS_NOT_FOUND;
            }

            PhFree(handles);
        }

        return status;
    }

    return status;
}

/**
 * Enumerates all pagefiles.
 *
 * \param Pagefiles A variable which receives a pointer to a buffer containing information about all
 * active pagefiles. You must free the structure using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumPagefiles(
    _Out_ PVOID *Pagefiles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemPageFileInformation,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Pagefiles = buffer;

    return status;
}

/**
 * Enumerates all pagefiles.
 *
 * \param Pagefiles A variable which receives a pointer to a buffer containing information about all
 * active pagefiles. You must free the structure using PhFree() when you no longer need it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumPagefilesEx(
    _Out_ PVOID *Pagefiles
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize = 0x200;

    buffer = PhAllocate(bufferSize);

    while ((status = NtQuerySystemInformation(
        SystemPageFileInformationEx,
        buffer,
        bufferSize,
        NULL
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *Pagefiles = buffer;

    return status;
}

/**
 * Enumerates pool tag information.
 *
 * \param Buffer A pointer to a buffer that will receive the pool tag information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumPoolTagInformation(
    _Out_ PVOID* Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemPoolTagInformation,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemPoolTagInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

/**
 * Enumerates information about the big pool allocations in the system.
 *
 * \param Buffer A pointer to a variable that receives the buffer containing the big pool information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumBigPoolInformation(
    _Out_ PVOID* Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemBigPoolInformation,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        buffer = PhReAllocate(buffer, bufferSize);

        status = NtQuerySystemInformation(
            SystemBigPoolInformation,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

typedef struct _PH_IS_CONTAINER_CONTEXT
{
    HANDLE ProcessObject;
    BOOLEAN Found;
} PH_IS_CONTAINER_CONTEXT, *PPH_IS_CONTAINER_CONTEXT;

_Function_class_(PH_ENUM_KEY_CALLBACK)
static BOOLEAN NTAPI PhEnumHostComputeServiceKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PVOID Information,
    _In_ PVOID Context
    )
{
    static CONST PH_STRINGREF objectNamePrefix = PH_STRINGREF_INIT(L"Container_");
    PKEY_BASIC_INFORMATION basicInfo = (PKEY_BASIC_INFORMATION)Information;
    PPH_IS_CONTAINER_CONTEXT context = (PPH_IS_CONTAINER_CONTEXT)Context;
    PH_STRINGREF keyName;
    PPH_STRING string;

    keyName.Buffer = basicInfo->Name;
    keyName.Length = basicInfo->NameLength;

    if (string = PhConcatStringRef2(&objectNamePrefix, &keyName))
    {
        HANDLE objectHandle;

        if (NT_SUCCESS(PhOpenJobObject(
            &objectHandle,
            JOB_OBJECT_QUERY,
            NULL,
            &string->sr
            )))
        {
            PJOBOBJECT_BASIC_PROCESS_ID_LIST processIdList;

            if (NT_SUCCESS(PhGetJobProcessIdList(objectHandle, &processIdList)))
            {
                for (ULONG i = 0; i < processIdList->NumberOfProcessIdsInList; i++)
                {
                    if (context->ProcessObject == (HANDLE)processIdList->ProcessIdList[i])
                    {
                        context->Found = TRUE;
                        break;
                    }
                }

                PhFree(processIdList);
            }

            // JobObjectAssociateCompletionPortInformation for process start/stop notifications (dmex)

            NtClose(objectHandle);
        }
    }

    if (context->Found)
        return FALSE;

    return TRUE;
}

NTSTATUS PhEnumHostComputeService(
    _In_ HANDLE ProcessId
    )
{
    static CONST PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\HostComputeService\\VolatileStore\\ComputeSystem");
    NTSTATUS status;
    HANDLE keyHandle;

    status = PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName,
        0
        );

    if (NT_SUCCESS(status))
    {
        PH_IS_CONTAINER_CONTEXT context;

        context.ProcessObject = ProcessId;
        context.Found = FALSE;

        status = PhEnumerateKey(
            keyHandle,
            KeyBasicInformation,
            PhEnumHostComputeServiceKeyCallback,
            &context
            );

        NtClose(keyHandle);
    }

    return status;
}

_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
static NTSTATUS NTAPI PhIsContainerEnumCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PVOID Context
    )
{
    PPH_IS_CONTAINER_CONTEXT context = (PPH_IS_CONTAINER_CONTEXT)Context;
    HANDLE objectHandle;

    if (!PhEqualStringRef2(TypeName, L"Job", FALSE))
        return STATUS_NAME_TOO_LONG;

    if (NT_SUCCESS(PhOpenJobObject(
        &objectHandle,
        JOB_OBJECT_QUERY,
        RootDirectory,
        Name
        )))
    {
        if (NT_SUCCESS(NtIsProcessInJob(
            context->ProcessObject,
            objectHandle
            )))
        {
            context->Found = TRUE;
            NtClose(objectHandle);
            return STATUS_NO_MORE_ENTRIES;
        }

        NtClose(objectHandle);
    }

    return STATUS_SUCCESS;
}

/**
 * Determines if a process is managed.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle A handle to the process.
 * \param IsContainer A variable which receives a boolean indicating whether the process is managed.
 * \return NTSTATUS Successful or errant status.
 */
_Use_decl_annotations_
NTSTATUS PhGetProcessIsContainer(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _Out_opt_ PBOOLEAN IsContainer
    )
{
    {
        static CONST PH_STRINGREF keyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\HostComputeService\\VolatileStore\\ComputeSystem");
        HANDLE keyHandle;
        PH_IS_CONTAINER_CONTEXT context;

        context.ProcessObject = NULL;
        context.Found = FALSE;

        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_READ,
            PH_KEY_LOCAL_MACHINE,
            &keyName,
            0
            )))
        {
            PhEnumerateKey(
                keyHandle,
                KeyBasicInformation,
                PhEnumHostComputeServiceKeyCallback,
                &context
                );

            NtClose(keyHandle);
        }

        if (context.Found)
        {
            if (IsContainer)
                *IsContainer = TRUE;
            return STATUS_SUCCESS;
        }
    }

    {
        static CONST PH_STRINGREF directoryName = PH_STRINGREF_INIT(L"\\");
        NTSTATUS status;
        HANDLE directoryHandle;
        HANDLE processHandle;
        PH_IS_CONTAINER_CONTEXT context;

        context.ProcessObject = NULL;
        context.Found = FALSE;

        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            ProcessId
            );

        if (NT_SUCCESS(status))
        {
            status = PhOpenDirectoryObject(
                &directoryHandle,
                DIRECTORY_QUERY,
                NULL,
                &directoryName
                );

            if (NT_SUCCESS(status))
            {
                context.ProcessObject = processHandle;

                status = PhEnumDirectoryObjects(
                    directoryHandle,
                    PhIsContainerEnumCallback,
                    NULL
                    );

                NtClose(directoryHandle);
            }

            NtClose(processHandle);
        }

        if (context.Found)
        {
            if (IsContainer)
                *IsContainer = TRUE;
            return STATUS_SUCCESS;
        }

        if (IsContainer)
            *IsContainer = FALSE;
        return status;
    }
}

/**
 * Determines if a process is managed.
 *
 * \param ProcessId The ID of the process.
 * \param IsDotNet A variable which receives a boolean indicating whether the process is managed.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessIsDotNet(
    _In_ HANDLE ProcessId,
    _Out_ PBOOLEAN IsDotNet
    )
{
    return PhGetProcessIsDotNetEx(ProcessId, NULL, 0, IsDotNet, NULL);
}

_Function_class_(PH_ENUM_PROCESS_MODULES_CALLBACK)
static BOOLEAN NTAPI PhpIsDotNetEnumProcessModulesCallback(
    _In_ PLDR_DATA_TABLE_ENTRY Module,
    _In_ PVOID Context
    )
{
    static CONST PH_STRINGREF clrString = PH_STRINGREF_INIT(L"clr.dll");
    static CONST PH_STRINGREF clrcoreString = PH_STRINGREF_INIT(L"coreclr.dll");
    static CONST PH_STRINGREF mscorwksString = PH_STRINGREF_INIT(L"mscorwks.dll");
    static CONST PH_STRINGREF mscorsvrString = PH_STRINGREF_INIT(L"mscorsvr.dll");
    static CONST PH_STRINGREF mscorlibString = PH_STRINGREF_INIT(L"mscorlib.dll");
    static CONST PH_STRINGREF mscorlibNiString = PH_STRINGREF_INIT(L"mscorlib.ni.dll");
    static CONST PH_STRINGREF frameworkString = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework\\");
    static CONST PH_STRINGREF framework64String = PH_STRINGREF_INIT(L"\\Microsoft.NET\\Framework64\\");
    PH_STRINGREF baseDllName;

    PhUnicodeStringToStringRef(&Module->BaseDllName, &baseDllName);

    if (
        PhEqualStringRef(&baseDllName, &clrString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorwksString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorsvrString, TRUE)
        )
    {
        PH_STRINGREF fileName;
        PH_STRINGREF systemRoot;
        PCPH_STRINGREF frameworkPart;

#ifdef _WIN64
        if (*(PULONG)Context & PH_CLR_PROCESS_IS_WOW64)
        {
#endif
            frameworkPart = &frameworkString;
#ifdef _WIN64
        }
        else
        {
            frameworkPart = &framework64String;
        }
#endif

        PhUnicodeStringToStringRef(&Module->FullDllName, &fileName);
        PhGetSystemRoot(&systemRoot);

        if (PhStartsWithStringRef(&fileName, &systemRoot, TRUE))
        {
            fileName.Buffer = PTR_ADD_OFFSET(fileName.Buffer, systemRoot.Length);
            fileName.Length -= systemRoot.Length;

            if (PhStartsWithStringRef(&fileName, frameworkPart, TRUE))
            {
                fileName.Buffer = PTR_ADD_OFFSET(fileName.Buffer, frameworkPart->Length);
                fileName.Length -= frameworkPart->Length;

                if (fileName.Length >= 4 * sizeof(WCHAR)) // vx.x
                {
                    if (fileName.Buffer[1] == L'1')
                    {
                        if (fileName.Buffer[3] == L'0')
                            *(PULONG)Context |= PH_CLR_VERSION_1_0;
                        else if (fileName.Buffer[3] == L'1')
                            *(PULONG)Context |= PH_CLR_VERSION_1_1;
                    }
                    else if (fileName.Buffer[1] == L'2')
                    {
                        *(PULONG)Context |= PH_CLR_VERSION_2_0;
                    }
                    else if (fileName.Buffer[1] >= L'4' && fileName.Buffer[1] <= L'9')
                    {
                        *(PULONG)Context |= PH_CLR_VERSION_4_ABOVE;
                    }
                }
            }
        }
    }
    else if (
        PhEqualStringRef(&baseDllName, &mscorlibString, TRUE) ||
        PhEqualStringRef(&baseDllName, &mscorlibNiString, TRUE)
        )
    {
        *(PULONG)Context |= PH_CLR_MSCORLIB_PRESENT;
    }
    else if (PhEqualStringRef(&baseDllName, &clrcoreString, TRUE))
    {
        *(PULONG)Context |= PH_CLR_CORELIB_PRESENT;
    }

    return TRUE;
}

typedef struct _PHP_PIPE_NAME_HASH
{
    ULONG NameHash;
    ULONG Found;
} PHP_PIPE_NAME_HASH, *PPHP_PIPE_NAME_HASH;

_Function_class_(PH_ENUM_DIRECTORY_FILE)
static BOOLEAN NTAPI PhpDotNetCorePipeHashCallback(
    _In_ HANDLE RootDirectory,
    _In_ PFILE_DIRECTORY_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PPHP_PIPE_NAME_HASH context = Context;
    PH_STRINGREF objectName;

    objectName.Length = Information->FileNameLength;
    objectName.Buffer = Information->FileName;

    if (PhHashStringRefEx(&objectName, FALSE, PH_STRING_HASH_X65599) == context->NameHash)
    {
        context->Found = TRUE;
        return FALSE;
    }

    return TRUE;
}

/**
 * Determines if a process is managed.
 *
 * \param ProcessId The ID of the process.
 * \param ProcessHandle An optional handle to the process. The handle must have
 * PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ access.
 * \param InFlags A combination of flags.
 * \li \c PH_CLR_USE_SECTION_CHECK Checks for the existence of related section objects to determine
 * whether the process is managed.
 * \li \c PH_CLR_NO_WOW64_CHECK Instead of a separate query, uses the presence of the
 * \c PH_CLR_KNOWN_IS_WOW64 flag to determine whether the process is running under WOW64.
 * \li \c PH_CLR_KNOWN_IS_WOW64 When \c PH_CLR_NO_WOW64_CHECK is specified, indicates that the
 * process is managed.
 * \param IsDotNet A variable which receives a boolean indicating whether the process is managed.
 * \param Flags A variable which receives additional flags.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessIsDotNetEx(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_ ULONG InFlags,
    _Out_opt_ PBOOLEAN IsDotNet,
    _Out_opt_ PULONG Flags
    )
{
    if (InFlags & PH_CLR_USE_SECTION_CHECK)
    {
        NTSTATUS status;
        HANDLE sectionHandle;
        SIZE_T returnLength;
        PH_STRINGREF objectName;
        PH_FORMAT format[2];
        WCHAR formatBuffer[0x80];

        // Most .NET processes have a handle open to a section named
        // \BaseNamedObjects\Cor_Private_IPCBlock(_v4)_<ProcessId>. This is the same object used by
        // the ICorPublish::GetProcess function. Instead of calling that function, we simply check
        // for the existence of that section object. This means:
        // * Better performance.
        // * No need for admin rights to get .NET status of processes owned by other users.

        // Version 4 section object

        PhInitFormatS(&format[0], L"\\BaseNamedObjects\\Cor_Private_IPCBlock_v4_");
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            objectName.Length = returnLength - sizeof(UNICODE_NULL);
            objectName.Buffer = formatBuffer;
        }
        else
        {
            return STATUS_NO_MEMORY;
        }

        status = PhOpenSection(
            &sectionHandle,
            SECTION_QUERY,
            NULL,
            &objectName
            );

        if (NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED)
        {
            if (NT_SUCCESS(status))
                NtClose(sectionHandle);

            if (IsDotNet)
                *IsDotNet = TRUE;

            if (Flags)
                *Flags = PH_CLR_VERSION_4_ABOVE;

            return STATUS_SUCCESS;
        }

        // Version 2 section object

        PhInitFormatS(&format[0], L"\\BaseNamedObjects\\Cor_Private_IPCBlock_");
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            objectName.Length = returnLength - sizeof(UNICODE_NULL);
            objectName.Buffer = formatBuffer;
        }
        else
        {
            return STATUS_NO_MEMORY;
        }

        status = PhOpenSection(
            &sectionHandle,
            SECTION_QUERY,
            NULL,
            &objectName
            );

        if (NT_SUCCESS(status) || status == STATUS_ACCESS_DENIED)
        {
            if (NT_SUCCESS(status))
                NtClose(sectionHandle);

            if (IsDotNet)
                *IsDotNet = TRUE;

            if (Flags)
                *Flags = PH_CLR_VERSION_2_0;

            return STATUS_SUCCESS;
        }

        // .NET Core 3.0 and above objects

        PhInitFormatS(&format[0], L"dotnet-diagnostic-");
        PhInitFormatU(&format[1], HandleToUlong(ProcessId));

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            PHP_PIPE_NAME_HASH context;

            objectName.Length = returnLength - sizeof(UNICODE_NULL);
            objectName.Buffer = formatBuffer;
            context.NameHash = PhHashStringRefEx(&objectName, FALSE, PH_STRING_HASH_X65599);
            context.Found = FALSE;

            status = PhEnumDirectoryNamedPipe(
                &objectName,
                PhpDotNetCorePipeHashCallback,
                &context
                );

            if (NT_SUCCESS(status))
            {
                if (!context.Found)
                {
                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            }

            // NOTE: NtQueryAttributesFile and other query functions connect to the pipe and should be avoided. (dmex)
            //
            //FILE_BASIC_INFORMATION fileInfo;
            //
            //objectNameSr.Length = returnLength - sizeof(UNICODE_NULL);
            //objectNameSr.Buffer = formatBuffer;
            //
            //PhStringRefToUnicodeString(&objectNameSr, &objectName);
            //InitializeObjectAttributes(
            //    &objectAttributes,
            //    &objectName,
            //    OBJ_CASE_INSENSITIVE,
            //    NULL,
            //    NULL
            //    );
            //
            //status = NtQueryAttributesFile(&objectAttributes, &fileInfo)
        }

        if (NT_SUCCESS(status))
        {
            if (IsDotNet)
                *IsDotNet = TRUE;
            if (Flags)
                *Flags = PH_CLR_VERSION_4_ABOVE | PH_CLR_CORE_3_0_ABOVE;

            return STATUS_SUCCESS;
        }

        return status;
    }
    else
    {
        NTSTATUS status;
        HANDLE processHandle = NULL;
        ULONG flags = 0;
#ifdef _WIN64
        BOOLEAN isWow64;
#endif

        if (!ProcessHandle)
        {
            if (!NT_SUCCESS(status = PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, ProcessId)))
                return status;

            ProcessHandle = processHandle;
        }

#ifdef _WIN64
        if (InFlags & PH_CLR_NO_WOW64_CHECK)
        {
            isWow64 = !!(InFlags & PH_CLR_KNOWN_IS_WOW64);
        }
        else
        {
            isWow64 = FALSE;
            PhGetProcessIsWow64(ProcessHandle, &isWow64);
        }

        if (isWow64)
        {
            flags |= PH_CLR_PROCESS_IS_WOW64;
            status = PhEnumProcessModules32(ProcessHandle, PhpIsDotNetEnumProcessModulesCallback, &flags);
        }
        else
        {
#endif
            status = PhEnumProcessModules(ProcessHandle, PhpIsDotNetEnumProcessModulesCallback, &flags);
#ifdef _WIN64
        }
#endif

        if (processHandle)
            NtClose(processHandle);

        if (IsDotNet)
            *IsDotNet = (flags & PH_CLR_VERSION_MASK) && (flags & (PH_CLR_MSCORLIB_PRESENT | PH_CLR_CORELIB_PRESENT));

        if (Flags)
            *Flags = flags;

        return status;
    }
}

/*
 * Opens a directory object.
 *
 * \param DirectoryHandle A variable which receives a handle to the directory object.
 * \param DesiredAccess The desired access to the directory object.
 * \param RootDirectory A handle to the root directory of the object.
 * \param ObjectName The name of the directory object.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhOpenDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenDirectoryObject(
        DirectoryHandle,
        DesiredAccess,
        &objectAttributes
        );

    return status;
}

/**
 * Enumerates the objects in a directory object.
 *
 * \param DirectoryHandle A handle to a directory. The handle must have DIRECTORY_QUERY access.
 * \param Callback A callback function which is executed for each object.
 * \param Context A user-defined value to pass to the callback function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumDirectoryObjects(
    _In_ HANDLE DirectoryHandle,
    _In_ PPH_ENUM_DIRECTORY_OBJECTS Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    ULONG context = 0;
    BOOLEAN firstTime = TRUE;
    ULONG bufferSize;
    POBJECT_DIRECTORY_INFORMATION buffer;
    ULONG i;

    bufferSize = 0x200;
    buffer = PhAllocateStack(bufferSize);
    if (!buffer) return STATUS_NO_MEMORY;

    while (TRUE)
    {
        // Get a batch of entries.

        while ((status = NtQueryDirectoryObject(
            DirectoryHandle,
            buffer,
            bufferSize,
            FALSE,
            firstTime,
            &context,
            NULL
            )) == STATUS_MORE_ENTRIES)
        {
            // Check if we have at least one entry. If not, we'll double the buffer size and try
            // again.
            if (buffer[0].Name.Buffer)
                break;

            // Make sure we don't use too much memory.
            if (bufferSize > PH_LARGE_BUFFER_SIZE)
            {
                PhFreeStack(buffer);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            PhFreeStack(buffer);
            bufferSize *= 2;
            buffer = PhAllocateStack(bufferSize);
            if (!buffer) return STATUS_NO_MEMORY;
        }

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_NO_MORE_ENTRIES)
                status = STATUS_SUCCESS;

            PhFreeStack(buffer);
            return status;
        }

        // Read the batch and execute the callback function for each object.

        i = 0;

        while (TRUE)
        {
            POBJECT_DIRECTORY_INFORMATION info;
            PH_STRINGREF name;
            PH_STRINGREF typeName;

            info = &buffer[i];

            if (!info->Name.Buffer)
                break;

            PhUnicodeStringToStringRef(&info->Name, &name);
            PhUnicodeStringToStringRef(&info->TypeName, &typeName);

            status = Callback(
                DirectoryHandle,
                &name,
                &typeName,
                Context
                );

            if (status == STATUS_NO_MORE_ENTRIES)
                break;

            i++;
        }

        if (status == STATUS_NO_MORE_ENTRIES)
            break;

        firstTime = FALSE;
    }

    if (status == STATUS_NO_MORE_ENTRIES)
        status = STATUS_SUCCESS;

    PhFreeStack(buffer);

    return status;
}

/**
 * Creates a symbolic link object.
 *
 * \param LinkHandle Pointer to a variable that receives the handle to the symbolic link object.
 * \param DesiredAccess Specifies the desired access rights for the symbolic link object.
 * \param RootDirectory Optional handle to the root directory for the symbolic link object name.
 * \param FileName Pointer to a string that specifies the target file or object to which the symbolic link points.
 * \param LinkName Pointer to a string that specifies the name of the symbolic link object to be created.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateSymbolicLinkObject(
    _Out_ PHANDLE LinkHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF FileName,
    _In_ PCPH_STRINGREF LinkName
    )
{
    NTSTATUS status;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    UNICODE_STRING objectTarget;

    if (!PhStringRefToUnicodeString(FileName, &objectName))
        return STATUS_NAME_TOO_LONG;
    if (!PhStringRefToUnicodeString(LinkName, &objectTarget))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtCreateSymbolicLinkObject(
        &linkHandle,
        DesiredAccess,
        &objectAttributes,
        &objectTarget
        );

    if (NT_SUCCESS(status))
    {
        *LinkHandle = linkHandle;
    }

    return status;
}

/**
 * Queries the target of a symbolic link object.
 *
 * \param LinkTarget Pointer to a variable that receives the target of the symbolic link as a PPH_STRING.
 * \param RootDirectory Optional handle to the root directory for the object name. Can be NULL.
 * \param ObjectName Pointer to a string reference that specifies the name of the symbolic link object.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQuerySymbolicLinkObject(
    _Out_ PPH_STRING* LinkTarget,
    _In_opt_ HANDLE RootDirectory,
    _In_ PCPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    HANDLE linkHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    UNICODE_STRING targetName;
    ULONG returnLength = 0;
    WCHAR stackBuffer[DOS_MAX_PATH_LENGTH];
    ULONG bufferLength = sizeof(stackBuffer);
    PWCHAR buffer = stackBuffer;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenSymbolicLinkObject(
        &linkHandle,
        SYMBOLIC_LINK_QUERY,
        &objectAttributes
        );

    if (!NT_SUCCESS(status))
        return status;

    RtlInitEmptyUnicodeString(&targetName, buffer, (USHORT)bufferLength);

    status = NtQuerySymbolicLinkObject(
        linkHandle,
        &targetName,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        bufferLength = returnLength;
        buffer = PhAllocate(bufferLength);

        RtlInitEmptyUnicodeString(&targetName, buffer, (USHORT)bufferLength);

        status = NtQuerySymbolicLinkObject(
            linkHandle,
            &targetName,
            &returnLength
            );
    }

    if (NT_SUCCESS(status))
    {
        *LinkTarget = PhCreateStringFromUnicodeString(&targetName);
    }

    if (buffer != stackBuffer)
    {
        PhFree(buffer);
    }

    NtClose(linkHandle);

    return status;
}

/**
 * Initializes the device prefixes module.
 */
VOID PhpInitializeDevicePrefixes(
    VOID
    )
{
    ULONG i;
    PWCHAR buffer;

    // Allocate one buffer for all 26 prefixes to reduce overhead.
    buffer = PhAllocate(PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR) * 26);

    for (i = 0; i < 26; i++)
    {
        PhDevicePrefixes[i].Length = 0;
        PhDevicePrefixes[i].MaximumLength = PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR);
        PhDevicePrefixes[i].Buffer = buffer;
        buffer = PTR_ADD_OFFSET(buffer, PH_DEVICE_PREFIX_LENGTH * sizeof(WCHAR));
    }
}

VOID PhUpdateMupDevicePrefixes(
    VOID
    )
{
    static CONST PH_STRINGREF orderKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order");
    static CONST PH_STRINGREF servicesStringPart = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services");
    static CONST PH_STRINGREF networkProviderStringPart = PH_STRINGREF_INIT(L"\\NetworkProvider");

    HANDLE orderKeyHandle;
    PPH_STRING providerOrder = NULL;
    ULONG i;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;

    // The provider names are stored in the ProviderOrder value in this key:
    // HKLM\System\CurrentControlSet\Control\NetworkProvider\Order
    // Each name can then be looked up, its device name in the DeviceName value in:
    // HKLM\System\CurrentControlSet\Services\<ProviderName>\NetworkProvider

    // Note that we assume the providers only claim their device name. Some providers such as DFS
    // claim an extra part, and are not resolved correctly here.

    if (NT_SUCCESS(PhOpenKey(
        &orderKeyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &orderKeyName,
        0
        )))
    {
        providerOrder = PhQueryRegistryStringZ(orderKeyHandle, L"ProviderOrder");
        NtClose(orderKeyHandle);
    }

    if (!providerOrder)
        return;

    PhAcquireQueuedLockExclusive(&PhDeviceMupPrefixesLock);

    for (i = 0; i < PhDeviceMupPrefixesCount; i++)
    {
        PhDereferenceObject(PhDeviceMupPrefixes[i]);
        PhDeviceMupPrefixes[i] = NULL;
    }

    PhDeviceMupPrefixesCount = 0;

    PhDeviceMupPrefixes[PhDeviceMupPrefixesCount++] = PhCreateString(L"\\Device\\Mup");

    // DFS claims an extra part of file names, which we don't handle.
    // PhDeviceMupPrefixes[PhDeviceMupPrefixesCount++] = PhCreateString(L"\\Device\\DfsClient");

    remainingPart = providerOrder->sr;

    while (remainingPart.Length != 0)
    {
        PPH_STRING serviceKeyName;
        HANDLE networkProviderKeyHandle;
        PPH_STRING deviceName;

        if (PhDeviceMupPrefixesCount == PH_DEVICE_MUP_PREFIX_MAX_COUNT)
            break;

        PhSplitStringRefAtChar(&remainingPart, L',', &part, &remainingPart);

        if (part.Length != 0)
        {
            serviceKeyName = PhConcatStringRef4(
                &servicesStringPart,
                &PhNtPathSeparatorString,
                &part,
                &networkProviderStringPart
                );

            if (NT_SUCCESS(PhOpenKey(
                &networkProviderKeyHandle,
                KEY_READ,
                PH_KEY_LOCAL_MACHINE,
                &serviceKeyName->sr,
                0
                )))
            {
                if (deviceName = PhQueryRegistryStringZ(networkProviderKeyHandle, L"DeviceName"))
                {
                    PhDeviceMupPrefixes[PhDeviceMupPrefixesCount] = deviceName;
                    PhDeviceMupPrefixesCount++;
                }

                NtClose(networkProviderKeyHandle);
            }

            PhDereferenceObject(serviceKeyName);
        }
    }

    PhReleaseQueuedLockExclusive(&PhDeviceMupPrefixesLock);

    PhDereferenceObject(providerOrder);
}

/**
 * Updates the DOS device names array.
 */
VOID PhUpdateDosDevicePrefixes(
    VOID
    )
{
    WCHAR deviceNameBuffer[7] = L"\\??\\ :";
    ULONG deviceMap = 0;

    PhGetProcessDeviceMap(NtCurrentProcess(), &deviceMap);

    PhAcquireQueuedLockExclusive(&PhDevicePrefixesLock);

    for (ULONG i = 0; i < 0x1A; i++)
    {
        HANDLE linkHandle;
        OBJECT_ATTRIBUTES objectAttributes;
        UNICODE_STRING deviceName;

        if (deviceMap)
        {
            if (!(deviceMap & (0x1 << i)))
            {
                PhDevicePrefixes[i].Length = 0;
                continue;
            }
        }

        deviceNameBuffer[4] = (WCHAR)('A' + i);
        deviceName.Buffer = deviceNameBuffer;
        deviceName.Length = sizeof(deviceNameBuffer) - sizeof(UNICODE_NULL);
        deviceName.MaximumLength = deviceName.Length + sizeof(UNICODE_NULL);

        InitializeObjectAttributes(
            &objectAttributes,
            &deviceName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        if (NT_SUCCESS(NtOpenSymbolicLinkObject(
            &linkHandle,
            SYMBOLIC_LINK_QUERY,
            &objectAttributes
            )))
        {
            if (!NT_SUCCESS(NtQuerySymbolicLinkObject(
                linkHandle,
                &PhDevicePrefixes[i],
                NULL
                )))
            {
                PhDevicePrefixes[i].Length = 0;
            }

            NtClose(linkHandle);
        }
        else
        {
            PhDevicePrefixes[i].Length = 0;
        }
    }

    PhReleaseQueuedLockExclusive(&PhDevicePrefixesLock);
}

// rev from FindFirstVolumeW (dmex)
/**
 * Retrieves the mount points of volumes.
 *
 * \param DeviceHandle A handle to the MountPointManager.
 * \param MountPoints An array of mounts.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetVolumeMountPoints(
    _In_ HANDLE DeviceHandle,
    _Out_ PMOUNTMGR_MOUNT_POINTS* MountPoints
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    MOUNTMGR_MOUNT_POINT inputBuffer = { 0 };
    PMOUNTMGR_MOUNT_POINTS outputBuffer;
    ULONG inputBufferLength = sizeof(inputBuffer);
    ULONG outputBufferLength;
    ULONG attempts = 16;

    outputBufferLength = 0x800;
    outputBuffer = PhAllocate(outputBufferLength);

    do
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            IOCTL_MOUNTMGR_QUERY_POINTS,
            &inputBuffer,
            inputBufferLength,
            outputBuffer,
            outputBufferLength
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            outputBufferLength = outputBuffer->Size;
            PhFree(outputBuffer);

            if (outputBufferLength > PH_LARGE_BUFFER_SIZE)
                return STATUS_INSUFFICIENT_RESOURCES;

            outputBuffer = PhAllocate(outputBufferLength);
        }
        else
        {
            PhFree(outputBuffer);
            return status;
        }
    } while (--attempts);

    if (NT_SUCCESS(status))
    {
        *MountPoints = outputBuffer;
    }
    else
    {
        PhFree(outputBuffer);
    }

    return status;
}

// rev from GetVolumePathNamesForVolumeNameW (dmex)
/**
 * \brief Retrieves a list of drive letters and mounted folder paths for the specified volume.
 *
 * \param DeviceHandle A handle to the MountPointManager.
 * \param VolumeName A volume GUID path for the volume.
 * \param VolumePathNames A pointer to a buffer that receives the list of drive letters and mounted folder paths.
 * \a The list is an array of null-terminated strings terminated by an additional NULL character.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetVolumePathNamesForVolumeName(
    _In_ HANDLE DeviceHandle,
    _In_ PCPH_STRINGREF VolumeName,
    _Out_ PMOUNTMGR_VOLUME_PATHS* VolumePathNames
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;
    PMOUNTMGR_TARGET_NAME inputBuffer;
    PMOUNTMGR_VOLUME_PATHS outputBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    ULONG attempts = 16;

    inputBufferLength = UFIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName[VolumeName->Length]) + sizeof(UNICODE_NULL);
    inputBuffer = PhAllocateStack(inputBufferLength); // Volume{guid}, CM_Get_Device_Interface_List, SymbolicLinks, [??]
    if (!inputBuffer) return STATUS_NO_MEMORY;
    inputBuffer->DeviceNameLength = (USHORT)VolumeName->Length;
    RtlCopyMemory(inputBuffer->DeviceName, VolumeName->Buffer, VolumeName->Length);

    outputBufferLength = UFIELD_OFFSET(MOUNTMGR_VOLUME_PATHS, MultiSz[DOS_MAX_PATH_LENGTH]) + sizeof(UNICODE_NULL);
    outputBuffer = PhAllocate(outputBufferLength);

    do
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS,
            inputBuffer,
            inputBufferLength,
            outputBuffer,
            outputBufferLength
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            outputBufferLength = (outputBuffer->MultiSzLength * sizeof(WCHAR)) + sizeof(UNICODE_NULL);
            PhFree(outputBuffer);
            outputBuffer = PhAllocate(outputBufferLength);
        }
        else
        {
            PhFreeStack(inputBuffer);
            PhFree(outputBuffer);
            return status;
        }
    } while (--attempts);

    if (NT_SUCCESS(status))
    {
        *VolumePathNames = outputBuffer;
    }
    else
    {
        PhFree(outputBuffer);
    }

    PhFreeStack(inputBuffer);

    return status;
}

/**
 * Flush file caches on all volumes.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhFlushVolumeCache(
    VOID
    )
{
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PMOUNTMGR_MOUNT_POINTS objectMountPoints;

    RtlInitUnicodeString(&objectName, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &deviceHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetVolumeMountPoints(
        deviceHandle,
        &objectMountPoints
        );

    NtClose(deviceHandle);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    for (ULONG i = 0; i < objectMountPoints->NumberOfMountPoints; i++)
    {
        PMOUNTMGR_MOUNT_POINT mountPoint = &objectMountPoints->MountPoints[i];
        objectName.Length = mountPoint->SymbolicLinkNameLength;
        objectName.MaximumLength = mountPoint->SymbolicLinkNameLength + sizeof(UNICODE_NULL);
        objectName.Buffer = PTR_ADD_OFFSET(objectMountPoints, mountPoint->SymbolicLinkNameOffset);

        if (MOUNTMGR_IS_VOLUME_NAME(&objectName)) // \\??\\Volume{1111-2222}
        {
            HANDLE volumeHandle;

            InitializeObjectAttributes(
                &objectAttributes,
                &objectName,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL
                );

            status = NtCreateFile(
                &volumeHandle,
                FILE_WRITE_DATA | SYNCHRONIZE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

            if (NT_SUCCESS(status))
            {
                //if (WindowsVersion >= WINDOWS_8)
                //{
                //    status = NtFlushBuffersFileEx(volumeHandle, 0, 0, 0, &ioStatusBlock);
                //}
                //else
                {
                    status = NtFlushBuffersFile(volumeHandle, &ioStatusBlock);
                }

                NtClose(volumeHandle);
            }
        }
    }

    PhFree(objectMountPoints);

CleanupExit:

    return status;
}

//NTSTATUS PhUpdateDosDeviceMountPrefixes(
//    VOID
//    )
//{
//    NTSTATUS status;
//    HANDLE deviceHandle;
//    UNICODE_STRING objectName;
//    OBJECT_ATTRIBUTES objectAttributes;
//    IO_STATUS_BLOCK ioStatusBlock;
//    PMOUNTMGR_MOUNT_POINTS deviceMountPoints;
//
//    RtlInitUnicodeString(&objectName, MOUNTMGR_DEVICE_NAME);
//    InitializeObjectAttributes(
//        &objectAttributes,
//        &objectName,
//        OBJ_CASE_INSENSITIVE,
//        NULL,
//        NULL
//        );
//
//    status = NtCreateFile(
//        &deviceHandle,
//        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
//        &objectAttributes,
//        &ioStatusBlock,
//        NULL,
//        FILE_ATTRIBUTE_NORMAL,
//        FILE_SHARE_READ | FILE_SHARE_WRITE,
//        FILE_OPEN,
//        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
//        NULL,
//        0
//        );
//
//    if (!NT_SUCCESS(status))
//        return status;
//
//    status = PhGetVolumeMountPoints(
//        deviceHandle,
//        &deviceMountPoints
//        );
//
//    if (!NT_SUCCESS(status))
//        goto CleanupExit;
//
//    for (ULONG i = 0; i < RTL_NUMBER_OF(PhDevicePrefixes); i++)
//    {
//        PhDevicePrefixes[i].Length = 0;
//    }
//
//    for (ULONG i = 0; i < deviceMountPoints->NumberOfMountPoints; i++)
//    {
//        PMOUNTMGR_MOUNT_POINT entry = &deviceMountPoints->MountPoints[i];
//        UNICODE_STRING linkName =
//        {
//            entry->SymbolicLinkNameLength,
//            entry->SymbolicLinkNameLength + sizeof(UNICODE_NULL),
//            PTR_ADD_OFFSET(deviceMountPoints, entry->SymbolicLinkNameOffset)
//        };
//        UNICODE_STRING deviceName =
//        {
//            entry->DeviceNameLength,
//            entry->DeviceNameLength + sizeof(UNICODE_NULL),
//            PTR_ADD_OFFSET(deviceMountPoints, entry->DeviceNameOffset)
//        };
//
//        if (MOUNTMGR_IS_DRIVE_LETTER(&linkName)) // \\DosDevices\\C:
//        {
//            USHORT index = (USHORT)(linkName.Buffer[12] - L'A');
//
//            if (index >= RTL_NUMBER_OF(PhDevicePrefixes))
//                continue;
//            if (deviceName.Length >= PhDevicePrefixes[index].MaximumLength - sizeof(UNICODE_NULL))
//                continue;
//
//            PhDevicePrefixes[index].Length = deviceName.Length;
//            memcpy_s(
//                PhDevicePrefixes[index].Buffer,
//                PhDevicePrefixes[index].MaximumLength,
//                deviceName.Buffer,
//                deviceName.Length
//                );
//        }
//
//        //if (MOUNTMGR_IS_VOLUME_NAME(&linkName)) // \\??\\Volume{1111-2222}
//        //{
//        //    PH_STRINGREF volumeLinkName;
//        //    PMOUNTMGR_VOLUME_PATHS volumePaths;
//        //
//        //    PhUnicodeStringToStringRef(&linkName, &volumeLinkName);
//        //
//        //    if (NT_SUCCESS(PhGetVolumePathNamesForVolumeName(deviceHandle, &volumeLinkName, &volumePaths)))
//        //    {
//        //        for (PWSTR path = volumePaths->MultiSz; *path; path += PhCountStringZ(path) + 1)
//        //        {
//        //            dprintf("%S\n", path); // C:\\Mounted\\Folders
//        //        }
//        //    }
//        //}
//    }
//
//    PhFree(deviceMountPoints);
//
//CleanupExit:
//    NtClose(deviceHandle);
//
//    return status;
//}

/**
 * Resolves the mount prefix for a given file name.
 *
 * \param Name A pointer to a constant PPH_STRINGREF structure that specifies the file name to resolve.
 * \param NativeFileName A BOOLEAN value indicating whether the file name is in native NT format (TRUE) or DOS/Win32 format (FALSE).
 * \return A pointer to a PPH_STRING structure containing the resolved mount prefix, or NULL if the resolution fails.
 */
PPH_STRING PhResolveMountPrefix(
    _In_ PCPH_STRINGREF Name,
    _In_ BOOLEAN NativeFileName
    )
{
    NTSTATUS status;
    HANDLE deviceHandle;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PMOUNTMGR_MOUNT_POINTS mountPoints;
    PPH_STRING newName = NULL;

    RtlInitUnicodeString(&objectName, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateFile(
        &deviceHandle,
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );

    if (!NT_SUCCESS(status))
        return NULL;

    status = PhGetVolumeMountPoints(
        deviceHandle,
        &mountPoints
        );

    NtClose(deviceHandle);

    if (!NT_SUCCESS(status))
    {
        return NULL;
    }

    for (ULONG i = 0; i < mountPoints->NumberOfMountPoints; i++)
    {
        PMOUNTMGR_MOUNT_POINT entry = &mountPoints->MountPoints[i];
        const PH_STRINGREF linkPrefix = {
            entry->SymbolicLinkNameLength,
            RTL_PTR_ADD(mountPoints, entry->SymbolicLinkNameOffset)
        };
        const PH_STRINGREF devicePrefix = {
            entry->DeviceNameLength,
            RTL_PTR_ADD(mountPoints, entry->DeviceNameOffset)
        };

        if (NativeFileName)
        {
            if (PhStartsWithStringRef(Name, &devicePrefix, TRUE))
            {
                if (PATH_IS_WIN32_DOSDEVICES_PREFIX(&linkPrefix) && linkPrefix.Buffer[1] == L'?')
                {
                    // \??\Volume -> \\?\Volume
                    linkPrefix.Buffer[1] = OBJ_NAME_PATH_SEPARATOR;
                }

                // \\Device\\VhdHardDisk{12345678-abcd-1234-abcd-123456789abc} -> \\\\?\\Volume{12345678-abcd-1234-abcd-123456789abc}

                newName = PhCreateStringEx(NULL, linkPrefix.Length + (Name->Length - devicePrefix.Length));
                memcpy(newName->Buffer, linkPrefix.Buffer, linkPrefix.Length);
                memcpy(
                    PTR_ADD_OFFSET(newName->Buffer, linkPrefix.Length),
                    &Name->Buffer[devicePrefix.Length / sizeof(WCHAR)],
                    Name->Length - devicePrefix.Length
                    );
                PhTrimToNullTerminatorString(newName);
                break;
            }
        }
        else
        {
            if (PhStartsWithStringRef(Name, &linkPrefix, TRUE))
            {
                // \\?\Volume{12345678-abcd-1234-abcd-123456789abc} -> \Device\VhdHardDisk{12345678-abcd-1234-abcd-123456789abc}

                newName = PhCreateStringEx(NULL, devicePrefix.Length + (Name->Length - linkPrefix.Length));
                memcpy(newName->Buffer, devicePrefix.Buffer, devicePrefix.Length);
                memcpy(
                    PTR_ADD_OFFSET(newName->Buffer, devicePrefix.Length),
                    &Name->Buffer[linkPrefix.Length / sizeof(WCHAR)],
                    Name->Length - linkPrefix.Length
                    );
                PhTrimToNullTerminatorString(newName);
                break;
            }
        }
    }

    PhFree(mountPoints);

    return newName;
}

/**
 * Resolves a NT path into a Win32 path.
 *
 * \param Name A string containing the path to resolve.
 * \return A pointer to a string containing the Win32 path. You must free the string using
 * PhDereferenceObject() when you no longer need it.
 */
PPH_STRING PhResolveDevicePrefix(
    _In_ PCPH_STRINGREF Name
    )
{
    ULONG i;
    PPH_STRING newName = NULL;

    if (PhBeginInitOnce(&PhDevicePrefixesInitOnce))
    {
        PhpInitializeDevicePrefixes();
        PhUpdateDosDevicePrefixes();
        PhUpdateMupDevicePrefixes();

        //PhUpdateDosDeviceMountPrefixes();

        PhEndInitOnce(&PhDevicePrefixesInitOnce);
    }

    PhAcquireQueuedLockShared(&PhDevicePrefixesLock);

    // Go through the DOS devices and try to find a matching prefix.
    for (i = 0; i < 26; i++)
    {
        BOOLEAN isPrefix = FALSE;
        PH_STRINGREF prefix;

        PhUnicodeStringToStringRef(&PhDevicePrefixes[i], &prefix);

        if (prefix.Length != 0)
        {
            if (PhStartsWithStringRef(Name, &prefix, TRUE))
            {
                // To ensure we match the longest prefix, make sure the next character is a
                // backslash or the path is equal to the prefix.
                if (Name->Length == prefix.Length || Name->Buffer[prefix.Length / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
                {
                    isPrefix = TRUE;
                }
            }
        }

        if (isPrefix)
        {
            // <letter>:path
            newName = PhCreateStringEx(NULL, 2 * sizeof(WCHAR) + Name->Length - prefix.Length);
            newName->Buffer[0] = (WCHAR)('A' + i);
            newName->Buffer[1] = L':';
            memcpy(
                &newName->Buffer[2],
                &Name->Buffer[prefix.Length / sizeof(WCHAR)],
                Name->Length - prefix.Length
                );

            break;
        }
    }

    PhReleaseQueuedLockShared(&PhDevicePrefixesLock);

    if (i == 26)
    {
        // Resolve network providers.

        PhAcquireQueuedLockShared(&PhDeviceMupPrefixesLock);

        for (i = 0; i < PhDeviceMupPrefixesCount; i++)
        {
            BOOLEAN isPrefix = FALSE;
            SIZE_T prefixLength;

            prefixLength = PhDeviceMupPrefixes[i]->Length;

            if (prefixLength != 0)
            {
                if (PhStartsWithStringRef(Name, &PhDeviceMupPrefixes[i]->sr, TRUE))
                {
                    // To ensure we match the longest prefix, make sure the next character is a
                    // backslash. Don't resolve if the name *is* the prefix. Otherwise, we will end
                    // up with a useless string like "\".
                    if (Name->Length != prefixLength && Name->Buffer[prefixLength / sizeof(WCHAR)] == OBJ_NAME_PATH_SEPARATOR)
                    {
                        isPrefix = TRUE;
                    }
                }
            }

            if (isPrefix)
            {
                // \path
                newName = PhCreateStringEx(NULL, 1 * sizeof(WCHAR) + Name->Length - prefixLength);
                newName->Buffer[0] = OBJ_NAME_PATH_SEPARATOR;
                memcpy(
                    &newName->Buffer[1],
                    &Name->Buffer[prefixLength / sizeof(WCHAR)],
                    Name->Length - prefixLength
                    );

                break;
            }
        }

        PhReleaseQueuedLockShared(&PhDeviceMupPrefixesLock);
    }

    if (PhIsNullOrEmptyString(newName) && (Name->Length != 0 && Name->Buffer[0] == OBJ_NAME_PATH_SEPARATOR))
    {
        // We didn't find a match. Try the mount point prefixes.
        newName = PhResolveMountPrefix(Name, TRUE);
    }

    if (newName)
        PhTrimToNullTerminatorString(newName);

    return newName;
}

/**
 * Converts a file name into Win32 format.
 *
 * \param FileName A string containing a file name.
 * \return A pointer to a string containing the Win32 file name. You must free the string using
 * PhDereferenceObject() when you no longer need it.
 * \remarks This function may convert NT object name paths to invalid ones. If the path to be
 * converted is not necessarily a file name, use PhResolveDevicePrefix().
 */
PPH_STRING PhGetFileName(
    _In_ PPH_STRING FileName
    )
{
    PPH_STRING newFileName;

    newFileName = FileName;

    // "\??\" refers to \GLOBAL??\. Just remove it.
    if (PhStartsWithString2(FileName, L"\\??\\", FALSE))
    {
        newFileName = PhCreateStringEx(NULL, FileName->Length - 4 * sizeof(WCHAR));
        memcpy(newFileName->Buffer, &FileName->Buffer[4], FileName->Length - 4 * sizeof(WCHAR));
    }
    // "\SystemRoot" means "C:\Windows".
    else if (PhStartsWithString2(FileName, L"\\SystemRoot", TRUE))
    {
        PH_STRINGREF systemRoot;

        PhGetSystemRoot(&systemRoot);
        newFileName = PhCreateStringEx(NULL, systemRoot.Length + FileName->Length - 11 * sizeof(WCHAR));
        memcpy(newFileName->Buffer, systemRoot.Buffer, systemRoot.Length);
        memcpy(PTR_ADD_OFFSET(newFileName->Buffer, systemRoot.Length), &FileName->Buffer[11], FileName->Length - 11 * sizeof(WCHAR));
    }
    // System32, SysWOW64, SysArm32, and SyChpe32 are all identical length, fixup is the same
    else if (
        // "System32\" means "C:\Windows\System32\".
        PhStartsWithString2(FileName, L"System32\\", TRUE)
#if _WIN64
        // "SysWOW64\" means "C:\Windows\SysWOW64\".
        || PhStartsWithString2(FileName, L"SysWOW64\\", TRUE)
#if _M_ARM64
        // "SysArm32\" means "C:\Windows\SysArm32\".
        || PhStartsWithString2(FileName, L"SysArm32\\", TRUE)
        // "SyChpe32\" means "C:\Windows\SyChpe32\".
        || PhStartsWithString2(FileName, L"SyChpe32\\", TRUE)
#endif
#endif
        )
    {
        PH_STRINGREF systemRoot;

        PhGetSystemRoot(&systemRoot);
        newFileName = PhCreateStringEx(NULL, systemRoot.Length + sizeof(UNICODE_NULL) + FileName->Length);
        memcpy(newFileName->Buffer, systemRoot.Buffer, systemRoot.Length);
        newFileName->Buffer[systemRoot.Length / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        memcpy(PTR_ADD_OFFSET(newFileName->Buffer, systemRoot.Length + sizeof(UNICODE_NULL)), FileName->Buffer, FileName->Length);
    }
    else if (FileName->Length != 0 && FileName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)
    {
        PPH_STRING resolvedName;

        resolvedName = PhResolveDevicePrefix(&FileName->sr);

        if (resolvedName)
        {
            newFileName = resolvedName;
        }
        else
        {
            // We didn't find a match.
            // If the file name starts with "\Windows", prepend the system drive.
            if (PhStartsWithString2(newFileName, L"\\Windows", TRUE))
            {
                PH_STRINGREF systemRoot;

                PhGetSystemRoot(&systemRoot);
                newFileName = PhCreateStringEx(NULL, FileName->Length + 2 * sizeof(WCHAR));
                newFileName->Buffer[0] = systemRoot.Buffer[0];
                newFileName->Buffer[1] = L':';
                memcpy(&newFileName->Buffer[2], FileName->Buffer, FileName->Length);
            }
            else
            {
                PhReferenceObject(newFileName);
            }
        }
    }
    else
    {
        // Just return the supplied file name. Note that we need to add a reference.
        PhReferenceObject(newFileName);
    }

    return newFileName;
}

/**
 * Converts a DOS-style path name to an NT-style path name.
 *
 * \param Name A pointer to a PPH_STRINGREF structure that contains the DOS path to convert.
 * \return A PPH_STRING containing the converted NT path, or NULL if the conversion fails.
 */
PPH_STRING PhDosPathNameToNtPathName(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_STRING newName = NULL;
    PH_STRINGREF prefix;
    ULONG index;

    if (PhBeginInitOnce(&PhDevicePrefixesInitOnce))
    {
        PhpInitializeDevicePrefixes();
        PhUpdateDosDevicePrefixes();
        PhUpdateMupDevicePrefixes();

        PhEndInitOnce(&PhDevicePrefixesInitOnce);
    }

    if (PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        index = (ULONG)(PhUpcaseUnicodeChar(Name->Buffer[0]) - L'A');

        if (index >= RTL_NUMBER_OF(PhDevicePrefixes))
            return NULL;

        PhAcquireQueuedLockShared(&PhDevicePrefixesLock);
        PhUnicodeStringToStringRef(&PhDevicePrefixes[index], &prefix);

        if (prefix.Length != 0)
        {
            // C:\\Name -> \\Device\\HardDiskVolumeX\\Name
            newName = PhCreateStringEx(NULL, prefix.Length + Name->Length - sizeof(WCHAR[2]));
            memcpy(
                newName->Buffer,
                prefix.Buffer,
                prefix.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length),
                PTR_ADD_OFFSET(Name->Buffer, sizeof(WCHAR[2])),
                Name->Length - sizeof(WCHAR[2])
                );
        }

        PhReleaseQueuedLockShared(&PhDevicePrefixesLock);
    }
    else if (PhStartsWithStringRef2(Name, L"\\SystemRoot", TRUE))
    {
        PhAcquireQueuedLockShared(&PhDevicePrefixesLock);
        PhUnicodeStringToStringRef(&PhDevicePrefixes[(ULONG)'C'-'A'], &prefix);

        if (prefix.Length != 0)
        {
            static CONST PH_STRINGREF systemRoot = PH_STRINGREF_INIT(L"\\Windows");

            // \\SystemRoot\\Name -> \\Device\\HardDiskVolumeX\\Windows\\Name
            newName = PhCreateStringEx(NULL, prefix.Length + Name->Length + systemRoot.Length - sizeof(L"SystemRoot"));
            memcpy(
                newName->Buffer,
                prefix.Buffer,
                prefix.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length),
                systemRoot.Buffer,
                systemRoot.Length
                );
            memcpy(
                PTR_ADD_OFFSET(newName->Buffer, prefix.Length + systemRoot.Length),
                PTR_ADD_OFFSET(Name->Buffer, sizeof(L"SystemRoot")),
                Name->Length - sizeof(L"SystemRoot")
                );
        }

        PhReleaseQueuedLockShared(&PhDevicePrefixesLock);
    }
    else if (PATH_IS_WIN32_DOSDEVICES_PREFIX(Name))
    {
        newName = PhResolveMountPrefix(Name, FALSE);
    }

    return newName;
}

/**
 * Converts a DOS-style file path to an NT-style file path.
 *
 * \param DosFileName The DOS-style file path to convert (e.g., "C:\\Windows\\System32").
 * \param NtFileName A pointer to a UNICODE_STRING structure that receives the resulting NT-style file path.
 * \param FilePart If specified, receives a pointer to the file part of the path (the final component).
 * \param RelativeName If specified, receives a pointer to a RTL_RELATIVE_NAME_U structure that describes the relative name.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhDosLongPathNameToNtPathNameWithStatus(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR* FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    )
{
    NTSTATUS status;

    if (
        WindowsVersion >= WINDOWS_10_RS1 && PhAreLongPathsEnabled() &&
        RtlDosLongPathNameToNtPathName_U_WithStatus_Import()
        )
    {
        status = RtlDosLongPathNameToNtPathName_U_WithStatus_Import()(
            DosFileName,
            NtFileName,
            FilePart,
            RelativeName
            );
    }
    else
    {
        status = RtlDosPathNameToNtPathName_U_WithStatus(
            DosFileName,
            NtFileName,
            FilePart,
            RelativeName
            );
    }

    return status;
}

/**
 * Retrieves the root prefix of an NT path from the specified string reference.
 *
 * \param Name A pointer to a PPH_STRINGREF structure that contains the NT path string.
 * \return A PPH_STRING representing the root prefix of the NT path, or NULL if the prefix cannot be determined.
 */
PPH_STRING PhGetNtPathRootPrefix(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_STRING pathDevicePrefix = NULL;
    PH_STRINGREF prefix;

    PhAcquireQueuedLockShared(&PhDevicePrefixesLock);

    for (ULONG i = 0; i < RTL_NUMBER_OF(PhDevicePrefixes); i++)
    {
        PhUnicodeStringToStringRef(&PhDevicePrefixes[i], &prefix);

        if (prefix.Length && PhStartsWithStringRef(Name, &prefix, FALSE))
        {
            pathDevicePrefix = PhCreateString2(&prefix);
            break;
        }
    }

    PhReleaseQueuedLockShared(&PhDevicePrefixesLock);

    return pathDevicePrefix;
}

/**
 * Retrieves the existing path prefix from the specified string reference.
 *
 * This function examines the provided string reference, which is expected to represent a file or directory path,
 * and returns a PPH_STRING containing the longest existing prefix of that path. If no part of the path exists,
 * the function may return NULL or an empty string, depending on implementation.
 *
 * \param Name A pointer to a PPH_STRINGREF structure that contains the path to be examined.
 * \return A PPH_STRING containing the longest existing prefix of the specified path, or NULL if no prefix exists.
 */
PPH_STRING PhGetExistingPathPrefix(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_STRING existingPathPrefix = NULL;
    PH_STRINGREF remainingPart;
    PH_STRINGREF directoryPart;
    PH_STRINGREF baseNamePart;

    if (PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        assert(FALSE);
        return NULL;
    }

    if (PhDoesDirectoryExist(Name))
    {
        return PhCreateString2(Name);
    }

    remainingPart = *Name;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtLastChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &baseNamePart);

        if (directoryPart.Length != 0)
        {
            if (PhDoesDirectoryExist(&directoryPart))
            {
                existingPathPrefix = PhCreateString2(&directoryPart);
                break;
            }
        }

        remainingPart = directoryPart;
    }

    //if (PhEqualStringRef(&existingPathPrefix, PhGetNtPathRootPrefix(Name), FALSE))
    //    return NULL;

    return existingPathPrefix;
}

/**
 * Retrieves the existing path prefix from a given string reference representing a path.
 *
 * This function examines the provided path and returns the longest existing prefix
 * (i.e., the deepest directory that actually exists on the filesystem).
 *
 * \param Name A pointer to a PPH_STRINGREF structure that contains the path to be checked.
 * \return A PPH_STRING containing the longest existing path prefix, or NULL if no part of the path exists.
 */
PPH_STRING PhGetExistingPathPrefixWin32(
    _In_ PCPH_STRINGREF Name
    )
{
    PPH_STRING existingPathPrefix = NULL;
    PH_STRINGREF remainingPart;
    PH_STRINGREF directoryPart;
    PH_STRINGREF baseNamePart;

    if (!PATH_IS_WIN32_DRIVE_PREFIX(Name))
    {
        assert(FALSE);
        return NULL;
    }

    if (PhDoesDirectoryExistWin32(PhGetStringRefZ(Name)))
    {
        return PhCreateString2(Name);
    }

    remainingPart = *Name;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtLastChar(&remainingPart, OBJ_NAME_PATH_SEPARATOR, &directoryPart, &baseNamePart);

        if (directoryPart.Length != 0)
        {
            existingPathPrefix = PhCreateString2(&directoryPart);

            if (PhDoesDirectoryExistWin32(PhGetString(existingPathPrefix)))
                break;

            PhClearReference(&existingPathPrefix);
        }

        remainingPart = directoryPart;
    }

    return existingPathPrefix;
}

// rev from GetLongPathNameW (dmex)
/**
 * Retrieves the long (non-8.3) path form of the specified file or directory name.
 *
 * \param FileName A pointer to a PPH_STRINGREF structure that contains the file or directory name for which to retrieve the long path name.
 * \return A PPH_STRING containing the long path name if successful, or NULL if the operation fails.
 */
PPH_STRING PhGetLongPathName(
    _In_ PCPH_STRINGREF FileName
    )
{
    PPH_STRING longPathName = NULL;
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    PFILE_BOTH_DIR_INFORMATION directoryInfoBuffer;
    ULONG directoryInfoLength;
    PH_STRINGREF baseNamePart;
    UNICODE_STRING baseNameUs;

    status = PhOpenFile(
        &fileHandle,
        FileName,
        FILE_READ_DATA | FILE_LIST_DIRECTORY | SYNCHRONIZE,
        NULL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
        NULL
        );

    if (!NT_SUCCESS(status))
        return NULL;

    if (!PhGetBasePath(FileName, NULL, &baseNamePart))
        goto CleanupExit;
    if (!PhStringRefToUnicodeString(&baseNamePart, &baseNameUs))
        goto CleanupExit;

    directoryInfoLength = PAGE_SIZE;
    directoryInfoBuffer = PhAllocate(directoryInfoLength);

    status = NtQueryDirectoryFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &ioStatusBlock,
        directoryInfoBuffer,
        directoryInfoLength,
        FileBothDirectoryInformation,
        TRUE,
        &baseNameUs,
        FALSE
        );

    if (NT_SUCCESS(status))
    {
        longPathName = PhCreateStringEx(directoryInfoBuffer->FileName, directoryInfoBuffer->FileNameLength);
    }

    PhFree(directoryInfoBuffer);

CleanupExit:
    NtClose(fileHandle);

    return longPathName;
}

/**
 * Retrieves the heap signature from a process heap structure.
 *
 * \param ProcessHandle A handle to the process. The handle must have PROCESS_VM_READ access.
 * \param HeapAddress The base address of the heap in the target process.
 * \param IsWow64 Specifies whether the target process is running under WOW64 (TRUE) or is native (FALSE).
 * \param HeapSignature A pointer to a variable that receives the heap signature value.
 * \return NTSTATUS Successful or errant status.
 * \remarks The heap signature is a validation field in the _HEAP structure used to verify heap integrity.
 * This function reads the SegmentSignature field at different offsets depending on the process architecture:
 * - For WOW64 processes: offset 0x8
 * - For native 64-bit processes: offset 0x10
 * The signature value is only available on Windows 7 and later versions.
 */
NTSTATUS PhGetProcessHeapSignature(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID HeapAddress,
    _In_ ULONG IsWow64,
    _Out_ ULONG *HeapSignature
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    ULONG heapSignature = ULONG_MAX;

    if (WindowsVersion >= WINDOWS_7)
    {
        // dt _HEAP SegmentSignature
        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x8 : 0x10),
            &heapSignature,
            sizeof(ULONG),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        if (HeapSignature)
            *HeapSignature = heapSignature;
    }

    return status;
}

/**
 * Retrieves the front-end type of a specified heap in a process.
 *
 * \param ProcessHandle A handle to the process containing the heap.
 * \param HeapAddress The base address of the heap to query.
 * \param IsWow64 A flag indicating whether the process is running under WOW64.
 * \param HeapFrontEndType A pointer to a UCHAR that receives the front-end type of the heap.
 * Possible values include 0x0 (no front-end), 0x1 (LFH), etc., depending on the heap configuration.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessHeapFrontEndType(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID HeapAddress,
    _In_ ULONG IsWow64,
    _Out_ UCHAR *HeapFrontEndType
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    UCHAR heapFrontEndType = UCHAR_MAX;

    if (WindowsVersion >= WINDOWS_10)
    {
        // dt _HEAP FrontEndHeapType
        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0ea : 0x1a2),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }
    else if (WindowsVersion >= WINDOWS_8_1)
    {
        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0d6 : 0x17a),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }
    else if (WindowsVersion >= WINDOWS_7)
    {
        status = PhReadVirtualMemory(
            ProcessHandle,
            PTR_ADD_OFFSET(HeapAddress, IsWow64 ? 0x0da : 0x182),
            &heapFrontEndType,
            sizeof(UCHAR),
            NULL
            );
    }

    if (NT_SUCCESS(status))
    {
        if (HeapFrontEndType)
            *HeapFrontEndType = heapFrontEndType;
    }

    return status;
}

/**
 * Queries heap information for a specified process.
 *
 * \param ProcessId Handle to the process whose heap information is to be queried.
 * \param HeapInformation Pointer to a variable that receives a pointer to a structure containing heap information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhQueryProcessHeapInformation(
    _In_ HANDLE ProcessId,
    _Out_ PPH_PROCESS_DEBUG_HEAP_INFORMATION* HeapInformation
    )
{
    NTSTATUS status;
    PRTL_DEBUG_INFORMATION debugBuffer = NULL;
    PPH_PROCESS_DEBUG_HEAP_INFORMATION heapDebugInfo = NULL;

    for (ULONG i = 0x400000; ; i *= 2) // rev from Heap32First/Heap32Next (dmex)
    {
        if (!(debugBuffer = RtlCreateQueryDebugBuffer(i, FALSE)))
            return STATUS_UNSUCCESSFUL;

        status = RtlQueryProcessDebugInformation(
            ProcessId,
            RTL_QUERY_PROCESS_HEAP_SUMMARY | RTL_QUERY_PROCESS_HEAP_ENTRIES | RTL_QUERY_PROCESS_NONINVASIVE,
            debugBuffer
            );

        if (!NT_SUCCESS(status))
        {
            RtlDestroyQueryDebugBuffer(debugBuffer);
            debugBuffer = NULL;
        }

        if (NT_SUCCESS(status) || status != STATUS_NO_MEMORY)
            break;

        if (2 * i <= i)
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    if (!NT_SUCCESS(status))
        return status;

    if (!debugBuffer->Heaps)
    {
        // The RtlQueryProcessDebugInformation function has two bugs on some versions
        // when querying the ProcessId for a frozen (suspended) immersive process. (dmex)
        //
        // 1) It'll deadlock the current thread for 30 seconds.
        // 2) It'll return STATUS_SUCCESS but with a NULL Heaps buffer.
        //
        // A workaround was implemented using PhCreateExecutionRequiredRequest() (dmex)

        RtlDestroyQueryDebugBuffer(debugBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    if (WindowsVersion > WINDOWS_11)
    {
        heapDebugInfo = PhAllocateZero(sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY));
        heapDebugInfo->NumberOfHeaps = ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->NumberOfHeaps;
    }
    else
    {
        heapDebugInfo = PhAllocateZero(sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY));
        heapDebugInfo->NumberOfHeaps = ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->NumberOfHeaps;
    }

    heapDebugInfo->DefaultHeap = debugBuffer->ProcessHeap;

    for (ULONG i = 0; i < heapDebugInfo->NumberOfHeaps; i++)
    {
        RTL_HEAP_INFORMATION_V2 heapInfo = { 0 };
        HANDLE processHandle;
        SIZE_T allocated = 0;
        SIZE_T committed = 0;

        if (WindowsVersion > WINDOWS_11)
        {
            heapInfo = ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->Heaps[i];
        }
        else
        {
            RTL_HEAP_INFORMATION_V1 heapInfoV1 = ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->Heaps[i];
            heapInfo.NumberOfEntries = heapInfoV1.NumberOfEntries;
            heapInfo.Entries = heapInfoV1.Entries;
            heapInfo.BytesCommitted = heapInfoV1.BytesCommitted;
            heapInfo.Flags = heapInfoV1.Flags;
            heapInfo.BaseAddress = heapInfoV1.BaseAddress;
        }

        // go through all heap entries and compute amount of allocated and committed bytes (ge0rdi)
        for (ULONG e = 0; e < heapInfo.NumberOfEntries; e++)
        {
            PRTL_HEAP_ENTRY entry = &heapInfo.Entries[e];

            if (entry->Flags & RTL_HEAP_BUSY)
                allocated += entry->Size;
            if (entry->Flags & RTL_HEAP_SEGMENT)
                committed += entry->u.s2.CommittedSize;
        }

        // sometimes computed number if committed bytes is few pages smaller than the one reported by API, lets use the higher value (ge0rdi)
        if (committed < heapInfo.BytesCommitted)
            committed = heapInfo.BytesCommitted;

        // make sure number of allocated bytes is not higher than number of committed bytes (as that would make no sense) (ge0rdi)
        if (allocated > committed)
            allocated = committed;

        heapDebugInfo->Heaps[i].Flags = heapInfo.Flags;
        heapDebugInfo->Heaps[i].Signature = ULONG_MAX;
        heapDebugInfo->Heaps[i].HeapFrontEndType = UCHAR_MAX;
        heapDebugInfo->Heaps[i].NumberOfEntries = heapInfo.NumberOfEntries;
        heapDebugInfo->Heaps[i].BaseAddress = heapInfo.BaseAddress;
        heapDebugInfo->Heaps[i].BytesAllocated = allocated;
        heapDebugInfo->Heaps[i].BytesCommitted = committed;

        if (NT_SUCCESS(PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
            ProcessId
            )))
        {
            ULONG signature = ULONG_MAX;
            UCHAR frontEndType = UCHAR_MAX;
#ifndef _WIN64
            BOOLEAN isWow64 = TRUE;
#else
            BOOLEAN isWow64 = FALSE;

            PhGetProcessIsWow64(processHandle, &isWow64);
#endif
            if (NT_SUCCESS(PhGetProcessHeapSignature(
                processHandle,
                heapInfo.BaseAddress,
                isWow64,
                &signature
                )))
            {
                heapDebugInfo->Heaps[i].Signature = signature;
            }

            if (NT_SUCCESS(PhGetProcessHeapFrontEndType(
                processHandle,
                heapInfo.BaseAddress,
                isWow64,
                &frontEndType
                )))
            {
                heapDebugInfo->Heaps[i].HeapFrontEndType = frontEndType;
            }

            NtClose(processHandle);
        }
    }

    if (HeapInformation)
        *HeapInformation = heapDebugInfo;
    else
        PhFree(heapDebugInfo);

    if (debugBuffer)
        RtlDestroyQueryDebugBuffer(debugBuffer);

    return STATUS_SUCCESS;
}

/**
 * Queries lock information for a specified process and invokes the provided callback
 * for each lock associated with the process.
 *
 * \param ProcessId The handle to the process for which to query lock information.
 * \param Callback A pointer to a callback function of type PPH_ENUM_PROCESS_LOCKS
 * that will be called for each lock.
 * \param Context An optional pointer to user-defined context data that will be
 * passed to the callback function.
 * \return NTSTATUS indicating success or failure.
 */
NTSTATUS PhQueryProcessLockInformation(
    _In_ HANDLE ProcessId,
    _In_ PPH_ENUM_PROCESS_LOCKS Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PRTL_DEBUG_INFORMATION debugBuffer = NULL;

    for (ULONG i = 0x400000; ; i *= 2) // rev from Heap32First/Heap32Next (dmex)
    {
        if (!(debugBuffer = RtlCreateQueryDebugBuffer(i, FALSE)))
            return STATUS_UNSUCCESSFUL;

        status = RtlQueryProcessDebugInformation(
            ProcessId,
            RTL_QUERY_PROCESS_LOCKS,
            debugBuffer
            );

        if (!NT_SUCCESS(status))
        {
            RtlDestroyQueryDebugBuffer(debugBuffer);
            debugBuffer = NULL;
        }

        if (NT_SUCCESS(status) || status != STATUS_NO_MEMORY)
            break;

        if (2 * i <= i)
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    if (!NT_SUCCESS(status))
        return status;

    if (!debugBuffer->Locks)
    {
        // The RtlQueryProcessDebugInformation function has two bugs on some versions
        // when querying the ProcessId for a frozen (suspended) immersive process. (dmex)
        //
        // 1) It'll deadlock the current thread for 30 seconds.
        // 2) It'll return STATUS_SUCCESS but with a NULL Heaps buffer.
        //
        // A workaround was implemented using PhCreateExecutionRequiredRequest() (dmex)

        RtlDestroyQueryDebugBuffer(debugBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    status = Callback(
        debugBuffer->Locks->NumberOfLocks,
        debugBuffer->Locks->Locks,
        Context
        );

    if (debugBuffer)
    {
        RtlDestroyQueryDebugBuffer(debugBuffer);
    }

    return status;
}

// rev from kernelbase!GetMachineTypeAttributes (dmex)
/**
 * Retrieves the attributes associated with a specified machine type.
 *
 * \param Machine The machine type identifier (e.g., IMAGE_FILE_MACHINE_I386).
 * \param Attributes A pointer to a MACHINE_ATTRIBUTES structure that receives the attributes.
 * \return NTSTATUS indicating success or failure.
 * \remarks Queries if the specified architecture is supported on the current system,
 * either natively or by any form of compatibility or emulation layer.
 */
NTSTATUS PhGetMachineTypeAttributes(
    _In_ USHORT Machine,
    _Out_ MACHINE_ATTRIBUTES* Attributes
    )
{
    NTSTATUS status;
    HANDLE input[1] = { 0 };
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION output[6] = { 0 };
    ULONG returnLength;

    status = NtQuerySystemInformationEx(
        SystemSupportedProcessorArchitectures2,
        input,
        sizeof(input),
        output,
        sizeof(output),
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        MACHINE_ATTRIBUTES attributes;

        memset(&attributes, 0, sizeof(MACHINE_ATTRIBUTES));

        for (ULONG i = 0; i < returnLength / sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION); i++)
        {
            if (output[i].Machine == Machine)
            {
                if (output[i].KernelMode)
                    SetFlag(attributes, KernelEnabled);
                if (output[i].UserMode)
                    SetFlag(attributes, UserEnabled);
                if (output[i].WoW64Container)
                    SetFlag(attributes, Wow64Container);
                break;
            }
        }

        *Attributes = attributes;
    }

    return status;
}

/**
 * Checks if firmware-related features are supported on the current system.
 *
 * \return TRUE if firmware features are supported, FALSE otherwise.
 */
BOOLEAN PhIsFirmwareSupported(
    VOID
    )
{
    static CONST UNICODE_STRING variableName = RTL_CONSTANT_STRING(L" ");
    static CONST GUID vendorGuid = { 0 };
    ULONG variableValueLength = 0;

    if (NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        NULL,
        &variableValueLength,
        NULL
        ) == STATUS_VARIABLE_NOT_FOUND)
    {
        return TRUE;
    }

    return FALSE;
}

// rev from GetFirmwareEnvironmentVariableW (dmex)
/**
 * Retrieves the value of a specified firmware environment variable.
 *
 * \param VariableName A pointer to a PH_STRINGREF structure that specifies the name of the variable to retrieve.
 * \param VendorGuid A pointer to a PH_STRINGREF structure that specifies the GUID of the firmware vendor.
 * \param ValueBuffer A pointer to a buffer that receives the value of the environment variable.
 * \param ValueLength An optional pointer to a ULONG that receives the length of the value returned in ValueBuffer.
 * \param ValueAttributes An optional pointer to a ULONG that receives the attributes of the environment variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetFirmwareEnvironmentVariable(
    _In_ PCPH_STRINGREF VariableName,
    _In_ PCPH_STRINGREF VendorGuid,
    _Out_writes_bytes_opt_(*ValueLength) PVOID* ValueBuffer,
    _Out_opt_ PULONG ValueLength,
    _Out_opt_ PULONG ValueAttributes
    )
{
    NTSTATUS status;
    GUID vendorGuid;
    UNICODE_STRING variableName;
    PVOID valueBuffer;
    ULONG valueLength = 0;
    ULONG valueAttributes = 0;

    if (!PhStringRefToUnicodeString(VariableName, &variableName))
        return STATUS_NAME_TOO_LONG;

    status = PhStringToGuid(
        VendorGuid,
        &vendorGuid
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        NULL,
        &valueLength,
        &valueAttributes
        );

    if (status != STATUS_BUFFER_TOO_SMALL)
        return STATUS_UNSUCCESSFUL;

    valueBuffer = PhAllocate(valueLength);
    memset(valueBuffer, 0, valueLength);

    status = NtQuerySystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        valueBuffer,
        &valueLength,
        &valueAttributes
        );

    if (NT_SUCCESS(status))
    {
        if (ValueBuffer)
            *ValueBuffer = valueBuffer;
        else
            PhFree(valueBuffer);

        if (ValueLength)
            *ValueLength = valueLength;

        if (ValueAttributes)
            *ValueAttributes = valueAttributes;
    }
    else
    {
        PhFree(valueBuffer);
    }

    return status;
}

/**
 * Sets a firmware environment variable.
 *
 * \param VariableName A pointer to a PH_STRINGREF structure that specifies the name of the variable to set.
 * \param VendorGuid A pointer to a PH_STRINGREF structure that specifies the vendor GUID associated with the variable.
 * \param ValueBuffer An optional pointer to the buffer containing the value to set for the variable.
 * \param ValueLength The length, in bytes, of the value buffer.
 * \param Attributes Attributes to apply to the variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetFirmwareEnvironmentVariable(
    _In_ PCPH_STRINGREF VariableName,
    _In_ PCPH_STRINGREF VendorGuid,
    _In_reads_bytes_opt_(ValueLength) PVOID ValueBuffer,
    _In_ ULONG ValueLength,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    GUID vendorGuid;
    UNICODE_STRING variableName;

    if (!PhStringRefToUnicodeString(VariableName, &variableName))
        return STATUS_NAME_TOO_LONG;

    status = PhStringToGuid(
        VendorGuid,
        &vendorGuid
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtSetSystemEnvironmentValueEx(
        &variableName,
        &vendorGuid,
        ValueBuffer,
        ValueLength,
        Attributes
        );

    return status;
}

/**
 * Enumerates firmware environment values based on the specified information class.
 *
 * \param InformationClass The type of system environment information to enumerate.
 * \param Variables A pointer to a variable that receives the enumerated firmware environment values.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumFirmwareEnvironmentValues(
    _In_ SYSTEM_ENVIRONMENT_INFORMATION_CLASS InformationClass,
    _Out_ PVOID* Variables
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferLength;

    bufferLength = PAGE_SIZE;
    buffer = PhAllocate(bufferLength);

    while (TRUE)
    {
        status = NtEnumerateSystemEnvironmentValuesEx(
            InformationClass,
            buffer,
            &bufferLength
            );

        if (status == STATUS_BUFFER_TOO_SMALL || status == STATUS_INFO_LENGTH_MISMATCH)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferLength);
        }
        else
        {
            break;
        }
    }

    if (NT_SUCCESS(status))
    {
        *Variables = buffer;
    }
    else
    {
        PhFree(buffer);
    }

    return status;
}

/**
 * Sets the system environment to boot into firmware on next restart.
 *
 * This function modifies the system environment variables to ensure that
 * the system boots directly into the firmware interface (such as UEFI or BIOS)
 * on the next system startup.
 *
 * \return NTSTATUS code indicating success or failure of the operation.
 */
NTSTATUS PhSetSystemEnvironmentBootToFirmware(
    VOID
    )
{
    static CONST GUID EFI_GLOBAL_VARIABLE_GUID = { 0x8be4df61, 0x93ca, 0x11d2, { 0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c } };
    static CONST UNICODE_STRING OsIndicationsSupportedName = RTL_CONSTANT_STRING(L"OsIndicationsSupported");
    static CONST UNICODE_STRING OsIndicationsName = RTL_CONSTANT_STRING(L"OsIndications");
    const ULONG64 EFI_OS_INDICATIONS_BOOT_TO_FW_UI = 0x0000000000000001ULL;
    ULONG osIndicationsLength = sizeof(ULONG64);
    ULONG osIndicationsAttributes = 0;
    ULONG64 osIndicationsSupported = 0;
    ULONG64 osIndicationsValue = 0;
    NTSTATUS status;

    status = NtQuerySystemEnvironmentValueEx(
        &OsIndicationsSupportedName,
        &EFI_GLOBAL_VARIABLE_GUID,
        &osIndicationsSupported,
        &osIndicationsLength,
        NULL
        );

    if (status == STATUS_VARIABLE_NOT_FOUND || !(osIndicationsSupported & EFI_OS_INDICATIONS_BOOT_TO_FW_UI))
    {
        status = STATUS_NOT_SUPPORTED;
    }

    if (NT_SUCCESS(status))
    {
        status = NtQuerySystemEnvironmentValueEx(
            &OsIndicationsName,
            &EFI_GLOBAL_VARIABLE_GUID,
            &osIndicationsValue,
            &osIndicationsLength,
            &osIndicationsAttributes
            );

        if (NT_SUCCESS(status) || status == STATUS_VARIABLE_NOT_FOUND)
        {
            osIndicationsValue |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;

            if (status == STATUS_VARIABLE_NOT_FOUND)
            {
                osIndicationsAttributes = EFI_VARIABLE_NON_VOLATILE;
            }

            status = NtSetSystemEnvironmentValueEx(
                &OsIndicationsName,
                &EFI_GLOBAL_VARIABLE_GUID,
                &osIndicationsValue,
                osIndicationsLength,
                osIndicationsAttributes
                );
        }
    }

    return status;
}

// rev from RtlpCreateExecutionRequiredRequest (dmex)
/**
 * Create a new power request object. The process will continue to run instead of being suspended or terminated by PLM (Process Lifetime Management).
 * This is mandatory on Windows 8 and above to prevent threads created by DebugActiveProcess, QueueUserAPC and RtlQueryProcessDebug* functions from deadlocking the current application.
 *
 * \param ProcessHandle A handle to the process for which the power request is to be created.
 * \param PowerRequestHandle A pointer to a variable that receives a handle to the new power request.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateExecutionRequiredRequest(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE PowerRequestHandle
    )
{
    NTSTATUS status;
    HANDLE powerRequestHandle = NULL;
    COUNTED_REASON_CONTEXT powerRequestReason;
    POWER_REQUEST_ACTION powerRequestAction;

    memset(&powerRequestReason, 0, sizeof(COUNTED_REASON_CONTEXT));
    powerRequestReason.Version = POWER_REQUEST_CONTEXT_VERSION;
    powerRequestReason.Flags = POWER_REQUEST_CONTEXT_NOT_SPECIFIED;

    status = NtPowerInformation(
        PlmPowerRequestCreate,
        &powerRequestReason,
        sizeof(COUNTED_REASON_CONTEXT),
        &powerRequestHandle,
        sizeof(HANDLE)
        );

    if (!NT_SUCCESS(status))
        return status;

    memset(&powerRequestAction, 0, sizeof(POWER_REQUEST_ACTION));
    powerRequestAction.PowerRequestHandle = powerRequestHandle;
    powerRequestAction.RequestType = PowerRequestExecutionRequiredInternal;
    powerRequestAction.SetAction = TRUE;
    powerRequestAction.ProcessHandle = ProcessHandle;

    status = NtPowerInformation(
        PowerRequestAction,
        &powerRequestAction,
        sizeof(POWER_REQUEST_ACTION),
        NULL,
        0
        );

    if (NT_SUCCESS(status))
    {
        *PowerRequestHandle = powerRequestHandle;
    }
    else
    {
        NtClose(powerRequestHandle);
    }

    return status;
}

// rev from RtlpDestroyExecutionRequiredRequest (dmex)
/**
 * Destroys an execution required power request handle.
 *
 * \param PowerRequestHandle An optional handle to the power request to destroy.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhDestroyExecutionRequiredRequest(
    _In_opt_ _Post_ptr_invalid_ HANDLE PowerRequestHandle
    )
{
    POWER_REQUEST_ACTION requestPowerAction;

    if (!PowerRequestHandle)
        return STATUS_INVALID_PARAMETER;

    memset(&requestPowerAction, 0, sizeof(POWER_REQUEST_ACTION));
    requestPowerAction.PowerRequestHandle = PowerRequestHandle;
    requestPowerAction.RequestType = PowerRequestExecutionRequiredInternal;
    requestPowerAction.SetAction = FALSE;
    requestPowerAction.ProcessHandle = NULL;

    NtPowerInformation(
        PowerRequestAction,
        &requestPowerAction,
        sizeof(POWER_REQUEST_ACTION),
        NULL,
        0
        );

    return NtClose(PowerRequestHandle);
}

/**
 * Retrieves the nominal frequency (in MHz) of the specified processor.
 *
 * \param ProcessorNumber The processor number structure identifying the target processor.
 * \param NominalFrequency Pointer to a variable that receives the nominal frequency of the processor.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessorNominalFrequency(
    _In_ PH_PROCESSOR_NUMBER ProcessorNumber,
    _Out_ PULONG NominalFrequency
    )
{
    NTSTATUS status;
    POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT frequencyInput;
    POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT frequencyOutput;

    memset(&frequencyInput, 0, sizeof(frequencyInput));
    frequencyInput.InternalType = PowerInternalProcessorBrandedFrequency;
    frequencyInput.ProcessorNumber.Group = ProcessorNumber.Group; // USHRT_MAX for max
    frequencyInput.ProcessorNumber.Number = (BYTE)ProcessorNumber.Number; // UCHAR_MAX for max
    frequencyInput.ProcessorNumber.Reserved = 0; // UCHAR_MAX

    memset(&frequencyOutput, 0, sizeof(frequencyOutput));
    frequencyOutput.Version = POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION;

    status = NtPowerInformation(
        PowerInformationInternal,
        &frequencyInput,
        sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT),
        &frequencyOutput,
        sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT)
        );

    if (NT_SUCCESS(status))
    {
        if (frequencyOutput.Version == POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION)
        {
            *NominalFrequency = frequencyOutput.NominalFrequency;
        }
        else
        {
            status = STATUS_INVALID_KERNEL_INFO_VERSION;
        }
    }

    return status;
}

//
// Process freeze/thaw support
//

/**
 * Freezes the execution of a specified process.
 *
 * \param[out] ProcessStateChangeHandle A pointer to a handle that will receive the state change handle for the frozen process.
 * \param[in] ProcessHandle The handle to the process to be frozen.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhFreezeProcess(
    _Out_ PHANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE processStateChangeHandle = NULL;

    if (!(NtCreateProcessStateChange_Import() && NtChangeProcessState_Import()))
        return STATUS_PROCEDURE_NOT_FOUND;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = NtCreateProcessStateChange_Import()(
        &processStateChangeHandle,
        STATECHANGE_SET_ATTRIBUTES,
        &objectAttributes,
        ProcessHandle,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtChangeProcessState_Import()(
        processStateChangeHandle,
        ProcessHandle,
        ProcessStateChangeSuspend,
        NULL,
        0,
        0
        );

    if (NT_SUCCESS(status))
    {
        *ProcessStateChangeHandle = processStateChangeHandle;
    }

    return status;
}

/**
 * Freezes a process specified by its process ID.
 *
 * \param[out] ProcessStateChangeHandle A pointer to a handle that will receive the state change handle for the frozen process.
 * \param[in] ProcessId The handle to the process to be frozen.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhFreezeProcessById(
    _Out_ PHANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE processStateChangeHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhFreezeProcess(
        &processStateChangeHandle,
        processHandle
        );

    NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        *ProcessStateChangeHandle = processStateChangeHandle;
    }

    return status;
}

/**
 * Thaws (resumes) a previously frozen process.
 *
 * \param ProcessStateChangeHandle Handle to the process state change object.
 * \param ProcessHandle Handle to the target process to be thawed.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhThawProcess(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;

    if (!NtChangeProcessState_Import())
    {
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    status = NtChangeProcessState_Import()(
        ProcessStateChangeHandle,
        ProcessHandle,
        ProcessStateChangeResume,
        NULL,
        0,
        0
        );

    return status;
}

/**
 * Thaws (resumes) a process specified by its process ID.
 *
 * \param ProcessStateChangeHandle Handle used to manage process state changes.
 * \param ProcessId Handle to the process to be thawed (resumed).
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhThawProcessById(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhThawProcess(
        ProcessStateChangeHandle,
        processHandle
        );

    NtClose(processHandle);

    return status;
}

/**
 * Freezes (suspends) the specified thread.
 *
 * \param[out] ThreadStateChangeHandle A pointer to a handle that receives the thread state change handle.
 * \param[in] ThreadHandle A handle to the thread to be frozen.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhFreezeThread(
    _Out_ PHANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadHandle
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE threadStateChangeHandle = NULL;

    if (!(NtCreateThreadStateChange_Import() && NtChangeThreadState_Import()))
    {
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = NtCreateThreadStateChange_Import()(
        &threadStateChangeHandle,
        STATECHANGE_SET_ATTRIBUTES,
        &objectAttributes,
        ThreadHandle,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    status = NtChangeThreadState_Import()(
        threadStateChangeHandle,
        ThreadHandle,
        ThreadStateChangeSuspend,
        NULL,
        0,
        0
        );

    if (NT_SUCCESS(status))
    {
        *ThreadStateChangeHandle = threadStateChangeHandle;
    }

    return status;
}

/**
 * Freezes the specified thread by its ID.
 *
 * \param ThreadStateChangeHandle A pointer to a handle that will receive the thread state change handle.
 * \param ThreadId The ID of the thread to freeze.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhFreezeThreadById(
    _Out_ PHANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    HANDLE threadStateChangeHandle;

    status = PhOpenThread(
        &threadHandle,
        THREAD_SET_INFORMATION | THREAD_SUSPEND_RESUME,
        ThreadId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhFreezeThread(
        &threadStateChangeHandle,
        threadHandle
        );

    if (NT_SUCCESS(status))
    {
        *ThreadStateChangeHandle = threadStateChangeHandle;
    }

    NtClose(threadHandle);

    return status;
}

/**
 * Resumes a suspended thread using the thread state change mechanism.
 *
 * \param ThreadStateChangeHandle A handle to the thread state change object, obtained from a prior suspend operation.
 * \param ThreadHandle A handle to the thread to be resumed.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhThawThread(
    _In_ HANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadHandle
    )
{
    NTSTATUS status;

    if (!NtChangeThreadState_Import())
    {
        return STATUS_PROCEDURE_NOT_FOUND;
    }

    status = NtChangeThreadState_Import()(
        ThreadStateChangeHandle,
        ThreadHandle,
        ThreadStateChangeResume,
        NULL,
        0,
        0
        );

    return status;
}

/**
 * Resumes (thaws) a suspended thread by its ID using a thread state change handle.
 *
 * \param ThreadStateChangeHandle A handle to the thread state change object, obtained from a previous suspend operation.
 * \param ThreadId The ID of the thread to resume.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhThawThreadById(
    _In_ HANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadId
    )
{
    NTSTATUS status;
    HANDLE threadHandle;

    status = PhOpenThread(
        &threadHandle,
        THREAD_SET_INFORMATION | THREAD_SUSPEND_RESUME,
        ThreadId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhThawThread(
        ThreadStateChangeHandle,
        threadHandle
        );

    NtClose(threadHandle);

    return status;
}

//
// Process execution request support
//

static PH_INITONCE PhExecutionRequestInitOnce = PH_INITONCE_INIT;
static PPH_HASHTABLE PhExecutionRequestHashtable = NULL;

typedef struct _PH_EXECUTIONREQUEST_CACHE_ENTRY
{
    HANDLE ProcessId;
    HANDLE ExecutionRequestHandle;
} PH_EXECUTIONREQUEST_CACHE_ENTRY, *PPH_EXECUTIONREQUEST_CACHE_ENTRY;

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
static BOOLEAN NTAPI PhExecutionRequestHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return
        ((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry1)->ProcessId ==
        ((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry2)->ProcessId;
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
static ULONG NTAPI PhExecutionRequestHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return HandleToUlong(((PPH_EXECUTIONREQUEST_CACHE_ENTRY)Entry)->ProcessId) / 4;
}

/**
 * Initializes the execution request table.
 *
 * This function sets up the execution request table, which is used to manage
 * and handle execution requests within the system.
 * \return TRUE if the initialization succeeds, FALSE otherwise.
 */
BOOLEAN PhInitializeExecutionRequestTable(
    VOID
    )
{
    if (PhBeginInitOnce(&PhExecutionRequestInitOnce))
    {
        PhExecutionRequestHashtable = PhCreateHashtable(
            sizeof(PH_EXECUTIONREQUEST_CACHE_ENTRY),
            PhExecutionRequestHashtableEqualFunction,
            PhExecutionRequestHashtableHashFunction,
            1
            );

        PhEndInitOnce(&PhExecutionRequestInitOnce);
    }

    return TRUE;
}

/**
 * Determines whether execution is required for the specified process.
 *
 * This function checks if the process identified by the given process ID requires
 * execution, typically in the context of process management or debugging.
 *
 * \param ProcessId A handle to the process ID to check.
 * \return TRUE if execution is required for the process, FALSE otherwise.
 */
BOOLEAN PhIsProcessExecutionRequired(
    _In_ HANDLE ProcessId
    )
{
    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;

        if (PhFindEntryHashtable(PhExecutionRequestHashtable, &entry))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Enables the execution required state for the specified process.
 * This prevents the process from being terminated or suspended by the system
 * under certain conditions, ensuring it remains running.
 *
 * \param ProcessId The handle to the process for which to enable execution required.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhProcessExecutionRequiredEnable(
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    HANDLE requestHandle;

    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;

        if (PhFindEntryHashtable(PhExecutionRequestHashtable, &entry))
        {
            return STATUS_SUCCESS;
        }
    }

    status = PhOpenProcess(
        &processHandle,
        PROCESS_SET_LIMITED_INFORMATION,
        ProcessId
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhCreateExecutionRequiredRequest(
        processHandle,
        &requestHandle
        );

    NtClose(processHandle);

    if (NT_SUCCESS(status))
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        entry.ProcessId = ProcessId;
        entry.ExecutionRequestHandle = requestHandle;

        PhAddEntryHashtable(PhExecutionRequestHashtable, &entry);
    }

    return status;
}

/**
 * Disables the execution required state for the specified process.
 *
 * \param ProcessId A handle to the process for which the execution required state should be disabled.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhProcessExecutionRequiredDisable(
    _In_ HANDLE ProcessId
    )
{
    if (PhInitializeExecutionRequestTable())
    {
        PH_EXECUTIONREQUEST_CACHE_ENTRY lookupEntry;
        PPH_EXECUTIONREQUEST_CACHE_ENTRY entry;

        lookupEntry.ProcessId = ProcessId;

        if (entry = PhFindEntryHashtable(PhExecutionRequestHashtable, &lookupEntry))
        {
            HANDLE requestHandle = entry->ExecutionRequestHandle;

            PhRemoveEntryHashtable(PhExecutionRequestHashtable, &lookupEntry);

            if (requestHandle)
            {
                return PhDestroyExecutionRequiredRequest(requestHandle);
            }
        }
    }

    return STATUS_UNSUCCESSFUL;
}

// KnownDLLs cache support

static PH_INITONCE PhKnownDllsInitOnce = PH_INITONCE_INIT;
static PPH_HASHTABLE PhKnownDllsHashtable = NULL;

typedef struct _PH_KNOWNDLL_CACHE_ENTRY
{
    PPH_STRING FileName;
} PH_KNOWNDLL_CACHE_ENTRY, *PPH_KNOWNDLL_CACHE_ENTRY;

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
static BOOLEAN NTAPI PhKnownDllsHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    return PhEqualStringRef(&((PPH_KNOWNDLL_CACHE_ENTRY)Entry1)->FileName->sr, &((PPH_KNOWNDLL_CACHE_ENTRY)Entry2)->FileName->sr, FALSE);
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
static ULONG NTAPI PhKnownDllsHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRefEx(&((PPH_KNOWNDLL_CACHE_ENTRY)Entry)->FileName->sr, FALSE, PH_STRING_HASH_XXH32);
}

_Function_class_(PH_ENUM_DIRECTORY_OBJECTS)
static NTSTATUS NTAPI PhpKnownDllObjectsCallback(
    _In_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF Name,
    _In_ PPH_STRINGREF TypeName,
    _In_ PVOID Context
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    UNICODE_STRING objectName;
    PVOID baseAddress;
    SIZE_T viewSize;
    PPH_STRING fileName;

    if (!PhStringRefToUnicodeString(Name, &objectName))
        goto CleanupExit;

    status = PhOpenSection(
        &sectionHandle,
        SECTION_MAP_READ,
        RootDirectory,
        Name
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    baseAddress = NULL;
    viewSize = PAGE_SIZE;

    status = PhMapViewOfSection(
        sectionHandle,
        NtCurrentProcess(),
        &baseAddress,
        0,
        NULL,
        &viewSize,
        ViewUnmap,
        WindowsVersion < WINDOWS_10_RS2 ? 0 : MEM_MAPPED,
        PAGE_READONLY
        );

    NtClose(sectionHandle);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhGetProcessMappedFileName(
        NtCurrentProcess(),
        baseAddress,
        &fileName
        );

    PhUnmapViewOfSection(NtCurrentProcess(), baseAddress);

    if (NT_SUCCESS(status))
    {
        PH_KNOWNDLL_CACHE_ENTRY entry;

        entry.FileName = fileName;

        PhAddEntryHashtable(PhKnownDllsHashtable, &entry);
    }

CleanupExit:
    return STATUS_SUCCESS; // continue enumeration (dmex)
}

/**
 * Initializes the known DLLs for the system based on the provided object name.
 * \param ObjectName A pointer to a constant string reference specifying the object name.
 */
VOID PhInitializeKnownDlls(
    _In_ PCPH_STRINGREF ObjectName
    )
{
    HANDLE directoryHandle;

    if (NT_SUCCESS(PhOpenDirectoryObject(
        &directoryHandle,
        DIRECTORY_QUERY,
        NULL,
        ObjectName
        )))
    {
        PhEnumDirectoryObjects(
            directoryHandle,
            PhpKnownDllObjectsCallback,
            NULL
            );
        NtClose(directoryHandle);
    }
}

/**
 * Initializes the Known DLLs table used by the system.
 * \return TRUE if the Known DLLs table was successfully initialized, FALSE otherwise.
 */
BOOLEAN PhInitializeKnownDllsTable(
    VOID
    )
{
    if (PhBeginInitOnce(&PhKnownDllsInitOnce))
    {
        static const PH_STRINGREF KnownDllsObjectName = PH_STRINGREF_INIT(L"\\KnownDlls");
        static const PH_STRINGREF KnownDlls32ObjectName = PH_STRINGREF_INIT(L"\\KnownDlls32");
#ifdef _ARM64_
        static const PH_STRINGREF KnownDllsArm32ObjectName = PH_STRINGREF_INIT(L"\\KnownDllsArm32");
        static const PH_STRINGREF KnownDllsChpe32ObjectName = PH_STRINGREF_INIT(L"\\KnownDllsChpe32");
#endif

        PhKnownDllsHashtable = PhCreateHashtable(
            sizeof(PH_KNOWNDLL_CACHE_ENTRY),
            PhKnownDllsHashtableEqualFunction,
            PhKnownDllsHashtableHashFunction,
            10
            );

        PhInitializeKnownDlls(&KnownDllsObjectName);
        PhInitializeKnownDlls(&KnownDlls32ObjectName);
#ifdef _ARM64_
        PhInitializeKnownDlls(&KnownDllsArm32ObjectName);
        PhInitializeKnownDlls(&KnownDllsChpe32ObjectName);
#endif
        PhEndInitOnce(&PhKnownDllsInitOnce);
    }

    return TRUE;
}

/**
 * Checks if the specified file name corresponds to a known DLL.
 *
 * \param FileName A pointer to a PPH_STRING containing the file name to check.
 * \return TRUE if the file name is a known DLL, FALSE otherwise.
 */
BOOLEAN PhIsKnownDllFileName(
    _In_ PPH_STRING FileName
    )
{
    if (PhInitializeKnownDllsTable())
    {
        PH_KNOWNDLL_CACHE_ENTRY entry;

        entry.FileName = FileName;

        if (PhFindEntryHashtable(PhKnownDllsHashtable, &entry))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Retrieves the system processor performance distribution information.
 *
 * \param Buffer A pointer to the performance distribution data for all processors in the system.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSystemProcessorPerformanceDistribution(
    _Out_ PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemProcessorPerformanceDistribution,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemProcessorPerformanceDistribution,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

/**
 * Retrieves the processor performance distribution information for a specified processor group.
 *
 * \param ProcessorGroup The processor group number for which to retrieve performance distribution information.
 * \param Buffer A pointer to a variable that receives a pointer to a SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION 
 * structure containing the performance distribution data.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSystemProcessorPerformanceDistributionEx(
    _In_ USHORT ProcessorGroup,
    _Out_ PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformationEx(
        SystemProcessorPerformanceDistribution,
        &ProcessorGroup,
        sizeof(USHORT),
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformationEx(
            SystemProcessorPerformanceDistribution,
            &ProcessorGroup,
            sizeof(USHORT),
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
}

/**
 * Retrieves information about the logical processors in the system.
 *
 * \param RelationshipType The relationship type to filter the logical processor information (e.g., processor core, NUMA node).
 * \param Buffer A pointer to a variable that receives a pointer to a buffer containing an array of SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX structures.
 * \param BufferLength A pointer to a variable that on input specifies the size of the buffer pointed to by Buffer,
 * and on output receives the number of bytes returned or required.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSystemLogicalProcessorInformation(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_ PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *Buffer,
    _Out_ PULONG BufferLength
    )
{
    static ULONG initialBufferSize[] = { 0x200, 0x80, 0x100, 0x1000 };
    NTSTATUS status;
    ULONG classIndex;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    switch (RelationshipType)
    {
    case RelationProcessorCore:
        classIndex = 0;
        break;
    case RelationProcessorPackage:
        classIndex = 1;
        break;
    case RelationGroup:
        classIndex = 2;
        break;
    case RelationAll:
        classIndex = 3;
        break;
    default:
        return STATUS_INVALID_INFO_CLASS;
    }

    bufferSize = initialBufferSize[classIndex];
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformationEx(
        SystemLogicalProcessorAndGroupInformation,
        &RelationshipType,
        sizeof(LOGICAL_PROCESSOR_RELATIONSHIP),
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformationEx(
            SystemLogicalProcessorAndGroupInformation,
            &RelationshipType,
            sizeof(LOGICAL_PROCESSOR_RELATIONSHIP),
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    if (bufferSize <= 0x100000) initialBufferSize[classIndex] = bufferSize;
    *Buffer = buffer;
    *BufferLength = bufferSize;

    return status;
}

/**
 * Retrieves information about the logical processor relationships in the system.
 *
 * \param LogicalProcessorInformation A pointer to a PH_LOGICAL_PROCESSOR_INFORMATION structure 
 * that receives the logical processor relationship information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSystemLogicalProcessorRelationInformation(
    _Out_ PPH_LOGICAL_PROCESSOR_INFORMATION LogicalProcessorInformation
    )
{
    NTSTATUS status;
    ULONG logicalInformationLength = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation;

    status = PhGetSystemLogicalProcessorInformation(
        RelationAll,
        &logicalInformation,
        &logicalInformationLength
        );

    if (NT_SUCCESS(status))
    {
        ULONG processorCoreCount = 0;
        ULONG processorNumaCount = 0;
        ULONG processorLogicalCount = 0;
        ULONG processorPackageCount = 0;

        for (
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo = logicalInformation;
            (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, logicalInformationLength);
            processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
            )
        {
            switch (processorInfo->Relationship)
            {
            case RelationProcessorCore:
                {
                    processorCoreCount++;

                    for (USHORT j = 0; j < processorInfo->Processor.GroupCount; j++)
                    {
                        processorLogicalCount += PhCountBitsUlongPtr(processorInfo->Processor.GroupMask[j].Mask); // RtlNumberOfSetBitsUlongPtr
                    }
                }
                break;
            case RelationNumaNode:
                processorNumaCount++;
                break;
            case RelationProcessorPackage:
                processorPackageCount++;
                break;
            }
        }

        memset(LogicalProcessorInformation, 0, sizeof(PH_LOGICAL_PROCESSOR_INFORMATION));
        LogicalProcessorInformation->ProcessorCoreCount = processorCoreCount;
        LogicalProcessorInformation->ProcessorNumaCount = processorNumaCount;
        LogicalProcessorInformation->ProcessorLogicalCount = processorLogicalCount;
        LogicalProcessorInformation->ProcessorPackageCount = processorPackageCount;

        PhFree(logicalInformation);
    }

    return status;
}

// based on RtlIsProcessorFeaturePresent (dmex)
/**
 * Checks if a specific processor feature is present on the system.
 *
 * This function first checks if the current Windows version is older than a specified version
 * and if the requested processor feature index is within the valid range. If so, it queries
 * the processor feature from the user shared data. Otherwise, it calls the system API
 * `IsProcessorFeaturePresent` to determine the presence of the feature.
 *
 * \param ProcessorFeature The index of the processor feature to check.
 * \return TRUE if the processor feature is present, FALSE otherwise.
 */
BOOLEAN PhIsProcessorFeaturePresent(
    _In_ ULONG ProcessorFeature
    )
{
    if (WindowsVersion < WINDOWS_NEW && ProcessorFeature < PROCESSOR_FEATURE_MAX)
    {
        return USER_SHARED_DATA->ProcessorFeatures[ProcessorFeature];
    }

    return !!IsProcessorFeaturePresent(ProcessorFeature); // RtlIsProcessorFeaturePresent
}

/**
 * Retrieves the processor number information for the current processor.
 *
 * \param ProcessorNumber A pointer to a PROCESSOR_NUMBER structure that receives the processor number details.
 */
VOID PhGetCurrentProcessorNumber(
    _Out_ PPROCESSOR_NUMBER ProcessorNumber
    )
{
    //if (PhIsProcessorFeaturePresent(PF_RDPID_INSTRUCTION_AVAILABLE))
//    _rdpid_u32();
//if (PhIsProcessorFeaturePresent(PF_RDTSCP_INSTRUCTION_AVAILABLE))
//    __rdtscp();

    memset(ProcessorNumber, 0, sizeof(PROCESSOR_NUMBER));

    RtlGetCurrentProcessorNumberEx(ProcessorNumber);
}

// based on GetActiveProcessorCount (dmex)
/**
 * Retrieves the number of active processors in the specified processor group.
 *
 * \param ProcessorGroup The processor group for which to retrieve the active processor count.
 * \return The number of active processors in the specified group.
 */
USHORT PhGetActiveProcessorCount(
    _In_ USHORT ProcessorGroup
    )
{
    if (PhSystemProcessorInformation.ActiveProcessorCount)
    {
        USHORT numberOfProcessors = 0;

        if (ProcessorGroup == ALL_PROCESSOR_GROUPS)
        {
            for (USHORT i = 0; i < PhSystemProcessorInformation.NumberOfProcessorGroups; i++)
            {
                numberOfProcessors += PhSystemProcessorInformation.ActiveProcessorCount[i];
            }
        }
        else
        {
            if (ProcessorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups)
            {
                numberOfProcessors = PhSystemProcessorInformation.ActiveProcessorCount[ProcessorGroup];
            }
        }

        return numberOfProcessors;
    }
    else
    {
        return PhSystemProcessorInformation.NumberOfProcessors;
    }
}

/**
 * Retrieves the processor number structure corresponding to a given processor index.
 *
 * \param ProcessorIndex The zero-based index of the processor.
 * \param ProcessorNumber A pointer to a PPH_PROCESSOR_NUMBER structure that receives the processor number information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessorNumberFromIndex(
    _In_ ULONG ProcessorIndex,
    _Out_ PPH_PROCESSOR_NUMBER ProcessorNumber
    )
{
    USHORT processorIndex = 0;

    for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
    {
        USHORT processorCount = PhGetActiveProcessorCount(processorGroup);

        for (USHORT processorNumber = 0; processorNumber < processorCount; processorNumber++)
        {
            if (processorIndex++ == ProcessorIndex)
            {
                memset(ProcessorNumber, 0, sizeof(PH_PROCESSOR_NUMBER));
                ProcessorNumber->Group = processorGroup;
                ProcessorNumber->Number = processorNumber;
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_UNSUCCESSFUL;
}

/**
 * Retrieves the active processor affinity mask for a specified processor group.
 *
 * \param ProcessorGroup The processor group number for which to retrieve the active affinity mask.
 * \param ActiveProcessorMask A pointer to a variable that receives the active processor affinity mask for the specified group.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessorGroupActiveAffinityMask(
    _In_ USHORT ProcessorGroup,
    _Out_ PKAFFINITY ActiveProcessorMask
    )
{
    NTSTATUS status;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInformation;
    ULONG processorInformationLength;

    status = PhGetSystemLogicalProcessorInformation(
        RelationGroup,
        &processorInformation,
        &processorInformationLength
        );

    if (NT_SUCCESS(status))
    {
        if (ProcessorGroup < processorInformation->Group.ActiveGroupCount)
        {
            *ActiveProcessorMask = processorInformation->Group.GroupInfo[ProcessorGroup].ActiveProcessorMask;
        }
        else
        {
            status = STATUS_UNSUCCESSFUL;
        }

        PhFree(processorInformation);
    }

    return status;
}

/**
 * Retrieves the system affinity mask for active processors.
 *
 * \param[out] ActiveProcessorsAffinityMask Pointer to a variable that receives the affinity mask representing active processors in the system.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetProcessorSystemAffinityMask(
    _Out_ PKAFFINITY ActiveProcessorsAffinityMask
    )
{
    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        *ActiveProcessorsAffinityMask = PhSystemBasicInformation.ActiveProcessorsAffinityMask;
        return STATUS_SUCCESS;
    }
    else
    {
        PROCESSOR_NUMBER processorNumber;

        PhGetCurrentProcessorNumber(&processorNumber);

        return PhGetProcessorGroupActiveAffinityMask(processorNumber.Group, ActiveProcessorsAffinityMask);
    }
}

// rev from GetNumaHighestNodeNumber (dmex)
/**
 * Retrieves the highest NUMA (Non-Uniform Memory Access) node number available on the system.
 *
 * \param[out] NodeNumber A pointer to a variable that receives the highest NUMA node number.
 * \return Returns an NTSTATUS code indicating success or failure of the operation.
 * \remarks The function queries the system for NUMA topology information and stores the highest node number
 * in the variable pointed to by NodeNumber. The value is zero-based; for example, if the system has
 * two NUMA nodes, the highest node number will be 1.
 */
NTSTATUS PhGetNumaHighestNodeNumber(
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_INFORMATION numaProcessorMap;

    status = NtQuerySystemInformation(
        SystemNumaProcessorMap,
        &numaProcessorMap,
        sizeof(SYSTEM_NUMA_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *NodeNumber = (USHORT)numaProcessorMap.HighestNodeNumber;
    }

    return status;
}

// rev from GetNumaProcessorNodeEx (dmex)
/**
 * Retrieves the NUMA node number for a specified processor.
 *
 * \param ProcessorNumber A pointer to a PPH_PROCESSOR_NUMBER structure that specifies the processor for which to retrieve the NUMA node.
 * \param NodeNumber A pointer to a variable that receives the NUMA node number associated with the specified processor.
 * \return Returns TRUE if the NUMA node number was successfully retrieved; otherwise, returns FALSE.
 */
BOOLEAN PhGetNumaProcessorNode(
    _In_ PPH_PROCESSOR_NUMBER ProcessorNumber,
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_INFORMATION numaProcessorMap;
    USHORT processorNode = 0;

    if (ProcessorNumber->Group >= 20 || ProcessorNumber->Number >= MAXIMUM_PROC_PER_GROUP)
    {
        *NodeNumber = USHRT_MAX;
        return FALSE;
    }

    status = NtQuerySystemInformation(
        SystemNumaProcessorMap,
        &numaProcessorMap,
        sizeof(SYSTEM_NUMA_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        *NodeNumber = USHRT_MAX;
        return FALSE;
    }

    while (
        numaProcessorMap.ActiveProcessorsGroupAffinity[processorNode].Group != ProcessorNumber->Group ||
        (numaProcessorMap.ActiveProcessorsGroupAffinity[processorNode].Mask & AFFINITY_MASK(ProcessorNumber->Number)) == 0
        )
    {
        if (++processorNode > numaProcessorMap.HighestNodeNumber)
        {
            *NodeNumber = USHRT_MAX;
            return FALSE;
        }
    }

    *NodeNumber = processorNode;
    return TRUE;
}

// rev from GetNumaProximityNodeEx (dmex)
/**
 * Retrieves the NUMA (Non-Uniform Memory Access) node number associated with a given proximity ID.
 *
 * \param ProximityId The proximity ID for which to retrieve the corresponding NUMA node number.
 * \param NodeNumber A pointer to a variable that receives the NUMA node number associated with the specified proximity ID.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetNumaProximityNode(
    _In_ ULONG ProximityId,
    _Out_ PUSHORT NodeNumber
    )
{
    NTSTATUS status;
    SYSTEM_NUMA_PROXIMITY_MAP numaProximityMap;

    memset(&numaProximityMap, 0, sizeof(SYSTEM_NUMA_PROXIMITY_MAP));
    numaProximityMap.NodeProximityId = ProximityId;

    status = NtQuerySystemInformation(
        SystemNumaProximityNodeInformation,
        &numaProximityMap,
        sizeof(SYSTEM_NUMA_PROXIMITY_MAP),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *NodeNumber = numaProximityMap.NodeNumber;
    }

    return status;
}

DECLSPEC_GUARDNOCF
static NTSTATUS PhpSetInformationVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ VIRTUAL_MEMORY_INFORMATION_CLASS VmInformationClass,
    _In_ ULONG_PTR NumberOfEntries,
    _In_reads_(NumberOfEntries) PMEMORY_RANGE_ENTRY VirtualAddresses,
    _In_reads_bytes_(VmInformationLength) PVOID VmInformation,
    _In_ ULONG VmInformationLength
    )
{
    assert(NtSetInformationVirtualMemory_Import());

    //
    // Our custom loader logic creates chicken and egg problem. To register
    // valid CFG call targets, we need access to NtSetInformationVirtualMemory,
    // but that function itself cannot be marked as a valid target until it has
    // been imported. Here we wrap the call in a routine that disables CFG.
    //
    return NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmInformationClass,
        NumberOfEntries,
        VirtualAddresses,
        VmInformation,
        VmInformationLength
        );
}

// rev from PrefetchVirtualMemory (dmex)
/**
 * Provides an efficient mechanism to bring into memory potentially discontiguous virtual address ranges in a process address space.
 *
 * \param ProcessHandle A handle to the process whose virtual address ranges are to be prefetched.
 * \param NumberOfEntries Number of entries in the array pointed to by the VirtualAddresses parameter.
 * \param VirtualAddresses A pointer to an array of MEMORY_RANGE_ENTRY structures which each specify a virtual address range
 * to be prefetched. The virtual address ranges may cover any part of the process address space accessible by the target process.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhPrefetchVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ SIZE_T NumberOfEntries,
    _In_ PMEMORY_RANGE_ENTRY VirtualAddresses
    )
{
    NTSTATUS status;
    ULONG prefetchInformationFlags;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&prefetchInformationFlags, 0, sizeof(prefetchInformationFlags));

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmPrefetchInformation,
        NumberOfEntries,
        VirtualAddresses,
        &prefetchInformationFlags,
        sizeof(prefetchInformationFlags)
        );

    return status;
}

// rev from OfferVirtualMemory (dmex)
//NTSTATUS PhOfferVirtualMemory(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress,
//    _In_ SIZE_T NumberOfBytes,
//    _In_ MEMORY_PAGE_PRIORITY_INFORMATION Priority
//    )
//{
//    NTSTATUS status;
//    MEMORY_RANGE_ENTRY virtualMemoryRange;
//    ULONG virtualMemoryFlags;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    // TODO: NtQueryVirtualMemory (dmex)
//
//    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
//    virtualMemoryRange.VirtualAddress = VirtualAddress;
//    virtualMemoryRange.NumberOfBytes = NumberOfBytes;
//
//    memset(&virtualMemoryFlags, 0, sizeof(virtualMemoryFlags));
//    virtualMemoryFlags = Priority;
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmPagePriorityInformation,
//        1,
//        &virtualMemoryRange,
//        &virtualMemoryFlags,
//        sizeof(virtualMemoryFlags)
//        );
//
//    return status;
//}
//
// rev from DiscardVirtualMemory (dmex)
//NTSTATUS PhDiscardVirtualMemory(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress,
//    _In_ SIZE_T NumberOfBytes
//    )
//{
//    NTSTATUS status;
//    MEMORY_RANGE_ENTRY virtualMemoryRange;
//    ULONG virtualMemoryFlags;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
//    virtualMemoryRange.VirtualAddress = VirtualAddress;
//    virtualMemoryRange.NumberOfBytes = NumberOfBytes;
//
//    memset(&virtualMemoryFlags, 0, sizeof(virtualMemoryFlags));
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmPagePriorityInformation,
//        1,
//        &virtualMemoryRange,
//        &virtualMemoryFlags,
//        sizeof(virtualMemoryFlags)
//        );
//
//    return status;
//}

/**
 * Sets the priority of a range of virtual memory pages in a specified process.
 *
 * \param ProcessHandle Handle to the process whose memory pages will be modified.
 * \param PagePriority The priority value to assign to the memory pages.
 * \param VirtualAddress Pointer to the starting address of the memory region.
 * \param NumberOfBytes Size, in bytes, of the memory region to modify.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetVirtualMemoryPagePriority(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG PagePriority,
    _In_ PVOID VirtualAddress,
    _In_ SIZE_T NumberOfBytes
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY virtualMemoryRange;

    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
    virtualMemoryRange.VirtualAddress = VirtualAddress;
    virtualMemoryRange.NumberOfBytes = NumberOfBytes;

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmPagePriorityInformation,
        1,
        &virtualMemoryRange,
        &PagePriority,
        sizeof(PagePriority)
        );

    return status;
}

// rev from SetProcessValidCallTargets (dmex)
//NTSTATUS PhSetProcessValidCallTarget(
//    _In_ HANDLE ProcessHandle,
//    _In_ PVOID VirtualAddress
//    )
//{
//    NTSTATUS status;
//    MEMORY_BASIC_INFORMATION basicInfo;
//    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
//    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
//    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
//    ULONG numberOfEntriesProcessed = 0;
//
//    if (!NtSetInformationVirtualMemory_Import())
//        return STATUS_PROCEDURE_NOT_FOUND;
//
//    status = NtQueryVirtualMemory(
//        ProcessHandle,
//        VirtualAddress,
//        MemoryBasicInformation,
//        &basicInfo,
//        sizeof(MEMORY_BASIC_INFORMATION),
//        NULL
//        );
//
//    if (!NT_SUCCESS(status))
//        return status;
//
//    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
//    cfgCallTargetInfo.Offset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)basicInfo.AllocationBase;
//    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_VALID;
//
//    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
//    cfgCallTargetRangeInfo.VirtualAddress = basicInfo.AllocationBase;
//    cfgCallTargetRangeInfo.NumberOfBytes = basicInfo.RegionSize;
//
//    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
//    cfgCallTargetListInfo.NumberOfEntries = 1;
//    cfgCallTargetListInfo.Reserved = 0;
//    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
//    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;
//
//    status = PhpSetInformationVirtualMemory(
//        ProcessHandle,
//        VmCfgCallTargetInformation,
//        1,
//        &cfgCallTargetRangeInfo,
//        &cfgCallTargetListInfo,
//        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
//        );
//
//    if (status == STATUS_INVALID_PAGE_PROTECTION)
//        status = STATUS_SUCCESS;
//
//    return status;
//}

// rev from RtlGuardGrantSuppressedCallAccess (dmex)
/**
 * Grants suppressed call access to a specific virtual address in the target process.
 *
 * \param ProcessHandle Handle to the process in which access is to be granted.
 * \param VirtualAddress Pointer to the virtual address for which suppressed call access is requested.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGuardGrantSuppressedCallAccess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = PAGE_ALIGN(VirtualAddress);
    cfgCallTargetRangeInfo.NumberOfBytes = PAGE_SIZE;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = BYTE_OFFSET(VirtualAddress);
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_VALID;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

// rev from RtlDisableXfgOnTarget (dmex)
/**
 * Disables Control Flow Guard (CFG/XFG) protection on a specified memory region in a target process.
 *
 * \param ProcessHandle Handle to the target process.
 * \param VirtualAddress Pointer to the base address of the memory region to modify.
 * \return NTSTATUS code indicating success or failure of the operation.
 */
NTSTATUS PhDisableXfgOnTarget(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    PS_SYSTEM_DLL_INIT_BLOCK systemDllInitBlock;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    if (!NT_SUCCESS(status = PhGetSystemDllInitBlock(&systemDllInitBlock)))
        return status;

    // Check if CFG is disabled. PhGetProcessIsCFGuardEnabled(NtCurrentProcess());
    if (!(systemDllInitBlock.CfgBitMap && systemDllInitBlock.CfgBitMapSize))
        return STATUS_SUCCESS;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = PAGE_ALIGN(VirtualAddress);
    cfgCallTargetRangeInfo.NumberOfBytes = PAGE_SIZE;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = BYTE_OFFSET(VirtualAddress);
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_CONVERT_XFG_TO_CFG;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = PhpSetInformationVirtualMemory(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Retrieves information about the system compression store.
 *
 * \param[out] SystemCompressionStoreInformation A pointer to the compression store information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSystemCompressionStoreInformation(
    _Out_ PPH_SYSTEM_STORE_COMPRESSION_INFORMATION SystemCompressionStoreInformation
    )
{
    NTSTATUS status;
    SYSTEM_STORE_INFORMATION storeInfo;
    SM_STORE_COMPRESSION_INFORMATION_REQUEST compressionInfo;

    memset(&compressionInfo, 0, sizeof(SM_STORE_COMPRESSION_INFORMATION_REQUEST));
    compressionInfo.Version = SYSTEM_STORE_COMPRESSION_INFORMATION_VERSION_V1;

    memset(&storeInfo, 0, sizeof(SYSTEM_STORE_INFORMATION));
    storeInfo.Version = SYSTEM_STORE_INFORMATION_VERSION;
    storeInfo.StoreInformationClass = MemCompressionInfoRequest;
    storeInfo.Data = &compressionInfo;
    storeInfo.Length = SYSTEM_STORE_COMPRESSION_INFORMATION_SIZE_V1;

    status = NtQuerySystemInformation(
        SystemStoreInformation,
        &storeInfo,
        sizeof(SYSTEM_STORE_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        memset(SystemCompressionStoreInformation, 0, sizeof(PH_SYSTEM_STORE_COMPRESSION_INFORMATION));
        SystemCompressionStoreInformation->CompressionPid = compressionInfo.CompressionPid;
        SystemCompressionStoreInformation->WorkingSetSize = compressionInfo.WorkingSetSize;
        SystemCompressionStoreInformation->TotalDataCompressed = compressionInfo.TotalDataCompressed;
        SystemCompressionStoreInformation->TotalCompressedSize = compressionInfo.TotalCompressedSize;
        SystemCompressionStoreInformation->TotalUniqueDataCompressed = compressionInfo.TotalUniqueDataCompressed;
    }

    return status;
}

// rev from PsmServiceExtHost!RmpMemoryMonitorEmptySystemStore (dmex)
/**
 * Requests a trim operation on the system compression store.
 * This function initiates a trim request for the system's compression store,
 * potentially freeing up unused or unnecessary compressed memory.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSystemCompressionStoreTrimRequest(
    VOID
    )
{
    NTSTATUS status;
    SYSTEM_STORE_INFORMATION storeInfo;
    SM_SYSTEM_STORE_TRIM_REQUEST trimRequestInfo;
    PH_SYSTEM_STORE_COMPRESSION_INFORMATION compressionInfo;

    status = PhGetSystemCompressionStoreInformation(&compressionInfo);

    if (!NT_SUCCESS(status))
        return status;

    memset(&trimRequestInfo, 0, sizeof(SM_SYSTEM_STORE_TRIM_REQUEST));
    trimRequestInfo.Version = SYSTEM_STORE_TRIM_INFORMATION_VERSION_V1;
    trimRequestInfo.PagesToTrim = BYTES_TO_PAGES(compressionInfo.WorkingSetSize);

    memset(&storeInfo, 0, sizeof(SYSTEM_STORE_INFORMATION));
    storeInfo.Version = SYSTEM_STORE_INFORMATION_VERSION;
    storeInfo.StoreInformationClass = SystemStoreTrimRequest;
    storeInfo.Data = &trimRequestInfo;
    storeInfo.Length = SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1;

    status = NtQuerySystemInformation(
        SystemStoreInformation,
        &storeInfo,
        sizeof(SYSTEM_STORE_INFORMATION),
        NULL
        );

    return status;
}

/**
 * Requests to set or clear high memory priority for the system compression store of a process.
 *
 * \param ProcessHandle Handle to the target process.
 * \param SetHighMemoryPriority TRUE to set high memory priority, FALSE to clear it.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSystemCompressionStoreHighMemoryPriorityRequest(
    _In_ HANDLE ProcessHandle,
    _In_ BOOLEAN SetHighMemoryPriority
    )
{
    NTSTATUS status;
    SYSTEM_STORE_INFORMATION storeInfo;
    SM_STORE_MEMORY_PRIORITY_REQUEST memoryPriorityInfo;

    memset(&memoryPriorityInfo, 0, sizeof(SM_STORE_MEMORY_PRIORITY_REQUEST));
    memoryPriorityInfo.Version = SYSTEM_STORE_PRIORITY_REQUEST_VERSION;
    memoryPriorityInfo.Flags = SYSTEM_STORE_PRIORITY_FLAG_REQUIRE_HANDLE;
    memoryPriorityInfo.ProcessHandle = ProcessHandle;

    if (SetHighMemoryPriority)
    {
        SetFlag(memoryPriorityInfo.Flags, SYSTEM_STORE_PRIORITY_FLAG_SET_PRIORITY);
    }

    memset(&storeInfo, 0, sizeof(SYSTEM_STORE_INFORMATION));
    storeInfo.Version = SYSTEM_STORE_INFORMATION_VERSION;
    storeInfo.StoreInformationClass = StoreHighMemoryPriorityRequest;
    storeInfo.Data = &memoryPriorityInfo;
    storeInfo.Length = SYSTEM_STORE_TRIM_INFORMATION_SIZE_V1;

    status = NtQuerySystemInformation(
        SystemStoreInformation,
        &storeInfo,
        sizeof(SYSTEM_STORE_INFORMATION),
        NULL
        );

    return status;
}

/**
 * Retrieves the current size limits for the working set of the virtual memory manager system cache.
 *
 * \param CacheInfo A pointer to a variable that receives the file cache information.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetSystemFileCacheSize(
    _Out_ PSYSTEM_FILECACHE_INFORMATION CacheInfo
    )
{
    return NtQuerySystemInformation(
        SystemFileCacheInformationEx,
        CacheInfo,
        sizeof(SYSTEM_FILECACHE_INFORMATION),
        0
        );
}

// rev from SetSystemFileCacheSize (MSDN) (dmex)
/**
 * Limits the size of the working set of the virtual memory manager system cache.
 *
 * \param CacheInfo The minimum size of the file cache, in bytes. The virtual memory manager
 * attempts to keep at least this much memory resident in the system file cache.
 * \param CacheInfo The maximum size of the file cache, in bytes. The virtual memory manager
 * enforces this limit only if this call or a previous call to SetSystemFileCacheSize
 * specifies FILE_CACHE_MAX_HARD_ENABLE.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhSetSystemFileCacheSize(
    _In_ SIZE_T MinimumFileCacheSize,
    _In_ SIZE_T MaximumFileCacheSize,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    SYSTEM_FILECACHE_INFORMATION cacheInfo;

    memset(&cacheInfo, 0, sizeof(SYSTEM_FILECACHE_INFORMATION));
    cacheInfo.MinimumWorkingSet = MinimumFileCacheSize;
    cacheInfo.MaximumWorkingSet = MaximumFileCacheSize;
    cacheInfo.Flags = Flags;

    status = NtSetSystemInformation(
        SystemFileCacheInformationEx,
        &cacheInfo,
        sizeof(SYSTEM_FILECACHE_INFORMATION)
        );

    return status;
}

/**
 * Creates an event object, sets the initial state of the event to the specified value,
 * and opens a handle to the object with the specified desired access.
 *
 * \param EventHandle A pointer to a variable that receives the event object handle.
 * \param DesiredAccess The access mask that specifies the requested access to the event object.
 * \param EventType The type of the event, which can be SynchronizationEvent or a NotificationEvent.
 * \param InitialState The initial state of the event object.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateEvent(
    _Out_ PHANDLE EventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ EVENT_TYPE EventType,
    _In_ BOOLEAN InitialState
    )
{
    NTSTATUS status;
    HANDLE eventHandle;
    OBJECT_ATTRIBUTES objectAttributes;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    status = NtCreateEvent(
        &eventHandle,
        DesiredAccess,
        &objectAttributes,
        EventType,
        InitialState
        );

    if (NT_SUCCESS(status))
    {
        *EventHandle = eventHandle;
    }

    return status;
}

/**
 * Sends a control code directly to a specified device driver, causing the corresponding device to perform the corresponding operation.
 *
 * \param DeviceHandle A handle to the device on which the operation is to be performed.
 * \param IoControlCode The control code for the operation. This value identifies the specific operation to be performed and the type of device on which to perform it.
 * \param InputBuffer A pointer to the input buffer that contains the data required to perform the operation.
 * \param InputBufferLength The size of the input buffer, in bytes.
 * \param OutputBuffer A pointer to the output buffer that is to receive the data returned by the operation.
 * \param OutputBufferLength The size of the output buffer, in bytes.
 * \param ReturnLength A pointer to a variable that receives the size of the data stored in the output buffer, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhDeviceIoControlFile(
    _In_ HANDLE DeviceHandle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    if (DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM)
    {
        status = NtFsControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IoControlCode,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength
            );
    }
    else
    {
        status = NtDeviceIoControlFile(
            DeviceHandle,
            NULL,
            NULL,
            NULL,
            &ioStatusBlock,
            IoControlCode,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength
            );
    }

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(DeviceHandle, FALSE, NULL);

        if (NT_SUCCESS(status))
        {
            status = ioStatusBlock.Status;
        }
    }

    if (ReturnLength)
    {
        *ReturnLength = (ULONG)ioStatusBlock.Information;
    }

    return status;
}

// rev from RtlpWow64SelectSystem32PathInternal (dmex)
/**
 * Selects the appropriate System32 path for the specified machine architecture.
 *
 * \param Machine The machine architecture identifier (e.g., IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_I386).
 * \param IncludePathSeperator If TRUE, appends a path separator to the returned path.
 * \param SystemPath A pointer to a PPH_STRINGREF that receives the selected System32 path.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhWow64SelectSystem32Path(
    _In_ USHORT Machine,
    _In_ BOOLEAN IncludePathSeperator,
    _Out_ PPH_STRINGREF SystemPath
    )
{
    PWSTR WithSeperators;
    PWSTR WithoutSeperators;

    if (Machine != IMAGE_FILE_MACHINE_TARGET_HOST)
    {
        switch (Machine)
        {
        case IMAGE_FILE_MACHINE_I386:
            WithoutSeperators = L"SysWOW64";
            WithSeperators = L"\\SysWOW64\\";
            goto CreateResult;
        case IMAGE_FILE_MACHINE_ARMNT:
            WithoutSeperators = L"SysArm32";
            WithSeperators = L"\\SysArm32\\";
            goto CreateResult;
        case IMAGE_FILE_MACHINE_CHPE_X86:
            WithoutSeperators = L"SyChpe32";
            WithSeperators = L"\\SyChpe32\\";
            goto CreateResult;
        }

        if (Machine != IMAGE_FILE_MACHINE_AMD64 && Machine != IMAGE_FILE_MACHINE_ARM64)
            return STATUS_INVALID_PARAMETER;
    }

    WithSeperators = L"\\System32\\";
    WithoutSeperators = L"System32";

CreateResult:
    if (!IncludePathSeperator)
        WithSeperators = WithoutSeperators;

    PhInitializeStringRefLongHint(SystemPath, WithSeperators); // RtlInitUnicodeString
    return STATUS_SUCCESS;
}

/**
 * Retrieves information about a range of pages within the virtual address space of a specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Callback A callback function which is executed for each memory region.
 * \param Context A user-defined value to pass to the callback function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;

    baseAddress = (PVOID)0;

    while (TRUE)
    {
        status = NtQueryVirtualMemory(
            ProcessHandle,
            baseAddress,
            MemoryBasicInformation,
            &basicInfo,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        if (basicInfo.State & MEM_FREE)
        {
            basicInfo.AllocationBase = basicInfo.BaseAddress;
            basicInfo.AllocationProtect = basicInfo.Protect;
        }

        status = Callback(ProcessHandle, &basicInfo, Context);

        if (!NT_SUCCESS(status))
            break;

        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);

        if ((ULONG_PTR)baseAddress >= PhSystemBasicInformation.MaximumUserModeAddress)
            break;
    }

    return status;
}

/**
 * Retrieves information about a range of pages within the virtual address space of a specified process in batches for improved performance.
 *
 * \param ProcessHandle A handle to a process.
 * \param BaseAddress The base address at which to begin retrieving information.
 * \param BulkQuery A boolean indicating the mode of bulk query (accuracy vs reliability).
 * \param Callback A callback function which is executed for each memory region.
 * \param Context A user-defined value to pass to the callback function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryBulk(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ BOOLEAN BulkQuery,
    _In_ PPH_ENUM_MEMORY_BULK_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;

    if (PhIsExecutingInWow64())
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (!NtPssCaptureVaSpaceBulk_Import())
    {
        return STATUS_NOT_SUPPORTED;
    }

    // BulkQuery... TRUE:
    // * Faster.
    // * More accurate snapshots.
    // * Copies the entire VA space into local memory.
    // * Wastes large amounts of heap memory due to buffer doubling.
    // * Unsuitable for low-memory situations and fails with insufficient system resources.
    // * ...
    //
    // BulkQuery... FALSE:
    // * Slightly slower.
    // * Slightly less accurate snapshots.
    // * Does not copy the VA space.
    // * Does not waste heap memory.
    // * Suitable for low-memory situations and doesn't fail with insufficient system resources.
    // * ...

    if (BulkQuery)
    {
        SIZE_T bufferLength;
        PNTPSS_MEMORY_BULK_INFORMATION buffer;
        PMEMORY_BASIC_INFORMATION information;

        bufferLength = sizeof(NTPSS_MEMORY_BULK_INFORMATION) + sizeof(MEMORY_BASIC_INFORMATION[20]);
        buffer = PhAllocate(bufferLength);
        buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;

        // Allocate a large buffer and copy all entries.

        while ((status = NtPssCaptureVaSpaceBulk_Import()(
            ProcessHandle,
            BaseAddress,
            buffer,
            bufferLength,
            NULL
            )) == STATUS_MORE_ENTRIES)
        {
            PhFree(buffer);
            bufferLength *= 2;

            if (bufferLength > PH_LARGE_BUFFER_SIZE)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferLength);
            buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;
        }

        if (NT_SUCCESS(status))
        {
            // Skip the enumeration header.

            information = PTR_ADD_OFFSET(buffer, RTL_SIZEOF_THROUGH_FIELD(NTPSS_MEMORY_BULK_INFORMATION, NextValidAddress));

            // Execute the callback.

            Callback(ProcessHandle, information, buffer->NumberOfEntries, Context);
        }

        PhFree(buffer);
    }
    else
    {
        UCHAR stackBuffer[sizeof(NTPSS_MEMORY_BULK_INFORMATION) + sizeof(MEMORY_BASIC_INFORMATION[20])];
        SIZE_T bufferLength;
        PNTPSS_MEMORY_BULK_INFORMATION buffer;
        PMEMORY_BASIC_INFORMATION information;

        bufferLength = sizeof(stackBuffer);
        buffer = (PNTPSS_MEMORY_BULK_INFORMATION)stackBuffer;
        buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;
        buffer->NextValidAddress = BaseAddress;

        while (TRUE)
        {
            // Get a batch of entries.

            status = NtPssCaptureVaSpaceBulk_Import()(
                ProcessHandle,
                buffer->NextValidAddress,
                buffer,
                bufferLength,
                NULL
                );

            if (!NT_SUCCESS(status))
                break;

            // Skip the enumeration header.

            information = PTR_ADD_OFFSET(buffer, RTL_SIZEOF_THROUGH_FIELD(NTPSS_MEMORY_BULK_INFORMATION, NextValidAddress));

            // Execute the callback.

            if (!NT_SUCCESS(Callback(ProcessHandle, information, buffer->NumberOfEntries, Context)))
                break;

            // Get the next batch.

            if (status != STATUS_MORE_ENTRIES)
                break;
        }
    }

    return status;
}

/**
 * Retrieves information about the pages currently added to the working set of the specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Callback A callback function which is executed for each memory page.
 * \param Context A user-defined value to pass to the callback function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryPages(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_PAGE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PMEMORY_WORKING_SET_INFORMATION pageInfo;

    status = PhGetProcessWorkingSetInformation(
        ProcessHandle,
        &pageInfo
        );

    if (NT_SUCCESS(status))
    {
        status = Callback(
            ProcessHandle,
            pageInfo->NumberOfEntries,
            pageInfo->WorkingSetInfo,
            Context
            );

        //for (ULONG_PTR i = 0; i < pageInfo->NumberOfEntries; i++)
        //{
        //    PMEMORY_WORKING_SET_BLOCK workingSetBlock = &pageInfo->WorkingSetInfo[i];
        //    PVOID virtualAddress = (PVOID)(workingSetBlock->VirtualPage << PAGE_SHIFT);
        //}

        PhFree(pageInfo);
    }

    return status;
}

/**
 * Retrieves extended information about memory pages in the working set
 * of the specified process, beginning at a given virtual address.
 *
 * \param[in] ProcessHandle A handle to the target process.
 * \param[in] BaseAddress The starting virtual address from which page information should be queried.
 * \param[in] Size The total size, in bytes, of the address range to examine.
 * \param[in] Callback A user-supplied callback invoked once for each page in the range.
 * \param[in] Context A user-defined value passed to the callback function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryAttributes(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size,
    _In_ PPH_ENUM_MEMORY_ATTRIBUTE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SIZE_T numberOfPages;
    ULONG_PTR virtualAddress;
    PMEMORY_WORKING_SET_EX_INFORMATION info;
    SIZE_T i;

    numberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);
    virtualAddress = (ULONG_PTR)PAGE_ALIGN(BaseAddress);

    if (!numberOfPages)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    info = PhAllocatePage(numberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION), NULL);

    if (!info)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    for (i = 0; i < numberOfPages; i++)
    {
        info[i].VirtualAddress = (PVOID)virtualAddress;
        virtualAddress += PAGE_SIZE;
    }

    status = NtQueryVirtualMemory(
        ProcessHandle,
        NULL,
        MemoryWorkingSetExInformation,
        info,
        numberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        status = Callback(
            ProcessHandle,
            BaseAddress,
            Size,
            numberOfPages,
            info,
            Context
            );
    }

    PhFreePage(info);

CleanupExit:
    return status;
}

/**
 * Retrieves information about the state of the kernel debugger.
 *
 * \param[out] KernelDebuggerEnabled Optional. Receives a BOOLEAN value that indicates whether the kernel debugger is enabled.
 * \param[out] KernelDebuggerPresent Optional. Receives a BOOLEAN value that indicates whether the kernel debugger is present.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetKernelDebuggerInformation(
    _Out_opt_ PBOOLEAN KernelDebuggerEnabled,
    _Out_opt_ PBOOLEAN KernelDebuggerPresent
    )
{
    NTSTATUS status;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION debugInfo;

    status = NtQuerySystemInformation(
        SystemKernelDebuggerInformation,
        &debugInfo,
        sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        if (KernelDebuggerEnabled)
            *KernelDebuggerEnabled = debugInfo.KernelDebuggerEnabled;
        if (KernelDebuggerPresent)
            *KernelDebuggerPresent = !debugInfo.KernelDebuggerNotPresent;
    }

    return status;
}

// rev from BasepIsDebugPortPresent (dmex)
/**
 * Checks if a debug port is present for the current process.
 * \return TRUE if a debug port is detected, otherwise FALSE.
 */
BOOLEAN PhIsDebugPortPresent(
    VOID
    )
{
    BOOLEAN isBeingDebugged;

    if (NT_SUCCESS(PhGetProcessIsBeingDebugged(NtCurrentProcess(), &isBeingDebugged)))
    {
        if (isBeingDebugged)
        {
            return TRUE;
        }
    }

    return FALSE;
}

// rev from IsDebuggerPresent (dmex)
/**
 * Determines whether the calling process is being debugged by a user-mode debugger.
 *
 * \return TRUE if the current process is running in the context of a debugger, otherwise the return value is FALSE.
 */
BOOLEAN PhIsDebuggerPresent(
    VOID
    )
{
#ifdef PHNT_NATIVE_DEBUGGER
    return !!IsDebuggerPresent();
#else
    return NtCurrentPeb()->BeingDebugged;
#endif
}

// rev from GetFileType (dmex)
/**
 * Retrieves the type of the specified file handle.
 *
 * \param ProcessHandle A handle to the process.
 * \param FileHandle A handle to the file.
 * \param DeviceType The type of the specified file
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetDeviceType(
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE FileHandle,
    _Out_ DEVICE_TYPE* DeviceType
    )
{
    NTSTATUS status;
    FILE_FS_DEVICE_INFORMATION debugInfo;
    IO_STATUS_BLOCK isb;

    status = PhQueryVolumeInformationFile(
        ProcessHandle,
        FileHandle,
        FileFsDeviceInformation,
        &debugInfo,
        sizeof(FILE_FS_DEVICE_INFORMATION),
        &isb
        );

    if (NT_SUCCESS(status))
    {
        *DeviceType = debugInfo.DeviceType;
    }

    return status;
}

/**
 * Checks if the specified file name is an App Execution Alias target.
 *
 * \param FileName A pointer to a PPH_STRING structure containing the file name to check.
 * \return TRUE if the file name is an App Execution Alias target, FALSE otherwise.
 */
BOOLEAN PhIsAppExecutionAliasTarget(
    _In_ PPH_STRING FileName
    )
{
    PPH_STRING targetFileName = NULL;
    PREPARSE_DATA_BUFFER reparseBuffer;
    ULONG reparseLength;
    HANDLE fileHandle;
    IO_STATUS_BLOCK isb;

    if (PhIsNullOrEmptyString(FileName))
        return FALSE;

    if (!NT_SUCCESS(PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT
        )))
    {
        return FALSE;
    }

    reparseLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
    reparseBuffer = PhAllocateZero(reparseLength);

    if (NT_SUCCESS(NtFsControlFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &isb,
        FSCTL_GET_REPARSE_POINT,
        NULL,
        0,
        reparseBuffer,
        reparseLength
        )))
    {
        if (
            IsReparseTagMicrosoft(reparseBuffer->ReparseTag) &&
            reparseBuffer->ReparseTag == IO_REPARSE_TAG_APPEXECLINK
            )
        {
            PCWSTR string;

            string = (PCWSTR)reparseBuffer->AppExecLinkReparseBuffer.StringList;

            for (ULONG i = 0; i < reparseBuffer->AppExecLinkReparseBuffer.StringCount; i++)
            {
                if (i == 2 && PhDoesFileExistWin32(string))
                {
                    targetFileName = PhCreateString(string);
                    break;
                }

                string += PhCountStringZ(string) + 1;
            }
        }
    }

    PhFree(reparseBuffer);
    NtClose(fileHandle);

    if (targetFileName)
    {
        if (PhDoesFileExistWin32(targetFileName->Buffer))
        {
            PhDereferenceObject(targetFileName);
            return TRUE;
        }

        PhDereferenceObject(targetFileName);
    }

    return FALSE;
}

/**
 * Enumerates the enclaves of a process.
 *
 * \param ProcessHandle Handle to the process whose enclaves are to be enumerated.
 * \param LdrEnclaveList Pointer to the process's loader enclave list.
 * \param Callback Callback function to be called for each enclave found.
 * \param Context Optional context to be passed to the callback function.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumProcessEnclaves(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID LdrEnclaveList,
    _In_ PPH_ENUM_PROCESS_ENCLAVES_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    LIST_ENTRY enclaveList;
    LDR_SOFTWARE_ENCLAVE enclave;

    status = PhReadVirtualMemory(
        ProcessHandle,
        LdrEnclaveList,
        &enclaveList,
        sizeof(LIST_ENTRY),
        NULL
        );
    if (!NT_SUCCESS(status))
        return status;

    for (PLIST_ENTRY link = enclaveList.Flink;
         link != LdrEnclaveList;
         link = enclave.Links.Flink)
    {
        PVOID enclaveAddress;

        enclaveAddress = CONTAINING_RECORD(link, LDR_SOFTWARE_ENCLAVE, Links);

        status = PhReadVirtualMemory(
            ProcessHandle,
            link,
            &enclave,
            sizeof(enclave),
            NULL
            );
        if (!NT_SUCCESS(status))
            return status;

        if (!Callback(ProcessHandle, enclaveAddress, &enclave, Context))
            break;
    }

    return status;
}

#ifdef _M_ARM64
// rev from ntdll!RtlEcContextToNativeContext (jxy-s)
VOID PhEcContextToNativeContext(
    _Out_ PCONTEXT Context,
    _In_ PARM64EC_NT_CONTEXT EcContext
    )
{
    Context->ContextFlags = 0;

    //#define CONTEXT_AMD64   0x00100000L

    //#define CONTEXT_CONTROL         (CONTEXT_AMD64 | 0x00000001L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000001L))
        SetFlag(Context->ContextFlags, CONTEXT_CONTROL);

    //#define CONTEXT_INTEGER         (CONTEXT_AMD64 | 0x00000002L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000002L))
        SetFlag(Context->ContextFlags, CONTEXT_INTEGER);

    //#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x00000008L)
    if (BooleanFlagOn(EcContext->ContextFlags, 0x00100000L | 0x00000008L))
        SetFlag(Context->ContextFlags, CONTEXT_FLOATING_POINT);

    SetFlag(Context->ContextFlags,
            EcContext->ContextFlags & (
                CONTEXT_EXCEPTION_ACTIVE |
                CONTEXT_SERVICE_ACTIVE |
                CONTEXT_EXCEPTION_REQUEST |
                CONTEXT_EXCEPTION_REPORTING
                ));

    Context->Cpsr = (EcContext->AMD64_EFlags & 0x00000100);         // Overflow Flag
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0x00000800) << 4); // Direction Flag
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0xFFFFFFC0) << 7); // Other Flags
    Context->Cpsr |= ((EcContext->AMD64_EFlags & 0x00000001) << 5); // Carry Flag
    Context->Cpsr <<= 13;

    Context->X0 = EcContext->X0;
    Context->X2 = EcContext->X2;
    Context->X4 = EcContext->X4;
    Context->X6 = EcContext->X6;
    Context->X7 = EcContext->X7;
    Context->X8 = EcContext->X8;
    Context->X9 = EcContext->X9;
    Context->X10 = EcContext->X10;
    Context->X11 = EcContext->X11;
    Context->X12 = EcContext->X12;
    Context->X14 = 0;
    Context->X15 = EcContext->X15;

    Context->X16 = EcContext->X16_0;
    Context->X16 |= ((ULONG64)EcContext->X16_1 << 16);
    Context->X16 |= ((ULONG64)EcContext->X16_2 << 32);
    Context->X16 |= ((ULONG64)EcContext->X16_3 << 48);

    Context->X17 = EcContext->X17_0;
    Context->X17 |= ((ULONG64)EcContext->X17_1 << 16);
    Context->X17 |= ((ULONG64)EcContext->X17_2 << 32);
    Context->X17 |= ((ULONG64)EcContext->X17_3 << 48);

    Context->X19 = EcContext->X19;
    Context->X21 = EcContext->X21;
    Context->X23 = 0;
    Context->X25 = EcContext->X25;
    Context->X27 = EcContext->X27;

    Context->Fp = EcContext->Fp;
    Context->Lr = EcContext->Lr;
    Context->Sp = EcContext->Sp;
    Context->Pc = EcContext->Pc;

    Context->V[0] = EcContext->V[0];
    Context->V[1] = EcContext->V[1];
    Context->V[2] = EcContext->V[2];
    Context->V[3] = EcContext->V[3];
    Context->V[4] = EcContext->V[4];
    Context->V[5] = EcContext->V[5];
    Context->V[6] = EcContext->V[6];
    Context->V[7] = EcContext->V[7];
    Context->V[8] = EcContext->V[8];
    Context->V[9] = EcContext->V[9];
    Context->V[10] = EcContext->V[10];
    Context->V[11] = EcContext->V[11];
    Context->V[12] = EcContext->V[12];
    Context->V[13] = EcContext->V[13];
    Context->V[14] = EcContext->V[14];
    Context->V[15] = EcContext->V[15];
    RtlZeroMemory(&Context->V[16], sizeof(ARM64_NT_NEON128));

    Context->Fpcr = (EcContext->AMD64_MxCsr & 0x00000080) == 0;         // IM: Invalid Operation Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000200) == 0) << 1; // ZM: Divide-by-Zero Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000400) == 0) << 2; // OM: Overflow Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000800) == 0) << 3; // UM: Underflow Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00001000) == 0) << 4; // PM: Precision Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x00000040) << 5;        // DAZ: Denormals Are Zero Mask
    Context->Fpcr |= ((EcContext->AMD64_MxCsr & 0x00000100) == 0) << 7; // DM: Denormal Operation Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x00002000);             // FZ: Flush to Zero Mask
    Context->Fpcr |= (EcContext->AMD64_MxCsr & 0x0000C000);             // RC: Rounding Control
    Context->Fpcr <<= 8;

    Context->Fpsr = EcContext->AMD64_MxCsr & 1;            // IE: Invalid Operation Flag
    Context->Fpsr |= (EcContext->AMD64_MxCsr & 2) << 6;    // DE: Denormal Flag
    Context->Fpsr |= (EcContext->AMD64_MxCsr >> 1) & 0x1E; // ZE | OE | UE | PE: Zero, Overflow, Underflow, Precision Flags

    RtlZeroMemory(Context->Bcr, sizeof(Context->Bcr));
    RtlZeroMemory(Context->Bvr, sizeof(Context->Bvr));
    RtlZeroMemory(Context->Wcr, sizeof(Context->Wcr));
    RtlZeroMemory(Context->Wvr, sizeof(Context->Wvr));
}

// rev from ntdll!RtlNativeContextToEcContext (jxy-s)
VOID PhNativeContextToEcContext(
    _When_(InitializeEc, _Out_) _When_(!InitializeEc, _Inout_) PARM64EC_NT_CONTEXT EcContext,
    _In_ PCONTEXT Context,
    _In_ BOOLEAN InitializeEc
    )
{
    if (InitializeEc)
    {
        EcContext->ContextFlags = 0;

        //#define CONTEXT_AMD64   0x00100000L

        //#define CONTEXT_CONTROL         (CONTEXT_AMD64 | 0x00000001L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_CONTROL))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000001L));

        //#define CONTEXT_INTEGER         (CONTEXT_AMD64 | 0x00000002L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_INTEGER))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000002L));

        //#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x00000008L)
        if (BooleanFlagOn(Context->ContextFlags, CONTEXT_FLOATING_POINT))
            SetFlag(EcContext->ContextFlags, (0x00100000L | 0x00000008L));

        EcContext->AMD64_P1Home = 0;
        EcContext->AMD64_P2Home = 0;
        EcContext->AMD64_P3Home = 0;
        EcContext->AMD64_P4Home = 0;
        EcContext->AMD64_P5Home = 0;
        EcContext->AMD64_P6Home = 0;

        EcContext->AMD64_Dr0 = 0;
        EcContext->AMD64_Dr1 = 0;
        EcContext->AMD64_Dr3 = 0;
        EcContext->AMD64_Dr6 = 0;
        EcContext->AMD64_Dr7 = 0;

        EcContext->AMD64_MxCsr_copy = 0;

        EcContext->AMD64_SegCs = 0x0033;
        EcContext->AMD64_SegDs = 0x002B;
        EcContext->AMD64_SegEs = 0x002B;
        EcContext->AMD64_SegFs = 0x0053;
        EcContext->AMD64_SegGs = 0x002B;
        EcContext->AMD64_SegSs = 0x002B;

        EcContext->AMD64_ControlWord = 0;
        EcContext->AMD64_StatusWord = 0;
        EcContext->AMD64_TagWord = 0;
        EcContext->AMD64_Reserved1 = 0;
        EcContext->AMD64_ErrorOpcode = 0;
        EcContext->AMD64_ErrorOffset = 0;
        EcContext->AMD64_ErrorSelector = 0;
        EcContext->AMD64_Reserved2= 0;
        EcContext->AMD64_DataOffset = 0;
        EcContext->AMD64_DataSelector = 0;
        EcContext->AMD64_Reserved3 = 0;

        EcContext->AMD64_MxCsr = 0;
        EcContext->AMD64_MxCsr_Mask = 0;

        EcContext->AMD64_St0_Reserved1 = 0;
        EcContext->AMD64_St0_Reserved2 = 0;
        EcContext->AMD64_St1_Reserved1 = 0;
        EcContext->AMD64_St1_Reserved2 = 0;
        EcContext->AMD64_St2_Reserved1 = 0;
        EcContext->AMD64_St2_Reserved2 = 0;
        EcContext->AMD64_St3_Reserved1 = 0;
        EcContext->AMD64_St3_Reserved2 = 0;
        EcContext->AMD64_St4_Reserved1 = 0;
        EcContext->AMD64_St4_Reserved2 = 0;
        EcContext->AMD64_St5_Reserved1 = 0;
        EcContext->AMD64_St5_Reserved2 = 0;
        EcContext->AMD64_St6_Reserved1 = 0;
        EcContext->AMD64_St6_Reserved2 = 0;
        EcContext->AMD64_St7_Reserved1 = 0;
        EcContext->AMD64_St7_Reserved2 = 0;

        EcContext->AMD64_EFlags = 0x202; // IF(EI) | RESERVED_1(always set)
    }

    EcContext->ContextFlags &= 0x7ffffffff;
    SetFlag(EcContext->ContextFlags,
            Context->ContextFlags & (
                CONTEXT_EXCEPTION_ACTIVE |
                CONTEXT_SERVICE_ACTIVE |
                CONTEXT_EXCEPTION_REQUEST |
                CONTEXT_EXCEPTION_REPORTING
                ));

    EcContext->AMD64_MxCsr_copy &= 0xFFFF0000;
    EcContext->AMD64_MxCsr_copy |= (Context->Fpsr & 0x00000080) >> 6;         // IM: Invalid Operation Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00400000) >> 8;         // ZM: Divide-by-Zero Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x01000000) >> 9;         // OM: Overflow Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00080000) >> 7;         // UM: Underflow Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpcr & 0x00800000) >> 7;         // PM: Precision Mask
    EcContext->AMD64_MxCsr_copy |= (Context->Fpsr & 0x0000001E) << 1;         // Other Flags
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000100) == 0) << 7;  // DAZ: Denormals Are Zeros
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00008000) == 0) << 8;  // DM: Denormal Operation Mask
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000200) == 0) << 9;  // FZ: Flush to Zero
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000400) == 0) << 10; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00000800) == 0) << 11; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= ((Context->Fpcr & 0x00001000) == 0) << 12; // RC: Rounding Control
    EcContext->AMD64_MxCsr_copy |= Context->Fpsr & 0x00000001;

    EcContext->AMD64_EFlags &= 0xFFFFF63E;
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0x200000) >> 13);         // Overflow Flag
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0x10000000) >> 17);       // Direction Flag
    EcContext->AMD64_EFlags |= (((Context->Cpsr >> 5) & 0x1000000) >> 20); // Carry Flag
    EcContext->AMD64_EFlags |= ((Context->Cpsr & 0xC0FFFFFF) >> 20);       // Other Flags

    EcContext->Pc = Context->Pc;

    EcContext->X8 = Context->X8;
    EcContext->X0 = Context->X0;
    EcContext->X1 = Context->X1;
    EcContext->X27 = Context->X27;
    EcContext->Sp = Context->Sp;
    EcContext->Fp = Context->Fp;
    EcContext->X25 = Context->X25;
    EcContext->X26 = Context->X26;
    EcContext->X2 = Context->X2;
    EcContext->X3 = Context->X3;
    EcContext->X4 = Context->X4;
    EcContext->X5 = Context->X5;
    EcContext->X19 = Context->X19;
    EcContext->X20 = Context->X20;
    EcContext->X21 = Context->X21;
    EcContext->X22 = Context->X22;

    EcContext->Lr = Context->Lr;
    EcContext->X16_0 = LOWORD(Context->X16);
    EcContext->X6 = Context->X6;
    EcContext->X16_1 = HIWORD(Context->X16);
    EcContext->X7 = Context->X7;
    EcContext->X16_2 = LOWORD(Context->X16 >> 32);
    EcContext->X9 = Context->X9;
    EcContext->X16_3 = HIWORD(Context->X16 >> 32);
    EcContext->X10 = Context->X10;
    EcContext->X17_0 = LOWORD(Context->X17);
    EcContext->X11 = Context->X11;
    EcContext->X17_1 = HIWORD(Context->X17);
    EcContext->X12 = Context->X12;
    EcContext->X17_2 = LOWORD(Context->X17 >> 32);
    EcContext->X15 = Context->X15;
    EcContext->X17_3 = HIWORD(Context->X17 >> 32);

    EcContext->V[0] = Context->V[0];
    EcContext->V[1] = Context->V[1];
    EcContext->V[2] = Context->V[2];
    EcContext->V[3] = Context->V[3];
    EcContext->V[4] = Context->V[4];
    EcContext->V[5] = Context->V[5];
    EcContext->V[6] = Context->V[6];
    EcContext->V[7] = Context->V[7];
    EcContext->V[8] = Context->V[8];
    EcContext->V[9] = Context->V[9];
    EcContext->V[10] = Context->V[10];
    EcContext->V[11] = Context->V[11];
    EcContext->V[12] = Context->V[12];
    EcContext->V[13] = Context->V[13];
    EcContext->V[14] = Context->V[14];
    EcContext->V[15] = Context->V[15];
}

// rev from ntdll!RtlIsEcCode (jxy-s)
NTSTATUS PhIsEcCode(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID CodePointer,
    _Out_ PBOOLEAN IsEcCode
    )
{
    NTSTATUS status;
    PVOID pebBaseAddress;
    PVOID ecCodeBitMap;
    ULONG64 bitmap;

    *IsEcCode = FALSE;

    // hack (jxy-s)
    // 0x00007ffffffeffff = LdrpEcBitmapData.HighestAddress (MmHighestUserAddress)
    // 0x0000000000010000 = MM_LOWEST_USER_ADDRESS
    if ((ULONG_PTR)CodePointer > 0x00007ffffffeffff || (ULONG_PTR)CodePointer < 0x0000000000010000)
        return STATUS_INVALID_PARAMETER_2;

    if (!NT_SUCCESS(status = PhGetProcessPeb(ProcessHandle, &pebBaseAddress)))
        return status;

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        PTR_ADD_OFFSET(pebBaseAddress, FIELD_OFFSET(PEB, EcCodeBitMap)),
        &ecCodeBitMap,
        sizeof(PVOID),
        NULL
        )))
        return status;

    if (!ecCodeBitMap)
        return STATUS_INVALID_PARAMETER_1;

    // each byte of bitmap indexes 8*4K = 2^15 byte span
    ecCodeBitMap = PTR_ADD_OFFSET(ecCodeBitMap, (ULONG_PTR)CodePointer >> 15);

    if (!NT_SUCCESS(status = PhReadVirtualMemory(
        ProcessHandle,
        ecCodeBitMap,
        &bitmap,
        sizeof(ULONG64),
        NULL
        )))
        return status;

    // index to the 4k page within the 8*4K span
    bitmap >>= (((ULONG_PTR)CodePointer >> PAGE_SHIFT) & 7);

    // test the specific page
    if (bitmap & 1)
        *IsEcCode = TRUE;

    return STATUS_SUCCESS;
}
#endif

/**
 * Retrieves the Mark of the Web (MOTW) information from a file.
 *
 * The MOTW information resides in the Zone.Identifier alternate data
 * stream on a file. This routine attempts to open and read this alternate data
 * stream and optionally returns the MOTW information.
 *
 * \param[in] FileName - File name to retrieve the MOTS information from.
 * \param[out] ZoneId - Optionally receives the zone identifier for the file.
 * \param[out] ReferrerUrl - Optionally receives the referrer URL for the file.
 * Can be an empty string if no referrer URL is present.
 * \param[out] HostUrl - Optionally receives the host URL for the file. Can be
 * an empty string if no referrer URL is present.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhGetFileMotw(
    _In_ PCPH_STRINGREF FileName,
    _Out_opt_ PPH_MOTW_ZONE_ID ZoneId,
    _Out_opt_ PPH_STRING* ReferrerUrl,
    _Out_opt_ PPH_STRING* HostUrl
    )
{
    static const PH_STRINGREF zoneIdentifierStream = PH_STRINGREF_INIT(L":Zone.Identifier");
    static const PH_STRINGREF newLine = PH_STRINGREF_INIT(L"\r\n");
    static const PH_STRINGREF zoneIdKey = PH_STRINGREF_INIT(L"ZoneId");
    static const PH_STRINGREF referrerUrlKey = PH_STRINGREF_INIT(L"ReferrerUrl");
    static const PH_STRINGREF hostUrlKey = PH_STRINGREF_INIT(L"HostUrl");

    NTSTATUS status;
    PPH_STRING fileName;
    HANDLE fileHandle;
    ULONG zoneId = ULONG_MAX;
    PPH_STRING referrerUrl = NULL;
    PPH_STRING hostUrl = NULL;

    if (ZoneId)
        *ZoneId = PhMotwZoneIdUnknown;
    if (ReferrerUrl)
        *ReferrerUrl = NULL;
    if (HostUrl)
        *HostUrl = NULL;

    fileName = PhConcatStringRef2(FileName, &zoneIdentifierStream);

    if (NT_SUCCESS(status = PhCreateFile(
        &fileHandle,
        &fileName->sr,
        FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        PPH_STRING fileText;

        if (NT_SUCCESS(status = PhGetFileText(&fileText, fileHandle, TRUE)))
        {
            PH_STRINGREF line;
            PH_STRINGREF remaining;
            PH_STRINGREF key;
            PH_STRINGREF value;

            status = STATUS_NOT_FOUND;

            remaining = fileText->sr;

            while (remaining.Length)
            {
                PhSplitStringRefAtString(&remaining, &newLine, FALSE, &line, &remaining);
                PhSplitStringRefAtChar(&line, L'=', &key, &value);

                if (!value.Length)
                    continue;

                if (zoneId == ULONG_MAX && PhEqualStringRef(&key, &zoneIdKey, TRUE))
                {
                    zoneId = value.Buffer[0] - L'0';
                    if (zoneId > PhMotwZoneIdRestrictedSites)
                        zoneId = PhMotwZoneIdUnknown;
                }
                else if (!referrerUrl && PhEqualStringRef(&key, &referrerUrlKey, TRUE))
                {
                    referrerUrl = PhCreateString2(&value);
                }
                else if (!hostUrl && PhEqualStringRef(&key, &hostUrlKey, TRUE))
                {
                    hostUrl = PhCreateString2(&value);
                }

                if (zoneId != ULONG_MAX && referrerUrl && hostUrl)
                    break;
            }

            if (zoneId == ULONG_MAX)
            {
                status = STATUS_NOT_FOUND;
                PhClearReference(&referrerUrl);
                PhClearReference(&hostUrl);
            }
            else
            {
                status = STATUS_SUCCESS;

                if (!referrerUrl)
                    referrerUrl = PhReferenceEmptyString();
                if (!hostUrl)
                    hostUrl = PhReferenceEmptyString();

                if (ZoneId)
                    *ZoneId = zoneId;

                if (ReferrerUrl)
                    PhMoveReference(ReferrerUrl, referrerUrl);
                else
                    PhClearReference(&referrerUrl);

                if (HostUrl)
                    PhMoveReference(HostUrl, hostUrl);
                else
                    PhClearReference(&hostUrl);
            }
        }

        NtClose(fileHandle);
    }

    return status;
}
