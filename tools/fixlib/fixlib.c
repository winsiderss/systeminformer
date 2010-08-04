#include <ph.h>

#define ARG_OUTFILE 1

PPH_STRING inFile = NULL;
PPH_STRING outFile = NULL;

BOOLEAN NTAPI CommandLineCallback(
    __in_opt PPH_COMMAND_LINE_OPTION Option,
    __in_opt PPH_STRING Value,
    __in_opt PVOID Context
    )
{
    if (Option)
    {
        switch (Option->Id)
        {
        case ARG_OUTFILE:
            {
                PhSwapReference(&outFile, Value);
            }
            break;
        }
    }
    else
    {
        PhSwapReference(&inFile, Value);
    }

    return TRUE;
}

int __cdecl main(int argc, char *argv[])
{
    static PH_COMMAND_LINE_OPTION options[] =
    {
        { ARG_OUTFILE, L"o", MandatoryArgumentType }
    };
    NTSTATUS status;
    PH_STRINGREF commandLine;
    PH_MAPPED_ARCHIVE mappedArchive;
    PH_MAPPED_ARCHIVE_MEMBER member;
    PH_MAPPED_ARCHIVE_IMPORT_ENTRY entry;

    if (!NT_SUCCESS(PhInitializePhLib()))
        return 1;

    commandLine.us = NtCurrentPeb()->ProcessParameters->CommandLine;

    if (!PhParseCommandLine(
        &commandLine,
        options,
        sizeof(options) / sizeof(PH_COMMAND_LINE_OPTION),
        PH_COMMAND_LINE_IGNORE_FIRST_PART,
        CommandLineCallback,
        NULL
        ) || !inFile)
    {
        wprintf(L"Usage: fixlib [-o outfile] infile\n");
        return 1;
    }

    if (!outFile)
        outFile = inFile;

    CopyFile(inFile->Buffer, outFile->Buffer, FALSE);

    status = PhLoadMappedArchive(outFile->Buffer, NULL, FALSE, &mappedArchive);

    if (!NT_SUCCESS(status))
    {
        wprintf(L"Error: %s\n", PhGetStringOrDefault(PhGetNtMessage(status), L"unknown"));
        return status;
    }

    member = *(mappedArchive.LastStandardMember);

    while (NT_SUCCESS(PhGetNextMappedArchiveMember(&member, &member)))
    {
        if (NT_SUCCESS(PhGetMappedArchiveImportEntry(&member, &entry)))
        {
            IMPORT_OBJECT_HEADER *header;
            PWSTR type = L"unknown";

            switch (entry.NameType)
            {
            case IMPORT_OBJECT_ORDINAL:
                type = L"ordinal";
                break;
            case IMPORT_OBJECT_NAME:
                type = L"name";
                break;
            case IMPORT_OBJECT_NAME_NO_PREFIX:
                type = L"name-noprefix";
                break;
            case IMPORT_OBJECT_NAME_UNDECORATE:
                type = L"name-undecorate";
                break;
            }

            wprintf(L"%S: %S (%s)\n", entry.DllName, entry.Name, type);

            header = (IMPORT_OBJECT_HEADER *)member.Data;

            // Changes

            header->NameType = IMPORT_OBJECT_NAME_UNDECORATE;
        }
    }

    PhUnloadMappedArchive(&mappedArchive);
}
