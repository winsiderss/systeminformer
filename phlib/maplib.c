/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2019
 *
 */

/*
 * This file contains functions to load and retrieve information for library/archive files (lib).
 * The file format for archive files is explained in the PE/COFF specification located at:
 *
 * http://www.microsoft.com/whdc/system/platform/firmware/PECOFF.mspx
 */

#include <ph.h>
#include <mapimg.h>

VOID PhpMappedArchiveProbe(
    _In_ PPH_MAPPED_ARCHIVE MappedArchive,
    _In_ PVOID Address,
    _In_ SIZE_T Length
    );

NTSTATUS PhpGetMappedArchiveMemberFromHeader(
    _In_ PPH_MAPPED_ARCHIVE MappedArchive,
    _In_ PIMAGE_ARCHIVE_MEMBER_HEADER Header,
    _Out_ PPH_MAPPED_ARCHIVE_MEMBER Member
    );

NTSTATUS PhInitializeMappedArchive(
    _Out_ PPH_MAPPED_ARCHIVE MappedArchive,
    _In_ PVOID ViewBase,
    _In_ SIZE_T Size
    )
{
    NTSTATUS status;
    PVOID start;

    start = ViewBase;

    memset(MappedArchive, 0, sizeof(PH_MAPPED_ARCHIVE));
    MappedArchive->ViewBase = ViewBase;
    MappedArchive->Size = Size;

    __try
    {
        // Verify the file signature.

        PhpMappedArchiveProbe(MappedArchive, start, IMAGE_ARCHIVE_START_SIZE);

        if (memcmp(start, IMAGE_ARCHIVE_START, IMAGE_ARCHIVE_START_SIZE) != 0)
            PhRaiseStatus(STATUS_INVALID_IMAGE_FORMAT);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // Get the members.
    // Note: the names are checked.

    // First linker member

    status = PhpGetMappedArchiveMemberFromHeader(
        MappedArchive,
        PTR_ADD_OFFSET(start, IMAGE_ARCHIVE_START_SIZE),
        &MappedArchive->FirstLinkerMember
        );

    if (!NT_SUCCESS(status))
        return status;

    if (MappedArchive->FirstLinkerMember.Type != LinkerArchiveMemberType)
        return STATUS_INVALID_PARAMETER;

    MappedArchive->FirstStandardMember = &MappedArchive->FirstLinkerMember;

    // Second linker member

    status = PhGetNextMappedArchiveMember(
        &MappedArchive->FirstLinkerMember,
        &MappedArchive->SecondLinkerMember
        );

    if (!NT_SUCCESS(status))
        return status;

    if (
        MappedArchive->SecondLinkerMember.Type != LinkerArchiveMemberType &&
        MappedArchive->SecondLinkerMember.Type != NormalArchiveMemberType // NormalArchiveMemberType might not be correct here but set by LLVM compiled libs (dmex)
        )
        return STATUS_INVALID_PARAMETER;

    // Longnames member
    // This member doesn't seem to be mandatory, contrary to the specification.
    // So we'll check if it's actually a longnames member, and if not, ignore it.

    status = PhGetNextMappedArchiveMember(
        &MappedArchive->SecondLinkerMember,
        &MappedArchive->LongnamesMember
        );

    if (
        NT_SUCCESS(status) &&
        MappedArchive->LongnamesMember.Type == LongnamesArchiveMemberType
        )
    {
        MappedArchive->HasLongnamesMember = TRUE;
        MappedArchive->LastStandardMember = &MappedArchive->LongnamesMember;
    }
    else
    {
        MappedArchive->LastStandardMember = &MappedArchive->SecondLinkerMember;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhLoadMappedArchive(
    _In_opt_ PWSTR FileName,
    _In_opt_ HANDLE FileHandle,
    _Out_ PPH_MAPPED_ARCHIVE MappedArchive
    )
{
    NTSTATUS status;

    status = PhMapViewOfEntireFile(
        FileName,
        FileHandle,
        &MappedArchive->ViewBase,
        &MappedArchive->Size
        );

    if (NT_SUCCESS(status))
    {
        status = PhInitializeMappedArchive(
            MappedArchive,
            MappedArchive->ViewBase,
            MappedArchive->Size
            );

        if (!NT_SUCCESS(status))
        {
            PhUnloadMappedArchive(MappedArchive);
        }
    }

    return status;
}

NTSTATUS PhUnloadMappedArchive(
    _Inout_ PPH_MAPPED_ARCHIVE MappedArchive
    )
{
    return NtUnmapViewOfSection(
        NtCurrentProcess(),
        MappedArchive->ViewBase
        );
}

VOID PhpMappedArchiveProbe(
    _In_ PPH_MAPPED_ARCHIVE MappedArchive,
    _In_ PVOID Address,
    _In_ SIZE_T Length
    )
{
    PhProbeAddress(Address, Length, MappedArchive->ViewBase, MappedArchive->Size, 1);
}

/**
 * Gets the next archive member.
 *
 * \param Member An archive member structure.
 * \param NextMember A variable which receives a structure describing the next archive member. This
 * pointer may be the same as the pointer specified in \a Member.
 */
NTSTATUS PhGetNextMappedArchiveMember(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member,
    _Out_ PPH_MAPPED_ARCHIVE_MEMBER NextMember
    )
{
    PIMAGE_ARCHIVE_MEMBER_HEADER nextHeader;

    nextHeader = (PIMAGE_ARCHIVE_MEMBER_HEADER)PTR_ADD_OFFSET(
        Member->Data,
        Member->Size
        );

    // 2 byte alignment.
    if ((ULONG_PTR)nextHeader & 0x1)
        nextHeader = (PIMAGE_ARCHIVE_MEMBER_HEADER)PTR_ADD_OFFSET(nextHeader, 1);

    return PhpGetMappedArchiveMemberFromHeader(
        Member->MappedArchive,
        nextHeader,
        NextMember
        );
}

NTSTATUS PhpGetMappedArchiveMemberFromHeader(
    _In_ PPH_MAPPED_ARCHIVE MappedArchive,
    _In_ PIMAGE_ARCHIVE_MEMBER_HEADER Header,
    _Out_ PPH_MAPPED_ARCHIVE_MEMBER Member
    )
{
    WCHAR integerString[11];
    ULONG64 size;
    PH_STRINGREF string;
    PWSTR digit;
    PSTR slash;

    if ((ULONG_PTR)Header >= (ULONG_PTR)MappedArchive->ViewBase + MappedArchive->Size)
        return STATUS_NO_MORE_ENTRIES;

    __try
    {
        PhpMappedArchiveProbe(MappedArchive, Header, sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    Member->MappedArchive = MappedArchive;
    Member->Header = Header;
    Member->Data = PTR_ADD_OFFSET(Header, sizeof(IMAGE_ARCHIVE_MEMBER_HEADER));
    Member->Type = NormalArchiveMemberType;

    // Read the size string, terminate it after the last digit and parse it.

    if (!PhCopyStringZFromBytes(Header->Size, 10, integerString, 11, NULL))
        return STATUS_INVALID_PARAMETER;

    string.Buffer = integerString;
    string.Length = 0;
    digit = string.Buffer;

    while (iswdigit(*digit++))
        string.Length += sizeof(WCHAR);

    if (!PhStringToInteger64(&string, 10, &size))
        return STATUS_INVALID_PARAMETER;

    Member->Size = (ULONG)size;

    __try
    {
        PhpMappedArchiveProbe(MappedArchive, Member->Data, Member->Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // Parse the name.

    if (!PhCopyBytesZ(Header->Name, 16, Member->NameBuffer, 20, NULL))
        return STATUS_INVALID_PARAMETER;

    Member->Name = Member->NameBuffer;

    slash = strchr(Member->NameBuffer, '/');

    if (!slash)
        return STATUS_INVALID_PARAMETER;

    // Special names:
    // * If the slash is the first character, then this is a linker member.
    // * If there is a slash after the slash which is a first character, then this is the longnames
    //   member.
    // * If there are digits after the slash, then the real name is stored in the longnames member.

    if (slash == Member->NameBuffer)
    {
        if (Member->NameBuffer[1] == '/')
        {
            // Longnames member. Set the name to "/".
            Member->NameBuffer[0] = '/';
            Member->NameBuffer[1] = ANSI_NULL;

            Member->Type = LongnamesArchiveMemberType;
        }
        else
        {
            // Linker member. Set the name to "".
            Member->NameBuffer[0] = ANSI_NULL;

            Member->Type = LinkerArchiveMemberType;
        }
    }
    else
    {
        if (isdigit(slash[1]))
        {
            PSTR digita;
            ULONG64 offset64;
            ULONG offset;

            // The name is stored in the longnames member.
            // Note: we make sure we have the longnames member first.

            if (!MappedArchive->LongnamesMember.Header)
                return STATUS_INVALID_PARAMETER;

            // Find the last digit and null terminate the string there.

            digita = slash + 2;

            while (isdigit(*digita))
                digita++;

            *digita = 0;

            // Parse the offset and make sure it lies within the longnames member.

            if (!PhCopyStringZFromBytes(slash + 1, -1, integerString, 11, NULL))
                return STATUS_INVALID_PARAMETER;
            PhInitializeStringRefLongHint(&string, integerString);
            if (!PhStringToInteger64(&string, 10, &offset64))
                return STATUS_INVALID_PARAMETER;

            offset = (ULONG)offset64;

            if (offset >= MappedArchive->LongnamesMember.Size)
                return STATUS_INVALID_PARAMETER;

            // TODO: Probe the name.
            Member->Name = PTR_ADD_OFFSET(MappedArchive->LongnamesMember.Data, offset);
        }
        else
        {
            // Null terminate the string.
            slash[0] = 0;
        }
    }

    return STATUS_SUCCESS;
}

BOOLEAN PhIsMappedArchiveMemberShortFormat(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member
    )
{
    PIMAGE_FILE_HEADER header;

    header = (PIMAGE_FILE_HEADER)Member->Data;

    return header->Machine != IMAGE_FILE_MACHINE_UNKNOWN;
}

NTSTATUS PhGetMappedArchiveImportEntry(
    _In_ PPH_MAPPED_ARCHIVE_MEMBER Member,
    _Out_ PPH_MAPPED_ARCHIVE_IMPORT_ENTRY Entry
    )
{
    IMPORT_OBJECT_HEADER *importHeader;

    importHeader = (IMPORT_OBJECT_HEADER *)Member->Data;

    if (
        importHeader->Sig1 != IMAGE_FILE_MACHINE_UNKNOWN ||
        importHeader->Sig2 != IMPORT_OBJECT_HDR_SIG2
        )
        return STATUS_INVALID_PARAMETER;

    Entry->Type = (BYTE)importHeader->Type;
    Entry->NameType = (BYTE)importHeader->NameType;
    Entry->Machine = importHeader->Machine;

    // TODO: Probe the name.
    Entry->Name = PTR_ADD_OFFSET(importHeader, sizeof(IMPORT_OBJECT_HEADER));
    Entry->DllName = PTR_ADD_OFFSET(Entry->Name, strlen(Entry->Name) + 1);

    // Ordinal/NameHint are union'ed, so these statements are exactly the same.
    // It's there in case this changes in the future.
    if (Entry->NameType == IMPORT_OBJECT_ORDINAL)
    {
        Entry->Ordinal = importHeader->Ordinal;
    }
    else
    {
        Entry->NameHint = importHeader->Hint;
    }

    return STATUS_SUCCESS;
}
