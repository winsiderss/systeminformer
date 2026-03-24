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
#include <phnative.h>

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

/*
 * Creates a directory object.
 *
 * \param DirectoryHandle A variable which receives a handle to the directory object.
 * \param DesiredAccess The desired access to the directory object.
 * \param RootDirectory A handle to the root directory of the object.
 * \param ObjectName The name of the directory object.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhCreateDirectoryObject(
    _Out_ PHANDLE DirectoryHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_opt_ PCPH_STRINGREF ObjectName
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (ObjectName)
    {
        if (!PhStringRefToUnicodeString(ObjectName, &objectName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&objectName, NULL, 0);
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtCreateDirectoryObject(
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
 * Creates a private namespace.
 *
 * \param NamespaceHandle Receives a handle to the created private namespace.
 * \param DesiredAccess Requested access mask.
 * \param RootDirectory Optional root directory handle for the namespace name.
 * \param ObjectName The name of the created private namespace.
 * \param BoundaryDescriptor Boundary descriptor created by PhCreateBoundaryDescriptor or similar.
 */
NTSTATUS PhCreatePrivateNamespace(
    _Out_ PHANDLE NamespaceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_opt_ PCPH_STRINGREF ObjectName,
    _In_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (ObjectName)
    {
        if (!PhStringRefToUnicodeString(ObjectName, &objectName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&objectName, NULL, 0);
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtCreatePrivateNamespace(
        NamespaceHandle,
        DesiredAccess,
        &objectAttributes,
        BoundaryDescriptor
        );

    return status;
}

/**
 * Opens a private namespace.
 *
 * \param NamespaceHandle Receives a handle to the opened private namespace.
 * \param DesiredAccess Requested access mask.
 * \param RootDirectory Optional root directory handle for the namespace name.
 * \param ObjectName Optional name of the namespace (stringref). If NULL, ObjectAttributes passed to NtOpenPrivateNamespace will be NULL.
 * \param BoundaryDescriptor Boundary descriptor used to identify the namespace.
 */
NTSTATUS PhOpenPrivateNamespace(
    _Out_ PHANDLE NamespaceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_opt_ PCPH_STRINGREF ObjectName,
    _In_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;

    if (ObjectName)
    {
        if (!PhStringRefToUnicodeString(ObjectName, &objectName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&objectName, NULL, 0);
    }

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    status = NtOpenPrivateNamespace(
        NamespaceHandle,
        DesiredAccess,
        &objectAttributes,
        BoundaryDescriptor
        );

    return status;
}


// rev from RtlCreateBoundaryDescriptor (dmex)
/**
 * Creates a new boundary descriptor with the specified name.
 *
 * \param Name The name of the private namespace.
 * \param Flags Flags for the boundary descriptor (e.g., BOUNDARY_DESCRIPTOR_ADD_APPCONTAINER_SID).
 * \return A pointer to the allocated boundary descriptor, or NULL on failure. 
 */
POBJECT_BOUNDARY_DESCRIPTOR PhCreateBoundaryDescriptor(
    _In_ PCPH_STRINGREF Name,
    _In_ ULONG Flags
    )
{
    PVOID buffer;
    ULONG rawSize;
    ULONG totalSize;
    ULONG nameLength;;
    POBJECT_BOUNDARY_DESCRIPTOR descriptor;
    POBJECT_BOUNDARY_ENTRY entry;

    //
    // Validate Flags. Only bit 0 (AddAppContainerSid) is allowed.
    //
    if (Flags & ~BOUNDARY_DESCRIPTOR_ADD_APPCONTAINER_SID)
        return NULL;

    //
    // Validate Name.
    // Must be present, non-empty, and have an even length (valid UTF-16).
    //
    if (!Name || Name->Length == 0 || (Name->Length & 1))
        return NULL;

    if (!NT_SUCCESS(RtlSizeTToULong(Name->Length, &nameLength)))
        return NULL;

    //
    // Calculate allocation size.
    // Layout: [Descriptor Header (16)] + [Entry Header (8)] + [Name Payload] + [Padding]
    // The total size must be aligned to 8 bytes.
    //
    rawSize = (ULONG)sizeof(OBJECT_BOUNDARY_DESCRIPTOR) + (ULONG)sizeof(OBJECT_BOUNDARY_ENTRY) + nameLength;
    totalSize = (ULONG)ALIGN_UP_BY(rawSize, sizeof(ULONG64));

    //
    // Allocate memory from the process heap.
    //
    buffer = PhAllocateZeroSafe(totalSize);
    if (!buffer) return NULL;

    //
    // Initialize Descriptor Header
    //
    descriptor = (POBJECT_BOUNDARY_DESCRIPTOR)buffer;
    descriptor->Version = 1;
    descriptor->Items = 1;
    descriptor->TotalSize = totalSize;
    descriptor->Flags = Flags;

    //
    // Initialize Name Entry
    // The entry starts immediately after the descriptor header.
    //
    entry = PTR_ADD_OFFSET(buffer, sizeof(OBJECT_BOUNDARY_DESCRIPTOR));
    entry->Type = BOUNDARY_ENTRY_TYPE_NAME;
    entry->Size = (ULONG)sizeof(OBJECT_BOUNDARY_ENTRY) + nameLength;

    //
    // Copy Name Payload
    // The payload follows the entry header.
    //
    RtlCopyMemory(
        PTR_ADD_OFFSET(entry, sizeof(OBJECT_BOUNDARY_ENTRY)),
        Name->Buffer,
        Name->Length
        );

    return buffer;
}

/**
 * Deletes a boundary descriptor.
 *
 * \param BoundaryDescriptor The boundary descriptor to delete.
 */
VOID PhDeleteBoundaryDescriptor(
    _In_ _Post_invalid_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor
    )
{
    if (BoundaryDescriptor)
    {
        PhFree(BoundaryDescriptor);
    }
}

// rev from RtlEnumerateBoundaryDescriptorEntries (dmex)
/**
 * Enumerates entries within a boundary descriptor.
 *
 * \param BoundaryDescriptor A pointer to the boundary descriptor to enumerate.
 * \param Callback A callback function invoked for each entry.
 * \param Context A user-defined context passed to the callback.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhEnumerateBoundaryDescriptorEntries(
    _In_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor,
    _In_opt_ POBJECT_BOUNDARY_ENUM_PROCEDURE Callback,
    _In_opt_ PVOID Context
    )
{
    ULONG totalSize;
    PBYTE bufferStart;
    PBYTE bufferEnd;
    PBYTE currentOffset;
    ULONG entryCount = 0;
    ULONG nameEntryCount = 0;
    ULONG ilEntryCount = 0;

    if (!BoundaryDescriptor)
        return STATUS_INVALID_PARAMETER;

    //
    // Validate Header
    // TotalSize must be at least the size of the descriptor header.
    //
    totalSize = BoundaryDescriptor->TotalSize;

    if (totalSize < sizeof(OBJECT_BOUNDARY_DESCRIPTOR))
        return STATUS_INVALID_BUFFER_SIZE;

    //
    // Version must be 1.
    //
    if (BoundaryDescriptor->Version != OBJECT_BOUNDARY_DESCRIPTOR_VERSION)
        return STATUS_INVALID_KERNEL_INFO_VERSION;

    //
    // Calculate Bounds
    //
    bufferStart = (PBYTE)BoundaryDescriptor;
    bufferEnd = (PBYTE)PTR_ADD_OFFSET(bufferStart, totalSize);

    //
    // Check for pointer overflow (wrap-around)
    //
    if ((ULONG_PTR)bufferEnd < (ULONG_PTR)bufferStart)
    {
        return STATUS_INVALID_PARAMETER;
    }

    currentOffset = (PBYTE)PTR_ADD_OFFSET(bufferStart, sizeof(OBJECT_BOUNDARY_DESCRIPTOR));

    while (TRUE)
    {
        POBJECT_BOUNDARY_ENTRY entry;
        ULONG entryType;
        ULONG entrySize;
        PBYTE entryEnd;

        //
        // Ensure there is enough space left for the entry header
        //
        if ((ULONG_PTR)PTR_ADD_OFFSET(currentOffset, sizeof(OBJECT_BOUNDARY_ENTRY)) > (ULONG_PTR)bufferEnd)
            break;

        entry = (POBJECT_BOUNDARY_ENTRY)currentOffset;
        entryType = entry->Type;
        entrySize = entry->Size;

        //
        // Validate Entry Size
        //
        if (entrySize < sizeof(OBJECT_BOUNDARY_ENTRY))
            return STATUS_INVALID_PARAMETER;

        //
        // Check overflow for current entry
        //
        if ((ULONG_PTR)PTR_ADD_OFFSET(currentOffset, entrySize) < (ULONG_PTR)currentOffset)
            return STATUS_INVALID_PARAMETER;

        //
        // Check if entry extends past the total descriptor size
        //
        if ((ULONG_PTR)PTR_ADD_OFFSET(currentOffset, entrySize) > (ULONG_PTR)bufferEnd)
            return STATUS_INVALID_PARAMETER;

        entryEnd = (PBYTE)PTR_ADD_OFFSET(currentOffset, entrySize);
        entryCount++;

        //
        // Validate Entry Type and Payload
        //
        switch (entryType)
        {
        case BOUNDARY_ENTRY_TYPE_NAME:
            {
                nameEntryCount++;

                if (nameEntryCount > 1)
                    return STATUS_DUPLICATE_NAME;
            }
            break;
        case BOUNDARY_ENTRY_TYPE_SID:
            {
                PSID sidBuffer;
                ULONG sidLength;

                sidLength = entrySize - sizeof(OBJECT_BOUNDARY_ENTRY);

                //
                // Check for minimum SID header size (Revision + SubAuthorityCount + IdentifierAuthority).
                // FIELD_OFFSET(SID, SubAuthority) is 8 bytes.
                //
                if (sidLength < 8)
                    return STATUS_INVALID_PARAMETER;

                sidBuffer = (PSID)PTR_ADD_OFFSET(currentOffset, sizeof(OBJECT_BOUNDARY_ENTRY));

                //
                // Check if the buffer is large enough for the number of sub-authorities
                // specified in the SID header (Sid[1] is SubAuthorityCount).
                //
                if (sidLength < PhLengthRequiredSid(*PhSubAuthorityCountSid(sidBuffer)))
                    return FALSE;

                //
                // Perform standard structural validation.
                //
                if (!PhValidSid(sidBuffer))
                    return FALSE;
            }
            break;
        case BOUNDARY_ENTRY_TYPE_IL:
            {
                PSID sidBuffer;
                ULONG sidLength;

                ilEntryCount++;

                if (ilEntryCount > 1)
                {
                    return STATUS_INVALID_PARAMETER;
                }

                sidLength = entrySize - sizeof(OBJECT_BOUNDARY_ENTRY);

                if (sidLength < 8)
                {
                    return STATUS_INVALID_PARAMETER;
                }

                sidBuffer = (PSID)PTR_ADD_OFFSET(currentOffset, sizeof(OBJECT_BOUNDARY_ENTRY));

                if (sidLength < PhLengthRequiredSid(*PhSubAuthorityCountSid(sidBuffer)))
                {
                    return STATUS_INVALID_PARAMETER;
                }

                if (!PhValidSid(sidBuffer))
                {
                    return STATUS_INVALID_PARAMETER;
                }
            }
            break;
        default:
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Invoke Callback
        //
        if (Callback)
        {
            if (!Callback(entry, Context))
                break;
        }

        //
        // Advance Pointer
        // Entries are aligned to 8-byte boundaries relative to the start.
        //
        currentOffset = (PBYTE)ALIGN_UP_BY((ULONG_PTR)entryEnd, sizeof(ULONG64));

        // Check if alignment pushed us past the end
        if ((ULONG_PTR)currentOffset > (ULONG_PTR)bufferEnd)
        {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Final Count Validation
    // The number of iterated entries must match the header item count.
    //
    if (BoundaryDescriptor->Items != entryCount)
        return STATUS_INVALID_PARAMETER;

    return STATUS_SUCCESS;
}

// rev from RtlAddSidToBoundaryDescriptor (dmex)
/**
 * Add a SID or Integrity Label entry to a boundary descriptor.
 *
 * \param BoundaryDescriptor A pointer to the boundary descriptor pointer.
 * \param Sid The SID to add.
 * \param IsIntegrityLabel TRUE if the SID is an integrity label, FALSE otherwise.
 * \return NTSTATUS Successful or errant status.
 */
static NTSTATUS PhAddSidToBoundaryDescriptorWorker(
    _Inout_ POBJECT_BOUNDARY_DESCRIPTOR *BoundaryDescriptor,
    _In_ PCSID Sid,
    _In_ BOOLEAN IsIntegrityLabel
    )
{
    NTSTATUS status;
    POBJECT_BOUNDARY_DESCRIPTOR oldDescriptor;
    POBJECT_BOUNDARY_DESCRIPTOR newDescriptor;
    POBJECT_BOUNDARY_ENTRY newEntry;
    ULONG sidLength;
    ULONG entrySize;
    ULONG oldTotalSize;
    ULONG newTotalSize;

    if (!BoundaryDescriptor || !*BoundaryDescriptor)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // 1. Validate the SID.
    //
    if (!PhValidSid(Sid))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // 2. Compute SID length and aligned entry size.
    //
    sidLength = PhLengthRequiredSid(*PhSubAuthorityCountSid(Sid));
    
    // EntrySize = Header (8) + Payload (SidLength) + Padding
    entrySize = (ULONG)ALIGN_UP_BY(sizeof(OBJECT_BOUNDARY_ENTRY) + sidLength, sizeof(ULONG64));
    oldDescriptor = *BoundaryDescriptor;
    oldTotalSize = oldDescriptor->TotalSize;

    //
    // 3. Calculate new total size and check for integer overflow.
    //
    newTotalSize = oldTotalSize + entrySize;

    if (newTotalSize < entrySize)
    {
        return STATUS_INTEGER_OVERFLOW; // 0xC0000173
    }

    //
    // 4. Allocate new buffer.
    // Using HEAP_ZERO_MEMORY (8) matches the observed behavior.
    //
    newDescriptor = PhAllocateZeroSafe(newTotalSize);
    if (!newDescriptor) return STATUS_INSUFFICIENT_RESOURCES;

    //
    // 5. Copy the old descriptor into the new buffer.
    //
    RtlCopyMemory(newDescriptor, oldDescriptor, oldTotalSize);

    //
    // 6. Update Header fields.
    //
    newDescriptor->TotalSize = newTotalSize;
    newDescriptor->Items++;

    //
    // 7. Initialize the new entry at the end of the previous data.
    //
    newEntry = (POBJECT_BOUNDARY_ENTRY)PTR_ADD_OFFSET(newDescriptor, oldTotalSize);
    newEntry->Type = IsIntegrityLabel ? BOUNDARY_ENTRY_TYPE_IL : BOUNDARY_ENTRY_TYPE_SID;
    newEntry->Size = entrySize;

    //
    // 8. Copy SID bytes into payload (Entry + HeaderSize).
    //
    RtlCopyMemory(
        PTR_ADD_OFFSET(newEntry, sizeof(OBJECT_BOUNDARY_ENTRY)),
        Sid,
        sidLength
        );

    //
    // 9. Validate the new descriptor logic.
    // This catches duplicate names (though not applicable here) or multiple ILs.
    //
    status = PhEnumerateBoundaryDescriptorEntries(newDescriptor, NULL, NULL);

    if (!NT_SUCCESS(status))
    {
        // Validation failed; free the new buffer and return the error.
        PhFree(newDescriptor);
        return status;
    }

    //
    // 10. Success. Free the old descriptor and update the caller's pointer.
    //
    PhFree(oldDescriptor);
    *BoundaryDescriptor = newDescriptor;

    return STATUS_SUCCESS;
}

/**
 * Adds a SID to the specified boundary descriptor.
 *
 * \param BoundaryDescriptor A pointer to the boundary descriptor pointer. 
 * \param RequiredSid The SID to add.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhAddSIDToBoundaryDescriptor(
    _Inout_ POBJECT_BOUNDARY_DESCRIPTOR *BoundaryDescriptor,
    _In_ PCSID RequiredSid
    )
{
    return PhAddSidToBoundaryDescriptorWorker(BoundaryDescriptor, RequiredSid, FALSE);
}

/**
 * Adds an Integrity Label to the specified boundary descriptor.
 *
 * \param BoundaryDescriptor A pointer to the boundary descriptor pointer.
 * \param IntegrityLabel The Integrity Label SID to add.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhAddIntegrityLabelToBoundaryDescriptor(
    _Inout_ POBJECT_BOUNDARY_DESCRIPTOR *BoundaryDescriptor,
    _In_ PCSID IntegrityLabel
    )
{
    return PhAddSidToBoundaryDescriptorWorker(BoundaryDescriptor, IntegrityLabel, TRUE);
}
