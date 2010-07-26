/*
 * Process Hacker - 
 *   mapped library
 * 
 * Copyright (C) 2010 wj32
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

/*
 * This file contains functions to load and retrieve information for 
 * library/archive files (lib). The file format for archive files is explained 
 * in the PE/COFF specification located at:
 *
 * http://www.microsoft.com/whdc/system/platform/firmware/PECOFF.mspx
 */

#include <ph.h>

VOID PhpMappedArchiveProbe(
    __in PPH_MAPPED_ARCHIVE MappedArchive,
    __in PVOID Address,
    __in SIZE_T Length
    );

NTSTATUS PhpGetMappedArchiveMemberFromHeader(
    __in PPH_MAPPED_ARCHIVE MappedArchive,
    __in PIMAGE_ARCHIVE_MEMBER_HEADER Header,
    __out PPH_MAPPED_ARCHIVE_MEMBER Member
    );

NTSTATUS PhInitializeMappedArchive(
    __out PPH_MAPPED_ARCHIVE MappedArchive,
    __in PVOID ViewBase,
    __in SIZE_T Size
    )
{
    NTSTATUS status;
    PCHAR start;

    start = (PCHAR)ViewBase;

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
        (PIMAGE_ARCHIVE_MEMBER_HEADER)(start + IMAGE_ARCHIVE_START_SIZE),
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

    if (MappedArchive->SecondLinkerMember.Type != LinkerArchiveMemberType)
        return STATUS_INVALID_PARAMETER;

    // Longnames member
    // This member doesn't seem to be mandatory, contrary to the specification.
    // So we'll check if it's actually a longnames member, and if not, ignore 
    // it.

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
    __in_opt PWSTR FileName,
    __in_opt HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __out PPH_MAPPED_ARCHIVE MappedArchive
    )
{
    NTSTATUS status;

    status = PhMapViewOfEntireFile(
        FileName,
        FileHandle,
        ReadOnly,
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
            NtUnmapViewOfSection(NtCurrentProcess(), MappedArchive->ViewBase);
        }
    }

    return status;
}

NTSTATUS PhUnloadMappedArchive(
    __inout PPH_MAPPED_ARCHIVE MappedArchive
    )
{
    return NtUnmapViewOfSection(
        NtCurrentProcess(),
        MappedArchive->ViewBase
        );
}

VOID PhpMappedArchiveProbe(
    __in PPH_MAPPED_ARCHIVE MappedArchive,
    __in PVOID Address,
    __in SIZE_T Length
    )
{
    PhProbeAddress(Address, Length, MappedArchive->ViewBase, MappedArchive->Size, 1);
}

/**
 * Gets the next archive member.
 *
 * \param Member An archive member structure.
 * \param NextMember A variable which receives a structure 
 * describing the next archive member. This pointer may be 
 * the same as the pointer specified in \a Member.
 */
NTSTATUS PhGetNextMappedArchiveMember(
    __in PPH_MAPPED_ARCHIVE_MEMBER Member,
    __out PPH_MAPPED_ARCHIVE_MEMBER NextMember
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
    __in PPH_MAPPED_ARCHIVE MappedArchive,
    __in PIMAGE_ARCHIVE_MEMBER_HEADER Header,
    __out PPH_MAPPED_ARCHIVE_MEMBER Member
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

    if (!PhCopyUnicodeStringZFromAnsi(Header->Size, 10, integerString, 11, NULL))
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

    if (!PhCopyAnsiStringZ(Header->Name, 16, Member->NameBuffer, 20, NULL))
        return STATUS_INVALID_PARAMETER;

    Member->Name = Member->NameBuffer;

    slash = strchr(Member->NameBuffer, '/');

    if (!slash)
        return STATUS_INVALID_PARAMETER;

    // Special names:
    // * If the slash is the first character, then this is a linker member.
    // * If there is a slash after the slash which is a first character, then 
    //   this is the longnames member.
    // * If there are digits after the slash, then the real name is stored 
    //   in the longnames member.

    if (slash == Member->NameBuffer)
    {
        if (Member->NameBuffer[1] == '/')
        {
            // Longnames member. Set the name to "/".
            Member->NameBuffer[0] = '/';
            Member->NameBuffer[1] = 0;

            Member->Type = LongnamesArchiveMemberType;
        }
        else
        {
            // Linker member. Set the name to "".
            Member->NameBuffer[0] = 0;

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

            // Parse the offset and make sure it lies within the 
            // longnames member.

            if (!PhCopyUnicodeStringZFromAnsi(slash + 1, -1, integerString, 11, NULL))
                return STATUS_INVALID_PARAMETER;
            PhInitializeStringRef(&string, integerString);
            if (!PhStringToInteger64(&string, 10, &offset64))
                return STATUS_INVALID_PARAMETER;

            offset = (ULONG)offset64;

            if (offset >= MappedArchive->LongnamesMember.Size)
                return STATUS_INVALID_PARAMETER;

            // TODO: Probe the name.
            Member->Name = (PSTR)PTR_ADD_OFFSET(MappedArchive->LongnamesMember.Data, offset);
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
    __in PPH_MAPPED_ARCHIVE_MEMBER Member
    )
{
    PIMAGE_FILE_HEADER header;
    
    header = (PIMAGE_FILE_HEADER)Member->Data;

    return header->Machine != IMAGE_FILE_MACHINE_UNKNOWN;
}

NTSTATUS PhGetMappedArchiveImportEntry(
    __in PPH_MAPPED_ARCHIVE_MEMBER Member,
    __out PPH_MAPPED_ARCHIVE_IMPORT_ENTRY Entry
    )
{
    IMPORT_OBJECT_HEADER *importHeader;

    importHeader = (IMPORT_OBJECT_HEADER *)Member->Data;

    if (Member->Type != NormalArchiveMemberType)
        return STATUS_INVALID_PARAMETER;
    if (
        importHeader->Sig1 != IMAGE_FILE_MACHINE_UNKNOWN ||
        importHeader->Sig2 != IMPORT_OBJECT_HDR_SIG2
        )
        return STATUS_INVALID_PARAMETER;

    Entry->Type = (BYTE)importHeader->Type;
    Entry->NameType = (BYTE)importHeader->NameType;
    Entry->Machine = importHeader->Machine;

    // TODO: Probe the name.
    Entry->Name = (PSTR)PTR_ADD_OFFSET(importHeader, sizeof(IMPORT_OBJECT_HEADER));
    Entry->DllName = (PSTR)PTR_ADD_OFFSET(Entry->Name, strlen(Entry->Name) + 1);

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
